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

class ProfilerDashboard : public yup::Component
{
public:
    ProfilerDashboard()
    {
        setTitle ("Profiler Dashboard");
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        font = theme->getDefaultFont();
    }

    void setSnapshot (const yup::PaintProfiler::Snapshot& newSnapshot, int selectedIndex)
    {
        snapshot = newSnapshot;
        selectedRow = selectedIndex;
        repaint();
    }

    int getSelectedRow() const { return selectedRow; }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();
        if (bounds.isEmpty())
            return;

        g.setFillColor (yup::Color (0xff1a1a2e));
        g.fillAll();

        const float divX = bounds.getWidth() * 0.60f;
        drawTable (g, bounds.withWidth (divX));
        drawRightPanel (g, bounds.withX (divX + 4.0f).withWidth (bounds.getWidth() - divX - 4.0f));
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        const float divX = getLocalBounds().to<float>().getWidth() * 0.60f;
        if (event.getPosition().getX() >= divX)
            return;

        const float headerH = 24.0f;
        const float rowH = 20.0f;
        const int row = (int) ((event.getPosition().getY() - headerH) / rowH);
        if (row >= 0 && row < (int) snapshot.components.size())
            selectedRow = row;
        repaint();
    }

private:
    static constexpr float kMaxColW = 62.0f;
    static constexpr float kAvgColW = 62.0f;
    static constexpr float kHeaderH = 24.0f;
    static constexpr float kRowH = 20.0f;

    void drawTable (yup::Graphics& g, yup::Rectangle<float> area)
    {
        if (area.isEmpty())
            return;

        const float histColW = area.getWidth() - kMaxColW - kAvgColW;

        auto header = area.removeFromTop (kHeaderH);
        g.setFillColor (yup::Color (0xff252545));
        g.fillRect (header);
        g.setFillColor (yup::Color (0xffaaaacc));

        const float nameW = histColW * 0.42f;

        auto hdrCopy = header.reduced (2, 0);
        g.fillFittedText ("Widget", font, hdrCopy.removeFromLeft (nameW), yup::Justification::centerLeft);
        g.fillFittedText ("max", font, hdrCopy.removeFromLeft (kMaxColW), yup::Justification::centerLeft);
        g.fillFittedText ("avg", font, hdrCopy.removeFromLeft (kAvgColW), yup::Justification::centerLeft);
        g.fillFittedText ("history (green=self / blue=children)", font, hdrCopy, yup::Justification::centerLeft);

        for (int i = 0; i < (int) snapshot.components.size(); ++i)
        {
            if (area.isEmpty())
                break;

            const auto& entry = snapshot.components[i];
            auto row = area.removeFromTop (kRowH);

            const bool isSelected = (i == selectedRow);
            const bool isHot = entry.total.maxMicros > 2000.0;
            const bool isWarm = ! isHot && entry.total.maxMicros > 500.0;

            g.setFillColor (isSelected ? yup::Color (0xff353565)
                            : isHot    ? yup::Color (0xff3a1010)
                            : isWarm   ? yup::Color (0xff3a2a10)
                                       : yup::Color (0xff1e1e38));
            g.fillRect (row);

            g.setFillColor (isHot    ? yup::Color (0xffff6060)
                            : isWarm ? yup::Color (0xffffcc44)
                                     : yup::Colors::white);

            auto r = row.reduced (2, 0);
            g.fillFittedText (entry.name, font, r.removeFromLeft (nameW), yup::Justification::centerLeft);
            g.fillFittedText (formatMicros (entry.total.maxMicros), font, r.removeFromLeft (kMaxColW), yup::Justification::centerLeft);
            g.fillFittedText (formatMicros (entry.total.meanMicros), font, r.removeFromLeft (kAvgColW), yup::Justification::centerLeft);

            drawSparkline (g, r, entry.stats);
        }
    }

    void drawSparkline (yup::Graphics& g, yup::Rectangle<float> area, const yup::PaintProfileStats* stats)
    {
        if (area.isEmpty() || stats == nullptr || stats->getSampleCount() == 0)
            return;

        const auto allSamples = stats->copySamples();
        const int n = std::min ((int) allSamples.size(), 80);
        const int start = (int) allSamples.size() - n;

        const float barW = area.getWidth() / (float) n;
        const float maxUs = 2000.0f;

        for (int i = 0; i < n; ++i)
        {
            const auto& sample = allSamples[(std::size_t) (start + i)];
            const double totalUs = sample.totalMicros;
            const double selfUs = sample.selfMicros;

            const float ratio = std::min (1.0f, (float) (totalUs / maxUs));
            const float totalH = std::max (2.0f, area.getHeight() * ratio);

            const float selfFrac = totalUs > 0.0 ? (float) (selfUs / totalUs) : 1.0f;
            const float selfH = std::max (1.0f, totalH * selfFrac);
            const float childH = totalH - selfH;

            const float x = area.getX() + i * barW;
            const float bW = std::max (1.0f, barW - 0.5f);

            // Self portion (bottom) — green normally, amber/red when hot
            g.setFillColor (totalUs > 2000.0  ? yup::Color (0xffcc2222)
                            : totalUs > 500.0 ? yup::Color (0xffbb8800)
                                              : yup::Color (0xff33aa44));
            g.fillRect ({ x, area.getBottom() - selfH, bW, selfH });

            // Children portion (above self) — blue normally, amber/red when hot
            if (childH > 0.5f)
            {
                g.setFillColor (totalUs > 2000.0  ? yup::Color (0xffff6666)
                                : totalUs > 500.0 ? yup::Color (0xffffcc44)
                                                  : yup::Color (0xff4488cc));
                g.fillRect ({ x, area.getBottom() - totalH, bW, childH });
            }
        }
    }

    void drawRightPanel (yup::Graphics& g, yup::Rectangle<float> area)
    {
        if (area.isEmpty())
            return;

        const float histH = (area.getHeight() - 6.0f) * 0.48f;

        auto globalArea = area.removeFromTop (histH);
        drawHistogramPanel (g, globalArea, "Global frame total", snapshot.globalFrameHistogram, -1.0);

        area.removeFromTop (6.0f);

        if (selectedRow >= 0 && selectedRow < (int) snapshot.components.size())
        {
            const auto& entry = snapshot.components[selectedRow];
            yup::PaintProfileHistogram selHisto;
            if (entry.stats != nullptr)
                selHisto = entry.stats->createHistogram (yup::PaintProfileTimeKind::total, 32);

            auto selArea = area.removeFromTop (histH);
            drawHistogramPanel (g, selArea, entry.name + " total", selHisto, entry.total.p95Micros);

            area.removeFromTop (4.0f);
            g.setFillColor (yup::Color (0xff888899));
            g.fillFittedText (
                yup::String::formatted ("self %.0f  children %.0f  fw %.0f  total %.0f us  [p95 %.0f]",
                                        entry.self.lastMicros,
                                        entry.children.lastMicros,
                                        entry.framework.lastMicros,
                                        entry.total.lastMicros,
                                        entry.total.p95Micros),
                font,
                area.reduced (4, 0),
                yup::Justification::topLeft);
        }
        else
        {
            auto selArea = area.removeFromTop (histH);
            drawHistogramPanel (g, selArea, "Click a row to inspect", {}, -1.0);
        }
    }

    void drawHistogramPanel (yup::Graphics& g,
                             yup::Rectangle<float> area,
                             const yup::String& label,
                             const yup::PaintProfileHistogram& histo,
                             double p95Micros)
    {
        if (area.isEmpty())
            return;

        g.setFillColor (yup::Color (0xff161628));
        g.fillRect (area);

        const float labelH = 15.0f;
        auto labelArea = area.removeFromTop (labelH);
        g.setFillColor (yup::Color (0xff8888bb));
        g.fillFittedText (label, font, labelArea.reduced (3, 0), yup::Justification::centerLeft);

        const float axisH = 13.0f;
        auto axisArea = area.removeFromBottom (axisH);

        if (histo.buckets.empty() || histo.rangeMaxMicros <= 0.0)
            return;

        int maxCount = 0;
        for (int c : histo.buckets)
            maxCount = std::max (maxCount, c);
        if (maxCount == 0)
            return;

        const int numBuckets = (int) histo.buckets.size();
        const float barW = area.getWidth() / (float) numBuckets;
        const double usPerBucket = histo.rangeMaxMicros / numBuckets;

        for (int i = 0; i < numBuckets; ++i)
        {
            const float ratio = (float) histo.buckets[i] / (float) maxCount;
            if (ratio <= 0.0f)
                continue;

            const double bucketMidUs = (i + 0.5) * usPerBucket;
            const yup::Color barColor = bucketMidUs > 2000.0 ? yup::Color (0xffcc3333)
                                      : bucketMidUs > 500.0  ? yup::Color (0xffcc8800)
                                                             : yup::Color (0xff33aa44);

            const float barH = area.getHeight() * ratio;
            g.setFillColor (barColor);
            g.fillRect ({ area.getX() + i * barW,
                          area.getBottom() - barH,
                          std::max (1.0f, barW - 0.5f),
                          barH });
        }

        auto drawThreshold = [&] (double thresholdUs, yup::Color color)
        {
            if (thresholdUs >= histo.rangeMaxMicros)
                return;
            const float x = area.getX() + area.getWidth() * (float) (thresholdUs / histo.rangeMaxMicros);
            g.setFillColor (color.withAlpha (0.55f));
            g.fillRect ({ x - 0.5f, area.getY(), 1.0f, area.getHeight() });
        };

        drawThreshold (500.0, yup::Color (0xffffff00));
        drawThreshold (2000.0, yup::Color (0xffff4444));

        if (p95Micros > 0.0 && p95Micros < histo.rangeMaxMicros)
        {
            const float x = area.getX() + area.getWidth() * (float) (p95Micros / histo.rangeMaxMicros);
            g.setFillColor (yup::Color (0xffffffff).withAlpha (0.7f));
            g.fillRect ({ x - 0.5f, area.getY(), 1.0f, area.getHeight() });
        }

        g.setFillColor (yup::Color (0xff666688));
        g.fillFittedText ("0", font, axisArea.removeFromLeft (28.0f), yup::Justification::centerLeft);

        const float pos500 = area.getWidth() * (float) (500.0 / histo.rangeMaxMicros);
        if (pos500 > 30.0f && pos500 < area.getWidth() - 30.0f)
        {
            g.setFillColor (yup::Color (0xffaaaa44));
            g.fillFittedText (L"500µs", font, { area.getX() + pos500 - 18.0f, axisArea.getY(), 36.0f, axisH }, yup::Justification::center);
        }

        const float pos2ms = area.getWidth() * (float) (2000.0 / histo.rangeMaxMicros);
        if (pos2ms > 30.0f && pos2ms < area.getWidth() - 30.0f)
        {
            g.setFillColor (yup::Color (0xffcc5555));
            g.fillFittedText ("2ms", font, { area.getX() + pos2ms - 16.0f, axisArea.getY(), 32.0f, axisH }, yup::Justification::center);
        }

        g.setFillColor (yup::Color (0xff666688));
        g.fillFittedText (formatMicros (histo.rangeMaxMicros), font, axisArea.removeFromRight (52.0f), yup::Justification::centerRight);
    }

    static yup::String formatMicros (double micros)
    {
        if (micros >= 1000.0)
            return yup::String::formatted ("%.2f ms", micros / 1000.0);
        return yup::String::formatted ("%.0f us", micros);
    }

    yup::PaintProfiler::Snapshot snapshot;

    int selectedRow = -1;
    yup::Font font;
};

