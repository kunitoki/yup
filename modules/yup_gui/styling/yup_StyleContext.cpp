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
    static const Array<Identifier> rawNames { Identifier ("transition"), Identifier ("animation"), Identifier ("cursor"), Identifier ("effect"), Identifier ("shadow"), Identifier ("drawing") };

    return rawNames.contains (name);
}

bool isManagedByRoot (const Component& component, const Component& root)
{
    for (auto* c = &component; c != nullptr; c = c->getParentComponent())
    {
        if (c == &root)
            return true;

        if (c->getStyleContext() != nullptr)
            return false;
    }

    return false;
}

bool hasAncestorInSet (Component& component, const Component& root, const std::set<Component*>& candidates)
{
    for (auto* parent = component.getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
    {
        if (candidates.find (parent) != candidates.end())
            return true;

        if (parent == &root || parent->getStyleContext() != nullptr)
            break;
    }

    return false;
}

bool isShadowLength (const String& token)
{
    const auto digits = token.startsWithChar ('-') ? token.substring (1) : token;
    return digits.containsAnyOf ("0123456789") && digits.containsOnly ("0123456789.");
}

std::optional<MouseCursor::Type> findStyleCursor (const String& name)
{
    static const std::pair<const char*, MouseCursor::Type> cursors[] = {
        { "none", MouseCursor::None },
        { "default", MouseCursor::Default },
        { "arrow", MouseCursor::Arrow },
        { "text", MouseCursor::Text },
        { "wait", MouseCursor::Wait },
        { "waitArrow", MouseCursor::WaitArrow },
        { "hand", MouseCursor::Hand },
        { "crosshair", MouseCursor::Crosshair },
        { "crossbones", MouseCursor::Crossbones },
        { "resizeLeftRight", MouseCursor::ResizeLeftRight },
        { "resizeUpDown", MouseCursor::ResizeUpDown },
        { "resizeTopLeftRightBottom", MouseCursor::ResizeTopLeftRightBottom },
        { "resizeBottomLeftRightTop", MouseCursor::ResizeBottomLeftRightTop },
        { "resizeAll", MouseCursor::ResizeAll }
    };

    for (const auto& [cursorName, type] : cursors)
        if (name == cursorName)
            return type;

    return std::nullopt;
}

/* The type an animated value interpolates as, guessed from the value when no descriptor types it. */
ComponentProperty::Type inferAnimatedType (const ComponentProperty* descriptor, const var& value)
{
    if (descriptor != nullptr && descriptor->type != ComponentProperty::Type::any)
        return descriptor->type;

    if (value.isInt64())
        return ComponentProperty::Type::color;

    if (value.isInt())
        return ComponentProperty::Type::integer;

    if (value.isBool())
        return ComponentProperty::Type::boolean;

    if (value.isString())
        return ComponentProperty::Type::string;

    return ComponentProperty::Type::number;
}

} // namespace

//==============================================================================

/* One declaration that won the cascade for a component or its descendants. */
struct StyleContext::ResolvedDeclaration
{
    String value;
    Identifier qualifier;
    DataTree element;
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

struct StyleContext::BoundDependency
{
    Component* component = nullptr;
    BindingProvider* provider = nullptr;
    Identifier property;
};

/* One compiled declaration value, kept per component so that frames reuse the program. */
struct StyleContext::CompiledValue
{
    String source;
    std::optional<ComponentProperty::Type> type;
    std::unique_ptr<PropertyExpression> expression;
    std::vector<BoundDependency> dependencies;
};

/* Everything the context keeps for a component it visited, styled or not, so later restyles transition. */
struct StyleContext::ComponentCache
{
    WeakReference<Component> component;
    std::map<Identifier, std::unique_ptr<CompiledValue>> values;
    NamedValueSet savedState;

    String drawingName;
    DrawingStyle::Ptr drawingStyle;
    ComponentStyle::Ptr savedStyle;

    String effectName;
    ComponentEffect::Ptr effect;
    ComponentEffect::Ptr savedEffect;

    std::optional<MouseCursor> cursor;
    MouseCursor savedCursor;

