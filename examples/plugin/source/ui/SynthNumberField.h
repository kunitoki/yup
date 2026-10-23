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

#include <yup_gui/yup_gui.h>

//==============================================================================
/** A sunken box showing a parameter's value, with an optional caption, changed by
    dragging up or down.

    A full drag of 200 pixels sweeps the whole range, ten times finer with shift held. A
    double-click returns the parameter to its default.

    @see SynthSliderField, ParameterKnob
*/
class SynthNumberField : public yup::Component
{
public:
    SynthNumberField (const yup::String& caption, yup::AudioParameter::Ptr parameterToControl);

    /** Binds another parameter, for a field that follows a mode such as ratio or fixed. */
    void setParameter (yup::AudioParameter::Ptr newParameter);

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
    static constexpr float pixelsPerRange = 200.0f;

    ParameterGesture gesture;
    yup::String caption;
    yup::String shownText;
};
