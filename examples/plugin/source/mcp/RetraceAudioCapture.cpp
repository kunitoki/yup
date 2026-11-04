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

#include "RetraceAudioCapture.h"

bool RetraceAudioCapture::start (double newSampleRate, double maxSeconds)
{
    const yup::CriticalSection::ScopedLockType lock (controlLock);

    if (armed.load() || newSampleRate <= 0.0 || maxSeconds <= 0.0)
        return false;

    buffer.setSize (2, yup::jmax (1, static_cast<int> (newSampleRate * maxSeconds)));
    numCaptured.store (0);
    sampleRate.store (newSampleRate);
    armed.store (true);
    return true;
}

yup::AudioBuffer<float> RetraceAudioCapture::stop()
{
    const yup::CriticalSection::ScopedLockType lock (controlLock);

    if (! armed.exchange (false))
        return {};

    // The audio thread checks armed after raising writing, so once writing drops here it
    // has either finished its block or will not touch the buffer at all.
    while (writing.load())
        yup::Thread::yield();

    const auto length = numCaptured.load();
    yup::AudioBuffer<float> result (buffer.getNumChannels(), length);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        result.copyFrom (channel, 0, buffer, channel, 0, length);

    return result;
}

bool RetraceAudioCapture::isFull() const noexcept
{
    return armed.load() && numCaptured.load() >= buffer.getNumSamples();
}

void RetraceAudioCapture::push (const yup::AudioBuffer<float>& audio) noexcept
{
    if (! armed.load())
        return;

    writing.store (true);

    if (armed.load() && audio.getNumChannels() > 0)
    {
        const auto written = numCaptured.load();
        const auto count = yup::jmin (audio.getNumSamples(), buffer.getNumSamples() - written);

        if (count > 0)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.copyFrom (channel, written, audio, yup::jmin (channel, audio.getNumChannels() - 1), 0, count);

            numCaptured.store (written + count);
        }
    }

    writing.store (false);
}