    std::optional<EdgeSpacing<float>> paintMargin;
    EdgeSpacing<float> savedPaintMargin;
};

/* A transition the new style of a component declares, with the value shown before the restyle. */
struct StyleContext::PendingTransition
{
    StyleAnimator::Transition transition;
    const ComponentProperty* descriptor = nullptr;
    Identifier storageId;
    var from;
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
    clearAllCaches();
}

//==============================================================================

void StyleContext::setStyleSheet (StyleSheet::Ptr newStyleSheet)
{
    if (styleSheet == newStyleSheet)
        return;

    clearSubtree (root);
    clearAllCaches();
    animator.clear();
    diagnostics.clear();

    styleSheet = std::move (newStyleSheet);

    markDirty (root);
}

void StyleContext::setComponentResolver (ComponentResolver resolver)
{
    componentResolver = std::move (resolver);

    for (auto& [ptr, cache] : caches)
    {
        if (cache == nullptr)
            continue;

        clearCacheValues (*cache);

        if (auto* comp = cache->component.get())
            restoreDrawing (*comp);
        else
        {
            cache->drawingName = {};
            cache->drawingStyle = nullptr;
            cache->savedStyle = nullptr;
        }
    }

    markDirty (root);
}

void StyleContext::setModelResolver (ModelResolver resolver)
{
    modelResolver = std::move (resolver);

    for (auto& [ptr, cache] : caches)
    {
        if (cache == nullptr)
            continue;

        clearCacheValues (*cache);

        if (auto* comp = cache->component.get())
            restoreDrawing (*comp);
        else
        {
            cache->drawingName = {};
            cache->drawingStyle = nullptr;
            cache->savedStyle = nullptr;
        }
    }

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
    if (styleSheet != nullptr && styleSheet->watchesProperty (name))
        markDirty (component);

    invalidateDependencies (&component, nullptr, name);
}

//==============================================================================

void StyleContext::refresh (double elapsedSeconds)
{
    clock += elapsedSeconds;

    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        if (const auto generation = theme->getPaletteGeneration(); generation != lastPaletteGeneration)
        {
            lastPaletteGeneration = generation;
            markPaletteDependentDirty (root);
        }
    }

    restyleNow();

    if (animator.isAnimating())
    {
        animator.advance (clock, [this] (Component& component, const Identifier& storageId, const var* value)
        {
            showAnimatedValue (component, storageId, value);
        });
    }
}

void StyleContext::restyleNow()
{
    if (dirtyComponents.isEmpty())
        return;

    diagnostics.clear();

    for (int pass = 0; pass < 4 && ! dirtyComponents.isEmpty(); ++pass)
    {
        auto pending = std::move (dirtyComponents);
        dirtyComponents.clear();

        std::vector<Component*> ordered;
        std::set<Component*> candidates;

        for (const auto& dirty : pending)
        {
            auto* component = dirty.get();

            if (component == nullptr || ! isManagedByRoot (*component, root))
                continue;

            if (candidates.insert (component).second)
                ordered.push_back (component);
        }

        for (auto* component : ordered)
        {
            if (hasAncestorInSet (*component, root, candidates))
                continue;

            restyleSubtree (*component, collectInherited (*component));
        }
    }

    // Drops the caches of the components that went away
    for (auto it = caches.begin(); it != caches.end();)
    {
        if (it->second == nullptr || it->second->component.get() != it->first)
        {
            if (it->second != nullptr)
                clearCacheValues (*it->second);

            it = caches.erase (it);
        }
        else
        {
            ++it;
        }
    }
}

//==============================================================================

