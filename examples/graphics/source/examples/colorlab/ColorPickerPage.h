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
/** Picks a color on an HSV wheel, and edits it in the RGBA, HSL, HSV or HSLuv spaces. */
class ColorPickerPage final : public yup::Component
{
public:
    ColorPickerPage()
        : yup::Component ("colorPickerPage")
    {
        setOpaque (false);

        spaceCombo.addItem ("RGBA", 1);
        spaceCombo.addItem ("HSL", 2);
        spaceCombo.addItem ("HSV", 3);
        spaceCombo.addItem ("HSLuv", 4);
        spaceCombo.setSelectedId (4, yup::dontSendNotification);
        spaceCombo.onSelectedItemChanged = [this]
        {
            editor.setColorSpace (static_cast<ColorSpace> (spaceCombo.getSelectedId() - 1));
        };
        addAndMakeVisible (spaceCombo);

        editor.setColorSpace (ColorSpace::hsluv);
        editor.setColor (yup::Color::fromHSLuv (0.1f, 0.8f, 0.55f));
        editor.onColorChanged = [this] (yup::Color)
        {
            repaint();
        };
        addAndMakeVisible (editor);
    }

    void paint (yup::Graphics& g) override
    {
        drawPanel (g, wheelPanel);
        drawWheel (g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12.0f);

        auto controls = bounds.removeFromRight (yup::jmin (300.0f, bounds.getWidth() * 0.45f));
        spaceCombo.setBounds (controls.removeFromTop (28.0f));
        controls.removeFromTop (12.0f);
        editor.setBounds (controls.removeFromTop (ColorChannelEditor::preferredHeight));

        wheelPanel = bounds.withTrimmedRight (16.0f);

        const auto wheelSize = yup::jmax (0.0f, yup::jmin (wheelPanel.getWidth(), wheelPanel.getHeight()) - 48.0f);
        wheelArea = yup::Rectangle<float> (0.0f, 0.0f, wheelSize, wheelSize).withCenter (wheelPanel.getCenter());
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        isDraggingWheel = wheelArea.contains (event.getPosition());
        mouseDrag (event);
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! isDraggingWheel || wheelArea.isEmpty())
            return;

        const auto offset = event.getPosition() - wheelArea.getCenter();
        const auto saturation = yup::jlimit (0.0f, 1.0f, event.getPosition().distanceTo (wheelArea.getCenter()) / (wheelArea.getWidth() * 0.5f));
        const auto hue = std::fmod (std::atan2 (offset.getY(), offset.getX()) / yup::MathConstants<float>::twoPi + 1.0f, 1.0f);

        const auto current = editor.getColor();
        [[maybe_unused]] const auto [h, s, value] = current.toHSV();
        editor.setColor (yup::Color::fromHSV (hue, saturation, value, current.getAlphaFloat()), yup::sendNotification);
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        isDraggingWheel = false;
    }

private:
    void drawWheel (yup::Graphics& g)
    {
        if (wheelArea.isEmpty())
            return;

        const auto color = editor.getColor();
        const auto [hue, saturation, value] = color.toHSV();

        const auto center = wheelArea.getCenter();
        const auto radius = wheelArea.getWidth() * 0.5f;

        constexpr int segments = 180;
        constexpr float step = 1.0f / segments;
        constexpr float overlap = step * 0.35f;

        for (int i = 0; i < segments; ++i)
        {
            const auto segmentHue = (float) i * step;
            const auto angle1 = (segmentHue - overlap) * yup::MathConstants<float>::twoPi;
            const auto angle2 = (segmentHue + step + overlap) * yup::MathConstants<float>::twoPi;
            const auto edge = center + yup::Point<float> (std::cos (angle2), std::sin (angle2)) * radius;

            g.setFillColorGradient (yup::ColorGradient (yup::Color::fromHSV (segmentHue + step * 0.5f, 0.0f, value),
                                                        center.getX(),
                                                        center.getY(),
                                                        yup::Color::fromHSV (segmentHue + step * 0.5f, 1.0f, value),
                                                        edge.getX(),
                                                        edge.getY(),
                                                        yup::ColorGradient::Radial));

            yup::Path wedge;
            wedge.moveTo (center);
            wedge.lineTo (center + yup::Point<float> (std::cos (angle1), std::sin (angle1)) * radius);
            wedge.addArc (wheelArea, angle1, angle2, false);
            wedge.close();
            g.fillPath (wedge);
        }

        g.setStrokeColor (paletteColor (yup::ThemePalette::Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeEllipse (wheelArea);

        const auto angle = hue * yup::MathConstants<float>::twoPi;
        const auto marker = center + yup::Point<float> (std::cos (angle), std::sin (angle)) * (saturation * radius);
        const auto markerArea = yup::Rectangle<float> (0.0f, 0.0f, 14.0f, 14.0f).withCenter (marker);

        g.setFillColor (color.withAlpha (1.0f));
        g.fillEllipse (markerArea);
        g.setStrokeColor (yup::Colors::white);
        g.setStrokeWidth (2.0f);
        g.strokeEllipse (markerArea);
    }

    yup::Rectangle<float> wheelPanel, wheelArea;
    bool isDraggingWheel = false;

    yup::ComboBox spaceCombo { "pickerSpace" };
    ColorChannelEditor editor { "Color" };
};

} // namespace colorlab
