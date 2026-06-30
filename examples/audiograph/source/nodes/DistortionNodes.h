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
        drive = NodeViewHelpers::createParameter ("drive", "Drive", 1.0f, 24.0f, 4.0f, 0.1f);
        oversamplingIndex = NodeViewHelpers::createParameter ("oversampling", "Oversampling", 0.0f, 3.0f, 2.0f, 1.0f);

        addParameter (drive);
        addParameter (oversamplingIndex);

        updateLatency();
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        const auto sampleRate = yup::jmax (1.0f, spec.sampleRate);
        const auto blockSize = yup::jmax (1, spec.maxBlockSize);

        oversampler2x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversampler4x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversampler8x.prepare (sampleRate, maximumOversampledChannels, blockSize);
        oversamplersPrepared = true;

        smoothedDrive.reset (sampleRate, 0.02);
        smoothedDrive.setCurrentAndTargetValue (getDrive());

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

        smoothedDrive.setTargetValue (getDrive());
        const auto currentOversamplingIndex = getOversamplingIndex();

        if (! oversamplersPrepared || currentOversamplingIndex == 0)
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float drive = smoothedDrive.getNextValue();
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    auto* channelData = audioBuffer.getWritePointer (channel);
                    channelData[sample] = processSample (channelData[sample], drive);
                }
            }
            return;
        }

        const float effectiveDrive = smoothedDrive.skip (numSamples);
        const int oversampledChannels = yup::jmin (numChannels, maximumOversampledChannels);

        switch (currentOversamplingIndex)
        {
            case 1:
                processOversampled (oversampler2x, audioBuffer, oversampledChannels, numSamples, effectiveDrive);
                break;

            case 2:
                processOversampled (oversampler4x, audioBuffer, oversampledChannels, numSamples, effectiveDrive);
                break;

            case 3:
                processOversampled (oversampler8x, audioBuffer, oversampledChannels, numSamples, effectiveDrive);
                break;

            default:
                break;
        }

        processChannelsInPlace (audioBuffer, oversampledChannels, numChannels, effectiveDrive);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        if (const auto result = NodeViewHelpers::loadParameterState (state, stateType, getParameters()); result.failed())
            return result;

        updateLatency();
        return yup::Result::ok();
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = NodeViewHelpers::createParameterState (stateType, getParameters());
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    float getDrive() const noexcept
    {
        return drive->getValue();
    }

    int getOversamplingIndex() const noexcept
    {
        return yup::roundToInt (oversamplingIndex->getValue());
    }

    void setDrive (float newDrive) noexcept
    {
        drive->setValue (newDrive);
    }

    void setOversamplingIndex (int newOversamplingIndex) noexcept
    {
        oversamplingIndex->setValue (static_cast<float> (newOversamplingIndex));
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
    static constexpr const char* stateType = "TanhDistortionState";
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

    yup::AudioParameter::Ptr drive;
    yup::AudioParameter::Ptr oversamplingIndex;
    yup::SmoothedValue<float> smoothedDrive;
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
        drive = NodeViewHelpers::createParameter ("drive", "Drive", 0.1f, 24.0f, 1.0f, 0.1f);
        output = NodeViewHelpers::createParameter ("output", "Output", 0.0f, 1.5f, 0.5f, 0.01f);

        addParameter (drive);
        addParameter (output);
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        for (auto& clipper : clippers)
            clipper.prepare (spec.sampleRate, spec.maxBlockSize);

        smoothedDrive.reset (spec.sampleRate, 0.02);
        smoothedDrive.setCurrentAndTargetValue (getDrive());
        smoothedOutput.reset (spec.sampleRate, 0.02);
        smoothedOutput.setCurrentAndTargetValue (getOutput());
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

        const int numChannels = audioBuffer.getNumChannels();
        const int numSamples = audioBuffer.getNumSamples();
        const int clipperChans = static_cast<int> (clippers.size());

        smoothedDrive.setTargetValue (getDrive());
        smoothedOutput.setTargetValue (getOutput());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float drive = smoothedDrive.getNextValue();
            const float out = smoothedOutput.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto& clipper = clippers[static_cast<std::size_t> (yup::jmin (channel, clipperChans - 1))];
                clipper.setParameters (drive, out);
                auto* channelData = audioBuffer.getWritePointer (channel);
                channelData[sample] = clipper.processSample (channelData[sample]);
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
            return yup::Result::fail ("Unsupported blunter soft clipper node state version");

        setDrive (stream.readFloat());
        setOutput (stream.readFloat());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        return AudioProcessor::saveStateIntoMemory (data);
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDrive() const noexcept
    {
        return drive->getValue();
    }

    float getOutput() const noexcept
    {
        return output->getValue();
    }

    void setDrive (float newDrive) noexcept
    {
        drive->setValue (newDrive);
    }

    void setOutput (float newOutput) noexcept
    {
        output->setValue (newOutput);
    }

private:
    static constexpr const char* stateType = "BlunterSoftClipperState";

    yup::AudioParameter::Ptr drive;
    yup::AudioParameter::Ptr output;
    yup::SmoothedValue<float> smoothedDrive;
    yup::SmoothedValue<float> smoothedOutput;
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

//==============================================================================
class AaIirHardClipperProcessor final : public yup::AudioProcessor
{
public:
    AaIirHardClipperProcessor()
        : AudioProcessor ("AA-IIR Hard Clipper",
                          yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        drive = NodeViewHelpers::createParameter ("drive", "Drive", 0.1f, 24.0f, 1.0f, 0.1f);
        output = NodeViewHelpers::createParameter ("output", "Output", 0.0f, 1.5f, 1.0f, 0.01f);

        addParameter (drive);
        addParameter (output);
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        for (auto& clipper : clippers)
            clipper.prepare (static_cast<double> (spec.sampleRate), spec.maxBlockSize);

        smoothedDrive.reset (spec.sampleRate, 0.02);
        smoothedDrive.setCurrentAndTargetValue (getDrive());
        smoothedOutput.reset (spec.sampleRate, 0.02);
        smoothedOutput.setCurrentAndTargetValue (getOutput());
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

        const int numChannels = audioBuffer.getNumChannels();
        const int numSamples = audioBuffer.getNumSamples();

        smoothedDrive.setTargetValue (getDrive());
        smoothedOutput.setTargetValue (getOutput());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float drive = smoothedDrive.getNextValue();
            const float out = smoothedOutput.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* channelData = audioBuffer.getWritePointer (channel);
                auto& clipper = clippers[static_cast<std::size_t> (yup::jmin (channel, clipperChannels - 1))];
                channelData[sample] = clipper.processSample (channelData[sample] * drive) * out;
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
            return yup::Result::fail ("Unsupported AA-IIR hard clipper node state version");

        setDrive (stream.readFloat());
        setOutput (stream.readFloat());

        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        return AudioProcessor::saveStateIntoMemory (data);
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    float getDrive() const noexcept
    {
        return drive->getValue();
    }

    float getOutput() const noexcept
    {
        return output->getValue();
    }

    void setDrive (float newDrive) noexcept
    {
        drive->setValue (newDrive);
    }

    void setOutput (float newOutput) noexcept
    {
        output->setValue (newOutput);
    }

private:
    static constexpr const char* stateType = "AaIirHardClipperState";
    static constexpr int clipperChannels = 2;

    yup::AudioParameter::Ptr drive;
    yup::AudioParameter::Ptr output;
    yup::SmoothedValue<float> smoothedDrive;
    yup::SmoothedValue<float> smoothedOutput;
    std::array<yup::HardClipperFloat, clipperChannels> clippers;
};

//==============================================================================
class AaIirHardClipperNodeView final : public yup::AudioGraphNodeView
{
public:
    AaIirHardClipperNodeView (yup::AudioGraphNodeID nodeID, AaIirHardClipperProcessor& processorIn)
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

    yup::String getNodeTitle() const override { return "AA-IIR"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 240; }

    yup::Color getNodeColor() const override { return yup::Color (0xff8b5cf6); }

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

    AaIirHardClipperProcessor& processor;
    yup::Slider driveSlider;
    yup::Slider outputSlider;
};