void StyleContext::restyleSubtree (Component& component, const DeclarationMap& inherited)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

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
            resolved.element = declaration.element;
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

    const ScopedValueSetter<const DeclarationMap*> reportedDeclarations (currentDeclarations, &direct);

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

    // A component styled for the first time shows its values at once, later restyles transition
    const bool firstStyle = findCache (component) == nullptr;
    auto& cache = getCache (component);

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
        {
            clearCacheValues (cache);
            return;
        }

        animator.removeComponent (component);
        restoreAllStateBacked (component);
        restoreDrawing (component);

        bool changed = styleState->clearComputedValues();
        changed |= styleState->clearAnimatedValues();
        changed |= styleState->setStylePalette (std::nullopt);
        changed |= applyEffects (component);
        styleState->setPaletteDependent (false);
        clearCacheValues (cache);

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
    static const Identifier transitionDeclaration ("transition");

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
            report (paletteDeclaration, "Unknown palette '" + name + "'");
        else
            changed |= state.setStylePalette (*palette);
    }
    else
    {
        changed |= state.setStylePalette (std::nullopt);
    }

    // The drawing comes next, the properties it declares type the declarations setting them
    const DrawingStyle* drawing = nullptr;

    if (const auto it = direct.find (drawingDeclaration); it != direct.end())
        drawing = installDrawing (component, it->second.value.unquoted().trim());
    else
        restoreDrawing (component);

    // The transitions of the new style apply, starting from the values shown before any is written
    std::vector<PendingTransition> pendingTransitions;

    if (const auto it = direct.find (transitionDeclaration); it != direct.end())
    {
        auto parsed = StyleAnimator::parseTransitions (it->second.value.unquoted());

        if (parsed.failed())
            report (transitionDeclaration, "transition: " + parsed.getErrorMessage());
        else
        {
            for (const auto& transition : parsed.getReference())
            {
                const auto* descriptor = findDescriptor (typeInfo, drawing, transition.property);
                const auto storageId = descriptor != nullptr ? descriptor->getStorageKey() : transition.property;

                pendingTransitions.push_back ({ transition, descriptor, storageId, firstStyle ? var() : component.findProperty (transition.property) });
            }
        }
    }

    for (const auto& [name, declaration] : direct)
    {
        if (name == paletteDeclaration)
            continue;

        const auto* descriptor = findDescriptor (typeInfo, drawing, name);
        const auto storageId = descriptor != nullptr ? descriptor->getStorageKey() : name;

        if (descriptor != nullptr && ! descriptor->isStyleable)
        {
            report (name, "The property '" + name.toString() + "' can't be styled");
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

            if (! evaluate (component, name, storageId, descriptor != nullptr ? std::optional (descriptor->type) : std::nullopt, declaration.value, value, valuePaletteDependent))
                continue;

            paletteDependent |= valuePaletteDependent;
        }

        written.add (storageId);

        if (state.setComputedValue (storageId, value))
        {
            changed = true;
            layoutChanged |= descriptor != nullptr && descriptor->affectsParentLayout;
        }
    }

    // Removes what the previous restyle computed and no rule matches anymore
    auto touched = written;

    for (int i = previous.size(); --i >= 0;)
    {
        const auto name = previous.getName (i);

        if (! written.contains (name))
        {
            touched.add (name);

            if (state.removeComputedValue (name))
            {
                changed = true;
                layoutChanged |= affectsParentLayout (name);
            }
        }
    }

    bool animatedChanged = updateTransitions (component, pendingTransitions, touched);
    animatedChanged |= updateAnimations (component, direct, previous, drawing, touched);

    if (animatedChanged)
    {
        changed = true;
        layoutChanged |= std::any_of (touched.begin(), touched.end(), affectsParentLayout);
    }

    // A property the component holds itself can only be styled through its setter
    for (const auto& storageId : touched)
        syncStateBacked (component, storageId);

    changed |= applyEffects (component, &paletteDependent);
    state.setPaletteDependent (paletteDependent);

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
                             const Identifier& declarationName,
                             const Identifier& storageId,
                             std::optional<ComponentProperty::Type> type,
                             const String& valueText,
                             var& result,
                             bool& paletteDependent)
{
    auto& entry = getCache (component).values[storageId];

    // A value that failed to compile is kept, so that it isn't compiled and reported again until it changes
    if (entry == nullptr || entry->source != valueText || entry->type != type)
    {
        if (entry != nullptr)
            removeDependencies (entry->dependencies);

        entry = std::make_unique<CompiledValue>();
        entry->source = valueText;
        entry->type = type;

        auto compiled = PropertyExpression::compile (component, valueText, type, componentResolver, modelResolver);

        if (compiled.failed())
        {
            report (declarationName, storageId.toString() + ": " + compiled.getErrorMessage());
            return false;
        }

        entry->expression = std::move (compiled.getReference());

        for (const auto& dep : entry->expression->getDependencies())
            entry->dependencies.push_back ({ dep.component, dep.provider, dep.property });

        addDependencies (entry->dependencies);
    }

    if (entry->expression == nullptr)
        return false;

    paletteDependent = entry->expression->readsPalette();
    result = entry->expression->evaluate();
    return ! result.isVoid();
}

void StyleContext::syncStateBacked (Component& component, const Identifier& storageId)
{
    const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId);

    if (descriptor == nullptr || ! descriptor->isStateBacked() || descriptor->setter == nullptr)
        return;

    const auto* styled = component.styleState != nullptr ? component.styleState->getStyleValue (storageId) : nullptr;

    if (styled == nullptr)
    {
        restoreStateBacked (component, storageId);
        return;
    }

    const auto value = *styled;
    auto& cache = getCache (component);

    if (! cache.savedState.contains (storageId))
        cache.savedState.set (storageId, descriptor->getter (component));

    if (! descriptor->getter (component).equals (value))
        descriptor->setter (component, value);
}

void StyleContext::restoreStateBacked (Component& component, const Identifier& storageId)
{
    auto* cache = findCache (component);
    if (cache == nullptr)
        return;

    auto& saved = cache->savedState;

    if (const auto* original = saved.getVarPointer (storageId))
    {
        const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId);

        if (descriptor != nullptr && descriptor->setter != nullptr)
            descriptor->setter (component, *original);

        saved.remove (storageId);
    }
}

