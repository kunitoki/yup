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

#include <array>
#include <atomic>
#include <utility>

#include <yup_dsp/yup_dsp.h>

#include "NodeViewHelpers.h"

//==============================================================================
class FractionalDelayProcessor final : public yup::AudioProcessor
{
public:
    FractionalDelayProcessor()
        : AudioProcessor ("Fractional Delay",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        delayLeftMilliseconds = NodeViewHelpers::createParameter ("delayLeftMilliseconds", "L Time", 1.0f, maximumDelayMilliseconds, defaultDelayLeftMilliseconds, 1.0f);
        delayRightMilliseconds = NodeViewHelpers::createParameter ("delayRightMilliseconds", "R Time", 1.0f, maximumDelayMilliseconds, defaultDelayRightMilliseconds, 1.0f);
        feedback = NodeViewHelpers::createParameter ("feedback", "Feedback", 0.0f, maximumFeedback, 0.35f, 0.01f);
        dryWet = NodeViewHelpers::createParameter ("dryWet", "Dry/Wet", 0.0f, 1.0f, 0.5f, 0.01f);

        addParameter (delayLeftMilliseconds);
        addParameter (delayRightMilliseconds);
        addParameter (feedback);
        addParameter (dryWet);
    }

    void prepareToPlay (float newSampleRate, int) override
    {
        sampleRate.store (yup::jmax (1.0f, newSampleRate), std::memory_order_relaxed);

        const int maxDelaySamples = yup::roundToInt (delayMillisecondsToSamples (maximumDelayMilliseconds));
        for (auto& delayLine : delayLines)
        {
            delayLine.setMaxDelaySamples (maxDelaySamples);
            delayLine.reset();
        }

        feedbackState = {};
    }

    void releaseResources() override {}

    void flush() override
    {
        for (auto& delayLine : delayLines)
            delayLine.reset();

        feedbackState = {};
    }

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        if (delayLines[0].getBufferSize() == 0 || delayLines[1].getBufferSize() == 0)
            return;

        const auto currentDelayLeftSamples = delayMillisecondsToSamples (getDelayLeftMilliseconds());
        const auto currentDelayRightSamples = delayMillisecondsToSamples (getDelayRightMilliseconds());

        delayLines[0].setDelaySamples (currentDelayLeftSamples);
        delayLines[1].setDelaySamples (currentDelayRightSamples);

        auto& audioBuffer = context.audio;

        const auto currentFeedback = getFeedback();
        const auto currentDryWet = getDryWet();
        const auto dryGain = 1.0f - currentDryWet;

        const int channels = yup::jmin (audioBuffer.getNumChannels(), static_cast<int> (delayLines.size()));

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* channelData = audioBuffer.getWritePointer (channel);
            auto& delayLine = delayLines[static_cast<size_t> (channel)];
            auto& channelFeedback = feedbackState[static_cast<size_t> (channel)];

