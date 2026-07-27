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
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "NodeViewHelpers.h"

//==============================================================================
/** Shared recording state accessed by both the background drain thread and the UI thread. */
struct RecordingData
{
    static constexpr int peakWindowSize = 512;

    std::mutex mutex;

    std::vector<float> samples[2];
    std::vector<std::pair<float, float>> peaks[2];
    float peakMin[2] = {};
    float peakMax[2] = {};
    int peakAccumCount = 0;
    double sampleRate = 44100.0;

    std::atomic<int> totalFrames { 0 };

    void reset()
    {
        std::lock_guard lock (mutex);

        for (int ch = 0; ch < 2; ++ch)
        {
            samples[ch].clear();
            peaks[ch].clear();
            peakMin[ch] = 0.0f;
            peakMax[ch] = 0.0f;
        }

        peakAccumCount = 0;
        totalFrames.store (0, std::memory_order_relaxed);
    }
};

//==============================================================================
class RecorderProcessor final : public yup::AudioProcessor
{
public:
    /** Ring buffer capacity: 65536 stereo frames (~1.37 s at 48 kHz). */
    static constexpr int fifoCapacity = 131072;
    static constexpr int fadeSamples = 512;
    static constexpr float fadeStep = 1.0f / static_cast<float> (fadeSamples);

    RecorderProcessor()
        : AudioProcessor ("Recorder",
                          yup::AudioBusLayout (
                              { yup::AudioBus ("Main", yup::AudioBus::Audio, yup::AudioBus::Input, 2) },
                              {}))
        , fifo (fifoCapacity)
    {
        fifoBuffer.resize (fifoCapacity);
    }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        currentSampleRate.store (spec.sampleRate > 0.0f ? spec.sampleRate : 44100.0f, std::memory_order_relaxed);
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        const int numFrames = audioBuffer.getNumSamples();
        if (numFrames == 0)
            return;

        const int cmd = recordCommand.exchange (0, std::memory_order_acq_rel);
        if (cmd == 1 && currentState == State::Idle)
        {
            currentState = State::FadingIn;
            fadeGain = 0.0f;
        }
        else if (cmd == 2 && currentState != State::Idle && currentState != State::FadingOut)
        {
            currentState = State::FadingOut;
        }

        if (currentState == State::Idle)
            return;

        const int evenAvailable = yup::jmin (numFrames * 2, fifo.getFreeSpace()) & ~1;
        if (evenAvailable < 2)
            return;

        auto scope = fifo.write (evenAvailable);
        int frameIdx = 0;

        auto writeFrame = [&] (int bufIdx)
        {
            float gain = 0.0f;

            if (currentState == State::FadingIn)
            {
                gain = fadeGain;
                fadeGain += fadeStep;
                if (fadeGain >= 1.0f)
                {
                    fadeGain = 1.0f;
                    currentState = State::Recording;
                }
            }
            else if (currentState == State::Recording)
            {
                gain = 1.0f;
            }
            else if (currentState == State::FadingOut)
            {
                gain = fadeGain;
                fadeGain -= fadeStep;
                if (fadeGain <= 0.0f)
                {
                    fadeGain = 0.0f;
                    currentState = State::Idle;
                }
            }

            const int nc = audioBuffer.getNumChannels();
            const float l = nc > 0 ? audioBuffer.getSample (0, frameIdx) * gain : 0.0f;
            const float r = nc > 1 ? audioBuffer.getSample (1, frameIdx) * gain : l;
            fifoBuffer[bufIdx] = l;
            fifoBuffer[bufIdx + 1] = r;
            ++frameIdx;
        };

        for (int i = 0; i < scope.blockSize1; i += 2)
            writeFrame (scope.startIndex1 + i);

        for (int i = 0; i < scope.blockSize2; i += 2)
            writeFrame (scope.startIndex2 + i);
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    bool supportsDataTreeState() const noexcept override { return true; }

    yup::Result loadStateFromDataTree (const yup::DataTree& state) override
    {
        if (! state.isValid() || state.getType().toString() != stateType)
            return yup::Result::fail ("Invalid node state");

        if (static_cast<int> (state.getProperty ("version", 0)) != 1)
            return yup::Result::fail ("Unsupported node state version");

        return yup::Result::ok();
    }

