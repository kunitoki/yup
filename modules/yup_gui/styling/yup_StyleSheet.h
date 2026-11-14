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

    Build one straight from `.yui` text with @ref parse, or from a parsed document with
    @ref fromTree. A sheet collects everything a document declares for styling: the rules
    and keyframes of its `style` blocks, its palettes, and its named drawings, which rules
    select with `drawing:`.

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
        /** The property name, without the qualifier. */
        Identifier name;

        /** The type a qualified name like `Slider.thumbColor` targets, or invalid when unqualified. */
        Identifier qualifier;

        /** The expression text of the value. */
        String value;
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

    //==============================================================================
    /** Builds a sheet from a parsed document.

        @param tree A `yui` document (see YuiParser), or a single `style` element.

        @returns The sheet, or a failure describing the first offending element.
    */
    static ResultValue<Ptr> fromTree (const DataTree& tree);

    /** Parses `.yui` text and builds a sheet from it.

        @param text The `.yui` text.

        @returns The sheet, or a failure with the first diagnostic.
    */
    static ResultValue<Ptr> parse (const String& text);

    //==============================================================================
    /** Returns the rules of the sheet, in source order. */
    const std::vector<Rule>& getRules() const noexcept { return rules; }

    /** Returns the keyframe sequence with a given name, or nullptr.

        @param name The sequence name.
    */
    const KeyframeSequence* findKeyframes (const Identifier& name) const;

    /** Returns the palette declared under a given name, or nullptr.

        @param name The palette name.
    */
    const ThemePalette* findPalette (const String& name) const;

    /** Returns the names of the palettes the sheet declares. */
    StringArray getPaletteNames() const;

    /** Returns the drawing declared under a given name, or an invalid tree.

        @param name The drawing name, as selected by a `drawing:` declaration.
    */
    DataTree findDrawing (const String& name) const;

    //==============================================================================
    /** Returns true when some selector reads a property, so changing it must restyle.

        @param name The property name.
    */
    bool watchesProperty (const Identifier& name) const;

private:
    //==============================================================================
    StyleSheet() = default;

    Result addElements (const DataTree& parent);

    std::vector<Rule> rules;
    std::vector<KeyframeSequence> keyframes;
    std::map<String, ThemePalette> palettes;
    std::map<String, DataTree> drawings;
    Array<Identifier> watchedProperties;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleSheet)
};

} // namespace yup
