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

#include <yup_audio_basics/yup_audio_basics.h>

#include <atomic>

//==============================================================================
/** Records what the processor plays, for the MCP server's live capture.

    The buffer is allocated when a capture starts, never on the audio thread, which only
    copies the end of each block into it while a capture runs and stops copying once it is
    full. Starting and stopping may happen on any thread but the audio one.
*/
class RetraceAudioCapture
{
public:
    RetraceAudioCapture() = default;

    /** Starts recording up to maxSeconds of stereo audio.

        @returns false when a capture is already running or the sample rate is not known.
    */
    bool start (double sampleRate, double maxSeconds);

    /** Stops recording and returns what was recorded, empty when no capture ran. */
    yup::AudioBuffer<float> stop();

    /** Returns true while a capture runs. */
    bool isCapturing() const noexcept { return armed.load(); }

    /** Returns true when a running capture filled its buffer. */
    bool isFull() const noexcept;

    /** Returns the sample rate of the running or last capture. */
    double getSampleRate() const noexcept { return sampleRate.load(); }

    /** Copies a processed block into the capture. Audio thread; never allocates. */
    void push (const yup::AudioBuffer<float>& audio) noexcept;

private:
    yup::AudioBuffer<float> buffer;
    std::atomic<int> numCaptured { 0 };
    std::atomic<bool> armed { false };
    std::atomic<bool> writing { false };
    std::atomic<double> sampleRate { 0.0 };
    yup::CriticalSection controlLock;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RetraceAudioCapture)
};
