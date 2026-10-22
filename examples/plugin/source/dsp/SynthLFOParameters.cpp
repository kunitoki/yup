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

#include "SynthLFOParameters.h"
#include "SynthFormat.h"

SynthLFOParameters::SynthLFOParameters (int lfoIndex)
{
    using namespace SynthParameterFactory;

    const SynthLFOValues defaults;
    const auto prefix = "lfo" + yup::String (lfoIndex + 1) + ".";
    const auto module = "LFO " + yup::String (lfoIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (4000 + 100 * lfoIndex);

    yup::NormalisableRange<float> rateRange (0.01f, 50.0f);
    rateRange.setSkewForCentre (1.0f);

    yup::NormalisableRange<float> smoothingRange (0.0f, 0.5f);
    smoothingRange.setSkewForCentre (0.05f);

    shape = createChoice (prefix + "shape", module + " Shape", hostBase + 0, module, getSynthLFOShapeNames(), static_cast<int> (defaults.shape));
    rate = createContinuous (prefix + "rate", module + " Rate", hostBase + 1, module, rateRange, defaults.rate, SynthFormat::hertz);
    tempoSync = createChoice (prefix + "sync", module + " Sync", hostBase + 2, module, { "Off", "On" }, defaults.tempoSync ? 1 : 0);
    division = createChoice (prefix + "division", module + " Division", hostBase + 3, module, getSynthLFODivisionNames(), defaults.division);
    phase = createContinuous (prefix + "phase", module + " Phase", hostBase + 4, module, { 0.0f, 1.0f }, defaults.phase, SynthFormat::percent);
    smoothing = createContinuous (prefix + "smooth", module + " Smooth", hostBase + 5, module, smoothingRange, defaults.smoothing, SynthFormat::milliseconds);
    retrigger = createChoice (prefix + "retrigger", module + " Retrigger", hostBase + 6, module, getSynthLFORetriggerNames(), static_cast<int> (defaults.retrigger));
    depth = createContinuous (prefix + "depth", module + " Depth", hostBase + 7, module, { 0.0f, 1.0f }, defaults.depth, SynthFormat::percent);
}

void SynthLFOParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { shape, rate, tempoSync, division, phase, smoothing, retrigger, depth })
        processor.addParameter (parameter);
}

SynthLFOValues SynthLFOParameters::read (SynthParameterReader reader) const noexcept
{
    SynthLFOValues values;

    values.shape = static_cast<SynthLFOValues::Shape> (reader.asInt (shape));
    values.rate = reader (rate);
    values.tempoSync = reader.asInt (tempoSync) != 0;
    values.division = reader.asInt (division);
    values.phase = reader (phase);
    values.smoothing = reader (smoothing);
    values.retrigger = static_cast<SynthLFOValues::RetriggerMode> (reader.asInt (retrigger));
    values.depth = reader (depth);

    return values;
}
