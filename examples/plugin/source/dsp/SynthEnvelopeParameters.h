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
/** A plain snapshot of one envelope's controls, in seconds apart from the sustain level. */
struct SynthEnvelopeValues
{
    float delay = 0.0f;
    float attack = 0.005f;
    float hold = 0.0f;
    float decay = 0.35f;
    float sustain = 0.7f;
    float release = 0.35f;

    bool operator== (const SynthEnvelopeValues&) const = default;
};

//==============================================================================
/** The parameters of one envelope, IDs prefixed "env1." or "env2.". */
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

    yup::AudioParameter::Ptr delay;
    yup::AudioParameter::Ptr attack;
    yup::AudioParameter::Ptr hold;
    yup::AudioParameter::Ptr decay;
    yup::AudioParameter::Ptr sustain;
    yup::AudioParameter::Ptr release;
};
