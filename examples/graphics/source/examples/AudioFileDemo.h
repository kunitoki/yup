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
#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_core/yup_core.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

/**
    Draws a multi-channel waveform with one horizontal lane per channel.
*/
class AudioFileWaveform : public yup::AudioViewComponent
{
public:
    AudioFileWaveform (std::shared_ptr<yup::AudioPeakProfileCache> cacheToUse)
        : yup::AudioViewComponent (std::move (cacheToUse))
    {
        addAndMakeVisible (playhead);
        playhead.setVisible (false);
    }

    void clear()
    {
        AudioViewComponent::clear();
        playheadSeconds = 0.0;
        lengthSeconds = 0.0;
        updatePlayheadPosition();
    }

    /** Updates the playhead without repainting the full waveform. */
    void setPlayhead (double newPlayheadSeconds, double newLengthSeconds)
    {
        playheadSeconds = newPlayheadSeconds;
        lengthSeconds = newLengthSeconds;
        updatePlayheadPosition();
    }

protected:
    void resized() override
    {
        AudioViewComponent::resized();
        updatePlayheadPosition();
    }

private:
    class PlayheadMarker : public yup::Component
    {
    public:
        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color (0xFFFFCC33));
            g.fillRect (getLocalBounds());
        }
    };

    void updatePlayheadPosition()
    {
        const double sampleRate = getSampleRate();
        if (lengthSeconds <= 0.0 || sampleRate <= 0.0 || getTotalSamples() <= 0)
        {
            playhead.setVisible (false);
            return;
        }

        const auto waveformBounds = getWaveformBounds();
        if (waveformBounds.getWidth() <= 0.0f)
        {
            playhead.setVisible (false);
            return;
        }

        const double clamped = yup::jlimit (0.0, lengthSeconds, playheadSeconds);
        const double samplePosition = clamped * sampleRate;
        const auto viewRange = getViewRangeSamples();

        if (viewRange.isEmpty()
            || samplePosition < viewRange.getStart()
            || samplePosition > viewRange.getEnd())
        {
            playhead.setVisible (false);
            return;
        }

        const float x = sampleToX (samplePosition, waveformBounds);
        const float lineWidth = 2.0f;
        playhead.setBounds (x - lineWidth * 0.5f,
                            waveformBounds.getY(),
                            lineWidth,
                            waveformBounds.getHeight());
        playhead.setVisible (true);
        playhead.repaint();
    }

    PlayheadMarker playhead;
    double playheadSeconds = 0.0;
    double lengthSeconds = 0.0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFileWaveform)
};

//==============================================================================

/**
    Wraps an AudioSource and taps the audio stream to send to a KMeterState.
*/
class MeteringAudioSource : public yup::PositionableAudioSource
{
public:
    MeteringAudioSource (yup::PositionableAudioSource* sourceToWrap, yup::KMeterState& meterState)
        : source (sourceToWrap)
        , meter (meterState)
    {
    }

    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        if (source != nullptr)
            source->prepareToPlay (samplesPerBlockExpected, newSampleRate);

        meter.prepare (newSampleRate, 2);
    }

    void releaseResources() override
    {
        if (source != nullptr)
            source->releaseResources();
    }

    void getNextAudioBlock (const yup::AudioSourceChannelInfo& bufferToFill) override
    {
        if (source != nullptr)
        {
            source->getNextAudioBlock (bufferToFill);

            // Tap the audio and push to meter
            const int numChannels = yup::jmin (bufferToFill.buffer->getNumChannels(), 2);
            if (numChannels > 0 && bufferToFill.numSamples > 0)
            {
                const float* channels[2] = { nullptr, nullptr };
                for (int i = 0; i < numChannels; ++i)
                    channels[i] = bufferToFill.buffer->getReadPointer (i, bufferToFill.startSample);

                meter.pushSamples (channels, numChannels, bufferToFill.numSamples);
                meter.processPendingAudio();
            }
        }
        else
        {
            bufferToFill.clearActiveBufferRegion();
        }
    }

    void setNextReadPosition (yup::int64 newPosition) override
    {
        if (source != nullptr)
            source->setNextReadPosition (newPosition);
    }

    yup::int64 getNextReadPosition() const override
    {
        return source != nullptr ? source->getNextReadPosition() : 0;
    }

    yup::int64 getTotalLength() const override
    {
        return source != nullptr ? source->getTotalLength() : 0;
    }

    bool isLooping() const override
    {
        return source != nullptr ? source->isLooping() : false;
    }

    void setLooping (bool shouldLoop) override
    {
        if (source != nullptr)
            source->setLooping (shouldLoop);
    }

private:
    yup::PositionableAudioSource* source;
    yup::KMeterState& meter;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeteringAudioSource)
};

//==============================================================================

