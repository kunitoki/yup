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
class LowPassFilterProcessor final : public yup::AudioProcessor
{
public:
    LowPassFilterProcessor()
        : AudioProcessor ("Low-pass",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        cutoff = NodeViewHelpers::createParameter ("cutoff", "Cutoff", 80.0f, 12000.0f, 800.0f);
        addParameter (cutoff);
    }

    void prepareToPlay (float newSampleRate, int) override
    {
        sampleRate = newSampleRate;
        state[0] = 0.0f;
        state[1] = 0.0f;
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        const auto currentCutoff = getCutoff();
        const auto alpha = static_cast<float> (1.0 - std::exp (-yup::MathConstants<double>::twoPi * currentCutoff / static_cast<double> (sampleRate)));

        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
        {
            auto y = state[static_cast<size_t> (yup::jmin (channel, 1))];

            for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
            {
                y += alpha * (audioBuffer.getSample (channel, sample) - y);
                audioBuffer.setSample (channel, sample, y);
            }

            state[static_cast<size_t> (yup::jmin (channel, 1))] = y;
        }
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

    double getCutoff() const noexcept { return static_cast<double> (cutoff->getValue()); }

    void setCutoff (double newCutoff) noexcept
    {
        cutoff->setValue (static_cast<float> (newCutoff));
    }

private:
    static constexpr const char* stateType = "LowPassFilterState";

    float sampleRate = 44100.0f;
    yup::AudioParameter::Ptr cutoff;
    float state[2] {};
};

//==============================================================================
class LowPassFilterNodeView final : public yup::AudioGraphNodeView
{
public:
    LowPassFilterNodeView (yup::AudioGraphNodeID nodeID, LowPassFilterProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , cutoffSlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (cutoffSlider, getPortKindColor (PortKind::parameter));
        cutoffSlider.setRange (80.0, 12000.0);
        cutoffSlider.setSkewFactorFromMidpoint (1000.0);
        cutoffSlider.setValue (processor.getCutoff(), yup::dontSendNotification);
        cutoffSlider.onValueChanged = [this] (double value)
        {
            processor.setCutoff (value);
            repaint();
        };
        addAndMakeVisible (cutoffSlider);
    }

    yup::String getNodeTitle() const override { return "LPF"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 220; }

    yup::Color getNodeColor() const override { return yup::Color (0xff7c3aed); }

    yup::String getNodeSubtitle() const override { return yup::String (processor.getCutoff(), 0) + " Hz"; }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 1; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { "Cutoff", yup::String (processor.getCutoff(), 0), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        cutoffSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
    }

private:
    LowPassFilterProcessor& processor;
    yup::Slider cutoffSlider;
};
