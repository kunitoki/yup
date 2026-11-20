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

#pragma once

namespace yup
{

//==============================================================================
/**
    Applies a stylesheet to a component tree.

    A context is created by Component::setStyleSheet() and owned by the component the sheet
    was attached to, which becomes its style root. It matches the sheet's rules against every
    component of the subtree, compiles the declaration values as expressions over the
    component's reflected properties, and writes the results into their StyleState, where
    Component::findProperty() picks them up. Values set explicitly in C++ always win, so a
    stylesheet never overwrites what the application asked for.

    Restyling is batched: class, state, property and hierarchy changes only mark components
    dirty, and the work happens once per frame in the refresh pass, top-down so that
    descendant selectors see their ancestors' state. A change of the theme palette, or of a
    palette scoped by a `palette:` declaration, restyles the components whose values read
    palette roles.

    The context also runs the `transition:` and `animation:` declarations through its
    StyleAnimator: a restyle changing a transitioned value starts a transition from the value
    shown before, and the animated values are written into the animated layer of the
    StyleState on every frame while something runs.

    The `cursor:`, `effect:` and `shadow:` declarations set the mouse cursor, a ComponentEffect
    from the ComponentEffectRegistry and feathered shadow layers painted behind the component,
    restoring what the component had before when no rule sets them anymore.

    @see StyleSheet, StyleState, StyleSelector, StyleAnimator, Component::setStyleSheet

    @tags{UI}
*/
class YUP_API StyleContext
{
public:
    //==============================================================================
    /** Turns the prefix of a symbol such as `knob` in `knob.value` into a component or model. */
    using ComponentResolver = DrawingScope::ComponentResolver;

    //==============================================================================
    /** Creates a context over a subtree.

        @param root       The component the sheet is attached to, which becomes the style root.
        @param styleSheet The sheet to apply, which may be nullptr.
    */
    StyleContext (Component& root, StyleSheet::Ptr styleSheet);

    /** Destructor. */
    ~StyleContext();

    //==============================================================================
    /** Replaces the stylesheet and restyles the whole subtree.

        @param newStyleSheet The new sheet, which may be nullptr.
    */
    void setStyleSheet (StyleSheet::Ptr newStyleSheet);

    /** Returns the stylesheet the context applies, which may be nullptr. */
    StyleSheet::Ptr getStyleSheet() const noexcept { return styleSheet; }

    /** Sets the resolver used by style and drawing expressions referencing named components or models. */
    void setComponentResolver (ComponentResolver resolver);

    //==============================================================================
    /** Marks a component and its descendants as needing a restyle.

        @param component The component whose styling may have changed.
    */
    void markDirty (Component& component);

    /** Marks a component dirty when the sheet's selectors read the property that changed.

        @param component The component whose property changed.
        @param name      The property name.
    */
    void propertyChanged (Component& component, const Identifier& name);

    //==============================================================================
    /** Restyles whatever is dirty, follows palette changes and advances the transitions and
        animations, called once per frame.

        @param elapsedSeconds The time elapsed since the previous frame, in seconds.
    */
    void refresh (double elapsedSeconds);

    /** Restyles whatever is dirty immediately, without waiting for the next frame. */
    void restyleNow();

    //==============================================================================
    /** Removes everything the context computed from the subtree, leaving explicit values alone. */
    void detach();

    /** Removes everything the context computed or animated for a detached child subtree.

        Running transitions and animations stop, and the values they showed are kept.
    */
    void detachComponent (Component& component);

    /** Drops animator tracks, dirty entries and caches for a component subtree being destroyed. */
    void forgetComponent (Component& component);

    /** Returns the root component this context is attached to. */
    Component& getRoot() noexcept { return root; }

    /** Styles a transient popup component against this context, anchored at @p owner when provided. */
    void stylePopup (Component& popup, Component* owner = nullptr);

    /** Copies visual style layers (shadows, glows and CSS filter) from @p source onto @p target. */
    static void copyVisualStyle (const Component& source, Component& target);

    /** Clears visual style layers previously copied with copyVisualStyle(). */
    static void clearVisualStyle (Component& target);

    /** Clears all computed values, animations and caches for a child subtree gaining its own stylesheet. */
    void releaseSubtree (Component& component);

    /** Optional callback fired when a global theme palette generation change is observed during refresh(). */
    std::function<void()> onPaletteChanged;

    //==============================================================================
    /** Returns the diagnostics the last restyle produced, one per declaration that failed.

        The element of each diagnostic is the declaration of the stylesheet it is about, when
        known, so a YuiSourceMap of the parsed document can locate it.
    */
    const std::vector<YuiDiagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Called with each diagnostic as soon as it is produced.

