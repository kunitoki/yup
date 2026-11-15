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
    const StyleSheet::Declaration* declaration = nullptr;
    int specificity = 0;
    int order = 0;
    bool direct = false;

    bool beats (const ResolvedDeclaration& other) const noexcept
    {
        if (specificity != other.specificity)
            return specificity > other.specificity;

        if (direct != other.direct)
            return direct;

        if (order != other.order)
            return order > other.order;

        return ! std::less<const StyleSheet::Declaration*>() (declaration, other.declaration);
    }

    bool comesBeforeInSource (const ResolvedDeclaration& other) const noexcept
    {
        if (order != other.order)
            return order < other.order;

        return std::less<const StyleSheet::Declaration*>() (declaration, other.declaration);
    }
};

struct StyleContext::BoundDependency
{
    Component* component = nullptr;
    BindingProvider* provider = nullptr;
    Identifier property;
};

struct StyleContext::ValueKey
{
    const StyleSheet::Declaration* declaration = nullptr;
    int layer = -1;

    bool operator== (const ValueKey& other) const noexcept = default;
};

struct StyleContext::ValueKeyHash
{
    std::size_t operator() (const ValueKey& key) const noexcept
    {
        return std::hash<const void*>() (key.declaration) ^ (static_cast<std::size_t> (key.layer + 1) * 0x9e3779b9u);
    }
};

/* One compiled declaration value, kept per component so that frames reuse the program. */
struct StyleContext::CompiledValue
{
    std::optional<ComponentProperty::Type> type;
    std::unique_ptr<PropertyExpression> expression;
    var constantValue;
    std::vector<BoundDependency> dependencies;
    bool paletteDependent = false;
    bool failed = false;
};

/* Everything the context keeps for a component it visited, styled or not, so later restyles transition. */
struct StyleContext::ComponentCache
{
    WeakReference<Component> component;
    std::unordered_map<ValueKey, CompiledValue, ValueKeyHash> values;
    NamedValueSet savedState;

    String drawingName;
    DrawingStyle::Ptr drawingStyle;
    ComponentStyle::Ptr savedStyle;

    StyleEffects effects;
};

struct StyleContext::CacheMap : std::map<Component*, ComponentCache>
{
};

/* A transition the new style of a component declares, with the value shown before the restyle. */
struct StyleContext::PendingTransition
{
    StyleAnimator::Transition transition;
    const ComponentProperty* descriptor = nullptr;
    Identifier storageId;
    var from;
};

void StyleContext::mergeResolvedDeclaration (DeclarationMap& target,
                                             const Identifier& key,
                                             const ResolvedDeclaration& candidate)
{
    const auto [it, inserted] = target.emplace (key, candidate);
    if (! inserted && candidate.beats (it->second))
        it->second = candidate;
}

//==============================================================================

StyleContext::StyleContext (Component& rootToUse, StyleSheet::Ptr sheetToUse)
    : root (rootToUse)
    , styleSheet (std::move (sheetToUse))
    , caches (std::make_unique<CacheMap>())
{
    if (auto theme = ApplicationTheme::getGlobalTheme())
        lastPaletteGeneration = theme->getPaletteGeneration();

    markDirty (root);
}

StyleContext::~StyleContext()
{
    clearAllCaches();
}

template <class Fn>
void StyleContext::forEachManaged (Component& component, Fn&& fn)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return;

    fn (component);

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            forEachManaged (*child, fn);
}

//==============================================================================

void StyleContext::setStyleSheet (StyleSheet::Ptr newStyleSheet)
{
    if (styleSheet == newStyleSheet)
        return;

    detach();
    styleSheet = std::move (newStyleSheet);
    markDirty (root);
}

void StyleContext::setComponentResolver (ComponentResolver resolver)
{
    componentResolver = std::move (resolver);
    resetCompiledValues();
}

void StyleContext::setModelResolver (ModelResolver resolver)
{
    modelResolver = std::move (resolver);
    resetCompiledValues();
}

void StyleContext::resetCompiledValues()
{
    for (auto& [ptr, cache] : *caches)
    {
        clearCacheValues (cache);

        if (auto* comp = cache.component.get())
            restoreDrawing (*comp, cache);
        else
        {
            cache.drawingName = {};
            cache.drawingStyle = nullptr;
            cache.savedStyle = nullptr;
        }
    }

    markDirty (root);
}

