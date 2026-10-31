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

#include "../dsp/SynthFilterParameters.h"
#include "ParameterChoice.h"
#include "ParameterKnob.h"
#include "SynthFilterResponse.h"
#include "SynthNumberField.h"

//==============================================================================
/** One voice filter: its type, cutoff and resonance over its response, then drive,
    keytracking and mix.

    @see SynthFilterParameters, SynthFilterResponse
*/
class SynthFilterPanel : public yup::Component
{
public:
    SynthFilterPanel (const SynthFilterParameters& parametersToEdit, std::shared_ptr<SynthWaveformShader> landscapeShader);

    /** Shows changes made by the host, a preset or a state load.

        @param modulated  The values the response is drawn from: the parameters, or a table
                          with the most recent note's modulation.
    */
    void syncFromParameters (SynthParameterReader modulated = {});

    /** Draws the response at the rate the voices play at, see SynthFilterResponse::setSampleRate(). */
    void setSampleRate (double sampleRate) { response.setSampleRate (sampleRate); }

    /** Lets the response's landscape move, while something plays. */
    void setAnimating (bool shouldAnimate) noexcept { response.setAnimating (shouldAnimate); }

    void resized() override;

private:
    static constexpr float lineHeight = 24.0f;
    static constexpr float knobRowHeight = 62.0f;

    ParameterChoice typeChoice;
    SynthNumberField cutoffField;
    SynthNumberField resonanceField;
    SynthFilterResponse response;
    ParameterKnob driveKnob;
    ParameterKnob keytrackKnob;
    ParameterKnob mixKnob;
};
