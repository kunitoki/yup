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

#include "ParameterChoice.h"
#include "ParameterKnob.h"
#include "ParameterToggle.h"
#include "WaveformEditor.h"

//==============================================================================
/** The editing surface of one oscillator: its waveform editor and every control.

    @see SynthOscillatorParameters, WaveformEditor
*/
class SynthOscillatorPanel : public yup::Component
{
public:
    SynthOscillatorPanel (const yup::String& panelTitle,
                          const SynthOscillatorParameters& parametersToEdit,
                          SynthPartials& partials,
                          const SynthOscillatorResources& resources,
                          std::shared_ptr<SynthWaveformShader> waveformShader,
                          const yup::Font& font);

    /** Publishes a frame's worth of partial edits and shows changes made anywhere else. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    //==============================================================================
    std::vector<ParameterKnob*> getKnobs();

    //==============================================================================
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float choiceRowHeight = 36.0f;
    static constexpr float knobRowHeight = 58.0f;
    static constexpr float buttonWidth = 68.0f;
    static constexpr float spacing = 6.0f;

    const SynthOscillatorParameters& parameters;
    const SynthPartials& partials;
    SynthOscillatorValues shownValues;

    yup::Label titleLabel;
    yup::ToggleButton partialsButton;
    yup::TextButton resetButton { "RESET" };
    WaveformEditor editor;

    ParameterChoice waveformChoice;
    ParameterChoice syncModeChoice;
    ParameterChoice frequencyModeChoice;
    ParameterToggle freeRunningButton;

    ParameterKnob levelKnob;
    ParameterKnob ratioKnob;
    ParameterKnob fineKnob;
    ParameterKnob fixedKnob;
    ParameterKnob panKnob;
    ParameterKnob phaseKnob;
    ParameterKnob toFilter1Knob;
    ParameterKnob toFilter2Knob;
    ParameterKnob directKnob;
    ParameterKnob ridgesKnob;
    ParameterKnob colorKnob;
    ParameterKnob dispersionKnob;
    ParameterKnob squeezeKnob;
    ParameterKnob squashKnob;
    ParameterKnob tiltKnob;
    ParameterKnob oddEvenKnob;
    ParameterKnob formantKnob;
    ParameterKnob formantPositionKnob;
    ParameterKnob scatterKnob;
    ParameterKnob syncRatioKnob;
};
