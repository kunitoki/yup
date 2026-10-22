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

#include "SynthOscillatorParameters.h"
#include "SynthFormat.h"

SynthOscillatorParameters::SynthOscillatorParameters (int oscillatorIndex, const SynthOscillatorValues& defaults)
{
    using namespace SynthParameterFactory;

    const auto prefix = "osc" + yup::String (oscillatorIndex + 1) + ".";
    const auto module = "Osc " + yup::String (oscillatorIndex + 1);
    const auto hostBase = static_cast<yup::uint32> (100 * (oscillatorIndex + 1));

    const auto continuous = [&] (const char* id, const char* name, yup::uint32 offset, float minimum, float maximum, float defaultValue, Formatter format)
    {
        return createContinuous (prefix + id, module + " " + name, hostBase + offset, module, yup::NormalisableRange<float> (minimum, maximum), defaultValue, std::move (format));
    };

    const auto unitless = [] (double value) { return SynthFormat::plain (value, 2); };

    waveform = createChoice (prefix + "waveform", module + " Waveform", hostBase + 0, module, getSynthWaveformNames(), static_cast<int> (defaults.waveform));
    syncMode = createChoice (prefix + "syncMode", module + " Sync", hostBase + 1, module, getSynthSyncModeNames(), static_cast<int> (defaults.syncMode));
    syncRatio = continuous ("syncRatio", "Sync Ratio", 2, 1.0f, 8.0f, defaults.syncRatio, SynthFormat::ratio);
    level = continuous ("level", "Level", 3, 0.0f, 1.0f, defaults.level, SynthFormat::percent);
    octave = createInteger (prefix + "octave", module + " Octave", hostBase + 4, module, -3, 3, defaults.octave, SynthFormat::octaves);
    detune = continuous ("detune", "Cents", 5, -100.0f, 100.0f, defaults.detuneSemitones * 100.0f, SynthFormat::cents);
    ridgeSpacing = continuous ("ridges", "Ridges", 6, 0.25f, 8.0f, defaults.ridgeSpacing, SynthFormat::ratio);
    color = continuous ("color", "Color", 7, 0.0f, 1.0f, defaults.color, SynthFormat::percent);
    dispersion = continuous ("dispersion", "Dispersion", 8, 0.01f, 0.99f, defaults.dispersion, unitless);
    squeeze = continuous ("squeeze", "Squeeze", 9, 0.0f, 0.5f, defaults.squeeze, SynthFormat::percent);
    squash = continuous ("squash", "Squash", 10, 0.1f, 4.0f, defaults.squash, unitless);
    tilt = continuous ("tilt", "Tilt", 11, -4.0f, 4.0f, defaults.tilt, unitless);
    oddEven = continuous ("oddEven", "Odd/Even", 12, 0.0f, 1.0f, defaults.oddEven, SynthFormat::percent);
    formant = continuous ("formant", "Formant", 13, -4.0f, 4.0f, defaults.formant, unitless);
    formantPosition = continuous ("formantPosition", "Formant Position", 14, 0.0f, 7.0f, defaults.formantPosition, unitless);
    scatter = continuous ("scatter", "Scatter", 15, 0.0f, 1.0f, defaults.scatter, SynthFormat::percent);
    unisonVoices = createInteger (prefix + "unison", module + " Unison", hostBase + 16, module, 1, SynthExample::maxUnisonVoices, defaults.unisonVoices, SynthFormat::count);
    unisonDetune = continuous ("unisonDetune", "Unison Detune", 17, 0.0f, 1.0f, defaults.unisonDetune, SynthFormat::percent);
    unisonSpread = continuous ("unisonSpread", "Unison Spread", 18, 0.0f, 1.0f, defaults.unisonSpread, SynthFormat::percent);
}

void SynthOscillatorParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { waveform, syncMode, syncRatio, level, octave, detune, ridgeSpacing, color, dispersion,
                                   squeeze, squash, tilt, oddEven, formant, formantPosition, scatter,
                                   unisonVoices, unisonDetune, unisonSpread })
        processor.addParameter (parameter);
}

SynthOscillatorValues SynthOscillatorParameters::read (SynthParameterReader reader) const noexcept
{
    SynthOscillatorValues values;

    values.waveform = static_cast<yup::Waveform> (reader.asInt (waveform));
    values.syncMode = static_cast<yup::SyncMode> (reader.asInt (syncMode));
    values.syncRatio = reader (syncRatio);
    values.level = reader (level);
    values.octave = reader.asInt (octave);
    values.detuneSemitones = reader (detune) * 0.01f;
    values.ridgeSpacing = reader (ridgeSpacing);
    values.color = reader (color);
    values.dispersion = reader (dispersion);
    values.squeeze = reader (squeeze);
    values.squash = reader (squash);
    values.tilt = reader (tilt);
    values.oddEven = reader (oddEven);
    values.formant = reader (formant);
    values.formantPosition = reader (formantPosition);
    values.scatter = reader (scatter);
    values.unisonVoices = reader.asInt (unisonVoices);
    values.unisonDetune = reader (unisonDetune);
    values.unisonSpread = reader (unisonSpread);

    return values;
}
