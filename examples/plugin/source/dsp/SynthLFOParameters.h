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
/** A plain snapshot of one LFO's controls. */
struct SynthLFOValues
{
    yup::LFO<float>::Shape shape = yup::LFO<float>::Shape::sine;
    float rate = 1.0f;
    float phase = 0.0f;
    bool retrigger = true; /**< Restart from the phase offset on every note, or run freely. */

    bool operator== (const SynthLFOValues&) const = default;
};

//==============================================================================
/** The parameters of one LFO, IDs prefixed "lfo1." or "lfo2.". */
class SynthLFOParameters
{
public:
    /** Creates the parameters of one LFO with the SynthLFOValues defaults.

        @param lfoIndex  The zero based LFO, which picks the ID prefix and host IDs
    */
    explicit SynthLFOParameters (int lfoIndex);

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot. */
    SynthLFOValues read (SynthParameterReader reader = {}) const noexcept;

    yup::AudioParameter::Ptr shape;
    yup::AudioParameter::Ptr rate;
    yup::AudioParameter::Ptr phase;
    yup::AudioParameter::Ptr retrigger; /**< Stepped, 0 or 1. */
};
