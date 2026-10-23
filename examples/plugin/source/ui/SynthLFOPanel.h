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

#include "LFODisplay.h"
#include "ParameterChoice.h"
#include "ParameterKnob.h"
#include "ParameterSegmented.h"

//==============================================================================
/** One LFO: its shape and retrigger mode, its display, and rate, phase, smoothing and
    depth. The rate knob turns into the tempo division while the LFO is synced.

    @see SynthLFOParameters
*/
class SynthLFOPanel : public yup::Component
{
public:
    SynthLFOPanel (const SynthLFOParameters& parametersToEdit, const SynthCurveState& curvesToShow, int lfoIndex);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    /** Moves the display marker to the engine's phase. */
    void setPhase (float phase) { display.setPhase (phase); }

    void resized() override;

private:
    static constexpr float lineHeight = 24.0f;
    static constexpr float knobRowHeight = 62.0f;
    static constexpr float retriggerWidth = 96.0f;

    static std::vector<ParameterSegmented::Segment> makeShapeSegments();

    const SynthLFOParameters& parameters;
    const SynthCurveState& curves;
    int index = 0;
    int shownGeneration = -1;
    SynthLFOValues shownValues;

    ParameterSegmented shapeSelector;
    ParameterChoice retriggerChoice;
    LFODisplay display;
    ParameterKnob rateKnob;
    ParameterKnob phaseKnob;
    ParameterKnob smoothKnob;
    ParameterKnob depthKnob;
};
