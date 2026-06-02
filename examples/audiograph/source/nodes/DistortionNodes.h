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
#include <cmath>

#include <yup_dsp/yup_dsp.h>

#include "NodeViewHelpers.h"

//==============================================================================
class TanhDistortionProcessor final : public yup::AudioProcessor
{
public:
    TanhDistortionProcessor()
        : AudioProcessor ("Tanh Distortion",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        updateLatency();
    }

    void prepareToPlay (float newSampleRate, int maxBlockSize) override
    {
        const auto sampleRate = yup::jmax (1.0f, newSampleRate);
        const auto blockSize = yup::jmax (1, maxBlockSize);

        oversampler2x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversampler4x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversampler8x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversamplersPrepared = true;

        updateLatency();
    }

    void releaseResources() override {}

    void flush() override
    {
        oversampler2x.reset();
        oversampler4x.reset();
        oversampler8x.reset();
    }

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;

        const int numChannels = audioBuffer.getNumChannels();
        const int numSamples = audioBuffer.getNumSamples();

        if (numChannels <= 0 || numSamples <= 0)
            return;

        const auto currentDrive = drive.load (std::memory_order_relaxed);
        const auto currentOversamplingIndex = oversamplingIndex.load (std::memory_order_relaxed);

        if (! oversamplersPrepared || currentOversamplingIndex == 0)
        {
            processChannelsInPlace (audioBuffer, 0, numChannels, currentDrive);
            return;
        }

        const int oversampledChannels = yup::jmin (numChannels, maximumOversampledChannels);

        switch (currentOversamplingIndex)
        {
            case 1:
                processOversampled (oversampler2x, audioBuffer, oversampledChannels, numSamples, currentDrive);
                break;

            case 2:
                processOversampled (oversampler4x, audioBuffer, oversampledChannels, numSamples, currentDrive);
                break;

            case 3:
                processOversampled (oversampler8x, audioBuffer, oversampledChannels, numSamples, currentDrive);
                break;

            default:
                break;
        }

        processChannelsInPlace (audioBuffer, oversampledChannels, numChannels, currentDrive);
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
            return yup::Result::fail ("Unsupported tanh distortion node state version");

        setDrive (stream.readFloat());
        setOversamplingIndex (stream.readInt());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        yup::MemoryOutputStream stream (data, false);
        stream.writeInt (1);
        stream.writeFloat (getDrive());
        stream.writeInt (getOversamplingIndex());
        stream.flush();

        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDrive() const noexcept
    {
        return drive.load (std::memory_order_relaxed);
    }

    int getOversamplingIndex() const noexcept
    {
        return oversamplingIndex.load (std::memory_order_relaxed);
    }

    void setDrive (float newDrive) noexcept
    {
        drive.store (yup::jlimit (1.0f, 24.0f, newDrive), std::memory_order_relaxed);
    }

    void setOversamplingIndex (int newOversamplingIndex) noexcept
    {
        oversamplingIndex.store (yup::jlimit (0, 3, newOversamplingIndex), std::memory_order_relaxed);
        updateLatency();
    }

    yup::String getOversamplingText() const
    {
        switch (getOversamplingIndex())
        {
            case 1:
                return "2x";
            case 2:
                return "4x";
            case 3:
                return "8x";
            default:
                return "1x";
        }
    }

private:
    static constexpr int maximumOversampledChannels = 2;

    static float processSample (float input, float currentDrive) noexcept
    {
        return std::tanh (input * currentDrive);
    }

    void processChannelsInPlace (yup::AudioBuffer<float>& audioBuffer, int startChannel, int endChannel, float currentDrive) noexcept
    {
        for (int channel = startChannel; channel < endChannel; ++channel)
        {
            auto* channelData = audioBuffer.getWritePointer (channel);

            for (int sample = 0; sample < audioBuffer.getNumSamples(); ++sample)
                channelData[sample] = processSample (channelData[sample], currentDrive);
        }
    }

