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

#include "ParameterGesture.h"
#include "SynthKnob.h"

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** A thumbless horizontal slider bound to a parameter: a caption, the value and a thin
    track filled up to the value, changed by dragging left or right.

    The boxed look sits in its own sunken box with the caption and value above the
    track; the bar look is a single line of caption, track and value. A bipolar field
    fills from a center tick. Routes reaching the parameter are drawn as ranges under
    the track in their source colors. A double-click returns the default.

    @see SynthNumberField, ParameterKnob
*/
class SynthSliderField : public yup::Component
{
public:
    /** How the field is drawn. */
    enum class Look
    {
        boxed, /**< In a sunken box, caption and value above the track. */
        bar    /**< One line: caption, track, value. */
    };

    SynthSliderField (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl, Look look, bool bipolar = false);

    /** Replaces the route ranges drawn under the track. */
    void setModulationArcs (std::vector<SynthKnob::ModulationArc> newArcs);

    /** Shows the parameter's current value unless it is being dragged.
        @returns true when the shown value changed.
    */
    bool syncFromParameter();

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;
    void mouseDoubleClick (const yup::MouseEvent& event) override;

private:
    static constexpr float captionWidth = 46.0f;
    static constexpr float valueWidth = 34.0f;

    yup::Rectangle<float> getTrackBounds() const;

    ParameterGesture gesture;
    yup::String caption;
    Look look;
    bool bipolar;
    float shownValue = -1.0f;
    std::vector<SynthKnob::ModulationArc> modulationArcs;
};
