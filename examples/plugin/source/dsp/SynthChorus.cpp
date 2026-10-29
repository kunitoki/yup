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

#include "SynthChorus.h"
#include "SynthEffectTools.h"

namespace
{
struct JunoMode
{
    double rate;
    float minimumMs, maximumMs;
};

JunoMode getJunoMode (SynthChorusMode mode) noexcept
{
    switch (mode)
    {
        case SynthChorusMode::junoI:
            return { 0.513, 1.66f, 5.35f };
        case SynthChorusMode::junoII:
            return { 0.863, 1.66f, 5.35f };
        case SynthChorusMode::junoBoth:
        case SynthChorusMode::generic:
            break;
    }

    return { 9.75, 3.3f, 3.7f };
}

constexpr float maximumDepthMs = 8.0f;
constexpr float minimumDelayMs = 0.5f;
constexpr float maximumDelayMs = 30.0f;
constexpr double bucketBrigadeCutoff = 7500.0;
} // namespace

//==============================================================================
void SynthChorus::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto& delay : delays)
        delay.setMaxDelaySamples (static_cast<int> (std::ceil (maximumDelayMs * 0.001 * sampleRate)) + 2);

    for (auto& filter : tone)
    {
        filter.prepare (sampleRate, 1);
        filter.setParameters (yup::FilterMode::lowpass, bucketBrigadeCutoff, 0.0, sampleRate);
    }

    for (auto* value : { &rate, &centerMs, &depthMs, &feedback, &mix, &toneAmount })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthChorus::reset() noexcept
{
    for (auto& delay : delays)
        delay.reset();

    for (auto& filter : tone)
        filter.reset();

    for (auto* value : { &rate, &centerMs, &depthMs, &feedback, &mix, &toneAmount })
        value->setCurrentAndTargetValue (value->getTargetValue());

    lastWet = {};
}

void SynthChorus::process (float* left, float* right, int numSamples, const SynthChorusValues& values) noexcept
{
    const auto generic = values.mode == SynthChorusMode::generic;
    const auto juno = getJunoMode (values.mode);

    rate.setTargetValue (generic ? values.rate : static_cast<float> (juno.rate));
    centerMs.setTargetValue (generic ? values.delayMs : 0.5f * (juno.minimumMs + juno.maximumMs));
    depthMs.setTargetValue (generic ? values.depth * maximumDepthMs : 0.5f * (juno.maximumMs - juno.minimumMs));
    feedback.setTargetValue (generic ? values.feedback : 0.0f);
    toneAmount.setTargetValue (generic ? 0.0f : 1.0f);
    mix.setTargetValue (values.mix);

    const auto rightOffset = generic ? 0.25 : 0.5;
    const auto samplesPerMs = static_cast<float> (sampleRate * 0.001);
    float* channels[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        SynthEffectTools::advance (phase, rate.getNextValue() / sampleRate);

        const auto center = centerMs.getNextValue();
        const auto depth = depthMs.getNextValue();
        const auto amount = feedback.getNextValue();
        const auto dark = toneAmount.getNextValue();
        const auto [dryGain, wetGain] = SynthEffectTools::sendGains (mix.getNextValue());

        for (std::size_t channel = 0; channel < delays.size(); ++channel)
        {
            const auto channelPhase = phase + rightOffset * static_cast<double> (channel);
            const auto lfo = generic ? SynthEffectTools::sine (channelPhase - std::floor (channelPhase))
                                     : SynthEffectTools::triangle (channelPhase - std::floor (channelPhase));
            const auto delayMs = yup::jlimit (minimumDelayMs, maximumDelayMs, center + depth * lfo);

            auto& delay = delays[channel];
            delay.setDelaySamples (delayMs * samplesPerMs);

            const auto input = channels[channel][i];
            auto wet = delay.processSample (input + amount * lastWet[channel]);
            lastWet[channel] = wet;
            wet += dark * (tone[channel].processSample (wet) - wet);

            channels[channel][i] = dryGain * input + wetGain * wet;
        }
    }
}
