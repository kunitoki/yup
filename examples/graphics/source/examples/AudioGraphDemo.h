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

#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_gui/yup_audio_gui.h>

//==============================================================================
class AudioGraphDemoProcessor : public yup::AudioProcessor
{
public:
    AudioGraphDemoProcessor (yup::StringRef name, yup::AudioBusLayout layout)
        : AudioProcessor (name, std::move (layout))
    {
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    yup::Result loadStateFromMemory (const yup::MemoryBlock&) override { return yup::Result::ok(); }

    yup::Result saveStateIntoMemory (yup::MemoryBlock&) override { return yup::Result::ok(); }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }
};

class OscillatorProcessor final : public AudioGraphDemoProcessor
{
public:
    OscillatorProcessor()
        : AudioGraphDemoProcessor ("Oscillator",
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

class GainProcessor final : public AudioGraphDemoProcessor
{
public:
    GainProcessor()
        : AudioGraphDemoProcessor ("Gain",
                                   yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                                        { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
    }

    void prepareToPlay (float, int) override {}

    void releaseResources() override {}

    void processBlock (yup::AudioBuffer<float>& audioBuffer, yup::MidiBuffer&) override
    {
        audioBuffer.applyGain (gain.load (std::memory_order_relaxed));
    }

    float getGain() const noexcept { return gain.load (std::memory_order_relaxed); }

    void setGain (float newGain) noexcept
    {
        gain.store (yup::jlimit (0.0f, 1.5f, newGain), std::memory_order_relaxed);
    }

private:
    std::atomic<float> gain { 0.75f };
};

class LowPassFilterProcessor final : public AudioGraphDemoProcessor
{
public:
    LowPassFilterProcessor()
        : AudioGraphDemoProcessor ("Low-pass",
                                   yup::AudioBusLayout ({ yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                                                        { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
    }

    void prepareToPlay (float newSampleRate, int) override
    {
        sampleRate = newSampleRate;
        state[0] = 0.0f;
        state[1] = 0.0f;
    }

    void releaseResources() override {}

    void processBlock (yup::AudioBuffer<float>& audioBuffer, yup::MidiBuffer&) override
    {
        const auto currentCutoff = static_cast<double> (cutoff.load (std::memory_order_relaxed));
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

    double getCutoff() const noexcept { return static_cast<double> (cutoff.load (std::memory_order_relaxed)); }

    void setCutoff (double newCutoff) noexcept
    {
        cutoff.store (static_cast<float> (yup::jlimit (80.0, 12000.0, newCutoff)), std::memory_order_relaxed);
    }

private:
    float sampleRate = 44100.0f;
    std::atomic<float> cutoff { 800.0f };
    float state[2] {};
};

namespace
{
void configureParameterSlider (yup::Slider& slider, yup::Color accent)
{
    slider.setSliderType (yup::Slider::LinearBarHorizontal);
    slider.setTextBoxStyle (yup::Slider::NoTextBox);
    slider.setColor (yup::Slider::Style::backgroundColorId, yup::Color (0xff26282c));
    slider.setColor (yup::Slider::Style::trackColorId, accent.withAlpha (0.65f));
    slider.setColor (yup::Slider::Style::thumbColorId, accent);
    slider.setColor (yup::Slider::Style::thumbOverColorId, accent.brighter (0.15f));
    slider.setColor (yup::Slider::Style::thumbDownColorId, accent.darker (0.15f));
}

yup::Rectangle<float> getInlineSliderBounds (const yup::Component& component, int preferredWidth)
{
    const auto bounds = component.getLocalBounds();
    const auto scale = bounds.getWidth() / static_cast<float> (preferredWidth);
    return { 62.0f * scale,
             49.0f * scale,
             yup::jmax (42.0f * scale, bounds.getWidth() - (150.0f * scale)),
             20.0f * scale };
}
} // namespace

//==============================================================================
class OscillatorNodeView final : public yup::AudioGraphNodeView
{
public:
    OscillatorNodeView (yup::AudioGraphNodeID nodeID, OscillatorProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , frequencySlider (yup::Slider::LinearBarHorizontal)
    {
        configureParameterSlider (frequencySlider, getPortKindColor (PortKind::parameter));
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
        frequencySlider.setBounds (getInlineSliderBounds (*this, getPreferredWidth()));
    }

private:
    OscillatorProcessor& processor;
    yup::Slider frequencySlider;
};

class GainNodeView final : public yup::AudioGraphNodeView
{
public:
    GainNodeView (yup::AudioGraphNodeID nodeID, GainProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , gainSlider (yup::Slider::LinearBarHorizontal)
    {
        configureParameterSlider (gainSlider, getPortKindColor (PortKind::parameter));
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
        gainSlider.setBounds (getInlineSliderBounds (*this, getPreferredWidth()));
    }

private:
    GainProcessor& processor;
    yup::Slider gainSlider;
};

class LowPassFilterNodeView final : public yup::AudioGraphNodeView
{
public:
    LowPassFilterNodeView (yup::AudioGraphNodeID nodeID, LowPassFilterProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , cutoffSlider (yup::Slider::LinearBarHorizontal)
    {
        configureParameterSlider (cutoffSlider, getPortKindColor (PortKind::parameter));
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
        cutoffSlider.setBounds (getInlineSliderBounds (*this, getPreferredWidth()));
    }

private:
    LowPassFilterProcessor& processor;
    yup::Slider cutoffSlider;
};

class SoundCardInputNodeView final : public yup::AudioGraphNodeView
{
public:
    SoundCardInputNodeView()
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
    {
    }

    yup::String getNodeTitle() const override { return "INPUT"; }

    yup::String getNodeSubtitle() const override { return "sound card"; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 150; }

    yup::Color getNodeColor() const override { return yup::Color (0xfff97316); }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }
};

class SoundCardOutputNodeView final : public yup::AudioGraphNodeView
{
public:
    SoundCardOutputNodeView()
        : AudioGraphNodeView (yup::AudioGraphNodeID::invalid())
    {
    }

    yup::String getNodeTitle() const override { return "OUTPUT"; }

    yup::String getNodeSubtitle() const override { return "sound card"; }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 0; }

    int getPreferredWidth() const override { return 150; }

    yup::Color getNodeColor() const override { return yup::Color (0xff06b6d4); }

    PortInfo getInputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }
};

//==============================================================================
class AudioGraphDemo final
    : public yup::Component
    , public yup::AudioIODeviceCallback
{
public:
    AudioGraphDemo()
    {
        deviceManager.initialiseWithDefaultDevices (2, 2);

        graph = std::make_shared<yup::AudioGraphProcessor>();

        auto oscillator = std::make_unique<OscillatorProcessor>();
        auto* oscillatorProcessor = oscillator.get();
        const auto oscillatorID = graph->addNode (std::move (oscillator));

        auto gain = std::make_unique<GainProcessor>();
        auto* gainProcessor = gain.get();
        const auto gainID = graph->addNode (std::move (gain));

        auto filter = std::make_unique<LowPassFilterProcessor>();
        auto* filterProcessor = filter.get();
        const auto filterID = graph->addNode (std::move (filter));

        graph->addConnection ({ yup::AudioGraphEndpoint::graphInput (0),
                                yup::AudioGraphEndpoint::nodeInput (gainID, 0) });
        graph->addConnection ({ yup::AudioGraphEndpoint::nodeOutput (oscillatorID, 0),
                                yup::AudioGraphEndpoint::nodeInput (gainID, 0) });
        graph->addConnection ({ yup::AudioGraphEndpoint::nodeOutput (gainID, 0),
                                yup::AudioGraphEndpoint::nodeInput (filterID, 0) });
        graph->addConnection ({ yup::AudioGraphEndpoint::nodeOutput (filterID, 0),
                                yup::AudioGraphEndpoint::graphOutput (0) });
        graph->commitChanges();

        graphComponent = std::make_unique<yup::AudioGraphComponent> (graph);
        addAndMakeVisible (*graphComponent);

        graphComponent->setGraphInputView (std::make_unique<SoundCardInputNodeView>(), { 40.0f, 210.0f });
        graphComponent->addNodeView (oscillatorID, std::make_unique<OscillatorNodeView> (oscillatorID, *oscillatorProcessor), { 70.0f, 80.0f });
        graphComponent->addNodeView (gainID, std::make_unique<GainNodeView> (gainID, *gainProcessor), { 300.0f, 145.0f });
        graphComponent->addNodeView (filterID, std::make_unique<LowPassFilterNodeView> (filterID, *filterProcessor), { 530.0f, 145.0f });
        graphComponent->setGraphOutputView (std::make_unique<SoundCardOutputNodeView>(), { 760.0f, 145.0f });
        graphComponent->zoomToFitNodes();
    }

    ~AudioGraphDemo() override
    {
        removeAudioCallback();
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        if (graphComponent != nullptr)
            graphComponent->setBounds (getLocalBounds());
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::Color (0xff101522));
        g.fillAll();
    }

    void visibilityChanged() override
    {
        if (isVisible())
            addAudioCallback();
        else
            removeAudioCallback();
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (outputChannelData[channel] != nullptr)
                yup::FloatVectorOperations::clear (outputChannelData[channel], numSamples);
        }

        if (graph == nullptr || numOutputChannels <= 0 || numSamples <= 0)
            return;

        yup::AudioBuffer<float> outputBuffer (outputChannelData, numOutputChannels, numSamples);
        const auto channelsToCopy = yup::jmin (numInputChannels, numOutputChannels);

        for (int channel = 0; channel < channelsToCopy; ++channel)
        {
            if (inputChannelData != nullptr && inputChannelData[channel] != nullptr && outputChannelData[channel] != nullptr)
                yup::FloatVectorOperations::copy (outputChannelData[channel], inputChannelData[channel], numSamples);
        }

        yup::MidiBuffer midi;
        graph->processBlock (outputBuffer, midi);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        if (graph != nullptr && device != nullptr)
            graph->prepareToPlay (static_cast<float> (device->getCurrentSampleRate()), device->getCurrentBufferSizeSamples());
    }

    void audioDeviceStopped() override
    {
        if (graph != nullptr)
            graph->releaseResources();
    }

private:
    void addAudioCallback()
    {
        if (! audioCallbackRegistered)
        {
            deviceManager.addAudioCallback (this);
            audioCallbackRegistered = true;
        }
    }

    void removeAudioCallback()
    {
        if (audioCallbackRegistered)
        {
            deviceManager.removeAudioCallback (this);
            audioCallbackRegistered = false;
        }
    }

    yup::AudioDeviceManager deviceManager;
    std::shared_ptr<yup::AudioGraphProcessor> graph;
    std::unique_ptr<yup::AudioGraphComponent> graphComponent;
    bool audioCallbackRegistered = false;
};
