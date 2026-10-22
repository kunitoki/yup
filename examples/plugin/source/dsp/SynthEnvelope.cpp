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

#include "SynthEnvelope.h"

void SynthEnvelope::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

    reset();
}

void SynthEnvelope::reset() noexcept
{
    stage = Stage::idle;
    level = 0.0f;
    stageSample = 0;
}

void SynthEnvelope::setParameters (const SynthEnvelopeValues& values) noexcept
{
    sustainLevel = yup::jlimit (0.0f, 1.0f, values.sustain);

    delaySamples = toSamples (values.delay);
    holdSamples = toSamples (values.hold);
    releaseSamples = toSamples (yup::jmax (0.003f, values.release));

    attackIncrement = 1.0f / static_cast<float> (toSamples (yup::jmax (0.003f, values.attack)));
    decayIncrement = (1.0f - sustainLevel) / static_cast<float> (toSamples (values.decay));
}

void SynthEnvelope::noteOn() noexcept
{
    stage = Stage::delay;
    level = 0.0f;
    stageSample = 0;
}

void SynthEnvelope::noteOff() noexcept
{
    if (stage == Stage::idle)
        return;

    releaseIncrement = level / static_cast<float> (releaseSamples);
    stage = Stage::release;
    stageSample = 0;
}

int SynthEnvelope::toSamples (float seconds) const noexcept
{
    return yup::jmax (1, static_cast<int> (static_cast<double> (seconds) * sampleRate));
}
