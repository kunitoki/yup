/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <memory>
#include <vector>

#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_audio_formats/yup_audio_formats.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

/**
    Draws a multi-channel waveform with one horizontal lane per channel.
*/
class AudioWaveformDisplay : public yup::Component
{
public:
    AudioWaveformDisplay()
    {
        addAndMakeVisible (playhead);
    }

    /** Assigns the buffer to render and refreshes the waveform cache. */
    void setAudioBuffer (const yup::AudioBuffer<float>* newBuffer)
    {
        audioBuffer = newBuffer;

        playhead.setLaneBounds (getWaveformBounds());

        rebuildCache();
        repaint();
    }

    /** Clears the waveform display back to its empty placeholder state. */
    void clear()
    {
        audioBuffer = nullptr;
        playheadSeconds = 0.0;
        lengthSeconds = 0.0;
        channelPeaks.clear();

        updatePlayheadBounds();

        repaint();
    }

    /** Updates the playhead marker position in seconds. */
    void setPlayhead (double newPlayheadSeconds, double newLengthSeconds)
    {
        playheadSeconds = newPlayheadSeconds;
        lengthSeconds = newLengthSeconds;

        updatePlayheadBounds();
    }

    void resized() override
    {
        rebuildCache();

        playhead.setLaneBounds (getWaveformBounds());

        updatePlayheadBounds();
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (8);
        g.setFillColor (yup::Color (0xFF101010));
        g.fillAll();

        if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        {
            g.setFillColor (yup::Colors::lightgray);
            auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f);
            g.fillFittedText ("Load an audio file to view its waveform.",
                              font,
                              bounds,
                              yup::Justification::center);
            return;
        }

        auto labelArea = bounds.removeFromLeft (labelWidth);
        auto waveformArea = bounds;
        const int numChannels = static_cast<int> (channelPeaks.size());

        if (numChannels == 0)
            return;

        const float laneHeight = waveformArea.getHeight() / static_cast<float> (numChannels);
        auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            yup::Rectangle<float> lane (waveformArea.getX(),
                                        waveformArea.getY() + laneHeight * channel,
                                        waveformArea.getWidth(),
                                        laneHeight);

            g.setFillColor (yup::Color (0xFF181818));
            g.fillRect (lane);

            g.setStrokeColor (yup::Color (0xFF2A2A2A));
            g.setStrokeWidth (1.0f);
            g.strokeRect (lane);

            auto labelBounds = labelArea.withY (lane.getY()).withHeight (lane.getHeight());
            g.setFillColor (yup::Colors::white);
            g.fillFittedText ("Ch " + yup::String (channel + 1),
                              font,
                              labelBounds,
                              yup::Justification::center);

            drawChannelWaveform (g, lane, channel);
        }
    }

