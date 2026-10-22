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

#include "SynthParameters.h"
#include "SynthFormat.h"

namespace
{
SynthOscillatorValues makeOscillatorDefaults (int index)
{
    SynthOscillatorValues values;

    if (index == 0)
    {
        values.color = 0.2f;
        return values;
    }

    values.waveform = yup::Waveform::triangle;
    values.syncMode = yup::SyncMode::hard;
    values.octave = -1;
    values.level = 0.3f;
    return values;
}

SynthEnvelopeValues makeEnvelopeDefaults (int index)
{
    SynthEnvelopeValues values;

    if (index == 0)
        return values;

    values.attack = 0.2f;
    values.decay = 0.5f;
    values.sustain = 0.3f;
    values.release = 0.5f;
    return values;
}
} // namespace

SynthParameters::SynthParameters()
    : oscillators { SynthOscillatorParameters (0, makeOscillatorDefaults (0)), SynthOscillatorParameters (1, makeOscillatorDefaults (1)) }
    , envelopes { SynthEnvelopeParameters (0, makeEnvelopeDefaults (0)), SynthEnvelopeParameters (1, makeEnvelopeDefaults (1)) }
    , lfos { SynthLFOParameters (0), SynthLFOParameters (1) }
{
    using namespace SynthParameterFactory;

    volume = createContinuous ("master.volume", "Master Volume", 1, "Master", { 0.0f, 1.0f }, 0.5f, SynthFormat::percent);
    playMode = createChoice ("voice.mode", "Voice Mode", 2, "Voice", getSynthPlayModeNames(), static_cast<int> (SynthPlayMode::poly));
    glide = createContinuous ("voice.glide", "Glide", 3, "Voice", { 0.0f, 2000.0f }, 120.0f, [] (double value) { return SynthFormat::milliseconds (value * 0.001); });
}

void SynthParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { volume, playMode, glide })
        processor.addParameter (parameter);

    for (const auto& oscillator : oscillators)
        oscillator.addTo (processor);

    filter.addTo (processor);

    for (const auto& envelope : envelopes)
        envelope.addTo (processor);

    for (const auto& lfo : lfos)
        lfo.addTo (processor);

    modulation.addTo (processor);
}
