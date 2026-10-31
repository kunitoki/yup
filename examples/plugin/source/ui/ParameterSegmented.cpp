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
#include "ParameterSegmented.h"

ParameterSegmented::ParameterSegmented (yup::AudioParameter::Ptr choiceParameter, std::vector<Segment> segmentsToShow)
    : SynthSegmented (std::move (segmentsToShow))
    , gesture (*this, std::move (choiceParameter))
{
    onSelect = [this] (int index)
    {
        gesture.setNormalized (gesture.getParameter()->convertToNormalizedValue (static_cast<float> (index)));
    };

    syncFromParameter();
}

bool ParameterSegmented::syncFromParameter()
{
    return setSelectedIndex (yup::roundToInt (gesture.getParameter()->getValue()));
}
