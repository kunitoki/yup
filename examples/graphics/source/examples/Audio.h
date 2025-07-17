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

class HarmonicSineGenerator
{
public:
    HarmonicSineGenerator()
        : sampleRate (44100.0)
        , currentAngle (0.0)
        , frequency (0.0)
        , amplitude (0.0)
    {
    }

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;
        frequency.reset (newSampleRate, 0.05);
        amplitude.reset (newSampleRate, 0.02);
    }

    void setFrequency (double newFrequency)
    {
        frequency.setTargetValue ((yup::MathConstants<double>::twoPi * newFrequency) / sampleRate);
    }

    void setAmplitude (float newAmplitude)
    {
        amplitude.setTargetValue (newAmplitude);
    }

    float getCurrentAmplitude() const
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

class HarmonicSynth
{
public:
    HarmonicSynth()
        : isNoteOn (false)
        , currentNote (-1)
        , fundamentalFrequency (0.0)
        , masterAmplitude (0.5f)
    {
        // Initialize harmonic generators
        const int numHarmonics = 16; // 4x4 grid
        harmonicGenerators.resize (numHarmonics);
        harmonicMultipliers.resize (numHarmonics);
        harmonicAmplitudes.resize (numHarmonics);

        for (int i = 0; i < numHarmonics; ++i)
        {
            harmonicGenerators[i] = std::make_unique<HarmonicSineGenerator>();

            // Set up harmonic relationships (1st, 2nd, 3rd harmonic, etc., plus some non-integer ratios)
            if (i < 8)
                harmonicMultipliers[i] = (i + 1); // 1x, 2x, 3x, 4x, 5x, 6x, 7x, 8x
            else
                harmonicMultipliers[i] = (i - 7) * 0.5 + 0.5; // 0.5x, 1x, 1.5x, 2x, 2.5x, 3x, 3.5x, 4x

            harmonicAmplitudes[i] = 0.0f; // Start silent
        }
    }

    void setSampleRate (double newSampleRate)
    {
        for (auto& generator : harmonicGenerators)
            generator->setSampleRate (newSampleRate);
    }

    void noteOn (int midiNoteNumber, float velocity)
    {
        currentNote = midiNoteNumber;
        isNoteOn = true;

        // Convert MIDI note to frequency: f = 440 * 2^((n-69)/12)
        fundamentalFrequency = 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);

        updateHarmonicFrequencies();
        updateHarmonicAmplitudes (velocity);
    }

    void noteOff (int midiNoteNumber)
    {
        if (currentNote == midiNoteNumber)
        {
            isNoteOn = false;
            for (auto& generator : harmonicGenerators)
                generator->setAmplitude (0.0f);
        }
    }

    void allNotesOff()
    {
        isNoteOn = false;
        currentNote = -1;
        for (auto& generator : harmonicGenerators)
            generator->setAmplitude (0.0f);
    }

    void setHarmonicAmplitude (int harmonicIndex, float amplitude)
    {
        if (harmonicIndex >= 0 && harmonicIndex < harmonicAmplitudes.size())
        {
            harmonicAmplitudes[harmonicIndex] = amplitude;
            if (isNoteOn)
                updateHarmonicAmplitudes (1.0f); // Use current velocity
        }
    }

    void setMasterAmplitude (float newAmplitude)
    {
        masterAmplitude = newAmplitude;
        if (isNoteOn)
            updateHarmonicAmplitudes (1.0f);
    }

    float getMasterAmplitude() const
    {
        return masterAmplitude;
    }

    bool isPlaying() const
    {
        if (! isNoteOn)
            return false;

        for (const auto& generator : harmonicGenerators)
        {
            if (generator->getCurrentAmplitude() > 0.001f)
                return true;
        }
        return false;
    }

    int getCurrentNote() const
    {
        return currentNote;
    }

    float getNextSample()
    {
        float mixedSample = 0.0f;

        for (auto& generator : harmonicGenerators)
        {
            mixedSample += generator->getNextSample();
        }

        return mixedSample * masterAmplitude;
    }

    double getHarmonicMultiplier (int index) const
    {
        if (index >= 0 && index < harmonicMultipliers.size())
            return harmonicMultipliers[index];
        return 1.0;
    }