void StyleContext::restoreAllStateBacked (Component& component)
{
    auto* cache = findCache (component);
    if (cache == nullptr)
        return;

    auto& saved = cache->savedState;

    for (int i = saved.size(); --i >= 0;)
        restoreStateBacked (component, saved.getName (i));
}

const DrawingStyle* StyleContext::installDrawing (Component& component, const String& name)
{
    auto& cache = getCache (component);

    if (cache.drawingName == name && component.getStyle().get() == cache.drawingStyle.get())
        return cache.drawingStyle.get();

    const auto drawing = styleSheet != nullptr ? styleSheet->findDrawing (name) : DataTree();

    if (! drawing.isValid())
    {
        report ("drawing", "Unknown drawing '" + name + "'");
        restoreDrawing (component);
        return nullptr;
    }

    auto created = DrawingStyle::create (component, drawing, componentResolver, modelResolver);

    if (created.failed())
    {
        report ({ .message = "drawing " + name + ": " + created.getErrorMessage(), .element = drawing });
        restoreDrawing (component);
        return nullptr;
    }

    // Only the style the component had before the first drawing is remembered, for restoring it
    if (cache.drawingStyle == nullptr || component.getStyle().get() != cache.drawingStyle.get())
        cache.savedStyle = component.getStyle();

    cache.drawingName = name;
    cache.drawingStyle = created.getReference();
    component.setStyle (cache.drawingStyle);
    return cache.drawingStyle.get();
}

const ComponentProperty* StyleContext::findDescriptor (const ComponentTypeInfo& typeInfo, const DrawingStyle* drawing, const Identifier& name)
{
    if (const auto* descriptor = typeInfo.findProperty (name))
        return descriptor;

    if (drawing != nullptr)
        for (const auto& property : drawing->getProperties())
            if (property.name == name)
                return &property;

    return nullptr;
}

const ComponentProperty* StyleContext::findStoredProperty (const ComponentTypeInfo& typeInfo, const Identifier& storageId)
{
    if (const auto* descriptor = typeInfo.findProperty (storageId))
        return descriptor;

    return typeInfo.findPropertyByStorageKey (storageId);
}

void StyleContext::restoreDrawing (Component& component)
{
    auto* cache = findCache (component);

    if (cache == nullptr || cache->drawingStyle == nullptr)
        return;

    // The application may have replaced the drawing in the meantime, and its choice wins
    if (component.getStyle().get() == cache->drawingStyle.get())
        component.setStyle (std::move (cache->savedStyle));

    cache->drawingName = {};
    cache->drawingStyle = nullptr;
    cache->savedStyle = nullptr;
}

bool StyleContext::applyEffects (Component& component, bool* paletteDependent)
{
    static const Identifier cursorDeclaration ("cursor");
    static const Identifier effectDeclaration ("effect");
    static const Identifier shadowDeclaration ("shadow");

    const auto* state = component.styleState.get();

    const auto valueOf = [state] (const Identifier& name)
    {
        const auto* value = state != nullptr ? state->getComputedValue (name) : nullptr;
        return value != nullptr ? value->toString().trim() : String();
    };

    applyCursor (component, valueOf (cursorDeclaration));
    applyEffect (component, valueOf (effectDeclaration));

    bool shadowsReadPalette = false;
    const bool changed = applyShadows (component, valueOf (shadowDeclaration), shadowsReadPalette);

    if (paletteDependent != nullptr)
        *paletteDependent |= shadowsReadPalette;

    return changed;
}

void StyleContext::applyCursor (Component& component, const String& name)
{
    auto& cache = getCache (component);

    // As with drawings, a cursor the application set in the meantime wins over the styled one
    const bool styled = cache.cursor.has_value() && component.getMouseCursor() == *cache.cursor;
    const auto type = findStyleCursor (name);

    if (name.isNotEmpty() && ! type.has_value())
        report ("cursor", "Unknown cursor '" + name + "'");

    if (! type.has_value())
    {
        if (styled)
            component.setMouseCursor (cache.savedCursor);

        cache.cursor.reset();
        return;
    }

    if (! styled)
        cache.savedCursor = component.getMouseCursor();

    cache.cursor = MouseCursor (*type);

    if (component.getMouseCursor() != *cache.cursor)
        component.setMouseCursor (*cache.cursor);
}

