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

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_gui/yup_gui.h>

//==============================================================================
/** A toggle button bound to an on/off AudioParameter. A click is one host gesture.

    @see ParameterKnob, ParameterChoice
*/
class ParameterToggle : public yup::ToggleButton
{
public:
    ParameterToggle (const yup::String& buttonText, yup::AudioParameter::Ptr parameterToControl);

    /** Shows the parameter's current value.

        @returns true when the shown value changed.
    */
    bool syncFromParameter();

private:
    bool isParameterOn() const { return parameter->getValue() >= 0.5f; }

    yup::AudioParameter::Ptr parameter;
};
