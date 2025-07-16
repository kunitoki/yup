/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <memory>
#include <cmath> // For sine wave generation

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
            frequency.setCurrentAndTargetValue ((yup::MathConstants<double>::twoPi * newFrequency) / sampleRate);
        else
            frequency.setTargetValue ((yup::MathConstants<double>::twoPi * newFrequency) / sampleRate);
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
        if (currentAngle >= yup::MathConstants<double>::twoPi)
            currentAngle -= yup::MathConstants<double>::twoPi;

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
        auto bounds = getLocalBounds();

        auto backgroundColor = yup::Color (0xff101010);
        g.setFillColor (backgroundColor);
        g.fillAll();

        auto lineColor = yup::Color (0xff4b4bff);
        if (renderData.empty())
            return;

        float xSize = getWidth() / float (renderData.size());
        float centerY = getHeight() * 0.5f;

        // Build the main waveform path
        path.clear();
        path.reserveSpace ((int) renderData.size());
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (std::size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (i * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        filledPath = path.createStrokePolygon (4.0f);

        g.setFillColor (lineColor);
        g.setFeather (8.0f);
        g.fillPath (filledPath);

        g.setFillColor (lineColor.brighter (0.2f));
        g.setFeather (4.0f);
        g.fillPath (filledPath);

        g.setStrokeColor (lineColor.withAlpha (0.8f));
        g.setStrokeWidth (2.0f);
        g.strokePath (path);

        g.setStrokeColor (lineColor.brighter (0.3f));
        g.setStrokeWidth (1.0f);
        g.strokePath (path);

        g.setStrokeColor (yup::Colors::white.withAlpha (0.9f));
        g.setStrokeWidth (0.5f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
    yup::Path filledPath;
};

//==============================================================================

class AudioExample
    : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::MidiKeyboardState::Listener
{
public:
    AudioExample()
        : Component ("AudioExample")
        , keyboardComponent (keyboardState, yup::MidiKeyboardComponent::horizontalKeyboard)
    {
        // Initialize the audio device
        deviceManager.initialiseWithDefaultDevices (0, 2);

        // Initialize sine wave generators for MIDI notes (128 possible notes)
        double sampleRate = deviceManager.getAudioDeviceSetup().sampleRate;
        sineWaveGenerators.resize (128);
        for (int i = 0; i < 128; ++i)
        {
            sineWaveGenerators[i] = std::make_unique<SineWaveGenerator>();
            sineWaveGenerators[i]->setSampleRate (sampleRate);
            // Convert MIDI note to frequency: f = 440 * 2^((n-69)/12)
            double frequency = 440.0 * std::pow (2.0, (i - 69) / 12.0);
            sineWaveGenerators[i]->setFrequency (frequency, true);
            sineWaveGenerators[i]->setAmplitude (0.0f); // Start silent
        }

        // Set up MIDI keyboard
        keyboardState.addListener (this);
        keyboardComponent.setAvailableRange (36, 84); // C2 to C6
        keyboardComponent.setLowestVisibleKey (48);   // Start from C3
        keyboardComponent.setMidiChannel (1);
        keyboardComponent.setVelocity (0.7f);
        addAndMakeVisible (keyboardComponent);

        // Add sliders for manual control (reduced number for layout)
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto slider = sliders.add (std::make_unique<yup::Slider> (yup::String (i)));

            slider->onValueChanged = [this, i, sampleRate] (float value)
            {
                // Map sliders to a specific range of notes for demonstration
                int noteNumber = 60 + i; // Start from middle C
                if (noteNumber < 128)
                {
                    double baseFreq = 440.0 * std::pow (2.0, (noteNumber - 69) / 12.0);
                    sineWaveGenerators[noteNumber]->setFrequency (baseFreq * (1.0 + value * 0.5));
                    sineWaveGenerators[noteNumber]->setAmplitude (value * 0.3f);
                }
            };

            addAndMakeVisible (slider);
        }

        // Add buttons
        button = std::make_unique<yup::TextButton> ("Randomize");
        button->onClick = [this]
        {
            for (int i = 0; i < sliders.size(); ++i)
                sliders[i]->setValue (yup::Random::getSystemRandom().nextFloat());
        };
        addAndMakeVisible (*button);

        // Add clear all notes button
        clearButton = std::make_unique<yup::TextButton> ("Clear All Notes");
        clearButton->onClick = [this]
        {
            keyboardState.allNotesOff (0); // Turn off all notes on all channels
        };
        addAndMakeVisible (*clearButton);

        // Add the oscilloscope
        addAndMakeVisible (oscilloscope);

        // Add volume control
        volumeSlider = std::make_unique<yup::Slider> ("Volume");
        volumeSlider->onValueChanged = [this] (float value)
        {
            masterVolume = value;
        };
        volumeSlider->setValue (0.5f); // Set initial volume to 50%
        addAndMakeVisible (*volumeSlider);
    }

    ~AudioExample() override
    {
        keyboardState.removeListener (this);
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Reserve space for MIDI keyboard at the bottom
        auto keyboardHeight = proportionOfHeight (0.20f);
        auto keyboardBounds = bounds.removeFromBottom (keyboardHeight);
        keyboardComponent.setBounds (keyboardBounds.reduced (proportionOfWidth (0.02f), proportionOfHeight (0.01f)));

        // Reserve space for oscilloscope above the keyboard
        auto oscilloscopeHeight = proportionOfHeight (0.2f);
        auto oscilloscopeBounds = bounds.removeFromBottom (oscilloscopeHeight);
        oscilloscope.setBounds (oscilloscopeBounds.reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        // Reserve space for buttons at the top
        bounds.removeFromTop (proportionOfHeight (0.1f));
        auto buttonHeight = proportionOfHeight (0.10f);
        auto buttonArea = bounds.removeFromTop (buttonHeight);

        auto buttonWidth = buttonArea.getWidth() / 3;
        if (button != nullptr)
            button->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        if (clearButton != nullptr)
            clearButton->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        if (volumeSlider != nullptr)
            volumeSlider->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        // Use remaining space for sliders
        auto sliderBounds = bounds.reduced (proportionOfWidth (0.1f), proportionOfHeight (0.05f));
        auto width = sliderBounds.getWidth() / totalColumns;
        auto height = sliderBounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows && sliders.size(); ++i)
        {
            auto row = sliderBounds.removeFromTop (height);
            for (int j = 0; j < totalColumns; ++j)
            {
                auto col = row.removeFromLeft (width);
                sliders.getUnchecked (i * totalColumns + j)->setBounds (col.largestFittingSquare());
            }
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();

        // Draw some labels
        auto bounds = getLocalBounds();
        auto titleArea = bounds.removeFromTop (proportionOfHeight (0.05f));
        auto subtitleArea = bounds.removeFromTop (proportionOfHeight (0.03f));

        yup::StyledText titleText;
        {
            auto modifier = titleText.startUpdate();
            modifier.setMaxSize (titleArea.getSize());
            modifier.setHorizontalAlign (yup::StyledText::center);
            modifier.appendText ("YUP Audio Synthesis Example with MIDI Keyboard",
                               yup::ApplicationTheme::getGlobalTheme()->getDefaultFont(), 16.0f);
        }

        yup::StyledText subtitleText;
        {
            auto modifier = subtitleText.startUpdate();
            modifier.setMaxSize (subtitleArea.getSize());
            modifier.setHorizontalAlign (yup::StyledText::center);
            modifier.appendText ("Use the MIDI keyboard below or adjust sliders to generate tones",
                               yup::ApplicationTheme::getGlobalTheme()->getDefaultFont(), 12.0f);
        }

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (titleText, titleArea);
        g.fillFittedText (subtitleText, subtitleArea);
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        takeKeyboardFocus();
    }

    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData, readPos);
        }

        if (oscilloscope.isVisible())
            oscilloscope.repaint();
    }

    // MIDI keyboard event handlers
    void handleNoteOn (yup::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128)
        {
            sineWaveGenerators[midiNoteNumber]->setAmplitude (velocity * 0.5f);
        }
    }

    void handleNoteOff (yup::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (midiNoteNumber >= 0 && midiNoteNumber < 128)
        {
            sineWaveGenerators[midiNoteNumber]->setAmplitude (0.0f);
        }
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
            int activeNotes = 0;

            // Mix all active MIDI notes
            for (int i = 0; i < 128; ++i)
            {
                float amplitude = sineWaveGenerators[i]->getAmplitude();
                if (amplitude > 0.001f) // Only process notes that are actually sounding
                {
                    mixedSample += sineWaveGenerators[i]->getNextSample();
                    activeNotes++;
                }
            }

            // Apply master volume and normalize if multiple notes are playing
            if (activeNotes > 0)
            {
                mixedSample *= masterVolume;
                if (activeNotes > 1)
                    mixedSample /= std::sqrt (static_cast<float> (activeNotes)); // Gentle normalization
            }

            // Apply soft limiting to prevent clipping
            mixedSample = std::tanh (mixedSample);

            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = mixedSample;

            // Store for oscilloscope display
            auto pos = readPos.fetch_add (1);
            inputData[pos] = mixedSample;
            readPos = readPos % inputData.size();
        }

        const yup::CriticalSection::ScopedLockType sl (renderMutex);
        std::swap (inputData, renderData);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        inputData.resize (device->getDefaultBufferSize());
        renderData.resize (device->getDefaultBufferSize());
        readPos = 0;
    }

    void audioDeviceStopped() override
    {
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            deviceManager.removeAudioCallback (this);
        else
            deviceManager.addAudioCallback (this);
    }

private:
    yup::AudioDeviceManager deviceManager;
    std::vector<std::unique_ptr<SineWaveGenerator>> sineWaveGenerators;

    // MIDI keyboard components
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;

    std::vector<float> renderData;
    std::vector<float> inputData;
    yup::CriticalSection renderMutex;
    std::atomic_int readPos = 0;

    yup::OwnedArray<yup::Slider> sliders;
    int totalRows = 3;
    int totalColumns = 4;

    std::unique_ptr<yup::TextButton> button;
    std::unique_ptr<yup::TextButton> clearButton;
    std::unique_ptr<yup::Slider> volumeSlider;
    Oscilloscope oscilloscope;

    float masterVolume = 0.5f;
};
