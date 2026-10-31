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

#include "SynthEnvelopeParameters.h"
#include "SynthFormat.h"

namespace
{
/** Returns the first point at the highest level up to the sustain point, or the end. */
int findPeak (const SynthCurveState::Curve& curve) noexcept
{
    const auto sustain = curve.getSustainIndex();
    const auto last = sustain >= 0 ? sustain : curve.getNumPoints() - 1;
    auto peak = 0;

    for (int index = 1; index <= last; ++index)
        if (curve.getPoint (index).level > curve.getPoint (peak).level)
            peak = index;

    return peak;
}

float getStageScale (const SynthEnvelopeStages& stages, int segment, int peak, int sustain) noexcept
{
    if (segment <= peak)
        return stages.attack;

    return sustain < 0 || segment <= sustain ? stages.decay : stages.release;
}
} // namespace

//==============================================================================
float SynthEnvelopeStages::getScale (const SynthCurveState::Curve& curve, int segment) const noexcept
{
    return getStageScale (*this, segment, findPeak (curve), curve.getSustainIndex());
}

SynthCurveState::Curve SynthEnvelopeStages::apply (const SynthCurveState::Curve& curve) const noexcept
{
    SynthCurveState::Curve result;
    const auto peak = findPeak (curve);
    const auto sustainIndex = curve.getSustainIndex();
    auto time = 0.0f;

    for (int index = 0; index < curve.getNumPoints(); ++index)
    {
        const auto& point = curve.getPoint (index);
        time = index == 0 ? point.time : time + (point.time - curve.getPoint (index - 1).time) * getStageScale (*this, index, peak, sustainIndex);
        result.addPoint (time, index == sustainIndex ? point.level * sustain : point.level, point.curve);
    }

    result.setSustainIndex (sustainIndex);
    result.setLoop (curve.getLoopStart(), curve.getLoopEnd());
    return result;
}

//==============================================================================
SynthEnvelopeParameters::SynthEnvelopeParameters (int envelopeIndex, const SynthEnvelopeValues& defaults)
{
    using namespace SynthParameterFactory;

    const auto prefix = "env" + yup::String (envelopeIndex + 1) + ".";
    const auto module = "Env " + yup::String (envelopeIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (3000 + 100 * envelopeIndex);

    yup::NormalisableRange<float> scaleRange (0.05f, 20.0f);
    scaleRange.setSkewForCentre (1.0f);

    yup::NormalisableRange<float> stageRange (0.1f, 10.0f);
    stageRange.setSkewForCentre (1.0f);

    attack = createContinuous (prefix + "attack", module + " Attack", hostBase + 5, module, stageRange, defaults.stages.attack, SynthFormat::ratio);
    decay = createContinuous (prefix + "decay", module + " Decay", hostBase + 6, module, stageRange, defaults.stages.decay, SynthFormat::ratio);
    sustain = createContinuous (prefix + "sustain", module + " Sustain", hostBase + 7, module, { 0.0f, 2.0f }, defaults.stages.sustain, SynthFormat::percent);
    release = createContinuous (prefix + "release", module + " Release", hostBase + 8, module, stageRange, defaults.stages.release, SynthFormat::ratio);
    timeScale = createContinuous (prefix + "timeScale", module + " Time Scale", hostBase + 0, module, scaleRange, defaults.timeScale, SynthFormat::ratio);
    velocity = createContinuous (prefix + "velocity", module + " Velocity", hostBase + 1, module, { 0.0f, 1.0f }, defaults.velocity, SynthFormat::percent);
    depth = createContinuous (prefix + "depth", module + " Depth", hostBase + 2, module, { 0.0f, 1.0f }, defaults.depth, SynthFormat::percent);

    if (envelopeIndex > 0)
    {
        enabled = createChoice (prefix + "enabled", module + " Enabled", hostBase + 3, module, { "Off", "On" }, defaults.enabled ? 1 : 0);
        polarity = createChoice (prefix + "polarity", module + " Polarity", hostBase + 9, module, { "Unipolar", "Bipolar" }, defaults.bipolar ? 1 : 0);
    }

    tempoSync = createChoice (prefix + "sync", module + " Sync", hostBase + 4, module, { "Off", "On" }, defaults.tempoSync ? 1 : 0);
}

void SynthEnvelopeParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { timeScale, velocity, depth })
        processor.addParameter (parameter);

    if (enabled != nullptr)
        processor.addParameter (enabled);

    processor.addParameter (tempoSync);

    for (const auto& parameter : { attack, decay, sustain, release })
        processor.addParameter (parameter);

    if (polarity != nullptr)
        processor.addParameter (polarity);
}

SynthEnvelopeValues SynthEnvelopeParameters::read (SynthParameterReader reader) const noexcept
{
    return { reader (timeScale),
             reader (velocity),
             reader (depth),
             enabled == nullptr || reader.asInt (enabled) != 0,
             reader.asInt (tempoSync) != 0,
             { reader (attack), reader (decay), reader (sustain), reader (release) },
             polarity != nullptr && reader.asInt (polarity) != 0 };
}
