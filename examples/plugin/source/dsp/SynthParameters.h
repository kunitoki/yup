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

#include "SynthEnvelopeParameters.h"
#include "SynthFMParameters.h"
#include "SynthFilterParameters.h"
#include "SynthLFOParameters.h"
#include "SynthModulationParameters.h"
#include "SynthOscillatorParameters.h"

//==============================================================================
/** A plain snapshot of the voice wide controls. */
struct SynthGlobalValues
{
    SynthPlayMode mode = SynthPlayMode::poly;
    float glideSeconds = 0.12f;
    int unisonVoices = 1;
    float unisonDetune = 0.2f; /**< Semitones between the outermost layers and the center. */
    float unisonSpread = 0.6f;
    SynthFilterRouting filterRouting = SynthFilterRouting::serial;

    bool operator== (const SynthGlobalValues&) const = default;
};

//==============================================================================
/** Every parameter of the synthesiser, grouped by section.

    The parameters are created here with their IDs, host IDs, ranges and defaults, and
    registered with the processor by addTo(). Defaults are what the Init preset plays.

    Host IDs are grouped per section so the sections can grow: globals below 100,
    oscillators from 1000, FM from 2000, envelopes from 3000, LFOs from 4000, filters
    from 5000 and route depths from 6000.

    @see SynthPatch, HarmonicSynthEngine
*/
class SynthParameters
{
public:
    /** Creates every parameter. */
    SynthParameters();

    /** Registers every parameter with the processor. Call once, from its constructor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot of the voice wide controls. */
    SynthGlobalValues readGlobals (SynthParameterReader reader = {}) const noexcept;

    /** Returns the quality the network runs at. Read when the processor prepares. */
    SynthQuality getQuality() const noexcept;

    std::array<SynthOscillatorParameters, SynthExample::oscillatorCount> oscillators;
    SynthFMParameters fm;
    std::array<SynthFilterParameters, SynthExample::filterCount> filters;
    std::array<SynthEnvelopeParameters, SynthExample::envelopeCount> envelopes;
    std::array<SynthLFOParameters, SynthExample::lfoCount> lfos;
    SynthModulationParameters modulation;

    yup::AudioParameter::Ptr volume;        /**< Master output gain, 0 to 1. */
    yup::AudioParameter::Ptr playMode;      /**< A SynthPlayMode. */
    yup::AudioParameter::Ptr glide;         /**< Portamento time in milliseconds, used by mono and legato. */
    yup::AudioParameter::Ptr quality;       /**< A SynthQuality. Not automatable: it changes the latency. */
    yup::AudioParameter::Ptr unisonVoices;  /**< Copies of the operator network per voice. */
    yup::AudioParameter::Ptr unisonDetune;  /**< Detune between the copies, 0 to 1 semitone. */
    yup::AudioParameter::Ptr unisonSpread;  /**< Stereo spread of the copies. */
    yup::AudioParameter::Ptr filterRouting; /**< A SynthFilterRouting. */
};
