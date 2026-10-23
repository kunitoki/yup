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

#include "SynthOperatorNetwork.h"

namespace
{
/** Returns the sends of an operator, indexed by bus. */
std::array<float, SynthOperatorNetwork::numBuses> getSends (const SynthOscillatorValues& values) noexcept
{
    return { values.toFilter1, values.toFilter2, values.direct };
}

/** Adds a signal scaled by a gain moving linearly across the block, reaching the end gain on its last sample. */
void addWithRamp (float* destination, const float* source, float from, float to, int numSamples) noexcept
{
    if (from == to)
    {
        yup::FloatVectorOperations::addWithMultiply (destination, source, from, numSamples);
        return;
    }

    const auto step = (to - from) / static_cast<float> (numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
        destination[sample] += source[sample] * (from + step * static_cast<float> (sample + 1));
}
} // namespace

void SynthOperatorNetwork::prepare (double newSampleRate,
                                    SynthQuality quality,
                                    const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots,
                                    const std::array<SynthPartials, SynthExample::oscillatorCount>& partials,
                                    const SynthOscillatorResources& resources)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    factor = getOversamplingFactor (quality);

    // A crossfade lasts as long at any quality.
    for (std::size_t index = 0; index < oscillators.size(); ++index)
        oscillators[index].prepare (sampleRate * factor, 64 * factor, slots[index], partials[index], resources);

    for (auto& matrix : matrices)
        matrix.prepare (sampleRate, factor);

    for (int layer = 0; layer < SynthExample::maxUnisonVoices; ++layer)
        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
            matrices[static_cast<std::size_t> (layer)].setTable (index, &oscillators[static_cast<std::size_t> (index)].getTable (layer));

    for (auto& buffer : operatorBuffers)
        buffer.assign (static_cast<std::size_t> (SynthExample::controlChunk * factor), 0.0f);

    const auto prepareDecimators = [this] (auto& set)
    {
        for (auto& decimator : set)
            decimator.prepare (sampleRate, 2, SynthExample::controlChunk);

        latency = set[0].getGenerationLatencyInSamples();
    };

    if (factor == 2)
        prepareDecimators (decimators.emplace<Decimators<2>>());
    else
        prepareDecimators (decimators.emplace<Decimators<4>>());

    const auto chunk = static_cast<std::size_t> (SynthExample::controlChunk);
    const auto delayLength = static_cast<std::size_t> (latency) + chunk;

    directScratch.assign (chunk, 0.0f);

    for (std::size_t bus = 0; bus < static_cast<std::size_t> (numBuses); ++bus)
    {
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            directBuses[bus][channel].assign (chunk, 0.0f);
            switchBuses[bus][channel].assign (chunk, 0.0f);
            delayLines[bus][channel].assign (delayLength, 0.0f);
        }
    }

    // Phase modulation that stopped is given 50 ms before the network leaves the
    // oversampled path, so a modulation crossing zero does not toggle it.
    holdSamples = static_cast<int> (0.05 * sampleRate);

    busActive.fill (false);
    busHold.fill (0);
    smoothedLayers = 0;
    numLayers = 1;
}

//==============================================================================
void SynthOperatorNetwork::startNote (const SynthNetworkControls& controls) noexcept
{
    numLayers = yup::jlimit (1, SynthExample::maxUnisonVoices, controls.globals.unisonVoices);

    for (int layer = 0; layer < SynthExample::maxUnisonVoices; ++layer)
    {
        auto& matrix = matrices[static_cast<std::size_t> (layer)];
        std::array<double, SynthExample::oscillatorCount> phases {};

        // Layers start spread over the period, so a unison stack does not begin in phase.
        const auto layerOffset = static_cast<double> (layer) / static_cast<double> (SynthExample::maxUnisonVoices);

        for (std::size_t index = 0; index < phases.size(); ++index)
        {
            const auto& values = controls.oscillators[index];
            phases[index] = values.freeRunning ? matrix.getPhase (static_cast<int> (index))
                                               : static_cast<double> (values.phase) + layerOffset;
        }

        matrix.reset (phases);
    }

    updateSeries (controls, true);

    // Tables rendered for the first time only count from the next step, so the targets
    // are set and the tables rendered twice before the first sample.
    for (int pass = 0; pass < 2; ++pass)
    {
        setTargets (controls, 0);
        renderTables (numLayers);
    }

    std::visit ([] (auto& set)
                {
                    for (auto& decimator : set)
                        decimator.reset();
                },
                decimators);

    busActive.fill (false);
    busHold.fill (0);
    smoothedLayers = 0;

    // A note picks its path straight away; only later changes go through a switch.
    const auto modulated = forceOversampled || ! controls.fm.isZero();
    zeroModulationSamples = modulated ? 0 : holdSamples;
    activePath = targetPath = modulated ? Path::oversampled : Path::direct;
    warmupRemaining = 0;
    fadePosition = 0;

    if (activePath == Path::direct)
        beginPath (Path::direct);
}

