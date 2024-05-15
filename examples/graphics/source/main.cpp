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
#include <juce_audio_devices/juce_audio_devices.h>
#include <yup_graphics/yup_graphics.h>
#include <yup_gui/yup_gui.h>

#include <memory>

//==============================================================================

class CustomSlider : public yup::Component
{
public:
    CustomSlider (int index)
         : index (index)
    {
        setTitle (yup::String (index));
    }

    void mouseEnter (const yup::MouseEvent& event) override
    {
        isInside = true;
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
        isInside = false;
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        origin = event.getPosition();

        takeFocus();
    }

    void mouseUp (const yup::MouseEvent& event) override
    {
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        //auto [x, y] = event.getPosition();

        const float multiplier = event.getModifiers().isShiftDown() ? 0.0001f : 0.0025f;
        const float distance = -origin.verticalDistanceTo (event.getPosition()) * multiplier;

        origin = event.getPosition();

        value = yup::jlimit (0.0f, 1.0f, value + distance);
    }

    void mouseWheel (const yup::MouseEvent& event, const yup::MouseWheelData& data) override
    {
        //auto [x, y] = event.getPosition();

        const float multiplier = event.getModifiers().isShiftDown() ? 0.001f : 0.0025f;
        const float distance = (data.getDeltaX() + data.getDeltaY()) * multiplier;

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

            g.setStrokeCap (yup::StrokeCap::Round);
            g.setColor (yup::Color (0xff636363));
            g.drawPath (arc, proportionOfWidth (0.075f));
        }

        {
            arc.clear();
            arc.addCenteredArc (center,
                                bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f, 0.0f,
                                fromRadians, toCurrentRadians, true);

            g.setStrokeCap (yup::StrokeCap::Round);
            g.setColor (isInside ? yup::Color (0xff4ebfff).brighter (0.3f) : yup::Color (0xff4ebfff));
            g.drawPath (arc, proportionOfWidth (0.075f));
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
    bool isInside = false;
};

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
    , public yup::AudioIODeviceCallback
{
public:
    CustomWindow()
    {
        //getNativeComponent()->enableContinuousRepainting (true);

        setTitle ("main");

        for (int i = 0; i < totalRows * totalColumns; ++i)
            addAndMakeVisible (sliders.add (std::make_unique<CustomSlider> (i)));

        deviceManager.addAudioCallback (this);
        deviceManager.initialiseWithDefaultDevices (1, 0);

        startTimerHz (1);
    }

    ~CustomWindow() override
    {
        inputReady.signal();
        renderReady.signal();

        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (100);
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
        //inputReady.wait();
        //std::swap (inputData, renderData);
        //renderReady.signal();

        float xSize = getWidth() / float (renderData.size());

        yup::Path path;
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        g.setColor (0xff004bff);
        g.drawPath (path, 5.0f);
    }

    void mouseEnter (const yup::MouseEvent& event) override
    {
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        switch (keys.getKey())
        {
        case yup::KeyPress::escapeKey:
            userTriedToCloseWindow();
            break;

        case yup::KeyPress::textAKey:
            getNativeComponent()->enableAtomicMode (!getNativeComponent()->isAtomicModeEnabled());
            fpsLastTime = 0;
            break;

        case yup::KeyPress::textWKey:
            getNativeComponent()->enableWireframe (!getNativeComponent()->isWireframeEnabled());
            break;

        case yup::KeyPress::textZKey:
            setFullScreen (!isFullScreen());
            break;
        }
    }

    void timerCallback() override
    {
        updateWindowTitle();
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        int copiedSamples = 0;
        while (copiedSamples < numSamples)
        {
            renderData[readPos % renderData.size()] = inputChannelData[0][copiedSamples];

            ++copiedSamples;
            ++readPos;

            readPos %= renderData.size();
        }

        //inputReady.signal();
        //renderReady.wait();
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        DBG ("audioDeviceAboutToStart");

        inputData.resize (device->getDefaultBufferSize());
        renderData.resize (device->getDefaultBufferSize());
        readPos = 0;
    }

    void audioDeviceStopped() override
    {
        DBG ("audioDeviceStopped");
    }

private:
    void updateWindowTitle()
    {
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        if (currentFps != 0)
            title << "[" << yup::String (currentFps, 1) << " FPS]";

        title << " | " << "YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        setTitle (title);
    }

    yup::AudioDeviceManager deviceManager;

    std::vector<float> inputData;
    yup::WaitableEvent inputReady;
    std::vector<float> renderData;
    yup::WaitableEvent renderReady;
    int readPos = 0;

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