    yup::Result saveStateIntoDataTree (yup::DataTree& state) override
    {
        state = yup::DataTree (stateType);
        auto transaction = state.beginTransaction();
        transaction.setProperty ("version", 1);
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    void startRecording() noexcept { recordCommand.store (1, std::memory_order_release); }

    void stopRecording() noexcept { recordCommand.store (2, std::memory_order_release); }

    float getSampleRate() const noexcept { return currentSampleRate.load (std::memory_order_relaxed); }

    /** Drains available frames from the ring buffer into the provided per-channel vectors.
        Safe to call from a single background thread concurrently with processBlock. */
    int drain (std::vector<float>& leftOut, std::vector<float>& rightOut)
    {
        const int ready = fifo.getNumReady() & ~1;
        if (ready <= 0)
            return 0;

        const auto scope = fifo.read (ready);
        int framesRead = 0;

        for (int i = 0; i < scope.blockSize1; i += 2)
        {
            leftOut.push_back (fifoBuffer[scope.startIndex1 + i]);
            rightOut.push_back (fifoBuffer[scope.startIndex1 + i + 1]);
            ++framesRead;
        }

        for (int i = 0; i < scope.blockSize2; i += 2)
        {
            leftOut.push_back (fifoBuffer[scope.startIndex2 + i]);
            rightOut.push_back (fifoBuffer[scope.startIndex2 + i + 1]);
            ++framesRead;
        }

        return framesRead;
    }

private:
    static constexpr const char* stateType = "RecorderState";

    enum class State
    {
        Idle,
        FadingIn,
        Recording,
        FadingOut
    };

    State currentState = State::Idle;
    float fadeGain = 0.0f;

    std::atomic<int> recordCommand { 0 };
    std::atomic<float> currentSampleRate { 44100.0f };

    yup::AbstractFifo fifo;
    std::vector<float> fifoBuffer;
};

//==============================================================================
/** Background thread that drains the processor ring buffer and appends to RecordingData. */
class RecordingEngine
{
public:
    RecordingEngine (RecorderProcessor& processorIn, RecordingData& dataIn, yup::AsyncUpdater& updaterIn)
        : processor (processorIn)
        , data (dataIn)
        , updater (updaterIn)
    {
    }

    ~RecordingEngine() { stop(); }

    void start()
    {
        shouldStop.store (false, std::memory_order_release);
        thread = std::thread ([this]
        {
            run();
        });
    }

    void stop()
    {
        shouldStop.store (true, std::memory_order_release);
        cv.notify_all();

        if (thread.joinable())
            thread.join();
    }

    void wakeUp() { cv.notify_all(); }

private:
    void run()
    {
        while (! shouldStop.load (std::memory_order_acquire))
        {
            drainOnce();

            std::unique_lock lock (cvMutex);
            cv.wait_for (lock, std::chrono::milliseconds (5), [this]
            {
                return shouldStop.load (std::memory_order_acquire);
            });
        }

        drainOnce(); // final drain after stop signal
    }

    void drainOnce()
    {
        tempLeft.clear();
        tempRight.clear();

        const int framesRead = processor.drain (tempLeft, tempRight);
        if (framesRead == 0)
            return;

        {
            std::lock_guard lock (data.mutex);

            data.samples[0].insert (data.samples[0].end(), tempLeft.begin(), tempLeft.end());
            data.samples[1].insert (data.samples[1].end(), tempRight.begin(), tempRight.end());

            for (int i = 0; i < framesRead; ++i)
            {
                const float sl = tempLeft[static_cast<size_t> (i)];
                const float sr = tempRight[static_cast<size_t> (i)];

                data.peakMin[0] = yup::jmin (data.peakMin[0], sl);
                data.peakMax[0] = yup::jmax (data.peakMax[0], sl);
                data.peakMin[1] = yup::jmin (data.peakMin[1], sr);
                data.peakMax[1] = yup::jmax (data.peakMax[1], sr);

                if (++data.peakAccumCount >= RecordingData::peakWindowSize)
                {
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        data.peaks[ch].emplace_back (data.peakMin[ch], data.peakMax[ch]);
                        data.peakMin[ch] = 0.0f;
                        data.peakMax[ch] = 0.0f;
                    }

                    data.peakAccumCount = 0;
                }
            }

            data.totalFrames.store (static_cast<int> (data.samples[0].size()), std::memory_order_relaxed);
        }

        updater.triggerAsyncUpdate();
    }

    RecorderProcessor& processor;
    RecordingData& data;
    yup::AsyncUpdater& updater;

    std::thread thread;
    std::atomic<bool> shouldStop { true };
    std::mutex cvMutex;
    std::condition_variable cv;

    std::vector<float> tempLeft, tempRight;
};

