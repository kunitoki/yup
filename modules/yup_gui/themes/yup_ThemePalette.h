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
    A small set of semantic colors derived from any number of source colors.

    A ThemePalette takes a handful of "generator" colors, for example the output of an online
    palette generator, and assigns them to the roles a theme needs: a background, surfaces,
    readable text, an accent and so on. Colors that are missing or unsuitable are derived, so
    any input (even a single color) produces a complete, readable palette.

    The assignment is deterministic and does not depend on the order of the source colors:

    - background is the darkest source (or the lightest in light mode), pushed to the extreme
      of the lightness range if needed.
    - text is the source with the highest contrast against the background, lightened or
      darkened until it reaches a WCAG contrast of 4.5.
    - accent is the most chromatic remaining source, adjusted to a contrast of at least 3.
    - surface is a remaining source slightly lighter (or darker, in light mode) than the
      background, otherwise it is derived from the background. surfaceRaised steps one more
      level away from the background.
    - textMuted is a low-chroma remaining source with a contrast between 3 and the text's.
    - outline and onAccent are always derived.

    Any role can be overridden afterwards with setColor().

    @code
    auto palette = ThemePalette::fromString ("https://coolors.co/100b00-85cb33-efffc8-a5cbc3-3b341f");
    if (palette.wasOk())
        ApplicationTheme::getGlobalTheme()->setPalette (palette.getReference());
    @endcode

    @see ApplicationTheme::setPalette

    @tags{UI}
*/
class YUP_API ThemePalette
{
public:
    //==============================================================================
    /** Whether the palette produces a dark or a light theme. */
    enum class Mode
    {
        dark,
        light
    };

    /** The semantic color roles a palette provides. */
    enum class Role
    {
        background,    ///< The window and page background.
        surface,       ///< Fields, lists and other inset areas.
        surfaceRaised, ///< Buttons, menus and other raised areas.
        outline,       ///< Borders and separators.
        text,          ///< Primary text.
        textMuted,     ///< Secondary text, hints and inactive elements.
        accent,        ///< Selection, focus and active values.
        onAccent,      ///< Text drawn on top of the accent color.
        numRoles
    };

    //==============================================================================
    /** Creates the default YUP palette (dark). */
    ThemePalette();

    /** Creates a palette by deriving all roles from the given source colors.

        @param sourceColors  The generator colors, in any order. An empty list gives the default palette.
        @param mode          Whether to build a dark or a light palette.
    */
    explicit ThemePalette (Span<const Color> sourceColors, Mode mode = Mode::dark);

    //==============================================================================
    /** Parses source colors from text and creates a palette from them.

        Accepted formats are XML with `<color hex="...">` or `<color r="..." g="..." b="...">`
        elements (as exported by palette generators), or any text containing 6 or 8 digit hex
        tokens, with or without a leading `#`, such as CSS, comma separated lists or palette URLs.
        8 digit tokens are read as RRGGBBAA.

        @param text  The text to parse.
        @param mode  Whether to build a dark or a light palette.

        @return The palette, or a failure if no valid color could be found.
    */
    static ResultValue<ThemePalette> fromString (const String& text, Mode mode = Mode::dark);

    //==============================================================================
    /** Returns the color assigned to a role. */
    Color getColor (Role role) const;

    /** Overrides the color assigned to a role. */
    void setColor (Role role, Color color);

    /** Returns whether this is a dark or a light palette. */
    Mode getMode() const;

    /** Returns the source colors the palette was derived from. */
    const std::vector<Color>& getSourceColors() const;

    //==============================================================================
    /** Returns true if both palettes have the same mode and role colors. */
    bool operator== (const ThemePalette& other) const;

    /** Returns true if the palettes differ in mode or in any role color. */
    bool operator!= (const ThemePalette& other) const;

private:
    void derive();

    std::array<Color, static_cast<std::size_t> (Role::numRoles)> roles;
    std::vector<Color> sourceColors;
    Mode mode = Mode::dark;
};

} // namespace yup
