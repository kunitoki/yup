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

#include "HarmonicSynthEngine.h"

HarmonicSynthEngine::HarmonicSynthEngine (const SynthParameters& synthParameters,
                                          const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials,
                                          const SynthRoutes& synthRoutes,
                                          const SynthCurveState& synthCurves,
                                          const SynthTuning& synthTuning)
    : parameters (synthParameters)
    , partials (oscillatorPartials)
    , routes (synthRoutes)
    , curves (synthCurves)
    , tuning (synthTuning)
{
    addSound (new SynthSound());

    for (int index = 0; index < SynthExample::voiceCount; ++index)
    {
        auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (parameters, partials, resources, oscillatorSlots, context));

        addVoice (voice);
        ownedVoices.add (voice);
    }
}

void HarmonicSynthEngine::prepare (double sampleRate, int maxBlockSize, SynthQuality quality, yup::Span<const yup::AudioParameter::Ptr> newProcessorParameters)
{
    allNotesOff (0, false);
    setCurrentPlaybackSampleRate (sampleRate);
    activeVoices.store (0);
    processorParameters = newProcessorParameters;
    heldSourceValues.fill (0.0f);

    for (int index = 0; index < ownedVoices.size(); ++index)
        ownedVoices[index]->prepare (sampleRate, maxBlockSize, quality, newProcessorParameters);

    latency = ownedVoices.isEmpty() ? 0 : ownedVoices[0]->getLatencyInSamples();

    spectralOwner.assign (newProcessorParameters.size(), -1);

    for (std::size_t oscillator = 0; oscillator < parameters.oscillators.size(); ++oscillator)
        for (const auto& parameter : parameters.oscillators[oscillator].getSpectralParameters())
            if (yup::isPositiveAndBelow (parameter->getIndexInContainer(), static_cast<int> (spectralOwner.size())))
                spectralOwner[static_cast<std::size_t> (parameter->getIndexInContainer())] = static_cast<int> (oscillator);

    // Whatever the curves and the tuning are, the first block copies them.
    context.curveGeneration = -1;
    curves.copyIfChanged (context.envelopeCurves, context.lfoCurves, context.curveGeneration);
    context.tuningGeneration = -1;
    tuning.copyIfChanged (context.tuning, context.tuningGeneration);
}

void HarmonicSynthEngine::renderRange (yup::AudioBuffer<float>& output,
                                       const yup::MidiBuffer& midi,
                                       int startSample,
                                       int numSamples,
                                       yup::Span<const float> parameterValues,
                                       double bpm)
{
    const auto requestedMode = static_cast<SynthPlayMode> (yup::jlimit (0, 2, yup::roundToInt (parameterValues[static_cast<std::size_t> (parameters.playMode->getIndexInContainer())])));
    const auto release = releaseRequested.exchange (false);
    if (requestedMode != mode || release)
    {
        allNotesOff (0, true);
        mode = requestedMode;
    }

    updateContext (parameterValues, bpm);
    samplesSinceSpectra += numSamples;

    // yup::Synthesiser::renderNextBlock() handles every event left in the buffer once it
    // reaches the end of its range, so the range is walked here instead.
    const auto endSample = startSample + numSamples;

    for (auto iterator = midi.findNextSamplePosition (startSample); iterator != midi.cend(); ++iterator)
    {
        const auto metadata = *iterator;

        if (metadata.samplePosition >= endSample)
            break;

        if (metadata.samplePosition > startSample)
        {
            renderVoices (output, startSample, metadata.samplePosition - startSample);
            startSample = metadata.samplePosition;
        }

        handleMidiEvent (metadata.getMessage());
    }

    if (endSample > startSample)
        renderVoices (output, startSample, endSample - startSample);

    publishVoiceState();
}

void HarmonicSynthEngine::updateContext (yup::Span<const float> parameterValues, double bpm) noexcept
{
    const SynthParameterReader reader { parameterValues };

    context.played = parameterValues;
    context.bpm = bpm;

    for (std::size_t index = 0; index < context.macros.size(); ++index)
        context.macros[index] = reader (parameters.macros[index]);

    context.network.globals = parameters.readGlobals (reader);
    portamentoSeconds = static_cast<double> (context.network.globals.glideSeconds);

    for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
    {
        auto& values = context.network.oscillators[slot];
        values = parameters.oscillators[slot].read (reader);
        partials[slot].fillValues (values);
    }

    context.network.fm = parameters.fm.read (reader);

    for (std::size_t index = 0; index < context.filters.size(); ++index)
        context.filters[index] = parameters.filters[index].read (reader);

    for (std::size_t index = 0; index < context.envelopes.size(); ++index)
        context.envelopes[index] = parameters.envelopes[index].read (reader);

    for (std::size_t index = 0; index < context.lfos.size(); ++index)
        context.lfos[index] = parameters.lfos[index].read (reader);

    updateRoutes (reader);

    curves.copyIfChanged (context.envelopeCurves, context.lfoCurves, context.curveGeneration);
    tuning.copyIfChanged (context.tuning, context.tuningGeneration);
    context.bendDown = static_cast<double> (tuning.getBendDown());
    context.bendUp = static_cast<double> (tuning.getBendUp());
    context.bendMode = tuning.getBendMode();

    // Every voice of a slot plays the same spectrum unless a modulation is routed
    // into it, so it is derived once here rather than once per voice, and no more
    // often than the spectral interval, since each derivation costs inverse FFTs.
    if (samplesSinceSpectra >= SynthExample::spectralChunk)
    {
        samplesSinceSpectra = 0;

        for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
            oscillatorSlots[slot].update (context.network.oscillators[slot], partials[slot], resources);
    }
}

