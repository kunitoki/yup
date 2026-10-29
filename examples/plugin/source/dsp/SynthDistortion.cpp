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

#include "SynthDistortion.h"
#include "SynthEffectTools.h"

namespace
{
constexpr double levelSeconds = 0.2;
constexpr double dcBlockerHz = 10.0;
constexpr float powerFloor = 1e-8f;
constexpr float minimumLevel = 1.0f / 64.0f;
constexpr float maximumLevel = 4.0f;
} // namespace

//==============================================================================
void SynthDistortion::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    levelCoefficient = static_cast<float> (1.0 - std::exp (-1.0 / (levelSeconds * sampleRate)));

    for (auto& channel : channels)
    {
        channel.soft.prepare (sampleRate, 1);
        channel.cubic.prepare (sampleRate, 1);
        channel.hard.prepare (sampleRate, 1);
        channel.dry.prepare (sampleRate, 1);

        channel.dcBlocker.prepare (sampleRate, 1);
        channel.dcBlocker.setParameters (yup::FilterMode::highpass, dcBlockerHz, 0.0, sampleRate);
        channel.tone.prepare (sampleRate, 1);
    }

    for (auto* value : { &drive, &bias, &tone, &output, &mix })
        value->reset (sampleRate, SynthEffectTools::smoothingSeconds);

    reset();
}

void SynthDistortion::reset() noexcept
{
    for (auto* value : { &drive, &bias, &tone, &output, &mix })
        value->setCurrentAndTargetValue (value->getTargetValue());

    for (auto& channel : channels)
    {
        channel.soft.reset();
        channel.cubic.reset();
        channel.hard.reset();
        channel.dry.reset();
        channel.dcBlocker.reset();
        channel.tone.reset();
    }

    dryPower = wetPower = 0.0f;
}

float SynthDistortion::curve (SynthDistortionType type, float x) noexcept
{
    switch (type)
    {
        case SynthDistortionType::cubic: return CubicTraits::f (x);
        case SynthDistortionType::hard: return yup::HardClipperTraits::f (x);
        case SynthDistortionType::soft: break;
    }

    return yup::TanhClipperTraits::f (x);
}

float SynthDistortion::shape (Channel& channel, float input) noexcept
{
    switch (type)
    {
        case SynthDistortionType::cubic: return channel.cubic.processSample (input);
        case SynthDistortionType::hard: return channel.hard.processSample (input);
        case SynthDistortionType::soft: break;
    }

    return channel.soft.processSample (input);
}

void SynthDistortion::process (float* left, float* right, int numSamples, const SynthDistortionValues& values) noexcept
{
    // A new curve starts from silence rather than from the old curve's history.
    if (values.type != type)
    {
        type = values.type;

        for (auto& channel : channels)
        {
            channel.soft.reset();
            channel.cubic.reset();
            channel.hard.reset();
        }
    }

    drive.setTargetValue (yup::Decibels::decibelsToGain (values.driveDb));
    bias.setTargetValue (values.bias);
    tone.setTargetValue (values.tone);
    output.setTargetValue (yup::Decibels::decibelsToGain (values.outputDb));
    mix.setTargetValue (values.mix);

    // The tone follows its smoothing once per chunk; the chunks are short.
    const auto toneHz = tone.skip (numSamples);

    for (auto& channel : channels)
        channel.tone.setParameters (yup::FilterMode::lowpass, toneHz, 0.0, sampleRate);

    float* buffers[] = { left, right };

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = drive.getNextValue();
        const auto offset = bias.getNextValue();
        const auto offsetLevel = curve (type, offset);
        const auto outputGain = output.getNextValue();
        const auto wetAmount = mix.getNextValue();

        std::array<float, 2> dry {}, wet {};
        auto dryEnergy = 0.0f, wetEnergy = 0.0f;

        for (std::size_t c = 0; c < channels.size(); ++c)
        {
            auto& channel = channels[c];
            const auto input = buffers[c][i];

            dry[c] = channel.dry.processSample (input);
            wet[c] = channel.dcBlocker.processSample (shape (channel, gain * input + offset) - offsetLevel);

            dryEnergy += dry[c] * dry[c];
            wetEnergy += wet[c] * wet[c];
        }

        dryPower += levelCoefficient * (dryEnergy - dryPower);
        wetPower += levelCoefficient * (wetEnergy - wetPower);

        const auto level = yup::jlimit (minimumLevel, maximumLevel, std::sqrt ((dryPower + powerFloor) / (wetPower + powerFloor)));

        for (std::size_t c = 0; c < channels.size(); ++c)
        {
            const auto shaped = outputGain * channels[c].tone.processSample (level * wet[c]);
            buffers[c][i] = dry[c] + wetAmount * (shaped - dry[c]);
        }
    }
}
