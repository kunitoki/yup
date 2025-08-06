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

#include <memory>
#include <random>
#include <chrono>
#include <complex>

//==============================================================================

class CrossoverFrequencyResponseDisplay : public yup::Component
{
public:
    void updateResponse (const std::vector<yup::Point<double>>& lowData,
                        const std::vector<yup::Point<double>>& highData)
    {
        lowPassData = lowData;
        highPassData = highData;
        repaint();
    }

    void setCrossoverFrequency (double freq)
    {
        crossoverFreq = freq;
        repaint();
    }

private:
    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setFillColor (yup::Color (0xFF1E1E1E));
        g.fillRect (bounds);

        // Reserve space for labels
        auto titleBounds = bounds.removeFromTop (25);
        titleBounds.removeFromLeft (5);
        auto bottomLabelSpace = bounds.removeFromBottom (20);
        auto leftLabelSpace = bounds.removeFromLeft (50);
        leftLabelSpace.removeFromRight (5);

        // Grid
        g.setStrokeColor (yup::Color (0xFF333333));
        g.setStrokeWidth (1.0f);

        // Frequency grid lines (logarithmic)
        for (double freq : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            float x = frequencyToX (freq, bounds);
            g.strokeLine ({ x, bounds.getY() }, { x, bounds.getBottom() });
        }

        // dB grid lines
        for (double db : { -48.0, -36.0, -24.0, -12.0, -6.0, 0.0, 6.0 })
        {
            float y = dbToY (db, bounds);
            g.strokeLine ({ bounds.getX(), y }, { bounds.getRight(), y });
        }

        // Zero line
        g.setStrokeColor (yup::Color (0xFF666666));
        g.setStrokeWidth (2.0f);
        float y0 = dbToY (0.0, bounds);
        g.strokeLine ({ bounds.getX(), y0 }, { bounds.getRight(), y0 });

        // -6dB crossover line
        g.setStrokeColor (yup::Color (0xFF444444));
        g.setStrokeWidth (1.0f);
        float y6 = dbToY (-6.0, bounds);
        g.strokeLine ({ bounds.getX(), y6 }, { bounds.getRight(), y6 });

        // Crossover frequency line
        if (crossoverFreq > 0)
        {
            g.setStrokeColor (yup::Color (0xFF888888));
            g.setStrokeWidth (1.0f);
            float xCross = frequencyToX (crossoverFreq, bounds);
            g.strokeLine ({ xCross, bounds.getY() }, { xCross, bounds.getBottom() });
        }

        // Plot frequency responses
        if (! lowPassData.empty())
        {
            // Low pass in blue
            yup::Path lowPath;
            bool firstPoint = true;

            g.setStrokeColor (yup::Color (0xFF4488FF));
            g.setStrokeWidth (2.0f);

            for (const auto& point : lowPassData)
            {
                float x = frequencyToX (point.getX(), bounds);
                float y = dbToY (point.getY(), bounds);

                if (firstPoint)
                {
                    lowPath.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else
                {
                    lowPath.lineTo (x, y);
                }
            }

            g.strokePath (lowPath);
        }

        if (! highPassData.empty())
        {
            // High pass in orange
            yup::Path highPath;
            bool firstPoint = true;

            g.setStrokeColor (yup::Color (0xFFFF8844));
            g.setStrokeWidth (2.0f);

            for (const auto& point : highPassData)
            {
                float x = frequencyToX (point.getX(), bounds);
                float y = dbToY (point.getY(), bounds);

                if (firstPoint)
                {
                    highPath.startNewSubPath (x, y);
                    firstPoint = false;
                }
                else
                {
                    highPath.lineTo (x, y);
                }
            }

            g.strokePath (highPath);
        }

        // Labels with smaller font
        g.setFillColor (yup::Colors::white);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);

        // Title (centered, leaving space for legend)
        auto titleArea = titleBounds.removeFromLeft (titleBounds.getWidth() - 120);
        g.fillFittedText ("Crossover Frequency Response", font.withHeight (12.0f), titleArea, yup::Justification::centerLeft);

        // Frequency labels
        for (double freq : { 100.0, 1000.0, 10000.0 })
        {
            float x = frequencyToX (freq, bounds);
            yup::String label;
            if (freq >= 1000.0)
                label = yup::String (freq / 1000.0, 0) + "k";
            else
                label = yup::String (static_cast<int> (freq));

            auto labelBounds = yup::Rectangle<int> (static_cast<int> (x - 20),
                                                    bottomLabelSpace.getY(),
                                                    40,
                                                    bottomLabelSpace.getHeight());
            g.fillFittedText (label, font, labelBounds, yup::Justification::center);
        }

