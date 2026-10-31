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

#include "SynthWaveshaper.h"
#include "SynthEffectTools.h"

namespace
{
constexpr double dcBlockerHz = 10.0;
} // namespace

//==============================================================================
SynthWaveshaper::SynthWaveshaper()
{
    fillTable (SynthCurveState::makeDefaultShaper());
}

void SynthWaveshaper::prepare (double sampleRate)
{
    for (auto& channel : channels)
    {
        channel.wet.setTraits ({ &table });
        channel.wet.prepare (sampleRate, 1);
        channel.dry.prepare (sampleRate, 1);

        channel.dcBlocker.prepare (sampleRate, 1);
        channel.dcBlocker.setParameters (yup::FilterMode::highpass, dcBlockerHz, 0.0, sampleRate);
    }

    for (auto* value : { &drive, &bias, &output, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthWaveshaper::reset() noexcept
{
    for (auto* value : { &drive, &bias, &output, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    for (auto& channel : channels)
    {
        channel.wet.reset();
        channel.dry.reset();
        channel.dcBlocker.reset();
    }

    outputQuiet = false;
}

void SynthWaveshaper::updateCurve (const SynthCurveState& curves) noexcept
{
    if (! curves.copyShaperIfChanged (curve, curveGeneration))
        return;

    fillTable (curve);

    // The next sample integrates from the previous input through the new curve.
    for (auto& channel : channels)
        channel.wet.setTraits ({ &table });

    outputQuiet = false;
}

void SynthWaveshaper::fillTable (const SynthCurveState::Curve& newCurve) noexcept
{
    for (int index = 0; index <= tableSegments; ++index)
    {
        const auto time = static_cast<float> (index) / static_cast<float> (tableSegments);
        table[static_cast<std::size_t> (index)] = 2.0f * newCurve.evaluate (time) - 1.0f;
    }
}

void SynthWaveshaper::process (float* left, float* right, int numSamples, const SynthWaveshaperValues& values) noexcept
{
    drive.setTargetValue (yup::Decibels::decibelsToGain (values.driveDb));
    bias.setTargetValue (values.bias);
    output.setTargetValue (yup::Decibels::decibelsToGain (values.outputDb));
    mix.setTargetValue (values.mix);

    // Silence in and nothing left ringing: the filters hold still and the output stays silent.
    // A moving bias would shift the curve's resting point, so it is followed rather than skipped.
    if (outputQuiet && ! bias.isSmoothing()
        && SynthEffectTools::isQuiet (left, numSamples) && SynthEffectTools::isQuiet (right, numSamples))
    {
        for (auto* value : { &drive, &bias, &output, &mix })
            value->skip (numSamples);

        return;
    }

    const CurveTraits shape { &table };
    float* buffers[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = drive.getNextValue();
        const auto offset = bias.getNextValue();
        const auto offsetLevel = shape.f (offset);
        const auto outputGain = output.getNextValue();
        const auto wetAmount = mix.getNextValue();

        for (std::size_t c = 0; c < channels.size(); ++c)
        {
            auto& channel = channels[c];
            const auto input = buffers[c][i];

            const auto dry = channel.dry.processSample (input);
            const auto wet = outputGain * channel.dcBlocker.processSample (channel.wet.processSample (gain * input + offset) - offsetLevel);
            buffers[c][i] = dry + wetAmount * (wet - dry);
        }
    }

    outputQuiet = SynthEffectTools::isQuiet (left, numSamples) && SynthEffectTools::isQuiet (right, numSamples);
}
