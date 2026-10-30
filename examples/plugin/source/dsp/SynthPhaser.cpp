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

#include "SynthPhaser.h"
#include "SynthEffectTools.h"

#include <algorithm>

//==============================================================================
void SynthPhaser::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto* value : { &rate, &depth, &center, &feedback, &stereo, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthPhaser::reset() noexcept
{
    for (auto* value : { &rate, &depth, &center, &feedback, &stereo, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    states = {};
    lastWet = {};
    samplesUntilUpdate = 0;
    snapCoefficients = true;
}

void SynthPhaser::process (float* left, float* right, int numSamples, const SynthPhaserValues& values) noexcept
{
    rate.setTargetValue (values.rate);
    depth.setTargetValue (values.depth);
    center.setTargetValue (values.center);
    feedback.setTargetValue (values.feedback);
    stereo.setTargetValue (values.stereo);
    mix.setTargetValue (values.mix);

    // Silence in and nothing left ringing: only the sweep moves on, the output stays silent.
    if (isSilent (left, right, numSamples))
    {
        SynthEffectTools::advance (phase, rate.getCurrentValue() * numSamples / sampleRate);

        for (auto* value : { &rate, &depth, &center, &feedback, &stereo, &mix })
            value->skip (numSamples);

        states = {};
        lastWet = {};
        samplesUntilUpdate = 0;
        snapCoefficients = true;
        return;
    }

    const auto numStages = yup::jlimit (1, maximumStages, values.stages);
    float* channels[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        SynthEffectTools::advance (phase, rate.getNextValue() / sampleRate);

        const auto sweep = 3.0f * depth.getNextValue();
        const auto frequency = center.getNextValue();
        const auto amount = feedback.getNextValue();
        const auto offset = 0.5 * stereo.getNextValue();
        const auto wetGain = mix.getNextValue();

        // The sweep is slow, so the coefficients follow it at control rate, ramping in between.
        if (samplesUntilUpdate == 0)
        {
            samplesUntilUpdate = controlInterval;

            for (std::size_t channel = 0; channel < coefficients.size(); ++channel)
            {
                const auto target = computeCoefficient (phase + offset * static_cast<double> (channel), sweep, frequency);

                if (snapCoefficients)
                    coefficients[channel] = target;

                coefficientSteps[channel] = (target - coefficients[channel]) / static_cast<float> (controlInterval);
            }

            snapCoefficients = false;
        }

        --samplesUntilUpdate;

        for (std::size_t channel = 0; channel < states.size(); ++channel)
        {
            const auto coefficient = coefficients[channel];
            coefficients[channel] += coefficientSteps[channel];

            const auto input = channels[channel][i];
            auto signal = input + amount * lastWet[channel];
            auto& state = states[channel];

            for (int stage = 0; stage < numStages; ++stage)
            {
                const auto output = coefficient * signal + state[static_cast<std::size_t> (stage)];
                state[static_cast<std::size_t> (stage)] = signal - coefficient * output;
                signal = output;
            }

            lastWet[channel] = signal;
            channels[channel][i] = input + wetGain * (signal - input);
        }
    }
}

bool SynthPhaser::isSilent (const float* left, const float* right, int numSamples) const noexcept
{
    const auto quiet = [] (float value) { return SynthEffectTools::isQuiet (value); };

    return SynthEffectTools::isQuiet (left, numSamples)
        && SynthEffectTools::isQuiet (right, numSamples)
        && std::all_of (lastWet.begin(), lastWet.end(), quiet)
        && std::all_of (states.begin(), states.end(), [&] (const auto& channel) { return std::all_of (channel.begin(), channel.end(), quiet); });
}

float SynthPhaser::computeCoefficient (double channelPhase, float sweep, float frequency) const noexcept
{
    const auto lfo = SynthEffectTools::sine (channelPhase - std::floor (channelPhase));
    const auto cutoff = yup::jlimit (20.0, 0.45 * sampleRate, static_cast<double> (frequency * std::exp2 (sweep * lfo)));
    const auto t = std::tan (cutoff * yup::MathConstants<double>::pi / sampleRate);
    return static_cast<float> ((t - 1.0) / (t + 1.0));
}
