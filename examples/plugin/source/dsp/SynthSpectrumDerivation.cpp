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

#include "SynthSpectrumDerivation.h"

namespace
{
/** Turns a control snapshot into the shape yup::PrismSpectrum reads. */
yup::PrismSpectrum<double>::Shape toPrismShape (const SynthOscillatorValues& values) noexcept
{
    return { static_cast<double> (values.ridgeSpacing),
             static_cast<double> (values.dispersion),
             static_cast<double> (values.squeeze),
             static_cast<double> (values.squash),
             static_cast<double> (values.tilt),
             static_cast<double> (values.oddEven),
             static_cast<double> (values.formant),
             static_cast<double> (values.formantPosition),
             static_cast<double> (values.scatter) };
}
} // namespace

void SynthSpectrumDerivation::prepare()
{
    spectrum.prepare (SynthExample::maxHarmonics);
    resampler.prepare (SynthExample::maxHarmonics);
    customSeries.resize (SynthExample::maxHarmonics);
    shapedSeries.resize (SynthExample::maxHarmonics);
    syncedSeries.resize (SynthExample::maxHarmonics);

    // The meter is only ever asked for a waveform, never played, and a frequency of
    // zero keeps every harmonic whatever rate it was prepared at.
    peakMeter.prepare (48000.0, SynthExample::maxHarmonics);
    peakMeter.setFrequency (0.0);
    peakMeter.setIncludeDC (true);

    hasApplied = false;
}

bool SynthSpectrumDerivation::update (const SynthOscillatorValues& values,
                                      const SynthPartials& partials,
                                      const SynthOscillatorResources& resources,
                                      int numOutputHarmonics) noexcept
{
    const auto neededHarmonics = numOutputHarmonics < 0 ? SynthExample::maxHarmonics
                                                        : yup::jmin (numOutputHarmonics, SynthExample::maxHarmonics);

    const auto partialsChanged = ! hasApplied
                              || applied.usesCustomSeries != values.usesCustomSeries
                              || applied.harmonicGeneration != values.harmonicGeneration
                              || applied.harmonicScale != values.harmonicScale;

    if (partialsChanged && values.usesCustomSeries)
        partials.copyHarmonicsInto (customSeries, values.harmonicScale);

    const auto& source = values.usesCustomSeries ? customSeries : resources.getFrame (values.waveform);
    const auto sourceChanged = partialsChanged || applied.waveform != values.waveform;

    const auto shapeChanged = applied.syncMode != values.syncMode
                           || applied.syncRatio != values.syncRatio
                           || applied.ridgeSpacing != values.ridgeSpacing
                           || applied.color != values.color
                           || applied.dispersion != values.dispersion
                           || applied.squeeze != values.squeeze
                           || applied.squash != values.squash
                           || applied.tilt != values.tilt
                           || applied.oddEven != values.oddEven
                           || applied.formant != values.formant
                           || applied.formantPosition != values.formantPosition
                           || applied.scatter != values.scatter;

    const auto bandwidthGrew = values.syncMode != yup::SyncMode::none && neededHarmonics > appliedOutputHarmonics;

    applied = values;
    hasApplied = true;

    if (sourceChanged)
        sourcePeak = measurePeak (source);

    if (! (sourceChanged || shapeChanged || bandwidthGrew))
        return false;

    spectrum.process (source, shapedSeries, toPrismShape (values), values.color);

    if (values.syncMode == yup::SyncMode::none)
    {
        derived = &shapedSeries;
    }
    else
    {
        const auto outputHarmonics = yup::jmin (SynthExample::maxHarmonics, neededHarmonics + yup::jmax (1, neededHarmonics / 8));

        resampler.transform (shapedSeries, static_cast<double> (values.syncRatio), values.syncMode, syncedSeries, outputHarmonics);
        appliedOutputHarmonics = outputHarmonics;
        derived = &syncedSeries;
    }

    matchSourcePeak (*derived);
    return true;
}

/** Returns the largest absolute value one period of a series reaches.

    The series is rendered with the same inverse FFT the voices use rather than
    summed harmonic by harmonic, which would cost a transcendental per harmonic per
    point.
*/
double SynthSpectrumDerivation::measurePeak (const yup::FourierSeries<double>& series) noexcept
{
    peakMeter.setSeries (series);
    peakMeter.render (false);

    const auto table = peakMeter.getRenderedTable();

    return static_cast<double> (yup::FloatVectorOperations::maxAbs (table.data(), static_cast<int> (table.size())));
}

void SynthSpectrumDerivation::matchSourcePeak (yup::FourierSeries<double>& series) noexcept
{
    const auto derivedPeak = measurePeak (series);

    if (derivedPeak <= 1.0e-9 || sourcePeak <= 1.0e-9)
        return;

    const auto scale = sourcePeak / derivedPeak;

    for (int harmonic = 1; harmonic <= series.getNumHarmonics(); ++harmonic)
        series.setHarmonic (harmonic,
                            series.getCosine (harmonic) * scale,
                            series.getSine (harmonic) * scale);
}
