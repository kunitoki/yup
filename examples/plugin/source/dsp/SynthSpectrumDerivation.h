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

#include "SynthOscillatorResources.h"
#include "SynthPartials.h"

//==============================================================================
/** Derives the series one oscillator plays: partials or preset, Prism shaping, sync.

    Shared by the engine's slot, by a voice that needs its own spectrum while an
    envelope modulates it, and by the waveform preview. prepare() allocates; update()
    does not.

    The derived series is rescaled so its waveform peaks where the source's does.
    yup::PrismSpectrum preserves the coefficient sum, which bounds a peak from well
    above - three times over for a sawtooth - and how close the waveform comes to
    that bound depends on how aligned the harmonic phases are. Dispersion, scatter and
    the sync reset all move exactly that, so without the rescale they would swing the
    output level as they are swept rather than only recoloring it.

    @see SynthOscillatorSlot, SynthOscillator, WaveformEditor
*/
class SynthSpectrumDerivation
{
public:
    /** Allocates the shaper, the resampler, the series and the peak meter. */
    void prepare();

    /** Rebuilds the series if anything it depends on moved. Returns true when it did.

        @param numOutputHarmonics  How many harmonics of the synced series are needed, -1 for
                                   all of them. Only a growing count forces a rebuild: a table
                                   played higher drops the extra harmonics by itself. An eighth
                                   more is computed, like WavetableOscillator::needsRender(), so a
                                   falling pitch does not rebuild on every block.
    */
    bool update (const SynthOscillatorValues& values,
                 const SynthPartials& partials,
                 const SynthOscillatorResources& resources,
                 int numOutputHarmonics = -1) noexcept;

    /** Returns the series the last update() derived. */
    const yup::FourierSeries<double>& getSeries() const noexcept { return *derived; }

    /** Forgets what was applied so the next update() rebuilds unconditionally. */
    void invalidate() noexcept { hasApplied = false; }

private:
    double measurePeak (const yup::FourierSeries<double>& series) noexcept;
    void matchSourcePeak (yup::FourierSeries<double>& series) noexcept;

    yup::PrismSpectrum<double> spectrum;
    yup::SyncSpectralResampler<double> resampler;
    yup::WavetableOscillator<float> peakMeter;
    double sourcePeak = 0.0;
    yup::FourierSeries<double> customSeries;
    yup::FourierSeries<double> shapedSeries;
    yup::FourierSeries<double> syncedSeries;
    yup::FourierSeries<double>* derived = &shapedSeries;
    SynthOscillatorValues applied;
    int appliedOutputHarmonics = 0;
    bool hasApplied = false;
};