private:
    class PlayheadComponent : public yup::Component
    {
    public:
        PlayheadComponent()
        {
            setOpaque (false);
        }

        void setPlayheadX (float newX)
        {
            playheadX = newX;

            updateBounds();
        }

        void setLaneBounds (const yup::Rectangle<float>& newBounds)
        {
            laneBounds = newBounds;

            updateBounds();
        }

    private:
        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color (0xFFFFCC33));
            g.fillRect (getLocalBounds());
        }

        void updateBounds()
        {
            if (laneBounds.getWidth() <= 0.0f || playheadX < 0.0f)
            {
                setVisible (false);
                return;
            }

            setVisible (true);
            const float snappedX = static_cast<float> (static_cast<int> (playheadX));
            setBounds (laneBounds.withX (laneBounds.getX() + snappedX).withWidth (1.0f).toNearestInt());

            repaint();
        }

        yup::Rectangle<float> laneBounds;
        float playheadX = -1.0f;
    };

    struct ChannelPeaks
    {
        std::vector<float> minValues;
        std::vector<float> maxValues;
    };

    yup::Rectangle<float> getWaveformBounds() const
    {
        auto bounds = getLocalBounds().reduced (8);
        bounds.removeFromLeft (labelWidth);
        return bounds;
    }

    void rebuildCache()
    {
        channelPeaks.clear();
        if (audioBuffer == nullptr)
            return;

        const int numSamples = audioBuffer->getNumSamples();
        const int numChannels = audioBuffer->getNumChannels();
        auto waveformBounds = getWaveformBounds();
        const int waveformWidth = static_cast<int> (waveformBounds.getWidth());

        if (numSamples <= 0 || numChannels <= 0 || waveformWidth <= 0)
            return;

        const int columns = yup::jmax (1, yup::jmin (waveformWidth, numSamples));
        const int samplesPerColumn = yup::jmax (1, numSamples / columns);

        channelPeaks.resize (static_cast<size_t> (numChannels));
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto& peaks = channelPeaks[static_cast<size_t> (channel)];
            peaks.minValues.assign (static_cast<size_t> (columns), 0.0f);
            peaks.maxValues.assign (static_cast<size_t> (columns), 0.0f);

            for (int column = 0; column < columns; ++column)
            {
                const int startSample = column * samplesPerColumn;
                const int endSample = (column == columns - 1)
                                        ? numSamples
                                        : yup::jmin (numSamples, startSample + samplesPerColumn);

                float minValue = 1.0f;
                float maxValue = -1.0f;

                for (int sample = startSample; sample < endSample; ++sample)
                {
                    const float value = audioBuffer->getSample (channel, sample);
                    minValue = yup::jmin (minValue, value);
                    maxValue = yup::jmax (maxValue, value);
                }

                peaks.minValues[static_cast<size_t> (column)] = minValue;
                peaks.maxValues[static_cast<size_t> (column)] = maxValue;
            }
        }
    }

    void updatePlayheadBounds()
    {
        if (lengthSeconds <= 0.0)
        {
            playhead.setPlayheadX (-1.0f);
            return;
        }

        auto waveformBounds = getWaveformBounds();
        playhead.setLaneBounds (waveformBounds);

        const double clamped = yup::jlimit (0.0, lengthSeconds, playheadSeconds);
        const float x = static_cast<float> (clamped / lengthSeconds) * waveformBounds.getWidth();
        playhead.setPlayheadX (x);
    }

    void drawChannelWaveform (yup::Graphics& g, const yup::Rectangle<float>& lane, int channelIndex)
    {
        if (channelIndex < 0 || channelIndex >= static_cast<int> (channelPeaks.size()))
            return;

        const auto& peaks = channelPeaks[static_cast<size_t> (channelIndex)];
        if (peaks.minValues.empty() || peaks.maxValues.empty())
            return;

        const float centerY = lane.getCenterY();
        const float amplitude = lane.getHeight() * 0.45f;
        const float startX = lane.getX();
        const float stepX = lane.getWidth() / static_cast<float> (peaks.minValues.size());

        g.setStrokeColor (getChannelColor (channelIndex));
        g.setStrokeWidth (1.0f);

        for (size_t i = 0; i < peaks.minValues.size(); ++i)
        {
            float x = startX + static_cast<float> (i) * stepX;
            float minValue = peaks.minValues[i];
            float maxValue = peaks.maxValues[i];

            float y1 = centerY - maxValue * amplitude;
            float y2 = centerY - minValue * amplitude;

            g.strokeLine ({ x, y1 }, { x, y2 });
        }

        g.setStrokeColor (yup::Color (0xFF3A3A3A));
        g.setStrokeWidth (1.0f);
        g.strokeLine ({ lane.getX(), centerY }, { lane.getRight(), centerY });
    }

    yup::Color getChannelColor (int channelIndex) const
    {
        static const yup::Color colors[] = {
            yup::Color (0xFF5BC0EB),
            yup::Color (0xFFFDE74C),
            yup::Color (0xFF9BC53D),
            yup::Color (0xFFE55934),
            yup::Color (0xFFFA7921),
            yup::Color (0xFF9D4EDD)
        };

        const int colorIndex = channelIndex % (static_cast<int> (sizeof (colors) / sizeof (colors[0])));
        return colors[colorIndex];
    }

    const yup::AudioBuffer<float>* audioBuffer = nullptr;
    std::vector<ChannelPeaks> channelPeaks;
    double playheadSeconds = 0.0;
    double lengthSeconds = 0.0;
    const int labelWidth = 48;
    PlayheadComponent playhead;
};

//==============================================================================

