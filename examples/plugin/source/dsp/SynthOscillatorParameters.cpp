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
    const auto hostBase = static_cast<yup::uint32> (1000 + 100 * oscillatorIndex);

    const auto continuous = [&] (const char* id, const char* name, yup::uint32 offset, yup::NormalisableRange<float> range, float defaultValue, Formatter format)
    {
        return createContinuous (prefix + id, module + " " + name, hostBase + offset, module, range, defaultValue, std::move (format));
    };

    const auto linear = [] (float minimum, float maximum) { return yup::NormalisableRange<float> (minimum, maximum); };
    const auto skewed = [] (float minimum, float maximum, float centre)
    {
        yup::NormalisableRange<float> range (minimum, maximum);
        range.setSkewForCentre (centre);
        return range;
    };

    const auto unitless = [] (double value) { return SynthFormat::plain (value, 2); };

    waveform = createChoice (prefix + "waveform", module + " Waveform", hostBase + 0, module, getSynthWaveformNames(), static_cast<int> (defaults.waveform));
    syncMode = createChoice (prefix + "syncMode", module + " Sync", hostBase + 1, module, getSynthSyncModeNames(), static_cast<int> (defaults.syncMode));
    syncRatio = continuous ("syncRatio", "Sync Ratio", 2, linear (1.0f, 8.0f), defaults.syncRatio, SynthFormat::ratio);
    frequencyMode = createChoice (prefix + "frequencyMode", module + " Frequency Mode", hostBase + 3, module, getSynthFrequencyModeNames(), static_cast<int> (defaults.frequencyMode));
    ratio = continuous ("ratio", "Ratio", 4, skewed (0.125f, 32.0f, 2.0f), defaults.ratio, SynthFormat::ratio);
    fine = continuous ("fine", "Fine", 5, linear (-100.0f, 100.0f), defaults.fineCents, SynthFormat::cents);
    fixedHz = continuous ("fixedHz", "Fixed Frequency", 6, skewed (1.0f, 20000.0f, 440.0f), defaults.fixedHz, SynthFormat::hertz);
    level = continuous ("level", "Level", 7, linear (0.0f, 1.0f), defaults.level, SynthFormat::percent);
    pan = continuous ("pan", "Pan", 8, linear (-1.0f, 1.0f), defaults.pan, SynthFormat::bipolar);
    phase = continuous ("phase", "Phase", 9, linear (0.0f, 1.0f), defaults.phase, SynthFormat::percent);
    freeRunning = createChoice (prefix + "freeRunning", module + " Free Running", hostBase + 10, module, { "Off", "On" }, defaults.freeRunning ? 1 : 0);
    toFilter1 = continuous ("toFilter1", "To Filter 1", 11, linear (0.0f, 1.0f), defaults.toFilter1, SynthFormat::percent);
    toFilter2 = continuous ("toFilter2", "To Filter 2", 12, linear (0.0f, 1.0f), defaults.toFilter2, SynthFormat::percent);
    direct = continuous ("direct", "Direct", 13, linear (0.0f, 1.0f), defaults.direct, SynthFormat::percent);
    ridgeSpacing = continuous ("ridges", "Ridges", 14, linear (0.25f, 8.0f), defaults.ridgeSpacing, SynthFormat::ratio);
    color = continuous ("color", "Color", 15, linear (0.0f, 1.0f), defaults.color, SynthFormat::percent);
    dispersion = continuous ("dispersion", "Dispersion", 16, linear (0.01f, 0.99f), defaults.dispersion, unitless);
    squeeze = continuous ("squeeze", "Squeeze", 17, linear (0.0f, 0.5f), defaults.squeeze, SynthFormat::percent);
    squash = continuous ("squash", "Squash", 18, linear (0.1f, 4.0f), defaults.squash, unitless);
    tilt = continuous ("tilt", "Tilt", 19, linear (-4.0f, 4.0f), defaults.tilt, unitless);
    oddEven = continuous ("oddEven", "Odd/Even", 20, linear (0.0f, 1.0f), defaults.oddEven, SynthFormat::percent);
    formant = continuous ("formant", "Formant", 21, linear (-4.0f, 4.0f), defaults.formant, unitless);
    formantPosition = continuous ("formantPosition", "Formant Position", 22, linear (0.0f, 7.0f), defaults.formantPosition, unitless);
    scatter = continuous ("scatter", "Scatter", 23, linear (0.0f, 1.0f), defaults.scatter, SynthFormat::percent);
    enabled = createChoice (prefix + "enabled", module + " Enabled", hostBase + 24, module, { "Off", "On" }, defaults.enabled ? 1 : 0);
}

void SynthOscillatorParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { waveform, syncMode, syncRatio, frequencyMode, ratio, fine, fixedHz, level, pan, phase, freeRunning,
                                   toFilter1, toFilter2, direct, ridgeSpacing, color, dispersion, squeeze, squash, tilt, oddEven,
                                   formant, formantPosition, scatter, enabled })
        processor.addParameter (parameter);
}

SynthOscillatorValues SynthOscillatorParameters::read (SynthParameterReader reader) const noexcept
{
    SynthOscillatorValues values;

    values.waveform = static_cast<yup::Waveform> (reader.asInt (waveform));
    values.syncMode = static_cast<yup::SyncMode> (reader.asInt (syncMode));
    values.syncRatio = reader (syncRatio);
    values.frequencyMode = static_cast<SynthFrequencyMode> (reader.asInt (frequencyMode));
    values.ratio = reader (ratio);
    values.fineCents = reader (fine);
    values.fixedHz = reader (fixedHz);
    values.enabled = reader.asInt (enabled) != 0;
    values.level = reader (level);
    values.pan = reader (pan);
    values.phase = reader (phase);
    values.freeRunning = reader.asInt (freeRunning) != 0;
    values.toFilter1 = reader (toFilter1);
    values.toFilter2 = reader (toFilter2);
    values.direct = reader (direct);
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

    return values;
}

std::vector<yup::AudioParameter::Ptr> SynthOscillatorParameters::getSpectralParameters() const
{
    return { syncRatio, ridgeSpacing, color, dispersion, squeeze, squash, tilt, oddEven, formant, formantPosition, scatter };
}
