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

#include "SynthFilterParameters.h"
#include "SynthOscillatorParameters.h"

#include <array>

//==============================================================================
/** One routing of the matrix. */
struct SynthModulationRoute
{
    SynthModulationSource source = SynthModulationSource::env1;
    SynthModulationDestination destination = SynthModulationDestination::none;
    float depth = 0.0f;
};

/** A snapshot of every routing. */
struct SynthModulationValues
{
    std::array<SynthModulationRoute, SynthExample::modulationSlots> routes;

    /** True when something is routed with depth into a spectrum parameter of this oscillator. */
    bool hasSpectrumRoute (int oscillator) const noexcept;
};

//==============================================================================
/** Everything a voice plays from, before or after modulation has been applied. */
struct SynthPatchValues
{
    std::array<SynthOscillatorValues, SynthExample::oscillatorCount> oscillators;
    SynthFilterValues filter;
};

/** Returns the field a destination modulates, or nullptr for none. */
float* getDestinationField (SynthPatchValues& patch, SynthModulationDestination destination) noexcept;

/** Adds every route whose source has a value to the patch: one full knob range per unit of depth times source. */
void applyModulation (SynthPatchValues& patch,
                      const SynthModulationValues& modulation,
                      const std::array<float, 5>& sourceValues) noexcept;
