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

/* Returns the graphics API of the window a component is in, or nothing when it is in none that draws with a GPU. */
std::optional<GpuPlatform> findWindowGraphicsApi (Component& component)
{
    const auto device = detail::findWindowDevice (&component);

    return device != nullptr && device->isGpuAvailable() ? std::optional<GpuPlatform> (device->getPlatform()) : std::nullopt;
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

int countAncestors (const Component& component)
{
    int count = 0;

    for (auto* parent = component.getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
        ++count;

    return count;
}

/* Tells whether a property is a state of the component alone: one a pseudo-class reads on the component, that each
   component it changes for is told about, and that no descendant takes from it as it would a value of its store. */
bool isOwnStyleState (const Component& component, const Identifier& name)
{
    const auto* descriptor = component.getTypeInfo().findProperty (name);

    // No property: each component the focus is within is told
    if (descriptor == nullptr)
        return name == StringRef ("focus-within");

    // The enablement is told to each descendant it changes for
    return descriptor->isStateBacked() && (descriptor->isState || name == StringRef ("enabled"));
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

    // The drawing made again here for one the component was given itself, kept to tell it from another one set since
    DrawingStyle::Ptr attachedDrawing;

    // What the context had loaded of its shaders when each of these drawings was last found to run them
    std::optional<uint32> drawingShaderGeneration;
    std::optional<uint32> attachedShaderGeneration;

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

    // What a device refused of the shader loaded, found by the paints of the components drawn with it: each is told once
    std::vector<DataTreeDiagnostic> drawProblems;

    // Set when a file of the shader changed, for the next restyle to load it again
    bool isStale = false;

    // Set when the problems of the stylesheet are found again, for the next restyle to report the ones kept here with them
    bool reportsAgain = false;

    // The graphics API the last load compiled the source of the shader for, whatever came of it, so that a source is
    // compiled once for an API: nothing for a bundle, loaded with the variants it holds, and for a source compiled for all
    std::optional<GpuPlatform> sourceApi;

    // Set once a paint in no window could not draw the shader, where nothing tells which graphics API paints: its source
    // is compiled for all of them from then on
    bool compilesForEveryApi = false;
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

bool StyleContext::isInsidePopup (const Component& component) const noexcept
{
    if (popups->empty())
        return false;

    for (auto* current = &component; current != nullptr; current = current->getParentComponent())
    {
        if (isManagedPopup (*current))
            return true;

        if (current == &root)
            break;
    }

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
            cache.attachedDrawing = nullptr;
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
    addDirtyComponent (component, true);
}

void StyleContext::addDirtyComponent (Component& component, bool withDescendants)
{
    // The frame that follows restyles the component before it is painted
    frameAttachment.setActive (true);

    if (! dirtyComponents.empty() && dirtyComponents.back().component.get() == &component)
    {
        dirtyComponents.back().withDescendants |= withDescendants;
        return;
    }

    dirtyComponents.push_back ({ WeakReference<Component> (&component), withDescendants });
}

void StyleContext::propertyChanged (Component& component, const Identifier& name)
{
    static const Identifier paletteProperty ("palette");

    if (styleSheet != nullptr && styleSheet->watchesProperty (name))
    {
        // A state the rules read on the component they match alone leaves its descendants to what its restyle changes
        addDirtyComponent (component, styleSheet->watchesPropertyForDescendants (name) || ! isOwnStyleState (component, name));
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

    // With every style applied and nothing left running, the window is free to skip its frames
    frameAttachment.setActive (! dirtyComponents.empty() || animator.isAnimating());
}

void StyleContext::themeChanged()
{
    frameAttachment.setActive (true);
}

void StyleContext::restyleNow()
{
    if (dirtyComponents.empty())
        return;

    const ScopedValueSetter<bool> restyling (isRestyling, true);
    diagnostics.clear();
    refreshShaders();

    for (int pass = 0; pass < 4 && ! dirtyComponents.empty(); ++pass)
    {
        const LayoutComponent::ScopedLayoutBatch layouts;

        auto pending = std::move (dirtyComponents);
        dirtyComponents.clear();

        std::vector<Component*> ordered;
        std::set<Component*> candidates;
        std::vector<std::pair<int, Component*>> states;
        std::set<Component*> stateCandidates;

        for (const auto& dirty : pending)
        {
            auto* component = dirty.component.get();

            if (component == nullptr || (! isManagedByRoot (*component, root) && ! isManagedPopup (*component)))
                continue;

            // A popup is styled as a child of its owner and not of its parent: what is in one takes its descendants along
            if (! dirty.withDescendants && ! isInsidePopup (*component))
            {
                if (stateCandidates.insert (component).second)
                    states.emplace_back (countAncestors (*component), component);

                continue;
            }

            if (candidates.insert (component).second)
                ordered.push_back (component);
        }

        // The components a state of their own changed on are restyled alone, the parents first. One whose style changed
        // takes its descendants along, which read it, and is then a component restyled with them like the others
        if (candidates.find (&root) == candidates.end())
        {
            std::stable_sort (states.begin(), states.end(), [] (const auto& a, const auto& b)
            {
                return a.first < b.first;
            });

            std::set<Component*> changed;

            for (const auto& state : states)
            {
                auto* component = state.second;

                if (candidates.find (component) != candidates.end() || hasAncestorInSet (*component, root, candidates))
                    continue;

                if (restyleSubtree (*component, collectInherited (*component), true))
                {
                    candidates.insert (component);
                    changed.insert (component);
                }
            }

            if (! changed.empty())
                markReadersDirty (changed);
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

/* Restyles a component, then its descendants: all of them, or only when the restyle changed the style of the component
   for one that is dirty by a state of its own. Returns whether the descendants were restyled. */
bool StyleContext::restyleSubtree (Component& component, const DeclarationMap& inherited, bool descendantsOnlyWhenChanged)
{
    if (&component != &root && component.getStyleContext() != nullptr)
        return false;

    if (const auto* menu = dynamic_cast<const PopupMenu*> (&component); menu != nullptr && menu->isSubmenu())
        return false;

    std::optional<DeclarationMap> popupInherited;
    const auto* base = &inherited;

    if (isManagedPopup (component))
        base = &popupInherited.emplace (collectInherited (component));

    // The children inherit what the component does, as it is, unless the component adds to it
    DeclarationMap extended;
    const bool changed = applyToComponent (component, *base, extended);
    const auto& inheritedForChildren = extended.empty() ? *base : extended;

    if (auto* menu = dynamic_cast<PopupMenu*> (&component))
        menu->refreshSubmenuStyle();

    // No rule reads the state for the descendants: it reaches them through what they read of the style of the component
    if (descendantsOnlyWhenChanged && ! changed)
        return false;

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

    return true;
}

StyleContext::DeclarationMap StyleContext::collectInherited (Component& component)
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

void StyleContext::collectDeclarations (Component& component, DeclarationMap& direct, DeclarationMap& qualified)
{
    if (styleSheet == nullptr)
        return;

    auto* parentOverride = findPopupOwner (component);

    // The rules the component can match, in the order of the sheet: the others need an id, a class or a type it has not
    styleSheet->findCandidateRules (component, candidateRules);

    for (const auto* rule : candidateRules)
    {
        if (! rule->selector.matches (component, &root, parentOverride))
            continue;

        const auto specificity = rule->selector.getSpecificity();

        for (const auto& declaration : rule->declarations)
        {
            const ResolvedDeclaration resolved { &declaration, specificity, rule->order, true };
            auto& target = declaration.qualifier.isValid() ? qualified : direct;
            const auto& key = declaration.cascadeKey.isValid() ? declaration.cascadeKey : declaration.name;

            mergeResolvedDeclaration (target, key, resolved);
        }
    }
}

/* Applies the rules matching a component to it. `extended` is left empty when the children of the component inherit
   what it does, and else receives what they inherit. Returns whether anything of the style of the component changed,
   which is what its descendants may read of it. */
bool StyleContext::applyToComponent (Component& component, const DeclarationMap& inherited, DeclarationMap& extended)
{
    // What its last paint found of the shaders the component is drawn with, before any of them is replaced
    reportShaderFailures (component);

    DeclarationMap direct, qualified;
    collectDeclarations (component, direct, qualified);

    // The qualified declarations of the ancestors keep flowing down, the closer ones winning: as they are, unless the
    // component has its own
    if (! qualified.empty())
    {
        extended = inherited;

        for (const auto& [key, declaration] : qualified)
            mergeResolvedDeclaration (extended, key, declaration);
    }

    const auto& inheritedForChildren = qualified.empty() ? inherited : extended;

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
        explicitDrawing = attachDrawingShaders (component, cache, *explicitDrawing);
    else
        cache.attachedDrawing = nullptr;

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
        // Every value the style holds is cleared below, with whatever a descendant read of them
        bool hadValues = false;

        if (const auto* styleState = component.styleState.get())
        {
            const auto& computed = styleState->getComputedValues();
            for (int i = 0; i < computed.size(); ++i)
                recordStoredLayoutEffect (computed.getName (i));

            const auto& animated = styleState->getAnimatedValues();
            for (int i = 0; i < animated.size(); ++i)
                recordStoredLayoutEffect (animated.getName (i));

            hadValues = computed.size() > 0 || animated.size() > 0;
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
        return changed || hadValues || firstStyle;
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

    // A property the component holds itself can only be styled through its setter. One the component was given another
    // value of since is set again here with no value of the style changing: its descendants read it as they do the others
    bool stateChanged = false;

    for (const auto& storageId : touched)
        stateChanged |= syncStateBacked (component, cache, storageId);

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
    return changed || stateChanged || firstStyle;
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

/* Gives the component the value its style has for a property it holds itself. Returns whether that changed the component. */
bool StyleContext::syncStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId)
{
    if (storageId == styleContextTransformId)
    {
        const auto* styled = component.styleState != nullptr ? component.styleState->getStyleValue (storageId) : nullptr;
        const auto next = styled != nullptr ? VariantConverter<AffineTransform>::fromVar (*styled) : AffineTransform();
        const bool differs = (component.styleState != nullptr ? component.styleState->getStyleTransform() : AffineTransform()) != next;

        component.setStyleTransform (next);
        return differs;
    }

    // The paint margin a rule sets is widened by what the shadows and the filter reach, so it is set with them
    if (storageId == styleContextFilterStorageId || storageId == styleContextPaintMarginId)
        return syncStyledFilterAndMargins (component, cache);

    const auto* descriptor = findStoredProperty (component.getTypeInfo(), storageId);

    if (descriptor == nullptr || ! descriptor->isStateBacked() || descriptor->setter == nullptr)
        return false;

    const auto* styled = component.styleState != nullptr ? component.styleState->getStyleValue (storageId) : nullptr;

    if (styled == nullptr)
    {
        const bool isRestored = cache.savedState.contains (storageId);
        restoreStateBacked (component, cache, storageId);
        return isRestored;
    }

    const auto value = *styled;

    if (! cache.savedState.contains (storageId))
        cache.savedState.set (storageId, descriptor->getter (component));

    if (descriptor->getter (component).equals (value))
        return false;

    descriptor->setter (component, value);
    return true;
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
        && runsLoadedShaders (*cache.drawingStyle, cache.drawingShaderGeneration))
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
    cache.drawingShaderGeneration.reset();
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
    if (styleSheet == nullptr)
        return false;

    for (const auto& used : drawing.getShaders())
    {
        const auto name = used.block.getProperty ("name").toString();
        const auto block = styleSheet->findShader (name);

        // A shader of another block is not one of the sheet, whatever its name: nothing is loaded for it
        if (block != used.block && ! block.isEquivalentTo (used.block))
            continue;

        if (const auto* loaded = findShader (name); loaded != nullptr && loaded->shader != used.shader)
            return true;
    }

    return false;
}

/* Tells whether a drawing the context keeps for a component runs the shaders the sheet has loaded. It does while none is
   loaded after it was found to, so it is compared with them again only once one was. */
bool StyleContext::runsLoadedShaders (const DrawingStyle& drawing, std::optional<uint32>& generation)
{
    if (generation == shaderGeneration)
        return true;

    if (holdsOtherShaders (drawing))
        return false;

    generation = shaderGeneration;
    return true;
}

/* A drawing set on the component itself, by its element or from C++, runs the shaders of the sheet as one a rule selects
   does: it is made again with the ones the sheet has loaded when it holds others, which repaints the component. The
   builder of a document makes such a drawing before the sheet is there, with no shader at all. */
const DrawingStyle* StyleContext::attachDrawingShaders (Component& component, ComponentCache& cache, const DrawingStyle& drawing)
{
    // Only the drawing made here is kept, so only that one is known to be the one compared before
    if (cache.attachedDrawing.get() != &drawing)
        cache.attachedDrawing = nullptr;

    const bool isCurrent = cache.attachedDrawing != nullptr ? runsLoadedShaders (drawing, cache.attachedShaderGeneration)
                                                            : ! holdsOtherShaders (drawing);

    if (isCurrent)
        return &drawing;

    DataTreeDiagnostic failure;
    const auto remade = drawing.createWithShaders (bindFront (&StyleContext::findDrawingShader, this), &failure);

    if (remade.failed())
    {
        report ({ .message = "drawing " + drawing.getName() + ": " + remade.getErrorMessage(), .element = failure.element, .property = failure.property });
        return &drawing;
    }

    cache.attachedDrawing = remade.getReference();
    cache.attachedShaderGeneration.reset();
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

        for (const auto& property : ShaderProperty::readBlock (block, name))
        {
            const auto value = property.element.getProperty ("value").toString().trim();

            if (value.isEmpty())
                continue;

            auto& fallback = entry.defaults.emplace_back();
            fallback.name = property.name;
            fallback.cascadeKey = fallback.name;
            fallback.value = value;
            fallback.element = property.element;
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

    // A source is compiled for the graphics API that draws it: the one of the window the root is in, or the one a window
    // is given by default while it is in none. What a paint in no window could not draw is compiled for all of them
    const auto windowApi = findWindowGraphicsApi (root);
    std::optional<GpuPlatform> api;

    if (! entry.compilesForEveryApi)
        api = windowApi.has_value() ? *windowApi : ComponentNative::getDefaultGraphicsApi();

    entry.sourceApi = api;

    auto bytes = ComponentDocument::readAssetFor (&root, bundlePath);
    const bool hasBundle = bytes.has_value();

    if (! hasBundle)
        bytes = ComponentDocument::readAssetFor (&root, path);

    if (! bytes.has_value())
    {
        const auto missing = bundlePath.isNotEmpty() ? "'" + path + "' nor its compiled bundle '" + bundlePath + "'" : "'" + path + "'";

        return fail ({ .message = "The shader '" + name + "' can't read " + missing, .element = block, .property = "fragment" });
    }

    // A bundle is loaded with the variants it was built with, whichever API draws it
    if (GpuImageShader::isCompiledBundle (bytes->getData(), bytes->getSize()))
        entry.sourceApi.reset();

    auto created = GpuImageShader::createFromData (bytes->getData(), bytes->getSize(), api);

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

    // A bundle built for other graphics APIs than the one of the window is told now when the window is known, and else
    // by the first paint drawing the shader
    if (windowApi.has_value() && ! created.getReference()->canDrawWith (*windowApi))
    {
        return fail ({ .message = "The shader '" + name + "' has no variant for the graphics API of its window: compile it for every language yup_shader_bundler targets",
                       .element = block,
                       .property = "fragment" });
    }

    // The members of the shader, which `created` keeps until the load is over
    const auto& uniforms = created.getReference()->getUniformLayout();
    std::vector<ShaderProperty> properties;
    StringArray declared;

    for (const auto& property : ShaderProperty::readBlock (block, name, &uniforms, created.getReference()->getInputTextureNames()))
    {
        declared.add (property.name.toString());

        if (property.problem.isEmpty())
            properties.push_back ({ property.name, property.type });
        else
            fail ({ .message = property.problem, .element = property.element });
    }

    // What a shader is given beside its properties fills the members of their names, whatever the shader means by them:
    // one declared as something else is a value of its own, which a property has to set
    for (const auto& given : detail::shaderStandardUniforms)
    {
        if (declared.contains (given.name) || ! detail::isShaderStandardUniformMismatched (uniforms, given))
            continue;

        fail ({ .message = String ("The member '") + given.name + "' of the uniform block of the shader '" + name + "' is not " + given.shape
                               + ", which is what a shader is given there: declare a property '" + given.name + "' to set it",
                .element = block,
                .property = "fragment" });
    }

    if (! entry.problems.empty())
        return false;

    entry.loaded.shader = created.getValue();
    entry.loaded.properties = std::move (properties);
    entry.drawProblems.clear();

    // The drawings made with the shaders loaded before are compared with them again
    ++shaderGeneration;
    return true;
}

void StyleContext::refreshShaders()
{
    if (shaders->empty())
        return;

    const auto windowApi = findWindowGraphicsApi (root);

    for (auto& [name, entry] : *shaders)
    {
        // A window drawing with another graphics API than the one the source of a shader was compiled for, as one created
        // with an API of its own does: the source is compiled for that one, once whatever comes of it
        if (entry.loaded.shader != nullptr && windowApi.has_value() && entry.sourceApi.has_value() && entry.sourceApi != windowApi)
        {
            entry.isStale = true;
            markDirty (root);
        }

        if (! entry.isStale)
        {
            if (std::exchange (entry.reportsAgain, false))
            {
                for (const auto& problem : entry.problems)
                    report (problem);

                for (const auto& problem : entry.drawProblems)
                    report (problem);
            }

            continue;
        }

        reloadShader (name, entry);
    }
}

/* Loads a shader again and gives it to the effects running the one it replaces. The drawings using it are made again by
   the restyle of their components. */
void StyleContext::reloadShader (const String& name, ShaderEntry& entry)
{
    // A shader that fails to load leaves the one it was to replace running
    if (! loadShader (name, entry))
        return;

    // The effects running the shader take the new one, unless it now reads more textures than an effect has: those are made again
    const bool runsAsEffect = ShaderProperty::getNumPictures (*entry.loaded.shader, entry.loaded.properties) <= 1;

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

/* Tells what a device refused of the shaders of the sheet a component is drawn with. It was found while the component was
   painted, where nothing is reported, and had the component restyled: it is told here, at the block declaring the shader,
   once for as long as that shader is loaded and however many components it could not be drawn for. And again with the
   problems of the sheet when they are told again, as what prevents a shader from loading is.

   A source compiled for one graphics API that a root in no window could not be painted with is not told at first: nothing
   says which API painted there, so it is compiled for all of them, once, and what the next paint can't draw of it is told. */
void StyleContext::reportShaderFailures (const Component& component)
{
    if (shaders->empty())
        return;

    const auto reportFailure = [this] (const GpuImageShader* shader, const String& error)
    {
        if (shader == nullptr || error.isEmpty())
            return;

        for (auto& [name, entry] : *shaders)
        {
            if (entry.loaded.shader.get() != shader)
                continue;

            if (entry.sourceApi.has_value() && ! entry.compilesForEveryApi && ! findWindowGraphicsApi (root).has_value())
            {
                entry.compilesForEveryApi = true;
                reloadShader (name, entry);
                markDirty (root);
                return;
            }

            const DataTreeDiagnostic problem { .message = "The shader '" + name + "' can't be drawn: " + error, .element = entry.loaded.block, .property = "fragment" };

            const bool isKnown = std::any_of (entry.drawProblems.begin(), entry.drawProblems.end(), [&problem] (const DataTreeDiagnostic& known)
            {
                return known.message == problem.message;
            });

            // Kept before it is told: who hears about it may have the shaders loaded again
            if (! isKnown)
            {
                entry.drawProblems.push_back (problem);
                report (problem);
            }

            return;
        }
    };

    if (const auto* effect = dynamic_cast<const ShaderEffect*> (component.getComponentEffect().get()))
        reportFailure (effect->getShader().get(), effect->getDrawError());

    if (const auto* drawing = dynamic_cast<const DrawingStyle*> (component.getStyle().get()))
    {
        for (const auto& failure : drawing->getShaderFailures())
            reportFailure (failure.shader.shader.get(), failure.message);
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

    if (ShaderProperty::getNumPictures (*loaded->shader, loaded->properties) > 1)
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

        std::erase_if (dirtyComponents, [&component] (const DirtyComponent& dirty)
        {
            const auto* dirtyComponent = dirty.component.get();
            return dirtyComponent == &component || dirtyComponent == nullptr;
        });

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
    ++shaderGeneration;
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

/* The values reading a component by its name, as `knob.thumbColor`, read what the sheet computed for it too, which no
   listener is told about. They followed the restyle of everything a state used to cause: the components holding them
   follow the ones a state restyled with their descendants, in the pass that comes next. One restyled with what it
   reads, below the same component, has read it. */
void StyleContext::markReadersDirty (const std::set<Component*>& restyled)
{
    const auto findRestyled = [this, &restyled] (const Component* component) -> const Component*
    {
        for (; component != nullptr; component = component->getParentComponent())
        {
            if (restyled.find (const_cast<Component*> (component)) != restyled.end())
                return component;

            if (component == &root)
                break;
        }

        return nullptr;
    };

    for (const auto& [ptr, cache] : *caches)
    {
        auto* styled = cache.component.get();
        if (styled == nullptr || styled != ptr)
            continue;

        const bool reads = std::any_of (cache.values.begin(), cache.values.end(), [&] (const auto& entry)
        {
            const auto& dependencies = entry.second.dependencies;
            return std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
            {
                const auto* source = findRestyled (dependency.component);
                return source != nullptr && source != findRestyled (styled);
            });
        });

        if (reads)
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