void StyleContext::applyEffect (Component& component, const String& name)
{
    auto& cache = getCache (component);

    const bool styled = cache.effectName.isNotEmpty() && component.getComponentEffect().get() == cache.effect.get();

    if (styled && name == cache.effectName)
        return;

    ComponentEffect::Ptr effect;

    if (name.isNotEmpty() && name != "none")
    {
        effect = ComponentEffectRegistry::getInstance().createEffect (name);

        if (effect == nullptr)
            report ("effect", "Unknown effect '" + name + "'");
    }

    if (name.isEmpty() || (effect == nullptr && name != "none"))
    {
        if (styled)
            component.setComponentEffect (std::move (cache.savedEffect));

        cache.effectName = {};
        cache.effect = nullptr;
        cache.savedEffect = nullptr;
        return;
    }

    if (! styled)
        cache.savedEffect = component.getComponentEffect();

    cache.effectName = name;
    cache.effect = effect;
    component.setComponentEffect (std::move (effect));
}

bool StyleContext::applyShadows (Component& component, const String& text, bool& paletteDependent)
{
    std::vector<StyleState::Shadow> shadows;

    if (text.isNotEmpty() && text != "none")
    {
        const auto layers = StyleSheet::tokenizeValue (text);

        for (int index = 0; index < static_cast<int> (layers.size()); ++index)
        {
            const auto& tokens = layers[static_cast<size_t> (index)];

            // Up to four lengths, then a color expression taking the remaining tokens
            int lengths = 0;

            while (lengths < jmin (4, tokens.size() - 1) && isShadowLength (tokens[lengths]))
                ++lengths;

            if (lengths < 2)
            {
                report ("shadow", "shadow: Expected 'x y [blur [spread]] color', got '" + tokens.joinIntoString (" ") + "'");
                shadows.clear();
                break;
            }

            var color;
            bool colorReadsPalette = false;
            const Identifier colorId ("shadow[" + String (index) + "]");

            if (! evaluate (component, "shadow", colorId, ComponentProperty::Type::color, tokens.joinIntoString (" ", lengths), color, colorReadsPalette))
            {
                shadows.clear();
                break;
            }

            paletteDependent |= colorReadsPalette;

            StyleState::Shadow shadow;
            shadow.offset = { tokens[0].getFloatValue(), tokens[1].getFloatValue() };
            shadow.blur = lengths > 2 ? jmax (0.0f, tokens[2].getFloatValue()) : 0.0f;
            shadow.spread = lengths > 3 ? tokens[3].getFloatValue() : 0.0f;
            shadow.color = Color (static_cast<uint32> (static_cast<int64> (color)));
            shadows.push_back (shadow);
        }
    }

    // The paint margin grows to hold the shadows, keeping what the application asked for
    auto& cache = getCache (component);
    const auto current = component.getPaintMargin();
    const bool styled = cache.paintMargin.has_value() && current == *cache.paintMargin;

    if (shadows.empty())
    {
        if (styled)
            component.setPaintMargin (cache.savedPaintMargin);

        cache.paintMargin.reset();
    }
    else
    {
        if (! styled)
            cache.savedPaintMargin = current;

        auto left = cache.savedPaintMargin.getLeft();
        auto top = cache.savedPaintMargin.getTop();
        auto right = cache.savedPaintMargin.getRight();
        auto bottom = cache.savedPaintMargin.getBottom();

        for (const auto& shadow : shadows)
        {
            const auto reach = shadow.blur + jmax (0.0f, shadow.spread);
            left = jmax (left, reach - shadow.offset.getX());
            top = jmax (top, reach - shadow.offset.getY());
            right = jmax (right, reach + shadow.offset.getX());
            bottom = jmax (bottom, reach + shadow.offset.getY());
        }

        cache.paintMargin = EdgeSpacing<float> (left, top, right, bottom);

        if (current != *cache.paintMargin)
            component.setPaintMargin (*cache.paintMargin);
    }

    if (component.styleState == nullptr && shadows.empty())
        return false;

    return component.getOrCreateStyleState().setShadows (std::move (shadows));
}

StyleContext::ComponentCache* StyleContext::findCache (const Component& component)
{
    const auto it = caches.find (const_cast<Component*> (&component));

    if (it == caches.end())
        return nullptr;

    if (it->second == nullptr || it->second->component.get() != &component)
    {
        if (it->second != nullptr)
            clearCacheValues (*it->second);

        caches.erase (it);
        return nullptr;
    }

    return it->second.get();
}

StyleContext::ComponentCache& StyleContext::getCache (Component& component)
{
    if (auto* existing = findCache (component))
        return *existing;

    auto& cache = caches[&component];
    cache = std::make_unique<ComponentCache>();
    cache->component = WeakReference<Component> (&component);
    return *cache;
}

void StyleContext::clearCacheValues (ComponentCache& cache)
{
    for (auto& [id, val] : cache.values)
        if (val != nullptr)
            removeDependencies (val->dependencies);

    cache.values.clear();
}