void HarmonicSynthEngine::updateRoutes (const SynthParameterReader& reader) noexcept
{
    context.numRoutes = 0;
    context.spectralRouted.fill (false);
    context.envelopeRouted.fill (false);
    context.lfoRouted.fill (false);

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
    {
        const auto route = routes.getRoute (slot);
        const auto depth = reader (parameters.modulation.depths[static_cast<std::size_t> (slot)]);

        if (! route.isActive() || depth == 0.0f || ! yup::isPositiveAndBelow (route.destination, static_cast<int> (spectralOwner.size())))
            continue;

        const auto index = static_cast<std::size_t> (context.numRoutes++);
        context.routes[index] = route;
        context.depths[index] = depth;
        context.slots[index] = slot;

        if (const auto owner = spectralOwner[static_cast<std::size_t> (route.destination)]; owner >= 0)
            context.spectralRouted[static_cast<std::size_t> (owner)] = true;

        if (const auto envelope = getSourceEnvelope (route.source); envelope >= 0)
            context.envelopeRouted[static_cast<std::size_t> (envelope)] = true;

        if (const auto lfo = getSourceLFO (route.source); lfo >= 0)
            context.lfoRouted[static_cast<std::size_t> (lfo)] = true;
    }
}

void HarmonicSynthEngine::publishVoiceState() noexcept
{
    int active = 0;
    int oversampling = 0;
    const SynthVoice* newest = nullptr;

    for (auto* voice : ownedVoices)
    {
        active += voice->isSounding() ? 1 : 0;
        oversampling += voice->isSounding() && voice->isOversampling() ? 1 : 0;

        if (voice->isVoiceActive() && (newest == nullptr || newest->wasStartedBefore (*voice)))
            newest = voice;
    }

    activeVoices.store (active);
    oversamplingVoices.store (oversampling);

    // The displays follow the LFOs and envelopes of the most recent note; without one they rest.
    for (int index = 0; index < SynthExample::lfoCount; ++index)
        lfoPhases[static_cast<std::size_t> (index)].store (newest != nullptr ? newest->getLFOPhase (index) : -1.0f);

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
        envelopePositions[static_cast<std::size_t> (index)].store (newest != nullptr ? newest->getEnvelopePosition (index) : -1.0f);

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
        routeAmounts[static_cast<std::size_t> (slot)].store (newest != nullptr ? newest->getRouteAmount (slot) : 0.0f);

    if (newest == nullptr)
        return;

    for (int index = 0; index < context.numRoutes; ++index)
    {
        const auto slot = static_cast<std::size_t> (context.slots[static_cast<std::size_t> (index)]);
        heldSources[slot] = context.routes[static_cast<std::size_t> (index)].source;
        heldSourceValues[slot] = newest->getRouteAmount (static_cast<int> (slot)) / context.depths[static_cast<std::size_t> (index)];
    }
}

float HarmonicSynthEngine::getGlobalSourceValue (SynthModulationSource source, int slot) const noexcept
{
    if (const auto macro = getSourceMacro (source); macro >= 0)
        return context.macros[static_cast<std::size_t> (macro)];

    if (source == SynthModulationSource::modWheel)
        return context.modWheel;

    if (source == SynthModulationSource::aftertouch)
        return context.aftertouch;

    if (source == SynthModulationSource::pitchBend)
        return context.pitchBend;

    const auto index = static_cast<std::size_t> (slot);
    return heldSources[index] == source ? heldSourceValues[index] : 0.0f;
}

yup::Span<const float> HarmonicSynthEngine::getGlobalValues (std::vector<float>& storage) const noexcept
{
    if (context.numRoutes == 0 || storage.size() < context.played.size() || processorParameters.size() < context.played.size())
        return context.played;

    std::copy (context.played.begin(), context.played.end(), storage.begin());

    for (int index = 0; index < context.numRoutes; ++index)
    {
        const auto& route = context.routes[static_cast<std::size_t> (index)];
        const auto amount = context.depths[static_cast<std::size_t> (index)] * getGlobalSourceValue (route.source, context.slots[static_cast<std::size_t> (index)]);

        if (amount == 0.0f || ! yup::isPositiveAndBelow (route.destination, static_cast<int> (context.played.size())))
            continue;

        const auto destination = static_cast<std::size_t> (route.destination);
        const auto& parameter = processorParameters[destination];
        const auto normalized = yup::jlimit (0.0f, 1.0f, parameter->convertToNormalizedValue (storage[destination]) + amount);

        storage[destination] = parameter->convertToDenormalizedValue (normalized);
    }

    return { storage.data(), context.played.size() };
}

