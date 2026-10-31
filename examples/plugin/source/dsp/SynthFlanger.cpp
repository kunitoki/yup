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

#include "SynthFlanger.h"
#include "SynthEffectTools.h"

namespace
{
constexpr float minimumDelayMs = 0.1f;
constexpr float maximumDelayMs = 20.0f;
} // namespace

//==============================================================================
void SynthFlanger::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto& delay : delays)
        delay.setMaxDelaySamples (static_cast<int> (std::ceil (maximumDelayMs * 0.001 * sampleRate)) + 2);

    for (auto* value : { &rate, &depth, &delayMs, &feedback, &stereo, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthFlanger::reset() noexcept
{
    for (auto& delay : delays)
        delay.reset();

    for (auto* value : { &rate, &depth, &delayMs, &feedback, &stereo, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    lastWet = {};
}

void SynthFlanger::process (float* left, float* right, int numSamples, const SynthFlangerValues& values) noexcept
{
    rate.setTargetValue (values.rate);
    depth.setTargetValue (values.depth);
    delayMs.setTargetValue (values.delayMs);
    feedback.setTargetValue (values.feedback);
    stereo.setTargetValue (values.stereo);
    mix.setTargetValue (values.mix);

    const auto samplesPerMs = static_cast<float> (sampleRate * 0.001);
    float* channels[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        SynthEffectTools::advance (phase, rate.getNextValue() / sampleRate);

        const auto sweep = depth.getNextValue();
        const auto base = delayMs.getNextValue();
        const auto amount = feedback.getNextValue();
        const auto offset = 0.5 * stereo.getNextValue();
        const auto wetGain = mix.getNextValue();

        for (std::size_t channel = 0; channel < delays.size(); ++channel)
        {
            const auto channelPhase = phase + offset * static_cast<double> (channel);
            const auto lfo = SynthEffectTools::triangle (channelPhase - std::floor (channelPhase));
            const auto time = yup::jlimit (minimumDelayMs, maximumDelayMs, base * (1.0f + sweep * lfo));

            auto& delay = delays[channel];
            delay.setDelaySamples (time * samplesPerMs);

            const auto input = channels[channel][i];
            const auto wet = delay.processSample (input + amount * lastWet[channel]);
            lastWet[channel] = wet;

            channels[channel][i] = input + wetGain * (wet - input);
        }
    }
}
