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

#include "SynthParameters.h"
#include "SynthVoice.h"

#include <vector>

#include <atomic>

//==============================================================================
/** Polyphonic synthesiser rendering the operator network of every voice.

    MIDI and rendering methods belong to the audio thread. The parameters are read from
    the per-chunk value table the processor passes in, the drawn partials from
    SynthPartials, whose generation tells the oscillator slots when to rebuild, and the
    routes, curves and tuning from their patch state objects, once per block. Keys the
    tuning leaves unmapped start no note.

    @see SynthParameters, SynthPartials, SynthRoutes, SynthCurveState, SynthTuning, RetraceProcessor
*/
class HarmonicSynthEngine : public yup::Synthesiser
{
public:
    HarmonicSynthEngine (const SynthParameters& parameters,
                         const std::array<SynthPartials, SynthExample::oscillatorCount>& partials,
                         const SynthRoutes& routes,
                         const SynthCurveState& curves,
                         const SynthTuning& tuning);

    /** Prepares every voice at a quality. Must run outside the audio callback.

        @param processorParameters  Every parameter of the processor, by index.
    */
    void prepare (double sampleRate, int maxBlockSize, SynthQuality quality, yup::Span<const yup::AudioParameter::Ptr> processorParameters);

    /** Returns how late the output is, in samples. Valid after prepare(). */
    int getLatencyInSamples() const noexcept { return latency; }

    /** Requests a release of all keys without taking the synthesiser lock on the UI thread. */
    void requestAllNotesOff() noexcept { releaseRequested.store (true); }

    /** Renders one range of the buffer with sample-accurate note boundaries.

        Only the MIDI events inside the range are handled, so a block can be rendered as
        a sequence of control chunks, each with its own parameter values.

        @param output           The buffer the voices add into
        @param midi             The whole block's MIDI; events outside the range are left alone
        @param startSample      The first sample of the range
        @param numSamples       The length of the range
        @param parameterValues  One real value per parameter index, for this range
        @param bpm              The host tempo, or zero when unknown
    */
    void renderRange (yup::AudioBuffer<float>& output,
                      const yup::MidiBuffer& midi,
                      int startSample,
                      int numSamples,
                      yup::Span<const float> parameterValues,
                      double bpm);

    void noteOn (int channel, int note, float velocity) override;
    void noteOff (int channel, int note, float velocity, bool tailOff) override;
    void allNotesOff (int channel, bool tailOff) override;
    void handleController (int channel, int controller, int value) override;
    void handleChannelPressure (int channel, int channelPressureValue) override;
    void handlePitchWheel (int channel, int wheelValue) override;

    /** Returns the phase of an LFO in the most recently started voice, for its display,
        or -1 when no note plays. */
    float getLFOPhase (int lfoIndex) const noexcept
    {
        return lfoPhases[static_cast<std::size_t> (lfoIndex)].load();
    }

    /** Returns how far a route slot moves its destination in the most recent note, in the
        destination's normalized range, or 0 when no note plays. */
    float getRouteAmount (int slot) const noexcept
    {
        if (! yup::isPositiveAndBelow (slot, SynthExample::modulationSlots))
            return 0.0f;

        return routeAmounts[static_cast<std::size_t> (slot)].load();
    }

    /** Returns the parameter values of the last rendered range with the routes applied
        monophonically, for global processing such as the master effects.

        Like a monophonic synth: macros, the wheels and aftertouch apply directly, while
        the per-note sources (envelopes, LFOs, velocity, note, random) follow the most
        recently started voice and hold their last value once no note plays. Call it on the
        audio thread after renderRange().

        @param storage  Room for one value per parameter index, filled when a route is active
        @returns        The modulated values, or the played ones when no route is active
    */
    yup::Span<const float> getGlobalValues (std::vector<float>& storage) const noexcept;

    /** Returns where an envelope of the most recent note is along its points (see yup::CurveEnvelope::getPointPosition()),
        or -1 when no note plays it. */
    float getEnvelopePosition (int envelopeIndex) const noexcept
    {
        if (! yup::isPositiveAndBelow (envelopeIndex, SynthExample::envelopeCount))
            return -1.0f;

        return envelopePositions[static_cast<std::size_t> (envelopeIndex)].load();
    }

    /** Returns the shared waveform presets, which the waveform displays also read. */
    const SynthOscillatorResources& getResources() const noexcept { return resources; }

    /** Returns the note of a sounding voice, or -1 when the synthesiser is silent. */
    int getCurrentlyPlayingNote() const noexcept;

    /** Returns how many voices are currently sounding. */
    int getNumActiveVoices() const noexcept { return activeVoices.load(); }

    /** Returns how many sounding voices run their operators oversampled. */
    int getNumOversamplingVoices() const noexcept { return oversamplingVoices.load(); }

private:
    void updateContext (yup::Span<const float> parameterValues, double bpm) noexcept;
    void updateRoutes (const SynthParameterReader& reader) noexcept;
    void publishVoiceState() noexcept;
    float getGlobalSourceValue (SynthModulationSource source, int slot) const noexcept;
    void playMonoNote (int channel, int note, float velocity);
    void selectMonoNote (bool tailOff);

    struct HeldNote
    {
        yup::uint64 order = 0;
        float velocity = 0.0f;
        bool down = false;
    };

    const SynthParameters& parameters;
    const std::array<SynthPartials, SynthExample::oscillatorCount>& partials;
    const SynthRoutes& routes;
    const SynthCurveState& curves;
    const SynthTuning& tuning;

    /** Per parameter index, the oscillator whose spectrum it shapes, or -1. */
    std::vector<int> spectralOwner;
    int latency = 0;
    int samplesSinceSpectra = SynthExample::spectralChunk;

    std::array<HeldNote, 16 * 128> heldNotes {};
    std::array<bool, 16> sustain {};
    yup::uint64 noteOrder = 0;
    SynthPlayMode mode = SynthPlayMode::poly;
    double portamentoSeconds = 0.0;
    bool monoKeyActive = false;
    std::atomic<bool> releaseRequested { false };
    std::atomic<int> activeVoices { 0 };
    std::atomic<int> oversamplingVoices { 0 };

    SynthOscillatorResources resources;
    std::array<SynthOscillatorSlot, SynthExample::oscillatorCount> oscillatorSlots;
    std::array<std::atomic<float>, SynthExample::lfoCount> lfoPhases {};
    std::array<std::atomic<float>, SynthExample::envelopeCount> envelopePositions {};
    std::array<std::atomic<float>, SynthExample::modulationSlots> routeAmounts {};

    /** Per route slot, the per-note source value of the most recent voice, kept once it stops. */
    std::array<float, SynthExample::modulationSlots> heldSourceValues {};
    std::array<SynthModulationSource, SynthExample::modulationSlots> heldSources {};
    yup::Span<const yup::AudioParameter::Ptr> processorParameters;
    SynthBlockContext context;
    yup::ReferenceCountedArray<SynthVoice> ownedVoices;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicSynthEngine)
};
