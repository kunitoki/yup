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
class GainProcessor final : public yup::AudioProcessor
{
public:
    GainProcessor()
        : AudioProcessor ("Gain",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        gain = NodeViewHelpers::createParameter ("gain", "Gain", 0.0f, 1.5f, 0.75f);
        addParameter (gain);
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        context.audio.applyGain (getGain());
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

    float getGain() const noexcept { return gain->getValue(); }

    void setGain (float newGain) noexcept
    {
        gain->setValue (newGain);
    }

private:
    static constexpr const char* stateType = "GainState";

    yup::AudioParameter::Ptr gain;
};

//==============================================================================
class GainNodeView final : public yup::AudioGraphNodeView
{
public:
    GainNodeView (yup::AudioGraphNodeID nodeID, GainProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , gainSlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (gainSlider, getPortKindColor (PortKind::parameter));
        gainSlider.setRange (0.0, 1.5);
        gainSlider.setValue (processor.getGain(), yup::dontSendNotification);
        gainSlider.onValueChanged = [this] (double value)
        {
            processor.setGain (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (gainSlider);
    }

    yup::String getNodeTitle() const override { return "GAIN"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 220; }

    yup::Color getNodeColor() const override { return yup::Color (0xff10b981); }

    yup::String getNodeSubtitle() const override { return yup::String ("x ") + yup::String (processor.getGain(), 2); }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 1; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { "Gain", yup::String (processor.getGain(), 2), getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        gainSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
    }

private:
    GainProcessor& processor;
    yup::Slider gainSlider;
};
