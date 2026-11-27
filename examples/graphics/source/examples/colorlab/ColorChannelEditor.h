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
/** The spaces the channel editors can show a color in. */
enum class ColorSpace
{
    rgba,
    hsl,
    hsv,
    hsluv
};

inline yup::Color paletteColor (yup::ThemePalette::Role role)
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (role);
}

inline yup::Font themeFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
}

/** Fills a rounded panel with the surface color and an outline. */
inline void drawPanel (yup::Graphics& g, const yup::Rectangle<float>& area)
{
    g.setFillColor (paletteColor (yup::ThemePalette::Role::surface));
    g.fillRoundedRect (area, 12.0f);
    g.setStrokeColor (paletteColor (yup::ThemePalette::Role::outline));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (area, 12.0f);
}

/** Fills an area with a checkerboard, so translucent colors drawn over it show their alpha. */
inline void fillCheckerboard (yup::Graphics& g, const yup::Rectangle<float>& area, float cellSize = 8.0f)
{
    g.setFillColor (yup::Color (0xffd0d0d0));
    g.fillRect (area);

    yup::Path cells;
    for (float y = 0.0f; y < area.getHeight(); y += cellSize)
    {
        const bool oddRow = int (y / cellSize) % 2 != 0;
        for (float x = oddRow ? cellSize : 0.0f; x < area.getWidth(); x += cellSize * 2.0f)
            cells.addRectangle (area.getX() + x, area.getY() + y, yup::jmin (cellSize, area.getWidth() - x), yup::jmin (cellSize, area.getHeight() - y));
    }

    g.setFillColor (yup::Color (0xff9a9a9a));
    g.fillPath (cells);
}

inline yup::Color colorFromChannels (ColorSpace space, const std::array<float, 4>& c)
{
    switch (space)
    {
        case ColorSpace::hsl:
            return yup::Color::fromHSL (c[0], c[1], c[2], c[3]);
        case ColorSpace::hsv:
            return yup::Color::fromHSV (c[0], c[1], c[2], c[3]);
        case ColorSpace::hsluv:
            return yup::Color::fromHSLuv (c[0], c[1], c[2], c[3]);
        case ColorSpace::rgba:
            break;
    }

    const auto toByte = [] (float value)
    {
        return (yup::uint8) yup::roundToInt (yup::jlimit (0.0f, 1.0f, value) * 255.0f);
    };

    return yup::Color::fromRGBA (toByte (c[0]), toByte (c[1]), toByte (c[2]), toByte (c[3]));
}

inline std::array<float, 4> channelsFromColor (ColorSpace space, yup::Color color)
{
    const auto alpha = color.getAlphaFloat();

    switch (space)
    {
        case ColorSpace::hsl:
        {
            const auto [h, s, l] = color.toHSL();
            return { h, s, l, alpha };
        }
        case ColorSpace::hsv:
        {
            const auto [h, s, v] = color.toHSV();
            return { h, s, v, alpha };
        }
        case ColorSpace::hsluv:
        {
            const auto [h, s, l] = color.toHSLuv();
            return { h, s, l, alpha };
        }
        case ColorSpace::rgba:
            break;
    }

    return { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat(), alpha };
}

//==============================================================================
/**
    Edits a color: a swatch over a checkerboard, a slider per channel of the chosen color
    space, and a hex field to type a color or copy it.
*/
class ColorChannelEditor final : public yup::Component
{
public:
    static constexpr float preferredHeight = 236.0f;

    explicit ColorChannelEditor (yup::StringRef titleToShow)
        : yup::Component ("colorChannelEditor")
        , title (titleToShow)
    {
        setOpaque (false);

        for (size_t index = 0; index < 4; ++index)
        {
            auto slider = sliders.add (std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal));
            slider->setRange (0.0, 1.0, 0.001);
            slider->onValueChanged = [this, index] (double value)
            {
                if (updatingControls)
                    return;

                channels[index] = (float) value;
                setColorInternal (colorFromChannels (space, channels), false);
            };
            addAndMakeVisible (slider);

            auto label = labels.add (std::make_unique<yup::Label> ("channelLabel"));
            label->setJustification (yup::Justification::left);
            addAndMakeVisible (label);
        }

