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

#include <atomic>
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
    }

    void prepareToPlay (float newSampleRate, int) override
    {
        sampleRate = newSampleRate;
        phase = 0.0;
    }

    void releaseResources() override {}

    void processBlock (yup::AudioBuffer<float>& audioBuffer, yup::MidiBuffer&) override
    {
        const auto currentFrequency = static_cast<double> (frequency.load (std::memory_order_relaxed));
        const auto increment = yup::MathConstants<double>::twoPi * currentFrequency / static_cast<double> (sampleRate);

        for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
        {
            const auto value = static_cast<float> (std::sin (phase) * 0.22);

            for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
                audioBuffer.setSample (channel, sample, value);

            phase += increment;
            if (phase >= yup::MathConstants<double>::twoPi)
                phase -= yup::MathConstants<double>::twoPi;
        }
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    yup::Result loadStateFromMemory (const yup::MemoryBlock& data) override
    {
        if (data.isEmpty())
            return yup::Result::ok();

        yup::MemoryInputStream stream (data, false);

        const int version = stream.readInt();
        if (version != 1)
            return yup::Result::fail ("Unsupported oscillator node state version");

        setFrequency (stream.readFloat());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        yup::MemoryOutputStream stream (data, false);
        stream.writeInt (1);
        stream.writeFloat (frequency.load (std::memory_order_relaxed));
        stream.flush();

        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    double getFrequency() const noexcept { return static_cast<double> (frequency.load (std::memory_order_relaxed)); }

    void setFrequency (double newFrequency) noexcept
    {
        frequency.store (static_cast<float> (yup::jlimit (40.0, 2000.0, newFrequency)), std::memory_order_relaxed);
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    std::atomic<float> frequency { 440.0f };
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
    }

    yup::String getNodeTitle() const override { return "OSC"; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 220; }

    yup::Color getNodeColor() const override { return yup::Color (0xff2563eb); }

    yup::String getNodeSubtitle() const override { return yup::String (processor.getFrequency(), 0) + " Hz"; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 1; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { "Frequency", yup::String (processor.getFrequency(), 0), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        frequencySlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
    }

private:
    OscillatorProcessor& processor;
    yup::Slider frequencySlider;
};
