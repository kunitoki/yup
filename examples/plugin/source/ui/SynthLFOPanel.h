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
#include "ParameterToggle.h"

//==============================================================================
/** Shape, rate or tempo division, phase, smoothing, retrigger and depth of one LFO, with its display.

    @see SynthLFOParameters
*/
class SynthLFOPanel : public yup::Component
{
public:
    SynthLFOPanel (const yup::String& panelTitle,
                   const SynthLFOParameters& parametersToEdit,
                   const SynthCurveState& curvesToShow,
                   int lfoIndex,
                   const yup::Font& font);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    /** Moves the display marker to the engine's phase. */
    void setPhase (float phase) { display.setPhase (phase); }

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float choiceHeight = 36.0f;
    static constexpr float choiceWidth = 90.0f;
    static constexpr float knobWidth = 58.0f;
    static constexpr float buttonWidth = 68.0f;
    static constexpr float spacing = 6.0f;

    const SynthLFOParameters& parameters;
    const SynthCurveState& curves;
    int index = 0;
    int shownGeneration = -1;
    SynthLFOValues shownValues;

    yup::Label titleLabel;
    ParameterToggle syncButton;
    LFODisplay display;
    ParameterChoice shapeChoice;
    ParameterChoice divisionChoice;
    ParameterChoice retriggerChoice;
    ParameterKnob rateKnob;
    ParameterKnob phaseKnob;
    ParameterKnob smoothKnob;
    ParameterKnob depthKnob;
};
