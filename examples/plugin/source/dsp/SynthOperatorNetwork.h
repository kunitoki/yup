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

#include "SynthFMParameters.h"
#include "SynthOscillator.h"
#include "SynthParameters.h"

#include <variant>

//==============================================================================
/** What the operator network plays for one control chunk, modulation applied. */
struct SynthNetworkControls
{
    std::array<SynthOscillatorValues, SynthExample::oscillatorCount> oscillators;
    SynthFMValues fm;
    SynthGlobalValues globals;
    double noteFrequency = 440.0;
    std::array<bool, SynthExample::oscillatorCount> deriveLocally {};
    bool refreshSpectra = false; /**< True when local spectra may be rebuilt in this chunk. */

    /** Scales each operator by its enable gain: its level and the phase modulation it gives
        and takes. A gain of 0 silences the operator and removes it from the matrix, so a
        disabled operator stops rendering; the voice fades the gains over a few
        milliseconds so switching an operator never clicks.
    */
    void applyEnables (const std::array<float, SynthExample::oscillatorCount>& gains) noexcept
    {
        for (std::size_t op = 0; op < oscillators.size(); ++op)
        {
            const auto gain = gains[op];

            if (gain >= 1.0f)
                continue;

            oscillators[op].level *= gain;

            for (std::size_t other = 0; other < oscillators.size(); ++other)
            {
                fm.amounts[other][op] *= gain;
                fm.amounts[op][other] *= gain;
            }
        }
    }
};

//==============================================================================
/** The four operators of a voice as a phase modulation network, stacked for unison.

    Each unison layer runs its own yup::PhaseModulationMatrix over its own copy of the
    operator tables, at the oversampled rate. Every layer's operators are panned and sent
    to three stereo buses, the two filter inputs and the direct output, and the layers
    are summed before decimation, so the decimators cost the same whatever the unison.
    Only buses something sends to are decimated, with the linear-phase FIR halfband
    cascade whose stopband starts at the host Nyquist.

    Without phase modulation nothing needs the oversampled rate: the matrices hold every
    table to the host Nyquist, so the same tables are read directly at the host rate,
    through a delay equal to the decimators' latency. The network switches to that path
    once every FM amount has been zero for a while, and back as soon as one is not. A
    switch starts the other path with the same phases, lets it warm up for one latency
    and crossfades into it, so neither the timing nor the timbre jumps.

    The buses come out at the host rate, getLatencyInSamples() late, on either path.

    prepare() allocates; the rest is allocation-free.

    @see yup::PhaseModulationMatrix, SynthOscillator, SynthVoice
*/
class SynthOperatorNetwork
{
public:
    /** The stereo buses the network writes. */
    enum Bus
    {
        filter1,
        filter2,
        direct,
        numBuses
    };

    /** One output pointer pair per bus, at the host rate. */
    using Outputs = std::array<std::array<float*, 2>, numBuses>;

    /** Allocates every table, matrix and decimator for a quality setting. */
    void prepare (double sampleRate,
                  SynthQuality quality,
                  const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots,
                  const std::array<SynthPartials, SynthExample::oscillatorCount>& partials,
                  const SynthOscillatorResources& resources);

    /** Returns how late the buses are, in host samples. */
    int getLatencyInSamples() const noexcept { return latency; }

    /** Starts a note: restarts the operators that are not free running and clears the
        decimators. Not called for legato transitions, which keep everything running.
    */
    void startNote (const SynthNetworkControls& controls) noexcept;

