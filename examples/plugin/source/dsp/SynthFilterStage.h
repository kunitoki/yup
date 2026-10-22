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

#include <array>

//==============================================================================
/** The per-voice filter: a drive stage into one VAStateVariableFilter per channel.

    Cutoff, resonance and drive are smoothed across a control chunk so modulation
    cannot zipper. Off skips everything, leaving the filter state where it was.

    @see SynthFilterParameters
*/
class SynthFilterStage
{
public:
    /** Prepares both channels and the parameter smoothers. */
    void prepare (double sampleRate);

    /** Clears the filter memory, for a fresh note. */
    void reset() noexcept;

    /** Filters a stereo chunk in place at the note's keytracked cutoff. */
    void process (float* left, float* right, int numSamples, const SynthFilterValues& values, double midiNote) noexcept;

private:
    std::array<yup::VAStateVariableFilter<float>, 2> filters;
    yup::SmoothedValue<float, yup::ValueSmoothingTypes::Multiplicative> cutoff;
    yup::SmoothedValue<float> resonance;
    yup::SmoothedValue<float> drive;
};
