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

#pragma once

#include <yup_dsp/yup_dsp.h>

#include "SynthConstants.h"

#include <cmath>

//==============================================================================
/** @internal Item names for yup::Waveform, index aligned with the enumeration. */
inline yup::StringArray getSynthWaveformNames()
{
    return { "Sine", "Cosine", "Sawtooth", "Square", "Triangle", "Pulse" };
}

/** @internal Item names for yup::SyncMode, index aligned with the enumeration. */
inline yup::StringArray getSynthSyncModeNames()
{
    return { "None", "Hard", "Mirrored", "Pulsar" };
}

//==============================================================================
/** The response the per-voice filter selects, Off skipping the stage entirely. */
enum class SynthFilterType
{
    off,
    lowpass,
    highpass,
    bandpass,
    notch,
    allpass,
    peak
};

/** @internal Item names for SynthFilterType, index aligned with the enumeration. */
inline yup::StringArray getSynthFilterTypeNames()
{
    return { "Off", "Lowpass", "Highpass", "Bandpass", "Notch", "Allpass", "Peak" };
}

/** Maps a type onto the yup::FilterMode the VAStateVariableFilter reads. */
inline yup::FilterModeType toFilterMode (SynthFilterType type) noexcept
{
    switch (type)
    {
        case SynthFilterType::highpass: return yup::FilterMode::highpass;
        case SynthFilterType::bandpass: return yup::FilterMode::bandpassCsg;
        case SynthFilterType::notch: return yup::FilterMode::bandstop;
        case SynthFilterType::allpass: return yup::FilterMode::allpass;
        case SynthFilterType::peak: return yup::FilterMode::peak;
        case SynthFilterType::off:
        case SynthFilterType::lowpass: break;
    }

    return yup::FilterMode::lowpass;
}

//==============================================================================
/** @internal Item names for yup::LFO::Shape, index aligned with the enumeration. */
inline yup::StringArray getSynthLFOShapeNames()
{
    return { "Sine", "Triangle", "Sawtooth", "Square", "S&H" };
}

//==============================================================================
/** Keyboard allocation and envelope behavior. */
enum class SynthPlayMode
{
    poly,   /**< Eight voices with rendered release tails when recycled. */
    mono,   /**< Last-note priority, retriggering the envelope on each note. */
    legato  /**< Overlapping notes preserve phases and envelope; glide is optional. */
};

/** @internal Item names for SynthPlayMode, index aligned with the enumeration. */
inline yup::StringArray getSynthPlayModeNames()
{
    return { "Poly / 8 voices", "Mono / retrigger", "Legato / glide" };
}

//==============================================================================
/** What can drive a modulation route. */
enum class SynthModulationSource
{
    env1, /**< The amplitude envelope, unipolar. */
    env2, /**< The free envelope, unipolar. */
    lfo1, /**< Bipolar. */
    lfo2,
    modWheel /**< MIDI controller 1, unipolar. */
};

/** @internal Item names for SynthModulationSource, index aligned with the enumeration. */
inline yup::StringArray getSynthModulationSourceNames()
{
    return { "ENV 1", "ENV 2", "LFO 1", "LFO 2", "MOD WHEEL" };
}

/** Every parameter the matrix can reach; the oscillator block repeats per oscillator. */
enum class SynthModulationDestination
{
    none,
    osc1Level, osc1Cents, osc1Ridges, osc1Color, osc1Dispersion, osc1Squeeze, osc1Squash, osc1Tilt,
    osc1OddEven, osc1Formant, osc1FormantPosition, osc1Scatter, osc1SyncRatio, osc1UnisonDetune, osc1UnisonSpread,
    osc2Level, osc2Cents, osc2Ridges, osc2Color, osc2Dispersion, osc2Squeeze, osc2Squash, osc2Tilt,
    osc2OddEven, osc2Formant, osc2FormantPosition, osc2Scatter, osc2SyncRatio, osc2UnisonDetune, osc2UnisonSpread,
    filterCutoff, filterResonance, filterDrive,
    count
};

