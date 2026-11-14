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

/* Declarations the style layer stores verbatim, because they are not expressions: the
   animator and the effects layer parse them from the computed values. */
bool isRawDeclaration (const Identifier& name)
{
    static const Array<Identifier> rawNames { Identifier ("transition"), Identifier ("animation"), Identifier ("cursor"), Identifier ("effect"), Identifier ("drawing") };

    return rawNames.contains (name);
}

bool isDescendantOfOrSame (const Component& component, const Component& ancestor)
{
    for (auto* c = &component; c != nullptr; c = c->getParentComponent())
        if (c == &ancestor)
            return true;

    return false;
}

} // namespace

//==============================================================================

/* One declaration that won the cascade for a component or its descendants. */
struct StyleContext::ResolvedDeclaration
{
    String value;
    Identifier qualifier;
    int specificity = 0;
    int order = 0;
    bool direct = false;

    bool beats (const ResolvedDeclaration& other) const noexcept
    {
        if (specificity != other.specificity)
            return specificity > other.specificity;

        if (direct != other.direct)
            return direct;

        return order >= other.order;
    }
};

/* One compiled declaration value, kept per component so that frames reuse the program. */
struct StyleContext::CompiledValue
{
    String source;
    std::unique_ptr<PropertyExpression> expression;
};

/* Everything the context keeps for one styled component. */
struct StyleContext::ComponentCache
{
    WeakReference<Component> component;
    std::map<Identifier, std::unique_ptr<CompiledValue>> values;
    NamedValueSet savedState;

    String drawingName;
    ComponentStyle::Ptr drawingStyle;
    ComponentStyle::Ptr savedStyle;
};

//==============================================================================

StyleContext::StyleContext (Component& rootToUse, StyleSheet::Ptr sheetToUse)
    : root (rootToUse)
    , styleSheet (std::move (sheetToUse))
{
    if (auto theme = ApplicationTheme::getGlobalTheme())
        lastPaletteGeneration = theme->getPaletteGeneration();

    markDirty (root);
}

StyleContext::~StyleContext()
{
    caches.clear();
}

//==============================================================================

void StyleContext::setStyleSheet (StyleSheet::Ptr newStyleSheet)
{
    if (styleSheet == newStyleSheet)
        return;

    clearSubtree (root);
    caches.clear();
    diagnostics.clear();

    styleSheet = std::move (newStyleSheet);

    markDirty (root);
}

//==============================================================================

void StyleContext::markDirty (Component& component)
{
    for (const auto& dirty : dirtyComponents)
        if (dirty.get() == &component)
            return;

    dirtyComponents.add (WeakReference<Component> (&component));
}

void StyleContext::propertyChanged (Component& component, const Identifier& name)
{
    if (styleSheet == nullptr || ! styleSheet->watchesProperty (name))
        return;

    markDirty (component);
}

//==============================================================================

void StyleContext::refresh (double)
{
    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        if (const auto generation = theme->getPaletteGeneration(); generation != lastPaletteGeneration)
        {
            lastPaletteGeneration = generation;
            markPaletteDependentDirty (root);
        }
    }

    restyleNow();
}

void StyleContext::restyleNow()
{
    if (dirtyComponents.isEmpty())
        return;

    auto pending = std::move (dirtyComponents);
    dirtyComponents.clear();
    diagnostics.clear();

    for (const auto& dirty : pending)
    {
        auto* component = dirty.get();

        if (component == nullptr || ! isDescendantOfOrSame (*component, root))
            continue;

        restyleSubtree (*component, collectInherited (*component));
    }

    // Drops the caches of the components that went away
    for (auto it = caches.begin(); it != caches.end();)
        it = it->second->component.get() == nullptr ? caches.erase (it) : std::next (it);
}

//==============================================================================

void StyleContext::restyleSubtree (Component& component, const DeclarationMap& inherited)
{
    DeclarationMap inheritedForChildren;
    applyToComponent (component, inherited, inheritedForChildren);

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            restyleSubtree (*child, inheritedForChildren);
}

StyleContext::DeclarationMap StyleContext::collectInherited (Component& component) const
{
    std::vector<Component*> chain;

    for (auto* c = component.getParentComponent(); c != nullptr; c = c->getParentComponent())
    {
        chain.push_back (c);

        if (c == &root)
            break;
    }

    DeclarationMap inherited;

    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
    {
        DeclarationMap direct, qualified;
        collectDeclarations (**it, direct, qualified);

        for (auto& [name, declaration] : qualified)
        {
            const auto existing = inherited.find (name);

            if (existing == inherited.end() || declaration.beats (existing->second))
                inherited[name] = declaration;
        }
    }

    return inherited;
}

