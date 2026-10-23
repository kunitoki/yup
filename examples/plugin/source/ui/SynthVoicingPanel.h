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

#include "../dsp/SynthParameters.h"
#include "ParameterGesture.h"
#include "ParameterKnob.h"
#include "SynthSpinner.h"

#include <yup_gui/yup_gui.h>

//==============================================================================
/** How notes take voices: mono and legato switches over the voice mode, the quality,
    the glide, and the unison count, detune and spread.

    Mono off plays polyphonically and greys out legato; mono on plays one voice, and
    legato on top of it keeps the envelopes and phases across overlapping notes.

    @see SynthParameters
*/
class SynthVoicingPanel : public yup::Component
{
public:
    explicit SynthVoicingPanel (const SynthParameters& parametersToEdit);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;
    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;

private:
    static constexpr float titleHeight = 24.0f;
    static constexpr float unisonWidth = 146.0f;
    static constexpr float checkSize = 13.0f;
    static constexpr float checkRowWidth = 74.0f;

    SynthPlayMode getMode() const;
    void setMode (SynthPlayMode mode);

    const SynthParameters& parameters;
    ParameterGesture modeGesture;
    SynthSpinner qualitySpinner;
    SynthSpinner unisonSpinner;
    ParameterKnob glideKnob;
    ParameterKnob detuneKnob;
    ParameterKnob spreadKnob;

    yup::Rectangle<float> voicingArea;
    yup::Rectangle<float> unisonArea;
    yup::Rectangle<float> monoBox;
    yup::Rectangle<float> legatoBox;
    yup::Rectangle<float> qualityCaption;
    SynthPlayMode shownMode = SynthPlayMode::poly;
};
