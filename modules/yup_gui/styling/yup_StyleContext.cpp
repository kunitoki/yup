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

const Identifier styleContextTransformId { "transform" };
const Identifier styleContextFilterPropertyId { "filter" };
const Identifier styleContextFilterStorageId { "__styleFilter" };
const Identifier styleContextPaintMarginId { "paintMargin" };

Identifier resolveStyleStorageId (const ComponentProperty* descriptor, const Identifier& propertyName)
{
    if (descriptor != nullptr)
        return descriptor->getStorageKey();

    if (propertyName == styleContextFilterPropertyId)
        return styleContextFilterStorageId;

    return propertyName;
}

/* The compiled bundle of a shader is the file next to its source, with `.ysl` added to its name: nothing for a path that is a bundle already. */
String getShaderBundlePath (const String& sourcePath)
{
    return sourcePath.endsWithIgnoreCase (".ysl") ? String() : sourcePath + ".ysl";
}

/* Tracks one component state property (style, cursor, effect, paint margin) owned by a
   stylesheet while leaving later explicit application overrides untouched. */
template <class T>
struct StyledOverride
{
    std::optional<T> applied;
    T saved {};

    template <class Getter>
    bool isActive (const Component& component, Getter&& get) const
    {
        return applied.has_value() && get (component) == *applied;
    }

    template <class Getter>
    T base (const Component& component, Getter&& get) const
    {
        return isActive (component, std::forward<Getter> (get)) ? saved : get (component);
    }

    template <class Getter, class Setter>
    bool apply (Component& component, T next, Getter&& get, Setter&& set)
    {
        const auto current = get (component);

        if (! (applied.has_value() && current == *applied))
            saved = current;

        applied = std::move (next);

        if (current == *applied)
            return false;

        set (component, *applied);
        return true;
    }

    template <class Getter, class Setter>
    bool restore (Component& component, Getter&& get, Setter&& set)
    {
        if (! applied.has_value())
            return false;

        const auto current = get (component);
        const bool shouldRestore = current == *applied && current != saved;

        if (current == *applied)
            set (component, saved);

        applied.reset();
        saved = T {};
        return shouldRestore;
    }
};

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

const StyleSheet::Declaration* findWinningDeclaration (const auto& declarations, const Identifier& name)
{
    const auto it = declarations.find (name);
    return it != declarations.end() ? it->second.declaration : nullptr;
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
    std::vector<DrawingScope::Dependency> dependencies;
    bool paletteDependent = false;
    bool failed = false;
};

/* Everything the context keeps for a component it visited, styled or not, so later restyles transition. */
struct StyleContext::ComponentCache
{
    WeakReference<Component> component;
    std::unordered_map<ValueKey, CompiledValue, ValueKeyHash> values;
    NamedValueSet savedState;

    String animationValue;
    bool paletteDependent = false;

    String drawingName;
    DrawingStyle::Ptr drawingStyle;
    StyledOverride<ComponentStyle::Ptr> drawingOverride;

    String effectName;
    StyledOverride<ComponentEffect::Ptr> effectOverride;
    StyledOverride<MouseCursor> cursorOverride;
    StyledOverride<EdgeSpacing<float>> paintMarginOverride;
};

struct StyleContext::CacheMap : std::map<Component*, ComponentCache>
{
};

struct StyleContext::PopupMap : std::map<Component*, std::pair<WeakReference<Component>, WeakReference<Component>>>
{
};

/* A shader of the stylesheet as it was loaded last, with the defaults of its properties as the declarations they are applied as, and what
   prevented its last load. One that could not be loaded is kept too, without a compiled shader, so that it is read once. */
struct StyleContext::ShaderEntry
{
    Shader loaded;
    std::vector<StyleSheet::Declaration> defaults;
    std::vector<DataTreeDiagnostic> problems;

    // Set when a file of the shader changed, for the next restyle to load it again
    bool isStale = false;

    // Set when the problems of the stylesheet are found again, for the next restyle to report the ones kept here with them
    bool reportsAgain = false;
};

struct StyleContext::ShaderMap : std::map<String, ShaderEntry>
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
    , popups (std::make_unique<PopupMap>())
    , shaders (std::make_unique<ShaderMap>())
{
    observer.onChanged = [this] (Component& component, const Identifier& property)
    {
        invalidateDependencies (component, property);
    };
    observer.onDeleted = [this] (Component& component)
    {
        for (auto& [ptr, cache] : *caches)
            for (auto& [key, compiled] : cache.values)
                for (auto& dependency : compiled.dependencies)
                    if (dependency.component == &component)
                        dependency.component = nullptr;
    };

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

    if (&component == &root)
    {
        std::vector<WeakReference<Component>> externalPopups;

        for (auto it = popups->begin(); it != popups->end();)
        {
            if (auto* popup = it->second.first.get())
            {
                if (! isManagedByRoot (*popup, root))
                    externalPopups.push_back (it->second.first);

                ++it;
            }
            else
            {
                it = popups->erase (it);
            }
        }

        for (const auto& popupRef : externalPopups)
            if (auto* popup = popupRef.get())
                forEachManaged (*popup, fn);
    }
}

bool StyleContext::isManagedPopup (const Component& component) const noexcept
{
    if (const auto it = popups->find (const_cast<Component*> (&component)); it != popups->end())
        return it->second.first.get() == &component;

    return false;
}

