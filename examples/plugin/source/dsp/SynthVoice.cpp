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

#include "SynthVoice.h"

SynthVoice::SynthVoice (const SynthParameters& synthParameters,
                        const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials,
                        const SynthOscillatorResources& oscillatorResources,
                        const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots,
                        const SynthBlockContext& blockContext)
    : parameters (synthParameters)
    , partials (oscillatorPartials)
    , resources (oscillatorResources)
    , slots (sharedSlots)
    , context (blockContext)
{
}

void SynthVoice::prepare (double sampleRate, int maxBlockSize, SynthQuality quality, yup::Span<const yup::AudioParameter::Ptr> newProcessorParameters)
{
    processorParameters = newProcessorParameters;
    playbackRate = sampleRate;

    network.prepare (sampleRate, quality, slots, partials, resources);

    for (auto& filter : filters)
        filter.prepare (sampleRate);

    for (auto& envelope : envelopes)
        envelope.prepare (sampleRate);

    amplitude.prepare (sampleRate);

    // A retriggered note ramps from where the amplitude is, never from a jump to zero.
    amplitude.setRetriggerMode (yup::CurveEnvelope<float>::RetriggerMode::legato);

    for (std::size_t index = 0; index < lfos.size(); ++index)
    {
        lfos[index].prepare (sampleRate);
        lfos[index].setCustomCurve (&context.lfoCurves[index]);
    }

    modulatedValues.assign (processorParameters.size(), 0.0f);

    for (auto& bus : buses)
        for (auto& channel : bus)
            channel.assign (static_cast<std::size_t> (SynthExample::controlChunk), 0.0f);

    gains.assign (static_cast<std::size_t> (SynthExample::controlChunk), 0.0f);
    scratch.assign (static_cast<std::size_t> (SynthExample::controlChunk), 0.0f);

    // Filter controls are replayed whole chunks later, the nearest to the network latency.
    delayChunks = yup::jlimit (0, delayedChunks - 1, (network.getLatencyInSamples() + SynthExample::controlChunk / 2) / SynthExample::controlChunk);
    delayedWrite = 0;
    gateOnCountdown = -1;
    gateOffCountdown = -1;
    appliedCurveGeneration = -1;

    hasPlayed = false;
    preserveNote = false;
    glideSeconds = 0.0;
    pitch.setCurrentAndTargetValue (69.0);
    bend.reset (sampleRate, SynthExample::levelRampSeconds);
    bend.setCurrentAndTargetValue (0.0);
    tailLength = yup::jlimit (2, yup::jmax (2, maxBlockSize), static_cast<int> (sampleRate * 0.006));
    tailBuffer.setSize (2, tailLength);
    tailScratch.setSize (2, tailLength);
    tailPosition = tailLength;
    clearCurrentNote();
}

