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

#include <yup_gui/yup_gui.h>

#include "../dsp/SynthCurveState.h"
#include "../dsp/SynthLFOParameters.h"
#include "SynthTimeGrid.h"

//==============================================================================
/** Draws an LFO shape at its own rate and smoothing: one period with a marker at the
    engine's current phase, or several periods of the random shapes, over a grid of one
    period's time, upside down when the LFO is inverted. */
class LFODisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthLFOValues& newValues);

    /** Updates the curve the custom shape draws. Called from the message thread. */
    void setCurve (const SynthCurveState::Curve& newCurve);

    /** Moves the marker; a negative phase, with no note playing, hides it.
        Called every frame from the message thread. */
    void setPhase (float newPhase);

    /** Sets the host tempo a synced LFO is drawn at, or zero to draw it at 120 BPM. */
    void setTempo (double bpm);

    /** Sets how one period's time reads, for the grid. */
    void setTiming (const SynthTimeGrid::Timing& newTiming);

    void paint (yup::Graphics& g) override;

private:
    static constexpr int points = 128;
    static constexpr int randomPeriods = 4;
    static constexpr double displayTempo = 120.0;
    static constexpr yup::uint32 displaySeed = 1;

    SynthLFOValues values;
    SynthCurveState::Curve curve;
    float phase = -1.0f;
    double tempo = 0.0;
    SynthTimeGrid::Timing timing;
    yup::Path path;
};
