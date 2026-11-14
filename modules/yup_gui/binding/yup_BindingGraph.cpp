/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

BindingGraph::BindingGraph() = default;

BindingGraph::~BindingGraph()
{
    clear();
}

//==============================================================================

void BindingGraph::setComponentResolver (DrawingScope::ComponentResolver resolver)
{
    componentResolver = std::move (resolver);
}

//==============================================================================

Result BindingGraph::bind (Component& target,
                           const Identifier& property,
                           const String& text,
                           std::optional<ComponentProperty::Type> type)
{
    auto compiled = PropertyExpression::compile (target, text, type, componentResolver);

    if (compiled.failed())
        return Result::fail (compiled.getErrorMessage());

    unbind (target, property);

    auto expression = std::move (compiled.getReference());

    if (! expression->hasInputs())
    {
        const auto value = expression->evaluate();

        if (value.isVoid())
            return Result::fail ("the value can't be evaluated");

        target.setProperty (property, value);
        return Result::ok();
    }

    auto binding = std::make_unique<PropertyBinding> (target, property, std::move (expression));
    auto* bindingToEvaluate = binding.get();

    addObservedOf (*binding);
    bindings.push_back (std::move (binding));

    markDirty (*bindingToEvaluate);

    return Result::ok();
}

bool BindingGraph::unbind (Component& target, const Identifier& property)
{
    for (int index = 0; index < static_cast<int> (bindings.size()); ++index)
    {
        if (bindings[index]->getTarget() == &target && bindings[index]->getProperty() == property)
        {
            removeBinding (index);
            return true;
        }
    }

    return false;
}

void BindingGraph::unbindAll (Component& target)
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
    {
        if (bindings[index]->getTarget() == &target)
            removeBinding (index);
    }
}

void BindingGraph::clear()
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
        removeBinding (index);

    jassert (observed.empty());

    dirty.clear();
    evaluations.clear();
}

//==============================================================================

const PropertyBinding* BindingGraph::findBinding (const Component& target, const Identifier& property) const
{
    for (const auto& binding : bindings)
    {
        if (binding->getTarget() == &target && binding->getProperty() == property)
            return binding.get();
    }

    return nullptr;
}

void BindingGraph::refreshAll()
{
    for (const auto& binding : bindings)
        dirty.push_back (binding.get());

    if (! isFlushing)
        flush();
}

void BindingGraph::clearDiagnostics()
{
    diagnostics.clear();
}

//==============================================================================

void BindingGraph::componentPropertyChanged (Component& component, const Identifier& property)
{
    for (int index = 0; index < static_cast<int> (bindings.size()); ++index)
    {
        auto* binding = bindings[index].get();

        if (binding != writingBinding && binding->getTarget() == &component && binding->getProperty() == property)
        {
            removeBinding (index);
            break;
        }
    }

    invalidate (component, property);
}

void BindingGraph::componentMoved (Component& component)
{
    static const Identifier xId ("x");
    static const Identifier yId ("y");

    invalidate (component, xId);
    invalidate (component, yId);
}

void BindingGraph::componentResized (Component& component)
{
    static const Identifier widthId ("width");
    static const Identifier heightId ("height");

    invalidate (component, widthId);
    invalidate (component, heightId);
}

void BindingGraph::componentBeingDeleted (Component& component)
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
    {
        auto* binding = bindings[index].get();

        if (binding->getTarget() == &component)
        {
            removeBinding (index);
            continue;
        }

        const auto& dependencies = binding->getDependencies();

        const auto readsIt = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
                                          {
            return dependency.component == &component;
        });

        if (readsIt)
            removeBinding (index);
    }
}

//==============================================================================

void BindingGraph::observe (Component& component)
{
    if (++observed[&component] == 1)
        component.addComponentListener (this);
}

void BindingGraph::unobserve (Component& component)
{
    const auto it = observed.find (&component);

    if (it == observed.end())
        return;

    if (--it->second > 0)
        return;

    observed.erase (it);
    component.removeComponentListener (this);
}

void BindingGraph::addObservedOf (const PropertyBinding& binding)
{
    if (auto* target = binding.getTarget())
        observe (*target);

    for (const auto& dependency : binding.getDependencies())
        observe (*dependency.component);
}

void BindingGraph::removeObservedOf (const PropertyBinding& binding)
{
    if (auto* target = binding.getTarget())
        unobserve (*target);

    for (const auto& dependency : binding.getDependencies())
        unobserve (*dependency.component);
}

void BindingGraph::removeBinding (int index)
{
    auto* binding = bindings[static_cast<size_t> (index)].get();

    removeObservedOf (*binding);

    for (auto& pending : dirty)
    {
        if (pending == binding)
            pending = nullptr;
    }

    evaluations.erase (binding);

    if (writingBinding == binding)
        writingBinding = nullptr;

    bindings.erase (bindings.begin() + index);
}

void BindingGraph::invalidate (const Component& component, const Identifier& property)
{
    for (const auto& binding : bindings)
    {
        if (binding->dependsOn (component, property))
            markDirty (*binding);
    }
}

void BindingGraph::markDirty (PropertyBinding& binding)
{
    const auto isPending = std::find (dirty.begin(), dirty.end(), &binding) != dirty.end();

    if (! isPending)
        dirty.push_back (&binding);

    if (! isFlushing)
        flush();
}

void BindingGraph::flush()
{
    const ScopedValueSetter<bool> flushing (isFlushing, true);

    evaluations.clear();

    for (size_t index = 0; index < dirty.size(); ++index)
    {
        auto* binding = dirty[index];

        if (binding == nullptr)
            continue;

        dirty[index] = nullptr;

        if (++evaluations[binding] > maxEvaluationsPerFlush)
        {
            diagnostics.add ("Binding cycle detected while evaluating \"" + binding->getSource() + "\", the binding was removed");

            for (int bindingIndex = 0; bindingIndex < static_cast<int> (bindings.size()); ++bindingIndex)
            {
                if (bindings[static_cast<size_t> (bindingIndex)].get() == binding)
                {
                    removeBinding (bindingIndex);
                    break;
                }
            }

            continue;
        }

        const ScopedValueSetter<PropertyBinding*> writing (writingBinding, binding);

        binding->evaluate();
    }

    dirty.clear();
    evaluations.clear();
}

} // namespace yup