/**
    Demonstrates loading, visualizing, playing, and exporting audio files.
*/
class AudioFileDemo : public yup::Component
    , public yup::Timer
{
public:
    AudioFileDemo()
        : Component ("AudioFileDemo")
        , loadButton ("Load Audio")
        , playButton ("Play")
        , stopButton ("Stop")
        , saveButton ("Save As")
        , loopButton ("Loop")
    {
        formatManager.registerDefaultFormats (
            yup::AudioFormatType::all & ~yup::AudioFormatType::coreAudio);

        deviceManager.initialiseWithDefaultDevices (0, 2);
        deviceManager.addAudioCallback (&sourcePlayer);
        sourcePlayer.setSource (&transportSource);

        setupUi();
        startTimerHz (30);
    }

    ~AudioFileDemo() override
    {
        stopPlayback();
        transportSource.setSource (nullptr);
        sourcePlayer.setSource (nullptr);
        deviceManager.removeAudioCallback (&sourcePlayer);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        auto header = bounds.removeFromTop (38);

        const int buttonHeight = 28;
        const int buttonWidth = 110;
        const int buttonMargin = 6;

        auto buttonRow = header.removeFromTop (buttonHeight);
        loadButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (buttonMargin);
        playButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (buttonMargin);
        stopButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (buttonMargin);
        saveButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (buttonMargin);
        loopButton.setBounds (buttonRow.removeFromLeft (buttonWidth));

        bounds.removeFromTop (6);
        infoLabel.setBounds (bounds.removeFromTop (22));
        statusLabel.setBounds (bounds.removeFromTop (22));
        bounds.removeFromTop (6);

        waveformDisplay.setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::darkslategray));
        g.fillAll();
    }

