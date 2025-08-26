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

#include <yup_dsp/yup_dsp.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_gui/yup_gui.h>

#include <memory>
#include <vector>
#include <iostream>
#include <atomic>

//==============================================================================

class ConvolutionDemo
    : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::Timer
{
public:
    ConvolutionDemo()
        : wetGainSlider (yup::Slider::LinearHorizontal)
        , dryGainSlider (yup::Slider::LinearHorizontal)
        , loadIRButton ("Load IR...")
    {
        formatManager.registerDefaultFormats();

        // Load default audio files
        loadAudioFile();
        loadDefaultImpulseResponse();

        // Audio device manager
        audioDeviceManager.initialiseWithDefaultDevices (0, 2);

        // Initialize smoothed values
        wetGain.reset (44100, 0.02);
        dryGain.reset (44100, 0.02);
        wetGain.setCurrentAndTargetValue (1.0f);
        dryGain.setCurrentAndTargetValue (0.3f);

        // Configure convolver with typical layout
        convolver.setTypicalLayout (256, {256, 1024, 4096});

        // Create UI
        createUI();

        // Start timer for waveform updates
        startTimerHz (30);
    }

    ~ConvolutionDemo() override
    {
        audioDeviceManager.removeAudioCallback (this);
        audioDeviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);

        // Top controls
        auto topControls = bounds.removeFromTop (120);

        // IR loading section
        auto irSection = topControls.removeFromTop (60);
        loadIRButton.setBounds (irSection.removeFromTop (30).reduced (5, 0));
        irInfoLabel.setBounds (irSection.removeFromTop (25));

        // Control sliders section
        auto controlsSection = topControls;
        auto wetSection = controlsSection.removeFromLeft (controlsSection.getWidth() / 2);
        wetGainLabel.setBounds (wetSection.removeFromTop (25));
        wetGainSlider.setBounds (wetSection.removeFromTop (30).reduced (5, 0));

        dryGainLabel.setBounds (controlsSection.removeFromTop (25));
        dryGainSlider.setBounds (controlsSection.removeFromTop (30).reduced (5, 0));

        // IR waveform display takes remaining space
        irWaveformDisplay.setBounds (bounds);
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            audioDeviceManager.removeAudioCallback (this);
        else
            audioDeviceManager.addAudioCallback (this);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        auto sampleRate = device->getCurrentSampleRate();

        // Update smoothed values
        wetGain.reset (sampleRate, 0.02);
        dryGain.reset (sampleRate, 0.02);

        // Reset convolver
        convolver.reset();
    }

    void audioDeviceStopped() override
    {
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        // Clear outputs
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            if (outputChannelData[ch] != nullptr)
                yup::FloatVectorOperations::clear (outputChannelData[ch], numSamples);
        }

        if (numOutputChannels < 2 || audioBuffer.getNumSamples() == 0)
            return;

        // Prepare buffers for processing
        tempDryBuffer.resize (static_cast<size_t> (numSamples));
        tempWetBuffer.resize (static_cast<size_t> (numSamples));

        // Process samples
        const int totalSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Get the audio sample from the loaded file (mono to stereo if needed)
            float audioSample = 0.0f;

            if (numChannels == 1)
            {
                // Mono file
                audioSample = audioBuffer.getSample (0, readPosition) * 0.5f;
            }
            else
            {
                // Stereo or multichannel - mix to mono
                for (int ch = 0; ch < yup::jmin (2, numChannels); ++ch)
                    audioSample += audioBuffer.getSample (ch, readPosition) * 0.5f;
                audioSample /= yup::jmin (2, numChannels);
            }

            // Increment read position and wrap around for looping
            readPosition++;
            if (readPosition >= totalSamples)
                readPosition = 0;

            // Store dry signal
            tempDryBuffer[static_cast<size_t> (i)] = audioSample;
        }

        // Process through convolver if IR is loaded
        std::fill (tempWetBuffer.begin(), tempWetBuffer.end(), 0.0f);
        if (hasImpulseResponse)
            convolver.process (tempDryBuffer.data(), tempWetBuffer.data(), static_cast<size_t> (numSamples));

        // Mix dry and wet signals with gains
        for (int i = 0; i < numSamples; ++i)
        {
            float wetGainValue = wetGain.getNextValue();
            float dryGainValue = dryGain.getNextValue();

            float drySignal = tempDryBuffer[static_cast<size_t> (i)] * dryGainValue;
            float wetSignal = tempWetBuffer[static_cast<size_t> (i)] * wetGainValue;
            float mixedSignal = drySignal + wetSignal;

            // Output to both channels (mono to stereo)
            outputChannelData[0][i] = mixedSignal;
            outputChannelData[1][i] = mixedSignal;
        }
    }

    void timerCallback() override
    {
        // Update waveform display if needed
        repaint();
    }

