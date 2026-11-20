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

//==============================================================================

class OpaqueBackgroundWidget : public yup::Component
{
public:
    OpaqueBackgroundWidget()
    {
        setTitle ("Opaque Background");
        setOpaque (true);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff1a472a));
        g.fillAll();
    }
};

//==============================================================================

class PathDrawingWidget : public yup::Component
    , public yup::Timer
{
public:
    PathDrawingWidget()
    {
        setTitle ("Path Drawing");
        startTimerHz (60);
    }

    ~PathDrawingWidget() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff1b3a6b));
        g.fillAll();

        const auto bounds = getLocalBounds().to<float>();
        const float cx = bounds.getCenterX();
        const float cy = bounds.getCenterY();
        const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.4f;

        yup::Path path;
        for (int i = 0; i <= 120; ++i)
        {
            const float angle = (float) i / 120.0f * yup::MathConstants<float>::twoPi + phase;
            const float radius = r * (0.5f + 0.5f * std::sin (angle * 3.0f + phase));
            const float px = cx + radius * std::cos (angle);
            const float py = cy + radius * std::sin (angle);
            if (i == 0)
                path.startNewSubPath (px, py);
            else
                path.lineTo (px, py);
        }
        path.closeSubPath();

        g.setFillColor (yup::Color (0xff4a90d9).withAlpha (0.6f));
        g.fillPath (path);
        g.setStrokeColor (yup::Color (0xff80c0ff));
        g.setStrokeWidth (1.5f + strokePhase);
        g.strokePath (path);
    }

    void timerCallback() override
    {
        phase += 0.03f;
        strokePhase = 0.5f + 0.5f * std::sin (phase * 0.7f);
        repaint();
    }

private:
    float phase = 0.0f;
    float strokePhase = 0.0f;
};

//==============================================================================

class TextGradientWidget : public yup::Component
    , public yup::Timer
{
public:
    TextGradientWidget()
    {
        setTitle ("Text Gradient");
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        font = theme->getDefaultFont();
        startTimerHz (30);
    }

    ~TextGradientWidget() override
    {
        stopTimer();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();
        g.setFillColor (yup::Color (0xff2d1b4e));
        g.fillAll();

        const float t = phase;
        g.setFillColor (yup::Color (0xff9b59b6).withAlpha (0.4f + 0.3f * std::sin (t)));
        g.fillRect (bounds.reduced (8.0f));

        g.setFillColor (yup::Colors::white);
        g.fillFittedText ("TextGradient", font, bounds, yup::Justification::center);
    }

    void timerCallback() override
    {
        phase += 0.05f;
        repaint();
    }

private:
    float phase = 0.0f;
    yup::Font font;
};

//==============================================================================

class NestedWidgetGrid : public yup::Component
{
    class GridCell : public yup::Component
    {
    public:
        explicit GridCell (yup::Color color, int index)
            : cellColor (color)
        {
            setTitle ("Grid Cell " + yup::String (index));
            setOpaque (true);
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (cellColor);
            g.fillAll();
        }

    private:
        yup::Color cellColor;
    };

public:
    NestedWidgetGrid()
    {
        setTitle ("Nested Widget Grid");
        for (int i = 0; i < 16; ++i)
        {
            const float hue = (float) i / 16.0f;
            cells.push_back (std::make_unique<GridCell> (yup::Color::fromHSL (hue, 0.7f, 0.5f, 1.0f), i));
            addAndMakeVisible (cells.back().get());
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff1c1c2e));
        g.fillAll();
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().to<float>();
        const int cols = 4, rows = 4;
        const float cellW = bounds.getWidth() / (float) cols;
        const float cellH = bounds.getHeight() / (float) rows;

        for (int i = 0; i < (int) cells.size(); ++i)
        {
            const int col = i % cols;
            const int row = i / cols;
            cells[i]->setBounds ({ col * cellW, row * cellH, cellW, cellH });
        }
    }

private:
    std::vector<std::unique_ptr<GridCell>> cells;
};

//==============================================================================

class PaintProfilerDemo : public yup::Component
{
public:
    PaintProfilerDemo()
    {
        setTitle ("Paint Profiler Demo");
        setWantsKeyboardFocus (true);

        opaqueWidget = std::make_unique<OpaqueBackgroundWidget>();
        addAndMakeVisible (opaqueWidget.get());

        pathWidget = std::make_unique<PathDrawingWidget>();
        addAndMakeVisible (pathWidget.get());

        textWidget = std::make_unique<TextGradientWidget>();
        addAndMakeVisible (textWidget.get());

        nestedGrid = std::make_unique<NestedWidgetGrid>();
        addAndMakeVisible (nestedGrid.get());

        paintProfileSession = yup::PaintProfiler::getInstance().startSession (*this);
    }

    ~PaintProfilerDemo() override
    {
        paintProfileSession.reset();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId)
                            .value_or (yup::Color (0xff282840)));
        g.fillAll();
    }

    void resized() override
    {
        const auto bounds = getLocalBounds().to<float>();
        const float widgetW = bounds.getWidth() / 2.0f;
        const float widgetH = bounds.getHeight() / 2.0f;

        opaqueWidget->setBounds ({ 0.0f, 0.0f, widgetW, widgetH });
        pathWidget->setBounds ({ widgetW, 0.0f, widgetW, widgetH });
        textWidget->setBounds ({ 0.0f, widgetH, widgetW, widgetH });
        nestedGrid->setBounds ({ widgetW, widgetH, widgetW, widgetH });
    }

    bool keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        if (keys.getKey() == yup::KeyPress::textRKey)
        {
            if (paintProfileSession != nullptr)
                paintProfileSession->reset();
        }
        else if (keys.getKey() == yup::KeyPress::textLKey)
        {
            logSnapshot();
        }
        else
        {
            return false;
        }

        return true;
    }

private:
    void logSnapshot()
    {
        if (paintProfileSession == nullptr)
            return;

        auto snap = paintProfileSession->createSnapshot (yup::PaintProfileTimeKind::total, 32);

        yup::Logger::outputDebugString ("Paint profile snapshot frame=" + yup::String (snap.frameIndex));
        yup::Logger::outputDebugString (yup::String::formatted ("%-30s %9s %9s %9s %9s",
                                                                "Widget",
                                                                "last",
                                                                "mean",
                                                                "p95",
                                                                "max"));

        for (const auto& entry : snap.components)
        {
            yup::Logger::outputDebugString (yup::String::formatted ("%-30s %8.2f ms %8.2f ms %8.2f ms %8.2f ms",
                                                                    entry.name.toRawUTF8(),
                                                                    entry.total.lastMicros / 1000.0,
                                                                    entry.total.meanMicros / 1000.0,
                                                                    entry.total.p95Micros / 1000.0,
                                                                    entry.total.maxMicros / 1000.0));
        }

        yup::String globalBuckets = "Global frame histogram total/us buckets:";
        for (int c : snap.globalFrameHistogram.buckets)
            globalBuckets += " " + yup::String (c);
        yup::Logger::outputDebugString (globalBuckets);
    }

    std::unique_ptr<OpaqueBackgroundWidget> opaqueWidget;
    std::unique_ptr<PathDrawingWidget> pathWidget;
    std::unique_ptr<TextGradientWidget> textWidget;
    std::unique_ptr<NestedWidgetGrid> nestedGrid;
    std::unique_ptr<yup::PaintProfiler::ScopedSession> paintProfileSession;
};
