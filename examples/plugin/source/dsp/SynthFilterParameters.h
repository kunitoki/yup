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
/** A plain snapshot of the filter section's controls. */
struct SynthFilterValues
{
    SynthFilterType type = SynthFilterType::lowpass;
    float cutoff = 8000.0f;
    float resonance = 0.2f;
    float drive = 0.0f;
    float keytrack = 0.0f;

    bool operator== (const SynthFilterValues&) const = default;
};

//==============================================================================
/** The parameters of the per-voice filter, IDs prefixed "filter.". */
class SynthFilterParameters
{
public:
    /** Creates the parameters with the SynthFilterValues defaults. */
    SynthFilterParameters();

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot. */
    SynthFilterValues read (SynthParameterReader reader = {}) const noexcept;

    yup::AudioParameter::Ptr type;
    yup::AudioParameter::Ptr cutoff;
    yup::AudioParameter::Ptr resonance;
    yup::AudioParameter::Ptr drive;
    yup::AudioParameter::Ptr keytrack;
};