void StyleContext::collectDeclarations (Component& component, DeclarationMap& direct, DeclarationMap& qualified) const
{
    if (styleSheet == nullptr)
        return;

    for (const auto& rule : styleSheet->getRules())
    {
        if (! rule.selector.matches (component, &root))
            continue;

        const auto specificity = rule.selector.getSpecificity();

        for (const auto& declaration : rule.declarations)
        {
            ResolvedDeclaration resolved;
            resolved.value = declaration.value;
            resolved.qualifier = declaration.qualifier;
            resolved.specificity = specificity;
            resolved.order = rule.order;
            resolved.direct = true;

            auto& target = declaration.qualifier.isValid() ? qualified : direct;
            const auto key = declaration.qualifier.isValid()
                               ? Identifier (declaration.qualifier.toString() + "." + declaration.name.toString())
                               : declaration.name;

            const auto existing = target.find (key);

            if (existing == target.end() || resolved.beats (existing->second))
                target[key] = std::move (resolved);
        }
    }
}

void StyleContext::applyToComponent (Component& component, const DeclarationMap& inherited, DeclarationMap& inheritedForChildren)
{
    DeclarationMap direct, qualified;
    collectDeclarations (component, direct, qualified);

    // The qualified declarations of the ancestors keep flowing down, the closer ones winning
    inheritedForChildren = inherited;

    for (auto& [name, declaration] : qualified)
    {
        const auto existing = inheritedForChildren.find (name);

        if (existing == inheritedForChildren.end() || declaration.beats (existing->second))
            inheritedForChildren[name] = declaration;
    }

    // An inherited qualified declaration applies when the component is of the type it names
    const auto& typeInfo = component.getTypeInfo();

    for (const auto& [name, declaration] : inheritedForChildren)
    {
        if (! typeInfo.isSubtypeOf (declaration.qualifier))
            continue;

        const Identifier propertyName (name.toString().fromLastOccurrenceOf (".", false, false));
        const auto existing = direct.find (propertyName);

        if (existing != direct.end() && existing->second.beats (declaration))
            continue;

        auto copy = declaration;
        copy.direct = false;
        direct[propertyName] = std::move (copy);
    }

    auto* styleState = component.styleState.get();
    NamedValueSet previous;

    if (styleState != nullptr)
        previous = styleState->getComputedValues();

    const auto affectsParentLayout = [&typeInfo] (const Identifier& name)
    {
        const auto* descriptor = typeInfo.findProperty (name);
        return descriptor != nullptr && descriptor->affectsParentLayout;
    };

    const auto notifyParentLayout = [&component]
    {
        if (auto* parent = component.getParentComponent())
            parent->childLayoutChanged (component);
    };

    if (direct.empty())
    {
        if (styleState == nullptr)
            return;

        restoreAllStateBacked (component);
        restoreDrawing (component);

        bool changed = styleState->clearComputedValues();
        changed |= styleState->setStylePalette (std::nullopt);
        styleState->setPaletteDependent (false);
        caches.erase (&component);

        if (changed)
            component.repaint();

        for (int i = 0; i < previous.size(); ++i)
        {
            if (affectsParentLayout (previous.getName (i)))
            {
                notifyParentLayout();
                break;
            }
        }

        return;
    }

    auto& state = component.getOrCreateStyleState();
    static const Identifier paletteDeclaration ("palette");
    static const Identifier drawingDeclaration ("drawing");

    bool changed = false;
    bool layoutChanged = false;
    bool paletteDependent = false;
    Array<Identifier> written;

    // The palette is resolved first, so that the values of this component read the right roles
    if (const auto it = direct.find (paletteDeclaration); it != direct.end())
    {
        const auto name = it->second.value.unquoted().trim();
        const auto* palette = styleSheet != nullptr ? styleSheet->findPalette (name) : nullptr;

        if (palette == nullptr)
            diagnostics.add ("Unknown palette '" + name + "'");
        else
            changed |= state.setStylePalette (*palette);
    }
    else
    {
        changed |= state.setStylePalette (std::nullopt);
    }

    for (const auto& [name, declaration] : direct)
    {
        if (name == paletteDeclaration)
            continue;

        const auto* descriptor = typeInfo.findProperty (name);
        const auto storageId = descriptor != nullptr ? descriptor->getStorageKey() : name;

        if (descriptor != nullptr && ! descriptor->isStyleable)
        {
            diagnostics.add ("The property '" + name.toString() + "' can't be styled");
            continue;
        }

        var value;

        if (isRawDeclaration (name))
        {
            value = declaration.value.unquoted();
        }
        else
        {
            bool valuePaletteDependent = false;

            if (! evaluate (component, storageId, descriptor, declaration.value, value, valuePaletteDependent))
                continue;

            paletteDependent |= valuePaletteDependent;
        }

        written.add (storageId);

        if (state.setComputedValue (storageId, value))
        {
            changed = true;
            layoutChanged |= descriptor != nullptr && descriptor->affectsParentLayout;
        }

        // A property the component holds itself can only be styled through its setter
        if (descriptor != nullptr && descriptor->isStateBacked() && descriptor->setter != nullptr)
        {
            auto& cache = getCache (component);

            if (! cache.savedState.contains (storageId))
                cache.savedState.set (storageId, descriptor->getter (component));

            if (! descriptor->getter (component).equals (value))
                descriptor->setter (component, value);
        }
    }

    // Removes what the previous restyle computed and no rule matches anymore
    for (int i = previous.size(); --i >= 0;)
    {
        const auto name = previous.getName (i);

        if (! written.contains (name))
        {
            restoreStateBacked (component, name);

            if (state.removeComputedValue (name))
            {
                changed = true;
                layoutChanged |= affectsParentLayout (name);
            }
        }
    }

    state.setPaletteDependent (paletteDependent);

    if (const auto it = direct.find (drawingDeclaration); it != direct.end())
        installDrawing (component, it->second.value.unquoted().trim());
    else
        restoreDrawing (component);

    if (changed)
    {
        component.styleChanged();
        component.repaint();
    }

    if (layoutChanged)
        notifyParentLayout();
}