    template <typename OversamplerType>
    void processOversampled (OversamplerType& oversampler,
                             yup::AudioBuffer<float>& audioBuffer,
                             int numChannels,
                             int numSamples,
                             float currentDrive) noexcept
    {
        oversampler.upsample (audioBuffer.getArrayOfReadPointers(), numChannels, numSamples);

        oversampler.processOversampledBlock ([&] (auto& oversampledBuffer)
        {
            const int oversampledSamples = oversampler.getOversampledNumSamples();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = oversampledBuffer.getWritePointer (channel);

                for (int sample = 0; sample < oversampledSamples; ++sample)
                    *channelData++ = processSample (*channelData, currentDrive);
            }
        });

        oversampler.downsample (audioBuffer.getArrayOfWritePointers(), numChannels, numSamples);
    }

    void updateLatency()
    {
        int latencySamples = 0;

        switch (getOversamplingIndex())
        {
            case 1:
                latencySamples = oversampler2x.getLatencyInSamples();
                break;
            case 2:
                latencySamples = oversampler4x.getLatencyInSamples();
                break;
            case 3:
                latencySamples = oversampler8x.getLatencyInSamples();
                break;
            default:
                break;
        }

        setLatencySamples (latencySamples);
    }

    std::atomic<float> drive { 4.0f };
    std::atomic<int> oversamplingIndex { 2 };
    yup::Oversampler2xFloat oversampler2x;
    yup::Oversampler4xFloat oversampler4x;
    yup::Oversampler8xFloat oversampler8x;
    bool oversamplersPrepared = false;
};

//==============================================================================
class BlunterSoftClipperProcessor final : public yup::AudioProcessor
{
public:
    BlunterSoftClipperProcessor()
        : AudioProcessor ("Blunter Soft Clip",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
    }

    void prepareToPlay (float newSampleRate, int maxBlockSize) override
    {
        for (auto& clipper : clippers)
            clipper.prepare (newSampleRate, maxBlockSize);
    }

    void releaseResources() override {}

    void flush() override
    {
        for (auto& clipper : clippers)
            clipper.reset();
    }

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;

        const auto currentDrive = drive.load (std::memory_order_relaxed);
        const auto currentOutput = output.load (std::memory_order_relaxed);
        const int clipperChannels = static_cast<int> (clippers.size());

        for (auto& clipper : clippers)
            clipper.setParameters (currentDrive, currentOutput);

        for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
        {
            auto* channelData = audioBuffer.getWritePointer (channel);
            auto& clipper = clippers[static_cast<std::size_t> (yup::jmin (channel, clipperChannels - 1))];

            clipper.processInPlace (channelData, audioBuffer.getNumSamples());
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
            return yup::Result::fail ("Unsupported blunter soft clipper node state version");

        setDrive (stream.readFloat());
        setOutput (stream.readFloat());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        yup::MemoryOutputStream stream (data, false);
        stream.writeInt (1);
        stream.writeFloat (getDrive());
        stream.writeFloat (getOutput());
        stream.flush();

        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDrive() const noexcept
    {
        return drive.load (std::memory_order_relaxed);
    }

    float getOutput() const noexcept
    {
        return output.load (std::memory_order_relaxed);
    }

    void setDrive (float newDrive) noexcept
    {
        drive.store (yup::jlimit (0.1f, 24.0f, newDrive), std::memory_order_relaxed);
    }

    void setOutput (float newOutput) noexcept
    {
        output.store (yup::jlimit (0.0f, 1.5f, newOutput), std::memory_order_relaxed);
    }

private:
    std::atomic<float> drive { 1.0f };
    std::atomic<float> output { 0.5f };
    std::array<yup::BlunterClipperFloat, 2> clippers;
};

//==============================================================================
class TanhDistortionNodeView final : public yup::AudioGraphNodeView
{
public:
    TanhDistortionNodeView (yup::AudioGraphNodeID nodeID, TanhDistortionProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , driveSlider (yup::Slider::LinearBarHorizontal)
        , oversamplingSlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (driveSlider, getPortKindColor (PortKind::parameter));
        driveSlider.setRange (1.0, 24.0, 0.1);
        driveSlider.setSkewFactorFromMidpoint (4.0);
        driveSlider.setValue (processor.getDrive(), yup::dontSendNotification);
        driveSlider.onValueChanged = [this] (double value)
        {
            processor.setDrive (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (driveSlider);

        NodeViewHelpers::configureParameterSlider (oversamplingSlider, getPortKindColor (PortKind::parameter));
        oversamplingSlider.setRange (0.0, 3.0, 1.0);
        oversamplingSlider.setValue (processor.getOversamplingIndex(), yup::dontSendNotification);
        oversamplingSlider.onValueChanged = [this] (double value)
        {
            processor.setOversamplingIndex (yup::roundToInt (value));
            repaint();
        };
        addAndMakeVisible (oversamplingSlider);
    }

    yup::String getNodeTitle() const override { return "TANH"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 240; }

    yup::Color getNodeColor() const override { return yup::Color (0xffef4444); }

    yup::String getNodeSubtitle() const override
    {
        return yup::String ("x ") + yup::String (processor.getDrive(), 1) + " / " + processor.getOversamplingText();
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 2; }

    ParameterInfo getParameterInfo (int parameterIndex) const override
    {
        switch (parameterIndex)
        {
            case 0:
                return { "Drive", yup::String (processor.getDrive(), 1), getPortKindColor (PortKind::parameter), normalizedDrive (processor.getDrive()), PortKind::parameter };

            case 1:
                return { "Oversampling", processor.getOversamplingText(), getPortKindColor (PortKind::parameter), static_cast<float> (processor.getOversamplingIndex()) / 3.0f, PortKind::parameter };

            default:
                return {};
        }
    }

    void resized() override
    {
        driveSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
        oversamplingSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 1));
    }

private:
    static float normalizedDrive (float value) noexcept
    {
        return yup::jlimit (0.0f, 1.0f, (value - 1.0f) / 23.0f);
    }

