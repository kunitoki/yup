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
#include <cmath> // For sine wave generation

//==============================================================================

class SineWaveGenerator
{
public:
    SineWaveGenerator()
        : sampleRate (44100.0)
        , currentAngle (0.0)
        , angleDelta (0.0)
        , amplitude (0.0)
    {
    }

    void setFrequency (double frequency, double newSampleRate)
    {
        sampleRate = newSampleRate;
        angleDelta = (juce::MathConstants<double>::twoPi * frequency) / sampleRate;

        amplitude.reset (sampleRate, 0.1);
    }

    void setAmplitude (float newAmplitude)
    {
        amplitude.setTargetValue (newAmplitude);
    }

    float getNextSample()
    {
        auto sample = std::sin (currentAngle) * amplitude.getNextValue();

        currentAngle += angleDelta;
        if (currentAngle >= juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        return static_cast<float> (sample);
    }

private:
    double sampleRate;
    double currentAngle;
    double angleDelta;
    yup::SmoothedValue<float> amplitude;
};

//==============================================================================

class Oscilloscope : public yup::Component
{
public:
    Oscilloscope ()
        : Component ("Oscilloscope")
    {
    }

    void setRenderData (const std::vector<float>& data)
    {
        renderData.resize (data.size());

        for (std::size_t i = 0; i < data.size(); ++i)
            renderData[i] = data[i];
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (0xff101010);
        g.fillAll();

        if (renderData.empty())
            return;

        float xSize = getWidth() / float (renderData.size());

        yup::Path path;
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        g.setStrokeColor (0xff4b4bff);
        g.setStrokeWidth (2.0f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
};

//==============================================================================

class CustomWindow
    : public yup::DocumentWindow
    , public yup::Timer
    , public yup::AudioIODeviceCallback
{
public:
    CustomWindow()
        : yup::DocumentWindow ({}, yup::Color (0xff404040))
    {
        rive::Factory* factory = getNativeComponent()->getFactory();
        if (factory == nullptr)
        {
            yup::Logger::outputDebugString ("Failed to create a graphics context");

            yup::YUPApplication::getInstance()->systemRequestedQuit();
            return;
        }

        setTitle ("main");

        // Load the font
        yup::File fontFilePath =
#if JUCE_WASM
            yup::File ("/data")
#else
            yup::File (__FILE__).getParentDirectory().getSiblingFile ("data")
#endif
            .getChildFile ("Roboto-Regular.ttf");

        if (auto result = font.loadFromFile (fontFilePath, factory); result.failed())
            yup::Logger::outputDebugString (result.getErrorMessage());

        // Initialize the audio device
        deviceManager.addAudioCallback (this);
        deviceManager.initialiseWithDefaultDevices (1, 0);

        // Initialize sine wave generators
        double sampleRate = deviceManager.getAudioDeviceSetup().sampleRate;
        sineWaveGenerators.resize (totalRows * totalColumns);
        for (size_t i = 0; i < sineWaveGenerators.size(); ++i)
        {
            sineWaveGenerators[i] = std::make_unique<SineWaveGenerator>();
            sineWaveGenerators[i]->setFrequency(440.0 * std::pow(1.1, i), sampleRate);
        }

        // Add sliders
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto slider = sliders.add (std::make_unique<yup::Slider> (yup::String (i), font));

            slider->onValueChanged = [this, i](float value)
            {
                sineWaveGenerators[i]->setAmplitude (value);
            };

            addAndMakeVisible (slider);
        }

        // Add buttons
        button = std::make_unique<yup::TextButton> ("Randomize", font);
        button->onClick = [this]
        {
            for (int i = 0; i < sliders.size(); ++i)
                sliders[i]->setValue (juce::Random::getSystemRandom().nextFloat());
        };
        addAndMakeVisible (*button);

        // Add the oscilloscope
        addAndMakeVisible (oscilloscope);

        // Start the timer
        startTimerHz (60);
    }

    ~CustomWindow() override
    {
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

        oscilloscope.setBounds (getLocalBounds().removeFromBottom (120).reduced (200, 10));
    }


    void mouseDown (const yup::MouseEvent& event) override
    {
        takeFocus();
    }

    void mouseDoubleClick (const yup::MouseEvent& event) override
    {
        DBG ("mouseDoubleClick");
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

        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData);
        }

        oscilloscope.repaint();
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
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mixedSample = 0.0f;

            for (int i = 0; i < sineWaveGenerators.size(); ++i)
                mixedSample += sineWaveGenerators[i]->getNextSample();

            mixedSample /= static_cast<float> (sineWaveGenerators.size());

            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = mixedSample;

            inputData[readPos++] = mixedSample;
            readPos %= inputData.size();
        }

        const yup::CriticalSection::ScopedLockType sl (renderMutex);
        std::swap (inputData, renderData);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        const yup::CriticalSection::ScopedLockType sl (renderMutex);

        inputData.resize (device->getDefaultBufferSize());
        renderData.resize (device->getDefaultBufferSize());
        readPos = 0;
    }

    void audioDeviceStopped() override
    {
    }

private:
    void updateWindowTitle()
    {
        /*
        yup::String title;

        title << "[" << yup::String (getNativeComponent()->getCurrentFrameRate(), 1) << " FPS]";
        title << " | "
              << "YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        setTitle (title);
        */
    }

    yup::AudioDeviceManager deviceManager;
    std::vector<std::unique_ptr<SineWaveGenerator>> sineWaveGenerators;

    std::vector<float> renderData;
    std::vector<float> inputData;
    yup::CriticalSection renderMutex;
    int readPos = 0;

    yup::OwnedArray<yup::Slider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

    std::unique_ptr<yup::TextButton> button;
    Oscilloscope oscilloscope;

    yup::Font font;
    yup::StyledText styleText;
};

//==============================================================================

struct Application : yup::YUPApplication
{
    Application() = default;

    const yup::String getApplicationName() override
    {
        return "yup! graphics";
    }

    const yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        YUP_PROFILE_START();

        yup::Logger::outputDebugString ("Starting app " + commandLineParameters);

        window = std::make_unique<CustomWindow>();
        window->centreWithSize ({ 800, 800 });
        window->setVisible (true);
    }

    void shutdown() override
    {
        yup::Logger::outputDebugString ("Shutting down");

        window.reset();

        YUP_PROFILE_STOP();
    }

private:
    std::unique_ptr<CustomWindow> window;
};

START_JUCE_APPLICATION (Application)

