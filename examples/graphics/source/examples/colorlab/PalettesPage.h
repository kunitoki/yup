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
/** Shows palettes built with HSLuv and spectral mixing. Click a swatch to copy its color. */
class PalettesPage final : public yup::Component
{
public:
    PalettesPage()
        : yup::Component ("palettesPage")
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        for (const auto& section : sections)
        {
            drawPanel (g, section.panel);

            g.setFillColor (paletteColor (yup::ThemePalette::Role::text));
            g.fillFittedText (section.title, themeFont (14.0f), section.titleArea, yup::Justification::centerLeft);
        }

        for (const auto& swatch : swatches)
        {
            g.setFillColor (swatch.color);
            g.fillRoundedRect (swatch.area, 8.0f);

            if (swatch.area.contains (hoverPosition))
            {
                g.setStrokeColor (paletteColor (yup::ThemePalette::Role::text));
                g.setStrokeWidth (2.0f);
                g.strokeRoundedRect (swatch.area, 8.0f);
            }
        }

        const auto& gallery = sections[2].content;
        const auto top = gallery.withHeight (gallery.getHeight() * 0.5f - 6.0f);
        const auto bottom = gallery.withTrimmedTop (gallery.getHeight() * 0.5f + 6.0f);

        g.setFillColorGradient (yup::ColorGradient::fromLinearColors (yup::Color::fromHSLuv (0.88f, 0.7f, 0.62f), top.getX(), top.getY(), yup::Color::fromHSLuv (0.2f, 0.9f, 0.55f), top.getRight(), top.getBottom(), 6));
        g.fillRoundedRect (top, 10.0f);

        g.setFillColorGradient (yup::ColorGradient::fromLinearColors (yup::Color::fromHSLuv (0.55f, 0.7f, 0.58f), bottom.getCenterX(), bottom.getCenterY(), yup::Color::fromHSLuv (0.95f, 0.2f, 0.2f), bottom.getRight(), bottom.getBottom(), 5, yup::ColorGradient::Radial));
        g.fillRoundedRect (bottom, 10.0f);

        g.setFillColor (paletteColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText (statusText, themeFont (12.0f), statusArea, yup::Justification::centerLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12.0f);
        statusArea = bounds.removeFromBottom (20.0f);
        bounds.removeFromBottom (8.0f);

        const auto sectionHeight = (bounds.getHeight() - 24.0f) / 3.0f;
        for (auto& section : sections)
        {
            section.panel = bounds.removeFromTop (sectionHeight);
            bounds.removeFromTop (12.0f);

            auto content = section.panel.reduced (16.0f, 12.0f);
            section.titleArea = content.removeFromTop (20.0f);
            section.content = content.withTrimmedTop (8.0f);
        }

        swatches.clear();

        // HSLuv keeps the perceived lightness constant across hues, unlike HSL
        constexpr int hues = 12;
        const auto palette = sections[0].content;
        for (int row = 0; row < 2; ++row)
        {
            auto rowArea = palette.withHeight (palette.getHeight() * 0.5f - 4.0f).withY (palette.getY() + row * palette.getHeight() * 0.5f);
            const auto width = rowArea.getWidth() / hues;

            for (int index = 0; index < hues; ++index)
            {
                const auto color = yup::Color::fromHSLuv ((float) index / hues, row == 0 ? 0.85f : 0.6f, row == 0 ? 0.62f : 0.38f);
                swatches.push_back ({ rowArea.removeFromLeft (width).reduced (3.0f, 0.0f), color });
            }
        }

        // Spectral mixing of red and blue goes through purples, as paint does
        constexpr int steps = 11;
        const auto leftColor = yup::Color::fromHSLuv (0.03f, 0.92f, 0.58f);
        const auto rightColor = yup::Color::fromHSLuv (0.65f, 0.85f, 0.55f);
        auto mix = sections[1].content;
        const auto width = mix.getWidth() / steps;
        for (int index = 0; index < steps; ++index)
            swatches.push_back ({ mix.removeFromLeft (width).reduced (3.0f, 0.0f), leftColor.mixedWith (rightColor, (float) index / (steps - 1)) });
    }

    void mouseMove (const yup::MouseEvent& event) override
    {
        hoverPosition = event.getPosition();
        repaint();
    }

    void mouseExit (const yup::MouseEvent&) override
    {
        hoverPosition = { -1.0f, -1.0f };
        repaint();
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        for (const auto& swatch : swatches)
        {
            if (! swatch.area.contains (event.getPosition()))
                continue;

            yup::SystemClipboard::copyTextToClipboard (swatch.color.toString());
            statusText = "Copied " + swatch.color.toString();
            repaint();
            return;
        }
    }

private:
    struct Section
    {
        yup::String title;
        yup::Rectangle<float> panel, titleArea, content;
    };

    struct Swatch
    {
        yup::Rectangle<float> area;
        yup::Color color;
    };

    std::array<Section, 3> sections { { { "HSLuv palette" }, { "Spectral mixing" }, { "Linear and radial gradients" } } };
    std::vector<Swatch> swatches;
    yup::Point<float> hoverPosition { -1.0f, -1.0f };
    yup::Rectangle<float> statusArea;
    yup::String statusText { "Click a swatch to copy its color." };
};

} // namespace colorlab