//==============================================================================
bool SynthVoice::canPlaySound (yup::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::setTransition (double seconds, bool legato) noexcept
{
    glideSeconds = yup::jlimit (0.0, 2.0, seconds);
    preserveNote = legato && amplitude.isActive();
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, yup::SynthesiserSound*, int currentPitchWheelPosition)
{
    const auto currentPitch = pitch.getCurrentValue();
    pitch.reset (playbackRate, glideSeconds);
    pitch.setCurrentAndTargetValue (currentPitch);

    if (glideSeconds > 0.0 && hasPlayed)
        pitch.setTargetValue (static_cast<double> (midiNoteNumber));
    else
        pitch.setCurrentAndTargetValue (static_cast<double> (midiNoteNumber));

    pitchWheelMoved (currentPitchWheelPosition);
    currentNote = pitch.getCurrentValue();

    if (! preserveNote)
    {
        noteVelocity = yup::jlimit (0.0f, 1.0f, velocity);
        noteRandom = random.nextFloat() * 2.0f - 1.0f;

        applyCurves();

        for (std::size_t index = 0; index < envelopes.size(); ++index)
        {
            envelopes[index].setTimeScale (context.envelopes[index].timeScale);
            envelopes[index].noteOn();
        }

        envelopeValues = context.envelopes;
        lfoValues = context.lfos;
        configureLFOs();

        for (auto& lfo : lfos)
            lfo.noteOn();

        auto controls = context.network;
        controls.noteFrequency = midiNoteToFrequency (currentNote);
        controls.deriveLocally = context.spectralRouted;
        controls.refreshSpectra = true;
        controls.applyEnables();
        network.startNote (controls);

        for (auto& filter : filters)
            filter.reset();

        // Until the network's first output arrives, the delayed stages play the controls of now.
        DelayedControls initial;
        initial.filters = context.filters;
        initial.routing = context.network.globals.filterRouting;
        initial.ampVelocityGain = context.envelopes[0].getVelocityGain (noteVelocity);
        initial.note = currentNote;
        delayedControls.fill (initial);

        amplitude.setTimeScale (context.envelopes[0].timeScale);
        gateOnCountdown = network.getLatencyInSamples();
        gateOffCountdown = -1;
        samplesSinceSpectra = SynthExample::spectralChunk;
    }

    preserveNote = false;
    hasPlayed = true;
    glideSeconds = 0.0;
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        for (auto& envelope : envelopes)
            envelope.noteOff();

        gateOffCountdown = network.getLatencyInSamples();
        return;
    }

    if (! preserveNote)
    {
        if (amplitude.isActive() || gateOnCountdown >= 0)
        {
            tailScratch.clear();
            renderNextBlock (tailScratch, 0, tailLength);

            for (int channel = 0; channel < 2; ++channel)
            {
                for (int sample = 0; sample < tailLength; ++sample)
                {
                    const auto fade = 0.5 + 0.5 * std::cos (yup::MathConstants<double>::pi * sample / (tailLength - 1));
                    tailBuffer.setSample (channel, sample, tailScratch.getSample (channel, sample) * static_cast<float> (fade));
                }
            }

            tailPosition = 0;
        }

        amplitude.reset();

        for (auto& envelope : envelopes)
            envelope.reset();

        gateOnCountdown = -1;
        gateOffCountdown = -1;
    }

    clearCurrentNote();
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue)
{
    const auto normalized = (static_cast<double> (newPitchWheelValue) - 8192.0) / 8192.0;
    bend.setTargetValue (normalized * pitchWheelRangeSemitones);
}

//==============================================================================
void SynthVoice::renderNextBlock (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! isSounding() || numSamples <= 0 || outputBuffer.getNumChannels() == 0)
        return;

    for (int offset = 0; offset < numSamples;)
    {
        const auto count = yup::jmin (numSamples - offset, SynthExample::controlChunk);
        renderChunk (outputBuffer, startSample + offset, count);
        offset += count;
    }
}