//==============================================================================

void StyleContext::markDirty (Component& component)
{
    if (! dirtyComponents.isEmpty() && dirtyComponents.getLast().get() == &component)
        return;

    dirtyComponents.add (WeakReference<Component> (&component));
}

void StyleContext::propertyChanged (Component& component, const Identifier& name)
{
    static const Identifier paletteProperty ("palette");

    if (styleSheet != nullptr && styleSheet->watchesProperty (name))
        markDirty (component);
    else if (! isRestyling && name == paletteProperty && component.styleState != nullptr && component.styleState->isPaletteDependent())
        markDirty (component);
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

            if (onPaletteChanged != nullptr)
                onPaletteChanged();
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

    const ScopedValueSetter<bool> restyling (isRestyling, true);
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

#if YUP_DEBUG
    for (const auto& [ptr, cache] : *caches)
        jassert (cache.component.get() == ptr);
#endif
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

        for (const auto& [key, declaration] : qualified)
            mergeResolvedDeclaration (inherited, key, declaration);
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
            const ResolvedDeclaration resolved { &declaration, specificity, rule.order, true };
            auto& target = declaration.qualifier.isValid() ? qualified : direct;
            const auto& key = declaration.cascadeKey.isValid() ? declaration.cascadeKey : declaration.name;

            mergeResolvedDeclaration (target, key, resolved);
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

    for (const auto& [key, declaration] : qualified)
        mergeResolvedDeclaration (inheritedForChildren, key, declaration);

    // An inherited qualified declaration applies when the component is of the type it names
    const auto& typeInfo = component.getTypeInfo();

    for (const auto& [key, declaration] : inheritedForChildren)
    {
        if (! typeInfo.isSubtypeOf (declaration.declaration->qualifier))
            continue;

        auto copy = declaration;
        copy.direct = false;
        mergeResolvedDeclaration (direct, declaration.declaration->name, copy);
    }

    bool firstStyle = false;
    auto& cache = getCache (component, firstStyle);

    using LayoutEffect = ComponentProperty::LayoutEffect;
    bool ownLayoutChanged = false;
    bool parentLayoutChanged = false;

    const auto recordLayoutEffect = [&] (const ComponentProperty* descriptor)
    {
        if (descriptor == nullptr)
            return;

        if (descriptor->layoutEffect == LayoutEffect::own)
            ownLayoutChanged = true;
        else if (descriptor->layoutEffect == LayoutEffect::parent)
            parentLayoutChanged = true;
    };

    const auto recordStoredLayoutEffect = [&] (const Identifier& storageId)
    {
        recordLayoutEffect (findStoredProperty (typeInfo, storageId));
    };

    const auto notifyLayoutEffects = [&]
    {
        const WeakReference<Component> alive (&component);

        if (ownLayoutChanged)
            component.layoutPropertyChanged();

        if (parentLayoutChanged && alive != nullptr)
            if (auto* parent = component.getParentComponent())
                parent->childLayoutChanged (component);
    };

    if (direct.empty())
    {
        auto* styleState = component.styleState.get();
        if (styleState == nullptr)
        {
            clearCacheValues (cache);
            return;
        }

        const auto& computed = styleState->getComputedValues();
        for (int i = 0; i < computed.size(); ++i)
            recordStoredLayoutEffect (computed.getName (i));

        const auto& animated = styleState->getAnimatedValues();
        for (int i = 0; i < animated.size(); ++i)
            recordStoredLayoutEffect (animated.getName (i));

        animator.removeComponent (component);

        bool paletteChanged = false;
        const bool changed = resetStyle (component, &cache, false, &paletteChanged);
        clearCacheValues (cache);

        if (paletteChanged && component.palette == nullptr)
            component.sendPaletteChanged();
        else if (changed)
            component.repaint();

        notifyLayoutEffects();

        return;
    }

    auto& state = component.getOrCreateStyleState();
    static const Identifier paletteDeclaration ("palette");
    static const Identifier drawingDeclaration ("drawing");
    static const Identifier transitionDeclaration ("transition");
    static const Identifier animationDeclaration ("animation");
    static const Identifier shadowDeclaration ("shadow");

    const String previousAnimation = state.getComputedValue (animationDeclaration) != nullptr
                                       ? state.getComputedValue (animationDeclaration)->toString()
                                       : String();

    bool changed = false;
    bool paletteChanged = false;
    bool paletteDependent = false;
    Array<Identifier> written;

    // The palette is resolved first, so that the values of this component read the right roles
    if (const auto it = direct.find (paletteDeclaration); it != direct.end())
    {
        const auto name = it->second.declaration->value.unquoted().trim();
        const auto* palette = styleSheet != nullptr ? styleSheet->findPalette (name) : nullptr;

        if (palette == nullptr)
            report (paletteDeclaration, "Unknown palette '" + name + "'");
        else
            paletteChanged = state.setStylePalette (*palette);
    }
    else
    {
        paletteChanged = state.setStylePalette (std::nullopt);
    }

    changed |= paletteChanged;

    // The drawing comes next, the properties it declares type the declarations setting them
    const DrawingStyle* drawing = nullptr;

    if (const auto it = direct.find (drawingDeclaration); it != direct.end())
        drawing = installDrawing (component, cache, it->second.declaration->value.unquoted().trim());
    else
        restoreDrawing (component, cache);

    // The transitions of the new style apply, starting from the values shown before any is written
    std::vector<PendingTransition> pendingTransitions;

    if (const auto it = direct.find (transitionDeclaration); it != direct.end())
    {
        auto parsed = StyleAnimator::parseTransitions (it->second.declaration->value.unquoted());

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

    std::vector<std::pair<Identifier, ResolvedDeclaration>> orderedDeclarations (direct.begin(), direct.end());
    std::sort (orderedDeclarations.begin(), orderedDeclarations.end(), [] (const auto& a, const auto& b)
    {
        return a.second.comesBeforeInSource (b.second);
    });

    for (const auto& [name, resolved] : orderedDeclarations)
    {
        if (name == paletteDeclaration)
            continue;

        const auto& declaration = *resolved.declaration;
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

            if (! evaluate (component, cache, { &declaration, -1 }, name, storageId, descriptor != nullptr ? std::optional (descriptor->type) : std::nullopt, declaration.value, value, valuePaletteDependent))
                continue;

            paletteDependent |= valuePaletteDependent;
        }

        written.add (storageId);

        if (state.setComputedValue (storageId, value))
        {
            changed = true;
            recordLayoutEffect (descriptor);
        }
    }

    // Removes what the previous restyle computed and no rule matches anymore
    auto touched = written;
    const auto& computed = state.getComputedValues();

    for (int i = computed.size(); --i >= 0;)
    {
        const auto name = computed.getName (i);

        if (! written.contains (name))
        {
            touched.add (name);

            if (state.removeComputedValue (name))
            {
                changed = true;
                recordStoredLayoutEffect (name);
            }
        }
    }

    bool animatedChanged = updateTransitions (component, cache, pendingTransitions, touched);
    animatedChanged |= updateAnimations (component, cache, direct, previousAnimation, drawing, touched);

    if (animatedChanged)
    {
        changed = true;

        for (const auto& storageId : touched)
            recordStoredLayoutEffect (storageId);
    }

    // A property the component holds itself can only be styled through its setter
    for (const auto& storageId : touched)
        syncStateBacked (component, cache, storageId);

    const auto shadowIt = direct.find (shadowDeclaration);
    changed |= applyEffects (component, cache, shadowIt != direct.end() ? shadowIt->second.declaration : nullptr, &paletteDependent);
    state.setPaletteDependent (paletteDependent);

    if (paletteChanged && component.palette == nullptr)
    {
        component.sendPaletteChanged();
    }
    else if (changed)
    {
        component.styleChanged();
        component.repaint();
    }

    notifyLayoutEffects();
}

