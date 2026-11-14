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

    @see StyleSheet, StyleState, StyleSelector, Component::setStyleSheet

    @tags{UI}
*/
class YUP_API StyleContext
{
public:
    //==============================================================================
    /** Creates a context over a subtree.

        @param root      The component the sheet is attached to, which becomes the style root.
        @param styleSheet The sheet to apply, which may be nullptr.
    */
    StyleContext (Component& root, StyleSheet::Ptr styleSheet);

    /** Destructor. */
    ~StyleContext();

    //==============================================================================
    /** Returns the component the context is attached to. */
    Component& getRoot() const noexcept { return root; }

    /** Replaces the stylesheet and restyles the whole subtree.

        @param newStyleSheet The new sheet, which may be nullptr.
    */
    void setStyleSheet (StyleSheet::Ptr newStyleSheet);

    /** Returns the stylesheet the context applies, which may be nullptr. */
    StyleSheet::Ptr getStyleSheet() const noexcept { return styleSheet; }

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
    /** Restyles whatever is dirty and follows palette changes, called once per frame.

        @param lastFrameTimeSeconds The time of the frame being prepared, in seconds.
    */
    void refresh (double lastFrameTimeSeconds);

    /** Restyles whatever is dirty immediately, without waiting for the next frame. */
    void restyleNow();

    //==============================================================================
    /** Removes everything the context computed from the subtree, leaving explicit values alone. */
    void detach();

    //==============================================================================
    /** Returns the diagnostics the last restyle produced, one per declaration that failed. */
    const StringArray& getDiagnostics() const noexcept { return diagnostics; }

private:
    //==============================================================================
    struct CompiledValue;
    struct ComponentCache;
    struct ResolvedDeclaration;

    using DeclarationMap = std::map<Identifier, ResolvedDeclaration>;

    void restyleSubtree (Component& component, const DeclarationMap& inherited);
    void applyToComponent (Component& component, const DeclarationMap& inherited, DeclarationMap& inheritedForChildren);
    DeclarationMap collectInherited (Component& component) const;
    void collectDeclarations (Component& component, DeclarationMap& direct, DeclarationMap& qualified) const;
    bool evaluate (Component& component, const Identifier& storageId, const ComponentProperty* descriptor, const String& valueText, var& result, bool& paletteDependent);
    ComponentCache& getCache (Component& component);
    void restoreStateBacked (Component& component, const Identifier& storageId);
    void restoreAllStateBacked (Component& component);
    void installDrawing (Component& component, const String& name);
    void restoreDrawing (Component& component);
    void clearSubtree (Component& component);
    void markPaletteDependentDirty (Component& component);

    Component& root;
    StyleSheet::Ptr styleSheet;
    Array<WeakReference<Component>> dirtyComponents;
    std::map<Component*, std::unique_ptr<ComponentCache>> caches;
    StringArray diagnostics;
    uint32 lastPaletteGeneration = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleContext)
};

} // namespace yup
