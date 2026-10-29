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

#include "SynthBitcrusher.h"
#include "SynthEffectTools.h"

//==============================================================================
void SynthBitcrusher::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto* value : { &bits, &rate, &dither, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthBitcrusher::reset() noexcept
{
    for (auto* value : { &bits, &rate, &dither, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    holdPhase = 1.0;
    held = {};
}

void SynthBitcrusher::process (float* left, float* right, int numSamples, const SynthBitcrusherValues& values) noexcept
{
    bits.setTargetValue (values.bits);
    rate.setTargetValue (values.rate);
    dither.setTargetValue (values.dither);
    mix.setTargetValue (values.mix);

    float* channels[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        const auto step = 2.0f / std::exp2 (bits.getNextValue());
        const auto noise = step * dither.getNextValue();
        const auto wet = mix.getNextValue();

        holdPhase += rate.getNextValue() / sampleRate;

        if (holdPhase >= 1.0)
        {
            holdPhase -= std::floor (holdPhase);

            for (std::size_t channel = 0; channel < held.size(); ++channel)
            {
                const auto tpdf = noise * (random.nextFloat() - random.nextFloat());
                held[channel] = std::round ((channels[channel][i] + tpdf) / step) * step;
            }
        }

        for (std::size_t channel = 0; channel < held.size(); ++channel)
            channels[channel][i] += wet * (held[channel] - channels[channel][i]);
    }
}
