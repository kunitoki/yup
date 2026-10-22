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

#include "SynthFilterStage.h"

void SynthFilterStage::prepare (double sampleRate)
{
    for (auto& filter : filters)
        filter.prepare (sampleRate, SynthExample::maxBlockSize);

    cutoff.reset (sampleRate, 0.005);
    resonance.reset (sampleRate, 0.005);
    drive.reset (sampleRate, 0.005);

    cutoff.setCurrentAndTargetValue (8000.0f);
    resonance.setCurrentAndTargetValue (0.2f);
    drive.setCurrentAndTargetValue (0.0f);
}

void SynthFilterStage::reset() noexcept
{
    for (auto& filter : filters)
        filter.reset();
}

void SynthFilterStage::process (float* left, float* right, int numSamples, const SynthFilterValues& values, double midiNote) noexcept
{
    if (values.type == SynthFilterType::off)
        return;

    const auto tracked = values.cutoff * std::exp2 (values.keytrack * static_cast<float> (midiNote - 60.0) / 12.0f);

    cutoff.setTargetValue (yup::jlimit (20.0f, 20000.0f, tracked));
    resonance.setTargetValue (values.resonance);
    drive.setTargetValue (values.drive);

    const auto mode = toFilterMode (values.type);

    for (auto& filter : filters)
    {
        filter.setMode (mode);
        filter.setShelfGain (12.0f);
    }

    float* channels[] = { left, right };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto frequency = cutoff.getNextValue();
        const auto q = yup::VAStateVariableFilter<float>::resonanceToQ (resonance.getNextValue());
        const auto amount = drive.getNextValue();
        const auto gain = 1.0f + 9.0f * amount;
        const auto makeup = amount > 0.0f ? 1.0f / std::tanh (gain) : 1.0f;

        for (std::size_t channel = 0; channel < filters.size(); ++channel)
        {
            auto& filter = filters[channel];
            filter.setCutoffFrequency (frequency);
            filter.setQ (q);

            auto x = channels[channel][sample];

            if (amount > 0.0f)
                x = std::tanh (x * gain) * makeup;

            channels[channel][sample] = filter.processSample (x);
        }
    }
}