//==============================================================================

bool StyleContext::evaluate (Component& component,
                             ComponentCache& cache,
                             ValueKey key,
                             const Identifier& declarationName,
                             const Identifier& storageId,
                             std::optional<ComponentProperty::Type> type,
                             const String& valueText,
                             var& result,
                             bool& paletteDependent)
{
    auto [it, inserted] = cache.values.try_emplace (key);
    auto& entry = it->second;

    // A value that failed to compile is kept, so that it isn't compiled and reported again until its type or declaration changes
    if (inserted || entry.type != type)
    {
        if (! inserted)
            removeDependencies (entry.dependencies);

        entry = {};
        entry.type = type;

        auto compiled = PropertyExpression::compile (component, valueText, type, componentResolver, modelResolver);

        if (compiled.failed())
        {
            entry.failed = true;
            const auto label = key.layer >= 0 ? "shadow[" + String (key.layer) + "]" : storageId.toString();
            report (declarationName, label + ": " + compiled.getErrorMessage());
            return false;
        }

        auto expr = std::move (compiled.getReference());
        entry.paletteDependent = expr->readsPalette();

        if (! expr->hasInputs())
        {
            entry.constantValue = expr->evaluate();
        }
        else
        {
            static const Identifier paletteProperty ("palette");

            for (const auto& dep : expr->getDependencies())
                if (dep.component != &component || dep.property != paletteProperty)
                    entry.dependencies.push_back ({ dep.component, dep.provider, dep.property });

            addDependencies (entry.dependencies);
            entry.expression = std::move (expr);
        }
    }

    if (entry.failed)
        return false;

    paletteDependent = entry.paletteDependent;
    result = entry.expression != nullptr ? entry.expression->evaluate() : entry.constantValue;
    return ! result.isVoid();
}

