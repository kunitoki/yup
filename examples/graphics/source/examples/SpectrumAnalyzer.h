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
#include <yup_audio_gui/yup_audio_gui.h>

//==============================================================================

class SignalGenerator
{
public:
    enum class SignalType
    {
        singleTone,
        frequencySweep,
        whiteNoise,
        pinkNoise,
        brownNoise
    };

    SignalGenerator()
        : sampleRate (44100.0)
        , frequency (440.0)
        , phase (0.0)
        , amplitude (0.5f)
        , signalType (SignalType::singleTone)
        , sweepStartFreq (20.0)
        , sweepEndFreq (22000.0)
        , sweepDurationSeconds (10.0)
        , sweepProgress (0.0)
        , pinkState (0.0)
        , brownState (0.0)
    {
        // Initialize pink noise filter state
        for (int i = 0; i < 7; ++i)
            pinkFilters[i] = 0.0;
    }

    void setSampleRate (double newSampleRate)
    {
        sampleRate = newSampleRate;
        updatePhaseIncrement();
    }

    void setFrequency (double newFrequency)
    {
        frequency = newFrequency;
        updatePhaseIncrement();
    }

    void setAmplitude (float newAmplitude)
    {
        amplitude = newAmplitude;
    }

    void setSignalType (SignalType type)
    {
        signalType = type;
        if (type == SignalType::frequencySweep)
            sweepProgress = 0.0;
    }

    void setSweepParameters (double startFreq, double endFreq, double durationSeconds)
    {
        sweepStartFreq = startFreq;
        sweepEndFreq = endFreq;
        sweepDurationSeconds = durationSeconds;
        sweepProgress = 0.0;
    }

    float getNextSample()
    {
        float sample = 0.0f;

        switch (signalType)
        {
            case SignalType::singleTone:
                sample = generateSine();
                break;
            case SignalType::frequencySweep:
                sample = generateSweep();
                break;
            case SignalType::whiteNoise:
                sample = generateWhiteNoise();
                break;
            case SignalType::pinkNoise:
                sample = generatePinkNoise();
                break;
            case SignalType::brownNoise:
                sample = generateBrownNoise();
                break;
        }

        return sample * amplitude;
    }

private:
    float generateSine()
    {
        float sample = std::sin (phase);
        phase += phaseIncrement;

        if (phase >= yup::MathConstants<double>::twoPi)
            phase -= yup::MathConstants<double>::twoPi;

        return sample;
    }

    float generateSweep()
    {
        // Linear frequency sweep
        double currentFreq = sweepStartFreq + (sweepEndFreq - sweepStartFreq) * sweepProgress;
        double currentPhaseIncrement = yup::MathConstants<double>::twoPi * currentFreq / sampleRate;

        float sample = std::sin (phase);
        phase += currentPhaseIncrement;

        if (phase >= yup::MathConstants<double>::twoPi)
            phase -= yup::MathConstants<double>::twoPi;

        // Update sweep progress
        sweepProgress += 1.0 / (sweepDurationSeconds * sampleRate);
        if (sweepProgress >= 1.0)
            sweepProgress = 0.0; // Loop the sweep

        return sample;
    }

