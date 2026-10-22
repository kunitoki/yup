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

#include "SynthParameterFactory.h"
#include "SynthTypes.h"

//==============================================================================
/** The depth of every modulation route, IDs "route.1.depth" to "route.32.depth".

    Sources and destinations are patch state in SynthRoutes, so any parameter can be a
    destination; only the depths are parameters, so the host can automate them.

    @see SynthRoutes
*/
class SynthModulationParameters
{
public:
    /** Creates every depth at zero. */
    SynthModulationParameters();

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    std::array<yup::AudioParameter::Ptr, SynthExample::modulationSlots> depths;
};
