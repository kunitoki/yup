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

#include <array>
#include <optional>

//==============================================================================
/** The palettes the synthesiser is drawn with.

    A theme gives every section of the instrument a palette of its own, so the header,
    the oscillators, the FM and modulation page, the filters, the envelopes, the LFOs,
    the voicing and the keys can each have their own accent and surfaces. The palettes
    are set on the components (see yup::Component::setPalette()), not globally, so every
    interface wears its own theme: applyTheme() gives the interface the base palette and
    each component marked with applySection() its section's, and everything inside finds
    its colors through the scope it paints for, looking up the component tree. A knob in
    the LFO section wears the LFO accent with no extra wiring, and the yup widgets follow
    the same palettes.

    Colors are read while painting, never while a component is built, since a component
    only reaches its palette once it is inside the themed interface.
*/
namespace SynthTheme
{
/** The sections of the instrument that can wear a palette of their own. */
enum class Section
{
    header,
    oscillators,
    matrix,
    filters,
    envelopes,
    lfos,
    voicing,
    keys
};

constexpr int sectionCount = 8;

/** A palette for each section, in the order of Section; a missing one follows the base palette. */
using SectionPalettes = std::array<std::optional<yup::ThemePalette>, sectionCount>;

/** Themes an interface: the base palette on the root, the accents every section can read
    and its section's palette on each component inside marked with applySection(). */
void applyTheme (yup::Component& root, const yup::ThemePalette& base, const SectionPalettes& sections);

/** Marks a component as a section, so applyTheme() gives it, everything inside it and the
    menus opened from it the section's palette. */
void applySection (yup::Component& component, Section section);

/** Returns a role of the palette a component is drawn with: its section's, or the base one. */
yup::Color role (const yup::Component& scope, yup::ThemePalette::Role paletteRole);

/** True when the palette a component is drawn with is dark. */
bool isDark (const yup::Component& scope);

/** Returns the accent of a section of the interface a component is in. */
yup::Color sectionAccent (const yup::Component& scope, Section section);

inline yup::Color windowBackground (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::background); }
inline yup::Color panelBorder (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::outline); }
inline yup::Color accent (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::accent); }
inline yup::Color onAccent (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::onAccent); }
inline yup::Color textPrimary (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::text); }
inline yup::Color textSecondary (const yup::Component& scope) { return role (scope, yup::ThemePalette::Role::textMuted); }

/** The color of one of the four modulation source families: the envelopes' accent, the
    LFOs' accent, then the base accent a third and two thirds of a turn of hue away. */
yup::Color sourceAccent (const yup::Component& scope, int index);

/** The colors the envelopes and the LFOs draw in, the same their modulation arcs use. */
inline yup::Color envelopeAccent (const yup::Component& scope) { return sourceAccent (scope, 0); }
inline yup::Color lfoAccent (const yup::Component& scope) { return sourceAccent (scope, 1); }

/** The levels of the instrument, back to front: the frame between panels, the strips
    of tabs, tiles and display bars, the panels, the sunken wells and the raised buttons.
    They keep that order on dark and light palettes alike. The frame belongs to no section.
*/
yup::Color frameColor (const yup::Component& scope);
yup::Color stripColor (const yup::Component& scope);
yup::Color panelColor (const yup::Component& scope);
yup::Color raisedColor (const yup::Component& scope);
yup::Color wellColor (const yup::Component& scope);

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
void paintBevel (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner);

/** Paints a panel body with its bevel. */
void paintPanel (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds);

/** Paints a tile, a strip colored box with a bevel. */
void paintTile (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds);

/** Paints a sunken well: shaded under its top edge, lit under its bottom one. */
void paintWell (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner = wellCorner);

/** Paints a raised box: a lit top edge and a one pixel drop shadow. */
void paintRaised (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner);

/** Fills a path with a band of light that ends at x and fades out over width to its left,
    the trail of a playhead moving to the right. */
void paintTrail (yup::Graphics& g, const yup::Path& area, float x, float width, yup::Color color);

/** Paints an engraved line between two areas of a panel. */
void paintSeam (yup::Graphics& g, const yup::Component& scope, yup::Point<float> from, yup::Point<float> to);

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
} // namespace SynthTheme
