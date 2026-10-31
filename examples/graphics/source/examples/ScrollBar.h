/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

class ScrollBarDemo;

class ScrollBarDemoCanvas : public yup::Component
{
public:
    explicit ScrollBarDemoCanvas (ScrollBarDemo& demo)
        : owner (demo)
    {
    }

    void paint (yup::Graphics& g) override;

private:
    ScrollBarDemo& owner;
};

class ScrollBarDemo : public yup::Component
{
public:
    ScrollBarDemo()
        : Component ("ScrollBarDemo")
        , canvas (std::make_unique<ScrollBarDemoCanvas> (*this))
    {
        canvas->setSize (canvasWidth, canvasHeight);
        viewport.setViewedComponent (canvas.get());
        addAndMakeVisible (viewport);
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
    }

private:
    friend class ScrollBarDemoCanvas;

    void drawCanvas (yup::Graphics& g)
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();
        const yup::Rectangle<float> canvasBounds (0.0f, 0.0f, canvasWidth, canvasHeight);
        g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
        g.fillRect (canvasBounds);

        g.setStrokeColor (yup::Colors::gray.withAlpha (0.2f));
        g.setStrokeWidth (1.0f);

        for (float x = 0.0f; x <= canvasWidth; x += 100.0f)
            g.strokeLine (x, 0.0f, x, canvasHeight);

        for (float y = 0.0f; y <= canvasHeight; y += 100.0f)
            g.strokeLine (0.0f, y, canvasWidth, y);

        g.setStrokeColor (yup::Colors::gray.withAlpha (0.4f));
        g.setStrokeWidth (2.0f);

        for (float x = 0.0f; x <= canvasWidth; x += 500.0f)
            g.strokeLine (x, 0.0f, x, canvasHeight);

        for (float y = 0.0f; y <= canvasHeight; y += 500.0f)
            g.strokeLine (0.0f, y, canvasWidth, y);

        drawShapes (g);
        drawCoordinateLabels (g);
    }

    void drawShapes (yup::Graphics& g)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                const auto x = 250.0f + col * 500.0f;
                const auto y = 250.0f + row * 500.0f;
                constexpr auto size = 150.0f;

                switch ((row * 4 + col) % 4)
                {
                    case 0:
                        g.setFillColor (yup::Colors::red.withAlpha (0.7f));
                        g.fillEllipse (x - size / 2.0f, y - size / 2.0f, size, size);
                        break;

                    case 1:
                        g.setFillColor (yup::Colors::green.withAlpha (0.7f));
                        g.fillRect (x - size / 2.0f, y - size / 2.0f, size, size);
                        break;

                    case 2:
                        g.setFillColor (yup::Colors::blue.withAlpha (0.7f));
                        g.fillRoundedRect (x - size / 2.0f, y - size / 2.0f, size, size, 20.0f);
                        break;

                    case 3:
                    {
                        yup::Path triangle;
                        triangle.moveTo (x, y - size / 2.0f);
                        triangle.lineTo (x + size / 2.0f, y + size / 2.0f);
                        triangle.lineTo (x - size / 2.0f, y + size / 2.0f);
                        triangle.closeSubPath();

                        g.setFillColor (yup::Colors::yellow.withAlpha (0.7f));
                        g.fillPath (triangle);
                        break;
                    }
                }
            }
        }
    }

    void drawCoordinateLabels (yup::Graphics& g)
    {
        const auto theme = yup::ApplicationTheme::getGlobalTheme();
        const auto font = theme->getDefaultFont();

        for (float x = 0.0f; x <= canvasWidth; x += 500.0f)
        {
            for (float y = 0.0f; y <= canvasHeight; y += 500.0f)
            {
                const yup::String label = "(" + yup::String (static_cast<int> (x)) + ", " + yup::String (static_cast<int> (y)) + ")";

                yup::StyledText text;
                {
                    auto modifier = text.startUpdate();
                    modifier.setMaxSize ({ 120.0f, 20.0f });
                    modifier.setHorizontalAlign (yup::StyledText::left);
                    modifier.appendText (label, font);
                }

                g.setFillColor (theme->getPalette().getColor (yup::ThemePalette::Role::text));
                g.fillFittedText (text, { x + 10.0f, y + 10.0f, 120.0f, 20.0f });
            }
        }
    }

    static constexpr float canvasWidth = 2000.0f;
    static constexpr float canvasHeight = 2000.0f;

    std::unique_ptr<ScrollBarDemoCanvas> canvas;
    yup::Viewport viewport;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollBarDemo)
};

inline void ScrollBarDemoCanvas::paint (yup::Graphics& g)
{
    owner.drawCanvas (g);
}