/** Destinations per oscillator in SynthModulationDestination. */
constexpr int synthOscillatorDestinations = 15;

/** @internal Item names for SynthModulationDestination, index aligned with the enumeration. */
inline yup::StringArray getSynthModulationDestinationNames()
{
    yup::StringArray names { "-" };

    for (int oscillator = 1; oscillator <= SynthExample::oscillatorCount; ++oscillator)
    {
        const auto prefix = yup::String ("OSC ") + yup::String (oscillator) + " ";

        for (const auto* name : { "Level", "Cents", "Ridges", "Color", "Dispersion", "Squeeze", "Squash", "Tilt",
                                  "Odd/Even", "Formant", "F.Pos", "Scatter", "Sync Ratio", "U.Detune", "Spread" })
            names.add (prefix + name);
    }

    names.add ("Filter Cutoff");
    names.add ("Filter Resonance");
    names.add ("Filter Drive");

    return names;
}

/** Returns which oscillator a destination belongs to, or -1 for the filter and none. */
constexpr int getDestinationOscillator (SynthModulationDestination destination) noexcept
{
    const auto index = static_cast<int> (destination) - 1;

    if (index < 0 || index >= SynthExample::oscillatorCount * synthOscillatorDestinations)
        return -1;

    return index / synthOscillatorDestinations;
}

/** True for the parameters that change the derived spectrum rather than how it is played. */
constexpr bool isSpectrumDestination (SynthModulationDestination destination) noexcept
{
    if (getDestinationOscillator (destination) < 0)
        return false;

    const auto field = (static_cast<int> (destination) - 1) % synthOscillatorDestinations;

    return field >= 2 && field <= 12;
}

/** The knob range of a destination, shared by the panel and the modulation mapping. */
struct SynthParameterRange
{
    float minimum = 0.0f;
    float maximum = 1.0f;
    bool logarithmic = false;

    /** Applies a normalized amount, one full range per unit, and clamps. */
    float modulate (float base, float amount) const noexcept
    {
        if (logarithmic)
            return yup::jlimit (minimum, maximum, base * std::exp2 (amount * std::log2 (maximum / minimum)));

        return yup::jlimit (minimum, maximum, base + amount * (maximum - minimum));
    }
};

/** Returns the range a destination's knob spans. */
inline SynthParameterRange getDestinationRange (SynthModulationDestination destination) noexcept
{
    switch (destination)
    {
        case SynthModulationDestination::filterCutoff: return { 20.0f, 20000.0f, true };
        case SynthModulationDestination::filterResonance: return { 0.0f, 1.0f };
        case SynthModulationDestination::filterDrive: return { 0.0f, 1.0f };
        case SynthModulationDestination::none:
        case SynthModulationDestination::count: return {};
        default: break;
    }

    switch ((static_cast<int> (destination) - 1) % synthOscillatorDestinations)
    {
        case 0: return { 0.0f, 1.0f };       // level
        case 1: return { -1.0f, 1.0f };      // cents, stored as semitones
        case 2: return { 0.25f, 8.0f };      // ridges
        case 3: return { 0.0f, 1.0f };       // color
        case 4: return { 0.01f, 0.99f };     // dispersion
        case 5: return { 0.0f, 0.5f };       // squeeze
        case 6: return { 0.1f, 4.0f };       // squash
        case 7: return { -4.0f, 4.0f };      // tilt
        case 8: return { 0.0f, 1.0f };       // odd/even
        case 9: return { -4.0f, 4.0f };      // formant
        case 10: return { 0.0f, 7.0f };      // formant position
        case 11: return { 0.0f, 1.0f };      // scatter
        case 12: return { 1.0f, 8.0f };      // sync ratio
        case 13: return { 0.0f, 1.0f };      // unison detune
        case 14: return { 0.0f, 1.0f };      // unison spread
        default: return {};
    }
}
