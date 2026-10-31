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

void SynthOscillator::prepare (double internalSampleRate,
                               int crossfadeLength,
                               const SynthOscillatorSlot& sharedSlot,
                               const SynthPartials& oscillatorPartials,
                               const SynthOscillatorResources& oscillatorResources)
{
    slot = &sharedSlot;
    partials = &oscillatorPartials;
    resources = &oscillatorResources;

    for (auto& table : tables)
        table.prepare (internalSampleRate, SynthExample::maxHarmonics, crossfadeLength);

    localDerivation.prepare();

    appliedSeriesGeneration = -1;
    sharedLayerCount = 0;
    usingLocalSeries = false;
}

void SynthOscillator::updateSeries (const SynthOscillatorValues& values, bool deriveLocally, bool refreshLocal, int numLocalHarmonics) noexcept
{
    if (deriveLocally)
    {
        if (! usingLocalSeries)
        {
            localDerivation.invalidate();
            usingLocalSeries = true;
            refreshLocal = true;
        }

        if (refreshLocal && localDerivation.update (values, *partials, *resources, numLocalHarmonics))
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

void SynthOscillator::renderTables (int numLayers) noexcept
{
    const auto layers = yup::jlimit (1, SynthExample::maxUnisonVoices, numLayers);
    auto& leader = tables[static_cast<std::size_t> (layers - 1)];

    const auto rendered = leader.needsRender();

    if (rendered)
        leader.render();

    if (! rendered && layers == sharedLayerCount)
        return;

    sharedLayerCount = layers;

    for (int layer = 0; layer < layers - 1; ++layer)
        tables[static_cast<std::size_t> (layer)].renderFrom (leader);
}

void SynthOscillator::setSeries (const yup::FourierSeries<double>& series) noexcept
{
    for (auto& table : tables)
        table.setSeries (series);
}
