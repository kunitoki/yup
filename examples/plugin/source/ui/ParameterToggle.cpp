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

#include "ParameterToggle.h"
#include "SynthTheme.h"

ParameterToggle::ParameterToggle (const yup::String& buttonText, yup::AudioParameter::Ptr parameterToControl)
    : parameter (std::move (parameterToControl))
{
    setButtonText (buttonText);
    setToggleState (isParameterOn(), yup::dontSendNotification);
    SynthTheme::styleToggle (*this);

    onClick = [this]
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (getToggleState() ? parameter->getMaximumValue() : parameter->getMinimumValue());
        parameter->endChangeGesture();
    };
}

bool ParameterToggle::syncFromParameter()
{
    if (getToggleState() == isParameterOn())
        return false;

    setToggleState (isParameterOn(), yup::dontSendNotification);
    return true;
}
