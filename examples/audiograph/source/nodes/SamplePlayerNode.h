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
#include <limits>
#include <memory>
#include <vector>

#include "NodeViewHelpers.h"

//==============================================================================
class SamplePlayerProcessor final : public yup::AudioProcessor
{
public:
    struct SampleData
    {
        yup::AudioBuffer<float> buffer;
        double sampleRate = 44100.0;
        yup::File file;
    };

    SamplePlayerProcessor()
        : AudioProcessor ("Sample Player",
                          yup::AudioBusLayout ({},
                                               { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        playbackSampleRate.store (spec.sampleRate > 0.0f ? spec.sampleRate : 44100.0f, std::memory_order_relaxed);
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        audioBuffer.clear();

        const auto* sample = currentSample.load (std::memory_order_acquire);
        if (sample == nullptr || sample->buffer.getNumSamples() <= 0 || sample->buffer.getNumChannels() <= 0)
            return;

        const auto& source = sample->buffer;
        const int sourceNumSamples = source.getNumSamples();
        const int sourceNumChannels = source.getNumChannels();
        const double outputSampleRate = static_cast<double> (playbackSampleRate.load (std::memory_order_relaxed));
        const double sourceSampleRate = sample->sampleRate > 0.0 ? sample->sampleRate : outputSampleRate;
        const double positionIncrement = sourceSampleRate / yup::jmax (1.0, outputSampleRate);

        double position = playbackPosition.load (std::memory_order_relaxed);

        while (position >= static_cast<double> (sourceNumSamples))
            position -= static_cast<double> (sourceNumSamples);

        for (int sampleIndex = 0; sampleIndex < audioBuffer.getNumSamples(); ++sampleIndex)
        {
            const int index0 = yup::jlimit (0, sourceNumSamples - 1, static_cast<int> (position));
            const int index1 = (index0 + 1) < sourceNumSamples ? index0 + 1 : 0;
            const float alpha = static_cast<float> (position - static_cast<double> (index0));

            for (int channel = 0; channel < audioBuffer.getNumChannels(); ++channel)
            {
                const int sourceChannel = yup::jmin (channel, sourceNumChannels - 1);
                const float s0 = source.getSample (sourceChannel, index0);
                const float s1 = source.getSample (sourceChannel, index1);
                audioBuffer.setSample (channel, sampleIndex, s0 + (s1 - s0) * alpha);
            }

            position += positionIncrement;
            while (position >= static_cast<double> (sourceNumSamples))
                position -= static_cast<double> (sourceNumSamples);
        }

        playbackPosition.store (position, std::memory_order_relaxed);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        if (! state.isValid() || state.getType() != stateType)
            return yup::Result::fail ("Invalid sample player state");

        if (static_cast<int> (state.getProperty ("version", 0)) != 1)
            return yup::Result::fail ("Unsupported sample player state version");

        const auto path = state.getProperty ("path", {}).toString();
        if (path.isEmpty())
            return yup::Result::ok();

        const auto result = loadSampleFile (yup::File (path));
        return result.wasOk() ? result : yup::Result::ok();
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = yup::DataTree (stateType);
        auto transaction = state.beginTransaction();
        transaction.setProperty ("version", 1);
        transaction.setProperty ("path", getSampleFile().getFullPathName());
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::Result loadSampleFile (const yup::File& file)
    {
        if (! file.existsAsFile())
            return yup::Result::fail ("File not found: " + file.getFullPathName());

        yup::AudioFormatManager formatManager;
        formatManager.registerDefaultFormats();

        auto reader = formatManager.createReaderFor (file);
        if (reader == nullptr)
            return yup::Result::fail ("Unsupported or unreadable format: " + file.getFileName());

        if (reader->numChannels <= 0 || reader->lengthInSamples <= 0)
            return yup::Result::fail ("The selected file contains no audio.");

        if (reader->lengthInSamples > static_cast<yup::int64> (std::numeric_limits<int>::max()))
            return yup::Result::fail ("The selected file is too long to load into memory.");

        auto data = std::make_shared<SampleData>();
        data->sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
        data->file = file;
        data->buffer.setSize (reader->numChannels, static_cast<int> (reader->lengthInSamples));

        if (! reader->read (&data->buffer, 0, data->buffer.getNumSamples(), 0, true, true))
            return yup::Result::fail ("Could not read audio from: " + file.getFileName());

        auto retainedSample = std::shared_ptr<const SampleData> (std::move (data));
        currentSampleForUi = retainedSample;
        retainedSamples.push_back (std::move (retainedSample));
        currentSample.store (currentSampleForUi.get(), std::memory_order_release);

        playbackPosition.store (0.0, std::memory_order_relaxed);
        return yup::Result::ok();
    }

    std::shared_ptr<const SampleData> getCurrentSample() const
    {
        return currentSampleForUi;
    }

    yup::File getSampleFile() const
    {
        const auto sample = getCurrentSample();
        return sample != nullptr ? sample->file : yup::File();
    }

    double getPlaybackPositionSamples() const noexcept
    {
        return playbackPosition.load (std::memory_order_relaxed);
    }

private:
    const yup::Identifier stateType = "SamplePlayerState";

    std::atomic<const SampleData*> currentSample { nullptr };
    std::shared_ptr<const SampleData> currentSampleForUi;
    std::vector<std::shared_ptr<const SampleData>> retainedSamples;
    std::atomic<double> playbackPosition { 0.0 };
    std::atomic<float> playbackSampleRate { 44100.0f };
};

//==============================================================================
class SampleWaveformThumbnail final : public yup::AudioThumbnail
{
public:
    explicit SampleWaveformThumbnail (yup::Color waveformColorIn)
        : waveformColor (waveformColorIn)
    {
    }

    yup::Color getChannelColor (int) const override
    {
        return waveformColor;
    }

private:
    yup::Color waveformColor;
};

//==============================================================================
class SampleWaveformComponent final
    : public yup::Component
    , public yup::AudioThumbnail::Listener
    , public yup::Timer
{
public:
    SampleWaveformComponent (SamplePlayerProcessor& processorIn, yup::Color waveformColor)
        : processor (processorIn)
        , thumbnail (waveformColor)
        , accentColor (waveformColor)
        , playheadColor (yup::Colors::yellow)
    {
        thumbnail.addListener (this);
        setOpaque (false);
    }

    ~SampleWaveformComponent() override
    {
        stopTimer();
        thumbnail.removeListener (this);
    }

    void setSample (std::shared_ptr<const SamplePlayerProcessor::SampleData> sample)
    {
        currentSample = std::move (sample);

        if (currentSample == nullptr)
        {
            thumbnail.clear();
            stopTimer();
            repaint();
            return;
        }

        thumbnail.setSource (currentSample->buffer, currentSample->sampleRate);
        startTimerHz (30);
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();

        if (currentSample == nullptr || thumbnail.getTotalSamples() <= 0)
        {
            g.setFillColor (accentColor.withAlpha (0.58f));
            g.fillFittedText ("Load sample", yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f), bounds.reduced (6.0f), yup::Justification::center);
            return;
        }

        const int numChannels = yup::jmin (2, thumbnail.getNumChannels());
        const auto sampleRange = yup::Range<double> (0.0, static_cast<double> (thumbnail.getTotalSamples()));
        const auto laneBounds = bounds.reduced (5.0f, 3.0f);
        const float laneHeight = laneBounds.getHeight() / static_cast<float> (numChannels);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto lane = laneBounds.withY (laneBounds.getY() + static_cast<float> (channel) * laneHeight)
                                  .withHeight (laneHeight - 2.0f);

            g.setFillColor (accentColor.withAlpha (0.12f));
            g.fillRect ({ lane.getX(), lane.getCenterY(), lane.getWidth(), 1.0f });

            thumbnail.paintChannel (g, lane, channel, sampleRange, lane.getWidth());
        }

        const auto playheadX = yup::jlimit (laneBounds.getX(),
                                            laneBounds.getRight() - 1.5f,
                                            laneBounds.getX()
                                                + laneBounds.getWidth()
                                                      * static_cast<float> (yup::jlimit (0.0,
                                                                                         1.0,
                                                                                         processor.getPlaybackPositionSamples()
                                                                                             / static_cast<double> (thumbnail.getTotalSamples()))));

        g.setFillColor (playheadColor.withAlpha (0.95f));
        g.fillRect ({ playheadX, laneBounds.getY(), 1.5f, laneBounds.getHeight() });

        if (thumbnail.isProgressVisible())
        {
            const auto progressBounds = bounds.withY (bounds.getBottom() - 3.0f).withHeight (3.0f);
            g.setFillColor (playheadColor.withAlpha (0.80f));
            g.fillRect (progressBounds.withWidth (progressBounds.getWidth() * static_cast<float> (thumbnail.getProgress())));
        }
    }

private:
    void thumbnailChanged (yup::AudioThumbnail&) override { repaint(); }

    void thumbnailProgressChanged (yup::AudioThumbnail&, double, bool) override { repaint(); }

    void timerCallback() override { repaint(); }

    SamplePlayerProcessor& processor;
    SampleWaveformThumbnail thumbnail;
    yup::Color accentColor, playheadColor;
    std::shared_ptr<const SamplePlayerProcessor::SampleData> currentSample;
};

//==============================================================================
class SamplePlayerNodeView final : public yup::AudioGraphNodeView
{
public:
    SamplePlayerNodeView (yup::AudioGraphNodeID nodeID, SamplePlayerProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , waveform (processorIn, getNodeColor())
    {
        setColor (Style::parameterBackgroundColorId, yup::Color (0x00000000));
        setColor (Style::parameterValueBackgroundColorId, yup::Color (0x00000000));

        loadButton.setButtonText ("Load");
        loadButton.onClick = [this]
        {
            chooseSampleFile();
        };
        addAndMakeVisible (loadButton);

        waveform.setSample (processor.getCurrentSample());
        addAndMakeVisible (waveform);
    }

    yup::String getNodeTitle() const override { return "SAMPLE"; }

    int getNumInputPorts() const override { return 0; }

    int getNumOutputPorts() const override { return 1; }

    int getPreferredWidth() const override { return 260; }

    yup::Color getNodeColor() const override { return yup::Color (0xff38bdf8); }

    yup::String getNodeSubtitle() const override
    {
        if (loadError.isNotEmpty())
            return loadError;

        const auto sample = processor.getCurrentSample();
        return sample != nullptr ? sample->file.getFileName() : "loop player";
    }

    PortInfo getOutputPortInfo (int) const override { return { "audio", getPortKindColor (PortKind::audio), PortKind::audio }; }

    int getNumParameterRows() const override { return 3; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { {}, {}, getNodeColor(), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        const auto scale = getLocalBounds().getWidth() / static_cast<float> (getPreferredWidth());
        auto body = getLocalBounds().to<float>().reduced (14.0f * scale, 0.0f);

        loadButton.setBounds (body.getX() + 8.0f * scale,
                              43.0f * scale,
                              62.0f * scale,
                              20.0f * scale);

        waveform.setBounds (body.getX() + 8.0f * scale,
                            67.0f * scale,
                            body.getWidth() - 16.0f * scale,
                            52.0f * scale);
    }

private:
    void chooseSampleFile()
    {
        fileChooser = yup::FileChooser::create (
            "Load Sample",
            processor.getSampleFile().existsAsFile()
                ? processor.getSampleFile().getParentDirectory()
                : yup::File::getSpecialLocation (yup::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

        yup::WeakReference<SamplePlayerNodeView> weakThis (this);

        fileChooser->browseForFileToOpen ([weakThis] (bool success, const yup::Array<yup::File>& results)
        {
            auto* self = weakThis.get();
            if (self == nullptr || ! success || results.isEmpty())
                return;

            self->loadSample (results[0]);
        });
    }

    void loadSample (const yup::File& file)
    {
        const auto result = processor.loadSampleFile (file);

        if (result.wasOk())
        {
            loadError.clear();
            loadButton.setButtonText ("Load");
            waveform.setSample (processor.getCurrentSample());
            repaint();
            return;
        }

        loadError = result.getErrorMessage();
        loadButton.setButtonText ("Retry");
        repaint();
    }

    SamplePlayerProcessor& processor;
    yup::TextButton loadButton;
    SampleWaveformComponent waveform;
    yup::FileChooser::Ptr fileChooser;
    yup::String loadError;

    YUP_DECLARE_WEAK_REFERENCEABLE (SamplePlayerNodeView)
};
