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

#include "NodeViewHelpers.h"

//==============================================================================
class StateVariableFilterProcessor final : public yup::AudioProcessor
{
public:
    StateVariableFilterProcessor()
        : AudioProcessor ("State Variable Filter",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        mode = NodeViewHelpers::createParameter ("mode", "Mode", 0.0f, 3.0f, 0.0f, 1.0f);
        cutoff = NodeViewHelpers::createParameter ("cutoff", "Cutoff", 20.0f, 20000.0f, 1000.0f);
        resonance = NodeViewHelpers::createParameter ("resonance", "Resonance", 0.5f, 20.0f, 0.707f);
        addParameter (mode);
        addParameter (cutoff);
        addParameter (resonance);
    }

    void prepareToPlay (float newSampleRate, int blockSize) override
    {
        sampleRate = newSampleRate;

        for (auto& f : filters)
            f.prepare (newSampleRate, blockSize);

        smoothedCutoff.reset (newSampleRate, 0.02);
        smoothedCutoff.setCurrentAndTargetValue (static_cast<float> (getCutoff()));

        smoothedResonance.reset (newSampleRate, 0.02);
        smoothedResonance.setCurrentAndTargetValue (static_cast<float> (getResonance()));
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        const int numSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();

        smoothedCutoff.setTargetValue (static_cast<float> (getCutoff()));
        smoothedResonance.setTargetValue (static_cast<float> (getResonance()));

        const auto filterMode = modeIndexToFilterMode (static_cast<int> (getMode()));

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float freq = smoothedCutoff.getNextValue();
            const float q = smoothedResonance.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto idx = static_cast<size_t> (yup::jmin (channel, 1));
                filters[idx].setParameters (filterMode, freq, q, sampleRate);
                audioBuffer.setSample (channel, sample, filters[idx].processSample (audioBuffer.getSample (channel, sample)));
            }
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

    float getMode() const noexcept { return mode->getValue(); }

    double getCutoff() const noexcept { return static_cast<double> (cutoff->getValue()); }

    double getResonance() const noexcept { return static_cast<double> (resonance->getValue()); }

    static yup::String modeIndexToString (int index)
    {
        switch (index)
        {
            case 1:
                return "HP";
            case 2:
                return "BP";
            case 3:
                return "Notch";
            default:
                return "LP";
        }
    }

private:
    static constexpr const char* stateType = "StateVariableFilterState";

    static yup::FilterModeType modeIndexToFilterMode (int index) noexcept
    {
        switch (index)
        {
            case 1:
                return yup::FilterMode::highpass;
            case 2:
                return yup::FilterMode::bandpass;
            case 3:
                return yup::FilterMode::bandstop;
            default:
                return yup::FilterMode::lowpass;
        }
    }

    float sampleRate = 44100.0f;
    yup::AudioParameter::Ptr mode;
    yup::AudioParameter::Ptr cutoff;
    yup::AudioParameter::Ptr resonance;
    yup::SmoothedValue<float> smoothedCutoff;
    yup::SmoothedValue<float> smoothedResonance;
    yup::StateVariableFilter<float> filters[2];
};

//==============================================================================
class StateVariableFilterNodeView final : public yup::AudioGraphNodeView
{
public:
    StateVariableFilterNodeView (yup::AudioGraphNodeID nodeID, StateVariableFilterProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , modeSlider (yup::Slider::LinearBarHorizontal)
        , cutoffSlider (yup::Slider::LinearBarHorizontal)
        , resonanceSlider (yup::Slider::LinearBarHorizontal)
    {
        const auto accent = getPortKindColor (PortKind::parameter);

        NodeViewHelpers::configureParameterSlider (modeSlider, accent);
        modeSlider.setRange (0.0, 3.0, 1.0);
        modeSlider.setValue (static_cast<double> (processor.getMode()), yup::dontSendNotification);
        modeSlider.onValueChanged = [this] (double value)
        {
            processor.getParameters()[0]->setValue (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (modeSlider);

        NodeViewHelpers::configureParameterSlider (cutoffSlider, accent);
        cutoffSlider.setRange (20.0, 20000.0);
        cutoffSlider.setSkewFactorFromMidpoint (1000.0);
        cutoffSlider.setValue (processor.getCutoff(), yup::dontSendNotification);
        cutoffSlider.onValueChanged = [this] (double value)
        {
            processor.getParameters()[1]->setValue (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (cutoffSlider);

        NodeViewHelpers::configureParameterSlider (resonanceSlider, accent);
        resonanceSlider.setRange (0.5, 20.0);
        resonanceSlider.setValue (processor.getResonance(), yup::dontSendNotification);
        resonanceSlider.onValueChanged = [this] (double value)
        {
            processor.getParameters()[2]->setValue (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (resonanceSlider);
    }

    yup::String getNodeTitle() const override { return "SVF"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 220; }

    yup::Color getNodeColor() const override { return yup::Color (0xff7c3aed); }

    yup::String getNodeSubtitle() const override
    {
        const auto modeStr = StateVariableFilterProcessor::modeIndexToString (static_cast<int> (processor.getMode()));
        return modeStr + " | " + yup::String (processor.getCutoff(), 0) + " Hz";
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 3; }

    ParameterInfo getParameterInfo (int rowIndex) const override
    {
        switch (rowIndex)
        {
            case 0:
                return { "Mode", StateVariableFilterProcessor::modeIndexToString (static_cast<int> (processor.getMode())), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
            case 1:
                return { "Cutoff", yup::String (processor.getCutoff(), 0) + " Hz", getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
            case 2:
                return { "Q", yup::String (processor.getResonance(), 3), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
            default:
                return {};
        }
    }

    void resized() override
    {
        modeSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
        cutoffSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 1));
        resonanceSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 2));
    }

private:
    StateVariableFilterProcessor& processor;
    yup::Slider modeSlider;
    yup::Slider cutoffSlider;
    yup::Slider resonanceSlider;
};