//==============================================================================
/** Draws an accumulating stereo waveform from a RecordingData peak array. */
class RecordingWaveformComponent final : public yup::Component
{
public:
    RecordingWaveformComponent (RecordingData& dataIn, yup::Color accentColor)
        : data (dataIn)
        , color (accentColor)
    {
        setOpaque (false);
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();

        std::vector<std::pair<float, float>> snapPeaks[2];
        {
            std::lock_guard lock (data.mutex);

            for (int ch = 0; ch < 2; ++ch)
                snapPeaks[ch] = data.peaks[ch];
        }

        if (snapPeaks[0].empty())
        {
            g.setFillColor (color.withAlpha (0.35f));
            g.fillFittedText ("No recording",
                              yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (9.0f),
                              bounds.reduced (6.0f),
                              yup::Justification::center);
            return;
        }

        const auto laneBounds = bounds.reduced (5.0f, 3.0f);
        const float laneHeight = laneBounds.getHeight() / 2.0f;

        for (int ch = 0; ch < 2; ++ch)
        {
            const auto lane = laneBounds
                                  .withY (laneBounds.getY() + static_cast<float> (ch) * laneHeight)
                                  .withHeight (laneHeight - 2.0f);

            g.setFillColor (color.withAlpha (0.12f));
            g.fillRect ({ lane.getX(), lane.getCenterY(), lane.getWidth(), 1.0f });

            const auto& peaks = snapPeaks[ch];
            if (peaks.empty())
                continue;

            const float xScale = lane.getWidth() / static_cast<float> (peaks.size());
            const float halfH = lane.getHeight() * 0.5f;
            const float centerY = lane.getCenterY();

            g.setFillColor (color.withAlpha (0.8f));

            for (size_t i = 0; i < peaks.size(); ++i)
            {
                const float x = lane.getX() + static_cast<float> (i) * xScale;
                const float yTop = centerY - peaks[i].second * halfH;
                const float yBot = centerY - peaks[i].first * halfH;
                g.fillRect ({ x, yTop, yup::jmax (1.0f, xScale), yup::jmax (1.0f, yBot - yTop) });
            }
        }
    }

private:
    RecordingData& data;
    yup::Color color;
};