private:
    void loadAudioFile()
    {
        // Create the path to the audio file
        auto dataDir = yup::File (__FILE__)
                           .getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getChildFile ("data");

        yup::File audioFile = dataDir.getChildFile ("break_boomblastic_92bpm.wav");
        if (! audioFile.existsAsFile())
        {
            std::cerr << "Could not find break_boomblastic_92bpm.wav" << std::endl;
            return;
        }

        // Load the audio file
        yup::AudioFormatManager formatManager;
        formatManager.registerDefaultFormats();

        if (auto reader = formatManager.createReaderFor (audioFile))
        {
            audioBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&audioBuffer, 0, (int) reader->lengthInSamples, 0, true, true);

            std::cout << "Loaded audio file: " << audioFile.getFileName() << std::endl;
            std::cout << "Sample rate: " << reader->sampleRate << " Hz" << std::endl;
            std::cout << "Channels: " << reader->numChannels << std::endl;
            std::cout << "Length: " << reader->lengthInSamples << " samples" << std::endl;
        }
        else
        {
            std::cerr << "Failed to create reader for audio file" << std::endl;
        }
    }

    void loadDefaultImpulseResponse()
    {
        // Create the path to the default impulse response file
        auto dataDir = yup::File (__FILE__)
                           .getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getChildFile ("data");

        yup::File irFile = dataDir.getChildFile ("ir_e112_g12_dyn_us_6v6.wav");
        loadImpulseResponseFromFile (irFile);
    }

    void loadImpulseResponseFromFile (const yup::File& file)
    {
        if (! file.existsAsFile())
        {
            std::cerr << "Could not find impulse response file: " << file.getFullPathName() << std::endl;
            updateIRInfo ("No IR loaded");
            return;
        }

        // Load the impulse response file
        if (auto reader = formatManager.createReaderFor (file))
        {
            impulseResponseBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&impulseResponseBuffer, 0, (int) reader->lengthInSamples, 0, true, true);

            // Convert to mono if stereo
            if (impulseResponseBuffer.getNumChannels() > 1)
            {
                for (int i = 0; i < impulseResponseBuffer.getNumSamples(); ++i)
                {
                    float monoSample = 0.0f;
                    for (int ch = 0; ch < impulseResponseBuffer.getNumChannels(); ++ch)
                        monoSample += impulseResponseBuffer.getSample (ch, i);
                    monoSample /= static_cast<float> (impulseResponseBuffer.getNumChannels());
                    impulseResponseBuffer.setSample (0, i, monoSample);
                }
                impulseResponseBuffer.setSize (1, impulseResponseBuffer.getNumSamples(), true);
            }

            // Extract samples for convolver and normalize
            const int numSamples = impulseResponseBuffer.getNumSamples();
            impulseResponseData.resize (static_cast<size_t> (numSamples));

            // Normalize IR to prevent clipping (very aggressive scaling for testing)
            float normalizationGain = 1.0f;
            for (int i = 0; i < numSamples; ++i)
                impulseResponseData[static_cast<size_t> (i)] = impulseResponseBuffer.getSample (0, i) * normalizationGain;

            // Set impulse response in convolver
            convolver.setImpulseResponse (impulseResponseData);
            hasImpulseResponse = true;

            std::cout << "Loaded impulse response: " << file.getFileName() << std::endl;
            std::cout << "Sample rate: " << reader->sampleRate << " Hz" << std::endl;
            std::cout << "Length: " << reader->lengthInSamples << " samples" << std::endl;

            // Update UI
            updateIRInfo (file.getFileName());
            updateWaveformDisplay();
        }
        else
        {
            std::cerr << "Failed to create reader for impulse response file" << std::endl;
            updateIRInfo ("Failed to load IR");
        }
    }

    void createUI()
    {
        setOpaque (false);

        // Get fonts
        auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);
        auto buttonFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f);

        // Load IR button
        // loadIRButton.setFont (buttonFont);
        loadIRButton.onClick = [this]
        {
            auto chooser = yup::FileChooser::create ("Load Impulse Response",
                                                     yup::File(),
                                                     "*.wav;*.aiff;*.aif");
            chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
            {
                if (success && results.size() > 0)
                {
                    loadImpulseResponseFromFile (results[0]);
                }
            });
        };
        addAndMakeVisible (loadIRButton);

        // IR info label
        irInfoLabel.setText ("Loading default IR...", yup::NotificationType::dontSendNotification);
        irInfoLabel.setFont (labelFont);
        irInfoLabel.setJustification (yup::Justification::center);
        addAndMakeVisible (irInfoLabel);

        // Wet gain slider
        wetGainLabel.setText ("Wet Gain", yup::NotificationType::dontSendNotification);
        wetGainLabel.setFont (labelFont);
        addAndMakeVisible (wetGainLabel);

        wetGainSlider.setRange (0.0, 2.0);
        wetGainSlider.setValue (1.0);
        wetGainSlider.onValueChanged = [this] (float value)
        {
            wetGain.setTargetValue (value);
        };
        addAndMakeVisible (wetGainSlider);

        // Dry gain slider
        dryGainLabel.setText ("Dry Gain", yup::NotificationType::dontSendNotification);
        dryGainLabel.setFont (labelFont);
        addAndMakeVisible (dryGainLabel);

        dryGainSlider.setRange (0.0, 2.0);
        dryGainSlider.setValue (0.3);
        dryGainSlider.onValueChanged = [this] (float value)
        {
            dryGain.setTargetValue (value);
        };
        addAndMakeVisible (dryGainSlider);

        // Configure IR waveform display
        setupWaveformDisplay();
        addAndMakeVisible (irWaveformDisplay);
    }

    void setupWaveformDisplay()
    {
        // Configure the CartesianPlane for waveform display
        irWaveformDisplay.setTitle ("Impulse Response Waveform");

        // Set linear axes
        irWaveformDisplay.setXRange (0.0, 1.0);
        irWaveformDisplay.setXScaleType (yup::CartesianPlane::AxisScaleType::linear);
        irWaveformDisplay.setYRange (-1.0, 1.0);
        irWaveformDisplay.setYScaleType (yup::CartesianPlane::AxisScaleType::linear);

        // Set margins
        irWaveformDisplay.setMargins (25, 25, 25, 25);

        // Add grid lines
        irWaveformDisplay.setVerticalGridLines ({ 0.0, 1.0 });
        irWaveformDisplay.setHorizontalGridLines ({ -1.0, -0.5, 0.5, 1.0 });
        irWaveformDisplay.addHorizontalGridLine (0.0, yup::Color (0xFF666666), 1.0f, true);

        irWaveformDisplay.clearXAxisLabels();
        irWaveformDisplay.setYAxisLabels ({ -1.0, -0.5, 0.5, 1.0 });

        // Add waveform signal
        waveformSignalIndex = irWaveformDisplay.addSignal ("IR", yup::Color (0xFF44AA44), 1.5f);

        // Configure legend
        irWaveformDisplay.setLegendVisible (false);
    }

    void updateWaveformDisplay()
    {
        if (impulseResponseData.empty())
            return;

        // Create waveform data points
        const size_t numPoints = std::min (static_cast<size_t> (2048), impulseResponseData.size());
        const size_t stride = impulseResponseData.size() / numPoints;

        std::vector<yup::Point<double>> waveformData;
        waveformData.reserve (numPoints);

        for (size_t i = 0; i < numPoints; ++i)
        {
            size_t sampleIndex = i * stride;
            if (sampleIndex >= impulseResponseData.size())
                sampleIndex = impulseResponseData.size() - 1;

            double normalizedTime = static_cast<double> (i) / static_cast<double> (numPoints - 1);
            double amplitude = static_cast<double> (impulseResponseData[sampleIndex]);

            waveformData.emplace_back (normalizedTime, amplitude);
        }

        // Update the display
        irWaveformDisplay.updateSignalData (waveformSignalIndex, waveformData);

        // Update X axis range to show time
        double lengthInSeconds = static_cast<double> (impulseResponseData.size()) / 44100.0; // Assume 44.1kHz
        irWaveformDisplay.setXRange (0.0, lengthInSeconds);

        // Update X axis labels to show time
        std::vector<double> timeLabels;
        for (int i = 0; i <= 4; ++i)
            timeLabels.push_back (lengthInSeconds * static_cast<double> (i) / 4.0);
        irWaveformDisplay.setXAxisLabels (timeLabels);
    }

    void updateIRInfo (const yup::String& info)
    {
        irInfoLabel.setText (info, yup::NotificationType::dontSendNotification);
    }

    // Audio
    yup::AudioFormatManager formatManager;
    yup::AudioDeviceManager audioDeviceManager;
    yup::AudioBuffer<float> audioBuffer;
    yup::AudioBuffer<float> impulseResponseBuffer;
    std::vector<float> impulseResponseData;
    int readPosition = 0;
    std::atomic<bool> hasImpulseResponse = false;

    // Processing
    yup::PartitionedConvolver convolver;
    std::vector<float> tempDryBuffer;
    std::vector<float> tempWetBuffer;

    // Smoothed parameters
    yup::SmoothedValue<float> wetGain, dryGain;

    // UI
    yup::TextButton loadIRButton;
    yup::Label irInfoLabel;
    yup::Label wetGainLabel;
    yup::Slider wetGainSlider;
    yup::Label dryGainLabel;
    yup::Slider dryGainSlider;
    yup::CartesianPlane irWaveformDisplay;

    // Display
    int waveformSignalIndex = -1;
};
