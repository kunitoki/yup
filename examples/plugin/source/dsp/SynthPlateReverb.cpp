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

#include "SynthPlateReverb.h"
#include "SynthEffectTools.h"

namespace
{
constexpr double referenceRate = 29761.0;
constexpr double maximumPredelaySeconds = 0.25;
constexpr double inputCutoff = 12000.0;
constexpr double modulationRate = 1.0;
constexpr float modulationExcursion = 16.0f;
constexpr float outputGain = 0.6f;

constexpr std::array<int, 4> diffuserLengths { 142, 107, 379, 277 };
constexpr std::array<float, 4> diffuserCoefficients { 0.75f, 0.75f, 0.625f, 0.625f };
constexpr std::array<int, 2> modulatedLengths { 672, 908 };
constexpr std::array<int, 2> firstDelayLengths { 4453, 4217 };
constexpr std::array<int, 2> decayAllpassLengths { 1800, 2656 };
constexpr std::array<int, 2> secondDelayLengths { 3720, 3163 };
constexpr float modulatedCoefficient = -0.7f;

float dampingPoleFor (float damping, double sampleRate) noexcept
{
    const auto cutoff = 18000.0 * std::pow (1000.0 / 18000.0, static_cast<double> (damping));
    return static_cast<float> (std::exp (-yup::MathConstants<double>::twoPi * yup::jmin (cutoff, 0.45 * sampleRate) / sampleRate));
}
} // namespace

//==============================================================================
void SynthPlateReverb::DelayBuffer::allocate (int maximumDelay)
{
    data.assign (static_cast<std::size_t> (yup::nextPowerOfTwo (maximumDelay + 2)), 0.0f);
    mask = static_cast<int> (data.size()) - 1;
    writeIndex = 0;
}

void SynthPlateReverb::DelayBuffer::clear() noexcept
{
    std::fill (data.begin(), data.end(), 0.0f);
}

void SynthPlateReverb::DelayBuffer::push (float sample) noexcept
{
    writeIndex = (writeIndex + 1) & mask;
    data[static_cast<std::size_t> (writeIndex)] = sample;
}

float SynthPlateReverb::DelayBuffer::read (int delay) const noexcept
{
    return data[static_cast<std::size_t> ((writeIndex - delay) & mask)];
}

float SynthPlateReverb::DelayBuffer::readFractional (float delay) const noexcept
{
    const auto whole = static_cast<int> (delay);
    const auto fraction = delay - static_cast<float> (whole);
    const auto a = read (whole);
    return a + fraction * (read (whole + 1) - a);
}

float SynthPlateReverb::Allpass::process (float input, float coefficient, float delay) noexcept
{
    const auto delayed = buffer.readFractional (delay - 1.0f);
    const auto stored = input - coefficient * delayed;
    buffer.push (stored);
    return coefficient * stored + delayed;
}

//==============================================================================
int SynthPlateReverb::scaled (int samplesAtReferenceRate) const noexcept
{
    return yup::jmax (1, yup::roundToInt (samplesAtReferenceRate * sampleRate / referenceRate));
}