/**
    Demonstrates loading, visualizing, playing, and exporting audio files.
*/
class AudioFileDemo : public yup::Component
{
public:
    AudioFileDemo()
        : Component ("AudioFileDemo")
        , waveformCache (std::make_shared<yup::AudioPeakProfileCache>())
        , loadButton ("Load Audio")
        , playButton ("Play")
        , stopButton ("Stop")
        , saveButton ("Save As")
        , loopButton ("Loop")
        , labelsButton ("Labels")
        , waveformDisplay (waveformCache)
        , meterState (48000.0, 2)
        , leftMeter (meterState, 0)
        , rightMeter (meterState, 1)
        , meteringSource (&transportSource, meterState)
    {
        formatManager.registerDefaultFormats (
            yup::AudioFormatType::all & ~yup::AudioFormatType::coreAudio);

        deviceManager.initialiseWithDefaultDevices (0, 2);
        deviceManager.addAudioCallback (&sourcePlayer);
        sourcePlayer.setSource (&meteringSource);

        // Configure K-Meters
        meterState.setScale (yup::KMeterState::Scale::k20);
        leftMeter.setShowPeakHold (true);
        rightMeter.setShowPeakHold (true);

        // Configure the waveform cache
        waveformCache->setThreadPool (&waveformThreadPool);

        setupUi();
    }

    ~AudioFileDemo() override
    {
        stopPlayback();
        transportSource.setSource (nullptr);
        meteringSource.setLooping (false);
        sourcePlayer.setSource (nullptr);
        deviceManager.removeAudioCallback (&sourcePlayer);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8);
        auto header = bounds.removeFromTop (122);

        const int buttonHeight = 28;
        const int buttonWidth = 100;
        const int smallButtonWidth = 60;
        const int mediumButtonWidth = 85;
        const int buttonMargin = 6;

        // First row of buttons
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
        buttonRow.removeFromLeft (buttonMargin);
        labelsButton.setBounds (buttonRow.removeFromLeft (buttonWidth));

        // Second row for K-scale buttons
        header.removeFromTop (4);
        auto scaleRow = header.removeFromTop (buttonHeight);
        k20Button.setBounds (scaleRow.removeFromLeft (smallButtonWidth));
        scaleRow.removeFromLeft (buttonMargin);
        k14Button.setBounds (scaleRow.removeFromLeft (smallButtonWidth));
        scaleRow.removeFromLeft (buttonMargin);
        k12Button.setBounds (scaleRow.removeFromLeft (smallButtonWidth));

        // Third row for metering standard buttons
        header.removeFromTop (4);
        auto standardRow = header.removeFromTop (buttonHeight);
        rmsButton.setBounds (standardRow.removeFromLeft (mediumButtonWidth));
        standardRow.removeFromLeft (buttonMargin);
        ituButton.setBounds (standardRow.removeFromLeft (mediumButtonWidth));
        standardRow.removeFromLeft (buttonMargin);
        ebuButton.setBounds (standardRow.removeFromLeft (mediumButtonWidth));

        // Fourth row for over counter mode buttons
        header.removeFromTop (4);
        auto modeRow = header.removeFromTop (buttonHeight);
        contiguousButton.setBounds (modeRow.removeFromLeft (mediumButtonWidth));
        modeRow.removeFromLeft (buttonMargin);
        totalButton.setBounds (modeRow.removeFromLeft (mediumButtonWidth));

        bounds.removeFromTop (6);
        infoLabel.setBounds (bounds.removeFromTop (22));
        statusLabel.setBounds (bounds.removeFromTop (22));
        bounds.removeFromTop (6);

        // Reserve space for K-Meters on the right
        const int meterWidth = 60;
        const int meterGap = 8;
        const int meterSectionWidth = (meterWidth * 2) + (meterGap * 3);

        auto meterArea = bounds.removeFromRight (meterSectionWidth);
        leftMeter.setBounds (meterArea.removeFromLeft (meterWidth));
        meterArea.removeFromLeft (meterGap);
        rightMeter.setBounds (meterArea.removeFromLeft (meterWidth));

        // Rest is for waveform
        bounds.removeFromRight (meterGap);
        waveformDisplay.setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::darkslategray));
        g.fillAll();
    }

    void refreshDisplay (double) override
    {
        if (! hasLoadedAudio)
            return;

        if (transportSource.hasStreamFinished())
            stopPlayback();

        waveformDisplay.setPlayhead (transportSource.getCurrentPosition(),
                                     audioLengthSeconds);
        updatePlaybackStatus();
    }