void StyleContext::clearAllCaches()
{
    for (auto& [component, count] : observedComponents)
        component->removeComponentListener (this);

    for (auto& [provider, count] : observedProviders)
        provider->removeListener (this);

    observedComponents.clear();
    observedProviders.clear();
    caches.clear();
}

//==============================================================================

var StyleContext::getBaseValue (Component& component, const ComponentProperty* descriptor, const Identifier& storageId)
{
    auto& state = component.getOrCreateStyleState();

    if (const auto* computed = state.getComputedValue (storageId))
        return *computed;

    if (descriptor != nullptr && descriptor->isStateBacked())
    {
        if (auto* cache = findCache (component))
            if (const auto* original = cache->savedState.getVarPointer (storageId))
                return *original;

        return descriptor->getter (component);
    }

    // What the component shows without its own animated value: explicit, inherited or themed
    std::optional<var> animated;

    if (const auto* value = state.getAnimatedValue (storageId))
        animated = *value;

    state.removeAnimatedValue (storageId);
    auto base = component.findProperty (descriptor != nullptr ? descriptor->name : storageId);

    if (animated.has_value())
        state.setAnimatedValue (storageId, *animated);

    return base;
}

bool StyleContext::updateTransitions (Component& component, const std::vector<PendingTransition>& pending, Array<Identifier>& touched)
{
    auto& state = component.getOrCreateStyleState();
    Array<Identifier> declared;
    bool changed = false;

    for (const auto& entry : pending)
    {
        declared.add (entry.storageId);
        touched.addIfNotAlreadyThere (entry.storageId);

        const auto to = getBaseValue (component, entry.descriptor, entry.storageId);

        // A restyle that keeps the target leaves the running transition alone
        if (const auto* target = animator.findTransitionTarget (component, entry.storageId); target != nullptr && target->equals (to))
            continue;

        if (entry.transition.duration > 0.0 && ! entry.from.isVoid() && ! to.isVoid() && ! entry.from.equals (to))
        {
            animator.startTransition (component, entry.storageId, inferAnimatedType (entry.descriptor, to), entry.from, to, entry.transition, clock);
            changed |= state.setAnimatedValue (entry.storageId, entry.from);
        }
        else if (animator.cancelTransition (component, entry.storageId))
        {
            changed |= state.removeAnimatedValue (entry.storageId);
        }
    }

    // Like in CSS, the transitions the new style doesn't declare jump to their end
    for (const auto& storageId : animator.getTransitions (component))
    {
        if (declared.contains (storageId))
            continue;

        animator.cancelTransition (component, storageId);
        changed |= state.removeAnimatedValue (storageId);
        touched.addIfNotAlreadyThere (storageId);
    }

    return changed;
}

bool StyleContext::updateAnimations (Component& component,
                                     const DeclarationMap& direct,
                                     const NamedValueSet& previous,
                                     const DrawingStyle* drawing,
                                     Array<Identifier>& touched)
{
    static const Identifier animationDeclaration ("animation");

    const auto it = direct.find (animationDeclaration);
    const auto text = it != direct.end() ? it->second.value.unquoted() : String();
    const auto* previousText = previous.getVarPointer (animationDeclaration);

    // The animations keep running until the declaration changes, which restarts them
    if (text == (previousText != nullptr ? previousText->toString() : String()))
        return false;

    auto& state = component.getOrCreateStyleState();
    bool changed = false;

    for (const auto& storageId : animator.stopAnimations (component))
    {
        if (animator.findTransitionTarget (component, storageId) == nullptr)
            changed |= state.removeAnimatedValue (storageId);

        touched.addIfNotAlreadyThere (storageId);
    }

    if (text.trim().isEmpty())
        return changed;

    auto parsed = StyleAnimator::parseAnimations (text);

    if (parsed.failed())
    {
        report ("animation", "animation: " + parsed.getErrorMessage());
        return changed;
    }

    for (const auto& animation : parsed.getReference())
    {
        const auto* keyframes = styleSheet != nullptr ? styleSheet->findKeyframes (animation.name) : nullptr;

        if (keyframes == nullptr)
        {
            report ("animation", "Unknown keyframes '" + animation.name.toString() + "'");
            continue;
        }

        animator.startAnimation (component, animation, compileTracks (component, *keyframes, drawing), clock);
    }

    return changed;
}

