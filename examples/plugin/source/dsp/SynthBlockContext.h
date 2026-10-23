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
#include "SynthOperatorNetwork.h"
#include "SynthRoutes.h"

//==============================================================================
/** The parameter snapshot for this control chunk, read by every voice while it renders.

    Written at the top of HarmonicSynthEngine::renderRange, before any voice runs, on the
    same thread, so no publication handshake is needed. Everything here is unmodulated:
    a voice with routes applies them to its own copy of the value table.
*/
struct SynthBlockContext
{
    /** One played value per parameter index, host modulation included. */
    yup::Span<const float> played;

    SynthNetworkControls network;
    std::array<SynthFilterValues, SynthExample::filterCount> filters;
    std::array<SynthEnvelopeValues, SynthExample::envelopeCount> envelopes;
    std::array<SynthLFOValues, SynthExample::lfoCount> lfos;

    /** The active routes, compacted, with their depths. */
    std::array<SynthRoute, SynthExample::modulationSlots> routes;
    std::array<float, SynthExample::modulationSlots> depths {};
    std::array<int, SynthExample::modulationSlots> slots {}; /**< The route slot each compacted route came from. */
    int numRoutes = 0;

    std::array<bool, SynthExample::oscillatorCount> spectralRouted {}; /**< A route reaches the oscillator's spectrum. */
    std::array<bool, SynthExample::envelopeCount> envelopeRouted {};   /**< The envelope drives a route. */
    std::array<bool, SynthExample::lfoCount> lfoRouted {};             /**< The LFO drives a route. */

    SynthCurveState::EnvelopeCurves envelopeCurves;
    SynthCurveState::LFOCurves lfoCurves;
    int curveGeneration = 0;

    float modWheel = 0.0f;   /**< The last controller 1 value, 0 to 1, kept across blocks. */
    float aftertouch = 0.0f; /**< The last channel pressure, 0 to 1, kept across blocks. */
    std::array<float, SynthExample::macroCount> macros {}; /**< The macro knobs as played, 0 to 1. */
    double bpm = 0.0;        /**< The host tempo, or zero when unknown. */
};