//==============================================================================
class RecorderNodeView final
    : public yup::AudioGraphNodeView
    , public yup::AsyncUpdater
{
public:
    RecorderNodeView (yup::AudioGraphNodeID nodeID, RecorderProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
        , engine (processorIn, data, *this)
        , waveform (data, getNodeColor())
    {
        setColor (Style::parameterBackgroundColorId, yup::Color (0x00000000));
        setColor (Style::parameterValueBackgroundColorId, yup::Color (0x00000000));

        recordButton.setButtonText ("● REC");
        recordButton.onClick = [this]
        {
            onRecordButtonClicked();
        };
        addAndMakeVisible (recordButton);

        saveButton.setButtonText ("Save WAV");
        saveButton.setEnabled (false);
        saveButton.onClick = [this]
        {
            onSaveButtonClicked();
        };
        addAndMakeVisible (saveButton);

        addAndMakeVisible (waveform);
        engine.start();
    }

    ~RecorderNodeView() override
    {
        processor.stopRecording();
        engine.stop();

        if (saveThread.joinable())
            saveThread.join();

        cancelPendingUpdate();
    }

    yup::String getNodeTitle() const override { return "REC"; }

    yup::String getNodeSubtitle() const override
    {
        switch (static_cast<SaveState> (saveState.load (std::memory_order_acquire)))
        {
            case SaveState::Saving:
                return "saving...";
            case SaveState::Saved:
                return "saved";
            case SaveState::Error:
                return "save failed";
            default:
                break;
        }

        const int frames = data.totalFrames.load (std::memory_order_relaxed);
        if (frames == 0)
            return "recorder";

        const double sr = data.sampleRate > 0.0 ? data.sampleRate : 44100.0;
        const int totalSecs = static_cast<int> (static_cast<double> (frames) / sr);
        const int mins = totalSecs / 60;
        const int secs = totalSecs % 60;
        const auto timeStr = yup::String (mins) + ":" + (secs < 10 ? "0" : "") + yup::String (secs);

        return isCurrentlyRecording ? timeStr : timeStr + " (stopped)";
    }

    int getNumInputPorts() const override { return 1; }

    int getNumOutputPorts() const override { return 0; }

    int getPreferredWidth() const override { return 260; }

    yup::Color getNodeColor() const override { return yup::Color (0xffe11d48); }

    PortInfo getInputPortInfo (int) const override
    {
        return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };
    }

    PortInfo getOutputPortInfo (int) const override
    {
        return { "audio", getPortKindColor (PortKind::audio), PortKind::audio };
    }

    int getNumParameterRows() const override { return 3; }

    ParameterInfo getParameterInfo (int) const override
    {
        return { {}, {}, getNodeColor(), -1.0f, PortKind::parameter };
    }

    void resized() override
    {
        const auto scale = getLocalBounds().getWidth() / static_cast<float> (getPreferredWidth());
        const auto body = getLocalBounds().to<float>().reduced (14.0f * scale, 0.0f);

        const float innerX = body.getX() + 8.0f * scale;
        const float innerW = body.getWidth() - 16.0f * scale;
        const float buttonY = 43.0f * scale;
        const float buttonH = 20.0f * scale;
        const float halfW = (innerW - 4.0f * scale) * 0.5f;

        recordButton.setBounds (innerX,
                                buttonY,
                                halfW,
                                buttonH);

        saveButton.setBounds (innerX + halfW + 4.0f * scale,
                              buttonY,
                              halfW,
                              buttonH);

        waveform.setBounds (innerX,
                            67.0f * scale,
                            innerW,
                            52.0f * scale);
    }

    void handleAsyncUpdate() override
    {
        waveform.repaint();
        updateButtonsFromSaveState();
        repaint(); // refreshes subtitle
    }

private:
    void onRecordButtonClicked()
    {
        if (isCurrentlyRecording)
        {
            processor.stopRecording();
            engine.wakeUp();
            isCurrentlyRecording = false;
            recordButton.setButtonText ("● REC");
            saveButton.setEnabled (data.totalFrames.load() > 0);
        }
        else
        {
            data.sampleRate = processor.getSampleRate();
            data.reset();
            saveState.store ((int) SaveState::None, std::memory_order_relaxed);
            processor.startRecording();
            isCurrentlyRecording = true;
            recordButton.setButtonText ("■ STOP");
            saveButton.setEnabled (false);
        }

        repaint();
    }

    void onSaveButtonClicked()
    {
        if (saveState.load (std::memory_order_relaxed) == (int) SaveState::Saving)
            return;

        saveState.store ((int) SaveState::Saving, std::memory_order_relaxed);
        saveButton.setButtonText ("Saving...");
        saveButton.setEnabled (false);

        std::vector<float> leftData, rightData;
        double sr = 44100.0;
        {
            std::lock_guard lock (data.mutex);
            leftData = data.samples[0];
            rightData = data.samples[1];
            sr = data.sampleRate;
        }

        if (saveThread.joinable())
            saveThread.join();

        saveThread = std::thread ([this, leftData = std::move (leftData), rightData = std::move (rightData), sr]
        {
            const auto result = writeToDisk (leftData, rightData, sr);
            saveState.store ((int) (result.wasOk() ? SaveState::Saved : SaveState::Error),
                             std::memory_order_release);
            triggerAsyncUpdate();
        });
    }

    static yup::Result writeToDisk (const std::vector<float>& left,
                                    const std::vector<float>& right,
                                    double sampleRate)
    {
        if (left.empty())
            return yup::Result::fail ("Nothing to save");

        const auto now = yup::Time::getCurrentTime();
        const auto filename = yup::String ("recording_") + now.formatted ("%Y%m%d_%H%M%S") + ".wav";
        const auto file = yup::File::getSpecialLocation (yup::File::userDesktopDirectory)
                              .getChildFile (filename);

        yup::AudioFormatManager formatManager;
        formatManager.registerDefaultFormats();

        auto writer = formatManager.createWriterFor (file,
                                                     static_cast<int> (sampleRate),
                                                     2,
                                                     24);
        if (writer == nullptr)
            return yup::Result::fail ("Could not create WAV writer: " + file.getFullPathName());

        const float* channels[2] = { left.data(), right.data() };
        if (! writer->write (channels, static_cast<int> (left.size())))
            return yup::Result::fail ("Failed writing WAV data");

        return yup::Result::ok();
    }

    void updateButtonsFromSaveState()
    {
        switch (static_cast<SaveState> (saveState.load (std::memory_order_acquire)))
        {
            case SaveState::Saved:
                saveButton.setButtonText ("Saved");
                saveButton.setEnabled (false);
                break;

            case SaveState::Error:
                saveButton.setButtonText ("Save WAV");
                saveButton.setEnabled (data.totalFrames.load() > 0 && ! isCurrentlyRecording);
                break;

            default:
                break;
        }
    }

    enum class SaveState
    {
        None,
        Saving,
        Saved,
        Error
    };

    RecorderProcessor& processor;
    RecordingData data;
    RecordingEngine engine;
    RecordingWaveformComponent waveform;

    yup::TextButton recordButton, saveButton;
    std::thread saveThread;

    std::atomic<int> saveState { (int) SaveState::None };
    bool isCurrentlyRecording = false;

    YUP_DECLARE_WEAK_REFERENCEABLE (RecorderNodeView)
};