std::vector<StyleAnimator::Track> StyleContext::compileTracks (Component& component, const StyleSheet::KeyframeSequence& keyframes, const DrawingStyle* drawing)
{
    struct CompiledTrack
    {
        StyleAnimator::Track track;
        const ComponentProperty* descriptor = nullptr;
    };

    const auto& typeInfo = component.getTypeInfo();
    std::vector<CompiledTrack> compiled;

    for (const auto& keyframe : keyframes.frames)
    {
        for (const auto& declaration : keyframe.declarations)
        {
            const auto* descriptor = findDescriptor (typeInfo, drawing, declaration.name);

            if (descriptor != nullptr && ! descriptor->isStyleable)
            {
                report ({ .message = "The property '" + declaration.name.toString() + "' can't be animated", .element = declaration.element });
                continue;
            }

            const auto storageId = descriptor != nullptr ? descriptor->getStorageKey() : declaration.name;
            auto existing = std::find_if (compiled.begin(), compiled.end(), [&] (const auto& entry)
            {
                return entry.track.storageId == storageId;
            });

            std::optional<ComponentProperty::Type> type;

            if (descriptor != nullptr)
                type = descriptor->type;
            else if (existing != compiled.end())
                type = existing->track.type;

            auto expression = PropertyExpression::compile (component, declaration.value, type, componentResolver, modelResolver);

            if (expression.failed())
            {
                report ({ .message = keyframes.name.toString() + " " + declaration.name.toString() + ": " + expression.getErrorMessage(), .element = declaration.element });
                continue;
            }

            auto value = expression.getReference()->evaluate();

            if (value.isVoid())
                continue;

            if (existing == compiled.end())
            {
                compiled.push_back ({ { storageId, inferAnimatedType (descriptor, value), {} }, descriptor });
                existing = std::prev (compiled.end());
            }

            existing->track.frames.push_back ({ keyframe.position, std::move (value) });
        }
    }

    std::vector<StyleAnimator::Track> tracks;

    for (auto& [track, descriptor] : compiled)
    {
        auto& frames = track.frames;

        // A missing first or last keyframe plays from or to the value the style shows
        if (frames.front().position > 0.0 || frames.back().position < 1.0)
        {
            if (const auto base = getBaseValue (component, descriptor, track.storageId); ! base.isVoid())
            {
                if (frames.front().position > 0.0)
                    frames.insert (frames.begin(), { 0.0, base });

                if (frames.back().position < 1.0)
                    frames.push_back ({ 1.0, base });
            }
        }

        tracks.push_back (std::move (track));
    }

    return tracks;
}

void StyleContext::showAnimatedValue (Component& component, const Identifier& storageId, const var* value)
{
    auto& state = component.getOrCreateStyleState();
    const bool changed = value != nullptr ? state.setAnimatedValue (storageId, *value) : state.removeAnimatedValue (storageId);

    if (! changed)
        return;

    syncStateBacked (component, storageId);
    component.styleChanged();
    component.repaint();

    if (const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId); descriptor != nullptr && descriptor->affectsParentLayout)
        if (auto* parent = component.getParentComponent())
            parent->childLayoutChanged (component);
}

//==============================================================================

void StyleContext::detach()
{
    clearSubtree (root);
    clearAllCaches();
    animator.clear();
    dirtyComponents.clear();
    diagnostics.clear();
}

void StyleContext::detachComponent (Component& component)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

    animator.removeComponent (component);

    for (int i = dirtyComponents.size(); --i >= 0;)
        if (dirtyComponents.getReference (i).get() == &component || dirtyComponents.getReference (i).get() == nullptr)
            dirtyComponents.remove (i);

    if (auto* state = component.styleState.get())
    {
        const auto shown = state->getAnimatedValues();

        restoreAllStateBacked (component);
        restoreDrawing (component);

        bool changed = state->clearComputedValues();
        changed |= state->clearAnimatedValues();
        changed |= state->setStylePalette (std::nullopt);
        changed |= applyEffects (component);
        state->setPaletteDependent (false);

        for (int i = 0; i < shown.size(); ++i)
            changed |= state->setComputedValue (shown.getName (i), *shown.getVarPointerAt (i));

        if (changed)
            component.repaint();
    }

    if (const auto it = caches.find (&component); it != caches.end())
    {
        if (it->second != nullptr)
            clearCacheValues (*it->second);

        caches.erase (it);
    }

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            detachComponent (*child);
}

void StyleContext::forgetComponent (Component& component)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

    animator.removeComponent (component);

    for (int i = dirtyComponents.size(); --i >= 0;)
        if (dirtyComponents.getReference (i).get() == &component || dirtyComponents.getReference (i).get() == nullptr)
            dirtyComponents.remove (i);

    if (const auto it = caches.find (&component); it != caches.end())
    {
        if (it->second != nullptr)
            clearCacheValues (*it->second);

        caches.erase (it);
    }

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            forgetComponent (*child);
}

