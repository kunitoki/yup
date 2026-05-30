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

#include "NodeViewHelpers.h"

//==============================================================================
class LatencyProcessor final : public yup::AudioProcessor
{
public:
    LatencyProcessor()
        : AudioProcessor ("Latency",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        setDelayMilliseconds (defaultDelayMilliseconds);
        reportUpdatedLatency();
    }

    void prepareToPlay (float newSampleRate, int maxBlockSize) override
    {
        sampleRate.store (yup::jmax (1.0f, newSampleRate), std::memory_order_relaxed);

        const int maxDelaySamples = delayMillisecondsToSamples (maximumDelayMilliseconds);
        history.setSize (2, maxDelaySamples + yup::jmax (1, maxBlockSize) + 1);
        history.clear();

        writePosition = 0;
    }

    void releaseResources() override {}

    void flush() override
    {
        history.clear();
        writePosition = 0;
    }

    void processBlock (yup::AudioBuffer<float>& audioBuffer, yup::MidiBuffer&) override
    {
        const int currentDelaySamples = getLatencySamples();
        if (currentDelaySamples <= 0)
            return;

        const int ringSize = history.getNumSamples();
        if (ringSize <= currentDelaySamples)
        {
            audioBuffer.clear();
            return;
        }

        const int channels = yup::jmin (audioBuffer.getNumChannels(), history.getNumChannels());

        for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
        {
            const int readPosition = (writePosition + ringSize - currentDelaySamples) % ringSize;

            for (int channel = 0; channel < channels; ++channel)
            {
                const float input = audioBuffer.getReadPointer (channel)[sample];
                audioBuffer.getWritePointer (channel)[sample] = history.getReadPointer (channel)[readPosition];
                history.getWritePointer (channel)[writePosition] = input;
            }

            writePosition = (writePosition + 1) % ringSize;
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
            return yup::Result::fail ("Unsupported latency node state version");

        setDelayMilliseconds (stream.readFloat());
        reportUpdatedLatency();

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        yup::MemoryOutputStream stream (data, false);
        stream.writeInt (1);
        stream.writeFloat (delayMilliseconds.load (std::memory_order_relaxed));
        stream.flush();

        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDelayMilliseconds() const noexcept
    {
        return delayMilliseconds.load (std::memory_order_relaxed);
    }

    int getDelaySamples() const noexcept
    {
        return delaySamples.load (std::memory_order_relaxed);
    }

    void setDelayMilliseconds (float newDelayMilliseconds)
    {
        delayMilliseconds.store (yup::jlimit (0.0f, maximumDelayMilliseconds, newDelayMilliseconds), std::memory_order_relaxed);

        const auto newDelaySamples = delayMillisecondsToSamples (delayMilliseconds.load (std::memory_order_relaxed));
        delaySamples.store (newDelaySamples, std::memory_order_relaxed);
    }

    void reportUpdatedLatency()
    {
        setLatencySamples (getDelaySamples());
    }

private:
    static constexpr float defaultDelayMilliseconds = 100.0f;
    static constexpr float maximumDelayMilliseconds = 1000.0f;

    int delayMillisecondsToSamples (float milliseconds) const noexcept
    {
        const auto sr = sampleRate.load (std::memory_order_relaxed);
        return yup::roundToInt ((static_cast<double> (milliseconds) * static_cast<double> (sr)) / 1000.0);
    }

    std::atomic<float> sampleRate { 44100.0f };
    std::atomic<float> delayMilliseconds { defaultDelayMilliseconds };
    std::atomic<int> delaySamples { 0 };
    int writePosition = 0;
    yup::AudioBuffer<float> history;
};

//==============================================================================
class LatencyNodeView final : public yup::AudioGraphNodeView
{
public:
    LatencyNodeView (yup::AudioGraphNodeID nodeID, LatencyProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , delaySlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (delaySlider, getPortKindColor (PortKind::parameter));
        delaySlider.setRange (0.0, 1000.0, 1.0);
        delaySlider.setSkewFactorFromMidpoint (100.0);
        delaySlider.setValue (processor.getDelayMilliseconds(), yup::dontSendNotification);
        delaySlider.onValueChanged = [this] (double value)
        {
            processor.setDelayMilliseconds (static_cast<float> (value));

            repaint();
        };
        delaySlider.onDragEnd = [this] (const yup::MouseEvent&)
        {
            processor.reportUpdatedLatency();
        };
        addAndMakeVisible (delaySlider);
    }

    yup::String getNodeTitle() const override { return "LATENCY"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 240; }

    yup::Color getNodeColor() const override { return yup::Color (0xffeab308); }

    yup::String getNodeSubtitle() const override
    {
        return yup::String (processor.getDelayMilliseconds(), 0) + " ms / " + yup::String (processor.getDelaySamples()) + " samples";
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 1; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { "Delay", yup::String (processor.getDelayMilliseconds(), 0) + " ms", getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        delaySlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
    }

private:
    LatencyProcessor& processor;
    yup::Slider delaySlider;
};