    TanhDistortionProcessor& processor;
    yup::Slider driveSlider;
    yup::Slider oversamplingSlider;
};

//==============================================================================
class BlunterSoftClipperNodeView final : public yup::AudioGraphNodeView
{
public:
    BlunterSoftClipperNodeView (yup::AudioGraphNodeID nodeID, BlunterSoftClipperProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , driveSlider (yup::Slider::LinearBarHorizontal)
        , outputSlider (yup::Slider::LinearBarHorizontal)
    {
        NodeViewHelpers::configureParameterSlider (driveSlider, getPortKindColor (PortKind::parameter));
        driveSlider.setRange (0.1, 24.0, 0.1);
        driveSlider.setSkewFactorFromMidpoint (2.0);
        driveSlider.setValue (processor.getDrive(), yup::dontSendNotification);
        driveSlider.onValueChanged = [this] (double value)
        {
            processor.setDrive (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (driveSlider);

        NodeViewHelpers::configureParameterSlider (outputSlider, getPortKindColor (PortKind::parameter));
        outputSlider.setRange (0.0, 1.5, 0.01);
        outputSlider.setValue (processor.getOutput(), yup::dontSendNotification);
        outputSlider.onValueChanged = [this] (double value)
        {
            processor.setOutput (static_cast<float> (value));
            repaint();
        };
        addAndMakeVisible (outputSlider);
    }

    yup::String getNodeTitle() const override { return "BLUNTER"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 240; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff97316); }

    yup::String getNodeSubtitle() const override
    {
        return yup::String ("x ") + yup::String (processor.getDrive(), 1) + " -> " + yup::String (processor.getOutput(), 2);
    }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 2; }

    ParameterInfo getParameterInfo (int parameterIndex) const override
    {
        switch (parameterIndex)
        {
            case 0:
                return { "Drive", yup::String (processor.getDrive(), 1), getPortKindColor (PortKind::parameter), normalizedDrive (processor.getDrive()), PortKind::parameter };

            case 1:
                return { "Output", yup::String (processor.getOutput(), 2), getPortKindColor (PortKind::parameter), processor.getOutput() / 1.5f, PortKind::parameter };

            default:
                return {};
        }
    }

    void resized() override
    {
        driveSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 0));
        outputSlider.setBounds (NodeViewHelpers::getInlineSliderBounds (*this, getPreferredWidth(), 1));
    }

private:
    static float normalizedDrive (float value) noexcept
    {
        return yup::jlimit (0.0f, 1.0f, (value - 0.1f) / 23.9f);
    }

    BlunterSoftClipperProcessor& processor;
    yup::Slider driveSlider;
    yup::Slider outputSlider;
};
