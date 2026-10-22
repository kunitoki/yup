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

//==============================================================================
/** Draws one cycle of an LFO shape with a marker at the engine's current phase. */
class LFODisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthLFOValues& newValues);

    /** Updates the curve the custom shape draws. Called from the message thread. */
    void setCurve (const SynthCurveState::Curve& newCurve);

    /** Moves the marker. Called every frame from the message thread. */
    void setPhase (float newPhase);

    void paint (yup::Graphics& g) override;

private:
    static constexpr int points = 128;

    SynthLFOValues values;
    SynthCurveState::Curve curve;
    float phase = 0.0f;
    yup::Path path;
};
