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

#include <array>

//==============================================================================
/** Two vertical bars showing the output level in dB, from -60 to 0, falling back
    slowly after a peak.

    @see SynthHeader
*/
class SynthLevelMeter : public yup::Component
{
public:
    SynthLevelMeter();

    /** Shows new linear peaks; a level below the shown one falls at about 20 dB a second
        at the 60 Hz the editor calls it.
    */
    void setLevels (float left, float right);

    void paint (yup::Graphics& g) override;

private:
    static constexpr float floorDecibels = -60.0f;
    static constexpr float fallPerCall = 20.0f / 60.0f;

    std::array<float, 2> shownDecibels { floorDecibels, floorDecibels };
};
