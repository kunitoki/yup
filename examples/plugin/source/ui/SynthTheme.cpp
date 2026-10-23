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
const yup::Identifier sectionProperty ("synthSection");
const yup::Identifier frameColorId ("synthFrame");
const yup::Identifier baseAccentId ("synthBaseAccent");

yup::Identifier getSectionAccentId (Section section)
{
    return yup::Identifier ("synthSectionAccent" + yup::String (static_cast<int> (section)));
}

yup::Color wellFrom (const yup::Component& scope, yup::Color background)
{
    return mix (background, isDark (scope) ? yup::Colors::black : yup::Colors::white, 0.35f);
}

void applySectionColors (yup::Component& component)
{
    const auto well = wellColor (component);
    const auto text = textPrimary (component);
    const auto muted = textSecondary (component);
    const auto sectionAccent = accent (component);

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

void applySectionPalettes (yup::Component& component, const SectionPalettes& sections)
{
    if (const auto section = component.getProperties().getVarPointer (sectionProperty); section != nullptr)
    {
        component.setPalette (sections[static_cast<std::size_t> (static_cast<int> (*section))]);
        applySectionColors (component);
    }

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        applySectionPalettes (*component.getChildComponent (index), sections);
}
} // namespace

void applyTheme (yup::Component& root, const yup::ThemePalette& base, const SectionPalettes& sections)
{
    root.setPalette (base);

    const auto background = base.getColor (yup::ThemePalette::Role::background);
    const auto text = base.getColor (yup::ThemePalette::Role::text);
    root.setColor (frameColorId, isDark (root) ? mix (background, yup::Colors::black, 0.5f) : mix (background, text, 0.25f));
    root.setColor (baseAccentId, base.getColor (yup::ThemePalette::Role::accent));

    for (int section = 0; section < sectionCount; ++section)
    {
        const auto& palette = sections[static_cast<std::size_t> (section)];
        root.setColor (getSectionAccentId (static_cast<Section> (section)), (palette.has_value() ? *palette : base).getColor (yup::ThemePalette::Role::accent));
    }

    applySectionPalettes (root, sections);
}

void applySection (yup::Component& component, Section section)
{
    component.getProperties().set (sectionProperty, static_cast<int> (section));
}

yup::Color role (const yup::Component& scope, yup::ThemePalette::Role paletteRole)
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette (scope).getColor (paletteRole);
}

bool isDark (const yup::Component& scope)
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette (scope).getMode() == yup::ThemePalette::Mode::dark;
}

yup::Color sectionAccent (const yup::Component& scope, Section section)
{
    return scope.findColor (getSectionAccentId (section)).value_or (accent (scope));
}

yup::Color sourceAccent (const yup::Component& scope, int index)
{
    if (index <= 0)
        return sectionAccent (scope, Section::envelopes);

    if (index == 1)
        return sectionAccent (scope, Section::lfos);

    const auto [hue, saturation, lightness] = scope.findColor (baseAccentId).value_or (accent (scope)).toHSL();
    const auto turn = static_cast<float> (yup::jmin (index, 3) - 1) / 3.0f;
    return yup::Color::fromHSL (std::fmod (hue + turn, 1.0f), saturation, lightness);
}

//==============================================================================
yup::Color frameColor (const yup::Component& scope)
{
    return scope.findColor (frameColorId).value_or (windowBackground (scope));
}

yup::Color panelColor (const yup::Component& scope)
{
    return isDark (scope) ? role (scope, yup::ThemePalette::Role::surfaceRaised)
                          : mix (windowBackground (scope), yup::Colors::white, 0.6f);
}

yup::Color stripColor (const yup::Component& scope)
{
    return mix (frameColor (scope), panelColor (scope), isDark (scope) ? 0.45f : 0.55f);
}

yup::Color raisedColor (const yup::Component& scope)
{
    return isDark (scope) ? mix (panelColor (scope), textPrimary (scope), 0.12f)
                          : mix (panelColor (scope), yup::Colors::white, 0.5f);
}

yup::Color wellColor (const yup::Component& scope)
{
    return wellFrom (scope, windowBackground (scope));
}

//==============================================================================
void paintBevel (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner)
{
    const auto highlight = yup::Colors::white.withAlpha (isDark (scope) ? 0.08f : 0.6f);
    const auto shade = yup::Colors::black.withAlpha (isDark (scope) ? 0.45f : 0.15f);
    const auto edge = bounds.reduced (0.5f);

    g.setStrokeColorGradient (makeVerticalGradient (edge, { { 0.0f, highlight }, { 0.35f, highlight.withAlpha (0.0f) }, { 0.65f, shade.withAlpha (0.0f) }, { 1.0f, shade } }));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (edge, corner);
}

void paintPanel (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds)
{
    g.setFillColor (panelColor (scope));
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, scope, bounds, panelCorner);
}

void paintTile (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds)
{
    g.setFillColor (stripColor (scope));
    g.fillRoundedRect (bounds, panelCorner);
    paintBevel (g, scope, bounds, panelCorner);
}

void paintWell (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (wellColor (scope));
    g.fillRoundedRect (bounds, corner);

    const auto edge = bounds.reduced (0.5f);
    const auto shade = yup::Colors::black.withAlpha (isDark (scope) ? 0.7f : 0.25f);

    g.setStrokeColorGradient (makeVerticalGradient (edge, { { 0.0f, shade }, { 0.4f, shade.withAlpha (0.0f) } }));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (edge, corner);

    g.setStrokeColor (yup::Colors::white.withAlpha (isDark (scope) ? 0.06f : 0.5f));
    g.strokeLine (bounds.getX() + corner, bounds.getBottom() + 0.5f, bounds.getRight() - corner, bounds.getBottom() + 0.5f);
}

void paintRaised (yup::Graphics& g, const yup::Component& scope, yup::Rectangle<float> bounds, float corner)
{
    g.setFillColor (yup::Colors::black.withAlpha (isDark (scope) ? 0.6f : 0.25f));
    g.fillRoundedRect (bounds.withY (bounds.getY() + 1.0f), corner);

    g.setFillColor (raisedColor (scope));
    g.fillRoundedRect (bounds, corner);
    paintBevel (g, scope, bounds, corner);
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

void paintSeam (yup::Graphics& g, const yup::Component& scope, yup::Point<float> from, yup::Point<float> to)
{
    const auto vertical = std::abs (to.getX() - from.getX()) < std::abs (to.getY() - from.getY());
    const auto offset = vertical ? yup::Point<float> (1.0f, 0.0f) : yup::Point<float> (0.0f, 1.0f);

    g.setStrokeWidth (1.0f);
    g.setStrokeColor (yup::Colors::black.withAlpha (isDark (scope) ? 0.5f : 0.15f));
    g.strokeLine (from, to);
    g.setStrokeColor (yup::Colors::white.withAlpha (isDark (scope) ? 0.05f : 0.6f));
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
