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
/** Draws a paint profiler snapshot: a table of the profiled components with their recent
    history, the global frame histogram and the histogram of the selected component.
*/
class PaintProfilerDashboard : public yup::Component
{
public:
    PaintProfilerDashboard()
    {
        setTitle ("Profiler Dashboard");
        auto theme = yup::ApplicationTheme::getGlobalTheme();
        font = theme->getDefaultFont().withHeight (12.0f); // the table rows and axes are sized for 12 px
    }

    void setSnapshot (const yup::PaintProfiler::Snapshot& newSnapshot)
    {
        snapshot = newSnapshot;
        if (selectedRow >= (int) snapshot.components.size())
            selectedRow = -1;
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();
        if (bounds.isEmpty())
            return;

        g.setFillColor (role (yup::ThemePalette::Role::background));
        g.fillAll();

        const auto [tableArea, detailArea] = splitBounds();
        drawTable (g, tableArea);
        drawRightPanel (g, detailArea);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        const auto tableArea = splitBounds().first;
        if (! tableArea.contains (event.getPosition()))
            return;

        const int row = (int) ((event.getPosition().getY() - tableArea.getY() - kHeaderH) / kRowH);
        if (row >= 0 && row < (int) snapshot.components.size())
            selectedRow = row;
        repaint();
    }

private:
    static constexpr float kMaxColW = 62.0f;
    static constexpr float kAvgColW = 62.0f;
    static constexpr float kHeaderH = 24.0f;
    static constexpr float kRowH = 20.0f;

    // The table beside the histograms, or above them when narrow (docked on mobile)
    std::pair<yup::Rectangle<float>, yup::Rectangle<float>> splitBounds() const
    {
        auto bounds = getLocalBounds();

        if (bounds.getWidth() < 600.0f)
        {
            auto table = bounds.removeFromTop (bounds.getHeight() * 0.55f);
            bounds.removeFromTop (4.0f);
            return { table, bounds };
        }

        auto table = bounds.removeFromLeft (bounds.getWidth() * 0.60f);
        bounds.removeFromLeft (4.0f);
        return { table, bounds };
    }

    static yup::Color role (yup::ThemePalette::Role paletteRole)
    {
        return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (paletteRole);
    }

    static yup::Color readable (yup::Color severityColor)
    {
        return severityColor.mixedWith (role (yup::ThemePalette::Role::text), 0.35f, yup::ColorSpace::SRGB);
    }

    void drawTable (yup::Graphics& g, yup::Rectangle<float> area)
    {
        if (area.isEmpty())
            return;

        const float histColW = area.getWidth() - kMaxColW - kAvgColW;

        auto header = area.removeFromTop (kHeaderH);
        g.setFillColor (role (yup::ThemePalette::Role::surfaceRaised));
        g.fillRect (header);
        g.setFillColor (role (yup::ThemePalette::Role::textMuted));

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

            const auto surface = role (yup::ThemePalette::Role::surface);
            g.setFillColor (isSelected ? surface.mixedWith (role (yup::ThemePalette::Role::accent), 0.3f, yup::ColorSpace::SRGB)
                            : isHot    ? surface.mixedWith (yup::Color (0xffcc2222), 0.25f, yup::ColorSpace::SRGB)
                            : isWarm   ? surface.mixedWith (yup::Color (0xffbb8800), 0.25f, yup::ColorSpace::SRGB)
                                       : surface);
            g.fillRect (row);

            g.setFillColor (isHot    ? readable (yup::Color (0xffff6060))
                            : isWarm ? readable (yup::Color (0xffffcc44))
                                     : role (yup::ThemePalette::Role::text));

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

            // Self portion (bottom), green normally, amber/red when hot
            g.setFillColor (totalUs > 2000.0  ? yup::Color (0xffcc2222)
                            : totalUs > 500.0 ? yup::Color (0xffbb8800)
                                              : yup::Color (0xff33aa44));
            g.fillRect ({ x, area.getBottom() - selfH, bW, selfH });

            // Children portion (above self), blue normally, amber/red when hot
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
            g.setFillColor (role (yup::ThemePalette::Role::textMuted));
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

        g.setFillColor (role (yup::ThemePalette::Role::surface));
        g.fillRect (area);

        const float labelH = 15.0f;
        auto labelArea = area.removeFromTop (labelH);
        g.setFillColor (role (yup::ThemePalette::Role::textMuted));
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
            g.setFillColor (role (yup::ThemePalette::Role::text).withAlpha (0.7f));
            g.fillRect ({ x - 0.5f, area.getY(), 1.0f, area.getHeight() });
        }

        g.setFillColor (role (yup::ThemePalette::Role::textMuted));
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

