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

//==============================================================================
/** Type, cutoff, resonance, drive, keytracking and mix of one voice filter.

    @see SynthFilterParameters
*/
class SynthFilterPanel : public yup::Component
{
public:
    SynthFilterPanel (const yup::String& panelTitle, const SynthFilterParameters& parametersToEdit, const yup::Font& font);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 16.0f;
    static constexpr float choiceHeight = 36.0f;
    static constexpr float spacing = 6.0f;

    yup::Label titleLabel;
    ParameterChoice typeChoice;
    ParameterKnob cutoffKnob;
    ParameterKnob resonanceKnob;
    ParameterKnob driveKnob;
    ParameterKnob keytrackKnob;
    ParameterKnob mixKnob;
};
