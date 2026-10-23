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

#include <yup_gui/yup_gui.h>

//==============================================================================
/** The palette the synthesiser panels share.

    The instrument draws its own chrome rather than leaning on the theme widgets, so
    that the oscillator, envelope and display panels read as one instrument. The colors
    come from the roles of the theme palette, so the instrument follows palette switches.
*/
namespace SynthTheme
{
inline yup::Color role (yup::ThemePalette::Role paletteRole) { return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (paletteRole); }

inline yup::Color windowBackground() { return role (yup::ThemePalette::Role::background); }
inline yup::Color panelBorder() { return role (yup::ThemePalette::Role::outline); }
inline yup::Color accent() { return role (yup::ThemePalette::Role::accent); }
inline yup::Color textPrimary() { return role (yup::ThemePalette::Role::text); }
inline yup::Color textSecondary() { return role (yup::ThemePalette::Role::textMuted); }

/** The dark well displays and knobs sit in, darker than the panels in either mode. */
inline yup::Color displayBackground()
{
    const auto isDark = yup::ApplicationTheme::getGlobalTheme()->getPalette().getMode() == yup::ThemePalette::Mode::dark;
    return windowBackground().mixedWith (isDark ? yup::Colors::black : yup::Colors::white, 0.35f, yup::ColorSpace::SRGB);
}

/** The color of one of the four modulation source families, the accent first and the
    others a quarter turn of hue apart, so routes read by color on any palette.
*/
inline yup::Color sourceAccent (int index)
{
    const auto [hue, saturation, lightness] = accent().toHSL();
    const auto turn = static_cast<float> (yup::jlimit (0, 3, index)) * 0.25f;

    return yup::Color::fromHSL (std::fmod (hue + turn, 1.0f), saturation, lightness);
}

//==============================================================================
/** True when the palette is dark. */
inline bool isDark() { return yup::ApplicationTheme::getGlobalTheme()->getPalette().getMode() == yup::ThemePalette::Mode::dark; }

/** The levels of the instrument, back to front: the frame between panels, the strips
    of tabs, tiles and display bars, the panels, the sunken wells and the raised buttons.
    They keep that order on dark and light palettes alike.
*/
yup::Color frameColor();
yup::Color stripColor();
yup::Color panelColor();
yup::Color raisedColor();
inline yup::Color wellColor() { return displayBackground(); }

/** The colors the envelopes and the LFOs draw in, the same their modulation arcs use. */
inline yup::Color envelopeAccent() { return sourceAccent (0); }
inline yup::Color lfoAccent() { return sourceAccent (1); }

constexpr float frameGap = 3.0f;
constexpr float panelPadding = 8.0f;
constexpr float controlGap = 6.0f;
constexpr float panelCorner = 4.0f;
constexpr float wellCorner = 3.0f;
constexpr float stripHeight = 30.0f;
constexpr float headerHeight = 66.0f;
constexpr float bottomRowHeight = 126.0f;
constexpr float rightColumnShare = 381.0f / 1174.0f;

/** Paints a one pixel highlight along the top edge and a shade along the bottom one. */
void paintBevel (yup::Graphics& g, yup::Rectangle<float> bounds, float corner);

/** Paints a panel body with its bevel. */
void paintPanel (yup::Graphics& g, yup::Rectangle<float> bounds);

/** Paints a tile, a strip colored box with a bevel. */
void paintTile (yup::Graphics& g, yup::Rectangle<float> bounds);

/** Paints a sunken well: shaded under its top edge, lit under its bottom one. */
void paintWell (yup::Graphics& g, yup::Rectangle<float> bounds, float corner = wellCorner);

/** Paints a raised box: a lit top edge and a one pixel drop shadow. */
void paintRaised (yup::Graphics& g, yup::Rectangle<float> bounds, float corner);

/** Paints an engraved line between two areas of a panel. */
void paintSeam (yup::Graphics& g, yup::Point<float> from, yup::Point<float> to);

/** The narrow, semibold face of captions and tab names. */
yup::Font captionFont (float height);

/** The narrow face of values. */
yup::Font valueFont (float height);

/** Returns the width a single line of text takes in a font. */
float textWidth (const yup::String& text, const yup::Font& font);

/** Draws a text button as a raised key with its text in caps. */
void styleRaisedButton (yup::TextButton& button);

/** Draws a toggle button as a raised key whose text lights in the accent while on. */
void styleRaisedToggle (yup::ToggleButton& button);

/** Colors a combo box as a well. */
inline void styleComboBox (yup::ComboBox& comboBox)
{
    comboBox.setColor (yup::ComboBox::Style::backgroundColorId, wellColor());
    comboBox.setColor (yup::ComboBox::Style::textColorId, textPrimary());
    comboBox.setColor (yup::ComboBox::Style::borderColorId, wellColor());
    comboBox.setColor (yup::ComboBox::Style::arrowColorId, textSecondary());
}
} // namespace SynthTheme
