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

#include "SynthDelay.h"
#include "SynthEffectTools.h"

namespace
{
constexpr double minimumSeconds = 0.02;
constexpr double maximumSeconds = 2.0;
constexpr double timeGlideSeconds = 0.25;
} // namespace

//==============================================================================
void SynthDelay::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    timeCoefficient = 1.0 - std::exp (-1.0 / (timeGlideSeconds * sampleRate));

    for (auto& delay : delays)
        delay.setMaxDelaySamples (static_cast<int> (std::ceil (maximumSeconds * sampleRate)) + 2);

    for (auto& filter : lowCuts)
        filter.prepare (sampleRate, 1);

    for (auto& filter : highCuts)
        filter.prepare (sampleRate, 1);

    for (auto* value : { &feedback, &pingPong, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthDelay::reset() noexcept
{
    for (auto& delay : delays)
        delay.reset();

    for (auto& filter : lowCuts)
        filter.reset();

    for (auto& filter : highCuts)
        filter.reset();

    for (auto* value : { &feedback, &pingPong, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    lastWet = {};
    jumpTime = true;
}

void SynthDelay::process (float* left, float* right, int numSamples, const SynthDelayValues& values, double bpm) noexcept
{
    const auto targetSamples = yup::jlimit (minimumSeconds, maximumSeconds, values.getSeconds (bpm)) * sampleRate;

    if (std::exchange (jumpTime, false))
        timeSamples = targetSamples;

    for (std::size_t channel = 0; channel < delays.size(); ++channel)
    {
        lowCuts[channel].setParameters (yup::FilterMode::highpass, values.lowCut, 0.0, sampleRate);
        highCuts[channel].setParameters (yup::FilterMode::lowpass, yup::jmin (static_cast<double> (values.highCut), 0.45 * sampleRate), 0.0, sampleRate);
    }

    feedback.setTargetValue (values.feedback);
    pingPong.setTargetValue (values.pingPong ? 1.0f : 0.0f);
    mix.setTargetValue (values.mix);

    for (int i = 0; i < numSamples; ++i)
    {
        timeSamples += timeCoefficient * (targetSamples - timeSamples);

        const auto amount = feedback.getNextValue();
        const auto cross = pingPong.getNextValue();
        const auto [dryGain, wetGain] = SynthEffectTools::sendGains (mix.getNextValue());

        const auto inLeft = left[i];
        const auto inRight = right[i];
        const auto [wetLeft, wetRight] = lastWet;

        const std::array<float, 2> inputs {
            inLeft + cross * 0.5f * (inRight - inLeft) + amount * (wetLeft + cross * (wetRight - wetLeft)),
            (1.0f - cross) * inRight + amount * (wetRight + cross * (wetLeft - wetRight))
        };

        for (std::size_t channel = 0; channel < delays.size(); ++channel)
        {
            auto& delay = delays[channel];
            delay.setDelaySamples (timeSamples);
            lastWet[channel] = highCuts[channel].processSample (lowCuts[channel].processSample (delay.processSample (inputs[channel])));
        }

        left[i] = dryGain * inLeft + wetGain * lastWet[0];
        right[i] = dryGain * inRight + wetGain * lastWet[1];
    }
}
