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

#include "SynthOscillator.h"

void SynthOscillator::prepare (double newSampleRate,
                               int maxBlockSize,
                               const SynthOscillatorSlot& sharedSlot,
                               const SynthPartials& oscillatorPartials,
                               const SynthOscillatorResources& oscillatorResources)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

    slot = &sharedSlot;
    partials = &oscillatorPartials;
    resources = &oscillatorResources;

    wavetable.prepare (sampleRate, SynthExample::maxHarmonics);

    for (auto& satellite : satellites)
        satellite.prepare (sampleRate, SynthExample::maxHarmonics);

    localDerivation.prepare();
    slotBuffer.assign (static_cast<std::size_t> (yup::jmax (1, maxBlockSize)), 0.0f);

    appliedSeriesGeneration = -1;
    usingLocalSeries = false;
}

void SynthOscillator::reset (double initialPhase) noexcept
{
    const auto phase = static_cast<float> (initialPhase);

    wavetable.setPhase (phase);

    for (std::size_t index = 0; index < satellites.size(); ++index)
    {
        const auto offset = static_cast<float> (index + 1) / static_cast<float> (satellites.size() + 1);

        satellites[index].setPhase (phase + offset - std::floor (phase + offset));
    }
}

void SynthOscillator::renderBlock (float* left,
                                   float* right,
                                   int numSamples,
                                   const SynthOscillatorValues& values,
                                   double frequency,
                                   bool deriveLocally) noexcept
{
    yup::FloatVectorOperations::clear (left, numSamples);
    yup::FloatVectorOperations::clear (right, numSamples);

    // The table holds one period of the synced waveform, which for mirrored sync is
    // two leader periods, so it is played at the fundamental the transform produced.
    const auto played = frequency * yup::SyncSpectralResampler<double>::getFundamentalScale (values.syncMode);

    const auto slotCount = yup::jlimit (1, SynthExample::maxUnisonVoices, values.unisonVoices);
    const auto centreIndex = (slotCount - 1) / 2;
    const auto slotGain = 1.0f / static_cast<float> (slotCount);

    // A locally synced series only needs what the lowest unison slot can play below Nyquist.
    const auto lowest = yup::jmin (detunedFrequency (played, values, 0, slotCount),
                                   detunedFrequency (played, values, slotCount - 1, slotCount));

    applySeries (values, deriveLocally, yup::getNyquistHarmonicLimit (lowest, sampleRate, SynthExample::maxHarmonics));

    renderTable (wavetable, numSamples, detunedFrequency (played, values, centreIndex, slotCount));
    accumulateSlot (left, right, numSamples, slotOffset (centreIndex, slotCount) * values.unisonSpread, slotGain);

    for (int index = 0, satellite = 0; index < slotCount; ++index)
    {
        if (index == centreIndex)
            continue;

        renderTable (satellites[static_cast<std::size_t> (satellite++)],
                     numSamples,
                     detunedFrequency (played, values, index, slotCount));

        accumulateSlot (left, right, numSamples, slotOffset (index, slotCount) * values.unisonSpread, slotGain);
    }
}

//==============================================================================
/** Returns where a unison slot sits across the spread, from -1 to 1. */
float SynthOscillator::slotOffset (int slotIndex, int slotCount) noexcept
{
    if (slotCount <= 1)
        return 0.0f;

    return 2.0f * static_cast<float> (slotIndex) / static_cast<float> (slotCount - 1) - 1.0f;
}

/** Combines the oscillator's own detune with the slot's share of the unison spread. */
double SynthOscillator::detunedFrequency (double frequency, const SynthOscillatorValues& values, int slotIndex, int slotCount) noexcept
{
    const auto semitones = static_cast<double> (values.unisonDetune) * static_cast<double> (slotOffset (slotIndex, slotCount));

    return frequency * std::pow (2.0, semitones / 12.0);
}

/** Mixes the rendered slot into the stereo pair with an equal power pan. */
void SynthOscillator::accumulateSlot (float* left, float* right, int numSamples, float pan, float gain) noexcept
{
    const auto angle = (yup::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 0.25f * yup::MathConstants<float>::pi;
    const auto leftGain = std::cos (angle) * gain;
    const auto rightGain = std::sin (angle) * gain;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto value = slotBuffer[static_cast<std::size_t> (sample)];

        left[sample] += value * leftGain;
        right[sample] += value * rightGain;
    }
}

/** Renders one table into the slot buffer; render() crossfades into a new table,
    which is what keeps a modulated shape from stepping. */
void SynthOscillator::renderTable (yup::WavetableOscillator<float>& table, int numSamples, double frequency) noexcept
{
    table.setFrequency (frequency);

    if (table.needsRender())
        table.render();

    table.processBlock (slotBuffer.data(), numSamples);
}

/** Hands the right series to every table when it moved since the last block. */
void SynthOscillator::applySeries (const SynthOscillatorValues& values, bool deriveLocally, int numLocalHarmonics) noexcept
{
    if (deriveLocally)
    {
        if (! usingLocalSeries)
        {
            localDerivation.invalidate();
            usingLocalSeries = true;
        }

        if (localDerivation.update (values, *partials, *resources, numLocalHarmonics))
            setSeries (localDerivation.getSeries());

        return;
    }

    const auto generation = slot->getGeneration();

    if (usingLocalSeries || generation != appliedSeriesGeneration)
    {
        usingLocalSeries = false;
        appliedSeriesGeneration = generation;
        setSeries (slot->getSeries());
    }
}

void SynthOscillator::setSeries (const yup::FourierSeries<double>& series) noexcept
{
    wavetable.setSeries (series);

    for (auto& satellite : satellites)
        satellite.setSeries (series);
}
