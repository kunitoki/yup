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

class ScrollBarDemo : public yup::Component
{
public:
    ScrollBarDemo()
        : Component ("ScrollBarDemo")
    {
        // Set want to grab focus
        setWantsKeyboardFocus (true);

        // Create a large virtual canvas (2000x2000)
        canvasWidth = 2000.0f;
        canvasHeight = 2000.0f;

        // Create vertical scrollbar
        verticalScrollBar = std::make_unique<yup::ScrollBar> (yup::ScrollBar::Orientation::vertical);
        verticalScrollBar->setVisibilityMode (yup::ScrollBar::VisibilityMode::alwaysVisible);
        verticalScrollBar->setRangeLimits (0.0, canvasHeight);
        verticalScrollBar->onScrollPositionChanged = [this] (double newPosition)
        {
            scrollY = static_cast<float> (newPosition);
            repaint();
        };
        addAndMakeVisible (verticalScrollBar.get());

        // Create horizontal scrollbar
        horizontalScrollBar = std::make_unique<yup::ScrollBar> (yup::ScrollBar::Orientation::horizontal);
        horizontalScrollBar->setVisibilityMode (yup::ScrollBar::VisibilityMode::alwaysVisible);
        horizontalScrollBar->setRangeLimits (0.0, canvasWidth);
        horizontalScrollBar->onScrollPositionChanged = [this] (double newPosition)
        {
            scrollX = static_cast<float> (newPosition);
            repaint();
        };
        addAndMakeVisible (horizontalScrollBar.get());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto scrollBarSize = 15.0f;

        // Set the scrollbar width to match our layout
        verticalScrollBar->setScrollBarWidth (scrollBarSize);
        horizontalScrollBar->setScrollBarWidth (scrollBarSize);

        // Position vertical scrollbar on the right (full height minus horizontal scrollbar)
        verticalScrollBar->setBounds (
            bounds.getWidth() - scrollBarSize,
            0.0f,
            scrollBarSize,
            bounds.getHeight() - scrollBarSize);

        // Position horizontal scrollbar at the bottom (full width minus vertical scrollbar)
        horizontalScrollBar->setBounds (
            0.0f,
            bounds.getHeight() - scrollBarSize,
            bounds.getWidth() - scrollBarSize,
            scrollBarSize);

        // Calculate viewport size (excluding scrollbar areas)
        auto viewportWidth = bounds.getWidth() - scrollBarSize;
        auto viewportHeight = bounds.getHeight() - scrollBarSize;

        // Clamp scroll positions to valid range
        scrollX = yup::jlimit (0.0f, canvasWidth - viewportWidth, scrollX);
        scrollY = yup::jlimit (0.0f, canvasHeight - viewportHeight, scrollY);

        // Update scrollbar ranges - the range represents the visible portion
        verticalScrollBar->setCurrentRange (scrollY, scrollY + viewportHeight);
        horizontalScrollBar->setCurrentRange (scrollX, scrollX + viewportWidth);
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto scrollBarSize = 15.0f;
        auto viewportWidth = bounds.getWidth() - scrollBarSize;
        auto viewportHeight = bounds.getHeight() - scrollBarSize;

        // Draw background for entire component (including scrollbar areas)
        g.setFillColor (yup::Color (0xff1a1a1a));
        g.fillAll();

        // Define the viewport (content area without scrollbars)
        yup::Rectangle<float> viewport (0.0f, 0.0f, viewportWidth, viewportHeight);

        // Calculate visible region in canvas coordinates
        float visibleLeft = scrollX;
        float visibleTop = scrollY;
        float visibleRight = scrollX + viewportWidth;
        float visibleBottom = scrollY + viewportHeight;

        // Draw content with clipping and translation
        {
            auto state = g.saveState();

            // Translate to viewport origin (no scrolling applied yet)
            auto currentTransform = g.getTransform();
            g.setTransform (currentTransform.translated (-scrollX, -scrollY));

            // Clip path ?
            auto bounds = getBounds();
            g.setClipPath (yup::Rectangle<float> (getLeft() + visibleLeft, getTop() + visibleTop, viewportWidth, viewportHeight));

            // Draw canvas background (only visible portion)
            g.setFillColor (yup::Color (0xff2a2a2a));
            g.fillRect (visibleLeft, visibleTop, viewportWidth, viewportHeight);

            // Draw grid lines (only in visible area)
            g.setStrokeColor (yup::Colors::gray.withAlpha (0.2f));
            g.setStrokeWidth (1.0f);

            // Vertical grid lines - only draw visible ones
            float startX = std::floor (visibleLeft / 100.0f) * 100.0f;
            for (float x = startX; x <= visibleRight; x += 100.0f)
                g.strokeLine (x, visibleTop, x, visibleBottom);

            // Horizontal grid lines - only draw visible ones
            float startY = std::floor (visibleTop / 100.0f) * 100.0f;
            for (float y = startY; y <= visibleBottom; y += 100.0f)
                g.strokeLine (visibleLeft, y, visibleRight, y);

            // Draw major grid lines (only visible ones)
            g.setStrokeColor (yup::Colors::gray.withAlpha (0.4f));
            g.setStrokeWidth (2.0f);

            startX = std::floor (visibleLeft / 500.0f) * 500.0f;
            for (float x = startX; x <= visibleRight; x += 500.0f)
                g.strokeLine (x, visibleTop, x, visibleBottom);

            startY = std::floor (visibleTop / 500.0f) * 500.0f;
            for (float y = startY; y <= visibleBottom; y += 500.0f)
                g.strokeLine (visibleLeft, y, visibleRight, y);

            // Draw colorful shapes (only visible ones)
            drawShapes (g, visibleLeft, visibleTop, visibleRight, visibleBottom);

            // Draw coordinate labels (only visible ones)
            drawCoordinateLabels (g, visibleLeft, visibleTop, visibleRight, visibleBottom);
        }

        // Draw viewport border
        g.setStrokeColor (yup::Colors::white.withAlpha (0.3f));
        g.setStrokeWidth (1.0f);
        g.strokeRect (viewport);

        // Draw scroll position indicator (always on top)
        drawScrollIndicator (g, viewportWidth, viewportHeight);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        takeKeyboardFocus();
    }

    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& wheelData) override
    {
        // Handle mouse wheel scrolling
        auto bounds = getLocalBounds();
        auto scrollBarSize = 15.0f;
        auto viewportWidth = bounds.getWidth() - scrollBarSize;
        auto viewportHeight = bounds.getHeight() - scrollBarSize;

        // Vertical scrolling
        if (std::abs (wheelData.getDeltaY()) > std::abs (wheelData.getDeltaX()))
        {
            auto newScrollY = scrollY - wheelData.getDeltaY() * 20.0f;
            newScrollY = yup::jlimit (0.0f, canvasHeight - viewportHeight, newScrollY);
            verticalScrollBar->setCurrentRangeStart (newScrollY);
        }
        // Horizontal scrolling
        else
        {
            auto newScrollX = scrollX - wheelData.getDeltaX() * 20.0f;
            newScrollX = yup::jlimit (0.0f, canvasWidth - viewportWidth, newScrollX);
            horizontalScrollBar->setCurrentRangeStart (newScrollX);
        }
    }

