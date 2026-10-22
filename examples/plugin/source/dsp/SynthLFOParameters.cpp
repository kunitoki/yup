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
    const auto hostBase = static_cast<yup::uint32> (600 + 100 * lfoIndex);

    yup::NormalisableRange<float> rateRange (0.01f, 20.0f);
    rateRange.setSkewForCentre (1.0f);

    shape = createChoice (prefix + "shape", module + " Shape", hostBase + 0, module, getSynthLFOShapeNames(), static_cast<int> (defaults.shape));
    rate = createContinuous (prefix + "rate", module + " Rate", hostBase + 1, module, rateRange, defaults.rate, SynthFormat::hertz);
    phase = createContinuous (prefix + "phase", module + " Phase", hostBase + 2, module, { 0.0f, 1.0f }, defaults.phase, SynthFormat::percent);
    retrigger = createChoice (prefix + "retrigger", module + " Retrigger", hostBase + 3, module, { "Off", "On" }, defaults.retrigger ? 1 : 0);
}

void SynthLFOParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { shape, rate, phase, retrigger })
        processor.addParameter (parameter);
}

SynthLFOValues SynthLFOParameters::read (SynthParameterReader reader) const noexcept
{
    return { static_cast<yup::LFO<float>::Shape> (reader.asInt (shape)), reader (rate), reader (phase), reader.asInt (retrigger) != 0 };
}
