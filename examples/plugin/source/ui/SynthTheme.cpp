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

#include "SynthTheme.h"

#include <vector>

namespace SynthTheme
{
namespace
{
yup::Color mix (yup::Color from, yup::Color to, float amount)
{
    return from.mixedWith (to, amount, yup::ColorSpace::SRGB);
}

yup::ColorGradient makeVerticalGradient (yup::Rectangle<float> bounds, std::initializer_list<std::pair<float, yup::Color>> stops)
{
    std::vector<yup::ColorGradient::ColorStop> colorStops;

    for (const auto& [position, color] : stops)
        colorStops.emplace_back (color, bounds.getX(), bounds.getY() + bounds.getHeight() * position, position);

    return yup::ColorGradient (yup::ColorGradient::Linear, std::move (colorStops));
}

yup::Font withAxis (yup::Font font, const char* tag, float value)
{
    if (const auto axis = font.getAxisDescription (tag); axis.has_value())
        return font.withAxisValue (tag, yup::jlimit (axis->minimumValue, axis->maximumValue, value));

    return font;
}
} // namespace

//==============================================================================
namespace
{
constexpr std::array<yup::ThemePalette::Role, 8> roles { yup::ThemePalette::Role::background, yup::ThemePalette::Role::surface,
                                                         yup::ThemePalette::Role::surfaceRaised, yup::ThemePalette::Role::outline,
                                                         yup::ThemePalette::Role::text, yup::ThemePalette::Role::textMuted,
                                                         yup::ThemePalette::Role::accent, yup::ThemePalette::Role::onAccent };

const yup::Identifier& getRoleId (yup::ThemePalette::Role paletteRole)
{
    static const std::array<yup::Identifier, 8> ids { "synthBackground", "synthSurface", "synthSurfaceRaised", "synthOutline",
                                                      "synthText", "synthTextMuted", "synthAccent", "synthOnAccent" };

    for (std::size_t index = 0; index < roles.size(); ++index)
        if (roles[index] == paletteRole)
            return ids[index];

    return ids.front();
}

const yup::ThemePalette& getGlobalPalette()
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette();
}

std::array<std::optional<yup::ThemePalette>, sectionCount>& getSectionPalettes()
{
    static std::array<std::optional<yup::ThemePalette>, sectionCount> palettes;
    return palettes;
}

yup::Color wellFrom (yup::Color background)
{
    return mix (background, isDark() ? yup::Colors::black : yup::Colors::white, 0.35f);
}
} // namespace

void setSectionPalettes (std::array<std::optional<yup::ThemePalette>, sectionCount> palettes)
{
    getSectionPalettes() = std::move (palettes);
}

yup::Color sectionRole (Section section, yup::ThemePalette::Role paletteRole)
{
    const auto& palette = getSectionPalettes()[static_cast<std::size_t> (section)];
    return palette.has_value() ? palette->getColor (paletteRole) : getGlobalPalette().getColor (paletteRole);
}

void applySection (yup::Component& component, Section section)
{
    for (const auto paletteRole : roles)
        component.setColor (getRoleId (paletteRole), sectionRole (section, paletteRole));

    const auto well = wellFrom (sectionRole (section, yup::ThemePalette::Role::background));
    const auto text = sectionRole (section, yup::ThemePalette::Role::text);
    const auto muted = sectionRole (section, yup::ThemePalette::Role::textMuted);
    const auto sectionAccent = sectionRole (section, yup::ThemePalette::Role::accent);

    component.setColor (yup::ComboBox::Style::backgroundColorId, well);
    component.setColor (yup::ComboBox::Style::textColorId, text);
    component.setColor (yup::ComboBox::Style::borderColorId, well);
    component.setColor (yup::ComboBox::Style::arrowColorId, muted);
    component.setColor (yup::Label::Style::textFillColorId, muted);
    component.setColor (yup::TextEditor::Style::backgroundColorId, yup::Colors::transparentBlack);
    component.setColor (yup::TextEditor::Style::outlineColorId, yup::Colors::transparentBlack);
    component.setColor (yup::TextEditor::Style::focusedOutlineColorId, sectionAccent);
    component.setColor (yup::TextEditor::Style::textColorId, text);
    component.setColor (yup::TextEditor::Style::caretColorId, sectionAccent);
}

yup::Color role (const yup::Component& scope, yup::ThemePalette::Role paletteRole)
{
    if (const auto color = scope.findColor (getRoleId (paletteRole)))
        return *color;

    return getGlobalPalette().getColor (paletteRole);
}

yup::Color sourceAccent (int index)
{
    if (index <= 0)
        return sectionRole (Section::envelopes, yup::ThemePalette::Role::accent);

    if (index == 1)
        return sectionRole (Section::lfos, yup::ThemePalette::Role::accent);

    const auto [hue, saturation, lightness] = getGlobalPalette().getColor (yup::ThemePalette::Role::accent).toHSL();
    const auto turn = static_cast<float> (yup::jmin (index, 3) - 1) / 3.0f;
    return yup::Color::fromHSL (std::fmod (hue + turn, 1.0f), saturation, lightness);
}

//==============================================================================
yup::Color frameColor()
{
    const auto background = getGlobalPalette().getColor (yup::ThemePalette::Role::background);
    const auto text = getGlobalPalette().getColor (yup::ThemePalette::Role::text);

    return isDark() ? mix (background, yup::Colors::black, 0.5f) : mix (background, text, 0.25f);
}

yup::Color panelColor (const yup::Component& scope)
{
    return isDark() ? role (scope, yup::ThemePalette::Role::surfaceRaised)
                    : mix (windowBackground (scope), yup::Colors::white, 0.6f);
}

