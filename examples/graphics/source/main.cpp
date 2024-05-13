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

class CustomSlider : public yup::Component
{
public:
    CustomSlider (int index)
         : index (index)
    {
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        origin = event.getPosition();
    }

    void mouseUp (const yup::MouseEvent& event) override
    {
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        //auto [x, y] = event.getPosition();

        const float distance = origin.horizontalDistanceTo (event.getPosition()) * 0.005f;
        origin = event.getPosition();

        value = yup::jlimit (0.0f, 1.0f, value + distance);
    }

    void paint (yup::Graphics& g, float frameRate) override
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));

        yup::Path path;
        path.addEllipse (bounds.reduced (proportionOfWidth (0.1f)));

        g.setColor (yup::Color (0xff3d3d3d));
        g.fillPath (path);

        g.setColor (yup::Color (0xff2b2b2b));
        g.drawPath (path, proportionOfWidth (0.015f));

        const auto fromRadians = yup::degreesToRadians(135.0f);
        const auto toRadians = fromRadians + yup::degreesToRadians(270.0f);
        const auto toCurrentRadians = fromRadians + yup::degreesToRadians(270.0f) * value;

        const auto center = bounds.to<float>().getCenter();

        yup::Path arc;

        {
            arc.addCenteredArc (center,
                                bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                fromRadians, toRadians, true);

            g.setStrokeCap (yup::StrokeCap::Butt);
            g.setColor (yup::Color (0xff636363));
            g.drawPath (arc, proportionOfWidth (0.1f));
        }

        {
            arc.clear();
            arc.addCenteredArc (center,
                                bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                fromRadians, toCurrentRadians, true);

            g.setStrokeCap (yup::StrokeCap::Round);
            g.setColor (yup::Color (0xff4ebfff));
            g.drawPath (arc, proportionOfWidth (0.1f));
        }

        {
            const auto reducedBounds = bounds.reduced (proportionOfWidth (0.175f));

            auto pos = center.getPointOnCircumference (
                reducedBounds.getWidth() / 2.0f,
                reducedBounds.getHeight() / 2.0f,
                toCurrentRadians);

            arc.clear();
            arc.addLine (yup::Line<float> (pos, center).keepOnlyStart (0.2f));

            g.setStrokeCap (yup::StrokeCap::Round);
            g.setColor (yup::Color (0xffffffff));
            g.drawPath (arc, proportionOfWidth (0.04f));
        }
    }

private:
    yup::Point<float> origin;
    float value = 0.0f;
    int index = 0;
};

//==============================================================================

class CustomWindow : public yup::DocumentWindow
{
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
                sliders.getUnchecked (i * totalRows + j)->setBounds (col.largestFittingSquare());
            }
        }
    }

    void paint (yup::Graphics& g, float frameRate) override
    {
        updateFrameTime();
    }

    void keyDown (const yup::KeyPress& keys, double x, double y) override
    {
        switch (keys.getKey())
        {
        case yup::KeyPress::escapeKey:
            userTriedToCloseWindow();
            break;

        case yup::KeyPress::textAKey:
            forceAtomicMode = !forceAtomicMode;
            fpsLastTime = 0;
            break;

        case yup::KeyPress::textWKey:
            wireframe = !wireframe;
            break;

        case yup::KeyPress::textSKey:
            disableStroke = !disableStroke;
            break;

        case yup::KeyPress::textFKey:
            disableFill = !disableFill;
            break;

        case yup::KeyPress::textZKey:
            setFullScreen (!isFullScreen());
            break;
        }
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    void updateFrameTime()
    {
        double time = yup::Time::getMillisecondCounterHiRes() / 1000.0;
        if (time - fpsLastTime > 1.0)
        {
            updateWindowTitle();
            fpsLastTime = time;
        }
    }

    void updateWindowTitle()
    {
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        if (currentFps != 0)
            title << "[" << yup::String (currentFps, 1) << " FPS]";

        title << " | " << "YUP On Rive Renderer";

        if (forceAtomicMode)
            title << " (atomic)";

        setTitle (title);
    }

    bool forceAtomicMode = false;
    bool wireframe = false;
    bool disableFill = false;
    bool disableStroke = false;

    yup::OwnedArray<CustomSlider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

    double fpsLastTime = 0;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup graphics!";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 800, 800 });
        window->setVisible (true);
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION(Application)
