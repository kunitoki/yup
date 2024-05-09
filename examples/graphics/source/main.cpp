/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

#include "rive/math/simd.hpp"

#include <memory>

//==============================================================================

class CustomSlider : public juce::Component
{
    int index = 0;

public:
    CustomSlider (int index)
         : index (index)
    {
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        origin = event.getPosition();
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        //auto [x, y] = event.getPosition();

        const float distance = origin.distanceX (event.getPosition()) * 0.005f;
        origin = event.getPosition();

        value = juce::jlimit (0.0f, 1.0f, value + distance);
    }

    void paint (juce::Graphics& g, float frameRate) override
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));

        juce::Path path;
        path.addEllipse (bounds.reduced (proportionOfWidth (0.1f)));

        g.setColor (0xff3d3d3d);
        g.fillPath (path);

        g.setColor (0xff2b2b2b);
        g.drawPath (path, proportionOfWidth (0.01f));

        const auto fromRadians = juce::degreesToRadians(135.0f);
        const auto toRadians = fromRadians + juce::degreesToRadians(270.0f);
        const auto toCurrentRadians = fromRadians + juce::degreesToRadians(270.0f) * value;

        const auto center = bounds.to<float>().getCenter();

        juce::Path arc;

        {
            arc.addCenteredArc (center,
                                bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                fromRadians, toRadians, true);

            g.setStrokeCap (juce::StrokeCap::Butt);
            g.setColor (0xff636363);
            g.drawPath (arc, proportionOfWidth (0.1f));
        }

        {
            arc.clear();
            arc.addCenteredArc (center,
                                bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                fromRadians, toCurrentRadians, true);

            g.setStrokeCap (juce::StrokeCap::Round);
            g.setColor (0xff4ebfff);
            g.drawPath (arc, proportionOfWidth (0.1f));
        }

        {
            const auto reducedBounds = bounds.reduced (proportionOfWidth (0.175f));

            auto pos = center.getPointOnCircumference (
                reducedBounds.getWidth() / 2.0f,
                reducedBounds.getHeight() / 2.0f,
                toCurrentRadians);

            arc.clear();
            arc.addLine (juce::Line<float> (pos, center).keepOnlyStart (0.2f));

            g.setStrokeCap (juce::StrokeCap::Round);
            g.setColor (0xffffffff);
            g.drawPath (arc, proportionOfWidth (0.04f));
        }
    }

private:
    juce::Point<float> origin;
    float value = 0.0f;
};

//==============================================================================

