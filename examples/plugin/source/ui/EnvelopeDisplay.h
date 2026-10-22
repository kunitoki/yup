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

#include <yup_gui/yup_gui.h>

#include "../dsp/SynthEnvelopeParameters.h"

//==============================================================================
/** Draws the shape an envelope traces, with a point at every breakpoint. */
class EnvelopeDisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthEnvelopeValues& newValues);

    void paint (yup::Graphics& g) override;

private:
    static constexpr float sustainShare = 0.22f;
    static constexpr float pointRadius = 3.0f;

    SynthEnvelopeValues values;
    yup::Path path;
};
