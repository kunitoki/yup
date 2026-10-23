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

#include <set>

namespace
{
SynthOscillatorValues makeOscillatorDefaults (int index)
{
    SynthOscillatorValues values;

    if (index == 0)
    {
        values.level = 0.8f;
        values.color = 0.2f;
        return values;
    }

    // The other operators start as silent sines, ready to modulate.
    values.waveform = yup::Waveform::sine;
    return values;
}

SynthFilterValues makeFilterDefaults (int index)
{
    SynthFilterValues values;

    if (index > 0)
        values.enabled = false;

    return values;
}
} // namespace

SynthParameters::SynthParameters()
    : oscillators { SynthOscillatorParameters (0, makeOscillatorDefaults (0)), SynthOscillatorParameters (1, makeOscillatorDefaults (1)),
                    SynthOscillatorParameters (2, makeOscillatorDefaults (2)), SynthOscillatorParameters (3, makeOscillatorDefaults (3)) }
    , filters { SynthFilterParameters (0, makeFilterDefaults (0)), SynthFilterParameters (1, makeFilterDefaults (1)) }
    , envelopes { SynthEnvelopeParameters (0, { 1.0f, 1.0f, 1.0f }), SynthEnvelopeParameters (1, {}), SynthEnvelopeParameters (2, {}), SynthEnvelopeParameters (3, {}) }
    , lfos { SynthLFOParameters (0), SynthLFOParameters (1), SynthLFOParameters (2), SynthLFOParameters (3) }
{
    using namespace SynthParameterFactory;

    const SynthGlobalValues defaults;

    volume = createContinuous ("global.volume", "Master Volume", 1, "Global", { 0.0f, 1.0f }, 0.5f, SynthFormat::percent);
    playMode = createChoice ("global.mode", "Voice Mode", 2, "Global", getSynthPlayModeNames(), static_cast<int> (defaults.mode));
    glide = createContinuous ("global.glide", "Glide", 3, "Global", { 0.0f, 2000.0f }, defaults.glideSeconds * 1000.0f, [] (double value) { return SynthFormat::milliseconds (value * 0.001); });

    quality = yup::AudioParameterBuilder()
                  .withID ("global.quality")
                  .withName ("Quality")
                  .withHostID (4)
                  .withModulePath ("Global")
                  .withRange (yup::NormalisableRange<float> (0.0f, 1.0f, 1.0f))
                  .withDefault (static_cast<float> (SynthQuality::x4))
                  .withStepped (true)
                  .withEnum (true)
                  .withAutomatable (false)
                  .withValueToString ([] (float value) { return getSynthQualityNames()[yup::roundToInt (value)]; })
                  .build();

    unisonVoices = createInteger ("global.unisonVoices", "Unison Voices", 5, "Global", 1, SynthExample::maxUnisonVoices, defaults.unisonVoices, SynthFormat::count);
    unisonDetune = createContinuous ("global.unisonDetune", "Unison Detune", 6, "Global", { 0.0f, 1.0f }, defaults.unisonDetune, SynthFormat::percent);
    unisonSpread = createContinuous ("global.unisonSpread", "Unison Spread", 7, "Global", { 0.0f, 1.0f }, defaults.unisonSpread, SynthFormat::percent);
    filterRouting = createChoice ("global.filterRouting", "Filter Routing", 8, "Global", getSynthFilterRoutingNames(), static_cast<int> (defaults.filterRouting));

    for (std::size_t index = 0; index < macros.size(); ++index)
    {
        const auto number = yup::String (static_cast<int> (index) + 1);
        macros[index] = createContinuous ("macro" + number, "Macro " + number, 7000 + static_cast<yup::uint32> (index), "Macros", { 0.0f, 1.0f }, 0.0f, SynthFormat::percent);
    }
}

void SynthParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : { volume, playMode, glide, quality, unisonVoices, unisonDetune, unisonSpread, filterRouting })
        processor.addParameter (parameter);

    for (const auto& oscillator : oscillators)
        oscillator.addTo (processor);

    fm.addTo (processor);

    for (const auto& filter : filters)
        filter.addTo (processor);

    for (const auto& envelope : envelopes)
        envelope.addTo (processor);

    for (const auto& lfo : lfos)
        lfo.addTo (processor);

    modulation.addTo (processor);

    for (const auto& macro : macros)
        processor.addParameter (macro);

#if YUP_DEBUG
    // A host ID used twice silently breaks automation in CLAP and VST3 hosts.
    std::set<yup::uint32> hostIDs;

    for (const auto& parameter : processor.getParameters())
        jassert (hostIDs.insert (parameter->getHostParameterID()).second);
#endif
}

SynthGlobalValues SynthParameters::readGlobals (SynthParameterReader reader) const noexcept
{
    SynthGlobalValues values;

    values.mode = static_cast<SynthPlayMode> (yup::jlimit (0, 2, reader.asInt (playMode)));
    values.glideSeconds = reader (glide) * 0.001f;
    values.unisonVoices = yup::jlimit (1, SynthExample::maxUnisonVoices, reader.asInt (unisonVoices));
    values.unisonDetune = reader (unisonDetune);
    values.unisonSpread = reader (unisonSpread);
    values.filterRouting = static_cast<SynthFilterRouting> (reader.asInt (filterRouting));

    return values;
}

SynthQuality SynthParameters::getQuality() const noexcept
{
    return static_cast<SynthQuality> (yup::jlimit (0, 1, yup::roundToInt (quality->getValue())));
}
