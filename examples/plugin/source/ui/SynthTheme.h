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

#include <vector>

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
inline yup::Color panelBackground() { return role (yup::ThemePalette::Role::surface); }
inline yup::Color panelBorder() { return role (yup::ThemePalette::Role::outline); }
inline yup::Color accent() { return role (yup::ThemePalette::Role::accent); }
inline yup::Color accentDim() { return accent().mixedWith (windowBackground(), 0.45f, yup::ColorSpace::SRGB); }
inline yup::Color textPrimary() { return role (yup::ThemePalette::Role::text); }
inline yup::Color textSecondary() { return role (yup::ThemePalette::Role::textMuted); }

inline yup::Color displayBackground()
{
    const auto isDark = yup::ApplicationTheme::getGlobalTheme()->getPalette().getMode() == yup::ThemePalette::Mode::dark;
    return windowBackground().mixedWith (isDark ? yup::Colors::black : yup::Colors::white, 0.35f, yup::ColorSpace::SRGB);
}

constexpr float panelCorner = 6.0f;

/** Colors a toggle button like the rest of the instrument's chrome. */
inline void styleToggle (yup::ToggleButton& button)
{
    button.setColor (yup::ToggleButton::Style::backgroundColorId, displayBackground());
    button.setColor (yup::ToggleButton::Style::backgroundToggledColorId, accentDim());
    button.setColor (yup::ToggleButton::Style::textColorId, textSecondary());
    button.setColor (yup::ToggleButton::Style::textToggledColorId, textPrimary());
    button.setColor (yup::ToggleButton::Style::borderColorId, panelBorder());
    button.setColor (yup::ToggleButton::Style::borderToggledColorId, accent());
}

/** Colors a text button like the rest of the instrument's chrome. */
inline void styleButton (yup::TextButton& button, yup::Color background)
{
    button.setColor (yup::TextButton::Style::backgroundColorId, background);
    button.setColor (yup::TextButton::Style::textColorId, textPrimary());
    button.setColor (yup::TextButton::Style::outlineColorId, panelBorder());
}

/** Colors a combo box like the rest of the instrument's chrome. */
inline void styleComboBox (yup::ComboBox& comboBox)
{
    comboBox.setColor (yup::ComboBox::Style::backgroundColorId, displayBackground());
    comboBox.setColor (yup::ComboBox::Style::textColorId, textPrimary());
    comboBox.setColor (yup::ComboBox::Style::borderColorId, panelBorder());
    comboBox.setColor (yup::ComboBox::Style::arrowColorId, accent());
}
} // namespace SynthTheme

/** @internal Paints the rounded frame every panel of the instrument sits in. */
inline void paintSynthPanel (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setFillColor (SynthTheme::panelBackground());
    g.fillRoundedRect (bounds, SynthTheme::panelCorner);

    g.setStrokeColor (SynthTheme::panelBorder());
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), SynthTheme::panelCorner);
}

/** @internal Spreads controls evenly across a row. */
inline void layoutControlsInRow (yup::Rectangle<float> area, const std::vector<yup::Component*>& controls)
{
    if (controls.empty())
        return;

    const auto width = area.getWidth() / static_cast<float> (controls.size());

    for (auto* control : controls)
        control->setBounds (area.removeFromLeft (width).reduced (3.0f, 0.0f));
}

/** @internal Styles a panel's title label. */
inline void setupPanelTitle (yup::Label& label, const yup::String& title, const yup::Font& font)
{
    label.setText (title, yup::dontSendNotification);
    label.setFont (font.withHeight (12.0f));
    label.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary());
}
