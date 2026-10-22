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

#include "SynthOscillatorSlot.h"

#include <vector>

//==============================================================================
/** One of a voice's oscillators: a wavetable playing a derived series, plus unison.

    prepare() allocates the backends; renderBlock() is allocation-free and only
    re-renders a table when its series moved. The series normally comes from the
    shared slot; while a modulation is routed into this oscillator's spectrum it is
    derived here instead, from the voice's own values.

    Unison is built from bare yup::WavetableOscillator satellites playing the same
    series as the center, detuned and panned around it. Only the highest slot renders
    a table; the others copy it with renderFrom() instead of running their own FFT.

    @see SynthOscillatorSlot, SynthSpectrumDerivation
*/
class SynthOscillator
{
public:
    /** Allocates every backend and attaches the shared slot, partials and resources. */
    void prepare (double newSampleRate,
                  int maxBlockSize,
                  const SynthOscillatorSlot& sharedSlot,
                  const SynthPartials& oscillatorPartials,
                  const SynthOscillatorResources& oscillatorResources);

    /** Restarts every backend, spreading the satellites so they do not stack in phase. */
    void reset (double initialPhase) noexcept;

    /** Applies the pending changes and writes one stereo block.

        The buffers are overwritten rather than added to, so the caller does not have to
        clear them first.

        @param deriveLocally  True while a modulation reaches this oscillator's spectrum,
                              in which case the series is derived here from these values
                              rather than taken from the slot.
    */
    void renderBlock (float* left,
                      float* right,
                      int numSamples,
                      const SynthOscillatorValues& values,
                      double frequency,
                      bool deriveLocally) noexcept;

private:
    //==============================================================================
    static float slotOffset (int slotIndex, int slotCount) noexcept;
    static double detunedFrequency (double frequency, const SynthOscillatorValues& values, int slotIndex, int slotCount) noexcept;

    void accumulateSlot (float* left, float* right, int numSamples, float pan, float gain) noexcept;
    void renderTable (yup::WavetableOscillator<float>& table,
                      const yup::WavetableOscillator<float>& lead,
                      bool share,
                      int numSamples,
                      double frequency) noexcept;
    void applySeries (const SynthOscillatorValues& values, bool deriveLocally, int numLocalHarmonics) noexcept;
    void setSeries (const yup::FourierSeries<double>& series) noexcept;

    //==============================================================================
    yup::WavetableOscillator<float> wavetable;
    std::array<yup::WavetableOscillator<float>, SynthExample::maxUnisonVoices - 1> satellites;
    SynthSpectrumDerivation localDerivation;

    const SynthOscillatorSlot* slot = nullptr;
    const SynthPartials* partials = nullptr;
    const SynthOscillatorResources* resources = nullptr;
    std::vector<float> slotBuffer;
    double sampleRate = 44100.0;
    int appliedSeriesGeneration = -1;
    int sharedSlotCount = 0;
    bool usingLocalSeries = false;
};
