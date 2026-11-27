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
    The rules, keyframes and palettes of a `.yui` stylesheet.

    A sheet is attached to any component tree with Component::setStyleSheet(), which creates
    the StyleContext that matches its rules and computes the values. Sheets are immutable
    once built and reference counted, so the same sheet can style several trees.

    Build one with @ref fromTree, from the tree of a document. The yup_gui_markup module parses
    that tree from `.yui` text, and without it the tree is written in code. A sheet collects
    everything a document declares for styling: the rules and keyframes of its `style` blocks,
    its palettes, its named drawings, which rules select with `drawing:`, and its shaders.

    @code
    palette Ember { colors: "#1b1b1e, #ff8800"; mode: dark; accent: #ff9a1f; }

    drawing Knob { arc { ... stroke: thumbColor; } }

    style {
        .synth       { palette: Ember; }
        Slider       { drawing: Knob; thumbColor: palette.textMuted; }
        Slider:hover { thumbColor: palette.accent.brighter (0.2); }
    }
    @endcode

    Declaration values are expression texts, compiled per component by the StyleContext so
    they can read the component's properties and palette roles. The values of `palette:`,
    `drawing:`, `transition:`, `animation:`, `cursor:` and `effect:` are names or lists
    rather than expressions, and are kept as written.

    @see StyleContext, StyleSelector, Component::setStyleSheet

    @tags{UI}
*/
class YUP_API StyleSheet : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** A reference counted pointer to a stylesheet. */
    using Ptr = ReferenceCountedObjectPtr<StyleSheet>;

    //==============================================================================
    /** One `name: value;` of a rule. */
    struct Declaration
    {
        /** The semantic kind of declaration, classified once when building the sheet. */
        enum class Kind
        {
            property,
            palette,
            drawing,
            transition,
            animation,
            cursor,
            effect,
            filter,
            shadow,
            glow
        };

        /** One pre-parsed geometry layer of a `shadow:` or `glow:` declaration. */
        struct ShadowLayer
        {
            Point<float> offset;
            float blur = 0.0f;
            float spread = 0.0f;
            String colorExpression;
        };

        /** The declaration kind. */
        Kind kind = Kind::property;

        /** The property name, without the qualifier. */
        Identifier name;

        /** The type a qualified name like `Slider.thumbColor` targets, or invalid when unqualified. */
        Identifier qualifier;

        /** The expression or raw text of the value. */
        String value;

        /** The element of the document the declaration comes from, to locate its diagnostics. */
        DataTree element;

        /** Precomputed cascade map key (`Type.property` when qualified, `property` otherwise). */
        Identifier cascadeKey;

        /** Unquoted and trimmed name/text for non-expression declarations. */
        String parsedName;

        /** Pre-parsed cursor type for `cursor:` declarations, when valid. */
        std::optional<MouseCursor::Type> cursor;

        /** Pre-parsed transition list for `transition:` declarations. */
        std::vector<StyleAnimator::Transition> transitions;

        /** Pre-parsed animation list for `animation:` declarations. */
        std::vector<StyleAnimator::Animation> animations;

        /** Pre-parsed shadow geometry layers for `shadow:` declarations. */
        std::vector<ShadowLayer> shadows;

        /** Pre-parsed filter steps for `filter:` declarations. */
        std::vector<FilterEffect::ParsedStep> filters;

        /** Pre-computed diagnostic message when a non-expression declaration fails to parse. */
        String parseError;
    };

    /** A selector and the declarations it applies. */
    struct Rule
    {
        /** The selector of the rule. */
        StyleSelector selector;

        /** The declarations the rule applies, in source order. */
        std::vector<Declaration> declarations;

        /** The position of the rule in the sheet, breaking specificity ties. */
        int order = 0;
    };

    /** One step of a `@keyframes` sequence. */
    struct Keyframe
    {
        /** The position in the sequence, 0 for `from` and 1 for `to`. */
        double position = 0.0;

        /** The declarations the step sets. */
        std::vector<Declaration> declarations;
    };

    /** A named `@keyframes` sequence, sorted by position. */
    struct KeyframeSequence
    {
        /** The name used by `animation:`. */
        Identifier name;

        /** The steps, sorted by position. */
        std::vector<Keyframe> frames;
    };

    /** The palettes declared under one name: a dark one, a light one, or both.

        Two `palette` blocks sharing a name, one `mode: dark` and one `mode: light`, are the
        two versions of one palette, and the mode of the application theme picks between them.
    */
    struct PaletteVersions
    {
        /** Returns the version for a mode, or the only one there is. */
        const ThemePalette* find (ThemePalette::Mode mode) const noexcept;

        /** Returns the version for the mode of the application theme, or the only one there is. */
        const ThemePalette* findForTheme() const;

        /** Returns true when there is a dark and a light version, so the palette follows the theme. */
        bool followsTheme() const noexcept { return dark.has_value() && light.has_value(); }

        /** The palette declared with `mode: dark`, or without a mode. */
        std::optional<ThemePalette> dark;

        /** The palette declared with `mode: light`. */
        std::optional<ThemePalette> light;
    };

    /** The palettes of a document, by name. */
    using Palettes = std::map<String, PaletteVersions>;

    //==============================================================================
    /** Builds a sheet from a parsed document.

        The sheet keeps the tree, so the elements of its declarations and drawings stay in
        their document and the diagnostics about them can be located with a DataTreeSourceMap.

        @param tree     A `yui` document, as the parser of a `.yui` text returns it, or a single
                        `style` element.
        @param failure  When not null, receives the first problem along with its element and
                        property, ready to be located.

        @returns The sheet, or a failure describing the first offending element.
    */
    static ResultValue<Ptr> fromTree (const DataTree& tree, DataTreeDiagnostic* failure = nullptr);

    /** Returns true when the element is a style, rule, palette, drawing, shader or keyframes block. */
    static bool isStyleElement (const DataTree& element) noexcept;

    /** Returns the names of the declarations a rule takes on top of the properties of the
        components it styles, such as `palette`, `drawing` and `transition`, for a tool to list them.
    */
    static const StringArray& getDeclarationNames();

    /** Parses a `palette` element into @p palettes, returning a diagnostic if invalid.

        A name can be declared once for each mode: a second palette of the same mode is invalid.
    */
    static std::optional<DataTreeDiagnostic> readPaletteElement (const DataTree& element, Palettes& palettes);

    /** Reads what a `property` element declares: its name and its type.

        This is the `property name: type;` a style, rule, drawing, shader, tokens or component
        block holds, which types a custom property for what reads it. A name written with a
        qualifier, as `Slider.thumbColor`, declares the name after it.

        @param element  The element to read.

        @returns The property, inherited by the components inside the one it is set on, or
                 nothing for an element that is no `property`, or that has no name or no type.
    */
    static std::optional<ComponentProperty> readDeclaredProperty (const DataTree& element);

    //==============================================================================
    /** Returns the rules of the sheet, in source order. */
    const std::vector<Rule>& getRules() const noexcept { return rules; }

    /** Finds the rules a component can match, in source order.

        The sheet files each rule under what the last compound of its selector requires of a
        component: its id, or else one of its classes, or else its type. The rules found are the
        ones filed under the id, the classes and the types of the component, and the ones requiring
        none of those, such as `*`, `:root`, `:hover` and `[text="Off"]`. Every rule matching the
        component is among them, so testing them with StyleSelector::matches gives what testing
        every rule of the sheet does.

        @param component  The component to find the rules for.
        @param candidates Receives the rules, in place of what it holds.
    */
    void findCandidateRules (const Component& component, std::vector<const Rule*>& candidates) const;

    /** Returns the keyframe sequence with a given name, or nullptr.

        @param name The sequence name.
    */
    const KeyframeSequence* findKeyframes (const Identifier& name) const;

    /** Returns the palette declared under a given name, or nullptr.

        A palette declared for both modes gives the version for the mode of the application
        theme, the one a `palette: Name;` declaration applies.

        @param name The palette name.
    */
    const ThemePalette* findPalette (const String& name) const;

    /** Returns the dark and light versions declared under a given name, or nullptr.

        @param name The palette name.
    */
    const PaletteVersions* findPaletteVersions (const String& name) const;

    /** Returns the names of the palettes the sheet declares. */
    StringArray getPaletteNames() const;

    /** Returns the drawing declared under a given name, or an invalid tree.

        @param name The drawing name, as selected by a `drawing:` declaration.
    */
    DataTree findDrawing (const String& name) const;

    /** Returns the shader declared under a given name, or an invalid tree.

        The tree is the `shader` element: its `fragment` or `material` property holds the path of
        the source, and each of its `property` children declares an input with its `name`, its
        `type` and, when it has a default, the expression of its `value`. A name is declared by
        one shader, and by no drawing.

        @param name The shader name, as written after `shader`.
    */
    DataTree findShader (const String& name) const;

    /** Returns a custom property declared inside a rule of this sheet (`property name: type`), or nullptr. */
    const ComponentProperty* findDeclaredProperty (const Identifier& name) const;

    //==============================================================================
    /** Returns true when some selector reads a property, so changing it must restyle.

        @param name The property name.
    */
    bool watchesProperty (const Identifier& name) const;

    /** Returns true when changing a property on a component can change the style of its descendants.

        That is so when a selector reads the property on a compound before a combinator, as in
        `.row:hover .icon`, or as an attribute, whose value a component can take from a parent, and
        when a rule reading it holds a qualified declaration such as `Slider.thumbColor`, which the
        descendants inherit. A state only read as a pseudo-class of the component a rule matches, as
        in `Slider:hover`, changes which rules that component matches and nothing below it.

        @param name The property name.

        @see watchesProperty, StyleSelector::selectsDescendantsBy
    */
    bool watchesPropertyForDescendants (const Identifier& name) const;

private:
    //==============================================================================
    StyleSheet() = default;

    std::optional<DataTreeDiagnostic> addElements (const DataTree& parent);
    std::optional<DataTreeDiagnostic> findSharedName() const;
    void indexRule (const Rule& rule);

    DataTree tree;
    std::vector<Rule> rules;
    std::unordered_map<String, std::vector<int>> rulesById;
    std::unordered_map<Identifier, std::vector<int>> rulesByClass;
    std::unordered_map<Identifier, std::vector<int>> rulesByType;
    std::vector<int> rulesWithoutKey;
    std::vector<KeyframeSequence> keyframes;
    Palettes palettes;
    std::map<String, DataTree> drawings;
    std::map<String, DataTree> shaders;
    std::map<Identifier, ComponentProperty> declaredProperties;
    Array<Identifier> watchedProperties;
    Array<Identifier> descendantProperties;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleSheet)
};

} // namespace yup
