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

class CustomComponent : public juce::Component
{
public:
    CustomComponent()
    {
        random.setSeedRandomly();
    }

    void paint (juce::Graphics& g, float frameRate) override
    {
    }

private:
    juce::Random& random = juce::Random::getSystemRandom();
};

//==============================================================================

class CustomWindow : public juce::DocumentWindow
{
    CustomComponent c;

public:
    CustomWindow()
    {
        addAndMakeVisible (c);
    }

    void resized() override
    {
        c.setBounds (getLocalBounds());
    }

    void paint (juce::Graphics& g, float frameRate) override
    {
        double time = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        auto renderer = g.getRenderer();
        auto factory = g.getFactory();

        rive::float2 p[9];
        for (int i = 0; i < 9; ++i)
            p[i] = pts[i] + translate;

        rive::RawPath rawPath;
        rawPath.moveTo (p[0].x, p[0].y);
        rawPath.cubicTo (p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);
        rive::float2 c0 = rive::simd::mix (p[3], p[4], rive::float2 (2 / 3.f));
        rive::float2 c1 = rive::simd::mix (p[5], p[4], rive::float2 (2 / 3.f));
        rawPath.cubicTo (c0.x, c0.y, c1.x, c1.y, p[5].x, p[5].y);
        rawPath.cubicTo (p[6].x, p[6].y, p[7].x, p[7].y, p[8].x, p[8].y);
        if (doClose)
            rawPath.close();

        auto path = factory->makeRenderPath (rawPath, rive::FillRule::nonZero);

        auto fillPaint = factory->makeRenderPaint();
        fillPaint->style (rive::RenderPaintStyle::fill);
        fillPaint->color (-1);

        auto strokePaint = factory->makeRenderPaint();
        strokePaint->style (rive::RenderPaintStyle::stroke);
        strokePaint->color (0x8000ffff);
        strokePaint->thickness (strokeWidth);
        strokePaint->join (join);
        strokePaint->cap (cap);

        renderer->drawPath (path.get(), fillPaint.get());
        renderer->drawPath (path.get(), strokePaint.get());

        // Draw the interactive points.
        auto pointPaint = factory->makeRenderPaint();
        pointPaint->style (rive::RenderPaintStyle::stroke);
        pointPaint->color (0xff0000ff);
        pointPaint->thickness (14);
        pointPaint->cap (rive::StrokeCap::round);

        auto pointPath = factory->makeEmptyRenderPath();
        for (int i : { 1, 2, 4, 6, 7 })
        {
            rive::float2 pt = pts[i] + translate;
            pointPath->moveTo (pt.x, pt.y);
        }

        renderer->drawPath (pointPath.get(), pointPaint.get());

        updateFrameTime (time);
        updateWindowTitle();
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
            fpsFrames = 0;
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

    void updateFrameTime (double time)
    {
        ++fpsFrames;

        double fpsElapsed = time - fpsLastTime;
        if (fpsElapsed > 1)
        {
            currentFps = fpsLastTime == 0 ? 0 : fpsFrames / fpsElapsed;

            updateWindowTitle ();

            fpsFrames = 0;
            fpsLastTime = time;
        }
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
    int fpsFrames = 0;
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