    float generateWhiteNoise()
    {
        return yup::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    float generatePinkNoise()
    {
        // Paul Kellett's refined method for pink noise
        float white = yup::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;

        pinkFilters[0] = 0.99886f * pinkFilters[0] + white * 0.0555179f;
        pinkFilters[1] = 0.99332f * pinkFilters[1] + white * 0.0750759f;
        pinkFilters[2] = 0.96900f * pinkFilters[2] + white * 0.1538520f;
        pinkFilters[3] = 0.86650f * pinkFilters[3] + white * 0.3104856f;
        pinkFilters[4] = 0.55000f * pinkFilters[4] + white * 0.5329522f;
        pinkFilters[5] = -0.7616f * pinkFilters[5] - white * 0.0168980f;

        float pink = pinkFilters[0] + pinkFilters[1] + pinkFilters[2] + pinkFilters[3] + pinkFilters[4] + pinkFilters[5] + pinkFilters[6] + white * 0.5362f;
        pinkFilters[6] = white * 0.115926f;

        return pink * 0.11f; // Scale down
    }

    float generateBrownNoise()
    {
        float white = yup::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        brownState = (brownState + (0.02f * white)) / 1.02f;
        brownState *= 3.5f; // Scale up
        return brownState;
    }

    void updatePhaseIncrement()
    {
        phaseIncrement = yup::MathConstants<double>::twoPi * frequency / sampleRate;
    }

    double sampleRate;
    double frequency;
    double phase;
    double phaseIncrement = 0.0;
    float amplitude;

    SignalType signalType;

    // Sweep parameters
    double sweepStartFreq, sweepEndFreq, sweepDurationSeconds;
    double sweepProgress;

    // Noise filter states
    double pinkFilters[7];
    double pinkState;
    double brownState;
};

//==============================================================================

class SpectrumAnalyzerDemo
    : public yup::Component
    , public yup::AudioIODeviceCallback
    , public yup::Timer
{
public:
    SpectrumAnalyzerDemo()
        : Component ("SpectrumAnalyzerDemo")
        , analyzerComponent (analyzerState)
    {
        setupUI();
        setupAudio();
    }

    ~SpectrumAnalyzerDemo() override
    {
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int margin = 10;

        // Title area with proper spacing
        auto titleBounds = bounds.removeFromTop (40);
        titleLabel->setBounds (titleBounds.reduced (margin, 8));

        // Control panel
        auto controlHeight = 180;
        auto controlPanel = bounds.removeFromTop (controlHeight);
        layoutControlPanel (controlPanel.reduced (margin));

        // Small gap before spectrum analyzer
        bounds.removeFromTop (5);

        // Spectrum analyzer takes the rest with proper margins for labels
        auto analyzerBounds = bounds.reduced (margin);
        analyzerComponent.setBounds (analyzerBounds);
    }

    void visibilityChanged() override
    {
        if (! isVisible())
        {
            deviceManager.removeAudioCallback (this);
            stopTimer();
        }
        else
        {
            deviceManager.addAudioCallback (this);
            startTimer (100); // Update UI every 100ms
        }
    }

    void timerCallback() override
    {
        // Update frequency display
        if (frequencyLabel)
        {
            yup::String freqText = "Frequency: " + yup::String (static_cast<int> (currentFrequency)) + " Hz";
            frequencyLabel->setText (freqText, yup::dontSendNotification);
        }

        // Update amplitude display
        if (amplitudeLabel)
        {
            yup::String ampText = "Amplitude: " + yup::String (currentAmplitude * 100, 0) + "%";
            amplitudeLabel->setText (ampText, yup::dontSendNotification);
        }

        // Update FFT info display
        if (fftInfoLabel)
        {
            yup::String fftText = "FFT: " + yup::String (currentFFTSize);
            fftInfoLabel->setText (fftText, yup::dontSendNotification);
        }
    }

    // AudioIODeviceCallback methods
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext& context) override
    {
        // Generate test audio samples
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate audio sample using signal generator
            const float audioSample = signalGenerator.getNextSample();

            // Output to all channels
            for (int channel = 0; channel < numOutputChannels; ++channel)
                outputChannelData[channel][sample] = audioSample;

            // Feed to spectrum analyzer
            analyzerState.pushSample (audioSample);
        }
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        double sampleRate = device->getCurrentSampleRate();

        // Setup signal generator
        signalGenerator.setSampleRate (sampleRate);
        signalGenerator.setFrequency (currentFrequency);
        signalGenerator.setAmplitude (currentAmplitude);
        signalGenerator.setSweepParameters (20.0, 22000.0, sweepDurationSeconds);

        // Configure spectrum analyzer
        analyzerComponent.setSampleRate (sampleRate);
    }

    void audioDeviceStopped() override
    {
    }

