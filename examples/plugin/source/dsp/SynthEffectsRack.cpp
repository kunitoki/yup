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

#include "SynthEffectsRack.h"

//==============================================================================
void SynthEffectsRack::prepare (double sampleRate, int maxBlockSize)
{
    bitcrusher.prepare (sampleRate);
    chorus.prepare (sampleRate);
    phaser.prepare (sampleRate);
    flanger.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);
    distortion.prepare (sampleRate);
    equalizer.prepare (sampleRate);
    compressor.prepare (sampleRate);
    surround.prepare (sampleRate);
    waveshaper.prepare (sampleRate);

    fadeStep = static_cast<float> (1.0 / (SynthExample::enableFadeSeconds * sampleRate));
    wetLeft.assign (static_cast<std::size_t> (maxBlockSize), 0.0f);
    wetRight.assign (static_cast<std::size_t> (maxBlockSize), 0.0f);
    enableGains.fill (0.0f);
}

void SynthEffectsRack::reset() noexcept
{
    for (int index = 0; index < SynthExample::effectCount; ++index)
        resetEffect (static_cast<SynthEffectType> (index));
}

void SynthEffectsRack::process (float* left, float* right, int numSamples, const SynthEffectOrder& order, const SynthEffectsValues& values, double bpm) noexcept
{
    jassert (numSamples <= static_cast<int> (wetLeft.size()));
    numSamples = yup::jmin (numSamples, static_cast<int> (wetLeft.size()));

    for (const auto type : order)
    {
        auto& gain = enableGains[static_cast<std::size_t> (type)];
        const auto target = values.isEnabled (type) ? 1.0f : 0.0f;

        if (gain == target)
        {
            if (target == 1.0f)
                processEffect (type, left, right, numSamples, values, bpm);

            continue;
        }

        std::copy (left, left + numSamples, wetLeft.begin());
        std::copy (right, right + numSamples, wetRight.begin());
        processEffect (type, wetLeft.data(), wetRight.data(), numSamples, values, bpm);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            gain = target > gain ? yup::jmin (target, gain + fadeStep) : yup::jmax (target, gain - fadeStep);
            left[sample] += gain * (wetLeft[static_cast<std::size_t> (sample)] - left[sample]);
            right[sample] += gain * (wetRight[static_cast<std::size_t> (sample)] - right[sample]);
        }

        if (gain == 0.0f)
            resetEffect (type);
    }
}

void SynthEffectsRack::processEffect (SynthEffectType type, float* left, float* right, int numSamples, const SynthEffectsValues& values, double bpm) noexcept
{
    switch (type)
    {
        case SynthEffectType::bitcrusher: bitcrusher.process (left, right, numSamples, values.bitcrusher); break;
        case SynthEffectType::chorus: chorus.process (left, right, numSamples, values.chorus); break;
        case SynthEffectType::phaser: phaser.process (left, right, numSamples, values.phaser); break;
        case SynthEffectType::flanger: flanger.process (left, right, numSamples, values.flanger); break;
        case SynthEffectType::delay: delay.process (left, right, numSamples, values.delay, bpm); break;
        case SynthEffectType::reverb: reverb.process (left, right, numSamples, values.reverb); break;
        case SynthEffectType::distortion: distortion.process (left, right, numSamples, values.distortion); break;
        case SynthEffectType::equalizer: equalizer.process (left, right, numSamples, values.equalizer); break;
        case SynthEffectType::compressor: compressor.process (left, right, numSamples, values.compressor); break;
        case SynthEffectType::surround: surround.process (left, right, numSamples, values.surround); break;
        case SynthEffectType::waveshaper: waveshaper.process (left, right, numSamples, values.waveshaper); break;
    }
}

void SynthEffectsRack::resetEffect (SynthEffectType type) noexcept
{
    switch (type)
    {
        case SynthEffectType::bitcrusher: bitcrusher.reset(); break;
        case SynthEffectType::chorus: chorus.reset(); break;
        case SynthEffectType::phaser: phaser.reset(); break;
        case SynthEffectType::flanger: flanger.reset(); break;
        case SynthEffectType::delay: delay.reset(); break;
        case SynthEffectType::reverb: reverb.reset(); break;
        case SynthEffectType::distortion: distortion.reset(); break;
        case SynthEffectType::equalizer: equalizer.reset(); break;
        case SynthEffectType::compressor: compressor.reset(); break;
        case SynthEffectType::surround: surround.reset(); break;
        case SynthEffectType::waveshaper: waveshaper.reset(); break;
    }
}
