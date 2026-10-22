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
    const auto prefix = "env" + yup::String (envelopeIndex + 1) + ".";
    const auto module = "Env " + yup::String (envelopeIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (400 + 100 * envelopeIndex);

    const auto create = [&] (const char* id, const char* name, yup::uint32 offset, float minimum, float maximum, float defaultValue, SynthParameterFactory::Formatter format)
    {
        return SynthParameterFactory::createContinuous (prefix + id, module + " " + name, hostBase + offset, module,
                                                        yup::NormalisableRange<float> (minimum, maximum), defaultValue, std::move (format));
    };

    delay = create ("delay", "Delay", 0, 0.0f, 2.0f, defaults.delay, SynthFormat::milliseconds);
    attack = create ("attack", "Attack", 1, 0.001f, 4.0f, defaults.attack, SynthFormat::milliseconds);
    hold = create ("hold", "Hold", 2, 0.0f, 2.0f, defaults.hold, SynthFormat::milliseconds);
    decay = create ("decay", "Decay", 3, 0.001f, 4.0f, defaults.decay, SynthFormat::milliseconds);
    sustain = create ("sustain", "Sustain", 4, 0.0f, 1.0f, defaults.sustain, SynthFormat::percent);
    release = create ("release", "Release", 5, 0.001f, 8.0f, defaults.release, SynthFormat::milliseconds);
}

void SynthEnvelopeParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { delay, attack, hold, decay, sustain, release })
        processor.addParameter (parameter);
}

SynthEnvelopeValues SynthEnvelopeParameters::read (SynthParameterReader reader) const noexcept
{
    return { reader (delay), reader (attack), reader (hold), reader (decay), reader (sustain), reader (release) };
}
