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

//==============================================================================
/** One operator of a voice: the tables every unison layer reads, and their series.

    The series normally comes from the shared slot; while a modulation is routed into
    this oscillator's spectrum it is derived here instead, from the voice's own values.

    Each layer owns its table, because a table's render crossfade advances on every
    read. Only the leader, the layer playing highest, runs the inverse FFT: the matrices
    set every table's bandwidth, the leader renders when it needs to, and the followers
    copy it with renderFrom(), so none of them aliases and none of them renders.

    prepare() allocates; everything else is allocation-free.

    @see SynthOperatorNetwork, SynthOscillatorSlot, SynthSpectrumDerivation
*/
class SynthOscillator
{
public:
    using Table = yup::WavetableOscillator<float>;

    /** Allocates every table at the network's internal rate and attaches the shared state. */
    void prepare (double internalSampleRate,
                  int crossfadeLength,
                  const SynthOscillatorSlot& sharedSlot,
                  const SynthPartials& oscillatorPartials,
                  const SynthOscillatorResources& oscillatorResources);

    /** Hands the right series to every table when it moved.

        @param deriveLocally      True while a modulation reaches this oscillator's spectrum.
        @param refreshLocal       True when a locally derived series may be rebuilt now; the
                                  derivation costs inverse FFTs, so the voice rate-limits it.
        @param numLocalHarmonics  Synced harmonics a local derivation needs.
    */
    void updateSeries (const SynthOscillatorValues& values, bool deriveLocally, bool refreshLocal, int numLocalHarmonics) noexcept;

    /** Renders the leader's table if it needs it and copies it into the followers.

        Call after the layers' matrices have set the tables' bandwidths for this step.

        @param numLayers  Layers in use; the last one is the leader.
    */
    void renderTables (int numLayers) noexcept;

    /** Returns a layer's table. */
    Table& getTable (int layer) noexcept { return tables[static_cast<std::size_t> (layer)]; }

private:
    void setSeries (const yup::FourierSeries<double>& series) noexcept;

    std::array<Table, SynthExample::maxUnisonVoices> tables;
    SynthSpectrumDerivation localDerivation;

    const SynthOscillatorSlot* slot = nullptr;
    const SynthPartials* partials = nullptr;
    const SynthOscillatorResources* resources = nullptr;
    int appliedSeriesGeneration = -1;
    int sharedLayerCount = 0;
    bool usingLocalSeries = false;
};
