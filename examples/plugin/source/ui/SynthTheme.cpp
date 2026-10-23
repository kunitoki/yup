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
yup::Color frameColor()
{
    return isDark() ? mix (windowBackground(), yup::Colors::black, 0.5f)
                    : mix (windowBackground(), textPrimary(), 0.25f);
}

yup::Color panelColor()
{
    return isDark() ? role (yup::ThemePalette::Role::surfaceRaised)
                    : mix (windowBackground(), yup::Colors::white, 0.6f);
}

yup::Color stripColor()
{
    return mix (frameColor(), panelColor(), isDark() ? 0.45f : 0.55f);
}

yup::Color raisedColor()
{
    return isDark() ? mix (panelColor(), textPrimary(), 0.12f)
                    : mix (panelColor(), yup::Colors::white, 0.5f);
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

void paintPanel (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setFillColor (panelColor());
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, bounds, panelCorner);
}

void paintTile (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setFillColor (stripColor());
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, bounds, panelCorner);
}

void paintWell (yup::Graphics& g, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (wellColor());
    g.fillRoundedRect (bounds, corner);

    const auto edge = bounds.reduced (0.5f);
    const auto shade = yup::Colors::black.withAlpha (isDark() ? 0.7f : 0.25f);

    g.setStrokeColorGradient (makeVerticalGradient (edge, { { 0.0f, shade }, { 0.4f, shade.withAlpha (0.0f) } }));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (edge, corner);

    g.setStrokeColor (yup::Colors::white.withAlpha (isDark() ? 0.06f : 0.5f));
    g.strokeLine (bounds.getX() + corner, bounds.getBottom() + 0.5f, bounds.getRight() - corner, bounds.getBottom() + 0.5f);
}

void paintRaised (yup::Graphics& g, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (yup::Colors::black.withAlpha (isDark() ? 0.6f : 0.25f));
    g.fillRoundedRect (bounds.withY (bounds.getY() + 1.0f), corner);

    g.setFillColor (raisedColor());
    g.fillRoundedRect (bounds, corner);
    paintBevel (g, bounds, corner);
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
    paintRaised (g, button.isButtonDown() ? bounds.withY (bounds.getY() + 1.0f) : bounds, wellCorner);

    const auto color = ! button.isEnabled() ? textSecondary()
                       : lit                 ? accent()
                       : button.isButtonOver() ? textPrimary()
                                               : textPrimary().withAlpha (0.85f);

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