        g.setFillColor (role (yup::ThemePalette::Role::textMuted));
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
/** The paint profiler of the demo app: profiles the paint of the current demo, and shows
    it in a dashboard with a small toolbar to pause, reset and log the collected data.

    Call setTarget() when the demo changes. Samples are only collected while a target is set.
*/
class PaintProfilerPanel final
    : public yup::Component
    , private yup::Timer
{
public:
    PaintProfilerPanel()
        : Component ("paintProfilerPanel")
    {
        setTitle ("Paint Profiler");

        pauseButton.onClick = [this]
        {
            if (session == nullptr)
                return;

            session->setPaused (! session->isPaused());
            pauseButton.setGlyph (session->isPaused() ? YUP_ICON_PLAY : YUP_ICON_PAUSE);
            pauseButton.setOn (session->isPaused());
        };
        addAndMakeVisible (pauseButton);

        resetButton.onClick = [this]
        {
            restartSession();
        };
        addAndMakeVisible (resetButton);

        logButton.onClick = [this]
        {
            logSnapshot();
        };
        addAndMakeVisible (logButton);

        addAndMakeVisible (dashboard);

        startTimerHz (10);
    }

    ~PaintProfilerPanel() override
    {
        session.reset();
    }

    /** Profiles the given component and its children, or nothing with nullptr. */
    void setTarget (yup::Component* newTarget)
    {
        target = newTarget;
        restartSession();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::textMuted));
        g.fillFittedText (targetName, yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f), toolbarBounds.reduced (8.0f, 0.0f), yup::Justification::centerLeft);
    }

    void resized() override
    {
        constexpr float toolbarHeight = 32.0f;

        auto bounds = getLocalBounds();
        toolbarBounds = bounds.removeFromTop (toolbarHeight);

        auto buttons = toolbarBounds.reduced (2.0f);
        logButton.setBounds (buttons.removeFromRight (toolbarHeight));
        resetButton.setBounds (buttons.removeFromRight (toolbarHeight));
        pauseButton.setBounds (buttons.removeFromRight (toolbarHeight));

        dashboard.setBounds (bounds);
    }

private:
    void restartSession()
    {
        // Also disables what timerCallback enabled after the session started
        if (session != nullptr && sessionTarget != nullptr)
            yup::PaintProfiler::getInstance().disableSubtree (*sessionTarget);

        session.reset();
        sessionTarget = target;

        if (target != nullptr)
            session = yup::PaintProfiler::getInstance().startSession (*target);

        targetName = target != nullptr ? target->getTitle() : yup::String();
        if (targetName.isEmpty() && target != nullptr)
            targetName = target->getComponentID();

        pauseButton.setGlyph (YUP_ICON_PAUSE);
        pauseButton.setOn (false);
        dashboard.setSnapshot ({});
        repaint();
    }

    void timerCallback() override
    {
        if (target == nullptr)
        {
            if (session != nullptr)
                restartSession();

            return;
        }

        if (session == nullptr || session->isPaused() || ! isShowing())
            return;

        // Picks up the components the demo created since (tab pages, list rows), keeping the samples of the others
        yup::PaintProfiler::getInstance().enableSubtree (*target);

        dashboard.setSnapshot (session->createSnapshot (yup::PaintProfileTimeKind::total, 32));
    }

    void logSnapshot()
    {
        if (session == nullptr)
            return;

        const auto snap = session->createSnapshot (yup::PaintProfileTimeKind::total, 32);

        yup::Logger::outputDebugString ("Paint profile snapshot frame=" + yup::String (snap.frameIndex));
        yup::Logger::outputDebugString (yup::String::formatted ("%-30s %9s %9s %9s %9s", "Widget", "last", "mean", "p95", "max"));

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
        for (int count : snap.globalFrameHistogram.buckets)
            globalBuckets += " " + yup::String (count);
        yup::Logger::outputDebugString (globalBuckets);
    }

    IconButton pauseButton { "profilerPause", YUP_ICON_PAUSE };
    IconButton resetButton { "profilerReset", YUP_ICON_ARROWS_ROTATE };
    IconButton logButton { "profilerLog", YUP_ICON_COPY };
    PaintProfilerDashboard dashboard;
    yup::WeakReference<yup::Component> target, sessionTarget;
    std::unique_ptr<yup::PaintProfiler::ScopedSession> session;
    yup::Rectangle<float> toolbarBounds;
    yup::String targetName;
};

//==============================================================================
/** A desktop window hosting a PaintProfilerPanel. */
class PaintProfilerWindow final : public yup::DocumentWindow
{
public:
    PaintProfilerWindow (PaintProfilerPanel& panelToShow, std::function<void()> onClose)
        : yup::DocumentWindow (yup::ComponentNative::Options())
        , panel (panelToShow)
        , closeCallback (std::move (onClose))
    {
        setTitle ("Paint Profiler");
        addAndMakeVisible (panel);
    }

    ~PaintProfilerWindow() override
    {
        removeChildComponent (&panel);
    }

    void resized() override
    {
        panel.setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        if (closeCallback != nullptr)
            yup::MessageManager::callAsync (closeCallback);
    }

private:
    PaintProfilerPanel& panel;
    std::function<void()> closeCallback;
};