void StyleContext::syncStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId)
{
    const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId);

    if (descriptor == nullptr || ! descriptor->isStateBacked() || descriptor->setter == nullptr)
        return;

    const auto* styled = component.styleState != nullptr ? component.styleState->getStyleValue (storageId) : nullptr;

    if (styled == nullptr)
    {
        restoreStateBacked (component, cache, storageId);
        return;
    }

    const auto value = *styled;

    if (! cache.savedState.contains (storageId))
        cache.savedState.set (storageId, descriptor->getter (component));

    if (! descriptor->getter (component).equals (value))
        descriptor->setter (component, value);
}

void StyleContext::restoreStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId)
{
    auto& saved = cache.savedState;

    if (const auto* original = saved.getVarPointer (storageId))
    {
        const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId);

        if (descriptor != nullptr && descriptor->setter != nullptr)
            descriptor->setter (component, *original);

        saved.remove (storageId);
    }
}

void StyleContext::restoreAllStateBacked (Component& component, ComponentCache& cache)
{
    auto& saved = cache.savedState;

    for (int i = saved.size(); --i >= 0;)
        restoreStateBacked (component, cache, saved.getName (i));
}

const DrawingStyle* StyleContext::installDrawing (Component& component, ComponentCache& cache, const String& name)
{
    if (cache.drawingName == name && component.getStyle().get() == cache.drawingStyle.get())
        return cache.drawingStyle.get();

    const auto drawing = styleSheet != nullptr ? styleSheet->findDrawing (name) : DataTree();

    if (! drawing.isValid())
    {
        report ("drawing", "Unknown drawing '" + name + "'");
        restoreDrawing (component, cache);
        return nullptr;
    }

    auto created = DrawingStyle::create (component, drawing, componentResolver, modelResolver);

    if (created.failed())
    {
        report ({ .message = "drawing " + name + ": " + created.getErrorMessage(), .element = drawing });
        restoreDrawing (component, cache);
        return nullptr;
    }

    clearCacheValues (cache);

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

void StyleContext::restoreDrawing (Component& component, ComponentCache& cache)
{
    if (cache.drawingStyle == nullptr)
        return;

    clearCacheValues (cache);

    // The application may have replaced the drawing in the meantime, and its choice wins
    if (component.getStyle().get() == cache.drawingStyle.get())
        component.setStyle (std::move (cache.savedStyle));

    cache.drawingName = {};
    cache.drawingStyle = nullptr;
    cache.savedStyle = nullptr;
}

bool StyleContext::applyEffects (Component& component, ComponentCache& cache, const StyleSheet::Declaration* shadowDecl, bool* paletteDependent)
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

    const auto reporter = [this] (const Identifier& declaration, const String& message)
    {
        report (declaration, message);
    };

    const auto evaluateColor = [&] (int layer, const String& source) -> std::optional<Color>
    {
        var color;
        bool readsPalette = false;

        if (! evaluate (component, cache, { shadowDecl, layer }, shadowDeclaration, {}, ComponentProperty::Type::color, source, color, readsPalette))
            return std::nullopt;

        if (paletteDependent != nullptr)
            *paletteDependent |= readsPalette;

        return Color (static_cast<uint32> (static_cast<int64> (color)));
    };

    cache.effects.applyCursor (component, valueOf (cursorDeclaration), reporter);
    cache.effects.applyEffect (component, valueOf (effectDeclaration), reporter);

    auto shadows = cache.effects.applyShadows (component, valueOf (shadowDeclaration), evaluateColor, reporter);

    if (component.styleState == nullptr && shadows.empty())
        return false;

    return component.getOrCreateStyleState().setShadows (std::move (shadows));
}

