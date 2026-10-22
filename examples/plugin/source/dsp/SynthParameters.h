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
#include "SynthLFOParameters.h"
#include "SynthModulationParameters.h"

//==============================================================================
/** Every parameter of the synthesiser, grouped by section.

    The parameters are created here with their IDs, host IDs, ranges and defaults, and
    registered with the processor by addTo(). Defaults are what the Init preset plays.

    @see SynthPatch, HarmonicSynthEngine
*/
class SynthParameters
{
public:
    /** Creates every parameter. */
    SynthParameters();

    /** Registers every parameter with the processor. Call once, from its constructor. */
    void addTo (yup::AudioProcessor& processor) const;

    std::array<SynthOscillatorParameters, SynthExample::oscillatorCount> oscillators;
    SynthFilterParameters filter;
    std::array<SynthEnvelopeParameters, SynthExample::envelopeCount> envelopes;
    std::array<SynthLFOParameters, SynthExample::lfoCount> lfos;
    SynthModulationParameters modulation;

    yup::AudioParameter::Ptr volume;   /**< Master output gain, 0 to 1. */
    yup::AudioParameter::Ptr playMode; /**< A SynthPlayMode. */
    yup::AudioParameter::Ptr glide;    /**< Portamento time in milliseconds, used by mono and legato. */
};
