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

#include "../dsp/SynthEnvelopeParameters.h"
#include "EnvelopeDisplay.h"
#include "ParameterKnob.h"

//==============================================================================
/** One envelope: its editable curve over its time, velocity and depth macros.

    @see SynthEnvelopeParameters, SynthCurveState
*/
class SynthEnvelopePanel : public yup::Component
{
public:
    SynthEnvelopePanel (const SynthEnvelopeParameters& parametersToEdit, SynthCurveState& curvesToEdit, int envelopeIndex);

    /** Shows where the most recent note is along the curve's points, or hides it when negative. */
    void setPlayhead (float position) { display.setPlayhead (position); }

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;

private:
    static constexpr float knobRowHeight = 62.0f;

    SynthCurveState& curves;
    int index = 0;
    int shownGeneration = -1;

    EnvelopeDisplay display;

    ParameterKnob timeScaleKnob;
    ParameterKnob velocityKnob;
    ParameterKnob depthKnob;
};