void SynthOperatorNetwork::render (const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept
{
    numSamples = yup::jlimit (0, SynthExample::controlChunk, numSamples);

    if (numSamples == 0)
        return;

    const auto requestedLayers = yup::jlimit (1, SynthExample::maxUnisonVoices, controls.globals.unisonVoices);

    if (requestedLayers != numLayers)
    {
        // New layers join from where the existing ones are, without a ramp from silence.
        numLayers = requestedLayers;
        setTargets (controls, 0);
    }

    const auto internalSamples = numSamples * factor;

    // The series first, so the tables the matrices size are the ones about to play.
    updateSeries (controls, controls.refreshSpectra);

    // While only the direct path plays, the matrices jump to their targets, so they hold
    // the current values whenever the oversampled path is started again.
    const auto playingOversampled = activePath == Path::oversampled || targetPath == Path::oversampled;
    setTargets (controls, playingOversampled ? internalSamples : 0);
    renderTables (numLayers);
    updatePanSends (controls, numSamples);
    updateBusActivity (controls, numSamples);
    updatePath (controls, numSamples);

    renderPath (activePath, outputs, numSamples, controls);

    if (targetPath == activePath)
        return;

    Outputs incoming;

    for (std::size_t bus = 0; bus < incoming.size(); ++bus)
        incoming[bus] = { switchBuses[bus][0].data(), switchBuses[bus][1].data() };

    renderPath (targetPath, incoming, numSamples, controls);

    // The path being switched to plays silently until its decimators or delay line
    // hold a latency's worth of its own output, then fades in.
    if (warmupRemaining > 0)
    {
        warmupRemaining = yup::jmax (0, warmupRemaining - numSamples);
        return;
    }

    for (std::size_t bus = 0; bus < outputs.size(); ++bus)
    {
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            auto* out = outputs[bus][channel];
            const auto* in = incoming[bus][channel];

            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto amount = yup::jmin (1.0f, static_cast<float> (fadePosition + sample + 1) / static_cast<float> (fadeLength));
                out[sample] += amount * (in[sample] - out[sample]);
            }
        }
    }

    fadePosition += numSamples;

    if (fadePosition >= fadeLength)
    {
        activePath = targetPath;
        fadePosition = 0;
    }
}

//==============================================================================
void SynthOperatorNetwork::updatePath (const SynthNetworkControls& controls, int numSamples) noexcept
{
    const auto modulated = forceOversampled || ! controls.fm.isZero();
    zeroModulationSamples = modulated ? 0 : yup::jmin (holdSamples, zeroModulationSamples + numSamples);

    const auto wanted = modulated || zeroModulationSamples < holdSamples ? Path::oversampled : Path::direct;

    if (targetPath != activePath)
    {
        // Wanting the current path back cancels the switch; the old path never stopped.
        if (wanted == activePath)
        {
            targetPath = activePath;
            warmupRemaining = 0;
            fadePosition = 0;
        }

        return;
    }

    if (wanted == activePath)
        return;

    targetPath = wanted;
    warmupRemaining = latency;
    fadePosition = 0;
    beginPath (wanted);
}

void SynthOperatorNetwork::beginPath (Path path) noexcept
{
    if (path == Path::oversampled)
    {
        for (int layer = 0; layer < SynthExample::maxUnisonVoices; ++layer)
            matrices[static_cast<std::size_t> (layer)].reset (directPhases[static_cast<std::size_t> (layer)]);

        std::visit ([] (auto& set)
                    {
                        for (auto& decimator : set)
                            decimator.reset();
                    },
                    decimators);

        return;
    }

    for (int layer = 0; layer < SynthExample::maxUnisonVoices; ++layer)
        for (int op = 0; op < SynthExample::oscillatorCount; ++op)
            directPhases[static_cast<std::size_t> (layer)][static_cast<std::size_t> (op)] = matrices[static_cast<std::size_t> (layer)].getPhase (op);

    for (auto& bus : delayLines)
        for (auto& channel : bus)
            std::fill (channel.begin(), channel.end(), 0.0f);

    delayWrite = 0;
    directLevelsValid = false;
}