private:
    void setupUi()
    {
        addAndMakeVisible (loadButton);
        loadButton.onClick = [this]
        {
            auto chooser = yup::FileChooser::create ("Load Audio File",
                                                     yup::File::getCurrentWorkingDirectory(),
                                                     getAudioFileFilter());
            chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
            {
                if (success && results.size() > 0)
                    loadAudioFile (results[0]);
                else
                    updateStatus ("Audio file selection cancelled.");
            });
        };

        addAndMakeVisible (playButton);
        playButton.onClick = [this]
        {
            togglePlayback();
        };

        addAndMakeVisible (stopButton);
        stopButton.onClick = [this]
        {
            stopPlayback();
        };

        addAndMakeVisible (saveButton);
        saveButton.onClick = [this]
        {
            saveAudioFile();
        };

        addAndMakeVisible (loopButton);
        loopButton.setToggleState (false, yup::NotificationType::dontSendNotification);
        loopButton.onClick = [this]
        {
            loopEnabled = loopButton.getToggleState();
            if (memorySource != nullptr)
                memorySource->setLooping (loopEnabled);
        };

        addAndMakeVisible (infoLabel);
        infoLabel.setText ("No audio loaded.", yup::NotificationType::dontSendNotification);
        infoLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::white);

        addAndMakeVisible (statusLabel);
        statusLabel.setText ("Choose an audio file to begin.", yup::NotificationType::dontSendNotification);
        statusLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);

        addAndMakeVisible (waveformDisplay);
    }

    void timerCallback() override
    {
        if (! hasLoadedAudio)
            return;

        if (transportSource.hasStreamFinished())
            stopPlayback();

        waveformDisplay.setPlayhead (transportSource.getCurrentPosition(),
                                     audioLengthSeconds);
        updatePlaybackStatus();
    }

    void updateStatus (const yup::String& newStatus)
    {
        statusLabel.setText (newStatus, yup::NotificationType::dontSendNotification);
    }

    void updatePlaybackStatus()
    {
        const double lengthSeconds = audioLengthSeconds;
        const double positionSeconds = transportSource.getCurrentPosition();
        yup::String positionText = formatTime (positionSeconds) + " / " + formatTime (lengthSeconds);

        if (transportSource.isPlaying())
            infoLabel.setText (currentFileName + "  |  " + positionText, yup::NotificationType::dontSendNotification);
        else
            infoLabel.setText (currentFileName + "  |  " + positionText + "  |  Stopped", yup::NotificationType::dontSendNotification);
    }

    void togglePlayback()
    {
        if (! hasLoadedAudio)
        {
            updateStatus ("Load an audio file before playback.");
            return;
        }

        if (transportSource.isPlaying())
        {
            transportSource.stop();
            playButton.setButtonText ("Play");
        }
        else
        {
            transportSource.start();
            playButton.setButtonText ("Pause");
        }
    }

    void stopPlayback()
    {
        if (! hasLoadedAudio)
            return;

        transportSource.stop();
        transportSource.setPosition (0.0);
        playButton.setButtonText ("Play");
    }

    void loadAudioFile (const yup::File& file)
    {
        if (! file.existsAsFile())
        {
            updateStatus ("File not found: " + file.getFullPathName());
            return;
        }

        auto reader = formatManager.createReaderFor (file);
        if (reader == nullptr)
        {
            updateStatus ("Unsupported or unreadable format: " + file.getFileName());
            return;
        }

        const int numChannels = reader->numChannels;
        const int numSamples = static_cast<int> (reader->lengthInSamples);
        audioBuffer.setSize (numChannels, numSamples);
        reader->read (&audioBuffer, 0, numSamples, 0, true, true);

        loadedSampleRate = reader->sampleRate;
        currentFileName = file.getFileName();
        hasLoadedAudio = true;
        audioLengthSeconds = loadedSampleRate > 0.0
                               ? static_cast<double> (numSamples) / loadedSampleRate
                               : 0.0;

        transportSource.stop();
        transportSource.setSource (nullptr);
        memorySource = std::make_unique<yup::MemoryAudioSource> (audioBuffer, false, loopEnabled);
        transportSource.setSource (memorySource.get(), 0, nullptr, loadedSampleRate, numChannels);

        waveformDisplay.setAudioBuffer (&audioBuffer);
        updateStatus ("Loaded " + file.getFileName() + " | " + yup::String (numChannels)
                      + " ch | " + yup::String (loadedSampleRate, 1) + " Hz | "
                      + formatTime (audioLengthSeconds));
        updatePlaybackStatus();
    }

    void saveAudioFile()
    {
        if (! hasLoadedAudio)
        {
            updateStatus ("Load an audio file before saving.");
            return;
        }

        auto chooser = yup::FileChooser::create ("Save Audio File",
                                                 yup::File::getCurrentWorkingDirectory(),
                                                 getAudioFileFilter());
        chooser->browseForFileToSave ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
            {
                updateStatus ("Save cancelled.");
                return;
            }

            auto destination = results[0];
            if (destination.getFileExtension().isEmpty())
                destination = destination.withFileExtension (".wav");

            yup::String status = "Unable to find a valid format to save.";

            for (const int bitsPerSample : { 16, 32 })
            {
                auto writer = formatManager.createWriterFor (destination,
                                                             static_cast<int> (loadedSampleRate),
                                                             audioBuffer.getNumChannels(),
                                                             bitsPerSample);

                if (writer == nullptr)
                {
                    status = "Failed to create writer for " + destination.getFileName();
                    continue;
                }

                if (! writer->writeFromAudioSampleBuffer (audioBuffer, 0, audioBuffer.getNumSamples()))
                {
                    status = "Failed to write audio data.";
                    continue;
                }

                status = "Saved: " + destination.getFileName();
                break;
            }

            updateStatus (status);
        },
                                      true);
    }

    yup::String formatTime (double seconds) const
    {
        if (seconds <= 0.0)
            return "0:00";

        const int totalSeconds = static_cast<int> (seconds);
        const int minutes = totalSeconds / 60;
        const int remainingSeconds = totalSeconds % 60;
        return yup::String (minutes) + ":" + yup::String::formatted ("%02d", remainingSeconds);
    }

    yup::String getAudioFileFilter() const
    {
        return "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.opus;*.m4a;*.wma;*.ogg";
    }

    yup::AudioFormatManager formatManager;
    yup::AudioDeviceManager deviceManager;
    yup::AudioSourcePlayer sourcePlayer;
    yup::AudioTransportSource transportSource;
    std::unique_ptr<yup::MemoryAudioSource> memorySource;
    yup::AudioBuffer<float> audioBuffer;

    yup::TextButton loadButton;
    yup::TextButton playButton;
    yup::TextButton stopButton;
    yup::TextButton saveButton;
    yup::ToggleButton loopButton;

    yup::Label infoLabel;
    yup::Label statusLabel;
    AudioWaveformDisplay waveformDisplay;

    yup::String currentFileName { "No audio loaded" };
    double loadedSampleRate = 0.0;
    double audioLengthSeconds = 0.0;
    bool hasLoadedAudio = false;
    bool loopEnabled = false;
};