StyleContext::ComponentCache* StyleContext::findCache (const Component& component)
{
    const auto it = caches->find (const_cast<Component*> (&component));
    return it != caches->end() ? &it->second : nullptr;
}

StyleContext::ComponentCache& StyleContext::getCache (Component& component, bool& created)
{
    const auto [it, inserted] = caches->try_emplace (&component);
    created = inserted;

    if (inserted)
        it->second.component = WeakReference<Component> (&component);

    return it->second;
}

void StyleContext::clearCacheValues (ComponentCache& cache)
{
    for (auto& [key, val] : cache.values)
        removeDependencies (val.dependencies);

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
    caches->clear();
}

//==============================================================================

var StyleContext::getBaseValue (Component& component, const ComponentCache& cache, const ComponentProperty* descriptor, const Identifier& storageId)
{
    auto& state = component.getOrCreateStyleState();

    if (const auto* computed = state.getComputedValue (storageId))
        return *computed;

    if (descriptor != nullptr && descriptor->isStateBacked())
    {
        if (const auto* original = cache.savedState.getVarPointer (storageId))
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

bool StyleContext::updateTransitions (Component& component, const ComponentCache& cache, const std::vector<PendingTransition>& pending, Array<Identifier>& touched)
{
    auto& state = component.getOrCreateStyleState();
    Array<Identifier> declared;
    bool changed = false;

    for (const auto& entry : pending)
    {
        declared.add (entry.storageId);
        touched.addIfNotAlreadyThere (entry.storageId);

        const auto to = getBaseValue (component, cache, entry.descriptor, entry.storageId);

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
                                     const ComponentCache& cache,
                                     const DeclarationMap& direct,
                                     const String& previousAnimation,
                                     const DrawingStyle* drawing,
                                     Array<Identifier>& touched)
{
    static const Identifier animationDeclaration ("animation");

    const auto it = direct.find (animationDeclaration);
    const auto text = it != direct.end() ? it->second.declaration->value.unquoted() : String();

    // The animations keep running until the declaration changes, which restarts them
    if (text == previousAnimation)
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

        animator.startAnimation (component, animation, compileTracks (component, cache, *keyframes, drawing), clock);
    }

    return changed;
}

std::vector<StyleAnimator::Track> StyleContext::compileTracks (Component& component,
                                                               const ComponentCache& cache,
                                                               const StyleSheet::KeyframeSequence& keyframes,
                                                               const DrawingStyle* drawing)
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
            if (const auto base = getBaseValue (component, cache, descriptor, track.storageId); ! base.isVoid())
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

    bool created = false;
    syncStateBacked (component, getCache (component, created), storageId);
    component.styleChanged();
    component.repaint();

    if (const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId))
    {
        const WeakReference<Component> alive (&component);

        if (descriptor->layoutEffect == ComponentProperty::LayoutEffect::own)
            component.layoutPropertyChanged();

        if (descriptor->layoutEffect == ComponentProperty::LayoutEffect::parent && alive != nullptr)
            if (auto* parent = component.getParentComponent())
                parent->childLayoutChanged (component);
    }
}

//==============================================================================

