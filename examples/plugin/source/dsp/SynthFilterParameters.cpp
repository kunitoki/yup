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

SynthFilterParameters::SynthFilterParameters()
{
    using namespace SynthParameterFactory;

    const SynthFilterValues defaults;
    const yup::String module ("Filter");

    yup::NormalisableRange<float> cutoffRange (20.0f, 20000.0f);
    cutoffRange.setSkewForCentre (1000.0f);

    type = createChoice ("filter.type", "Filter Type", 300, module, getSynthFilterTypeNames(), static_cast<int> (defaults.type));
    cutoff = createContinuous ("filter.cutoff", "Filter Cutoff", 301, module, cutoffRange, defaults.cutoff, SynthFormat::hertz);
    resonance = createContinuous ("filter.resonance", "Filter Resonance", 302, module, { 0.0f, 1.0f }, defaults.resonance, SynthFormat::percent);
    drive = createContinuous ("filter.drive", "Filter Drive", 303, module, { 0.0f, 1.0f }, defaults.drive, SynthFormat::percent);
    keytrack = createContinuous ("filter.keytrack", "Filter Keytrack", 304, module, { 0.0f, 1.0f }, defaults.keytrack, SynthFormat::percent);
}

void SynthFilterParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { type, cutoff, resonance, drive, keytrack })
        processor.addParameter (parameter);
}

SynthFilterValues SynthFilterParameters::read (SynthParameterReader reader) const noexcept
{
    return { static_cast<SynthFilterType> (reader.asInt (type)), reader (cutoff), reader (resonance), reader (drive), reader (keytrack) };
}
