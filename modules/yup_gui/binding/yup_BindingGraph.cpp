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

namespace
{

/* Adds a value to those an index keeps for a property, once however many times in a row it is added. */
template <class Index, class Key, class Value>
void addToPropertyIndex (Index& index, const Key& key, const Value& value)
{
    auto& values = index[key];

    if (values.empty() || values.back() != value)
        values.push_back (value);
}

/* Removes a value from those an index keeps for a property, and the property with the last of them. */
template <class Index, class Key, class Value>
void removeFromPropertyIndex (Index& index, const Key& key, const Value& value)
{
    const auto found = index.find (key);

    if (found == index.end())
        return;

    auto& values = found->second;

    // Searched from the end: what goes is most often what came last, as when everything is cleared
    if (const auto position = std::find (values.rbegin(), values.rend(), value); position != values.rend())
        values.erase (std::next (position).base());

    if (values.empty())
        index.erase (found);
}

} // namespace

struct BindingGraph::Binding
{
    Binding (Component& targetToUse, const Identifier& propertyToUse, std::unique_ptr<PropertyExpression> expressionToUse)
        : target (&targetToUse)
        , targetAddress (&targetToUse)
        , property (propertyToUse)
        , expression (std::move (expressionToUse))
        , dependencies (expression->getDependencies())
    {
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

    void rememberValue()
    {
        if (const auto* component = target.get())
            valueLeft = component->getProperty (property);
    }

    bool wasOverwritten (const Component& component) const
    {
        return component.getProperty (property) != valueLeft;
    }

    WeakReference<Component> target;
    const Component* const targetAddress; // What the binding is indexed by, never read through: the target may be gone
    Identifier property;
    std::unique_ptr<PropertyExpression> expression;
    std::vector<DrawingScope::Dependency> dependencies;
    var valueLeft;
    uint64 flushEpoch = 0;
    int evaluations = 0;
};

//==============================================================================

BindingGraph::BindingGraph()
{
    observer.onChanged = [this] (Component& component, const Identifier& property)
    {
        handlePropertyChanged (component, property);
    };

    observer.onDeleted = [this] (Component& component)
    {
        handleComponentDeleted (component);
    };
}

BindingGraph::~BindingGraph()
{
    clear();
}

//==============================================================================

Result BindingGraph::bind (Component& target,
                           const Identifier& property,
                           const String& text,
                           std::optional<ComponentProperty::Type> type,
                           DrawingScope::ComponentResolver resolver,
                           bool* isLive,
                           Component* locals)
{
    auto compiled = PropertyExpression::compile (target, text, type, std::move (resolver), locals, baseDirectory);

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
    addToIndex (*binding);
    bindings.push_back (std::move (binding));

    markDirty (*bindingToEvaluate);

    return Result::ok();
}

bool BindingGraph::unbind (Component& target, const Identifier& property)
{
    const auto found = bindingsByTarget.find (PropertyKey { &target, property });

    if (found == bindingsByTarget.end())
        return false;

    removeBinding (*found->second);
    return true;
}

void BindingGraph::clear()
{
    for (int index = static_cast<int> (bindings.size()) - 1; index >= 0; --index)
        removeBinding (index);

    for (int index = static_cast<int> (links.size()) - 1; index >= 0; --index)
        removeLink (index);

    jassert (observer.isEmpty());
    jassert (bindingsByTarget.empty() && bindingsByInput.empty() && linksByProperty.empty());

    dirty.clear();
}

//==============================================================================

void BindingGraph::link (Component& target, const Identifier& property, Component& source, const Identifier& sourceProperty, int sourceIndex)
{
    auto newLink = std::make_shared<Link>();
    newLink->first = &target;
    newLink->firstProperty = property;
    newLink->second = &source;
    newLink->secondProperty = sourceProperty;
    newLink->secondIndex = sourceIndex;

    observer.observe (target);
    observer.observe (source);
    links.push_back (newLink);
    addToPropertyIndex (linksByProperty, PropertyKey { &target, property }, newLink);
    addToPropertyIndex (linksByProperty, PropertyKey { &source, sourceProperty }, newLink);

    const auto value = readLinkSide (*newLink, false);

    if (! value.isVoid() && readLinkSide (*newLink, true) != value)
    {
        const ScopedValueSetter<bool> syncing (newLink->isSyncing, true);
        writeLinkSide (*newLink, true, value);
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

std::optional<BindingGraph::ModelProperty> BindingGraph::findLinkedModel (const Component& component, const Identifier& property) const
{
    // Breadth first, so that the model the fewest links away is the one found
    std::vector<PropertyKey> reached { { &component, property } };

    for (size_t index = 0; index < reached.size(); ++index)
    {
        const auto current = reached[index];

        if (auto* model = dynamic_cast<BindingModel*> (const_cast<Component*> (current.component)))
            return ModelProperty { model, current.property };

        const auto found = linksByProperty.find (current);

        if (found == linksByProperty.end())
            continue;

        for (const auto& candidate : found->second)
        {
            if (candidate->secondIndex >= 0)
                continue;

            const auto isFirst = candidate->first == current.component && candidate->firstProperty == current.property;
            const PropertyKey other { isFirst ? candidate->second : candidate->first, isFirst ? candidate->secondProperty : candidate->firstProperty };

            if (std::find (reached.begin(), reached.end(), other) == reached.end())
                reached.push_back (other);
        }
    }

    return std::nullopt;
}

//==============================================================================

bool BindingGraph::hasBinding (const Component& target, const Identifier& property) const noexcept
{
    return bindingsByTarget.find (PropertyKey { &target, property }) != bindingsByTarget.end();
}

std::vector<BindingGraph::BindingInfo> BindingGraph::getAllBindings() const
{
    std::vector<BindingInfo> result;
    result.reserve (bindings.size());

    for (const auto& binding : bindings)
        result.push_back ({ binding->target.get(), binding->property, binding->dependencies });

    return result;
}

void BindingGraph::refreshAll(){
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

void BindingGraph::handlePropertyChanged (Component& component, const Identifier& property)
{
    const PropertyKey changed { &component, property };

    if (const auto found = bindingsByTarget.find (changed);
        found != bindingsByTarget.end() && found->second != writingBinding && found->second->wasOverwritten (component))
    {
        removeBinding (*found->second);
    }

    invalidate (changed);
    syncLinks (changed);
}

void BindingGraph::handleComponentDeleted (Component& component)
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

//==============================================================================

void BindingGraph::addObservedOf (const Binding& binding)
{
    if (auto* target = binding.target.get())
        observer.observe (*target);

    observer.observe (binding.dependencies);
}

void BindingGraph::removeObservedOf (const Binding& binding)
{
    if (auto* target = binding.target.get())
        observer.unobserve (*target);

    observer.unobserve (binding.dependencies);
}

void BindingGraph::addToIndex (Binding& binding)
{
    bindingsByTarget[PropertyKey { binding.targetAddress, binding.property }] = &binding;

    for (const auto& dependency : binding.dependencies)
        addToPropertyIndex (bindingsByInput, PropertyKey { dependency.component, dependency.property }, &binding);

    // What reads a role of the palette follows the palette its target is drawn with
    if (binding.expression->readsPalette())
        addToPropertyIndex (bindingsByInput, PropertyKey { binding.targetAddress, paletteProperty }, &binding);
}

void BindingGraph::removeFromIndex (const Binding& binding)
{
    if (const auto found = bindingsByTarget.find (PropertyKey { binding.targetAddress, binding.property });
        found != bindingsByTarget.end() && found->second == &binding)
    {
        bindingsByTarget.erase (found);
    }

    for (const auto& dependency : binding.dependencies)
        removeFromPropertyIndex (bindingsByInput, PropertyKey { dependency.component, dependency.property }, &binding);

    if (binding.expression->readsPalette())
        removeFromPropertyIndex (bindingsByInput, PropertyKey { binding.targetAddress, paletteProperty }, &binding);
}

void BindingGraph::removeBinding (int index)
{
    auto* binding = bindings[static_cast<size_t> (index)].get();

    removeObservedOf (*binding);
    removeFromIndex (*binding);

    for (auto& pending : dirty)
        if (pending == binding)
            pending = nullptr;

    if (writingBinding == binding)
        writingBinding = nullptr;

    bindings.erase (bindings.begin() + index);
}

void BindingGraph::removeBinding (const Binding& binding)
{
    const auto position = std::find_if (bindings.begin(), bindings.end(), [&binding] (const auto& candidate)
    {
        return candidate.get() == &binding;
    });

    if (position != bindings.end())
        removeBinding (static_cast<int> (position - bindings.begin()));
}

void BindingGraph::removeLink (int index)
{
    const auto removed = links[static_cast<size_t> (index)];

    links.erase (links.begin() + index);
    removeFromPropertyIndex (linksByProperty, PropertyKey { removed->first, removed->firstProperty }, removed);
    removeFromPropertyIndex (linksByProperty, PropertyKey { removed->second, removed->secondProperty }, removed);

    observer.unobserve (*removed->first);
    observer.unobserve (*removed->second);
}

var BindingGraph::readLinked (const Component& component, const Identifier& property)
{
    const auto given = component.getProperty (property);

    return given.isVoid() ? component.findProperty (property) : given;
}

void BindingGraph::syncLinks (const PropertyKey& changed)
{
    // Looked up again at each step: writing a value can add and remove links, those of this property too
    for (size_t index = 0;; ++index)
    {
        const auto found = linksByProperty.find (changed);

        if (found == linksByProperty.end() || index >= found->second.size())
            return;

        // Held, so that a link removed while its value is written stays valid until it is done
        const auto current = found->second[index];

        if (current->isSyncing)
            continue;

        const auto isFirst = current->first == changed.component && current->firstProperty == changed.property;
        const auto value = readLinkSide (*current, isFirst);

        if (value.isVoid() || readLinkSide (*current, ! isFirst) == value)
            continue;

        const ScopedValueSetter<bool> syncing (current->isSyncing, true);
        writeLinkSide (*current, ! isFirst, value);
    }
}

var BindingGraph::readLinkSide (const Link& link, bool isFirst)
{
    const auto value = readLinked (isFirst ? *link.first : *link.second, isFirst ? link.firstProperty : link.secondProperty);

    if (isFirst || link.secondIndex < 0)
        return value;

    const auto* list = value.getArray();

    return list != nullptr && isPositiveAndBelow (link.secondIndex, list->size()) ? list->getReference (link.secondIndex) : var();
}

void BindingGraph::writeLinkSide (const Link& link, bool isFirst, const var& value)
{
    auto& component = isFirst ? *link.first : *link.second;
    const auto& property = isFirst ? link.firstProperty : link.secondProperty;

    if (isFirst || link.secondIndex < 0)
    {
        component.setProperty (property, value);
        return;
    }

    // An element is written by giving the list again with that element replaced
    const auto current = readLinked (component, property);
    const auto* list = current.getArray();

    if (list == nullptr || ! isPositiveAndBelow (link.secondIndex, list->size()))
        return;

    Array<var> changed (*list);
    changed.set (link.secondIndex, value);

    component.setProperty (property, var (changed));
}

void BindingGraph::invalidate (const PropertyKey& changed)
{
    // Queue everything before flushing: a flush can remove bindings (cycles) while iterating
    if (const auto found = bindingsByInput.find (changed); found != bindingsByInput.end())
    {
        for (auto* binding : found->second)
            if (std::find (dirty.begin(), dirty.end(), binding) == dirty.end())
                dirty.push_back (binding);
    }

    if (! isFlushing && ! dirty.empty())
        flush();
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

            removeBinding (*binding);

            if (onDiagnostic != nullptr)
                onDiagnostic (target, property, message);

            continue;
        }

        const ScopedValueSetter<Binding*> writing (writingBinding, binding);
        binding->evaluate();

        // What the write set off may have removed the binding
        if (writingBinding == binding)
            binding->rememberValue();
    }

    dirty.clear();
}

} // namespace yup