//==============================================================================
void HarmonicSynthEngine::noteOn (int channel, int note, float velocity)
{
    if (! context.tuning.isMapped (note))
        return;

    if (mode == SynthPlayMode::poly)
    {
        yup::Synthesiser::noteOn (channel, note, velocity);
        return;
    }
    auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
    held = { ++noteOrder, velocity, true };
    playMonoNote (channel, note, velocity);
}

void HarmonicSynthEngine::noteOff (int channel, int note, float velocity, bool tailOff)
{
    if (mode == SynthPlayMode::poly)
    {
        yup::Synthesiser::noteOff (channel, note, velocity, tailOff);
        return;
    }
    auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
    held.down = false;
    if (! sustain[static_cast<std::size_t> (channel - 1)] || ! tailOff)
        held.order = 0;
    selectMonoNote (tailOff);
}

void HarmonicSynthEngine::allNotesOff (int channel, bool tailOff)
{
    for (int index = 0; index < static_cast<int> (heldNotes.size()); ++index)
        if (channel <= 0 || index / 128 == channel - 1)
            heldNotes[static_cast<std::size_t> (index)] = {};
    for (int index = 0; index < 16; ++index)
        if (channel <= 0 || index == channel - 1)
            sustain[static_cast<std::size_t> (index)] = false;
    yup::Synthesiser::allNotesOff (channel, tailOff);
    if (mode != SynthPlayMode::poly)
        selectMonoNote (tailOff);
}

void HarmonicSynthEngine::handleChannelPressure (int channel, int channelPressureValue)
{
    context.aftertouch = static_cast<float> (channelPressureValue) / 127.0f;
    yup::Synthesiser::handleChannelPressure (channel, channelPressureValue);
}

void HarmonicSynthEngine::handlePitchWheel (int channel, int wheelValue)
{
    context.pitchBend = static_cast<float> (SynthTuning::normalizePitchWheel (wheelValue));
    yup::Synthesiser::handlePitchWheel (channel, wheelValue);
}

void HarmonicSynthEngine::handleController (int channel, int controller, int value)
{
    // Kept on the engine rather than per voice, so notes started after the wheel moved see it too.
    if (controller == 1)
        context.modWheel = static_cast<float> (value) / 127.0f;

    if (mode != SynthPlayMode::poly && controller == 64)
    {
        sustain[static_cast<std::size_t> (channel - 1)] = value >= 64;
        if (value < 64)
        {
            for (int note = 0; note < 128; ++note)
            {
                auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
                if (! held.down)
                    held.order = 0;
            }
            selectMonoNote (true);
        }
        return;
    }
    yup::Synthesiser::handleController (channel, controller, value);
}

int HarmonicSynthEngine::getCurrentlyPlayingNote() const noexcept
{
    for (int index = 0; index < ownedVoices.size(); ++index)
        if (ownedVoices[index] != nullptr && ownedVoices[index]->isVoiceActive())
            return ownedVoices[index]->getCurrentlyPlayingNote();

    return -1;
}

//==============================================================================
void HarmonicSynthEngine::playMonoNote (int channel, int note, float velocity)
{
    auto* voice = ownedVoices[0].get();
    const auto overlapping = voice->isVoiceActive() && monoKeyActive;
    voice->setTransition (overlapping ? portamentoSeconds : 0.0,
                          overlapping && mode == SynthPlayMode::legato);
    startVoice (voice, getSound (0).get(), channel, note, velocity);
    monoKeyActive = true;
}

void HarmonicSynthEngine::selectMonoNote (bool tailOff)
{
    int latest = -1;
    for (int index = 0; index < static_cast<int> (heldNotes.size()); ++index)
        if (heldNotes[static_cast<std::size_t> (index)].order != 0
            && (latest < 0 || heldNotes[static_cast<std::size_t> (index)].order > heldNotes[static_cast<std::size_t> (latest)].order))
            latest = index;

    auto* voice = ownedVoices[0].get();
    if (latest < 0)
    {
        if (monoKeyActive)
            voice->stopNote (0.0f, tailOff);
        monoKeyActive = false;
        return;
    }
    const auto channel = latest / 128 + 1;
    const auto note = latest % 128;
    if (! voice->isVoiceActive() || voice->getCurrentlyPlayingNote() != note || ! voice->isPlayingChannel (channel))
        playMonoNote (channel, note, heldNotes[static_cast<std::size_t> (latest)].velocity);
}