bool StyleContext::resetStyle (Component& component, ComponentCache* cache, bool keepAnimatedValues, bool* paletteCleared)
{
    auto* state = component.styleState.get();
    if (state == nullptr)
        return false;

    const auto shown = keepAnimatedValues ? state->getAnimatedValues() : NamedValueSet {};

    if (cache != nullptr)
    {
        restoreAllStateBacked (component, *cache);
        restoreDrawing (component, *cache);
    }

    bool changed = state->clearComputedValues();
    changed |= state->clearAnimatedValues();
    const bool clearedPalette = state->setStylePalette (std::nullopt);
    changed |= clearedPalette;

    if (paletteCleared != nullptr)
        *paletteCleared = clearedPalette;

    if (cache != nullptr)
        changed |= applyEffects (component, *cache);

    state->setPaletteDependent (false);

    for (int i = 0; i < shown.size(); ++i)
        changed |= state->setComputedValue (shown.getName (i), *shown.getVarPointerAt (i));

    return changed;
}

void StyleContext::release (Component& subtreeRoot, ReleaseMode mode)
{
    forEachManaged (subtreeRoot, [this, mode] (Component& component)
    {
        animator.removeComponent (component);

        for (int i = dirtyComponents.size(); --i >= 0;)
            if (dirtyComponents.getReference (i).get() == &component || dirtyComponents.getReference (i).get() == nullptr)
                dirtyComponents.remove (i);

        const auto it = caches->find (&component);
        auto* cache = it != caches->end() ? &it->second : nullptr;

        if (mode != ReleaseMode::forgetOnly && resetStyle (component, cache, mode == ReleaseMode::freezeAnimatedAndForget))
            component.repaint();

        if (it != caches->end())
        {
            clearCacheValues (it->second);
            caches->erase (it);
        }
    });
}

void StyleContext::detach()
{
    forEachManaged (root, [this] (Component& component)
    {
        if (resetStyle (component, findCache (component)))
            component.repaint();
    });

    clearAllCaches();
    animator.clear();
    dirtyComponents.clear();
    diagnostics.clear();
}

void StyleContext::detachComponent (Component& component)
{
    release (component, ReleaseMode::freezeAnimatedAndForget);
}

void StyleContext::forgetComponent (Component& component)
{
    release (component, ReleaseMode::forgetOnly);
}

void StyleContext::releaseSubtree (Component& component)
{
    release (component, ReleaseMode::resetAndForget);
}

void StyleContext::markPaletteDependentDirty (Component& component)
{
    forEachManaged (component, [this] (Component& current)
    {
        if (auto* state = current.styleState.get(); state != nullptr && state->isPaletteDependent())
            markDirty (current);
    });
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
    for (const auto& [ptr, cache] : *caches)
    {
        auto* styled = cache.component.get();
        if (styled == nullptr || styled != ptr)
            continue;

        bool matched = false;

        for (const auto& [key, compiled] : cache.values)
        {
            for (const auto& dependency : compiled.dependencies)
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

void StyleContext::componentBeingDeleted (Component& component)
{
    observedComponents.erase (&component);

    for (auto& [ptr, cache] : *caches)
        for (auto& [key, compiled] : cache.values)
            for (auto& dependency : compiled.dependencies)
                if (dependency.component == &component)
                    dependency.component = nullptr;
}

void StyleContext::bindingValueChanged (BindingProvider& provider, const Identifier& name)
{
    invalidateDependencies (nullptr, &provider, name);
}

void StyleContext::bindingProviderBeingDeleted (BindingProvider& provider)
{
    observedProviders.erase (&provider);

    for (auto& [ptr, cache] : *caches)
        for (auto& [key, compiled] : cache.values)
            for (auto& dependency : compiled.dependencies)
                if (dependency.provider == &provider)
                    dependency.provider = nullptr;
}

void StyleContext::report (const Identifier& declarationName, const String& message)
{
    // Reports by name only happen while a component is styled, about one of its declarations
    jassert (currentDeclarations != nullptr && currentDeclarations->count (declarationName) == 1);

    report ({ .message = message, .element = currentDeclarations->at (declarationName).declaration->element });
}

void StyleContext::report (const YuiDiagnostic& diagnostic)
{
    diagnostics.push_back (diagnostic);

    if (onDiagnostic != nullptr)
        onDiagnostic (diagnostic);
}

} // namespace yup