void SynthVoice::renderChunk (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept
{
    auto& outLeft = buses[SynthOperatorNetwork::direct][0];
    auto& outRight = buses[SynthOperatorNetwork::direct][1];
    auto active = isVoiceActive();

    if (active)
    {
        if (context.curveGeneration != appliedCurveGeneration)
            applyCurves();

        currentNote = pitch.skip (numSamples) + bend.skip (numSamples);

        SynthNetworkControls controls = context.network;
        DelayedControls delayed;
        prepareControls (controls, delayed);
        controls.applyEnables();

        controls.noteFrequency = midiNoteToFrequency (currentNote);
        controls.refreshSpectra = samplesSinceSpectra >= SynthExample::spectralChunk;
        samplesSinceSpectra = controls.refreshSpectra ? numSamples : samplesSinceSpectra + numSamples;

        SynthOperatorNetwork::Outputs outputs;

        for (std::size_t bus = 0; bus < buses.size(); ++bus)
            outputs[bus] = { buses[bus][0].data(), buses[bus][1].data() };

        network.render (outputs, numSamples, controls);
        advanceSources (numSamples);

        // The filters and the amplitude play the controls from the network's latency ago.
        delayedControls[static_cast<std::size_t> (delayedWrite)] = delayed;
        const auto& post = delayedControls[static_cast<std::size_t> ((delayedWrite - delayChunks + delayedChunks) % delayedChunks)];
        delayedWrite = (delayedWrite + 1) % delayedChunks;

        auto* filter1Left = buses[SynthOperatorNetwork::filter1][0].data();
        auto* filter1Right = buses[SynthOperatorNetwork::filter1][1].data();
        auto* filter2Left = buses[SynthOperatorNetwork::filter2][0].data();
        auto* filter2Right = buses[SynthOperatorNetwork::filter2][1].data();

        filters[0].process (filter1Left, filter1Right, numSamples, post.filters[0], post.note);

        if (post.routing == SynthFilterRouting::serial)
        {
            yup::FloatVectorOperations::add (filter2Left, filter1Left, numSamples);
            yup::FloatVectorOperations::add (filter2Right, filter1Right, numSamples);
            filters[1].process (filter2Left, filter2Right, numSamples, post.filters[1], post.note);
        }
        else
        {
            filters[1].process (filter2Left, filter2Right, numSamples, post.filters[1], post.note);
            yup::FloatVectorOperations::add (filter2Left, filter1Left, numSamples);
            yup::FloatVectorOperations::add (filter2Right, filter1Right, numSamples);
        }

        yup::FloatVectorOperations::add (outLeft.data(), filter2Left, numSamples);
        yup::FloatVectorOperations::add (outRight.data(), filter2Right, numSamples);

        renderAmplitude (gains.data(), numSamples);
        yup::FloatVectorOperations::multiply (gains.data(), post.ampVelocityGain, numSamples);
        yup::FloatVectorOperations::multiply (outLeft.data(), gains.data(), numSamples);
        yup::FloatVectorOperations::multiply (outRight.data(), gains.data(), numSamples);
    }
    else
    {
        yup::FloatVectorOperations::clear (outLeft.data(), numSamples);
        yup::FloatVectorOperations::clear (outRight.data(), numSamples);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        auto left = outLeft[static_cast<std::size_t> (sample)];
        auto right = outRight[static_cast<std::size_t> (sample)];

        if (tailPosition < tailLength)
        {
            left += tailBuffer.getSample (0, tailPosition);
            right += tailBuffer.getSample (1, tailPosition++);
        }

        if (outputBuffer.getNumChannels() == 1)
        {
            outputBuffer.addSample (0, startSample + sample, (left + right) * 0.5f);
        }
        else
        {
            outputBuffer.addSample (0, startSample + sample, left);
            outputBuffer.addSample (1, startSample + sample, right);
        }
    }

    if (active && ! amplitude.isActive() && gateOnCountdown < 0 && gateOffCountdown < 0)
        clearCurrentNote();
}

//==============================================================================
void SynthVoice::prepareControls (SynthNetworkControls& controls, DelayedControls& delayed) noexcept
{
    controls.deriveLocally = context.spectralRouted;

    delayed.filters = context.filters;
    delayed.note = currentNote;
    routeAmounts.fill (0.0f);

    if (context.numRoutes == 0 || modulatedValues.empty())
    {
        envelopeValues = context.envelopes;
        lfoValues = context.lfos;
    }
    else
    {
        std::copy (context.played.begin(), context.played.end(), modulatedValues.begin());

        // Sources are read before anything is modulated, at the top of the chunk, with the
        // envelope and LFO settings the previous chunk's routes left: a route into an
        // LFO's depth or an envelope's velocity acts one chunk later.
        for (int index = 0; index < context.numRoutes; ++index)
        {
            const auto& route = context.routes[static_cast<std::size_t> (index)];
            const auto amount = context.depths[static_cast<std::size_t> (index)] * getSourceValue (route.source);
            routeAmounts[static_cast<std::size_t> (context.slots[static_cast<std::size_t> (index)])] = amount;

            if (amount == 0.0f || ! yup::isPositiveAndBelow (route.destination, static_cast<int> (modulatedValues.size())))
                continue;

            const auto destination = static_cast<std::size_t> (route.destination);
            const auto& parameter = processorParameters[destination];
            const auto normalized = yup::jlimit (0.0f, 1.0f, parameter->convertToNormalizedValue (modulatedValues[destination]) + amount);

            modulatedValues[destination] = parameter->convertToDenormalizedValue (normalized);
        }

        const SynthParameterReader reader { yup::Span<const float> (modulatedValues.data(), modulatedValues.size()) };

        for (std::size_t index = 0; index < controls.oscillators.size(); ++index)
        {
            auto values = parameters.oscillators[index].read (reader);
            const auto& shared = context.network.oscillators[index];

            values.harmonicScale = shared.harmonicScale;
            values.usesCustomSeries = shared.usesCustomSeries;
            values.harmonicGeneration = shared.harmonicGeneration;
            controls.oscillators[index] = values;
        }

        controls.fm = parameters.fm.read (reader);
        controls.globals = parameters.readGlobals (reader);

        for (std::size_t index = 0; index < delayed.filters.size(); ++index)
            delayed.filters[index] = parameters.filters[index].read (reader);

        for (std::size_t index = 0; index < envelopeValues.size(); ++index)
            envelopeValues[index] = parameters.envelopes[index].read (reader);

        for (std::size_t index = 0; index < lfoValues.size(); ++index)
            lfoValues[index] = parameters.lfos[index].read (reader);
    }

    delayed.routing = controls.globals.filterRouting;
    delayed.ampVelocityGain = envelopeValues[0].getVelocityGain (noteVelocity);
}

float SynthVoice::getEnvelopePosition (int envelopeIndex) const noexcept
{
    if (envelopeIndex == 0)
        return amplitude.isActive() ? amplitude.getPointPosition() : -1.0f;

    const auto index = static_cast<std::size_t> (envelopeIndex);

    if (! yup::isPositiveAndBelow (envelopeIndex, SynthExample::envelopeCount) || ! context.envelopeRouted[index]
        || ! envelopeValues[index].enabled || ! envelopes[index].isActive())
        return -1.0f;

    return envelopes[index].getPointPosition();
}

float SynthVoice::getSourceValue (SynthModulationSource source) const noexcept
{
    if (const auto envelope = getSourceEnvelope (source); envelope >= 0)
    {
        const auto& values = envelopeValues[static_cast<std::size_t> (envelope)];

        if (! values.enabled)
            return 0.0f;

        return envelopes[static_cast<std::size_t> (envelope)].getLevel() * values.depth * values.getVelocityGain (noteVelocity);
    }

    if (const auto lfo = getSourceLFO (source); lfo >= 0)
    {
        const auto& values = lfoValues[static_cast<std::size_t> (lfo)];
        return values.enabled ? lfos[static_cast<std::size_t> (lfo)].getValue() * values.depth : 0.0f;
    }

    switch (source)
    {
        case SynthModulationSource::velocity: return noteVelocity;
        case SynthModulationSource::note: return yup::jlimit (-1.0f, 1.0f, static_cast<float> ((currentNote - 60.0) / 60.0));
        case SynthModulationSource::modWheel: return context.modWheel;
        case SynthModulationSource::aftertouch: return context.aftertouch;
        case SynthModulationSource::random: return noteRandom;
        default: break;
    }

    return 0.0f;
}

void SynthVoice::advanceSources (int numSamples) noexcept
{
    // Envelopes and LFOs nothing reads, or that are disabled, are not advanced; they still follow the notes.
    for (std::size_t index = 0; index < envelopes.size(); ++index)
    {
        envelopes[index].setTimeScale (envelopeValues[index].timeScale);

        if (context.envelopeRouted[index] && envelopeValues[index].enabled)
            envelopes[index].processBlock (scratch.data(), numSamples);
    }

    amplitude.setTimeScale (envelopeValues[0].timeScale);

    configureLFOs();

    for (std::size_t index = 0; index < lfos.size(); ++index)
        if (context.lfoRouted[index] && lfoValues[index].enabled)
            lfos[index].skip (numSamples);
}

void SynthVoice::configureLFOs() noexcept
{
    for (std::size_t index = 0; index < lfos.size(); ++index)
    {
        const auto& values = lfoValues[index];
        auto& lfo = lfos[index];

        lfo.setShape (values.shape);
        lfo.setFrequency (values.getFrequency (context.bpm));
        lfo.setPhaseOffset (values.phase);
        lfo.setSmoothing (values.smoothing);
        lfo.setRetriggerMode (values.retrigger);
    }
}

void SynthVoice::applyCurves() noexcept
{
    for (std::size_t index = 0; index < envelopes.size(); ++index)
        envelopes[index].setCurve (context.envelopeCurves[index]);

    amplitude.setCurve (context.envelopeCurves[0]);
    appliedCurveGeneration = context.curveGeneration;
}

void SynthVoice::renderAmplitude (float* output, int numSamples) noexcept
{
    int offset = 0;

    while (offset < numSamples)
    {
        auto next = numSamples;

        if (gateOnCountdown >= 0)
            next = yup::jmin (next, offset + gateOnCountdown);

        if (gateOffCountdown >= 0)
            next = yup::jmin (next, offset + gateOffCountdown);

        const auto count = next - offset;

        if (count > 0)
            amplitude.processBlock (output + offset, count);

        if (gateOnCountdown >= 0)
            gateOnCountdown -= count;

        if (gateOffCountdown >= 0)
            gateOffCountdown -= count;

        offset = next;

        if (gateOnCountdown == 0)
        {
            amplitude.noteOn();
            gateOnCountdown = -1;
        }

        if (gateOffCountdown == 0 && gateOnCountdown < 0)
        {
            amplitude.noteOff();
            gateOffCountdown = -1;
        }
    }
}
