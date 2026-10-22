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

#include "SynthFilterParameters.h"
#include "SynthFormat.h"

SynthFilterParameters::SynthFilterParameters (int filterIndex, const SynthFilterValues& defaults)
{
    using namespace SynthParameterFactory;

    const auto prefix = "flt" + yup::String (filterIndex + 1) + ".";
    const auto module = "Filter " + yup::String (filterIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (5000 + 100 * filterIndex);

    yup::NormalisableRange<float> cutoffRange (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (1000.0f);

    type = createChoice (prefix + "type", module + " Type", hostBase + 0, module, getSynthFilterTypeNames(), static_cast<int> (defaults.type));
    cutoff = createContinuous (prefix + "cutoff", module + " Cutoff", hostBase + 1, module, cutoffRange, defaults.cutoff, SynthFormat::hertz);
    resonance = createContinuous (prefix + "resonance", module + " Resonance", hostBase + 2, module, { 0.0f, 1.0f }, defaults.resonance, SynthFormat::percent);
    drive = createContinuous (prefix + "drive", module + " Drive", hostBase + 3, module, { 0.0f, 1.0f }, defaults.drive, SynthFormat::percent);
    keytrack = createContinuous (prefix + "keytrack", module + " Keytrack", hostBase + 4, module, { 0.0f, 1.0f }, defaults.keytrack, SynthFormat::percent);
    mix = createContinuous (prefix + "mix", module + " Mix", hostBase + 5, module, { 0.0f, 1.0f }, defaults.mix, SynthFormat::percent);
}

void SynthFilterParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { type, cutoff, resonance, drive, keytrack, mix })
        processor.addParameter (parameter);
}

SynthFilterValues SynthFilterParameters::read (SynthParameterReader reader) const noexcept
{
    return { static_cast<SynthFilterType> (reader.asInt (type)), reader (cutoff), reader (resonance), reader (drive), reader (keytrack), reader (mix) };
}