            for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
            {
                const auto input = channelData[sample];
                const auto delayed = delayLine.processSample (input + channelFeedback * currentFeedback);

                channelFeedback = delayed;
                channelData[sample] = dryGain * input + currentDryWet * delayed;
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

    yup::Result loadStateFromMemory (const yup::MemoryBlock& data) override
    {
        if (const auto result = AudioProcessor::loadStateFromMemory (data); result.wasOk())
            return result;

        if (data.isEmpty())
            return yup::Result::ok();

        yup::MemoryInputStream stream (data, false);

        const int version = stream.readInt();
        if (version != 1)
            return yup::Result::fail ("Unsupported fractional delay node state version");

        setDelayLeftMilliseconds (stream.readFloat());
        setDelayRightMilliseconds (stream.readFloat());
        setFeedback (stream.readFloat());
        setDryWet (stream.readFloat());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        return AudioProcessor::saveStateIntoMemory (data);
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDelayLeftMilliseconds() const noexcept
    {
        return delayLeftMilliseconds->getValue();
    }

    float getDelayRightMilliseconds() const noexcept
    {
        return delayRightMilliseconds->getValue();
    }

    float getFeedback() const noexcept
    {
        return feedback->getValue();
    }

    float getDryWet() const noexcept
    {
        return dryWet->getValue();
    }

    void setDelayLeftMilliseconds (float newDelayMilliseconds) noexcept
    {
        delayLeftMilliseconds->setValue (limitDelayMilliseconds (newDelayMilliseconds));
    }

    void setDelayRightMilliseconds (float newDelayMilliseconds) noexcept
    {
        delayRightMilliseconds->setValue (limitDelayMilliseconds (newDelayMilliseconds));
    }

    void setFeedback (float newFeedback) noexcept
    {
        feedback->setValue (newFeedback);
    }

    void setDryWet (float newDryWet) noexcept
    {
        dryWet->setValue (newDryWet);
    }

private:
    static constexpr const char* stateType = "FractionalDelayState";

    static constexpr float defaultDelayLeftMilliseconds = 250.0f;
    static constexpr float defaultDelayRightMilliseconds = 375.0f;
    static constexpr float maximumDelayMilliseconds = 2000.0f;
    static constexpr float maximumFeedback = 0.95f;

    static float limitDelayMilliseconds (float milliseconds) noexcept
    {
        return yup::jlimit (1.0f, maximumDelayMilliseconds, milliseconds);
    }

    double delayMillisecondsToSamples (float milliseconds) const noexcept
    {
        const auto sr = sampleRate.load (std::memory_order_relaxed);
        return (static_cast<double> (milliseconds) * static_cast<double> (sr)) / 1000.0;
    }

    std::atomic<float> sampleRate { 44100.0f };
    yup::AudioParameter::Ptr delayLeftMilliseconds;
    yup::AudioParameter::Ptr delayRightMilliseconds;
    yup::AudioParameter::Ptr feedback;
    yup::AudioParameter::Ptr dryWet;
    std::array<yup::FractionallyAddressedDelayFloat, 2> delayLines;
    std::array<float, 2> feedbackState {};
};

//==============================================================================
class FractionalDelayNodeView final : public yup::AudioGraphNodeView
{
public:
    FractionalDelayNodeView (yup::AudioGraphNodeID nodeID, FractionalDelayProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , delayLeftSlider (yup::Slider::LinearBarHorizontal)
        , delayRightSlider (yup::Slider::LinearBarHorizontal)
        , feedbackSlider (yup::Slider::LinearBarHorizontal)
        , dryWetSlider (yup::Slider::LinearBarHorizontal)
    {
        configureDelaySlider (delayLeftSlider, processor.getDelayLeftMilliseconds(), [this] (double value)
        {
            processor.setDelayLeftMilliseconds (static_cast<float> (value));
            repaint();
        });

        configureDelaySlider (delayRightSlider, processor.getDelayRightMilliseconds(), [this] (double value)
        {
            processor.setDelayRightMilliseconds (static_cast<float> (value));
            repaint();
        });

        configureUnitSlider (feedbackSlider, processor.getFeedback(), maximumFeedback, [this] (double value)
        {
            processor.setFeedback (static_cast<float> (value));
            repaint();
        });

        configureUnitSlider (dryWetSlider, processor.getDryWet(), 1.0, [this] (double value)
        {
            processor.setDryWet (static_cast<float> (value));
            repaint();
        });

        addAndMakeVisible (delayLeftSlider);
        addAndMakeVisible (delayRightSlider);
        addAndMakeVisible (feedbackSlider);
        addAndMakeVisible (dryWetSlider);
    }

    yup::String getNodeTitle() const override { return "FAD"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 260; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff97316); }

    yup::String getNodeSubtitle() const override
    {
        return yup::String ("L ") + yup::String (processor.getDelayLeftMilliseconds(), 0)
             + " / R " + yup::String (processor.getDelayRightMilliseconds(), 0) + " ms";
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 4; }

    ParameterInfo getParameterInfo (int parameterIndex) const override
    {
        switch (parameterIndex)
        {
            case 0:
                return { "L Time", millisecondsString (processor.getDelayLeftMilliseconds()), getPortKindColor (PortKind::parameter), delayNormalized (processor.getDelayLeftMilliseconds()), PortKind::parameter };

            case 1:
                return { "R Time", millisecondsString (processor.getDelayRightMilliseconds()), getPortKindColor (PortKind::parameter), delayNormalized (processor.getDelayRightMilliseconds()), PortKind::parameter };

            case 2:
                return { "Feedback", percentageString (processor.getFeedback()), getPortKindColor (PortKind::parameter), processor.getFeedback() / maximumFeedback, PortKind::parameter };

            case 3:
                return { "Dry/Wet", percentageString (processor.getDryWet()), getPortKindColor (PortKind::parameter), processor.getDryWet(), PortKind::parameter };

            default:
                return {};
        }
    }

    void resized() override
    {
        delayLeftSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
        delayRightSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 1));
        feedbackSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 2));
        dryWetSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 3));
    }

private:
    static constexpr float maximumFeedback = 0.95f;

    template <typename Callback>
    void configureDelaySlider (yup::Slider& slider, float value, Callback&& callback)
    {
        NodeViewHelpers::configureParameterSlider (slider, getPortKindColor (PortKind::parameter));
        slider.setRange (1.0, 2000.0, 1.0);
        slider.setSkewFactorFromMidpoint (250.0);
        slider.setValue (value, yup::dontSendNotification);
        slider.onValueChanged = std::forward<Callback> (callback);
    }

    template <typename Callback>
    void configureUnitSlider (yup::Slider& slider, float value, double maximum, Callback&& callback)
    {
        NodeViewHelpers::configureParameterSlider (slider, getPortKindColor (PortKind::parameter));
        slider.setRange (0.0, maximum, 0.01);
        slider.setValue (value, yup::dontSendNotification);
        slider.onValueChanged = std::forward<Callback> (callback);
    }

    static float delayNormalized (float milliseconds) noexcept
    {
        return yup::jlimit (0.0f, 1.0f, (milliseconds - 1.0f) / 1999.0f);
    }

    static yup::String millisecondsString (float milliseconds)
    {
        return yup::String (milliseconds, 0) + " ms";
    }

    static yup::String percentageString (float value)
    {
        return yup::String (value * 100.0f, 0) + "%";
    }

    FractionalDelayProcessor& processor;
    yup::Slider delayLeftSlider;
    yup::Slider delayRightSlider;
    yup::Slider feedbackSlider;
    yup::Slider dryWetSlider;
};
