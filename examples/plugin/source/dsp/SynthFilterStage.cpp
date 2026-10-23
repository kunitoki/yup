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

void SynthFilterStage::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

    for (auto& stage : stateVariable)
        for (auto& filter : stage)
            filter.prepare (sampleRate, SynthExample::maxBlockSize);

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto index = static_cast<std::size_t> (channel);
        moog[index].prepare (sampleRate, SynthExample::maxBlockSize);
        korg[index].prepare (sampleRate, SynthExample::maxBlockSize);
        diode[index].prepare (sampleRate, SynthExample::maxBlockSize);
        vowel[index].prepare (sampleRate, SynthExample::maxBlockSize);
        comb[index].prepare (sampleRate, SynthExample::maxBlockSize);
        // The second order kernel (AA-IIR-1) costs one complex pole per sample; the
        // default tenth order one costs five, which made the drive the hottest stage.
        drivers[index].configure (Antialiaser::makeButterworthOrder2());
        drivers[index].prepare (sampleRate, SynthExample::maxBlockSize);
    }

    reset();
}

void SynthFilterStage::reset() noexcept
{
    for (auto& stage : stateVariable)
        for (auto& filter : stage)
            filter.reset();

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto index = static_cast<std::size_t> (channel);
        moog[index].reset();
        korg[index].reset();
        diode[index].reset();
        vowel[index].reset();
        comb[index].reset();
        drivers[index].reset();
    }

    hasState = false;
}

void SynthFilterStage::process (float* left, float* right, int numSamples, const SynthFilterValues& values, double midiNote) noexcept
{
    if (numSamples <= 0 || (! values.enabled && (! hasState || mix <= 0.0f)))
        return;

    if (values.type != activeType)
    {
        reset();
        activeType = values.type;
    }

    const auto tracked = values.cutoff * std::exp2 (values.keytrack * static_cast<float> (midiNote - 60.0) / 12.0f);
    const auto targetCutoff = yup::jlimit (20.0f, 20000.0f, tracked);

    if (! hasState)
    {
        cutoff = targetCutoff;
        resonance = values.resonance;
        drive = values.drive;
        mix = values.enabled ? values.mix : 0.0f;
        hasState = true;
    }

    // Cutoff ramps in octaves, the rest linearly, all from the last chunk's values.
    const auto inverse = 1.0f / static_cast<float> (numSamples);
    const auto cutoffStep = std::exp2 (std::log2 (targetCutoff / cutoff) * inverse);
    const auto resonanceStep = (values.resonance - resonance) * inverse;
    const auto driveStep = (values.drive - drive) * inverse;
    const auto targetMix = values.enabled ? values.mix : 0.0f;
    const auto mixStep = (targetMix - mix) * inverse;
    const auto driving = drive > 0.0f || values.drive > 0.0f;

    float* channels[] = { left, right };

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (sample % coefficientInterval == 0)
            updateCoefficients (activeType, cutoff, resonance, 0.25f * drive);

        const auto gain = 1.0f + 9.0f * drive;
        const auto makeup = 1.0f / std::tanh (gain);

        for (int channel = 0; channel < 2; ++channel)
        {
            const auto dry = channels[channel][sample];
            auto x = dry;

            if (driving)
                x = drivers[static_cast<std::size_t> (channel)].processSample (x * gain) * makeup;

            const auto wet = filterSample (channel, activeType, x);
            channels[channel][sample] = dry + mix * (wet - dry);
        }

        cutoff *= cutoffStep;
        resonance += resonanceStep;
        drive += driveStep;
        mix += mixStep;
    }

    cutoff = targetCutoff;
    resonance = values.resonance;
    drive = values.drive;
    mix = targetMix;
}

void SynthFilterStage::updateCoefficients (SynthFilterType type, float frequency, float normalizedResonance, float saturation) noexcept
{
    const auto q = yup::VAStateVariableFilter<float>::resonanceToQ (normalizedResonance);

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto index = static_cast<std::size_t> (channel);

        if (isStateVariableFilter (type))
        {
            for (auto& stage : stateVariable)
                stage[index].setParameters (toFilterMode (type), frequency, q, 12.0f, sampleRate);

            continue;
        }

        switch (type)
        {
            case SynthFilterType::moogLadder:
                moog[index].setParameters (yup::AnalogMoogLadderMode::lowpass24, frequency, normalizedResonance, saturation, sampleRate);
                break;

            case SynthFilterType::korg35:
                korg[index].setParameters (yup::FilterMode::lowpass, frequency, normalizedResonance, saturation, sampleRate);
                break;

            case SynthFilterType::diode:
                diode[index].setParameters (frequency, normalizedResonance, saturation, sampleRate);
                break;

            case SynthFilterType::vowel:
                // The cutoff's position across 20 Hz to 20 kHz sweeps the vowels.
                vowel[index].setParameters (std::log2 (frequency / 20.0f) / std::log2 (1000.0f), normalizedResonance, saturation, sampleRate);
                break;

            case SynthFilterType::comb:
                comb[index].setParameters (frequency, 0.95f * normalizedResonance, saturation, sampleRate);
                break;

            default:
                break;
        }
    }
}

void SynthFilterStage::configureForDisplay (const SynthFilterValues& values) noexcept
{
    activeType = values.type;
    updateCoefficients (values.type, yup::jlimit (20.0f, 20000.0f, values.cutoff), values.resonance, 0.0f);
}

float SynthFilterStage::getMagnitudeResponse (float frequency) const noexcept
{
    if (isStateVariableFilter (activeType))
    {
        const auto magnitude = stateVariable[0][0].getMagnitudeResponse (frequency);
        return isCascadedFilter (activeType) ? magnitude * stateVariable[1][0].getMagnitudeResponse (frequency) : magnitude;
    }

    switch (activeType)
    {
        case SynthFilterType::moogLadder: return moog[0].getMagnitudeResponse (frequency);
        case SynthFilterType::korg35: return korg[0].getMagnitudeResponse (frequency);
        case SynthFilterType::diode: return diode[0].getMagnitudeResponse (frequency);
        case SynthFilterType::vowel: return vowel[0].getMagnitudeResponse (frequency);
        case SynthFilterType::comb: return comb[0].getMagnitudeResponse (frequency);
        default: break;
    }

    return 1.0f;
}

float SynthFilterStage::filterSample (int channel, SynthFilterType type, float input) noexcept
{
    const auto index = static_cast<std::size_t> (channel);

    if (isStateVariableFilter (type))
    {
        auto output = stateVariable[0][index].processSample (input);

        if (isCascadedFilter (type))
            output = stateVariable[1][index].processSample (output);

        return output;
    }

    switch (type)
    {
        case SynthFilterType::moogLadder: return moog[index].processSample (input);
        case SynthFilterType::korg35: return korg[index].processSample (input);
        case SynthFilterType::diode: return diode[index].processSample (input);
        case SynthFilterType::vowel: return vowel[index].processSample (input);
        case SynthFilterType::comb: return comb[index].processSample (input);
        default: break;
    }

    return input;
}
