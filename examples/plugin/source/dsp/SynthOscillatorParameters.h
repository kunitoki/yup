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

#include "SynthParameterFactory.h"
#include "SynthTypes.h"

//==============================================================================
/** A plain snapshot of one oscillator's controls.

    The audio thread takes one snapshot per control chunk and compares it with the
    values it applied last time, so a control that did not move never costs a spectral
    transform or a table render.

    The edited partials are deliberately not copied here. They live in SynthPartials
    behind harmonicGeneration, so a block only pays for them when the editor moved.
*/
struct SynthOscillatorValues
{
    yup::Waveform waveform = yup::Waveform::sawtooth;
    yup::SyncMode syncMode = yup::SyncMode::none;
    float syncRatio = 1.5f;
    float level = 0.5f;
    int octave = 0;
    float detuneSemitones = 0.0f;
    float ridgeSpacing = 1.5f;
    float color = 0.0f;
    float dispersion = 0.5f;
    float squeeze = 0.0f;
    float squash = 1.0f;
    float tilt = 0.0f;
    float oddEven = 0.5f;
    float formant = 0.0f;
    float formantPosition = 2.0f;
    float scatter = 0.0f;
    int unisonVoices = 1;
    float unisonDetune = 0.2f;
    float unisonSpread = 0.6f;
    float harmonicScale = 1.0f;
    bool usesCustomSeries = false;
    int harmonicGeneration = 0;

    bool operator== (const SynthOscillatorValues&) const = default;
};

//==============================================================================
/** The parameters of one oscillator, IDs prefixed "osc1." or "osc2.".

    @see SynthOscillatorValues, SynthPartials
*/
class SynthOscillatorParameters
{
public:
    /** Creates the parameters of one oscillator.

        @param oscillatorIndex  The zero based oscillator, which picks the ID prefix and host IDs
        @param defaults         The values the parameters start from and reset to
    */
    SynthOscillatorParameters (int oscillatorIndex, const SynthOscillatorValues& defaults);

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot; the partial fields keep their defaults, SynthPartials fills them. */
    SynthOscillatorValues read (SynthParameterReader reader = {}) const noexcept;

    yup::AudioParameter::Ptr waveform;
    yup::AudioParameter::Ptr syncMode;
    yup::AudioParameter::Ptr syncRatio;
    yup::AudioParameter::Ptr level;
    yup::AudioParameter::Ptr octave;
    yup::AudioParameter::Ptr detune; /**< In cents; read() hands out semitones. */
    yup::AudioParameter::Ptr ridgeSpacing;
    yup::AudioParameter::Ptr color;
    yup::AudioParameter::Ptr dispersion;
    yup::AudioParameter::Ptr squeeze;
    yup::AudioParameter::Ptr squash;
    yup::AudioParameter::Ptr tilt;
    yup::AudioParameter::Ptr oddEven;
    yup::AudioParameter::Ptr formant;
    yup::AudioParameter::Ptr formantPosition;
    yup::AudioParameter::Ptr scatter;
    yup::AudioParameter::Ptr unisonVoices;
    yup::AudioParameter::Ptr unisonDetune;
    yup::AudioParameter::Ptr unisonSpread;
};