void StyleContext::clearSubtree (Component& component)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

    if (auto* state = component.styleState.get())
    {
        restoreAllStateBacked (component);
        restoreDrawing (component);

        bool changed = state->clearComputedValues();
        changed |= state->clearAnimatedValues();
        changed |= state->setStylePalette (std::nullopt);
        changed |= applyEffects (component);
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
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

    if (auto* state = component.styleState.get(); state != nullptr && state->isPaletteDependent())
        markDirty (component);

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            markPaletteDependentDirty (*child);
}

void StyleContext::addDependencies (const std::vector<BoundDependency>& dependencies)
{
    for (const auto& dependency : dependencies)
    {
        if (dependency.component != nullptr)
        {
            if (++observedComponents[dependency.component] == 1)
                dependency.component->addComponentListener (this);
        }
        else if (dependency.provider != nullptr)
        {
            if (++observedProviders[dependency.provider] == 1)
                dependency.provider->addListener (this);
        }
    }
}

void StyleContext::removeDependencies (const std::vector<BoundDependency>& dependencies)
{
    for (const auto& dependency : dependencies)
    {
        if (dependency.component != nullptr)
        {
            if (const auto it = observedComponents.find (dependency.component); it != observedComponents.end() && --it->second <= 0)
            {
                auto* component = it->first;
                observedComponents.erase (it);
                component->removeComponentListener (this);
            }
        }
        else if (dependency.provider != nullptr)
        {
            if (const auto it = observedProviders.find (dependency.provider); it != observedProviders.end() && --it->second <= 0)
            {
                auto* provider = it->first;
                observedProviders.erase (it);
                provider->removeListener (this);
            }
        }
    }
}

void StyleContext::invalidateDependencies (const Component* sourceComponent, const BindingProvider* sourceProvider, const Identifier& property)
{
    for (const auto& [ptr, cache] : caches)
    {
        if (cache == nullptr)
            continue;

        auto* styled = cache->component.get();
        if (styled == nullptr || styled != ptr)
            continue;

        bool matched = false;

        for (const auto& [storageId, compiled] : cache->values)
        {
            if (compiled == nullptr)
                continue;

            for (const auto& dependency : compiled->dependencies)
            {
                if ((sourceComponent != nullptr && dependency.component == sourceComponent && dependency.property == property)
                    || (sourceProvider != nullptr && dependency.provider == sourceProvider && dependency.property == property))
                {
                    matched = true;
                    break;
                }
            }

            if (matched)
                break;
        }

        if (matched)
            markDirty (*styled);
    }
}

void StyleContext::componentPropertyChanged (Component& component, const Identifier& property)
{
    invalidateDependencies (&component, nullptr, property);
}

void StyleContext::componentMoved (Component& component)
{
    static const Identifier xId ("x");
    static const Identifier yId ("y");

    invalidateDependencies (&component, nullptr, xId);
    invalidateDependencies (&component, nullptr, yId);
}

void StyleContext::componentResized (Component& component)
{
    static const Identifier widthId ("width");
    static const Identifier heightId ("height");

    invalidateDependencies (&component, nullptr, widthId);
    invalidateDependencies (&component, nullptr, heightId);
}

void StyleContext::componentBeingDeleted (Component& component)
{
    observedComponents.erase (&component);

    for (auto& [ptr, cache] : caches)
    {
        if (cache == nullptr)
            continue;

        for (auto& [storageId, compiled] : cache->values)
        {
            if (compiled == nullptr)
                continue;

            for (auto& dependency : compiled->dependencies)
                if (dependency.component == &component)
                    dependency.component = nullptr;
        }
    }
}

void StyleContext::bindingValueChanged (BindingProvider& provider, const Identifier& name)
{
    invalidateDependencies (nullptr, &provider, name);
}

void StyleContext::bindingProviderBeingDeleted (BindingProvider& provider)
{
    observedProviders.erase (&provider);

    for (auto& [ptr, cache] : caches)
    {
        if (cache == nullptr)
            continue;

        for (auto& [storageId, compiled] : cache->values)
        {
            if (compiled == nullptr)
                continue;

            for (auto& dependency : compiled->dependencies)
                if (dependency.provider == &provider)
                    dependency.provider = nullptr;
        }
    }
}

void StyleContext::report (const Identifier& declarationName, const String& message)
{
    // Reports by name only happen while a component is styled, about one of its declarations
    jassert (currentDeclarations != nullptr && currentDeclarations->count (declarationName) == 1);

    report ({ .message = message, .element = currentDeclarations->at (declarationName).element });
}

void StyleContext::report (const YuiDiagnostic& diagnostic)
{
    diagnostics.push_back (diagnostic);

    if (onDiagnostic != nullptr)
        onDiagnostic (diagnostic);
}

} // namespace yup