        // dB labels
        g.setFillColor (yup::Colors::gray);
        for (double db : { -24.0, -12.0, -6.0, 0.0 })
        {
            float y = dbToY (db, bounds);
            auto label = yup::String (static_cast<int> (db)) + " dB";
            g.fillFittedText (label, font, leftLabelSpace.withY (static_cast<int> (y - 8)).withHeight (16), yup::Justification::right);
        }

        // Legend (on the right side of title area)
        auto legendBounds = titleBounds;
        legendBounds = legendBounds.withY (legendBounds.getY() + 4);

        g.setFillColor (yup::Color (0xFF4488FF));
        auto lowRect = legendBounds.removeFromLeft (15).reduced (2);
        g.fillRect (lowRect);
        g.setFillColor (yup::Colors::white);
        g.fillFittedText ("Low", font, legendBounds.removeFromLeft (25), yup::Justification::left);

        g.setFillColor (yup::Color (0xFFFF8844));
        auto highRect = legendBounds.removeFromLeft (15).reduced (2);
        g.fillRect (highRect);
        g.setFillColor (yup::Colors::white);
        g.fillFittedText ("High", font, legendBounds, yup::Justification::left);
    }

    float frequencyToX (double freq, const yup::Rectangle<float>& bounds) const
    {
        if (freq <= 0)
            return bounds.getX();

        const double minLog = std::log10 (20.0);
        const double maxLog = std::log10 (20000.0);
        const double logFreq = std::log10 (freq);
        const double normalised = (logFreq - minLog) / (maxLog - minLog);

        return bounds.getX() + static_cast<float> (normalised * bounds.getWidth());
    }

    float dbToY (double db, const yup::Rectangle<float>& bounds) const
    {
        const double minDb = -48.0;
        const double maxDb = 12.0;
        const double normalised = 1.0 - (db - minDb) / (maxDb - minDb);

        return bounds.getY() + static_cast<float> (normalised * bounds.getHeight());
    }

    std::vector<yup::Point<double>> lowPassData;
    std::vector<yup::Point<double>> highPassData;
    double crossoverFreq = 1000.0;
};

//==============================================================================

