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

#include "../dsp/SynthModulationParameters.h"
#include "ParameterChoice.h"
#include "ParameterKnob.h"

//==============================================================================
/** One routing: which source drives which destination, and how much. */
class SynthModulationRow : public yup::Component
{
public:
    SynthModulationRow (const SynthModulationParameters::Slot& slotToEdit, const yup::Font& font);

    /** Shows changes made by the host, a preset or a state load. */
    void syncFromParameters();

    void resized() override;

private:
    static constexpr float knobWidth = 58.0f;
    static constexpr float spacing = 6.0f;

    ParameterChoice sourceChoice;
    ParameterChoice destinationChoice;
    ParameterKnob depthKnob;
};