private:
    void setupUI()
    {
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        // Title
        titleLabel = std::make_unique<yup::Label> ("Title");
        titleLabel->setText ("Real-Time Spectrum Analyzer Demo");
        titleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        titleLabel->setFont (font);
        addAndMakeVisible (*titleLabel);

        // Signal type selector
        signalTypeCombo = std::make_unique<yup::ComboBox> ("SignalType");
        signalTypeCombo->addItem ("Single Tone", 1);
        signalTypeCombo->addItem ("Sweep", 2);
        signalTypeCombo->addItem ("White Noise", 3);
        signalTypeCombo->addItem ("Pink Noise", 4);
        signalTypeCombo->addItem ("Brown Noise", 5);
        signalTypeCombo->setSelectedId (1);
        signalTypeCombo->onSelectedItemChanged = [this]
        {
            updateSignalType();
        };
        addAndMakeVisible (*signalTypeCombo);

        // Frequency control
        frequencySlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Frequency");
        frequencySlider->setRange ({ 20.0, 22000.0 });
        frequencySlider->setSkewFactorFromMidpoint (440.0);
        frequencySlider->setValue (440.0);
        frequencySlider->onValueChanged = [this] (float value)
        {
            currentFrequency = value;
            signalGenerator.setFrequency (value);
        };
        addAndMakeVisible (*frequencySlider);

        // Amplitude control
        amplitudeSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Amplitude");
        amplitudeSlider->setRange ({ 0.0, 1.0 });
        amplitudeSlider->setValue (0.5);
        amplitudeSlider->onValueChanged = [this] (float value)
        {
            currentAmplitude = value;
            signalGenerator.setAmplitude (value);
        };
        addAndMakeVisible (*amplitudeSlider);

        // Sweep duration control
        sweepDurationSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Sweep Duration");
        sweepDurationSlider->setRange ({ 1.0, 60.0 });
        sweepDurationSlider->setValue (10.0);
        sweepDurationSlider->onValueChanged = [this] (float value)
        {
            sweepDurationSeconds = value;
            signalGenerator.setSweepParameters (20.0, 22000.0, value);
        };
        addAndMakeVisible (*sweepDurationSlider);

        // FFT size selector
        fftSizeCombo = std::make_unique<yup::ComboBox> ("FFTSize");
        int fftSizeId = 1;
        for (int size = 32; size <= 16384; size *= 2)
            fftSizeCombo->addItem (yup::String (size), fftSizeId++);
        fftSizeCombo->setSelectedId (8);
        fftSizeCombo->onSelectedItemChanged = [this]
        {
            updateFFTSize();
        };
        addAndMakeVisible (*fftSizeCombo);

        // Window type selector
        windowTypeCombo = std::make_unique<yup::ComboBox> ("WindowType");
        windowTypeCombo->addItem ("Rectangular", 1);
        windowTypeCombo->addItem ("Hann", 2);
        windowTypeCombo->addItem ("Hamming", 3);
        windowTypeCombo->addItem ("Blackman", 4);
        windowTypeCombo->addItem ("B-Harris", 5);
        windowTypeCombo->addItem ("Kaiser", 6);
        windowTypeCombo->addItem ("Gaussian", 7);
        windowTypeCombo->addItem ("Tukey", 8);
        windowTypeCombo->addItem ("Bartlett", 9);
        windowTypeCombo->addItem ("Welch", 10);
        windowTypeCombo->addItem ("Flat-top", 11);
        windowTypeCombo->setSelectedId (4);
        windowTypeCombo->onSelectedItemChanged = [this]
        {
            updateWindowType();
        };
        addAndMakeVisible (*windowTypeCombo);

        // Display type selector
        displayTypeCombo = std::make_unique<yup::ComboBox> ("DisplayType");
        displayTypeCombo->addItem ("Filled", 1);
        displayTypeCombo->addItem ("Lines", 2);
        displayTypeCombo->setSelectedId (1);
        displayTypeCombo->onSelectedItemChanged = [this]
        {
            updateDisplayType();
        };
        addAndMakeVisible (*displayTypeCombo);

        // Release control
        releaseSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Release");
        releaseSlider->setRange ({ 0.0, 5.0 });
        releaseSlider->setValue (1.0);
        releaseSlider->onValueChanged = [this] (float value)
        {
            analyzerComponent.setReleaseTimeSeconds (value);
        };
        addAndMakeVisible (*releaseSlider);
        
        // Overlap control for responsiveness
        overlapSlider = std::make_unique<yup::Slider> (yup::Slider::LinearBarHorizontal, "Overlap");
        overlapSlider->setRange ({ 0.0, 0.95 });
        overlapSlider->setValue (0.75);
        overlapSlider->onValueChanged = [this] (float value)
        {
            analyzerComponent.setOverlapFactor (value);
        };
        addAndMakeVisible (*overlapSlider);

        // Status labels with appropriate font size
        auto statusFont = font.withHeight (11.0f);

        frequencyLabel = std::make_unique<yup::Label> ("FrequencyLabel");
        frequencyLabel->setText ("Frequency: 440 Hz");
        frequencyLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
        frequencyLabel->setFont (statusFont);
        addAndMakeVisible (*frequencyLabel);

        amplitudeLabel = std::make_unique<yup::Label> ("AmplitudeLabel");
        amplitudeLabel->setText ("Amplitude: 50%");
        amplitudeLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
        amplitudeLabel->setFont (statusFont);
        addAndMakeVisible (*amplitudeLabel);

        fftInfoLabel = std::make_unique<yup::Label> ("FFTInfoLabel");
        fftInfoLabel->setText ("FFT: 2048");
        fftInfoLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
        fftInfoLabel->setFont (statusFont);
        addAndMakeVisible (*fftInfoLabel);

        // Configure spectrum analyzer
        analyzerComponent.setWindowType (yup::WindowType::hann);
        analyzerComponent.setFrequencyRange (20.0f, 22000.0f);
        analyzerComponent.setDecibelRange (-100.0f, 10.0f);
        analyzerComponent.setUpdateRate (30);
        analyzerComponent.setSampleRate (44100.0);
        analyzerComponent.setOverlapFactor (0.75f); // 75% overlap for better responsiveness
        addAndMakeVisible (analyzerComponent);

        // Create parameter labels with proper font sizing
        auto labelFont = font.withHeight (12.0f);

        for (const auto& labelText : { "Signal Type:", "Frequency:", "Amplitude:", "Sweep Duration:", "FFT Size:", "Window:", "Display:", "Release:", "Overlap:" })
        {
            auto label = parameterLabels.add (std::make_unique<yup::Label> (labelText));
            label->setText (labelText);
            label->setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
            label->setFont (labelFont);
            addAndMakeVisible (*label);
        }
    }

    void setupAudio()
    {
        // Initialize audio device
        deviceManager.initialiseWithDefaultDevices (0, 2);
    }

    void layoutControlPanel (yup::Rectangle<float> bounds)
    {
        const int margin = 8;
        const int labelHeight = 18;
        const int controlHeight = 32;
        const int rowHeight = labelHeight + controlHeight + margin;
        const int colWidth = bounds.getWidth() / 5 - margin;

        // First row: Signal controls
        auto row1 = bounds.removeFromTop (rowHeight);
        auto signalTypeSection = row1.removeFromLeft (colWidth);
        auto freqSection = row1.removeFromLeft (colWidth);
        auto ampSection = row1.removeFromLeft (colWidth);
        auto sweepSection = row1.removeFromLeft (colWidth);
        auto smoothingSection = row1.removeFromLeft (colWidth);

        parameterLabels[0]->setBounds (signalTypeSection.removeFromTop (labelHeight));
        signalTypeCombo->setBounds (signalTypeSection.removeFromTop (controlHeight));

        parameterLabels[1]->setBounds (freqSection.removeFromTop (labelHeight));
        frequencySlider->setBounds (freqSection.removeFromTop (controlHeight));

        parameterLabels[2]->setBounds (ampSection.removeFromTop (labelHeight));
        amplitudeSlider->setBounds (ampSection.removeFromTop (controlHeight));

        parameterLabels[3]->setBounds (sweepSection.removeFromTop (labelHeight));
        sweepDurationSlider->setBounds (sweepSection.removeFromTop (controlHeight));

        parameterLabels[7]->setBounds (smoothingSection.removeFromTop (labelHeight));
        releaseSlider->setBounds (smoothingSection.removeFromTop (controlHeight));

        // Second row: FFT controls
        auto row2 = bounds.removeFromTop (rowHeight);
        auto fftSizeSection = row2.removeFromLeft (colWidth);
        auto windowSection = row2.removeFromLeft (colWidth);
        auto displaySection = row2.removeFromLeft (colWidth);
        auto overlapSection = row2.removeFromLeft (colWidth);

        parameterLabels[4]->setBounds (fftSizeSection.removeFromTop (labelHeight));
        fftSizeCombo->setBounds (fftSizeSection.removeFromTop (controlHeight));

        parameterLabels[5]->setBounds (windowSection.removeFromTop (labelHeight));
        windowTypeCombo->setBounds (windowSection.removeFromTop (controlHeight));

        parameterLabels[6]->setBounds (displaySection.removeFromTop (labelHeight));
        displayTypeCombo->setBounds (displaySection.removeFromTop (controlHeight));
        
        parameterLabels[8]->setBounds (overlapSection.removeFromTop (labelHeight));
        overlapSlider->setBounds (overlapSection.removeFromTop (controlHeight));

        // Third row: Status labels
        auto row3 = bounds.removeFromTop (30);
        auto freqStatus = row3.removeFromLeft (bounds.getWidth() / 3);
        auto ampStatus = row3.removeFromLeft (bounds.getWidth() / 3);
        auto fftStatus = row3.removeFromLeft (bounds.getWidth() / 3);

        frequencyLabel->setBounds (freqStatus);
        amplitudeLabel->setBounds (ampStatus);
        fftInfoLabel->setBounds (fftStatus);
    }

    void updateSignalType()
    {
        SignalGenerator::SignalType signalType = SignalGenerator::SignalType::singleTone;

        switch (signalTypeCombo->getSelectedId())
        {
            case 1:
                signalType = SignalGenerator::SignalType::singleTone;
                break;
            case 2:
                signalType = SignalGenerator::SignalType::frequencySweep;
                break;
            case 3:
                signalType = SignalGenerator::SignalType::whiteNoise;
                break;
            case 4:
                signalType = SignalGenerator::SignalType::pinkNoise;
                break;
            case 5:
                signalType = SignalGenerator::SignalType::brownNoise;
                break;
        }

        signalGenerator.setSignalType (signalType);

        // Enable/disable frequency and sweep controls based on signal type
        bool isToneOrSweep = (signalType == SignalGenerator::SignalType::singleTone || signalType == SignalGenerator::SignalType::frequencySweep);
        frequencySlider->setEnabled (signalType == SignalGenerator::SignalType::singleTone);
        sweepDurationSlider->setEnabled (signalType == SignalGenerator::SignalType::frequencySweep);
    }

    void updateFFTSize()
    {
        int selectedId = fftSizeCombo->getSelectedId();
        currentFFTSize = 32 << (selectedId - 1); // 32, 64, 128, 256, ..., 16384

        // Update the analyzer component (which will update the state)
        analyzerComponent.setFFTSize (currentFFTSize);
    }

    void updateWindowType()
    {
        yup::WindowType windowType = yup::WindowType::hann;

        switch (windowTypeCombo->getSelectedId())
        {
            case 1:
                windowType = yup::WindowType::rectangular;
                break;
            case 2:
                windowType = yup::WindowType::hann;
                break;
            case 3:
                windowType = yup::WindowType::hamming;
                break;
            case 4:
                windowType = yup::WindowType::blackman;
                break;
            case 5:
                windowType = yup::WindowType::blackmanHarris;
                break;
            case 6:
                windowType = yup::WindowType::kaiser;
                break;
            case 7:
                windowType = yup::WindowType::gaussian;
                break;
            case 8:
                windowType = yup::WindowType::tukey;
                break;
            case 9:
                windowType = yup::WindowType::bartlett;
                break;
            case 10:
                windowType = yup::WindowType::welch;
                break;
            case 11:
                windowType = yup::WindowType::flattop;
                break;
        }

        analyzerComponent.setWindowType (windowType);
    }

    void updateDisplayType()
    {
        yup::SpectrumAnalyzerComponent::DisplayType displayType = yup::SpectrumAnalyzerComponent::DisplayType::filled;

        switch (displayTypeCombo->getSelectedId())
        {
            case 1:
                displayType = yup::SpectrumAnalyzerComponent::DisplayType::filled;
                break;
            case 2:
                displayType = yup::SpectrumAnalyzerComponent::DisplayType::lines;
                break;
        }

        analyzerComponent.setDisplayType (displayType);
    }

    // Audio components
    yup::AudioDeviceManager deviceManager;
    SignalGenerator signalGenerator;

    // Spectrum analyzer
    yup::SpectrumAnalyzerState analyzerState;
    yup::SpectrumAnalyzerComponent analyzerComponent;

    // UI components
    std::unique_ptr<yup::Label> titleLabel;

    // Signal controls
    std::unique_ptr<yup::ComboBox> signalTypeCombo;
    std::unique_ptr<yup::Slider> frequencySlider;
    std::unique_ptr<yup::Slider> amplitudeSlider;
    std::unique_ptr<yup::Slider> sweepDurationSlider;

    // FFT controls
    std::unique_ptr<yup::ComboBox> fftSizeCombo;
    std::unique_ptr<yup::ComboBox> windowTypeCombo;
    std::unique_ptr<yup::ComboBox> displayTypeCombo;
    std::unique_ptr<yup::Slider> releaseSlider;
    std::unique_ptr<yup::Slider> overlapSlider;

    // Status labels
    std::unique_ptr<yup::Label> frequencyLabel;
    std::unique_ptr<yup::Label> amplitudeLabel;
    std::unique_ptr<yup::Label> fftInfoLabel;

    yup::OwnedArray<yup::Label> parameterLabels;

    // Parameters
    double currentFrequency = 440.0;
    float currentAmplitude = 0.5f;
    double sweepDurationSeconds = 10.0;
    int currentFFTSize = 4096;
};
