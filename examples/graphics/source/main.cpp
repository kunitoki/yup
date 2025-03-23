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

#if JUCE_ANDROID
#include <BinaryData.h>
#endif

//==============================================================================

class SineWaveGenerator
{
public:
    SineWaveGenerator()
        : sampleRate (44100.0)
        , currentAngle (0.0)
        , frequency (0.0)
        , amplitude (0.0)
    {
    }

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;

        frequency.reset (newSampleRate, 0.1);
        amplitude.reset (newSampleRate, 0.1);
    }

    void setFrequency (double newFrequency, bool immediate = false)
    {
        if (immediate)
            frequency.setCurrentAndTargetValue ((juce::MathConstants<double>::twoPi * newFrequency) / sampleRate);
        else
            frequency.setTargetValue ((juce::MathConstants<double>::twoPi * newFrequency) / sampleRate);
    }

    void setAmplitude (float newAmplitude)
    {
        amplitude.setTargetValue (newAmplitude);
    }

    float getAmplitude() const
    {
        return amplitude.getCurrentValue();
    }

    float getNextSample()
    {
        auto sample = std::sin (currentAngle) * amplitude.getNextValue();

        currentAngle += frequency.getNextValue();
        if (currentAngle >= juce::MathConstants<double>::twoPi)
            currentAngle -= juce::MathConstants<double>::twoPi;

        return static_cast<float> (sample);
    }

private:
    double sampleRate;
    double currentAngle;
    yup::SmoothedValue<float> frequency;
    yup::SmoothedValue<float> amplitude;
};

//==============================================================================

class Oscilloscope : public yup::Component
{
public:
    Oscilloscope()
        : Component ("Oscilloscope")
    {
    }

    void setRenderData (const std::vector<float>& data, int newReadPos)
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

        path.reserveSpace ((int) renderData.size());
        path.clear();
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        g.setStrokeColor (0xff4b4bff);
        g.setStrokeWidth (2.0f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
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
        setTitle ("main");

#if JUCE_WASM
        auto baseFilePath = yup::File ("/data");
#else
        auto baseFilePath = yup::File (__FILE__).getParentDirectory().getSiblingFile ("data");
#endif

        // Load the font
        {
#if JUCE_ANDROID
            yup::MemoryBlock mb (yup::RobotoRegularFont_data, yup::RobotoRegularFont_size);
            if (auto result = font.loadFromData (mb); result.failed())
                yup::Logger::outputDebugString (result.getErrorMessage());
#else
            auto fontFilePath = baseFilePath.getChildFile ("Roboto-Regular.ttf");
            if (auto result = font.loadFromFile (fontFilePath); result.failed())
                yup::Logger::outputDebugString (result.getErrorMessage());
#endif
        }

        // Load an image
        {
            yup::MemoryBlock mb;
            auto imageFile = baseFilePath.getChildFile ("logo.png");
            if (imageFile.loadFileAsData (mb))
            {
                auto loadedImage = yup::Image::loadFromData (mb.asBytes());
                if (loadedImage.wasOk())
                    image = std::move (loadedImage.getReference());
            }
            else
            {
                yup::Logger::outputDebugString ("Unable to load requested image");
            }
        }

        // Initialize the audio device
        deviceManager.initialiseWithDefaultDevices (0, 2);

        // Initialize sine wave generators
        double sampleRate = deviceManager.getAudioDeviceSetup().sampleRate;
        sineWaveGenerators.resize (totalRows * totalColumns);
        for (size_t i = 0; i < sineWaveGenerators.size(); ++i)
        {
            sineWaveGenerators[i] = std::make_unique<SineWaveGenerator>();
            sineWaveGenerators[i]->setSampleRate (sampleRate);
            sineWaveGenerators[i]->setFrequency (440.0 * std::pow (1.1, i + 1), true);
        }

        deviceManager.addAudioCallback (this);

        // Add sliders
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto slider = sliders.add (std::make_unique<yup::Slider> (yup::String (i)));

            slider->onValueChanged = [this, i, sampleRate] (float value)
            {
                sineWaveGenerators[i]->setFrequency (440.0 * std::pow (1.1 + value, i + 1));
                sineWaveGenerators[i]->setAmplitude (value * 0.5);
            };

            addAndMakeVisible (slider);
        }

        // Add buttons
        button = std::make_unique<yup::TextButton> ("Randomize");
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
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f), proportionOfHeight (0.2f));
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
            button->setBounds (getLocalBounds()
                                   .removeFromTop (proportionOfHeight (0.2f))
                                   .reduced (proportionOfWidth (0.2f), 0.0f));

        oscilloscope.setBounds (getLocalBounds()
                                    .removeFromBottom (proportionOfHeight (0.2f))
                                    .reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));
    }

    void paint (yup::Graphics& g) override
    {
        yup::DocumentWindow::paint (g);
        //g.drawImageAt (image, getLocalBounds().getCenter());
    }

    void paintOverChildren (yup::Graphics& g) override
    {
        if (! image.isValid())
            return;

        g.setBlendMode (yup::BlendMode::ColorDodge);
        g.setOpacity (1.0f);
        g.drawImageAt (image, getLocalBounds().getCenter());
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
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData, readPos);
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
            float totalScale = 0.0f;

            for (int i = 0; i < sineWaveGenerators.size(); ++i)
            {
                mixedSample += sineWaveGenerators[i]->getNextSample();
                totalScale += sineWaveGenerators[i]->getAmplitude();
            }

            if (totalScale > 1.0f)
                mixedSample /= static_cast<float> (totalScale);

            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = mixedSample;

            auto pos = readPos.fetch_add (1);
            inputData[pos] = mixedSample;
            readPos = readPos % inputData.size();
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
        yup::String title;

        auto currentFps = getNativeComponent()->getCurrentFrameRate();
        title << "[" << yup::String (currentFps, 1) << " FPS]";

        title << " (x" << (totalRows * totalColumns) << " instances)";
        title << " | "
              << "YUP On Rive Renderer";

        if (getNativeComponent()->isAtomicModeEnabled())
            title << " (atomic)";

        auto [width, height] = getNativeComponent()->getContentSize();
        title << " | " << width << " x " << height;

        setTitle (title);
    }

    yup::AudioDeviceManager deviceManager;
    std::vector<std::unique_ptr<SineWaveGenerator>> sineWaveGenerators;

    std::vector<float> renderData;
    std::vector<float> inputData;
    yup::CriticalSection renderMutex;
    std::atomic_int readPos = 0;

    yup::OwnedArray<yup::Slider> sliders;
    int totalRows = 4;
    int totalColumns = 4;

    std::unique_ptr<yup::TextButton> button;
    Oscilloscope oscilloscope;

    yup::Font font;
    yup::StyledText styleText;

    yup::Image image;
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

#if JUCE_IOS
        window->centreWithSize ({ 320, 480 });
#elif JUCE_ANDROID
        window->centreWithSize ({ 1080, 2400 });
        // window->setFullScreen(true);
#else
        window->centreWithSize ({ 800, 800 });
#endif

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
