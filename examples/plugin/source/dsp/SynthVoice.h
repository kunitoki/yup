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

#include "SynthBlockContext.h"
#include "SynthFilterStage.h"
#include "SynthSound.h"

#include <vector>

//==============================================================================
/** A polyphonic voice: the operator network into two filters and the amplitude envelope.

    Modulation is applied once per control chunk: the voice copies the block's value
    table, adds every route in normalized parameter space and reads its controls from
    the copy, so a route can reach any continuous parameter.

    The network's buses come out a fixed latency late, so everything after it lives on
    that delayed timeline: the amplitude envelope's note events and the filter controls
    are delayed by the same amount. Envelope 1 is both a modulation source, on the
    network's timeline, and the amplitude, on the delayed one; the voice ends when the
    delayed copy finishes.

    @see SynthOperatorNetwork, SynthFilterStage
*/
class SynthVoice : public yup::SynthesiserVoice
{
public:
    SynthVoice (const SynthParameters& synthParameters,
                const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials,
                const SynthOscillatorResources& oscillatorResources,
                const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots,
                const SynthBlockContext& blockContext);

    /** Allocates the network and every buffer. Must run outside the audio callback.

        @param processorParameters  Every parameter of the processor, by index, which the
                                    routes use to map values into normalized space.
    */
    void prepare (double sampleRate, int maxBlockSize, SynthQuality quality, yup::Span<const yup::AudioParameter::Ptr> processorParameters);

    /** Returns how late the voice output is, in samples. */
    int getLatencyInSamples() const noexcept { return network.getLatencyInSamples(); }

    //==============================================================================
    bool canPlaySound (yup::SynthesiserSound* sound) override;

    /** Configures the next mono transition. Legato retains the envelopes and phases.
        Glide is measured in seconds and interpolates pitch in semitones. */
    void setTransition (double seconds, bool legato) noexcept;

    void startNote (int midiNoteNumber, float velocity, yup::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote (float, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int, int) override {}

    /** Includes a recycled voice's short continuation in the activity meter. */
    bool isSounding() const noexcept { return isVoiceActive() || tailPosition < tailLength; }

    /** True while the voice plays its oversampled path, for the load readout. */
    bool isOversampling() const noexcept { return network.isOversampling(); }

    /** Returns the phase of one of this voice's LFOs, for the display. */
    float getLFOPhase (int lfoIndex) const noexcept { return lfos[static_cast<std::size_t> (lfoIndex)].getPhase(); }

    /** Returns where an envelope is along its points (see yup::CurveEnvelope::getPointPosition()), or -1 when it is
        not playing: finished, or a modulation envelope nothing reads. The first envelope
        is the amplitude, which always plays. */
    float getEnvelopePosition (int envelopeIndex) const noexcept;

    /** Returns how far a route slot moved its destination in the last block, in the
        destination's normalized range. */
    float getRouteAmount (int slot) const noexcept
    {
        return yup::isPositiveAndBelow (slot, SynthExample::modulationSlots) ? routeAmounts[static_cast<std::size_t> (slot)] : 0.0f;
    }

    //==============================================================================
    void renderNextBlock (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    //==============================================================================
    /** The post-network controls of one chunk, replayed a latency later. */
    struct DelayedControls
    {
        std::array<SynthFilterValues, SynthExample::filterCount> filters;
        SynthFilterRouting routing = SynthFilterRouting::serial;
        float ampVelocityGain = 1.0f;
        double note = 60.0;
    };

    static constexpr int delayedChunks = 16;

    void renderChunk (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept;
    /** Moves each operator's enable gain toward its switch, over enableFadeSeconds. */
    void fadeEnables (const SynthNetworkControls& controls, int numSamples) noexcept;
    void prepareControls (SynthNetworkControls& controls, DelayedControls& delayed) noexcept;
    void configureLFOs() noexcept;
    float getSourceValue (SynthModulationSource source) const noexcept;
    void advanceSources (int numSamples) noexcept;
    void applyCurves() noexcept;
    void renderAmplitude (float* gains, int numSamples) noexcept;

    static double midiNoteToFrequency (double midiNoteNumber) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
    }

    //==============================================================================
    static constexpr double pitchWheelRangeSemitones = 2.0;

    const SynthParameters& parameters;
    const std::array<SynthPartials, SynthExample::oscillatorCount>& partials;
    const SynthOscillatorResources& resources;
    const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots;
    const SynthBlockContext& context;
    yup::Span<const yup::AudioParameter::Ptr> processorParameters;

    SynthOperatorNetwork network;
    std::array<SynthFilterStage, SynthExample::filterCount> filters;
    std::array<yup::CurveEnvelope<float>, SynthExample::envelopeCount> envelopes;
    yup::CurveEnvelope<float> amplitude;
    std::array<yup::LFO<float>, SynthExample::lfoCount> lfos;

    std::array<SynthEnvelopeValues, SynthExample::envelopeCount> envelopeValues;
    std::array<SynthLFOValues, SynthExample::lfoCount> lfoValues;
    std::vector<float> modulatedValues;
    std::array<float, SynthExample::modulationSlots> routeAmounts {};
    std::array<float, SynthExample::oscillatorCount> enableGains {};
    std::array<std::array<std::vector<float>, 2>, SynthOperatorNetwork::numBuses> buses;
    std::vector<float> gains;
    std::vector<float> scratch;

    std::array<DelayedControls, delayedChunks> delayedControls;
    int delayedWrite = 0;
    int delayChunks = 1;
    int gateOnCountdown = -1;
    int gateOffCountdown = -1;
    int samplesSinceSpectra = 0;
    int appliedCurveGeneration = -1;

    yup::SmoothedValue<double> pitch;
    yup::SmoothedValue<double> bend;
    yup::AudioBuffer<float> tailBuffer;
    yup::AudioBuffer<float> tailScratch;
    yup::Random random;
    double playbackRate = 44100.0;
    double glideSeconds = 0.0;
    int tailLength = 0;
    int tailPosition = 0;
    bool preserveNote = false;
    bool hasPlayed = false;
    double currentNote = 60.0;
    float noteVelocity = 1.0f;
    float noteRandom = 0.0f;
};
