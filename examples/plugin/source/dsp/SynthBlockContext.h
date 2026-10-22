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
#include "SynthModulationMatrix.h"

//==============================================================================
/** The parameter snapshot for this control chunk, read by every voice while it renders.

    Written at the top of HarmonicSynthEngine::renderNextBlock, before any voice runs,
    on the same thread, so no publication handshake is needed.
*/
struct SynthBlockContext
{
    SynthPatchValues patch;
    SynthModulationValues modulation;
    std::array<SynthEnvelopeValues, SynthExample::envelopeCount> envelopes;
    std::array<SynthLFOValues, SynthExample::lfoCount> lfos;
    float modWheel = 0.0f; /**< The last controller 1 value, 0 to 1, kept across blocks. */
};
