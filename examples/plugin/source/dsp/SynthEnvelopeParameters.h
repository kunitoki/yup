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

#include "SynthCurveState.h"
#include "SynthParameterFactory.h"

//==============================================================================
/** Reshapes an envelope's drawn curve as an attack, decay, sustain and release.

    The attack runs up to the curve's highest point before its sustain point, the decay
    from there to the sustain point and the release after it; a curve without a sustain
    point decays to its end. Each stage stretches the durations of its segments, and the
    sustain scales the sustain point's level.
*/
struct SynthEnvelopeStages
{
    float attack = 1.0f;  /**< Multiplies the attack durations. */
    float decay = 1.0f;   /**< Multiplies the decay durations. */
    float sustain = 1.0f; /**< Multiplies the sustain level. */
    float release = 1.0f; /**< Multiplies the release durations. */

    bool operator== (const SynthEnvelopeStages&) const = default;

    /** Returns how much the segment ending at a point of a curve is stretched. */
    float getScale (const SynthCurveState::Curve& curve, int segment) const noexcept;

    /** Returns the curve as it plays, with every stage applied. */
    SynthCurveState::Curve apply (const SynthCurveState::Curve& curve) const noexcept;
};

//==============================================================================
/** The host automatable macros of one envelope. Its shape is a curve in the patch state. */
struct SynthEnvelopeValues
{
    float timeScale = 1.0f; /**< Multiplies every segment duration. */
    float velocity = 0.0f;  /**< How much the note velocity scales the output, 0 to 1. */
    float depth = 1.0f;     /**< Output scale as a modulation source. */
    bool enabled = true;    /**< Off is not computed and sources 0; the amplitude envelope is always on. */
    bool tempoSync = false; /**< The curve's times are quarter note beats instead of seconds. */
    SynthEnvelopeStages stages;
    bool bipolar = false; /**< As a modulation source, swings from -depth to depth instead of 0 to depth. */

    bool operator== (const SynthEnvelopeValues&) const = default;

    /** The tempo a synced envelope plays at while the host reports none. */
    static constexpr double fallbackTempo = 120.0;

    /** Returns the modulation source value for an envelope level and a note velocity. */
    float getSourceValue (float level, float noteVelocity) const noexcept
    {
        return (bipolar ? level * 2.0f - 1.0f : level) * depth * getVelocityGain (noteVelocity);
    }

    /** Returns the output scale for a note velocity. */
    float getVelocityGain (float noteVelocity) const noexcept
    {
        return 1.0f - velocity + velocity * noteVelocity;
    }

    /** Returns the factor turning the curve's times into seconds: the time scale, and the
        length of a beat when synced.

        @param bpm  The host tempo, or zero when unknown, which plays at fallbackTempo
    */
    float getTimeScale (double bpm) const noexcept
    {
        if (! tempoSync)
            return timeScale;

        return timeScale * static_cast<float> (60.0 / (bpm > 0.0 ? bpm : fallbackTempo));
    }
};

//==============================================================================
/** The macros of one envelope, IDs prefixed "env1." to "env4.". */
class SynthEnvelopeParameters
{
public:
    /** Creates the parameters of one envelope.

        @param envelopeIndex  The zero based envelope, which picks the ID prefix and host IDs
        @param defaults       The values the parameters start from and reset to
    */
    SynthEnvelopeParameters (int envelopeIndex, const SynthEnvelopeValues& defaults);

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot. */
    SynthEnvelopeValues read (SynthParameterReader reader = {}) const noexcept;

    yup::AudioParameter::Ptr attack;
    yup::AudioParameter::Ptr decay;
    yup::AudioParameter::Ptr sustain;
    yup::AudioParameter::Ptr release;
    yup::AudioParameter::Ptr timeScale;
    yup::AudioParameter::Ptr velocity;
    yup::AudioParameter::Ptr depth;
    yup::AudioParameter::Ptr enabled; /**< Stepped, 0 or 1; null for the amplitude envelope, which is always on. */
    yup::AudioParameter::Ptr tempoSync; /**< Stepped, 0 or 1. */
    yup::AudioParameter::Ptr polarity;  /**< Unipolar 0 or bipolar 1; null for the amplitude envelope, which only shapes the level. */
};
