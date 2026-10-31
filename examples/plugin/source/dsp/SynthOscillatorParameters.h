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

#include <vector>

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

    SynthFrequencyMode frequencyMode = SynthFrequencyMode::ratio;
    float ratio = 1.0f;
    float fineCents = 0.0f;
    float fixedHz = 440.0f;

    bool enabled = true;
    float level = 0.0f;
    float pan = 0.0f;
    float phase = 0.0f;
    bool freeRunning = false;
    float toFilter1 = 1.0f;
    float toFilter2 = 0.0f;
    float direct = 0.0f;

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

    float harmonicScale = 1.0f;
    bool usesCustomSeries = false;
    int harmonicGeneration = 0;

    bool operator== (const SynthOscillatorValues&) const = default;

    /** Returns the operator's frequency for a played note frequency. */
    double getFrequency (double noteFrequency) const noexcept
    {
        const auto fine = std::exp2 (static_cast<double> (fineCents) / 1200.0);

        if (frequencyMode == SynthFrequencyMode::fixed)
            return static_cast<double> (fixedHz) * fine;

        return noteFrequency * static_cast<double> (ratio) * fine;
    }

    /** Returns the total of the three output sends, so a silent operator can be skipped. */
    float getTotalSend() const noexcept { return toFilter1 + toFilter2 + direct; }
};

//==============================================================================
/** The parameters of one oscillator, IDs prefixed "osc1." to "osc4.".

    Each oscillator is an operator of the phase modulation network: its own pitch,
    level, pan and sends, plus the spectral shaping of the waveform it plays.

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

    /** Returns the parameters that change the derived spectrum rather than how it is played. */
    std::vector<yup::AudioParameter::Ptr> getSpectralParameters() const;

    yup::AudioParameter::Ptr waveform;
    yup::AudioParameter::Ptr syncMode;
    yup::AudioParameter::Ptr syncRatio;
    yup::AudioParameter::Ptr frequencyMode;
    yup::AudioParameter::Ptr ratio;
    yup::AudioParameter::Ptr fine; /**< In cents. */
    yup::AudioParameter::Ptr fixedHz;
    yup::AudioParameter::Ptr level;
    yup::AudioParameter::Ptr pan;
    yup::AudioParameter::Ptr phase;
    yup::AudioParameter::Ptr freeRunning; /**< Stepped, 0 or 1. */
    yup::AudioParameter::Ptr toFilter1;
    yup::AudioParameter::Ptr toFilter2;
    yup::AudioParameter::Ptr direct;
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
    yup::AudioParameter::Ptr enabled; /**< Stepped, 0 or 1; off neither sounds nor modulates. */
};
