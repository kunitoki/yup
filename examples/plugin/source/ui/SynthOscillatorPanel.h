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
#include "ParameterLed.h"
#include "ParameterSegmented.h"
#include "SynthIconButton.h"
#include "SynthNumberField.h"
#include "SynthSegmented.h"
#include "SynthSliderField.h"
#include "WaveformEditor.h"

#include <array>

//==============================================================================
/** The editing surface of one oscillator.

    The waveform editor sits in a sunken display whose top bar picks the waveform and
    the pitch, whose bottom bar holds free running, sync and the partials switches, and
    whose left rail picks the drawing tool and opens the editing menu.
    Level, pan, phase and the three sends sit under it; the spectral shaping fields fill
    the column on the right.

    @see SynthOscillatorParameters, WaveformEditor
*/
class SynthOscillatorPanel : public yup::Component
{
public:
    SynthOscillatorPanel (const SynthOscillatorParameters& parametersToEdit,
                          SynthPartials& partials,
                          const SynthOscillatorResources& resources,
                          std::shared_ptr<SynthWaveformShader> waveformShader);

    /** Publishes a frame's worth of partial edits and shows changes made anywhere else.

        @param modulated  The parameter values the wave is drawn from: by default the
                          parameters, or a table with the most recent note's modulation.
    */
    void syncFromParameters (SynthParameterReader modulated = {});

    /** Lets the waveform display move, while something plays. */
    void setAnimating (bool shouldAnimate) noexcept { editor.setAnimating (shouldAnimate); }

    void resized() override;
    void paint (yup::Graphics& g) override;

private:
    //==============================================================================
    void showModes (const SynthOscillatorValues& values);
    void showEditingPartials (bool shouldEdit);
    void showToolsMenu();

    static std::vector<SynthSegmented::Segment> makeDrawToolSegments();
    static std::vector<SynthSegmented::Segment> makePartialsToolSegments();

    //==============================================================================
    static constexpr float spectrumShare = 262.0f / 774.0f;
    static constexpr float barHeight = 30.0f;
    static constexpr float railWidth = 28.0f;
    static constexpr float toolHeight = 24.0f;
    static constexpr float controlsHeight = 78.0f;

    const SynthOscillatorParameters& parameters;
    const SynthPartials& partials;
    SynthOscillatorValues shownValues;

    WaveformEditor editor;
    yup::Rectangle<float> displayArea;
    yup::Rectangle<float> topBar;
    yup::Rectangle<float> bottomBar;
    yup::Rectangle<float> toolRail;
    yup::Rectangle<float> syncCaptionArea;
    float seamX = 0.0f;

    ParameterChoice waveformChoice;
    ParameterSegmented frequencyModeSelector;
    SynthNumberField pitchField;
    SynthNumberField fineField;

    ParameterLed freeRunningLed;
    ParameterChoice syncModeChoice;
    SynthNumberField syncRatioField;
    yup::ToggleButton partialsButton;
    yup::TextButton resetButton { "Reset" };

    SynthSegmented drawTools;
    SynthSegmented partialsTools;
    SynthIconButton toolsMenuButton { "toolsMenu", YUP_ICON_ELLIPSIS, 13.0f, false };

    ParameterKnob levelKnob;
    ParameterKnob panKnob;
    ParameterKnob phaseKnob;
    std::array<SynthSliderField, 3> sends;
    std::array<SynthSliderField, 10> spectrum;
    yup::Font captionFont;
};
