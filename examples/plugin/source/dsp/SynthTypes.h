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

#include <array>
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
/** How an operator's frequency follows the keyboard. */
enum class SynthFrequencyMode
{
    ratio, /**< A multiple of the played note, as on the TX81Z. */
    fixed  /**< A frequency in Hz, whatever note is played. */
};

/** @internal Item names for SynthFrequencyMode, index aligned with the enumeration. */
inline yup::StringArray getSynthFrequencyModeNames()
{
    return { "Ratio", "Fixed" };
}

//==============================================================================
/** The internal rate of the operator network, as a multiple of the host rate. */
enum class SynthQuality
{
    x2,
    x4
};

/** @internal Item names for SynthQuality, index aligned with the enumeration. */
inline yup::StringArray getSynthQualityNames()
{
    return { "2x", "4x" };
}

/** Returns the oversampling factor of a quality setting. */
constexpr int getOversamplingFactor (SynthQuality quality) noexcept
{
    return quality == SynthQuality::x2 ? 2 : 4;
}

//==============================================================================
/** The responses a voice filter can select. */
enum class SynthFilterType
{
    lowpass12,
    lowpass24,
    highpass12,
    highpass24,
    bandpass12,
    bandpass24,
    notch,
    peak,
    allpass,
    moogLadder,
    korg35,
    diode,
    vowel,
    comb
};

/** @internal Item names for SynthFilterType, index aligned with the enumeration. */
inline yup::StringArray getSynthFilterTypeNames()
{
    return { "LP 12", "LP 24", "HP 12", "HP 24", "BP 12", "BP 24", "Notch", "Peak", "Allpass",
             "Moog", "Korg 35", "Diode", "Vowel", "Comb" };
}

/** True for the types played by two cascaded state variable filters. */
constexpr bool isCascadedFilter (SynthFilterType type) noexcept
{
    return type == SynthFilterType::lowpass24 || type == SynthFilterType::highpass24 || type == SynthFilterType::bandpass24;
}

/** True for the state variable filter types, as opposed to the analog models and the comb. */
constexpr bool isStateVariableFilter (SynthFilterType type) noexcept
{
    return type < SynthFilterType::moogLadder;
}

/** Maps a state variable type onto the yup::FilterMode the VAStateVariableFilter reads. */
inline yup::FilterModeType toFilterMode (SynthFilterType type) noexcept
{
    switch (type)
    {
        case SynthFilterType::highpass12:
        case SynthFilterType::highpass24: return yup::FilterMode::highpass;
        case SynthFilterType::bandpass12:
        case SynthFilterType::bandpass24: return yup::FilterMode::bandpassCsg;
        case SynthFilterType::notch: return yup::FilterMode::bandstop;
        case SynthFilterType::peak: return yup::FilterMode::peak;
        case SynthFilterType::allpass: return yup::FilterMode::allpass;
        default: break;
    }

    return yup::FilterMode::lowpass;
}

/** How the two voice filters are wired. */
enum class SynthFilterRouting
{
    serial,  /**< Filter 2 also receives filter 1's output. */
    parallel /**< Each filter only receives its own sends. */
};

/** @internal Item names for SynthFilterRouting, index aligned with the enumeration. */
inline yup::StringArray getSynthFilterRoutingNames()
{
    return { "Serial", "Parallel" };
}

//==============================================================================
/** @internal Item names of the yup::LFO::Shape values offered, index aligned with the enumeration; the custom shape is not offered yet. */
inline yup::StringArray getSynthLFOShapeNames()
{
    return { "Sine", "Triangle", "Saw Up", "Square", "S&H", "Saw Down", "Smooth Random" };
}

/** @internal Item names for yup::LFO::RetriggerMode, index aligned with the enumeration. */
inline yup::StringArray getSynthLFORetriggerNames()
{
    return { "Free", "Retrigger", "One Shot" };
}

/** A tempo synced LFO period. */
struct SynthLFODivision
{
    const char* name;
    double beats;
};

/** The divisions a tempo synced LFO can pick, longest first. */
inline const std::array<SynthLFODivision, 14>& getSynthLFODivisions()
{
    static const std::array<SynthLFODivision, 14> divisions { {
        { "8 Bars", 32.0 }, { "4 Bars", 16.0 }, { "2 Bars", 8.0 }, { "1 Bar", 4.0 },
        { "1/2", 2.0 }, { "1/4", 1.0 }, { "1/8", 0.5 }, { "1/16", 0.25 }, { "1/32", 0.125 },
        { "1/4 T", 2.0 / 3.0 }, { "1/8 T", 1.0 / 3.0 }, { "1/16 T", 1.0 / 6.0 },
        { "1/4 D", 1.5 }, { "1/8 D", 0.75 } } };

    return divisions;
}

/** @internal Item names for the LFO divisions. */
inline yup::StringArray getSynthLFODivisionNames()
{
    yup::StringArray names;

    for (const auto& division : getSynthLFODivisions())
        names.add (division.name);

    return names;
}

//==============================================================================
/** Keyboard allocation and envelope behavior. */
enum class SynthPlayMode
{
    poly,  /**< Eight voices with rendered release tails when recycled. */
    mono,  /**< Last-note priority, retriggering the envelopes on each note. */
    legato /**< Overlapping notes preserve phases and envelopes; glide is optional. */
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
    env2, /**< Unipolar. */
    env3,
    env4,
    lfo1, /**< Bipolar. */
    lfo2,
    lfo3,
    lfo4,
    velocity,   /**< The note's velocity, unipolar. */
    note,       /**< The note relative to middle C, bipolar, one unit per five octaves. */
    modWheel,   /**< MIDI controller 1, unipolar. */
    aftertouch, /**< Channel pressure, unipolar. */
    random,     /**< A value drawn per note, bipolar. */
    count
};

/** @internal Short names for SynthModulationSource, index aligned with the enumeration. */
inline yup::StringArray getSynthModulationSourceNames()
{
    return { "ENV 1", "ENV 2", "ENV 3", "ENV 4", "LFO 1", "LFO 2", "LFO 3", "LFO 4",
             "VELOCITY", "NOTE", "MOD WHEEL", "AFTERTOUCH", "RANDOM" };
}

/** True for the sources that swing both ways, so a route from them reaches depth either side. */
constexpr bool isBipolarSource (SynthModulationSource source) noexcept
{
    return (source >= SynthModulationSource::lfo1 && source <= SynthModulationSource::lfo4)
        || source == SynthModulationSource::note
        || source == SynthModulationSource::random;
}

/** Returns the envelope a source reads, or -1. */
constexpr int getSourceEnvelope (SynthModulationSource source) noexcept
{
    const auto index = static_cast<int> (source) - static_cast<int> (SynthModulationSource::env1);
    return index >= 0 && index < SynthExample::envelopeCount ? index : -1;
}

/** Returns the LFO a source reads, or -1. */
constexpr int getSourceLFO (SynthModulationSource source) noexcept
{
    const auto index = static_cast<int> (source) - static_cast<int> (SynthModulationSource::lfo1);
    return index >= 0 && index < SynthExample::lfoCount ? index : -1;
}
