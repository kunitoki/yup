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

#include "SynthSpectrumDerivation.h"

//==============================================================================
/** The series every voice of one oscillator slot plays, rebuilt once per control chunk.

    A slot's spectrum does not vary per voice unless something is routed into it, so
    it is derived once here rather than inside each voice. Voices publish nothing back;
    they compare getGeneration() and re-render their own table when it moves, which
    keeps yup::WavetableOscillator's crossfade doing the smoothing. Runs on the audio
    thread before any voice reads it.

    @see SynthSpectrumDerivation, SynthOscillator
*/
class SynthOscillatorSlot
{
public:
    SynthOscillatorSlot() { derivation.prepare(); }

    /** Rebuilds the slot's series if anything it depends on moved. Audio thread. */
    void update (const SynthOscillatorValues& values,
                 const SynthPartials& partials,
                 const SynthOscillatorResources& resources) noexcept
    {
        if (derivation.update (values, partials, resources))
            ++generation;
    }

    /** Returns the series the voices of this slot should be playing. */
    const yup::FourierSeries<double>& getSeries() const noexcept { return derivation.getSeries(); }

    /** Bumped whenever getSeries() changed, so a voice knows to re-render its table. */
    int getGeneration() const noexcept { return generation; }

private:
    SynthSpectrumDerivation derivation;
    int generation = 0;
};
