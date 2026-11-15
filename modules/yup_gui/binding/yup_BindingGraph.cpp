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

struct BindingGraph::Binding
{
    Binding (Component& targetToUse, const Identifier& propertyToUse, std::unique_ptr<PropertyExpression> expressionToUse)
        : target (&targetToUse)
        , property (propertyToUse)
        , expression (std::move (expressionToUse))
        , dependencies (expression->getDependencies())
    {
    }

    bool dependsOn (const Component& component, const Identifier& name) const noexcept
    {
        return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.component == &component && dependency.property == name;
        });
    }

    bool dependsOn (const BindingProvider& provider, const Identifier& name) const noexcept
    {
        return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.provider == &provider && dependency.property == name;
        });
    }

    void evaluate()
    {
        auto* component = target.get();
        if (component == nullptr)
            return;

        const auto value = expression->evaluate();

        if (component->getProperty (property) != value)
            component->setProperty (property, value);
    }

    WeakReference<Component> target;
    Identifier property;
    std::unique_ptr<PropertyExpression> expression;
    std::vector<DrawingScope::Dependency> dependencies;
    uint64 flushEpoch = 0;
    int evaluations = 0;
};

//==============================================================================

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

void BindingGraph::setModelResolver (DrawingScope::ModelResolver resolver)
{
    modelResolver = std::move (resolver);
}

//==============================================================================

Result BindingGraph::bind (Component& target,
                           const Identifier& property,
                           const String& text,
                           std::optional<ComponentProperty::Type> type,
                           bool* isLive)
{
    auto compiled = PropertyExpression::compile (target, text, type, componentResolver, modelResolver);

    if (compiled.failed())
        return Result::fail (compiled.getErrorMessage());

    unbind (target, property);

    auto expression = std::move (compiled.getReference());
    const bool live = expression->hasInputs();

    if (isLive != nullptr)
        *isLive = live;

    if (! live)
    {
        const auto value = expression->evaluate();

        if (value.isVoid())
            return Result::fail ("the value can't be evaluated");

        target.setProperty (property, value);
        return Result::ok();
    }

    auto binding = std::make_unique<Binding> (target, property, std::move (expression));
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
        if (bindings[index]->target.get() == &target && bindings[index]->property == property)
        {
            removeBinding (index);
            return true;
        }
    }

    return false;
}

void BindingGraph::forgetProvider (BindingProvider& provider)
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
    {
        const auto& dependencies = bindings[static_cast<size_t> (index)]->dependencies;

        const bool readsIt = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.provider == &provider;
        });

        if (readsIt)
            removeBinding (index);
    }

    if (const auto it = observedProviders.find (&provider); it != observedProviders.end())
    {
        observedProviders.erase (it);
        provider.removeListener (this);
    }
}

void BindingGraph::clear()
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
        removeBinding (index);

    for (int index = static_cast<int> (links.size()) - 1; index >= 0; --index)
        removeLink (index);

    jassert (observed.empty());
    jassert (observedProviders.empty());

    dirty.clear();
}

//==============================================================================

void BindingGraph::link (Component& target, const Identifier& property, Component& source, const Identifier& sourceProperty)
{
    auto newLink = std::make_shared<Link>();
    newLink->first = &target;
    newLink->firstProperty = property;
    newLink->second = &source;
    newLink->secondProperty = sourceProperty;

    observe (target);
    observe (source);
    links.push_back (newLink);

    const auto value = source.findProperty (sourceProperty);

    if (! value.isVoid() && target.findProperty (property) != value)
    {
        const ScopedValueSetter<bool> syncing (newLink->isSyncing, true);
        target.setProperty (property, value);
    }
}

bool BindingGraph::unlink (Component& component, const Identifier& property)
{
    bool removed = false;

    for (int index = static_cast<int> (links.size()) - 1; index >= 0; --index)
    {
        if (links[static_cast<size_t> (index)]->touches (component, property))
        {
            removeLink (index);
            removed = true;
        }
    }

    return removed;
}

//==============================================================================

bool BindingGraph::hasBinding (const Component& target, const Identifier& property) const noexcept
{
    return std::any_of (bindings.begin(), bindings.end(), [&] (const auto& binding)
    {
        return binding->target.get() == &target && binding->property == property;
    });
}

void BindingGraph::refreshAll()
{
    for (const auto& binding : bindings)
        dirty.push_back (binding.get());

    if (! isFlushing)
        flush();
}

void BindingGraph::refreshPaletteBindings()
{
    for (const auto& binding : bindings)
        if (binding->expression->readsPalette())
            dirty.push_back (binding.get());

    if (! isFlushing && ! dirty.empty())
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

        if (binding != writingBinding && binding->target.get() == &component && binding->property == property)
        {
            removeBinding (index);
            break;
        }
    }

    invalidate (component, property);
    syncLinks (component, property);
}