void SynthOperatorNetwork::renderPath (Path path, const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept
{
    if (path == Path::oversampled)
        renderOversampled (outputs, numSamples);
    else
        renderDirect (outputs, numSamples, controls);
}

void SynthOperatorNetwork::renderOversampled (const Outputs& outputs, int numSamples) noexcept
{
    const auto internalSamples = numSamples * factor;

    std::visit ([&] (auto& set)
                {
                    std::array<std::array<float*, 2>, numBuses> internal {};
                    std::array<bool, numBuses> generating {};

                    for (int bus = 0; bus < numBuses; ++bus)
                    {
                        const auto index = static_cast<std::size_t> (bus);

                        if (! busActive[index] || ! set[index].beginGeneration (2, numSamples))
                            continue;

                        generating[index] = true;

                        for (int channel = 0; channel < 2; ++channel)
                        {
                            auto* data = set[index].getOversampledChannelData (channel);
                            yup::FloatVectorOperations::clear (data, internalSamples);
                            internal[index][static_cast<std::size_t> (channel)] = data;
                        }
                    }

                    float* operatorPointers[SynthExample::oscillatorCount];

                    for (std::size_t index = 0; index < operatorBuffers.size(); ++index)
                        operatorPointers[index] = operatorBuffers[index].data();

                    const auto layerGain = 1.0f / static_cast<float> (numLayers);

                    for (int layer = 0; layer < numLayers; ++layer)
                    {
                        auto& matrix = matrices[static_cast<std::size_t> (layer)];
                        matrix.process (operatorPointers, internalSamples);

                        for (int op = 0; op < SynthExample::oscillatorCount; ++op)
                        {
                            if (! matrix.isOperatorAudible (op))
                                continue;

                            const auto& starts = panSendStart[static_cast<std::size_t> (layer)][static_cast<std::size_t> (op)];
                            const auto& ends = panSendEnd[static_cast<std::size_t> (layer)][static_cast<std::size_t> (op)];
                            const auto* source = operatorPointers[op];

                            for (std::size_t bus = 0; bus < static_cast<std::size_t> (numBuses); ++bus)
                            {
                                if (! generating[bus])
                                    continue;

                                for (std::size_t channel = 0; channel < 2; ++channel)
                                {
                                    const auto from = starts[bus][channel] * layerGain;
                                    const auto to = ends[bus][channel] * layerGain;

                                    if (from != 0.0f || to != 0.0f)
                                        addWithRamp (internal[bus][channel], source, from, to, internalSamples);
                                }
                            }
                        }
                    }

                    for (std::size_t bus = 0; bus < static_cast<std::size_t> (numBuses); ++bus)
                    {
                        if (generating[bus])
                        {
                            float* channels[] = { outputs[bus][0], outputs[bus][1] };
                            set[bus].downsample (channels, 2, numSamples);
                        }
                        else
                        {
                            yup::FloatVectorOperations::clear (outputs[bus][0], numSamples);
                            yup::FloatVectorOperations::clear (outputs[bus][1], numSamples);
                        }
                    }
                },
                decimators);
}

void SynthOperatorNetwork::renderDirect (const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept
{
    for (std::size_t bus = 0; bus < directBuses.size(); ++bus)
        for (auto& channel : directBuses[bus])
            yup::FloatVectorOperations::clear (channel.data(), numSamples);

    const auto layerGain = 1.0f / static_cast<float> (numLayers);

    for (int layer = 0; layer < numLayers; ++layer)
    {
        const auto layerIndex = static_cast<std::size_t> (layer);
        const auto& matrix = matrices[layerIndex];
        const auto shape = getLayerShape (layer, controls.globals);

        for (int op = 0; op < SynthExample::oscillatorCount; ++op)
        {
            const auto opIndex = static_cast<std::size_t> (op);
            auto& table = oscillators[opIndex].getTable (layer);
            auto& phase = directPhases[layerIndex][opIndex];
            auto& level = directLevels[layerIndex][opIndex];
            const auto increment = getPlayedFrequency (controls, op, shape) / sampleRate;

            // Operators nobody hears still advance, so they are in phase when they return.
            if (! matrix.isOperatorAudible (op))
            {
                phase += increment * numSamples;
                phase -= std::floor (phase);
                table.advanceCrossfade (numSamples);
                level = 0.0f;
                continue;
            }

            for (int sample = 0; sample < numSamples; ++sample)
            {
                directScratch[static_cast<std::size_t> (sample)] = table.processSampleAtPhase (phase);
                phase += increment;
                phase -= std::floor (phase);
            }

            const auto& starts = panSendStart[layerIndex][opIndex];
            const auto& ends = panSendEnd[layerIndex][opIndex];
            const auto levelTarget = layerGain * controls.oscillators[opIndex].level;
            const auto levelStart = directLevelsValid ? level : levelTarget;

            level = levelTarget;

            // The level ramps across the chunk as the matrix ramps it, and pan and sends
            // glide as they do on the oversampled path.
            for (std::size_t bus = 0; bus < static_cast<std::size_t> (numBuses); ++bus)
            {
                if (! busActive[bus])
                    continue;

                for (std::size_t channel = 0; channel < 2; ++channel)
                {
                    const auto from = starts[bus][channel] * levelStart;
                    const auto to = ends[bus][channel] * levelTarget;

                    if (from != 0.0f || to != 0.0f)
                        addWithRamp (directBuses[bus][channel].data(), directScratch.data(), from, to, numSamples);
                }
            }
        }
    }

    directLevelsValid = true;

    // The delay matches the decimators, so both paths come out on the same timeline.
    const auto delayLength = static_cast<int> (delayLines[0][0].size());

    for (std::size_t bus = 0; bus < directBuses.size(); ++bus)
    {
        for (std::size_t channel = 0; channel < 2; ++channel)
        {
            auto& line = delayLines[bus][channel];
            const auto* in = directBuses[bus][channel].data();
            auto* out = outputs[bus][channel];

            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto write = (delayWrite + sample) % delayLength;
                line[static_cast<std::size_t> (write)] = in[sample];
                out[sample] = line[static_cast<std::size_t> ((write - latency + delayLength) % delayLength)];
            }
        }
    }

    delayWrite = (delayWrite + numSamples) % delayLength;
}

//==============================================================================
SynthOperatorNetwork::LayerShape SynthOperatorNetwork::getLayerShape (int layer, const SynthGlobalValues& globals) const noexcept
{
    if (numLayers <= 1)
        return {};

    // Layers are spread from -1 to 1, so the last one plays highest and leads the renders.
    const auto offset = 2.0f * static_cast<float> (layer) / static_cast<float> (numLayers - 1) - 1.0f;

    return { std::exp2 (static_cast<double> (offset * globals.unisonDetune) / 12.0), offset * globals.unisonSpread };
}

void SynthOperatorNetwork::updateSeries (const SynthNetworkControls& controls, bool refreshLocal) noexcept
{
    for (std::size_t index = 0; index < oscillators.size(); ++index)
    {
        const auto& values = controls.oscillators[index];
        const auto lowest = values.getFrequency (controls.noteFrequency) * getLayerShape (0, controls.globals).frequencyScale;
        const auto harmonics = yup::getNyquistHarmonicLimit (lowest, sampleRate, SynthExample::maxHarmonics);

        oscillators[index].updateSeries (values, controls.deriveLocally[index], refreshLocal, harmonics);
    }
}

double SynthOperatorNetwork::getPlayedFrequency (const SynthNetworkControls& controls, int op, const LayerShape& shape) const noexcept
{
    const auto& values = controls.oscillators[static_cast<std::size_t> (op)];

    // The table holds one period of the synced waveform, which for mirrored sync is
    // two leader periods, so it is played at the fundamental the transform produced.
    return values.getFrequency (controls.noteFrequency)
         * yup::SyncSpectralResampler<double>::getFundamentalScale (values.syncMode)
         * shape.frequencyScale;
}

std::array<float, 2> SynthOperatorNetwork::getPanGains (float pan, float gain) noexcept
{
    const auto angle = (yup::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 0.25f * yup::MathConstants<float>::pi;
    return { std::cos (angle) * gain, std::sin (angle) * gain };
}

void SynthOperatorNetwork::setTargets (const SynthNetworkControls& controls, int numInternalSamples) noexcept
{
    for (int layer = 0; layer < numLayers; ++layer)
    {
        const auto shape = getLayerShape (layer, controls.globals);
        yup::PhaseModulationMatrix<float>::Controls targets;

        for (std::size_t op = 0; op < targets.frequencies.size(); ++op)
        {
            const auto& values = controls.oscillators[op];

            targets.frequencies[op] = getPlayedFrequency (controls, static_cast<int> (op), shape);
            targets.gains[op] = values.getTotalSend() > 0.0f ? static_cast<double> (values.level) : 0.0;

            for (std::size_t modulator = 0; modulator < targets.frequencies.size(); ++modulator)
                targets.amounts[op][modulator] = static_cast<double> (controls.fm.amounts[op][modulator]);
        }

        matrices[static_cast<std::size_t> (layer)].setTargets (targets, numInternalSamples);
    }
}

void SynthOperatorNetwork::renderTables (int layers) noexcept
{
    const auto& leader = matrices[static_cast<std::size_t> (layers - 1)];

    for (int op = 0; op < SynthExample::oscillatorCount; ++op)
        if (leader.isOperatorActive (op))
            oscillators[static_cast<std::size_t> (op)].renderTables (layers);
}

/** Moves every layer's pan and sends a chunk's step towards their targets, as a one-pole
    smoother stepped once per chunk, so an abrupt change glides over a few milliseconds.
    A layer that was not playing starts on its target. */
void SynthOperatorNetwork::updatePanSends (const SynthNetworkControls& controls, int numSamples) noexcept
{
    const auto amount = static_cast<float> (1.0 - std::exp (-static_cast<double> (numSamples) / (panSendSmoothingSeconds * sampleRate)));

    for (int layer = 0; layer < numLayers; ++layer)
    {
        const auto layerIndex = static_cast<std::size_t> (layer);
        const auto shape = getLayerShape (layer, controls.globals);
        const auto continues = layer < smoothedLayers;

        for (int op = 0; op < SynthExample::oscillatorCount; ++op)
        {
            const auto opIndex = static_cast<std::size_t> (op);
            const auto& values = controls.oscillators[opIndex];
            const auto panGains = getPanGains (values.pan + shape.panOffset, 1.0f);
            const auto sends = getSends (values);

            for (std::size_t bus = 0; bus < static_cast<std::size_t> (numBuses); ++bus)
            {
                for (std::size_t channel = 0; channel < 2; ++channel)
                {
                    const auto target = sends[bus] * panGains[channel];
                    auto& start = panSendStart[layerIndex][opIndex][bus][channel];
                    auto& end = panSendEnd[layerIndex][opIndex][bus][channel];

                    start = continues ? end : target;
                    end = start + (target - start) * amount;

                    if (std::abs (end - target) < panSendSilence)
                        end = target;
                }
            }
        }
    }

    smoothedLayers = numLayers;
}

void SynthOperatorNetwork::updateBusActivity (const SynthNetworkControls& controls, int numSamples) noexcept
{
    std::array<bool, numBuses> sending {};

    for (int op = 0; op < SynthExample::oscillatorCount; ++op)
    {
        const auto& values = controls.oscillators[static_cast<std::size_t> (op)];

        if (values.level <= 0.0f)
            continue;

        const auto sends = getSends (values);

        // A send that was just turned off is still gliding out, so it keeps its bus going.
        for (std::size_t bus = 0; bus < sending.size(); ++bus)
        {
            sending[bus] = sending[bus] || sends[bus] > 0.0f;

            for (int layer = 0; layer < numLayers && ! sending[bus]; ++layer)
            {
                const auto& end = panSendEnd[static_cast<std::size_t> (layer)][static_cast<std::size_t> (op)][bus];
                sending[bus] = end[0] != 0.0f || end[1] != 0.0f;
            }
        }
    }

    for (std::size_t bus = 0; bus < sending.size(); ++bus)
    {
        if (sending[bus])
        {
            if (! busActive[bus])
                std::visit ([bus] (auto& set) { set[bus].reset(); }, decimators);

            busActive[bus] = true;
            busHold[bus] = 2 * latency + SynthExample::controlChunk;
            continue;
        }

        // A bus nothing sends to keeps decimating until its tail has left the filter.
        busHold[bus] = yup::jmax (0, busHold[bus] - numSamples);
        busActive[bus] = busActive[bus] && busHold[bus] > 0;
    }
}