    /** Renders one control chunk of at most SynthExample::controlChunk samples.

        Every bus pointer is written, silent buses with zeros.
    */
    void render (const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept;

    /** Keeps the network on the oversampled path even without phase modulation, so the
        two paths can be compared.
    */
    void setForceOversampled (bool shouldForce) noexcept { forceOversampled = shouldForce; }

    /** True while the oversampled path is the one being heard. */
    bool isOversampling() const noexcept { return activePath == Path::oversampled; }

private:
    //==============================================================================
    // Float coefficients select the SIMD dot product; with double ones the decimator
    // falls back to a scalar loop that costs several percent per voice.
    template <int Factor>
    using Decimators = std::array<yup::HalfbandOversampler<float, Factor, float>, numBuses>;

    struct LayerShape
    {
        double frequencyScale = 1.0;
        float panOffset = 0.0f;
    };

    enum class Path
    {
        oversampled,
        direct
    };

    static constexpr int fadeLength = 64;
    static constexpr double panSendSmoothingSeconds = 0.005;
    static constexpr double phaseSmoothingSeconds = 0.02;
    static constexpr float panSendSilence = 1.0e-5f;

    using Buses = std::array<std::array<std::vector<float>, 2>, numBuses>;
    using PanSends = std::array<std::array<std::array<std::array<float, 2>, numBuses>, SynthExample::oscillatorCount>, SynthExample::maxUnisonVoices>;

    LayerShape getLayerShape (int layer, const SynthGlobalValues& globals) const noexcept;
    void updateSeries (const SynthNetworkControls& controls, bool refreshLocal) noexcept;
    void setTargets (const SynthNetworkControls& controls, int numInternalSamples) noexcept;
    void renderTables (int numLayers) noexcept;
    void updatePanSends (const SynthNetworkControls& controls, int numSamples) noexcept;
    void updateBusActivity (const SynthNetworkControls& controls, int numSamples) noexcept;
    void updatePath (const SynthNetworkControls& controls, int numSamples) noexcept;
    void beginPath (Path path) noexcept;
    void renderPath (Path path, const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept;
    void renderOversampled (const Outputs& outputs, int numSamples) noexcept;
    void renderDirect (const Outputs& outputs, int numSamples, const SynthNetworkControls& controls) noexcept;
    double getPlayedFrequency (const SynthNetworkControls& controls, int op, const LayerShape& shape) const noexcept;
    static std::array<float, 2> getPanGains (float pan, float gain) noexcept;

    //==============================================================================
    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<yup::PhaseModulationMatrix<float>, SynthExample::maxUnisonVoices> matrices;
    std::array<std::vector<float>, SynthExample::oscillatorCount> operatorBuffers;
    std::variant<Decimators<2>, Decimators<4>> decimators;

    std::array<bool, numBuses> busActive {};
    std::array<int, numBuses> busHold {};

    // Each operator's pan and sends per layer, smoothed across chunks so that an abrupt
    // change glides: where the current chunk starts and ends, for both paths alike.
    PanSends panSendStart {};
    PanSends panSendEnd {};
    int smoothedLayers = 0;

    // Each operator's phase offset, smoothed across chunks like pan and sends.
    std::array<double, SynthExample::oscillatorCount> smoothedPhases {};

    // The direct path: phases, phase offsets and last levels per layer and operator, the
    // delay that aligns it with the decimators, and the second path's output during a switch.
    std::array<std::array<double, SynthExample::oscillatorCount>, SynthExample::maxUnisonVoices> directPhases {};
    std::array<std::array<double, SynthExample::oscillatorCount>, SynthExample::maxUnisonVoices> directOffsets {};
    std::array<std::array<float, SynthExample::oscillatorCount>, SynthExample::maxUnisonVoices> directLevels {};
    std::vector<float> directScratch;
    Buses directBuses;
    Buses delayLines;
    Buses switchBuses;
    int delayWrite = 0;
    bool directLevelsValid = false;

    Path activePath = Path::oversampled;
    Path targetPath = Path::oversampled;
    int warmupRemaining = 0;
    int fadePosition = 0;
    int zeroModulationSamples = 0;
    int holdSamples = 0;
    bool forceOversampled = false;

    double sampleRate = 44100.0;
    int factor = 4;
    int latency = 0;
    int numLayers = 1;
};