private:
    void setupUi()
    {
        addAndMakeVisible (leftMeter);
        addAndMakeVisible (rightMeter);

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

        addAndMakeVisible (labelsButton);
        labelsButton.setToggleState (true, yup::NotificationType::dontSendNotification);
        labelsButton.onClick = [this]
        {
            waveformDisplay.setChannelLabelsVisible (labelsButton.getToggleState());
        };

        // K-Scale selection buttons (manual radio button behavior)
        addAndMakeVisible (k20Button);
        k20Button.setButtonText ("K-20");
        k20Button.setToggleState (true, yup::NotificationType::dontSendNotification);
        k20Button.onClick = [this]
        {
            k20Button.setToggleState (true, yup::NotificationType::dontSendNotification);
            k14Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            k12Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            meterState.setScale (yup::KMeterState::Scale::k20);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        addAndMakeVisible (k14Button);
        k14Button.setButtonText ("K-14");
        k14Button.setToggleState (false, yup::NotificationType::dontSendNotification);
        k14Button.onClick = [this]
        {
            k20Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            k14Button.setToggleState (true, yup::NotificationType::dontSendNotification);
            k12Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            meterState.setScale (yup::KMeterState::Scale::k14);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        addAndMakeVisible (k12Button);
        k12Button.setButtonText ("K-12");
        k12Button.setToggleState (false, yup::NotificationType::dontSendNotification);
        k12Button.onClick = [this]
        {
            k20Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            k14Button.setToggleState (false, yup::NotificationType::dontSendNotification);
            k12Button.setToggleState (true, yup::NotificationType::dontSendNotification);
            meterState.setScale (yup::KMeterState::Scale::k12);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        // Metering standard selection buttons
        addAndMakeVisible (rmsButton);
        rmsButton.setButtonText ("RMS Flat");
        rmsButton.setToggleState (true, yup::NotificationType::dontSendNotification);
        rmsButton.onClick = [this]
        {
            rmsButton.setToggleState (true, yup::NotificationType::dontSendNotification);
            ituButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            ebuButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            meterState.setMeteringStandard (yup::KMeterState::MeteringStandard::rmsFlat);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        addAndMakeVisible (ituButton);
        ituButton.setButtonText ("ITU BS.1770-4");
        ituButton.setToggleState (false, yup::NotificationType::dontSendNotification);
        ituButton.onClick = [this]
        {
            rmsButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            ituButton.setToggleState (true, yup::NotificationType::dontSendNotification);
            ebuButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            meterState.setMeteringStandard (yup::KMeterState::MeteringStandard::ituBS1770_4);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        addAndMakeVisible (ebuButton);
        ebuButton.setButtonText ("EBU R128");
        ebuButton.setToggleState (false, yup::NotificationType::dontSendNotification);
        ebuButton.onClick = [this]
        {
            rmsButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            ituButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            ebuButton.setToggleState (true, yup::NotificationType::dontSendNotification);
            meterState.setMeteringStandard (yup::KMeterState::MeteringStandard::ebuR128);
            leftMeter.repaint();
            rightMeter.repaint();
        };

        // Over counter mode selection buttons
        addAndMakeVisible (contiguousButton);
        contiguousButton.setButtonText ("Contiguous");
        contiguousButton.setToggleState (true, yup::NotificationType::dontSendNotification);
        contiguousButton.onClick = [this]
        {
            contiguousButton.setToggleState (true, yup::NotificationType::dontSendNotification);
            totalButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            meterState.setOverCounterMode (yup::KMeterState::OverCounterMode::contiguous);
        };

        addAndMakeVisible (totalButton);
        totalButton.setButtonText ("Total");
        totalButton.setToggleState (false, yup::NotificationType::dontSendNotification);
        totalButton.onClick = [this]
        {
            contiguousButton.setToggleState (false, yup::NotificationType::dontSendNotification);
            totalButton.setToggleState (true, yup::NotificationType::dontSendNotification);
            meterState.setOverCounterMode (yup::KMeterState::OverCounterMode::total);
        };

        addAndMakeVisible (infoLabel);
        infoLabel.setText ("No audio loaded.", yup::NotificationType::dontSendNotification);
        infoLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::white);

        addAndMakeVisible (statusLabel);
        statusLabel.setText ("Choose an audio file to begin.", yup::NotificationType::dontSendNotification);
        statusLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);

        addAndMakeVisible (waveformDisplay);
        waveformDisplay.setSelectable (true);
        waveformDisplay.setChannelLabelsVisible (labelsButton.getToggleState());
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
        waveformDisplay.setPlayhead (0.0, audioLengthSeconds);
        updatePlaybackStatus();
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

        waveformDisplay.setSource (&audioBuffer, loadedSampleRate);
        waveformDisplay.setPlayhead (0.0, audioLengthSeconds);
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

    yup::ThreadPool waveformThreadPool;
    std::shared_ptr<yup::AudioPeakProfileCache> waveformCache;

    yup::TextButton loadButton;
    yup::TextButton playButton;
    yup::TextButton stopButton;
    yup::TextButton saveButton;
    yup::ToggleButton loopButton;
    yup::ToggleButton labelsButton;
    yup::ToggleButton k20Button;
    yup::ToggleButton k14Button;
    yup::ToggleButton k12Button;
    yup::ToggleButton rmsButton;
    yup::ToggleButton ituButton;
    yup::ToggleButton ebuButton;
    yup::ToggleButton contiguousButton;
    yup::ToggleButton totalButton;

    yup::Label infoLabel;
    yup::Label statusLabel;
    AudioFileWaveform waveformDisplay;

    // K-Meter components
    yup::KMeterState meterState;
    yup::KMeterComponent leftMeter;
    yup::KMeterComponent rightMeter;
    MeteringAudioSource meteringSource;

    yup::String currentFileName { "No audio loaded" };
    double loadedSampleRate = 0.0;
    double audioLengthSeconds = 0.0;
    bool hasLoadedAudio = false;
    bool loopEnabled = false;
};