void BindingGraph::componentBeingDeleted (Component& component)
{
    for (int index = static_cast<int> (links.size()) - 1; index >= 0; --index)
    {
        const auto& candidate = *links[static_cast<size_t> (index)];

        if (candidate.first == &component || candidate.second == &component)
            removeLink (index);
    }

    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
    {
        auto* binding = bindings[index].get();

        if (binding->target.get() == &component)
        {
            removeBinding (index);
            continue;
        }

        const auto readsIt = std::any_of (binding->dependencies.begin(), binding->dependencies.end(), [&] (const auto& dependency)
        {
            return dependency.component == &component;
        });

        if (readsIt)
            removeBinding (index);
    }
}

void BindingGraph::bindingValueChanged (BindingProvider& provider, const Identifier& name)
{
    invalidate (provider, name);
}

void BindingGraph::bindingProviderBeingDeleted (BindingProvider& provider)
{
    forgetProvider (provider);
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

    if (it == observed.end() || --it->second > 0)
        return;

    observed.erase (it);
    component.removeComponentListener (this);
}

void BindingGraph::observe (BindingProvider& provider)
{
    if (++observedProviders[&provider] == 1)
        provider.addListener (this);
}

void BindingGraph::unobserve (BindingProvider& provider)
{
    const auto it = observedProviders.find (&provider);

    if (it == observedProviders.end() || --it->second > 0)
        return;

    observedProviders.erase (it);
    provider.removeListener (this);
}

void BindingGraph::addObservedOf (const Binding& binding)
{
    if (auto* target = binding.target.get())
        observe (*target);

    for (const auto& dependency : binding.dependencies)
    {
        if (dependency.component != nullptr)
            observe (*dependency.component);
        else if (dependency.provider != nullptr)
            observe (*dependency.provider);
    }
}

void BindingGraph::removeObservedOf (const Binding& binding)
{
    if (auto* target = binding.target.get())
        unobserve (*target);

    for (const auto& dependency : binding.dependencies)
    {
        if (dependency.component != nullptr)
            unobserve (*dependency.component);
        else if (dependency.provider != nullptr)
            unobserve (*dependency.provider);
    }
}

void BindingGraph::removeBinding (int index)
{
    auto* binding = bindings[static_cast<size_t> (index)].get();

    removeObservedOf (*binding);

    for (auto& pending : dirty)
        if (pending == binding)
            pending = nullptr;

    if (writingBinding == binding)
        writingBinding = nullptr;

    bindings.erase (bindings.begin() + index);
}

void BindingGraph::removeLink (int index)
{
    const auto removed = links[static_cast<size_t> (index)];

    links.erase (links.begin() + index);

    unobserve (*removed->first);
    unobserve (*removed->second);
}

void BindingGraph::syncLinks (Component& component, const Identifier& property)
{
    for (size_t index = 0; index < links.size(); ++index)
    {
        // Held, so that a link removed while its value is written stays valid until it is done
        const auto current = links[index];

        if (current->isSyncing || ! current->touches (component, property))
            continue;

        const auto isFirst = current->first == &component && current->firstProperty == property;
        auto& other = isFirst ? *current->second : *current->first;
        const auto& otherProperty = isFirst ? current->secondProperty : current->firstProperty;

        const auto value = component.findProperty (property);

        if (value.isVoid() || other.findProperty (otherProperty) == value)
            continue;

        const ScopedValueSetter<bool> syncing (current->isSyncing, true);
        other.setProperty (otherProperty, value);
    }
}

void BindingGraph::invalidate (const Component& component, const Identifier& property)
{
    for (const auto& binding : bindings)
        if (binding->dependsOn (component, property))
            markDirty (*binding);
}

void BindingGraph::invalidate (const BindingProvider& provider, const Identifier& property)
{
    for (const auto& binding : bindings)
        if (binding->dependsOn (provider, property))
            markDirty (*binding);
}

void BindingGraph::markDirty (Binding& binding)
{
    if (std::find (dirty.begin(), dirty.end(), &binding) == dirty.end())
        dirty.push_back (&binding);

    if (! isFlushing)
        flush();
}

void BindingGraph::flush()
{
    const ScopedValueSetter<bool> flushing (isFlushing, true);
    ++currentFlushEpoch;

    for (size_t index = 0; index < dirty.size(); ++index)
    {
        auto* binding = dirty[index];

        if (binding == nullptr)
            continue;

        dirty[index] = nullptr;

        if (binding->flushEpoch != currentFlushEpoch)
        {
            binding->flushEpoch = currentFlushEpoch;
            binding->evaluations = 0;
        }

        if (++binding->evaluations > maxEvaluationsPerFlush)
        {
            const auto message = "Binding cycle detected while evaluating \"" + binding->expression->getSource() + "\", the binding was removed";
            diagnostics.add (message);

            auto* target = binding->target.get();
            const auto property = binding->property;

            for (int bindingIndex = 0; bindingIndex < static_cast<int> (bindings.size()); ++bindingIndex)
            {
                if (bindings[static_cast<size_t> (bindingIndex)].get() == binding)
                {
                    removeBinding (bindingIndex);
                    break;
                }
            }

            if (onDiagnostic != nullptr)
                onDiagnostic (target, property, message);

            continue;
        }

        const ScopedValueSetter<Binding*> writing (writingBinding, binding);
        binding->evaluate();
    }

    dirty.clear();
}

} // namespace yup