        Unlike @ref getDiagnostics, which only holds the last restyle, this also receives the
        problems of the declarations applied later, such as a `:hover` rule matching when the
        mouse enters a component. A value that failed is reported again only once it changes.
    */
    std::function<void (const YuiDiagnostic&)> onDiagnostic;

private:
    //==============================================================================
    struct ValueKey;
    struct ValueKeyHash;
    struct CompiledValue;
    struct ComponentCache;
    struct CacheMap;
    struct PopupMap;
    struct ResolvedDeclaration;
    struct PendingTransition;

    enum class ReleaseMode
    {
        forgetOnly,
        resetAndForget,
        freezeAnimatedAndForget
    };

    using DeclarationMap = std::unordered_map<Identifier, ResolvedDeclaration>;

    template <class Fn>
    void forEachManaged (Component& component, Fn&& fn);

    void restyleSubtree (Component& component, const DeclarationMap& inherited);
    void applyToComponent (Component& component, const DeclarationMap& inherited, DeclarationMap& inheritedForChildren);
    DeclarationMap collectInherited (Component& component) const;
    void collectDeclarations (Component& component, DeclarationMap& direct, DeclarationMap& qualified) const;
    static void mergeResolvedDeclaration (DeclarationMap& target, const Identifier& key, const ResolvedDeclaration& candidate);
    bool evaluate (Component& component,
                   ComponentCache& cache,
                   ValueKey key,
                   const DataTree& element,
                   const Identifier& storageId,
                   std::optional<ComponentProperty::Type> type,
                   const String& valueText,
                   var& result,
                   bool& paletteDependent,
                   const ComponentProperty* descriptor = nullptr);
    ComponentCache* findCache (const Component& component);
    ComponentCache& getCache (Component& component, bool& created);
    void clearCacheValues (ComponentCache& cache);
    void clearAllCaches();
    void resetCompiledValues();
    bool resetStyle (Component& component, ComponentCache* cache, bool keepAnimatedValues = false, bool* paletteCleared = nullptr);
    void release (Component& subtreeRoot, ReleaseMode mode);
    void syncStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId);
    void restoreStateBacked (Component& component, ComponentCache& cache, const Identifier& storageId);
    void restoreAllStateBacked (Component& component, ComponentCache& cache);
    const DrawingStyle* installDrawing (Component& component, ComponentCache& cache, const StyleSheet::Declaration& declaration);
    void restoreDrawing (Component& component, ComponentCache& cache);
    bool evaluateFilterDeclaration (Component& component,
                                    ComponentCache& cache,
                                    const StyleSheet::Declaration& filterDeclaration,
                                    std::vector<FilterEffect::Operation>& outOperations,
                                    bool* paletteDependent);
    bool syncStyledFilterAndMargins (Component& component, ComponentCache& cache);
    bool applyEffects (Component& component,
                       ComponentCache& cache,
                       const StyleSheet::Declaration* cursorDeclaration,
                       const StyleSheet::Declaration* effectDeclaration,
                       const StyleSheet::Declaration* shadowDeclaration,
                       const StyleSheet::Declaration* glowDeclaration,
                       bool* paletteDependent);
    const ComponentProperty* findDescriptor (const Component& component, const DrawingStyle* drawing, const Identifier& name) const;
    static const ComponentProperty* findStoredProperty (const ComponentTypeInfo& typeInfo, const Identifier& storageId);
    var getBaseValue (Component& component, const ComponentCache& cache, const ComponentProperty* descriptor, const Identifier& storageId);
    bool updateTransitions (Component& component, const ComponentCache& cache, const std::vector<PendingTransition>& pending, Array<Identifier>& touched);
    bool updateAnimations (Component& component, ComponentCache& cache, const StyleSheet::Declaration* animationDeclaration, const DrawingStyle* drawing, Array<Identifier>& touched);
    std::vector<StyleAnimator::Track> compileTracks (Component& component, ComponentCache& cache, const StyleSheet::KeyframeSequence& keyframes, const DrawingStyle* drawing);
    void showAnimatedValue (Component& component, const Identifier& storageId, const var* value);
    void markPaletteDependentDirty (Component& component);
    void report (const YuiDiagnostic& diagnostic);
    void rebuildObserver();
    void invalidateDependencies (const Component& sourceComponent, const Identifier& property);

    bool isManagedPopup (const Component& component) const noexcept;
    Component* findPopupOwner (const Component& component) const noexcept;

    Component& root;
    StyleSheet::Ptr styleSheet;
    ComponentResolver componentResolver;
    Array<WeakReference<Component>> dirtyComponents;
    std::unique_ptr<CacheMap> caches;
    std::unique_ptr<PopupMap> popups;
    DependencyObserver observer;
    StyleAnimator animator;
    std::vector<YuiDiagnostic> diagnostics;
    double clock = 0.0;
    uint32 lastPaletteGeneration = 0;
    bool isRestyling = false;
    bool observerDirty = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleContext)
};

} // namespace yup