class CustomWindow : public juce::DocumentWindow
{
    juce::OwnedArray<CustomSlider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

public:
    CustomWindow()
    {
        for (int i = 0; i < totalRows * totalColumns; ++i)
            addAndMakeVisible (sliders.add (std::make_unique<CustomSlider> (i)));
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (200);
        auto width = bounds.getWidth() / totalColumns;
        auto height = bounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows; ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                sliders.getUnchecked (i * totalRows + j)->setBounds (col.largerSquareFitting());
            }
        }
    }

    void paint (juce::Graphics& g, float frameRate) override
    {
        const double time = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        updateFrameTime (time);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        auto [x, y] = event.getPosition();

        dragLastPos = rive::float2 { x, y };
        if (event.isLeftButtoDown())
        {
            dragIdx = -1;
            for (int i = 0; i < kNumInteractivePts; ++i)
            {
                if (rive::simd::all (rive::simd::abs (dragLastPos - (pts[i] + translate)) < 20))
                {
                    dragIdx = i;
                    break;
                }
            }
        }
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
    }

    void mouseMove (const juce::MouseEvent& event) override
    {
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        auto [x, y] = event.getPosition();

        if (event.isLeftButtoDown())
        {
            rive::float2 pos = rive::float2 { x, y };
            if (dragIdx >= 0)
                pts[dragIdx] += (pos - dragLastPos);
            else
                translate += (pos - dragLastPos);

            dragLastPos = pos;
        }
    }

    void keyDown (const juce::KeyPress& keys, double x, double y) override
    {
        switch (keys.getKey())
        {
        case juce::KeyPress::escapeKey:
            userTriedToCloseWindow();
            break;

        case juce::KeyPress::textAKey:
            forceAtomicMode = !forceAtomicMode;
            fpsLastTime = 0;
            break;

        case juce::KeyPress::textDKey:
            printf ("static float scale = %f;\n", scale);
            printf ("static float2 translate = {%f, %f};\n", translate.x, translate.y);
            printf ("static float2 pts[] = {");
            for (int i = 0; i < kNumInteractivePts; i++)
            {
                printf("{%g, %g}", pts[i].x, pts[i].y);
                if (i < kNumInteractivePts - 1)
                    printf(", ");
                else
                    printf("};\n");
            }
            fflush(stdout);
            break;

        case juce::KeyPress::number1Key:
            strokeWidth /= 1.5f;
            break;

        case juce::KeyPress::number2Key:
            strokeWidth *= 1.5f;
            break;

        case juce::KeyPress::textWKey:
            wireframe = !wireframe;
            break;

        case juce::KeyPress::textCKey:
            cap = static_cast<rive::StrokeCap> ((static_cast<int> (cap) + 1) % 3);
            break;

        case juce::KeyPress::textOKey:
            doClose = !doClose;
            break;

        case juce::KeyPress::textSKey:
            disableStroke = !disableStroke;
            break;

        case juce::KeyPress::textFKey:
            disableFill = !disableFill;
            break;

        case juce::KeyPress::textZKey:
            setFullScreen (!isFullScreen());
            break;
        }
    }

    void userTriedToCloseWindow() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    void updateFrameTime (double time)
    {
        double fpsElapsed = time - fpsLastTime;
        if (fpsElapsed > 1)
        {
            currentFps = getNativeComponent()->getCurrentFrameRate();
            updateWindowTitle();

            fpsLastTime = time;
        }
    }

    void updateWindowTitle()
    {
        juce::String title;

        if (currentFps != 0)
            title << "[" << juce::String (currentFps, 1) << " FPS]";

        title << " | " << "YUP On Rive Renderer";

        if (forceAtomicMode)
            title << " (atomic)";

        setTitle (title);
    }

    bool forceAtomicMode = false;
    bool wireframe = false;
    bool disableFill = false;
    bool disableStroke = false;

    rive::float2 pts[9] = {
        {260 + 2 * 100, 60 + 2 * 500},
        {260 + 2 * 257, 60 + 2 * 233},
        {260 + 2 * -100, 60 + 2 * 300},
        {260 + 2 * 100, 60 + 2 * 200},
        {260 + 2 * 250, 60 + 2 * 0},
        {260 + 2 * 400, 60 + 2 * 200},
        {260 + 2 * 213, 60 + 2 * 200},
        {260 + 2 * 213, 60 + 2 * 300},
        {260 + 2 * 391, 60 + 2 * 480}
    };

    static constexpr int kNumInteractivePts = sizeof(pts) / sizeof(*pts);

    float strokeWidth = 70;
    rive::float2 translate;
    float scale = 1;
    rive::StrokeJoin join = rive::StrokeJoin::miter;
    rive::StrokeCap cap = rive::StrokeCap::butt;
    bool doClose = false;
    int dragIdx = -1;
    rive::float2 dragLastPos;

    int lastWidth = 0, lastHeight = 0;
    double fpsLastTime = 0;
    double currentFps = 0;
};

//==============================================================================

struct Application : juce::JUCEApplication
{
    Application() = default;

    const juce::String getApplicationName() override
    {
        return "yup graphics!";
    }

    const juce::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const juce::String& commandLineParameters) override
    {
        juce::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 800, 800 });
        window->setVisible (true);
    }

    void shutdown() override
    {
        juce::Logger::outputDebugString ("Shutting down");

        window.reset();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION(Application)
