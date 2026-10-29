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

#include "SynthSurround.h"
#include "SynthEffectTools.h"

namespace
{
constexpr double maximumDelayMs = 30.0;
} // namespace

//==============================================================================
void SynthSurround::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    delayLine.assign (static_cast<std::size_t> (std::ceil (maximumDelayMs * 0.001 * sampleRate)) + 2, 0.0f);
    sideHighpass.prepare (sampleRate, 1);

    for (auto* value : { &width, &depth, &delaySamples, &monoBelow, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthSurround::reset() noexcept
{
    for (auto* value : { &width, &depth, &delaySamples, &monoBelow, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    std::fill (delayLine.begin(), delayLine.end(), 0.0f);
    writeIndex = 0;
    sideHighpass.reset();
}

float SynthSurround::readDelayed (float delay) const noexcept
{
    const auto size = static_cast<int> (delayLine.size());
    const auto whole = static_cast<int> (delay);
    const auto fraction = delay - static_cast<float> (whole);

    const auto newer = delayLine[static_cast<std::size_t> ((writeIndex - whole + size) % size)];
    const auto older = delayLine[static_cast<std::size_t> ((writeIndex - whole - 1 + 2 * size) % size)];
    return newer + fraction * (older - newer);
}

void SynthSurround::process (float* left, float* right, int numSamples, const SynthSurroundValues& values) noexcept
{
    const auto maximumDelay = static_cast<float> (delayLine.size() - 2);

    width.setTargetValue (values.width);
    depth.setTargetValue (values.depth);
    delaySamples.setTargetValue (yup::jlimit (1.0f, maximumDelay, static_cast<float> (values.timeMs * 0.001 * sampleRate)));
    monoBelow.setTargetValue (values.monoBelow);
    mix.setTargetValue (values.mix);

    sideHighpass.setParameters (yup::FilterMode::highpass, monoBelow.skip (numSamples), 0.0, sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto mid = 0.5f * (left[i] + right[i]);
        const auto side = 0.5f * (left[i] - right[i]);

        delayLine[static_cast<std::size_t> (writeIndex)] = mid;
        const auto ambience = depth.getNextValue() * readDelayed (delaySamples.getNextValue());
        writeIndex = (writeIndex + 1) % static_cast<int> (delayLine.size());

        const auto wide = sideHighpass.processSample (width.getNextValue() * side + ambience);
        const auto wetAmount = mix.getNextValue();
        const auto newSide = side + wetAmount * (wide - side);

        left[i] = mid + newSide;
        right[i] = mid - newSide;
    }
}