//==============================================================================

class ProfilerWindow : public yup::DocumentWindow
{
public:
    explicit ProfilerWindow (std::function<void()> onClose)
        : yup::DocumentWindow (
              yup::ComponentNative::Options()
                  .withResizableWindow (true)
                  .withRenderContinuous (false),
              yup::Color (0xff1a1a2e))
        , closeCallback (std::move (onClose))
    {
        setTitle ("Paint Profiler");
        dashboard = std::make_unique<ProfilerDashboard>();
        addAndMakeVisible (dashboard.get());
    }

    void resized() override
    {
        dashboard->setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        if (closeCallback != nullptr)
            yup::MessageManager::callAsync (closeCallback);
    }

    void updateSnapshot (const yup::PaintProfiler::Snapshot& snap)
    {
        dashboard->setSnapshot (snap, dashboard->getSelectedRow());
    }

private:
    std::unique_ptr<ProfilerDashboard> dashboard;
    std::function<void()> closeCallback;
};

//==============================================================================

class PaintProfilerDemo : public yup::Component
    , public yup::Timer
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

        startTimerHz (10);
    }

    ~PaintProfilerDemo() override
    {
        stopTimer();
        profilerWindow.reset();
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

    void timerCallback() override
    {
        if (paintProfileSession == nullptr || paintProfileSession->isPaused())
            return;

        if (profilerWindow == nullptr || ! profilerWindow->isVisible())
            return;

        auto snap = paintProfileSession->createSnapshot (yup::PaintProfileTimeKind::total, 32);
        profilerWindow->updateSnapshot (snap);
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        if (keys.getKey() == yup::KeyPress::textPKey)
        {
            toggleProfilerWindow();
        }
        else if (keys.getKey() == yup::KeyPress::textRKey)
        {
            if (paintProfileSession != nullptr)
                paintProfileSession->reset();
        }
        else if (keys.getKey() == yup::KeyPress::textLKey)
        {
            logSnapshot();
        }
    }

private:
    void toggleProfilerWindow()
    {
        if (profilerWindow == nullptr)
        {
            profilerWindow = std::make_unique<ProfilerWindow> ([this]
            {
                profilerWindow.reset();
            });
            profilerWindow->centreWithSize ({ 920, 520 });
            profilerWindow->setVisible (true);
            profilerWindow->toFront (true);
        }
        else
        {
            profilerWindow.reset();
        }
    }

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
    std::unique_ptr<ProfilerWindow> profilerWindow;
    std::unique_ptr<yup::PaintProfiler::ScopedSession> paintProfileSession;
};
