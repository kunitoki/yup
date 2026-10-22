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

#include "SynthModulationMatrix.h"

//==============================================================================
/** The sixteen routings of the modulation matrix, IDs "mod1.source" to "mod16.depth". */
class SynthModulationParameters
{
public:
    /** The three parameters of one routing. */
    struct Slot
    {
        yup::AudioParameter::Ptr source;
        yup::AudioParameter::Ptr destination;
        yup::AudioParameter::Ptr depth;
    };

    /** Creates every slot unrouted. */
    SynthModulationParameters();

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Takes a snapshot of every routing. */
    SynthModulationValues read (SynthParameterReader reader = {}) const noexcept;

    std::array<Slot, SynthExample::modulationSlots> slots;
};
