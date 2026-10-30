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
/** A sunken box showing a stepped parameter's value with up and down arrows, which step
    it by one. Dragging up or down steps it too.

    @see SynthNumberField
*/
class SynthSpinner : public yup::Component
{
public:
    explicit SynthSpinner (yup::AudioParameter::Ptr steppedParameter);

    /** Shows the parameter's current value.
        @returns true when the shown value changed.
    */
    bool syncFromParameter();

    void paint (yup::Graphics& g) override;
    void mouseDown (const yup::MouseEvent& event) override;
    void mouseDrag (const yup::MouseEvent& event) override;
    void mouseUp (const yup::MouseEvent& event) override;

private:
    static constexpr float arrowWidth = 12.0f;
    static constexpr float pixelsPerStep = 12.0f;

    void stepTo (float value);

    ParameterGesture gesture;
    yup::String shownText;
    std::optional<float> shownValue;
    float dragStartValue = 0.0f;
    bool dragged = false;
    yup::Font valueFont;
};