void SynthPlateReverb::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    inputPole = static_cast<float> (std::exp (-yup::MathConstants<double>::twoPi * yup::jmin (inputCutoff, 0.45 * sampleRate) / sampleRate));

    predelay.allocate (static_cast<int> (std::ceil (maximumPredelaySeconds * sampleRate)) + 1);

    for (std::size_t i = 0; i < diffusers.size(); ++i)
        diffusers[i].buffer.allocate (scaled (diffuserLengths[i]));

    const auto excursion = static_cast<int> (std::ceil (modulationExcursion * sampleRate / referenceRate));

    for (std::size_t side = 0; side < 2; ++side)
    {
        modulatedAllpasses[side].buffer.allocate (scaled (modulatedLengths[side]) + excursion + 1);
        decayAllpasses[side].buffer.allocate (scaled (decayAllpassLengths[side]));
        firstDelays[side].allocate (scaled (firstDelayLengths[side]));
        secondDelays[side].allocate (scaled (secondDelayLengths[side]));
    }

    for (auto* value : { &predelaySamples, &decay, &dampingPole, &width, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthPlateReverb::reset() noexcept
{
    predelay.clear();

    for (auto& diffuser : diffusers)
        diffuser.buffer.clear();

    for (std::size_t side = 0; side < 2; ++side)
    {
        modulatedAllpasses[side].buffer.clear();
        decayAllpasses[side].buffer.clear();
        firstDelays[side].clear();
        secondDelays[side].clear();
    }

    for (auto* value : { &predelaySamples, &decay, &dampingPole, &width, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    bandwidthState = 0.0f;
    dampingStates = {};
    lastLeftTank = 0.0f;
    lastRightTank = 0.0f;
}

void SynthPlateReverb::process (float* left, float* right, int numSamples, const SynthReverbValues& values) noexcept
{
    predelaySamples.setTargetValue (static_cast<float> (yup::jlimit (0.0, maximumPredelaySeconds, values.predelayMs * 0.001) * sampleRate));
    decay.setTargetValue (0.2f + 0.78f * values.decay);
    dampingPole.setTargetValue (dampingPoleFor (values.damping, sampleRate));
    width.setTargetValue (values.width);
    mix.setTargetValue (values.mix);

    const auto rateScale = static_cast<float> (sampleRate / referenceRate);
    const auto excursion = modulationExcursion * rateScale;
    const auto tap = [this] (const DelayBuffer& buffer, int delay) { return buffer.read (scaled (delay)); };

    for (int i = 0; i < numSamples; ++i)
    {
        SynthEffectTools::advance (modulationPhase, modulationRate / sampleRate);

        const auto gain = decay.getNextValue();
        const auto decayDiffusion = yup::jlimit (0.25f, 0.5f, gain + 0.15f);
        const auto pole = dampingPole.getNextValue();
        const auto spread = width.getNextValue();
        const auto [dryGain, wetGain] = SynthEffectTools::sendGains (mix.getNextValue());

        const auto inLeft = left[i];
        const auto inRight = right[i];

        predelay.push (0.5f * (inLeft + inRight));
        const auto delayedInput = predelay.readFractional (predelaySamples.getNextValue());
        bandwidthState = delayedInput + inputPole * (bandwidthState - delayedInput);

        auto diffused = bandwidthState;

        for (std::size_t d = 0; d < diffusers.size(); ++d)
            diffused = diffusers[d].process (diffused, diffuserCoefficients[d], static_cast<float> (scaled (diffuserLengths[d])));

        const std::array<float, 2> tankInputs { diffused + gain * lastRightTank, diffused + gain * lastLeftTank };
        const std::array<float, 2> modulation { SynthEffectTools::sine (modulationPhase),
                                                SynthEffectTools::sine (modulationPhase + 0.25 - std::floor (modulationPhase + 0.25)) };

        for (std::size_t side = 0; side < 2; ++side)
        {
            const auto modulatedDelay = static_cast<float> (scaled (modulatedLengths[side])) + excursion * modulation[side];
            auto signal = modulatedAllpasses[side].process (tankInputs[side], modulatedCoefficient, modulatedDelay);

            firstDelays[side].push (signal);
            signal = firstDelays[side].read (scaled (firstDelayLengths[side]));

            dampingStates[side] = signal + pole * (dampingStates[side] - signal);
            signal = decayAllpasses[side].process (gain * dampingStates[side], decayDiffusion, static_cast<float> (scaled (decayAllpassLengths[side])));

            secondDelays[side].push (signal);
        }

        lastLeftTank = secondDelays[0].read (scaled (secondDelayLengths[0]));
        lastRightTank = secondDelays[1].read (scaled (secondDelayLengths[1]));

        const auto& [leftFirst, rightFirst] = firstDelays;
        const auto& [leftSecond, rightSecond] = secondDelays;
        const auto& leftDecay = decayAllpasses[0].buffer;
        const auto& rightDecay = decayAllpasses[1].buffer;

        const auto wetLeft = outputGain * (tap (rightFirst, 266) + tap (rightFirst, 2974) - tap (rightDecay, 1913) + tap (rightSecond, 1996)
                                           - tap (leftFirst, 1990) - tap (leftDecay, 187) - tap (leftSecond, 1066));
        const auto wetRight = outputGain * (tap (leftFirst, 353) + tap (leftFirst, 3627) - tap (leftDecay, 1228) + tap (leftSecond, 2673)
                                            - tap (rightFirst, 2111) - tap (rightDecay, 335) - tap (rightSecond, 121));

        const auto mid = 0.5f * (wetLeft + wetRight);
        const auto side = 0.5f * spread * (wetLeft - wetRight);

        left[i] = dryGain * inLeft + wetGain * (mid + side);
        right[i] = dryGain * inRight + wetGain * (mid - side);
    }
}
