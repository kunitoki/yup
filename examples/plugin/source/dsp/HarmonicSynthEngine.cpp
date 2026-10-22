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
                                          const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials)
    : parameters (synthParameters)
    , partials (oscillatorPartials)
{
    addSound (new SynthSound());

    for (int index = 0; index < SynthExample::voiceCount; ++index)
    {
        auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (partials, resources, oscillatorSlots, context));

        addVoice (voice);
        ownedVoices.add (voice);
    }
}

void HarmonicSynthEngine::prepare (double sampleRate, int maxBlockSize)
{
    allNotesOff (0, false);
    setCurrentPlaybackSampleRate (sampleRate);
    activeVoices.store (0);

    for (int index = 0; index < ownedVoices.size(); ++index)
        ownedVoices[index]->prepare (sampleRate, maxBlockSize);
}

void HarmonicSynthEngine::renderRange (yup::AudioBuffer<float>& output,
                                       const yup::MidiBuffer& midi,
                                       int startSample,
                                       int numSamples,
                                       yup::Span<const float> parameterValues)
{
    const auto requestedMode = static_cast<SynthPlayMode> (yup::jlimit (0, 2, yup::roundToInt (parameterValues[static_cast<std::size_t> (parameters.playMode->getIndexInContainer())])));
    const auto release = releaseRequested.exchange (false);
    if (requestedMode != mode || release)
    {
        allNotesOff (0, true);
        mode = requestedMode;
    }

    updateContext (parameterValues);

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

void HarmonicSynthEngine::updateContext (yup::Span<const float> parameterValues) noexcept
{
    const SynthParameterReader reader { parameterValues };

    portamentoSeconds = static_cast<double> (reader (parameters.glide)) * 0.001;

    for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
    {
        auto& values = context.patch.oscillators[slot];
        values = parameters.oscillators[slot].read (reader);
        partials[slot].fillValues (values);
    }

    context.patch.filter = parameters.filter.read (reader);
    context.modulation = parameters.modulation.read (reader);

    for (std::size_t index = 0; index < context.envelopes.size(); ++index)
        context.envelopes[index] = parameters.envelopes[index].read (reader);

    for (std::size_t index = 0; index < context.lfos.size(); ++index)
        context.lfos[index] = parameters.lfos[index].read (reader);

    // Every voice of a slot plays the same spectrum unless a modulation is routed
    // into it, so it is derived once here rather than once per voice.
    for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
        oscillatorSlots[slot].update (context.patch.oscillators[slot], partials[slot], resources);
}

void HarmonicSynthEngine::publishVoiceState() noexcept
{
    int active = 0;
    const SynthVoice* newest = nullptr;

    for (auto* voice : ownedVoices)
    {
        active += voice->isSounding() ? 1 : 0;

        if (voice->isVoiceActive() && (newest == nullptr || newest->wasStartedBefore (*voice)))
            newest = voice;
    }

    activeVoices.store (active);

    // The displays follow the LFOs of the most recent note.
    if (newest != nullptr)
        for (int index = 0; index < SynthExample::lfoCount; ++index)
            lfoPhases[static_cast<std::size_t> (index)].store (newest->getLFOPhase (index));
}

//==============================================================================
void HarmonicSynthEngine::noteOn (int channel, int note, float velocity)
{
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
