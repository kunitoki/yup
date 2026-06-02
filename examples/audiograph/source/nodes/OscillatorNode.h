/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <cmath>

#include "NodeViewHelpers.h"

//==============================================================================
class OscillatorProcessor final : public yup::AudioProcessor
{
public:
    OscillatorProcessor()
        : AudioProcessor ("Oscillator",
                          yup::AudioBusLayout ({},
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        frequency = NodeViewHelpers::createParameter ("frequency", "Frequency", 40.0f, 2000.0f, 440.0f);
        sweepEnabled = NodeViewHelpers::createParameter ("sweepEnabled", "Sweep", 0.0f, 1.0f, 0.0f, 1.0f);

        addParameter (frequency);
        addParameter (sweepEnabled);
    }

    void prepareToPlay (float newSampleRate, int) override
    {
        sampleRate = newSampleRate;
        phase = 0.0;
        sweepPositionSeconds = 0.0;
        wasSweepActive = false;
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        const auto baseFrequency = getFrequency();
        const auto nyquist = static_cast<double> (sampleRate) * 0.5;
        const auto sweepActive = isSweepEnabled() && nyquist > sweepStartFrequency;

        if (sweepActive && ! wasSweepActive)
            sweepPositionSeconds = 0.0;

        for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
        {
            const auto currentFrequency = sweepActive ? getSweepFrequency (nyquist)
                                                      : baseFrequency;
            const auto increment = yup::MathConstants<double>::twoPi * currentFrequency / static_cast<double> (sampleRate);
            const auto value = static_cast<float> (std::sin (phase) * 0.22);

            for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
                audioBuffer.setSample (channel, sample, value);

            phase += increment;
            if (phase >= yup::MathConstants<double>::twoPi)
                phase -= yup::MathConstants<double>::twoPi;

            if (sweepActive)
            {
                sweepPositionSeconds += 1.0 / static_cast<double> (sampleRate);
                while (sweepPositionSeconds >= sweepDurationSeconds)
                    sweepPositionSeconds -= sweepDurationSeconds;
            }
            else
            {
                sweepPositionSeconds = 0.0;
            }
        }

        wasSweepActive = sweepActive;
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        return NodeViewHelpers::loadParameterState (state, stateType, getParameters());
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = NodeViewHelpers::createParameterState (stateType, getParameters());
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    double getFrequency() const noexcept { return static_cast<double> (frequency->getValue()); }

    void setFrequency (double newFrequency) noexcept
    {
        frequency->setValue (static_cast<float> (newFrequency));
    }

    bool isSweepEnabled() const noexcept { return sweepEnabled->getValue() >= 0.5f; }

    void setSweepEnabled (bool shouldBeEnabled) noexcept
    {
        sweepEnabled->setValue (shouldBeEnabled ? 1.0f : 0.0f);
    }

private:
    static constexpr const char* stateType = "OscillatorState";
    static constexpr double sweepStartFrequency = 20.0;
    static constexpr double sweepDurationSeconds = 10.0;

    double getSweepProgress() const noexcept
    {
        return sweepPositionSeconds / sweepDurationSeconds;
    }

    double getSweepFrequency (double nyquist) const noexcept
    {
        const auto progress = yup::jlimit (0.0, 1.0, getSweepProgress());
        return sweepStartFrequency * std::pow (nyquist / sweepStartFrequency, progress);
    }

    double sampleRate = 44100.0;
    double phase = 0.0;
    double sweepPositionSeconds = 0.0;
    bool wasSweepActive = false;
    yup::AudioParameter::Ptr frequency;
    yup::AudioParameter::Ptr sweepEnabled;
};

//==============================================================================
class OscillatorNodeView final : public yup::AudioGraphNodeView
{
public:
    OscillatorNodeView (yup::AudioGraphNodeID nodeID, OscillatorProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , frequencySlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (frequencySlider, getPortKindColor (PortKind::parameter));
        frequencySlider.setRange (40.0, 2000.0);
        frequencySlider.setSkewFactorFromMidpoint (440.0);
        frequencySlider.setValue (processor.getFrequency(), yup::dontSendNotification);
        frequencySlider.onValueChanged = [this] (double value)
        {
            processor.setFrequency (value);
            repaint();
        };
        addAndMakeVisible (frequencySlider);

        sweepButton.setButtonText ("Sweep");
        sweepButton.setToggleState (processor.isSweepEnabled(), yup::dontSendNotification);
        sweepButton.onClick = [this]
        {
            processor.setSweepEnabled (sweepButton.getToggleState());
            repaint();
        };
        addAndMakeVisible (sweepButton);
    }

    yup::String getNodeTitle() const override { return "OSC"; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 220; }

    yup::Color getNodeColor() const override { return yup::Color (0xff2563eb); }

    yup::String getNodeSubtitle() const override
    {
        if (! processor.isSweepEnabled())
            return yup::String (processor.getFrequency(), 0) + " Hz";

        return "20 Hz-Nyq / 10 s";
    }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 2; }

    ParameterInfo getParameterInfo (int parameterIndex) const override
    {
        if (parameterIndex == 0)
            return { "Frequency", yup::String (processor.getFrequency(), 0), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };

        return { "Sweep", processor.isSweepEnabled() ? "20 Hz-Nyq" : "off", getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        frequencySlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));

        const auto scale = getLocalBounds().getWidth() / static_cast<float> (getPreferredWidth());
        sweepButton.setBounds (62.0f * scale,
                               74.0f * scale,
                               72.0f * scale,
                               20.0f * scale);
    }

private:
    OscillatorProcessor& processor;
    yup::Slider frequencySlider;
    yup::ToggleButton sweepButton;
};
