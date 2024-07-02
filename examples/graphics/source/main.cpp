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

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
    , public yup::AudioIODeviceCallback
{
public:
    CustomWindow()
        : yup::DocumentWindow (yup::ComponentNative::defaultFlags, yup::Color (0xff404040))
    {
        rive::Factory* factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
        {
            yup::Logger::outputDebugString ("Failed to create a graphics context");

            yup::YUPApplication::getInstance()->systemRequestedQuit();
            return;
        }

#if JUCE_WASM
        yup::File dataPath = yup::File ("/data");
#else
        yup::File dataPath = yup::File (__FILE__).getParentDirectory().getSiblingFile ("data");
#endif

        yup::File fontFilePath = dataPath.getChildFile ("Roboto-Regular.ttf");

        if (auto result = font.loadFromFile (fontFilePath, factory); result.failed())
            yup::Logger::outputDebugString (result.getErrorMessage());

        setTitle ("main");

        for (int i = 0; i < totalRows * totalColumns; ++i)
            addAndMakeVisible (sliders.add (std::make_unique<yup::Slider> (yup::String (i), font)));

        //button = std::make_unique<yup::TextButton> ("xyz", font);
        //addAndMakeVisible (*button);

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

        for (int i = 0; i < totalRows && sliders.size(); ++i)
        {
            auto row = bounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                sliders.getUnchecked (i * totalRows + j)->setBounds (col.largestFittingSquare());
            }
        }

        if (button != nullptr)
            button->setBounds (getLocalBounds().removeFromTop (80).reduced (proportionOfWidth (0.4f), 0.0f));
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);

        /*
        //inputReady.wait();
        //std::swap (inputData, renderData);
        //renderReady.signal();

        float xSize = getWidth() / float (renderData.size());

        yup::Path path;
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        g.setColor (0xff4b4bff);
        g.drawPath (path, 5.0f);
        */
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        takeFocus();
    }

    void mouseExit (const yup::MouseEvent& event) override
    {
    }

    void keyDown (const yup::KeyPress& keys, const yup::Point<float>& position) override
    {
        switch (keys.getKey())
        {
            case yup::KeyPress::textQKey:
                std::cout << 'a';
                break;

            case yup::KeyPress::escapeKey:
                userTriedToCloseWindow();
                break;

            case yup::KeyPress::textAKey:
                getNativeComponent()->enableAtomicMode (! getNativeComponent()->isAtomicModeEnabled());
                break;

            case yup::KeyPress::textWKey:
                getNativeComponent()->enableWireframe (! getNativeComponent()->isWireframeEnabled());
                break;

            case yup::KeyPress::textZKey:
                setFullScreen (! isFullScreen());
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

        title << "[" << yup::String (getNativeComponent()->getCurrentFrameRate(), 1) << " FPS]";
        title << " | "
              << "YUP On Rive Renderer";

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

    yup::OwnedArray<yup::Slider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

    std::unique_ptr<yup::TextButton> button;

    yup::Font font;
    yup::StyledText styleText;
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

START_JUCE_APPLICATION (Application)