class CrossoverDemo : public yup::Component,
                     public yup::AudioIODeviceCallback,
                     public yup::Timer
{
public:
    CrossoverDemo()
        : freqSlider (yup::Slider::LinearHorizontal)
        , lowGainSlider (yup::Slider::LinearVertical)
        , highGainSlider (yup::Slider::LinearVertical)
    {
        // Load the audio file
        loadAudioFile();

        // Audio device manager
        audioDeviceManager.initialiseWithDefaultDevices (0, 2);

        // Initialize smoothed values
        lowGain.reset (44100, 0.02);
        highGain.reset (44100, 0.02);
        crossoverFreq.reset (44100, 0.05);
        lowGain.setCurrentAndTargetValue (1.0f);
        highGain.setCurrentAndTargetValue (1.0f);
        crossoverFreq.setCurrentAndTargetValue (1000.0f);

        // Create UI
        createUI();

        // Start timer for frequency response updates
        startTimerHz (30);
    }

    ~CrossoverDemo() override
    {
        audioDeviceManager.removeAudioCallback (this);
        audioDeviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);

        // Top controls
        auto topControls = bounds.removeFromTop (80);

        // Order selection
        auto orderSection = topControls.removeFromLeft (150);
        orderLabel.setBounds (orderSection.removeFromTop (25));
        orderComboBox.setBounds (orderSection.removeFromTop (30).reduced (5, 0));

        // Crossover frequency
        auto freqSection = topControls;
        freqLabel.setBounds (freqSection.removeFromTop (25));
        freqSlider.setBounds (freqSection.removeFromTop (40));

        // Right side volume controls
        auto rightControls = bounds.removeFromRight (120);

        // Low gain control
        auto lowSection = rightControls.removeFromLeft (55);
        lowGainLabel.setBounds (lowSection.removeFromBottom (25));
        lowGainSlider.setBounds (lowSection.reduced (5, 5));

        // High gain control
        auto highSection = rightControls;
        highGainLabel.setBounds (highSection.removeFromBottom (25));
        highGainSlider.setBounds (highSection.reduced (5, 5));

        // Frequency response display takes remaining space
        frequencyDisplay.setBounds (bounds);
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

        // Update filter sample rates
        filter2.setSampleRate (sampleRate);
        filter4.setSampleRate (sampleRate);
        filter8.setSampleRate (sampleRate);

        // Update smoothed values
        lowGain.reset (sampleRate, 0.02);
        highGain.reset (sampleRate, 0.02);
        crossoverFreq.reset (sampleRate, 0.05);
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

        // Get the active filter
        yup::LinkwitzRileyFilter<float>* activeFilter = nullptr;

        if (currentOrder == 2)
        {
            filterProcess = [this](float inL, float inR, float& lowL, float& lowR, float& highL, float& highR)
            {
                filter2.processSample (inL, inR, lowL, lowR, highL, highR);
            };
        }
        else if (currentOrder == 4)
        {
            filterProcess = [this](float inL, float inR, float& lowL, float& lowR, float& highL, float& highR)
            {
                filter4.processSample (inL, inR, lowL, lowR, highL, highR);
            };
        }
        else
        {
            filterProcess = [this](float inL, float inR, float& lowL, float& lowR, float& highL, float& highR)
            {
                filter8.processSample (inL, inR, lowL, lowR, highL, highR);
            };
        }

        // Process samples
        const int totalSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Update crossover frequency smoothly
            if (crossoverFreq.isSmoothing())
            {
                float freq = crossoverFreq.getNextValue();
                filter2.setFrequency (freq);
                filter4.setFrequency (freq);
                filter8.setFrequency (freq);
            }

            // Get the audio sample from the loaded file (mono to stereo if needed)
            float audioSample = 0.0f;

            if (numChannels == 1)
            {
                // Mono file
                audioSample = audioBuffer.getSample (0, readPosition) * 0.3f;
            }
            else
            {
                // Stereo or multichannel - mix to mono
                for (int ch = 0; ch < yup::jmin(2, numChannels); ++ch)
                    audioSample += audioBuffer.getSample (ch, readPosition) * 0.3f;
                audioSample /= yup::jmin(2, numChannels);
            }

            // Increment read position and wrap around for looping
            readPosition++;
            if (readPosition >= totalSamples)
                readPosition = 0;

            // Process through crossover
            float lowLeft, lowRight, highLeft, highRight;
            filterProcess (audioSample, audioSample, lowLeft, lowRight, highLeft, highRight);

            // Apply gains
            float lowGainValue = lowGain.getNextValue();
            float highGainValue = highGain.getNextValue();

            // Mix to output (mono to stereo)
            outputChannelData[0][i] = lowLeft * lowGainValue + highLeft * highGainValue;
            outputChannelData[1][i] = lowRight * lowGainValue + highRight * highGainValue;
        }
    }

    void timerCallback() override
    {
        updateFrequencyResponse();
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

    void createUI()
    {
        setOpaque (false);

        // Get a 12pt font
        auto labelFont = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Order selection
        orderLabel.setText ("Filter Order", yup::NotificationType::dontSendNotification);
        orderLabel.setFont (labelFont);
        addAndMakeVisible (orderLabel);

        orderComboBox.addItem ("2nd Order", 1);
        orderComboBox.addItem ("4th Order", 2);
        orderComboBox.addItem ("8th Order", 3);
        orderComboBox.setSelectedId (2); // Default to 4th order
        orderComboBox.onSelectedItemChanged = [this]
        {
            switch (orderComboBox.getSelectedId())
            {
                case 1: currentOrder = 2; break;
                case 2: currentOrder = 4; break;
                case 3: currentOrder = 8; break;
            }
            updateFrequencyResponse();
        };
        addAndMakeVisible (orderComboBox);

        // Crossover frequency slider
        freqLabel.setText ("Crossover Frequency", yup::NotificationType::dontSendNotification);
        freqLabel.setFont (labelFont);
        addAndMakeVisible (freqLabel);

        freqSlider.setRange (20.0, 20000.0);
        freqSlider.setSkewFactorFromMidpoint (1000.0);
        freqSlider.setValue (1000.0);
        //freqSlider.setTextValueSuffix (" Hz");
        freqSlider.onValueChanged = [this](float value)
        {
            crossoverFreq.setTargetValue (value);
            frequencyDisplay.setCrossoverFrequency (value);
        };
        addAndMakeVisible (freqSlider);

        // Low gain slider
        lowGainLabel.setText ("Low", yup::NotificationType::dontSendNotification);
        lowGainLabel.setFont (labelFont);
        lowGainLabel.setJustification (yup::Justification::center);
        //lowGainLabel.setColour (yup::Label::textColourId, yup::Color (0xFF4488FF));
        addAndMakeVisible (lowGainLabel);

        lowGainSlider.setRange (0.0, 2.0);
        lowGainSlider.setValue (1.0);
        //lowGainSlider.setTextValueSuffix (" x");
        lowGainSlider.onValueChanged = [this](float value)
        {
            lowGain.setTargetValue (value);
        };
        addAndMakeVisible (lowGainSlider);

        // High gain slider
        highGainLabel.setText ("High", yup::NotificationType::dontSendNotification);
        highGainLabel.setFont (labelFont);
        highGainLabel.setJustification (yup::Justification::center);
        //highGainLabel.setColour (yup::Label::textColourId, yup::Color (0xFFFF8844));
        addAndMakeVisible (highGainLabel);

        highGainSlider.setRange (0.0, 2.0);
        highGainSlider.setValue (1.0);
        //highGainSlider.setTextValueSuffix (" x");
        highGainSlider.onValueChanged = [this](float value)
        {
            highGain.setTargetValue (value);
        };
        addAndMakeVisible (highGainSlider);

        // Frequency display
        addAndMakeVisible (frequencyDisplay);

        // Initialize frequency response
        updateFrequencyResponse();
    }

    void updateFrequencyResponse()
    {
        const int numPoints = 512;
        const double minFreq = 20.0;
        const double maxFreq = 20000.0;
        const double logMin = std::log10 (minFreq);
        const double logMax = std::log10 (maxFreq);

        std::vector<yup::Point<double>> lowResponse, highResponse;
        lowResponse.reserve (numPoints);
        highResponse.reserve (numPoints);

        auto sampleRate = audioDeviceManager.getCurrentAudioDevice() ?
                          audioDeviceManager.getCurrentAudioDevice()->getCurrentSampleRate() : 44100.0;

        for (int i = 0; i < numPoints; ++i)
        {
            double normalised = static_cast<double> (i) / (numPoints - 1);
            double logFreq = logMin + normalised * (logMax - logMin);
            double freq = std::pow (10.0, logFreq);

            // Calculate response based on order
            double lowMag = 0.0, highMag = 0.0;

            switch (currentOrder)
            {
                case 2:
                {
                    lowMag = filter2.getMagnitudeResponseLowBand (freq);
                    highMag = filter2.getMagnitudeResponseHighBand (freq);
                    break;
                }

                case 4:
                {
                    lowMag = filter4.getMagnitudeResponseLowBand (freq);
                    highMag = filter4.getMagnitudeResponseHighBand (freq);
                    break;
                }

                case 8:
                {
                    lowMag = filter8.getMagnitudeResponseLowBand (freq);
                    highMag = filter8.getMagnitudeResponseHighBand (freq);
                    break;
                }
            }

            // Convert to dB
            double lowDb = 20.0 * std::log10 (std::max (lowMag, 1e-10));
            double highDb = 20.0 * std::log10 (std::max (highMag, 1e-10));

            lowResponse.emplace_back (freq, lowDb);
            highResponse.emplace_back (freq, highDb);
        }

        frequencyDisplay.updateResponse (lowResponse, highResponse);
    }

    // Audio
    yup::AudioDeviceManager audioDeviceManager;
    yup::AudioBuffer<float> audioBuffer;
    int readPosition = 0;

    // Filters
    yup::LinkwitzRiley2Filter<float> filter2;
    yup::LinkwitzRiley4Filter<float> filter4;
    yup::LinkwitzRiley8Filter<float> filter8;
    int currentOrder = 4;

    // Process
    yup::FixedSizeFunction<16, void (float, float, float&, float&, float&, float&)> filterProcess;

    // Gains
    yup::SmoothedValue<float> lowGain, highGain, crossoverFreq;

    // UI
    yup::Label orderLabel;
    yup::ComboBox orderComboBox;
    yup::Label freqLabel;
    yup::Slider freqSlider;
    yup::Label lowGainLabel;
    yup::Slider lowGainSlider;
    yup::Label highGainLabel;
    yup::Slider highGainSlider;
    CrossoverFrequencyResponseDisplay frequencyDisplay;
};
