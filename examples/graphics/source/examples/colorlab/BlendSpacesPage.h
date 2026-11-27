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

namespace colorlab
{

//==============================================================================
/** Compares blending two colors in sRGB, in linear RGB and with spectral mixing. */
class BlendSpacesPage final : public yup::Component
{
public:
    BlendSpacesPage()
        : yup::Component ("blendSpacesPage")
    {
        setOpaque (false);

        for (auto* editor : { &startEditor, &endEditor })
        {
            editor->onColorChanged = [this] (yup::Color)
            {
                repaint();
            };
            addAndMakeVisible (editor);
        }

        startEditor.setColor (yup::Color::fromHSLuv (0.08f, 0.85f, 0.6f));
        endEditor.setColor (yup::Color::fromHSLuv (0.7f, 0.8f, 0.45f));
    }

    void paint (yup::Graphics& g) override
    {
        drawPanel (g, rampsPanel);

        auto content = rampsPanel.reduced (20.0f);
        const auto rowHeight = content.getHeight() / 3.0f;

        drawRamp (g, content.removeFromTop (rowHeight), "sRGB", "Straight interpolation of the stored values: muddy in the middle.", yup::ColorSpace::SRGB, 24);
        drawRamp (g, content.removeFromTop (rowHeight), "Linear RGB", "Interpolates light intensity: brighter, more even midtones.", yup::ColorSpace::RGB, 2);
        drawRamp (g, content, "Spectral", "Mixes like paint: blue and yellow make green.", yup::ColorSpace::Spectral, 48);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12.0f);

        auto editors = bounds.removeFromRight (yup::jmin (300.0f, bounds.getWidth() * 0.45f));
        startEditor.setBounds (editors.removeFromTop (ColorChannelEditor::preferredHeight));
        editors.removeFromTop (16.0f);
        endEditor.setBounds (editors.removeFromTop (ColorChannelEditor::preferredHeight));

        rampsPanel = bounds.withTrimmedRight (16.0f);
    }

private:
    void drawRamp (yup::Graphics& g, yup::Rectangle<float> area, const yup::String& name, const yup::String& description, yup::ColorSpace colorSpace, size_t steps)
    {
        g.setFillColor (paletteColor (yup::ThemePalette::Role::text));
        g.fillFittedText (name, themeFont (14.0f), area.removeFromTop (20.0f), yup::Justification::centerLeft);

        g.setFillColor (paletteColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText (description, themeFont (12.0f), area.removeFromTop (18.0f), yup::Justification::centerLeft);

        area = area.reduced (0.0f, 8.0f);

        const auto start = yup::Point<float> (area.getX(), area.getCenterY());
        const auto end = yup::Point<float> (area.getRight(), area.getCenterY());
        g.setFillColorGradient (yup::ColorGradient::fromLinearColors (startEditor.getColor(), start, endEditor.getColor(), end, steps, yup::ColorGradient::Type::Linear, colorSpace));
        g.fillRoundedRect (area, 8.0f);

        g.setStrokeColor (paletteColor (yup::ThemePalette::Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (area, 8.0f);
    }

    yup::Rectangle<float> rampsPanel;
    ColorChannelEditor startEditor { "Start color" };
    ColorChannelEditor endEditor { "End color" };
};

} // namespace colorlab