yup::Color stripColor (const yup::Component& scope)
{
    return mix (frameColor(), panelColor (scope), isDark() ? 0.45f : 0.55f);
}

yup::Color raisedColor (const yup::Component& scope)
{
    return isDark() ? mix (panelColor (scope), textPrimary (scope), 0.12f)
                    : mix (panelColor (scope), yup::Colors::white, 0.5f);
}

yup::Color wellColor (const yup::Component& scope)
{
    return wellFrom (windowBackground (scope));
}

//==============================================================================
void paintBevel (yup::Graphics& g, yup::Rectangle<float> bounds, float corner)
{
    const auto highlight = yup::Colors::white.withAlpha (isDark() ? 0.08f : 0.6f);
    const auto shade = yup::Colors::black.withAlpha (isDark() ? 0.45f : 0.15f);
    const auto edge = bounds.reduced (0.5f);

    g.setStrokeColorGradient (makeVerticalGradient (edge, { { 0.0f, highlight }, { 0.35f, highlight.withAlpha (0.0f) }, { 0.65f, shade.withAlpha (0.0f) }, { 1.0f, shade } }));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (edge, corner);
}

void paintPanel (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds)
{
    g.setFillColor (panelColor (scope));
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, bounds, panelCorner);
}

void paintTile (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds)
{
    g.setFillColor (stripColor (scope));
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, bounds, panelCorner);
}

void paintWell (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (wellColor (scope));
    g.fillRoundedRect (bounds, corner);

    const auto edge = bounds.reduced (0.5f);
    const auto shade = yup::Colors::black.withAlpha (isDark() ? 0.7f : 0.25f);

    g.setStrokeColorGradient (makeVerticalGradient (edge, { { 0.0f, shade }, { 0.4f, shade.withAlpha (0.0f) } }));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (edge, corner);

    g.setStrokeColor (yup::Colors::white.withAlpha (isDark() ? 0.06f : 0.5f));
    g.strokeLine (bounds.getX() + corner, bounds.getBottom() + 0.5f, bounds.getRight() - corner, bounds.getBottom() + 0.5f);
}

void paintRaised (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (yup::Colors::black.withAlpha (isDark() ? 0.6f : 0.25f));
    g.fillRoundedRect (bounds.withY (bounds.getY() + 1.0f), corner);

    g.setFillColor (raisedColor (scope));
    g.fillRoundedRect (bounds, corner);
    paintBevel (g, bounds, corner);
}

void paintTrail (yup::Graphics& g, const yup::Path& area, float x, float width, yup::Color color)
{
    const auto start = x - width;

    yup::ColorGradient gradient;
    gradient.addColorStop (color.withAlpha (0.0f), start, 0.0f, 0.0f);
    gradient.addColorStop (color.withAlpha (0.5f), x, 0.0f, width / (width + 1.0f));
    gradient.addColorStop (color.withAlpha (0.0f), x + 1.0f, 0.0f, 1.0f);

    g.setFillColorGradient (gradient);
    g.fillPath (area);
}

void paintSeam (yup::Graphics& g, yup::Point<float> from, yup::Point<float> to)
{
    const auto vertical = std::abs (to.getX() - from.getX()) < std::abs (to.getY() - from.getY());
    const auto offset = vertical ? yup::Point<float> (1.0f, 0.0f) : yup::Point<float> (0.0f, 1.0f);

    g.setStrokeWidth (1.0f);
    g.setStrokeColor (yup::Colors::black.withAlpha (isDark() ? 0.5f : 0.15f));
    g.strokeLine (from, to);
    g.setStrokeColor (yup::Colors::white.withAlpha (isDark() ? 0.05f : 0.6f));
    g.strokeLine (from + offset, to + offset);
}

//==============================================================================
namespace
{
void paintRaisedKey (yup::Graphics& g, const yup::Button& button, const yup::String& text, bool lit)
{
    const auto bounds = button.getLocalBounds().withTrimmedBottom (1.0f);
    paintRaised (g, button, button.isButtonDown() ? bounds.withY (bounds.getY() + 1.0f) : bounds, wellCorner);

    const auto color = ! button.isEnabled() ? textSecondary (button)
                       : lit                 ? accent (button)
                       : button.isButtonOver() ? textPrimary (button)
                                               : textPrimary (button).withAlpha (0.85f);

    g.setFillColor (color);
    g.fillFittedText (text.toUpperCase(), captionFont (yup::jmin (12.0f, bounds.getHeight() * 0.62f)), bounds.reduced (6.0f, 0.0f), yup::Justification::center);
}
} // namespace

void styleRaisedButton (yup::TextButton& button)
{
    button.setStyle (yup::ComponentStyle::createStyle<yup::TextButton> ([] (yup::Graphics& g, const yup::ApplicationTheme&, const yup::TextButton& b)
    {
        paintRaisedKey (g, b, b.getButtonText(), false);
    }));
}

void styleRaisedToggle (yup::ToggleButton& button)
{
    button.setStyle (yup::ComponentStyle::createStyle<yup::ToggleButton> ([] (yup::Graphics& g, const yup::ApplicationTheme&, const yup::ToggleButton& b)
    {
        paintRaisedKey (g, b, b.getButtonText(), b.getToggleState());
    }));
}

//==============================================================================
yup::Font captionFont (float height)
{
    const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
    return withAxis (withAxis (font, "wdth", 70.0f), "wght", 600.0f);
}

yup::Font valueFont (float height)
{
    const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
    return withAxis (font, "wdth", 75.0f);
}
float textWidth (const yup::String& text, const yup::Font& font)
{
    yup::StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setWrap (yup::StyledText::noWrap);
        modifier.appendText (text, font);
    }

    return std::ceil (styledText.getComputedTextBounds().getWidth()) + 1.0f;
}
} // namespace SynthTheme
