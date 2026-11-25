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
    and feathered shadow layers painted behind the component, restoring what the component had
    before when no rule sets them anymore. The effect is a ShaderEffect running the `shader`
    block of the sheet that has the name, or else the one the ComponentEffectRegistry makes for
    it. The context loads the shaders of the sheet when they are first used (see findShader),
    and gives a component the defaults a shader declares for its properties as the styled
    values of lowest priority of the rule selecting it.

    A drawing a `drawing:` declaration installs finds the shaders its `use` elements name in the
    sheet too (see DrawingStyle::ShaderResolver), and is made again when one of them is loaded
    again. So is a DrawingStyle set on a component itself, by its element in a `.yui` document or
    from C++: when it runs a shader of the sheet as another one than the sheet has loaded, none
    included, the restyle of its component makes it again with that one (see
    DrawingStyle::createWithShaders), whatever made it first.

    @see StyleSheet, StyleState, StyleSelector, StyleAnimator, ShaderEffect, Component::setStyleSheet

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
    /** A `shader` block of the stylesheet, loaded for the components of this context. */
    struct Shader
    {
        /** The compiled shader, to draw with GpuImageShader::draw(). It is replaced when its file
            changes (see assetChanged), so read it each time it is drawn rather than keeping it.
        */
        GpuImageShader::Ptr shader;

        /** The inputs the block declares, in the order it declares them. Each has a type a uniform
            takes, and fills the member of the uniform block of the shader that has its name (see
            ShaderEffect::setUniform).
        */
        std::vector<ShaderEffect::Property> properties;

        /** The `shader` element of the stylesheet. Each of its `property` children declares one of
            the @ref properties, by its `name`, and holds the expression of its default as its
            `value` when it has one.
        */
        DataTree block;
    };

    /** Returns the shader a `shader` block of the stylesheet declares, loading it the first time
        it is asked for.

        This is where an `effect:` declaration gets the shader it runs, where the drawings of the
        components the context styles get the ones they `use`, and where anything else drawing a
        shader of the stylesheet gets it. A shader is loaded once for the whole context, and kept
        until the stylesheet is replaced. One whose file changed is loaded again by the next
        restyle (see assetChanged).

        The bytes are read with ComponentDocument::readAssetFor() for the root of the context, so
        they come from where the document of the interface does: a folder, embedded resources or
        the unsaved texts of an editor. For a stylesheet attached to a tree made in C++ they are
        read from the file. For `fragment: "fx/crt.frag"` the compiled bundle `fx/crt.frag.ysl` is
        loaded when there is one, which is what a build embeds, else `fx/crt.frag` itself: a
        bundle, or GLSL source when the shader transpiler is built in (see
        GpuImageShader::createFromData). A path ending in `.ysl` is read as it is.

        Loading needs no GPU, and reports what prevents it as a diagnostic (see @ref onDiagnostic):
        the file can't be read, the shader does not compile, or a property the block declares has
        a type a uniform can't take, or no member of its name that is large enough in the uniform
        block of the shader. A shader is reported when it is loaded and not each time it is asked
        for, then again with the other problems of the stylesheet once its values are compiled
        again (see @ref setComponentResolver), as they are each time a document is loaded. Call
        this when the style is applied and not while painting, so that a shader is loaded on the
        message thread.

        @param name  The name of the block, as written after `shader`.

        @returns The shader, or nullptr when no block has that name, when the block declares a
                 `material` instead of a `fragment`, and when the shader could not be loaded. The
                 pointer stays valid until the stylesheet of the context is replaced.
    */
    const Shader* findShader (const String& name);

    /** Tells that a file changed, for the shaders read from it to be loaded again.

        Nothing is loaded here. The next restyle loads each of these shaders again, once however
        many of its files it was told about, then gives the new shader to the effects running the
        one before and repaints their components, and makes again the drawings using it, which
        are then painted with the new one (see @ref refresh and @ref restyleNow).

        A `.yui` document calls this for the files it is told about, which its loader does for
        the files it watches, for the overrides of an editor, at each reload and when it loads
        another source. Call it yourself for a stylesheet attached to a tree made in C++, whose
        files nothing watches.

        A shader that fails to load is reported as a diagnostic and the one it was to replace
        keeps running, so that a mistake made while editing does not blank the interface. The
        problem stands until the shader loads: it is reported again each time the problems of the
        stylesheet are (see findShader), so it is still among those of a document loaded again
        in the meantime.

        @param path  The path of the file as the stylesheet writes it, or that path with `.ysl`
                     added for its compiled bundle.
    */
    void assetChanged (const String& path);

    //==============================================================================
    /** Returns the diagnostics the last restyle produced, one per declaration that failed.

        The element of each diagnostic is the declaration of the stylesheet it is about, when
        known, so a DataTreeSourceMap of the parsed document can locate it.
    */
    const std::vector<DataTreeDiagnostic>& getDiagnostics() const noexcept { return diagnostics; }

    /** Called with each diagnostic as soon as it is produced.

        Unlike @ref getDiagnostics, which only holds the last restyle, this also receives the
        problems of the declarations applied later, such as a `:hover` rule matching when the
        mouse enters a component. A value that failed is reported again only once it changes.
    */
    std::function<void (const DataTreeDiagnostic&)> onDiagnostic;

private:
    //==============================================================================
    struct ValueKey;
    struct ValueKeyHash;
    struct CompiledValue;
    struct ComponentCache;
    struct CacheMap;
    struct PopupMap;
    struct ShaderEntry;
    struct ShaderMap;
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
    DrawingStyle::Shader findDrawingShader (const String& name);
    bool holdsOtherShaders (const DrawingStyle& drawing);
    const DrawingStyle* attachDrawingShaders (Component& component, const DrawingStyle& drawing);
    bool evaluateFilterDeclaration (Component& component,
                                    ComponentCache& cache,
                                    const StyleSheet::Declaration& filterDeclaration,
                                    std::vector<FilterEffect::Operation>& outOperations,
                                    bool* paletteDependent);
    bool syncStyledFilterAndMargins (Component& component, ComponentCache& cache);
    ShaderEntry* findShaderEntry (const String& name);
    bool loadShader (const String& name, ShaderEntry& entry);
    void refreshShaders();
    ComponentEffect::Ptr createEffect (Component& component, const StyleSheet::Declaration& effectDeclaration);
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
    void report (const DataTreeDiagnostic& diagnostic);
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
    std::unique_ptr<ShaderMap> shaders;
    DependencyObserver observer;
    StyleAnimator animator;
    std::vector<DataTreeDiagnostic> diagnostics;
    double clock = 0.0;
    uint32 lastPaletteGeneration = 0;
    bool isRestyling = false;
    bool observerDirty = false;
    FrameAttachment frameAttachment { root, bindFront (&StyleContext::refresh, this) };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleContext)
};

} // namespace yup