private:
    void updateHarmonicFrequencies()
    {
        for (size_t i = 0; i < harmonicGenerators.size(); ++i)
        {
            double harmonicFreq = fundamentalFrequency * harmonicMultipliers[i];
            harmonicGenerators[i]->setFrequency (harmonicFreq);
        }
    }

    void updateHarmonicAmplitudes (float velocity)
    {
        for (size_t i = 0; i < harmonicGenerators.size(); ++i)
        {
            float amplitude = harmonicAmplitudes[i] * velocity * masterAmplitude;
            harmonicGenerators[i]->setAmplitude (amplitude);
        }
    }

    std::vector<std::unique_ptr<HarmonicSineGenerator>> harmonicGenerators;
    std::vector<double> harmonicMultipliers;
    std::vector<float> harmonicAmplitudes;

    bool isNoteOn;
    int currentNote;
    double fundamentalFrequency;
    float masterAmplitude;
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

        // Initialize harmonic synthesizer
        double sampleRate = deviceManager.getAudioDeviceSetup().sampleRate;
        harmonicSynth.setSampleRate (sampleRate);

        // Set up MIDI keyboard
        keyboardState.addListener (this);
        keyboardComponent.setAvailableRange (36, 84); // C2 to C6
        keyboardComponent.setLowestVisibleKey (48);   // Start from C3
        keyboardComponent.setMidiChannel (1);
        keyboardComponent.setVelocity (0.7f);
        addAndMakeVisible (keyboardComponent);

        // Create title and subtitle labels
        titleLabel = std::make_unique<yup::Label> ("Title");
        titleLabel->setText ("YUP Harmonic Synthesizer");
        //titleLabel->setJustification (yup::Justification::centred);
        //titleLabel->setFont (16.0f);
        titleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        addAndMakeVisible (*titleLabel);

        subtitleLabel = std::make_unique<yup::Label> ("Subtitle");
        subtitleLabel->setText ("Each knob controls a harmonic of the played note - experiment to create rich tones!");
        //subtitleLabel->setJustification (yup::Justification::centred);
        //subtitleLabel->setFont (12.0f);
        subtitleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        addAndMakeVisible (*subtitleLabel);

        // Create note indicator label
        noteIndicatorLabel = std::make_unique<yup::Label> ("NoteIndicator");
        noteIndicatorLabel->setText ("");
        //noteIndicatorLabel->setJustification (yup::Justification::centred);
        //noteIndicatorLabel->setFont (12.0f);
        noteIndicatorLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::black);
        noteIndicatorLabel->setColor (yup::Label::Style::backgroundColorId, yup::Colors::yellow.withAlpha (0.8f));
        addChildComponent (*noteIndicatorLabel);

        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        // Add harmonic control sliders (4x4 grid)
        for (int i = 0; i < totalRows * totalColumns; ++i)
        {
            auto slider = sliders.add (std::make_unique<yup::Slider> (yup::Slider::RotaryVerticalDrag));

            // Configure slider range and default value
            slider->setRange (0.0f, 1.0f);
            slider->setDefaultValue (0.0f);

            slider->onValueChanged = [this, i] (float value)
            {
                harmonicSynth.setHarmonicAmplitude (i, value * 0.4f); // Scale down to prevent clipping
            };

            addAndMakeVisible (slider);

            // Create harmonic labels for each slider
            auto label = harmonicLabels.add (std::make_unique<yup::Label> (yup::String ("HarmonicLabel") + yup::String (i)));
            //label->setJustificationType (yup::Justification::centred);
            //label->setFont (10.0f);
            label->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
            label->setFont (font.withHeight (8.0f));

            // Set the harmonic multiplier text
            auto multiplier = harmonicSynth.getHarmonicMultiplier (i);
            label->setText (yup::String (multiplier, 1) + "x", yup::dontSendNotification);

            addAndMakeVisible (*label);
        }

        // Add buttons
        randomizeButton = std::make_unique<yup::TextButton> ("Randomize");
        randomizeButton->onClick = [this]
        {
            for (int i = 0; i < sliders.size(); ++i)
                sliders[i]->setValue (yup::Random::getSystemRandom().nextFloat());
        };
        addAndMakeVisible (*randomizeButton);

        // Add clear all notes button
        clearButton = std::make_unique<yup::TextButton> ("All Notes Off");
        clearButton->onClick = [this]
        {
            keyboardState.allNotesOff (0); // Turn off all notes on all channels
            harmonicSynth.allNotesOff();
        };
        addAndMakeVisible (*clearButton);

        // Add volume control
        volumeSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal, "Volume");

        // Configure slider range and default value
        volumeSlider->setRange ({ 0.0f, 1.0f });
        volumeSlider->setDefaultValue (0.5f);

        volumeSlider->onValueChanged = [this] (float value)
        {
            masterVolume = value;
        };
        volumeSlider->setValue (0.5f); // Set initial volume to 50%
        addAndMakeVisible (*volumeSlider);

        // Add the oscilloscope
        addAndMakeVisible (oscilloscope);

        // Set some initial harmonic values for a nice sound
        if (sliders.size() >= 4)
        {
            sliders[0]->setValue (0.8f); // Fundamental
            sliders[1]->setValue (0.4f); // 2nd harmonic
            sliders[2]->setValue (0.2f); // 3rd harmonic
            sliders[3]->setValue (0.1f); // 4th harmonic
        }
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

        // Title area at the top
        auto titleHeight = proportionOfHeight (0.05f);
        auto titleBounds = bounds.removeFromTop (titleHeight);
        titleLabel->setBounds (titleBounds);

        // Subtitle area
        auto subtitleHeight = proportionOfHeight (0.03f);
        auto subtitleBounds = bounds.removeFromTop (subtitleHeight);
        subtitleLabel->setBounds (subtitleBounds);

        // Reserve space for MIDI keyboard at the bottom
        auto keyboardHeight = proportionOfHeight (0.20f);
        auto keyboardBounds = bounds.removeFromBottom (keyboardHeight);
        keyboardComponent.setBounds (keyboardBounds.reduced (proportionOfWidth (0.02f), proportionOfHeight (0.01f)));

        // Reserve space for oscilloscope above the keyboard
        auto oscilloscopeHeight = proportionOfHeight (0.2f);
        auto oscilloscopeBounds = bounds.removeFromBottom (oscilloscopeHeight);
        oscilloscope.setBounds (oscilloscopeBounds.reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        // Reserve space for buttons area
        auto buttonHeight = proportionOfHeight (0.08f);
        auto buttonArea = bounds.removeFromBottom (buttonHeight);

        auto buttonWidth = buttonArea.getWidth() / 3;
        if (randomizeButton != nullptr)
            randomizeButton->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        if (clearButton != nullptr)
            clearButton->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        if (volumeSlider != nullptr)
            volumeSlider->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        // Use remaining space for harmonic control sliders with labels
        auto sliderBounds = bounds.reduced (proportionOfWidth (0.05f), proportionOfHeight (0.02f));
        auto width = sliderBounds.getWidth() / totalColumns;
        auto height = sliderBounds.getHeight() / totalRows;

        for (int i = 0; i < totalRows && i * totalColumns < sliders.size(); ++i)
        {
            auto row = sliderBounds.removeFromTop (height);
            for (int j = 0; j < totalColumns && i * totalColumns + j < sliders.size(); ++j)
            {
                auto col = row.removeFromLeft (width);
                auto harmonicIndex = i * totalColumns + j;

                // Reserve space for label at bottom of column
                auto labelHeight = 10;
                auto labelBounds = col.removeFromBottom (labelHeight);
                harmonicLabels[harmonicIndex]->setBounds (labelBounds);

                // Use remaining space for slider - make it rectangular for slider appearance
                auto sliderArea = col.largestFittingSquare();
                sliders.getUnchecked (harmonicIndex)->setBounds (sliderArea);
            }
        }

        // Position note indicator at bottom left
        auto noteIndicatorBounds = yup::Rectangle<int> (10, getHeight() - 40, 200, 30);
        noteIndicatorLabel->setBounds (noteIndicatorBounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
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

        // Update note indicator
        if (harmonicSynth.isPlaying())
        {
            if (! noteIndicatorLabel->isVisible())
            {
                noteIndicatorLabel->setVisible (true);
            }
            auto noteText = yup::String ("Playing Note: ") + yup::String (harmonicSynth.getCurrentNote());
            noteIndicatorLabel->setText (noteText, yup::dontSendNotification);
        }
        else
        {
            noteIndicatorLabel->setVisible (false);
        }
    }

    // MIDI keyboard event handlers
    void handleNoteOn (yup::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override
    {
        harmonicSynth.noteOn (midiNoteNumber, velocity);
    }

    void handleNoteOff (yup::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override
    {
        harmonicSynth.noteOff (midiNoteNumber);
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
            // Generate the next sample from the harmonic synth
            float synthSample = harmonicSynth.getNextSample();

            // Apply master volume
            synthSample *= masterVolume;

            // Apply soft limiting to prevent clipping
            synthSample = std::tanh (synthSample);

            // Output to all channels
            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = synthSample;

            // Store for oscilloscope display
            auto pos = readPos.fetch_add (1);
            inputData[pos] = synthSample;
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
    HarmonicSynth harmonicSynth;

    // MIDI keyboard components
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;

    std::vector<float> renderData;
    std::vector<float> inputData;
    yup::CriticalSection renderMutex;
    std::atomic_int readPos = 0;

    // UI Components
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> subtitleLabel;
    std::unique_ptr<yup::Label> noteIndicatorLabel;

    yup::OwnedArray<yup::Slider> sliders;
    yup::OwnedArray<yup::Label> harmonicLabels;
    int totalRows = 4;
    int totalColumns = 4;

    std::unique_ptr<yup::TextButton> randomizeButton;
    std::unique_ptr<yup::TextButton> clearButton;
    std::unique_ptr<yup::Slider> volumeSlider;
    Oscilloscope oscilloscope;

    float masterVolume = 0.5f;
};
