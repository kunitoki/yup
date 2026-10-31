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

#include "SynthEqualizer.h"
#include "SynthEffectTools.h"

namespace
{
constexpr float shelfQ = 0.707f;
} // namespace

//==============================================================================
void SynthEqualizer::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto* band : { &low, &mid, &high })
    {
        for (auto& filter : band->filters)
            filter.prepare (sampleRate, 1);

        for (auto* value : { &band->gain, &band->frequency, &band->q })
            value->reset (sampleRate, SynthEffectTools::smoothingSeconds);
    }

    low.q.setCurrentAndTargetValue (shelfQ);
    high.q.setCurrentAndTargetValue (shelfQ);

    reset();
}

void SynthEqualizer::reset() noexcept
{
    for (auto* band : { &low, &mid, &high })
    {
        for (auto* value : { &band->gain, &band->frequency, &band->q })
            value->setCurrentAndTargetValue (value->getTargetValue());

        for (auto& filter : band->filters)
            filter.reset();
    }
}

void SynthEqualizer::process (float* left, float* right, int numSamples, const SynthEqualizerValues& values) noexcept
{
    low.gain.setTargetValue (values.lowGainDb);
    low.frequency.setTargetValue (values.lowFrequency);
    mid.gain.setTargetValue (values.midGainDb);
    mid.frequency.setTargetValue (values.midFrequency);
    mid.q.setTargetValue (values.midQ);
    high.gain.setTargetValue (values.highGainDb);
    high.frequency.setTargetValue (values.highFrequency);

    const std::pair<Band*, yup::FilterModeType> bands[] = { { &low, yup::FilterMode::lowshelf },
                                                            { &mid, yup::FilterMode::peak },
                                                            { &high, yup::FilterMode::highshelf } };

    for (const auto& [band, mode] : bands)
    {
        const auto gainDb = band->gain.skip (numSamples);
        const auto frequency = band->frequency.skip (numSamples);
        const auto q = band->q.skip (numSamples);

        // A flat band is skipped rather than filtered; its state is cleared so it comes back clean.
        if (std::abs (gainDb) < 0.01f && ! band->gain.isSmoothing())
        {
            for (auto& filter : band->filters)
                filter.reset();

            continue;
        }

        for (auto& filter : band->filters)
            filter.setParameters (mode, frequency, q, gainDb, sampleRate);

        band->filters[0].processInPlace (left, numSamples);
        band->filters[1].processInPlace (right, numSamples);
    }
}