private:
    void drawShapes (yup::Graphics& g, float visibleLeft, float visibleTop, float visibleRight, float visibleBottom)
    {
        // Draw a pattern of colored shapes across the canvas (only visible ones)
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                float x = 250.0f + col * 500.0f;
                float y = 250.0f + row * 500.0f;
                float size = 150.0f;

                // Skip shapes outside visible area (with some margin)
                if (x + size / 2 < visibleLeft - 50.0f || x - size / 2 > visibleRight + 50.0f || y + size / 2 < visibleTop - 50.0f || y - size / 2 > visibleBottom + 50.0f)
                {
                    continue;
                }

                // Alternate between different shapes and colors
                int shapeType = (row * 4 + col) % 4;

                switch (shapeType)
                {
                    case 0: // Red circle
                        g.setFillColor (yup::Colors::red.withAlpha (0.7f));
                        g.fillEllipse (x - size / 2, y - size / 2, size, size);
                        break;

                    case 1: // Green rectangle
                        g.setFillColor (yup::Colors::green.withAlpha (0.7f));
                        g.fillRect (x - size / 2, y - size / 2, size, size);
                        break;

                    case 2: // Blue rounded rectangle
                        g.setFillColor (yup::Colors::blue.withAlpha (0.7f));
                        g.fillRoundedRect (x - size / 2, y - size / 2, size, size, 20.0f);
                        break;

                    case 3: // Yellow triangle (using path)
                    {
                        yup::Path triangle;
                        triangle.moveTo (x, y - size / 2);
                        triangle.lineTo (x + size / 2, y + size / 2);
                        triangle.lineTo (x - size / 2, y + size / 2);
                        triangle.closeSubPath();

                        g.setFillColor (yup::Colors::yellow.withAlpha (0.7f));
                        g.fillPath (triangle);
                        break;
                    }
                }
            }
        }
    }

    void drawCoordinateLabels (yup::Graphics& g, float visibleLeft, float visibleTop, float visibleRight, float visibleBottom)
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        // Draw coordinate labels at major grid intersections (only visible ones)
        float startX = std::floor (visibleLeft / 500.0f) * 500.0f;
        float startY = std::floor (visibleTop / 500.0f) * 500.0f;

        for (float x = startX; x <= visibleRight && x <= canvasWidth; x += 500.0f)
        {
            for (float y = startY; y <= visibleBottom && y <= canvasHeight; y += 500.0f)
            {
                yup::String label = "(" + yup::String (static_cast<int> (x)) + ", " + yup::String (static_cast<int> (y)) + ")";

                yup::StyledText text;
                {
                    auto modifier = text.startUpdate();
                    modifier.setMaxSize ({ 120.0f, 20.0f });
                    modifier.setHorizontalAlign (yup::StyledText::left);
                    modifier.appendText (label, font);
                }

                g.setFillColor (yup::Colors::white);
                g.fillFittedText (text, yup::Rectangle<float> (x + 10.0f, y + 10.0f, 120.0f, 20.0f));
            }
        }
    }

    void drawScrollIndicator (yup::Graphics& g, float viewportWidth, float viewportHeight)
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        // Draw scroll position in top-left corner
        yup::String info = "Scroll: (" + yup::String (static_cast<int> (scrollX)) + ", " + yup::String (static_cast<int> (scrollY)) + ")";

        yup::StyledText scrollText;
        {
            auto modifier = scrollText.startUpdate();
            modifier.setMaxSize ({ 150.0f, 20.0f });
            modifier.setHorizontalAlign (yup::StyledText::left);
            modifier.appendText (info, font);
        }

        // Draw background for text
        g.setFillColor (yup::Colors::black.withAlpha (0.7f));
        g.fillRoundedRect (10.0f, 10.0f, 160.0f, 30.0f, 5.0f);

        // Draw text
        g.setFillColor (yup::Colors::white);
        g.fillFittedText (scrollText, yup::Rectangle<float> (15.0f, 15.0f, 150.0f, 20.0f));

        // Draw canvas info in top-right corner
        yup::String canvasInfo = "Canvas: " + yup::String (static_cast<int> (canvasWidth)) + "x" + yup::String (static_cast<int> (canvasHeight));

        yup::StyledText canvasText;
        {
            auto modifier = canvasText.startUpdate();
            modifier.setMaxSize ({ 150.0f, 20.0f });
            modifier.setHorizontalAlign (yup::StyledText::left);
            modifier.appendText (canvasInfo, font);
        }

        g.setFillColor (yup::Colors::black.withAlpha (0.7f));
        g.fillRoundedRect (viewportWidth - 170.0f, 10.0f, 160.0f, 30.0f, 5.0f);

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (canvasText, yup::Rectangle<float> (viewportWidth - 165.0f, 15.0f, 150.0f, 20.0f));
    }

private:
    std::unique_ptr<yup::ScrollBar> verticalScrollBar;
    std::unique_ptr<yup::ScrollBar> horizontalScrollBar;

    float canvasWidth = 2000.0f;
    float canvasHeight = 2000.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScrollBarDemo)
};