        hexEditor.onTextChange = [this]
        {
            const auto text = hexEditor.getText().trim();
            if (! updatingControls && text.startsWith ("#") && (text.length() == 7 || text.length() == 9))
                setColorInternal (yup::Color::fromString (text), true);
        };
        addAndMakeVisible (hexEditor);

        copyButton.onClick = [this]
        {
            yup::SystemClipboard::copyTextToClipboard (color.toString());
        };
        addAndMakeVisible (copyButton);

        updateControls();
    }

    /** Changes the color, calling onColorChanged when the notification says so. */
    void setColor (yup::Color newColor, yup::NotificationType notification = yup::dontSendNotification)
    {
        color = newColor;
        channels = channelsFromColor (space, color);
        updateControls();

        if (notification != yup::dontSendNotification && onColorChanged)
            onColorChanged (color);
    }

    yup::Color getColor() const { return color; }

    /** Shows the channels of another color space. */
    void setColorSpace (ColorSpace newSpace)
    {
        space = newSpace;
        channels = channelsFromColor (space, color);
        updateControls();
    }

    /** Called when the user changes the color. */
    std::function<void (yup::Color)> onColorChanged;

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (paletteColor (yup::ThemePalette::Role::text));
        g.fillFittedText (title, themeFont (13.0f), titleArea, yup::Justification::centerLeft);

        {
            const auto state = g.saveState();
            g.setClipPath (yup::Path().addRoundedRectangle (swatchArea, 8.0f));
            fillCheckerboard (g, swatchArea);
            g.setFillColor (color);
            g.fillRect (swatchArea);
        }

        g.setStrokeColor (paletteColor (yup::ThemePalette::Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (swatchArea, 8.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        titleArea = bounds.removeFromTop (20.0f);
        bounds.removeFromTop (4.0f);
        swatchArea = bounds.removeFromTop (40.0f);
        bounds.removeFromTop (8.0f);

        for (int index = 0; index < sliders.size(); ++index)
        {
            auto row = bounds.removeFromTop (26.0f);
            labels[index]->setBounds (row.removeFromLeft (64.0f));
            sliders[index]->setBounds (row);
            bounds.removeFromTop (6.0f);
        }

        auto hexRow = bounds.removeFromTop (28.0f);
        copyButton.setBounds (hexRow.removeFromRight (28.0f));
        hexEditor.setBounds (hexRow.withTrimmedRight (6.0f));
    }

private:
    void setColorInternal (yup::Color newColor, bool updateChannels)
    {
        color = newColor;

        if (updateChannels)
            channels = channelsFromColor (space, color);

        updateControls();

        if (onColorChanged)
            onColorChanged (color);
    }

    void updateControls()
    {
        const yup::ScopedValueSetter<bool> updating (updatingControls, true);

        static constexpr const char* names[][4] = {
            { "R", "G", "B", "A" },
            { "H", "S", "L", "A" },
            { "H", "S", "V", "A" },
            { "H", "S", "L", "A" },
        };

        for (size_t index = 0; index < 4; ++index)
        {
            sliders[(int) index]->setValue (channels[index], yup::dontSendNotification);

            const auto value = channels[index];
            const auto text = space == ColorSpace::rgba && index < 3 ? yup::String (yup::roundToInt (value * 255.0f))
                            : space != ColorSpace::rgba && index == 0 ? yup::String (yup::roundToInt (value * 360.0f)) + yup::String::fromUTF8 ("\xc2\xb0")
                                                                       : yup::String (value, 2);

            labels[(int) index]->setText (yup::String (names[(size_t) space][index]) + "  " + text, yup::dontSendNotification);
        }

        if (! hexEditor.hasKeyboardFocus())
            hexEditor.setText (color.toString(), yup::dontSendNotification);

        repaint();
    }

    yup::String title;
    yup::Color color { yup::Colors::white };
    ColorSpace space = ColorSpace::rgba;
    std::array<float, 4> channels { 1.0f, 1.0f, 1.0f, 1.0f };
    bool updatingControls = false;

    yup::Rectangle<float> titleArea, swatchArea;
    yup::OwnedArray<yup::Slider> sliders;
    yup::OwnedArray<yup::Label> labels;
    yup::TextEditor hexEditor { "hexEditor" };
    IconButton copyButton { "copyHex", YUP_ICON_COPY };
};

} // namespace colorlab