Component* StyleContext::findPopupOwner (const Component& component) const noexcept
{
    if (const auto it = popups->find (const_cast<Component*> (&component));
        it != popups->end() && it->second.first.get() == &component)
    {
        return it->second.second.get();
    }

    return nullptr;
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

void StyleContext::resetCompiledValues()
{
    for (auto& [ptr, cache] : *caches)
    {
        clearCacheValues (cache);

        if (auto* comp = cache.component.get())
        {
            restoreDrawing (*comp, cache);
        }
        else
        {
            cache.drawingName = {};
            cache.drawingStyle = nullptr;
            cache.drawingOverride = {};
        }
    }

    // What prevents a shader from loading is reported with the values, so it is again once they are compiled again
    for (auto& [name, entry] : *shaders)
        entry.reportsAgain = true;

    if (observerDirty)
        rebuildObserver();

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
    {
        markDirty (component);
    }
    else if (! isRestyling && name == paletteProperty)
    {
        if (const auto* cache = findCache (component); cache != nullptr && cache->paletteDependent)
            markDirty (component);
    }
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
    refreshShaders();

    for (int pass = 0; pass < 4 && ! dirtyComponents.isEmpty(); ++pass)
    {
        const LayoutComponent::ScopedLayoutBatch layouts;

        auto pending = std::move (dirtyComponents);
        dirtyComponents.clear();

        std::vector<Component*> ordered;
        std::set<Component*> candidates;

        for (const auto& dirty : pending)
        {
            auto* component = dirty.get();

            if (component == nullptr || (! isManagedByRoot (*component, root) && ! isManagedPopup (*component)))
                continue;

            if (candidates.insert (component).second)
                ordered.push_back (component);
        }

        const bool rootRestyled = candidates.find (&root) != candidates.end();

        if (! rootRestyled)
        {
            for (const auto& [popupKey, entry] : *popups)
            {
                auto* popup = entry.first.get();
                auto* owner = entry.second.get();

                if (popup == nullptr || owner == nullptr || candidates.find (popup) != candidates.end())
                    continue;

                if (candidates.find (owner) != candidates.end() || hasAncestorInSet (*owner, root, candidates))
                {
                    candidates.insert (popup);
                    ordered.push_back (popup);
                }
            }
        }

        for (auto* component : ordered)
        {
            if ((rootRestyled && component != &root) || hasAncestorInSet (*component, root, candidates))
                continue;

            restyleSubtree (*component, collectInherited (*component));
        }
    }

    if (observerDirty)
        rebuildObserver();

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

    if (const auto* menu = dynamic_cast<const PopupMenu*> (&component); menu != nullptr && menu->isSubmenu())
        return;

    DeclarationMap inheritedForChildren;
    if (isManagedPopup (component))
        applyToComponent (component, collectInherited (component), inheritedForChildren);
    else
        applyToComponent (component, inherited, inheritedForChildren);

    if (auto* menu = dynamic_cast<PopupMenu*> (&component))
        menu->refreshSubmenuStyle();

    for (int i = 0; i < component.getNumChildComponents(); ++i)
        if (auto* child = component.getChildComponent (i))
            restyleSubtree (*child, inheritedForChildren);

    if (&component == &root)
    {
        std::vector<WeakReference<Component>> externalPopups;

        for (auto it = popups->begin(); it != popups->end();)
        {
            if (auto* popup = it->second.first.get())
            {
                if (! isManagedByRoot (*popup, root))
                    externalPopups.push_back (it->second.first);

                ++it;
            }
            else
            {
                it = popups->erase (it);
            }
        }

        for (const auto& popupRef : externalPopups)
            if (auto* popup = popupRef.get())
                restyleSubtree (*popup, collectInherited (*popup));
    }
}

StyleContext::DeclarationMap StyleContext::collectInherited (Component& component) const
{
    std::vector<Component*> chain;

    auto* start = findPopupOwner (component);
    if (start == nullptr)
        start = component.getParentComponent();

    for (auto* c = start; c != nullptr; c = c->getParentComponent())
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

    auto* parentOverride = findPopupOwner (component);

    for (const auto& rule : styleSheet->getRules())
    {
        if (! rule.selector.matches (component, &root, parentOverride))
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

    // A DrawingStyle set on the component itself takes precedence over the stylesheet rules. It runs the shaders of the
    // sheet, whether a rule styles the component or not
    static constexpr auto getComponentStyle = [] (const Component& c) { return c.getStyle(); };
    const auto* explicitDrawing = ! cache.drawingOverride.isActive (component, getComponentStyle)
                                    ? dynamic_cast<const DrawingStyle*> (component.getStyle().get())
                                    : nullptr;

    if (explicitDrawing != nullptr)
        explicitDrawing = attachDrawingShaders (component, *explicitDrawing);

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
        if (const auto* styleState = component.styleState.get())
        {
            const auto& computed = styleState->getComputedValues();
            for (int i = 0; i < computed.size(); ++i)
                recordStoredLayoutEffect (computed.getName (i));

            const auto& animated = styleState->getAnimatedValues();
            for (int i = 0; i < animated.size(); ++i)
                recordStoredLayoutEffect (animated.getName (i));
        }

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
    static const Identifier cursorDeclaration ("cursor");
    static const Identifier effectDeclaration ("effect");
    static const Identifier filterDeclaration ("filter");
    static const Identifier shadowDeclaration ("shadow");
    static const Identifier glowDeclaration ("glow");

    const auto* paletteDecl = findWinningDeclaration (direct, paletteDeclaration);
    const auto* drawingDecl = findWinningDeclaration (direct, drawingDeclaration);
    const auto* transitionDecl = findWinningDeclaration (direct, transitionDeclaration);
    const auto* animationDecl = findWinningDeclaration (direct, animationDeclaration);
    const auto* cursorDecl = findWinningDeclaration (direct, cursorDeclaration);
    const auto* effectDecl = findWinningDeclaration (direct, effectDeclaration);
    const auto* filterDecl = findWinningDeclaration (direct, filterDeclaration);
    const auto* shadowDecl = findWinningDeclaration (direct, shadowDeclaration);
    const auto* glowCandidate = findWinningDeclaration (direct, glowDeclaration);
    const auto* glowDecl = (glowCandidate != nullptr && glowCandidate->kind == StyleSheet::Declaration::Kind::glow)
                             ? glowCandidate
                             : nullptr;

    bool changed = false;
    bool paletteChanged = false;
    bool paletteDependent = false;
    Array<Identifier> written;

    // The palette is resolved first, so that the values of this component read the right roles
    if (paletteDecl != nullptr)
    {
        const auto* versions = styleSheet != nullptr ? styleSheet->findPaletteVersions (paletteDecl->parsedName) : nullptr;

        if (versions == nullptr)
        {
            report ({ .message = "Unknown palette '" + paletteDecl->parsedName + "'", .element = paletteDecl->element });
        }
        else
        {
            paletteChanged = state.setStylePalette (*versions->findForTheme());

            paletteDependent = versions->followsTheme();
        }
    }
    else if (auto* owner = findPopupOwner (component);
             owner != nullptr && (component.getParentComponent() == nullptr || component.getParentComponent()->findPalette() == nullptr))
    {
        if (const auto* ownerPalette = owner->findPalette())
            paletteChanged = state.setStylePalette (*ownerPalette);
        else
            paletteChanged = state.setStylePalette (std::nullopt);
    }
    else
    {
        paletteChanged = state.setStylePalette (std::nullopt);
    }

    changed |= paletteChanged;

    // The drawing comes next, the properties it declares type the declarations setting them
    const DrawingStyle* drawing = explicitDrawing;

    if (drawing == nullptr && drawingDecl != nullptr)
        drawing = installDrawing (component, cache, *drawingDecl);
    else if (drawing == nullptr)
        restoreDrawing (component, cache);

    // The transitions of the new style apply, starting from the values shown before any is written
    std::vector<PendingTransition> pendingTransitions;

    if (transitionDecl != nullptr)
    {
        if (transitionDecl->parseError.isNotEmpty())
        {
            report ({ .message = transitionDecl->parseError, .element = transitionDecl->element });
        }
        else
        {
            for (const auto& transition : transitionDecl->transitions)
            {
                const auto* descriptor = findDescriptor (component, drawing, transition.property);
                const auto storageId = resolveStyleStorageId (descriptor, transition.property);
                var from;

                if (! firstStyle)
                {
                    if (storageId == styleContextTransformId)
                    {
                        from = VariantConverter<AffineTransform>::toVar (component.styleState != nullptr ? component.styleState->getStyleTransform() : AffineTransform());
                    }
                    else if (storageId == styleContextFilterStorageId)
                    {
                        const auto* currentFilter = component.styleState != nullptr ? component.styleState->getStyleValue (styleContextFilterStorageId) : nullptr;
                        from = currentFilter != nullptr ? *currentFilter : FilterEffect::operationsToVar ({});
                    }
                    else
                    {
                        from = component.findProperty (transition.property);
                    }
                }

                pendingTransitions.push_back ({ transition, descriptor, storageId, std::move (from) });
            }
        }
    }

    // The defaults of the shader selected as the effect are the declarations of lowest priority, for what the component doesn't hold itself
    if (effectDecl != nullptr)
    {
        if (const auto* shaderEntry = findShaderEntry (effectDecl->parsedName))
        {
            for (const auto& fallback : shaderEntry->defaults)
                if (! typeInfo.hasProperty (fallback.name))
                    direct.try_emplace (fallback.name, ResolvedDeclaration { &fallback, -1, -1, false });
        }
    }

    std::vector<std::pair<Identifier, ResolvedDeclaration>> orderedDeclarations (direct.begin(), direct.end());
    std::sort (orderedDeclarations.begin(), orderedDeclarations.end(), [] (const auto& a, const auto& b)
    {
        return a.second.comesBeforeInSource (b.second);
    });

    for (const auto& [name, resolved] : orderedDeclarations)
    {
        const auto& declaration = *resolved.declaration;
        if (declaration.kind != StyleSheet::Declaration::Kind::property)
            continue;

        const auto* descriptor = findDescriptor (component, drawing, name);
        const auto storageId = descriptor != nullptr ? descriptor->getStorageKey() : name;

        if (descriptor != nullptr && ! descriptor->isStyleable)
        {
            report ({ .message = "The property '" + name.toString() + "' can't be styled", .element = declaration.element });
            continue;
        }

        var value;
        bool valuePaletteDependent = false;

        if (! evaluate (component,
                        cache,
                        { &declaration, -1 },
                        declaration.element,
                        storageId,
                        descriptor != nullptr ? std::optional (descriptor->type) : std::nullopt,
                        declaration.value,
                        value,
                        valuePaletteDependent,
                        descriptor))
        {
            continue;
        }

        static const Identifier shadowCornerRadiusName ("cornerRadius");
        if (descriptor == nullptr && name != shadowCornerRadiusName)
        {
            report ({
                .message = "Undeclared property '" + name.toString() + "' on " + typeInfo.getTypeName().toString(),
                .element = declaration.element,
                .severity = DataTreeDiagnostic::Severity::warning
            });
        }

        paletteDependent |= valuePaletteDependent;
        written.add (storageId);

        if (state.setComputedValue (storageId, value))
        {
            changed = true;
            recordLayoutEffect (descriptor);
        }
    }

    if (filterDecl != nullptr)
    {
        std::vector<FilterEffect::Operation> filterOperations;

        if (evaluateFilterDeclaration (component, cache, *filterDecl, filterOperations, &paletteDependent))
        {
            written.add (styleContextFilterStorageId);
            changed |= state.setComputedValue (styleContextFilterStorageId, FilterEffect::operationsToVar (filterOperations));
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
    animatedChanged |= updateAnimations (component, cache, animationDecl, drawing, touched);

    if (animatedChanged)
    {
        changed = true;

        for (const auto& storageId : touched)
            recordStoredLayoutEffect (storageId);
    }

    // A property the component holds itself can only be styled through its setter
    for (const auto& storageId : touched)
        syncStateBacked (component, cache, storageId);

    changed |= applyEffects (component, cache, cursorDecl, effectDecl, shadowDecl, glowDecl, &paletteDependent);
    cache.paletteDependent = paletteDependent;

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
                             const DataTree& element,
                             const Identifier& storageId,
                             std::optional<ComponentProperty::Type> type,
                             const String& valueText,
                             var& result,
                             bool& paletteDependent,
                             const ComponentProperty* descriptor)
{
    auto [it, inserted] = cache.values.try_emplace (key);
    auto& entry = it->second;

    // A value that failed to compile is kept, so that it isn't compiled and reported again until its type or declaration changes
    if (inserted || entry.type != type)
    {
        if (! inserted && ! entry.dependencies.empty())
            observerDirty = true;

        entry = {};
        entry.type = type;

        const auto isEnum = type == ComponentProperty::Type::enumeration && descriptor != nullptr;

        // The name of a drawing or of a shader of the sheet is written bare, as an element writes it
        const auto isName = type == ComponentProperty::Type::drawing;
        const auto named = isName ? valueText.unquoted().trim() : String();

        if (isEnum)
        {
            const auto enumValue = valueText.unquoted().trim();

            if (descriptor->enumValues.contains (enumValue))
            {
                entry.constantValue = enumValue;
                paletteDependent = false;
                result = entry.constantValue;
                return true;
            }
        }

        if (isName)
        {
            const bool isDeclared = styleSheet != nullptr && (styleSheet->findDrawing (named).isValid() || styleSheet->findShader (named).isValid());

            if (isDeclared || named.isEmpty() || named == "none")
            {
                entry.constantValue = isDeclared ? named : String();
                paletteDependent = false;
                result = entry.constantValue;
                return true;
            }
        }

        const auto compileType = (isEnum || isName) ? std::optional (ComponentProperty::Type::string) : type;
        auto compiled = PropertyExpression::compile (component, valueText, compileType, componentResolver);

        if (compiled.failed())
        {
            entry.failed = true;
            String label = storageId.toString();

            if (key.layer >= 0)
            {
                const auto* ownerDecl = key.declaration;
                const auto prefix = ownerDecl != nullptr && ownerDecl->kind == StyleSheet::Declaration::Kind::glow
                                      ? "glow["
                                      : (ownerDecl != nullptr && ownerDecl->kind == StyleSheet::Declaration::Kind::filter
                                             ? "filter["
                                             : "shadow[");
                label = String (prefix) + String (key.layer) + "]";
            }

            const auto detail = isEnum ? ("expected one of " + descriptor->enumValues.joinIntoString (", "))
                                       : (isName ? ("unknown drawing or shader '" + named + "'") : compiled.getErrorMessage());
            report ({ .message = label + ": " + detail, .element = element });
            return false;
        }

        auto expr = std::move (compiled.getReference());
        entry.paletteDependent = expr->readsPalette();

        if (! expr->hasInputs())
        {
            entry.constantValue = expr->evaluate();

            if (isEnum && ! descriptor->enumValues.contains (entry.constantValue.toString()))
            {
                entry.failed = true;
                report ({ .message = storageId.toString() + ": expected one of " + descriptor->enumValues.joinIntoString (", "), .element = element });
                return false;
            }
        }
        else
        {
            entry.dependencies = expr->getDependencies();
            observer.observe (entry.dependencies);
            entry.expression = std::move (expr);
        }
    }

    if (entry.failed)
        return false;

    paletteDependent = entry.paletteDependent;
    result = entry.expression != nullptr ? entry.expression->evaluate() : entry.constantValue;

    if (type == ComponentProperty::Type::enumeration && descriptor != nullptr && ! descriptor->enumValues.contains (result.toString()))
        return false;

    return ! result.isVoid();
}

void StyleContext::syncStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId)
{
    if (storageId == styleContextTransformId)
    {
        const auto* styled = component.styleState != nullptr ? component.styleState->getStyleValue (storageId) : nullptr;
        component.setStyleTransform (styled != nullptr ? VariantConverter<AffineTransform>::fromVar (*styled) : AffineTransform());
        return;
    }

    // The paint margin a rule sets is widened by what the shadows and the filter reach, so it is set with them
    if (storageId == styleContextFilterStorageId || storageId == styleContextPaintMarginId)
    {
        syncStyledFilterAndMargins (component, cache);
        return;
    }

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

const DrawingStyle* StyleContext::installDrawing (Component& component, ComponentCache& cache, const StyleSheet::Declaration& declaration)
{
    static constexpr auto getStyle = [] (const Component& c) { return c.getStyle(); };
    static constexpr auto setStyle = [] (Component& c, ComponentStyle::Ptr style) { c.setStyle (std::move (style)); };

    const auto& name = declaration.parsedName;

    // The drawing installed is kept while it runs the shaders the sheet has loaded
    if (cache.drawingName == name
        && cache.drawingStyle != nullptr
        && cache.drawingOverride.isActive (component, getStyle)
        && ! holdsOtherShaders (*cache.drawingStyle))
    {
        return cache.drawingStyle.get();
    }

    const auto drawing = styleSheet != nullptr ? styleSheet->findDrawing (name) : DataTree();

    if (! drawing.isValid())
    {
        report ({ .message = "Unknown drawing '" + name + "'", .element = declaration.element });
        restoreDrawing (component, cache);
        return nullptr;
    }

    const auto findDrawing = [this] (const String& drawingName)
    {
        return styleSheet != nullptr ? styleSheet->findDrawing (drawingName) : DataTree();
    };

    DataTreeDiagnostic failure;
    auto created = DrawingStyle::create (component, drawing, componentResolver, &failure, findDrawing, bindFront (&StyleContext::findDrawingShader, this));

    if (created.failed())
    {
        report ({
            .message = "drawing " + name + ": " + created.getErrorMessage(),
            .element = failure.element.isValid() ? failure.element : drawing,
            .property = failure.property
        });
        restoreDrawing (component, cache);
        return nullptr;
    }

    clearCacheValues (cache);

    cache.drawingName = name;
    cache.drawingStyle = created.getReference();
    cache.drawingOverride.apply (component, ComponentStyle::Ptr (cache.drawingStyle.get()), getStyle, setStyle);
    return cache.drawingStyle.get();
}

const ComponentProperty* StyleContext::findDescriptor (const Component& component, const DrawingStyle* drawing, const Identifier& name) const
{
    if (const auto* descriptor = component.getTypeInfo().findProperty (name))
        return descriptor;

    if (drawing != nullptr)
        for (const auto& property : drawing->getProperties())
            if (property.name == name)
                return &property;

    for (const auto* ancestor = component.getParentComponent(); ancestor != nullptr; ancestor = ancestor->getParentComponent())
    {
        const auto* candidate = ancestor->getTypeInfo().findProperty (name);
        if (candidate != nullptr && candidate->isStoreBacked() && candidate->isInherited)
            return candidate;
    }

    if (styleSheet != nullptr)
        if (const auto* declared = styleSheet->findDeclaredProperty (name))
            return declared;

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
    if (cache.drawingStyle == nullptr && ! cache.drawingOverride.applied.has_value())
        return;

    clearCacheValues (cache);

    cache.drawingOverride.restore (component,
                                   [] (const Component& c) { return c.getStyle(); },
                                   [] (Component& c, ComponentStyle::Ptr style) { c.setStyle (std::move (style)); });
    cache.drawingName = {};
    cache.drawingStyle = nullptr;
}

/* Finds a shader of the sheet for a drawing using it: its block, and the shader it is loaded as the first time it is asked
   for, which reports what prevents it. One that is not loaded leaves the drawing running nothing where it uses it. */
DrawingStyle::Shader StyleContext::findDrawingShader (const String& name)
{
    DrawingStyle::Shader found;
    found.block = styleSheet != nullptr ? styleSheet->findShader (name) : DataTree();

    if (const auto* loaded = findShader (name))
        found.shader = loaded->shader;

    return found;
}

/* Tells whether a drawing runs a shader of the sheet as another one than the sheet has loaded: none, for a drawing made
   before the shader was there, or the one its file was loaded as before it changed. */
bool StyleContext::holdsOtherShaders (const DrawingStyle& drawing)
{
    for (const auto& used : drawing.getShaders())
    {
        const auto name = used.block.getProperty ("name").toString();

        // A shader of another block is not one of the sheet, whatever its name: nothing is loaded for it
        if (styleSheet == nullptr || ! styleSheet->findShader (name).isEquivalentTo (used.block))
            continue;

        if (const auto* loaded = findShader (name); loaded != nullptr && loaded->shader != used.shader)
            return true;
    }

    return false;
}

/* A drawing set on the component itself, by its element or from C++, runs the shaders of the sheet as one a rule selects
   does: it is made again with the ones the sheet has loaded when it holds others, which repaints the component. The
   builder of a document makes such a drawing before the sheet is there, with no shader at all. */
const DrawingStyle* StyleContext::attachDrawingShaders (Component& component, const DrawingStyle& drawing)
{
    if (! holdsOtherShaders (drawing))
        return &drawing;

    DataTreeDiagnostic failure;
    const auto remade = drawing.createWithShaders (bindFront (&StyleContext::findDrawingShader, this), &failure);

    if (remade.failed())
    {
        report ({ .message = "drawing " + drawing.getName() + ": " + remade.getErrorMessage(), .element = failure.element, .property = failure.property });
        return &drawing;
    }

    component.setStyle (remade.getReference());
    return remade.getReference().get();
}

bool StyleContext::evaluateFilterDeclaration (Component& component,
                                              ComponentCache& cache,
                                              const StyleSheet::Declaration& filterDecl,
                                              std::vector<FilterEffect::Operation>& outOperations,
                                              bool* paletteDependent)
{
    outOperations.clear();

    if (filterDecl.parseError.isNotEmpty())
    {
        report ({ .message = filterDecl.parseError, .element = filterDecl.element });
        return false;
    }

    outOperations.reserve (filterDecl.filters.size());

    for (int stepIndex = 0; stepIndex < static_cast<int> (filterDecl.filters.size()); ++stepIndex)
    {
        const auto& step = filterDecl.filters[static_cast<size_t> (stepIndex)];
        FilterEffect::Operation op {
            .type = step.type,
            .amount = step.amount,
            .offset = step.offset,
            .color = step.color
        };

        if (step.amountExpression.isNotEmpty())
        {
            var amountVar;
            bool readsPalette = false;

            if (! evaluate (component,
                            cache,
                            { &filterDecl, stepIndex },
                            filterDecl.element,
                            {},
                            ComponentProperty::Type::number,
                            step.amountExpression,
                            amountVar,
                            readsPalette))
            {
                outOperations.clear();
                return false;
            }

            if (paletteDependent != nullptr)
                *paletteDependent |= readsPalette;

            op.amount = FilterEffect::clampAmount (op.type, static_cast<float> (static_cast<double> (amountVar)));
        }
        else if (step.colorExpression.isNotEmpty())
        {
            var colorVar;
            bool readsPalette = false;

            if (! evaluate (component,
                            cache,
                            { &filterDecl, stepIndex },
                            filterDecl.element,
                            {},
                            ComponentProperty::Type::color,
                            step.colorExpression,
                            colorVar,
                            readsPalette))
            {
                outOperations.clear();
                return false;
            }

            if (paletteDependent != nullptr)
                *paletteDependent |= readsPalette;

            op.color = Color (static_cast<uint32> (static_cast<int64> (colorVar)));
        }

        outOperations.push_back (op);
    }

    return true;
}

bool StyleContext::syncStyledFilterAndMargins (Component& component, ComponentCache& cache)
{
    static constexpr auto getMargin = [] (const Component& c) { return c.getPaintMargin(); };
    static constexpr auto setMargin = [] (Component& c, const EdgeSpacing<float>& margin) { c.setPaintMargin (margin); };

    bool changed = false;

    const auto* styledFilter = component.styleState != nullptr ? component.styleState->getStyleValue (styleContextFilterStorageId) : nullptr;
    auto filterOperations = styledFilter != nullptr ? FilterEffect::operationsFromVar (*styledFilter) : std::vector<FilterEffect::Operation> {};

    if (component.styleState != nullptr || ! filterOperations.empty())
    {
        if (component.getOrCreateStyleState().setFilterOperations (std::move (filterOperations)))
        {
            component.releaseUnusedEffectCanvases();
            changed = true;
        }
    }

    const auto* activeState = component.styleState.get();
    const auto filterMargin = (activeState != nullptr && activeState->getFilterEffect() != nullptr)
                                ? activeState->getFilterEffect()->getRequiredMargin()
                                : EdgeSpacing<float>();
    const bool hasShadows = activeState != nullptr && ! activeState->getShadows().empty();
    const bool hasGlows = activeState != nullptr && ! activeState->getGlows().empty();
    const bool hasFilterMargin = filterMargin != EdgeSpacing<float>();
    const auto* styledMargin = activeState != nullptr ? activeState->getStyleValue (styleContextPaintMarginId) : nullptr;

    if (! hasShadows && ! hasGlows && ! hasFilterMargin && styledMargin == nullptr)
    {
        changed |= cache.paintMarginOverride.restore (component, getMargin, setMargin);
    }
    else
    {
        // A rule sets the margin in place of the one the component was given
        const auto base = styledMargin != nullptr ? LayoutComponent::parseSpacing (*styledMargin)
                                                  : cache.paintMarginOverride.base (component, getMargin);
        auto left = base.getLeft();
        auto top = base.getTop();
        auto right = base.getRight();
        auto bottom = base.getBottom();

        const auto expandMargin = [&] (const std::vector<StyleState::Shadow>& layers)
        {
            for (const auto& layer : layers)
            {
                const auto reach = layer.blur + jmax (0.0f, layer.spread);
                left = jmax (left, reach - layer.offset.getX());
                top = jmax (top, reach - layer.offset.getY());
                right = jmax (right, reach + layer.offset.getX());
                bottom = jmax (bottom, reach + layer.offset.getY());
            }
        };

        if (activeState != nullptr)
        {
            expandMargin (activeState->getShadows());
            expandMargin (activeState->getGlows());
        }

        // The filter runs over what is painted in the margin and over the shadows, and spreads them further
        changed |= cache.paintMarginOverride.apply (component, EdgeSpacing<float> (left, top, right, bottom) + filterMargin, getMargin, setMargin);
    }

    return changed;
}

//==============================================================================

const StyleContext::Shader* StyleContext::findShader (const String& name)
{
    const auto* entry = findShaderEntry (name);
    return entry != nullptr ? &entry->loaded : nullptr;
}

StyleContext::ShaderEntry* StyleContext::findShaderEntry (const String& name)
{
    auto found = shaders->find (name);

    if (found == shaders->end())
    {
        const auto block = styleSheet != nullptr ? styleSheet->findShader (name) : DataTree();

        // A material is not drawn over a picture: what uses one as such tells
        if (! block.isValid() || ! block.hasProperty ("fragment"))
            return nullptr;

        found = shaders->try_emplace (name).first;

        auto& entry = found->second;
        entry.loaded.block = block;

        for (const auto& property : block)
        {
            const auto propertyName = property.getProperty ("name").toString().trim();
            const auto value = property.getProperty ("value").toString().trim();

            if (property.getType().toString() != "property" || propertyName.isEmpty() || value.isEmpty())
                continue;

            auto& fallback = entry.defaults.emplace_back();
            fallback.name = Identifier (propertyName);
            fallback.cascadeKey = fallback.name;
            fallback.value = value;
            fallback.element = property;
        }

        loadShader (name, entry);
    }

    auto& entry = found->second;
    return entry.loaded.shader != nullptr ? &entry : nullptr;
}

bool StyleContext::loadShader (const String& name, ShaderEntry& entry)
{
    // What prevents the load is reported now, and kept for the restyles that report the problems of the sheet again
    entry.problems.clear();
    entry.isStale = false;
    entry.reportsAgain = false;

    const auto fail = [this, &entry] (DataTreeDiagnostic problem)
    {
        report (problem);
        entry.problems.push_back (std::move (problem));
        return false;
    };

    const auto& block = entry.loaded.block;
    const auto path = block.getProperty ("fragment").toString();
    const auto bundlePath = getShaderBundlePath (path);

    auto bytes = ComponentDocument::readAssetFor (&root, bundlePath);
    const bool hasBundle = bytes.has_value();

    if (! hasBundle)
        bytes = ComponentDocument::readAssetFor (&root, path);

    if (! bytes.has_value())
    {
        const auto missing = bundlePath.isNotEmpty() ? "'" + path + "' nor its compiled bundle '" + bundlePath + "'" : "'" + path + "'";

        return fail ({ .message = "The shader '" + name + "' can't read " + missing, .element = block, .property = "fragment" });
    }

    auto created = GpuImageShader::createFromData (bytes->getData(), bytes->getSize());

    if (created.failed())
    {
        auto message = "shader " + name + ": " + created.getErrorMessage();

#if ! YUP_ENABLE_SHADER_TRANSPILER
        // A source is not compiled here, so what is missing is the bundle a build makes of it
        if (! hasBundle && bundlePath.isNotEmpty())
            message << ", and there is no '" << bundlePath << "'";
#endif

        return fail ({ .message = message, .element = block, .property = "fragment" });
    }

    const auto uniforms = created.getReference()->createUniforms();
    std::vector<ShaderEffect::Property> properties;

    for (const auto& property : block)
    {
        const auto propertyName = property.getProperty ("name").toString().trim();

        if (property.getType().toString() != "property" || propertyName.isEmpty())
            continue;

        const auto typeName = property.getProperty ("type").toString().trim();
        const auto type = ComponentProperty::getTypeFromName (typeName);

        // Of the arrays, the one of numbers fills a uniform
        const bool isFilling = type != ComponentProperty::Type::array
                            || ComponentProperty::getElementTypeFromName (typeName) == ComponentProperty::Type::number;

        const auto numComponents = isFilling ? ShaderEffect::getNumUniformComponents (type) : 0;
        String problem;

        if (numComponents == 0)
            problem = "The property '" + propertyName + "' of the shader '" + name + "' can't be of type '" + typeName + "': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]";
        else if (! uniforms.hasMember (propertyName))
            problem = "The uniform block of the shader '" + name + "' has no member '" + propertyName + "'";
        else if (uniforms.getNumComponents (propertyName) < numComponents)
            problem = "The member '" + propertyName + "' of the uniform block of the shader '" + name + "' is too small for a " + typeName;

        if (problem.isEmpty())
            properties.push_back ({ Identifier (propertyName), type });
        else
            fail ({ .message = problem, .element = property });
    }

    if (! entry.problems.empty())
        return false;

    entry.loaded.shader = created.getValue();
    entry.loaded.properties = std::move (properties);
    return true;
}

void StyleContext::refreshShaders()
{
    for (auto& [name, entry] : *shaders)
    {
        if (! entry.isStale)
        {
            if (std::exchange (entry.reportsAgain, false))
            {
                for (const auto& problem : entry.problems)
                    report (problem);
            }

            continue;
        }

        // A shader that fails to load leaves the one it was to replace running
        if (! loadShader (name, entry))
            continue;

        // The effects running the shader take the new one, unless it now reads more textures than an effect has: those are made again
        const bool runsAsEffect = entry.loaded.shader->getNumInputTextures() <= 1;

        for (auto& [component, cache] : *caches)
        {
            if (cache.effectName != name)
                continue;

            auto* effect = cache.effectOverride.applied.has_value() ? dynamic_cast<ShaderEffect*> (cache.effectOverride.applied->get()) : nullptr;

            if (runsAsEffect && effect != nullptr)
                effect->setShader (entry.loaded.shader);
            else
                cache.effectName = {};
        }
    }
}

void StyleContext::assetChanged (const String& path)
{
    if (path.isEmpty())
        return;

    for (auto& [name, entry] : *shaders)
    {
        const auto source = entry.loaded.block.getProperty ("fragment").toString();

        if (path != source && path != getShaderBundlePath (source))
            continue;

        // Loaded by the next restyle: once, however many of its files changed, and after a document that tells before it loads
        // cleared its problems, so that what prevents the load is one of them
        entry.isStale = true;
        markDirty (root);
    }
}

ComponentEffect::Ptr StyleContext::createEffect (Component& component, const StyleSheet::Declaration& effectDecl)
{
    const auto& name = effectDecl.parsedName;
    const auto block = styleSheet != nullptr ? styleSheet->findShader (name) : DataTree();

    // A shader of the sheet comes before a registered effect of the same name
    if (! block.isValid())
    {
        auto created = ComponentEffectRegistry::getInstance().createEffect (name);

        if (created == nullptr)
            report ({ .message = "Unknown effect '" + name + "'", .element = effectDecl.element });

        return created;
    }

    if (block.hasProperty ("material"))
    {
        report ({ .message = "The shader '" + name + "' is a material, an effect takes a shader with a 'fragment'", .element = effectDecl.element });
        return nullptr;
    }

    // What prevents a shader from loading was reported when it was loaded
    const auto* loaded = findShader (name);

    if (loaded == nullptr)
        return nullptr;

    if (loaded->shader->getNumInputTextures() > 1)
    {
        report ({ .message = "The shader '" + name + "' reads two textures, an effect gives it one", .element = effectDecl.element });
        return nullptr;
    }

    return ComponentEffect::Ptr (new ShaderEffect (component, loaded->shader, loaded->properties));
}

bool StyleContext::applyEffects (Component& component,
                                 ComponentCache& cache,
                                 const StyleSheet::Declaration* cursorDecl,
                                 const StyleSheet::Declaration* effectDecl,
                                 const StyleSheet::Declaration* shadowDecl,
                                 const StyleSheet::Declaration* glowDecl,
                                 bool* paletteDependent)
{
    static constexpr auto getCursor = [] (const Component& c) { return c.getMouseCursor(); };
    static constexpr auto setCursor = [] (Component& c, const MouseCursor& cursor) { c.setMouseCursor (cursor); };
    static constexpr auto getEffect = [] (const Component& c) { return c.getComponentEffect(); };
    static constexpr auto setEffect = [] (Component& c, ComponentEffect::Ptr effect) { c.setComponentEffect (std::move (effect)); };

    bool changed = false;

    if (cursorDecl != nullptr)
    {
        if (cursorDecl->parseError.isNotEmpty())
            report ({ .message = cursorDecl->parseError, .element = cursorDecl->element });

        if (cursorDecl->cursor.has_value())
            changed |= cache.cursorOverride.apply (component, MouseCursor (*cursorDecl->cursor), getCursor, setCursor);
        else
            changed |= cache.cursorOverride.restore (component, getCursor, setCursor);
    }
    else
    {
        changed |= cache.cursorOverride.restore (component, getCursor, setCursor);
    }

    if (effectDecl != nullptr && effectDecl->parsedName.isNotEmpty())
    {
        const auto& name = effectDecl->parsedName;

        if (! (cache.effectName == name && cache.effectOverride.isActive (component, getEffect)))
        {
            if (name == "none")
            {
                cache.effectName = name;
                changed |= cache.effectOverride.apply (component, ComponentEffect::Ptr {}, getEffect, setEffect);
            }
            else if (auto created = createEffect (component, *effectDecl); created != nullptr)
            {
                cache.effectName = name;
                changed |= cache.effectOverride.apply (component, std::move (created), getEffect, setEffect);
            }
            else
            {
                cache.effectName = {};
                changed |= cache.effectOverride.restore (component, getEffect, setEffect);
            }
        }
    }
    else
    {
        cache.effectName = {};
        changed |= cache.effectOverride.restore (component, getEffect, setEffect);
    }

    const auto evaluateShadowLayers = [&] (const StyleSheet::Declaration* decl)
    {
        std::vector<StyleState::Shadow> layers;

        if (decl == nullptr)
            return layers;

        if (decl->parseError.isNotEmpty())
        {
            report ({ .message = decl->parseError, .element = decl->element });
            return layers;
        }

        layers.reserve (decl->shadows.size());

        for (int layerIndex = 0; layerIndex < static_cast<int> (decl->shadows.size()); ++layerIndex)
        {
            const auto& layer = decl->shadows[static_cast<size_t> (layerIndex)];
            var colorVar;
            bool readsPalette = false;

            if (! evaluate (component,
                            cache,
                            { decl, layerIndex },
                            decl->element,
                            {},
                            ComponentProperty::Type::color,
                            layer.colorExpression,
                            colorVar,
                            readsPalette))
            {
                layers.clear();
                break;
            }

            if (paletteDependent != nullptr)
                *paletteDependent |= readsPalette;

            layers.push_back ({
                .offset = layer.offset,
                .blur = layer.blur,
                .spread = layer.spread,
                .color = Color (static_cast<uint32> (static_cast<int64> (colorVar)))
            });
        }

        return layers;
    };

    auto shadows = evaluateShadowLayers (shadowDecl);
    auto glows = evaluateShadowLayers (glowDecl);

    if (component.styleState != nullptr || ! shadows.empty() || ! glows.empty())
    {
        auto& state = component.getOrCreateStyleState();
        changed |= state.setShadows (std::move (shadows));
        changed |= state.setGlows (std::move (glows));
    }

    changed |= syncStyledFilterAndMargins (component, cache);
    return changed;
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
    for (const auto& [key, val] : cache.values)
    {
        if (! val.dependencies.empty())
        {
            observerDirty = true;
            break;
        }
    }

    cache.values.clear();
}

void StyleContext::clearAllCaches()
{
    observer.clear();
    observerDirty = false;
    caches->clear();
}

//==============================================================================

var StyleContext::getBaseValue (Component& component, const ComponentCache& cache, const ComponentProperty* descriptor, const Identifier& storageId)
{
    auto& state = component.getOrCreateStyleState();

    if (const auto* computed = state.getComputedValue (storageId))
        return *computed;

    if (storageId == styleContextTransformId)
        return VariantConverter<AffineTransform>::toVar (AffineTransform());

    if (storageId == styleContextFilterStorageId)
        return FilterEffect::operationsToVar ({});

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
                                     ComponentCache& cache,
                                     const StyleSheet::Declaration* animationDecl,
                                     const DrawingStyle* drawing,
                                     Array<Identifier>& touched)
{
    const String nextAnimation = animationDecl != nullptr ? animationDecl->parsedName : String();

    // The animations keep running until the declaration changes, which restarts them
    if (nextAnimation == cache.animationValue)
        return false;

    cache.animationValue = nextAnimation;

    auto& state = component.getOrCreateStyleState();
    bool changed = false;

    for (const auto& storageId : animator.stopAnimations (component))
    {
        if (animator.findTransitionTarget (component, storageId) == nullptr)
            changed |= state.removeAnimatedValue (storageId);

        touched.addIfNotAlreadyThere (storageId);
    }

    if (animationDecl == nullptr || nextAnimation.trim().isEmpty())
        return changed;

    if (animationDecl->parseError.isNotEmpty())
    {
        report ({ .message = animationDecl->parseError, .element = animationDecl->element });
        return changed;
    }

    for (const auto& animation : animationDecl->animations)
    {
        const auto* keyframes = styleSheet != nullptr ? styleSheet->findKeyframes (animation.name) : nullptr;

        if (keyframes == nullptr)
        {
            report ({ .message = "Unknown keyframes '" + animation.name.toString() + "'", .element = animationDecl->element });
            continue;
        }

        animator.startAnimation (component, animation, compileTracks (component, cache, *keyframes, drawing), clock);
    }

    return changed;
}

std::vector<StyleAnimator::Track> StyleContext::compileTracks (Component& component,
                                                               ComponentCache& cache,
                                                               const StyleSheet::KeyframeSequence& keyframes,
                                                               const DrawingStyle* drawing)
{
    struct CompiledTrack
    {
        StyleAnimator::Track track;
        const ComponentProperty* descriptor = nullptr;
    };

    std::vector<CompiledTrack> compiled;

    for (const auto& keyframe : keyframes.frames)
    {
        for (const auto& declaration : keyframe.declarations)
        {
            if (declaration.kind == StyleSheet::Declaration::Kind::filter)
            {
                std::vector<FilterEffect::Operation> ops;
                if (! evaluateFilterDeclaration (component, cache, declaration, ops, &cache.paletteDependent))
                    continue;

                auto existing = std::find_if (compiled.begin(), compiled.end(), [] (const auto& entry)
                {
                    return entry.track.storageId == styleContextFilterStorageId;
                });

                if (existing == compiled.end())
                {
                    compiled.push_back ({ { styleContextFilterStorageId, ComponentProperty::Type::any, {} }, nullptr });
                    existing = std::prev (compiled.end());
                }

                existing->track.frames.push_back ({ keyframe.position, FilterEffect::operationsToVar (ops) });
                continue;
            }

            const auto* descriptor = findDescriptor (component, drawing, declaration.name);

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

            auto expression = PropertyExpression::compile (component, declaration.value, type, componentResolver);

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
    bool changed = false;

    if (cache != nullptr)
    {
        cache->animationValue = {};
        cache->paletteDependent = false;
        restoreAllStateBacked (component, *cache);
        restoreDrawing (component, *cache);
        if (component.styleState != nullptr)
        {
            // What the effects read of the styled values goes before they are applied with none
            for (const auto& storageId : { styleContextFilterStorageId, styleContextPaintMarginId })
            {
                component.styleState->removeComputedValue (storageId);
                if (! keepAnimatedValues)
                    component.styleState->removeAnimatedValue (storageId);
            }
        }
        changed |= applyEffects (component, *cache, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    auto* state = component.styleState.get();
    if (state == nullptr)
        return changed;

    const auto shown = keepAnimatedValues ? state->getAnimatedValues() : NamedValueSet {};

    changed |= state->clearComputedValues();
    changed |= state->clearAnimatedValues();
    if (state->setFilterEffect (nullptr))
    {
        component.releaseUnusedEffectCanvases();
        changed = true;
    }
    changed |= state->setShadows ({});
    changed |= state->setGlows ({});
    const bool clearedPalette = state->setStylePalette (std::nullopt);
    changed |= clearedPalette;

    if (paletteCleared != nullptr)
        *paletteCleared = clearedPalette;

    for (int i = 0; i < shown.size(); ++i)
        changed |= state->setComputedValue (shown.getName (i), *shown.getVarPointerAt (i));

    const auto* styledTransform = state->getStyleValue (styleContextTransformId);
    const auto nextStyleTransform = styledTransform != nullptr ? VariantConverter<AffineTransform>::fromVar (*styledTransform) : AffineTransform();
    if (state->getStyleTransform() != nextStyleTransform)
    {
        component.setStyleTransform (nextStyleTransform);
        changed = true;
    }

    if (const auto* styledFilter = state->getStyleValue (styleContextFilterStorageId))
        changed |= state->setFilterOperations (FilterEffect::operationsFromVar (*styledFilter));

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

    popups->erase (&subtreeRoot);

    if (observerDirty)
        rebuildObserver();
}

void StyleContext::detach()
{
    forEachManaged (root, [this] (Component& component)
    {
        if (resetStyle (component, findCache (component)))
            component.repaint();
    });

    for (const auto& [popupKey, entry] : *popups)
    {
        if (auto* popup = entry.first.get(); popup != nullptr && ! isManagedByRoot (*popup, root))
            if (resetStyle (*popup, findCache (*popup)))
                popup->repaint();
    }

    popups->clear();
    clearAllCaches();
    shaders->clear();
    animator.clear();
    dirtyComponents.clear();
    diagnostics.clear();
}

void StyleContext::stylePopup (Component& popup, Component* owner)
{
    if (popup.getStyleContext() != nullptr)
        return;

    (*popups)[&popup] = { WeakReference<Component> (&popup), WeakReference<Component> (owner) };
    markDirty (popup);
    restyleNow();
}

void StyleContext::copyVisualStyle (const Component& source, Component& target)
{
    const auto* sourceState = source.styleState.get();
    if (sourceState == nullptr)
        return;

    const bool hasShadows = ! sourceState->getShadows().empty();
    const bool hasGlows = ! sourceState->getGlows().empty();
    const auto sourceFilter = sourceState->getFilterEffect();
    const auto sourceTransform = sourceState->getStyleTransform();
    const bool hasTransform = ! sourceTransform.isIdentity();

    if (! hasShadows && ! hasGlows && sourceFilter == nullptr && ! hasTransform)
        return;

    auto& targetState = target.getOrCreateStyleState();

    if (hasShadows)
        targetState.setShadows (sourceState->getShadows());

    if (hasGlows)
        targetState.setGlows (sourceState->getGlows());

    if (sourceFilter != nullptr)
        targetState.setFilterOperations (sourceFilter->getOperations());

    if (hasTransform)
        target.setStyleTransform (sourceTransform);
}

void StyleContext::clearVisualStyle (Component& target)
{
    if (target.styleState == nullptr)
        return;

    target.styleState->setShadows ({});
    target.styleState->setGlows ({});
    target.styleState->setFilterOperations ({});
    target.setStyleTransform ({});
    target.releaseUnusedEffectCanvases();
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
        if (const auto* cache = findCache (current); cache != nullptr && cache->paletteDependent)
            markDirty (current);
    });
}

void StyleContext::rebuildObserver()
{
    observerDirty = false;
    observer.clear();

    for (const auto& [ptr, cache] : *caches)
    {
        if (cache.component.get() != ptr)
            continue;

        for (const auto& [key, compiled] : cache.values)
            if (! compiled.dependencies.empty())
                observer.observe (compiled.dependencies);
    }
}

void StyleContext::invalidateDependencies (const Component& sourceComponent, const Identifier& property)
{
    for (const auto& [ptr, cache] : *caches)
    {
        auto* styled = cache.component.get();
        if (styled == nullptr || styled != ptr)
            continue;

        const bool matched = std::any_of (cache.values.begin(), cache.values.end(), [&] (const auto& entry)
        {
            const auto& dependencies = entry.second.dependencies;
            return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
            {
                return dependency.component == &sourceComponent && dependency.property == property;
            });
        });

        if (matched)
            markDirty (*styled);
    }
}

void StyleContext::report (const DataTreeDiagnostic& diagnostic)
{
    diagnostics.push_back (diagnostic);

    if (onDiagnostic != nullptr)
        onDiagnostic (diagnostic);
}

} // namespace yup
