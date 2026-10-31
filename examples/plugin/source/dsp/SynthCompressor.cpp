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

#include "SynthCompressor.h"
#include "SynthEffectTools.h"

namespace
{
constexpr float kneeDb = 6.0f;
constexpr float floorDb = -120.0f;

float coefficientFor (float milliseconds, double sampleRate) noexcept
{
    return static_cast<float> (1.0 - std::exp (-1000.0 / (yup::jmax (0.01, static_cast<double> (milliseconds)) * sampleRate)));
}

/** Returns how many decibels a level above the threshold is turned down, with the knee centered on the threshold. */
float gainReductionDb (float levelDb, float thresholdDb, float ratio) noexcept
{
    const auto over = levelDb - thresholdDb;
    const auto slope = 1.0f - 1.0f / ratio;

    if (over <= -0.5f * kneeDb)
        return 0.0f;

    if (over >= 0.5f * kneeDb)
        return slope * over;

    const auto inKnee = over + 0.5f * kneeDb;
    return slope * inKnee * inKnee / (2.0f * kneeDb);
}
} // namespace

//==============================================================================
void SynthCompressor::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    for (auto* value : { &threshold, &ratio, &makeup, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthCompressor::reset() noexcept
{
    for (auto* value : { &threshold, &ratio, &makeup, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    reductionDb = 0.0f;
}

void SynthCompressor::process (float* left, float* right, int numSamples, const SynthCompressorValues& values) noexcept
{
    threshold.setTargetValue (values.thresholdDb);
    ratio.setTargetValue (values.ratio);
    makeup.setTargetValue (yup::Decibels::decibelsToGain (values.makeupDb));
    mix.setTargetValue (values.mix);

    const auto attack = coefficientFor (values.attackMs, sampleRate);
    const auto release = coefficientFor (values.releaseMs, sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto peak = yup::jmax (std::abs (left[i]), std::abs (right[i]));
        const auto levelDb = yup::Decibels::gainToDecibels (peak, floorDb);
        const auto targetDb = gainReductionDb (levelDb, threshold.getNextValue(), ratio.getNextValue());

        reductionDb += (targetDb > reductionDb ? attack : release) * (targetDb - reductionDb);

        const auto gain = makeup.getNextValue() * yup::Decibels::decibelsToGain (-reductionDb);
        const auto wetAmount = mix.getNextValue();
        const auto blend = 1.0f + wetAmount * (gain - 1.0f);

        left[i] *= blend;
        right[i] *= blend;
    }
}
