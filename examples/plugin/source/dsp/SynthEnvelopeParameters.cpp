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

SynthEnvelopeParameters::SynthEnvelopeParameters (int envelopeIndex, const SynthEnvelopeValues& defaults)
{
    using namespace SynthParameterFactory;

    const auto prefix = "env" + yup::String (envelopeIndex + 1) + ".";
    const auto module = "Env " + yup::String (envelopeIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (3000 + 100 * envelopeIndex);

    yup::NormalisableRange<float> scaleRange (0.05f, 20.0f);
    scaleRange.setSkewForCentre (1.0f);

    timeScale = createContinuous (prefix + "timeScale", module + " Time Scale", hostBase + 0, module, scaleRange, defaults.timeScale, SynthFormat::ratio);
    velocity = createContinuous (prefix + "velocity", module + " Velocity", hostBase + 1, module, { 0.0f, 1.0f }, defaults.velocity, SynthFormat::percent);
    depth = createContinuous (prefix + "depth", module + " Depth", hostBase + 2, module, { 0.0f, 1.0f }, defaults.depth, SynthFormat::percent);

    if (envelopeIndex > 0)
        enabled = createChoice (prefix + "enabled", module + " Enabled", hostBase + 3, module, { "Off", "On" }, defaults.enabled ? 1 : 0);

    tempoSync = createChoice (prefix + "sync", module + " Sync", hostBase + 4, module, { "Off", "On" }, defaults.tempoSync ? 1 : 0);
}

void SynthEnvelopeParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { timeScale, velocity, depth })
        processor.addParameter (parameter);

    if (enabled != nullptr)
        processor.addParameter (enabled);

    processor.addParameter (tempoSync);
}

SynthEnvelopeValues SynthEnvelopeParameters::read (SynthParameterReader reader) const noexcept
{
    return { reader (timeScale), reader (velocity), reader (depth), enabled == nullptr || reader.asInt (enabled) != 0, reader.asInt (tempoSync) != 0 };
}
