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

//==============================================================================
/** The host automatable macros of one envelope. Its shape is a curve in the patch state. */
struct SynthEnvelopeValues
{
    float timeScale = 1.0f; /**< Multiplies every segment duration. */
    float velocity = 0.0f;  /**< How much the note velocity scales the output, 0 to 1. */
    float depth = 1.0f;     /**< Output scale as a modulation source. */

    bool operator== (const SynthEnvelopeValues&) const = default;

    /** Returns the output scale for a note velocity. */
    float getVelocityGain (float noteVelocity) const noexcept
    {
        return 1.0f - velocity + velocity * noteVelocity;
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

    yup::AudioParameter::Ptr timeScale;
    yup::AudioParameter::Ptr velocity;
    yup::AudioParameter::Ptr depth;
};