//==============================================================================

bool StyleContext::evaluate (Component& component,
                             const Identifier& storageId,
                             const ComponentProperty* descriptor,
                             const String& valueText,
                             var& result,
                             bool& paletteDependent)
{
    auto& entry = getCache (component).values[storageId];

    // A value that failed to compile is kept, so that it isn't compiled and reported again until it changes
    if (entry == nullptr || entry->source != valueText)
    {
        entry = std::make_unique<CompiledValue>();
        entry->source = valueText;

        std::optional<ComponentProperty::Type> type;

        if (descriptor != nullptr)
            type = descriptor->type;

        auto compiled = PropertyExpression::compile (component, valueText, type);

        if (compiled.failed())
        {
            diagnostics.add (storageId.toString() + ": " + compiled.getErrorMessage());
            return false;
        }

        entry->expression = std::move (compiled.getReference());
    }

    if (entry->expression == nullptr)
        return false;

    paletteDependent = entry->expression->readsPalette();
    result = entry->expression->evaluate();
    return ! result.isVoid();
}

void StyleContext::restoreStateBacked (Component& component, const Identifier& storageId)
{
    const auto it = caches.find (&component);

    if (it == caches.end())
        return;

    auto& saved = it->second->savedState;

    if (const auto* original = saved.getVarPointer (storageId))
    {
        const auto& typeInfo = component.getTypeInfo();
        const auto* descriptor = typeInfo.findProperty (storageId);

        if (descriptor == nullptr)
            descriptor = typeInfo.findPropertyByStorageKey (storageId);

        if (descriptor != nullptr && descriptor->setter != nullptr)
            descriptor->setter (component, *original);

        saved.remove (storageId);
    }
}

void StyleContext::restoreAllStateBacked (Component& component)
{
    const auto it = caches.find (&component);

    if (it == caches.end())
        return;

    auto& saved = it->second->savedState;

    for (int i = saved.size(); --i >= 0;)
        restoreStateBacked (component, saved.getName (i));
}

void StyleContext::installDrawing (Component& component, const String& name)
{
    auto& cache = getCache (component);

    if (cache.drawingName == name && component.getStyle() == cache.drawingStyle)
        return;

    const auto drawing = styleSheet != nullptr ? styleSheet->findDrawing (name) : DataTree();

    if (! drawing.isValid())
    {
        diagnostics.add ("Unknown drawing '" + name + "'");
        restoreDrawing (component);
        return;
    }

    auto created = DrawingStyle::create (component, drawing);

    if (created.failed())
    {
        diagnostics.add ("drawing " + name + ": " + created.getErrorMessage());
        restoreDrawing (component);
        return;
    }

    // Only the style the component had before the first drawing is remembered, for restoring it
    if (cache.drawingStyle == nullptr || component.getStyle() != cache.drawingStyle)
        cache.savedStyle = component.getStyle();

    cache.drawingName = name;
    cache.drawingStyle = created.getReference();
    component.setStyle (cache.drawingStyle);
}

void StyleContext::restoreDrawing (Component& component)
{
    const auto it = caches.find (&component);

    if (it == caches.end() || it->second->drawingStyle == nullptr)
        return;

    auto& cache = *it->second;

    // The application may have replaced the drawing in the meantime, and its choice wins
    if (component.getStyle() == cache.drawingStyle)
        component.setStyle (std::move (cache.savedStyle));

    cache.drawingName = {};
    cache.drawingStyle = nullptr;
    cache.savedStyle = nullptr;
}

StyleContext::ComponentCache& StyleContext::getCache (Component& component)
{
    auto& cache = caches[&component];

    if (cache == nullptr)
    {
        cache = std::make_unique<ComponentCache>();
        cache->component = WeakReference<Component> (&component);
    }

    return *cache;
}

//==============================================================================

void StyleContext::detach()
{
    clearSubtree (root);
    caches.clear();
    dirtyComponents.clear();
    diagnostics.clear();
}

void StyleContext::clearSubtree (Component& component)
{
    if (auto* state = component.styleState.get())
    {
        restoreAllStateBacked (component);
        restoreDrawing (component);

        bool changed = state->clearComputedValues();
        changed |= state->setStylePalette (std::nullopt);
        state->setPaletteDependent (false);

        if (changed)
            component.repaint();
    }

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            clearSubtree (*child);
}

void StyleContext::markPaletteDependentDirty (Component& component)
{
    if (auto* state = component.styleState.get(); state != nullptr && state->isPaletteDependent())
        markDirty (component);

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            markPaletteDependentDirty (*child);
}

} // namespace yup
