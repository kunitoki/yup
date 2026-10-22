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

SynthVoice::SynthVoice (const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials,
                        const SynthOscillatorResources& oscillatorResources,
                        const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots,
                        const SynthBlockContext& blockContext)
    : partials (oscillatorPartials)
    , resources (oscillatorResources)
    , slots (sharedSlots)
    , context (blockContext)
{
}

void SynthVoice::prepare (double sampleRate, int maxBlockSize)
{
    for (std::size_t slot = 0; slot < oscillators.size(); ++slot)
        oscillators[slot].prepare (sampleRate, maxBlockSize, slots[slot], partials[slot], resources);

    for (auto& level : levels)
        level.reset (sampleRate, SynthExample::levelRampSeconds);

    filter.prepare (sampleRate);
    envelope.prepare (sampleRate);
    modulationEnvelope.prepare (sampleRate);

    for (auto& lfo : lfos)
        lfo.prepare (sampleRate);
    playbackRate = sampleRate;
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

    const auto blockSize = static_cast<std::size_t> (yup::jmax (1, maxBlockSize));

    oscLeft.assign (blockSize, 0.0f);
    oscRight.assign (blockSize, 0.0f);
    mixLeft.assign (blockSize, 0.0f);
    mixRight.assign (blockSize, 0.0f);
}

//==============================================================================
bool SynthVoice::canPlaySound (yup::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::setTransition (double seconds, bool legato) noexcept
{
    glideSeconds = yup::jlimit (0.0, 2.0, seconds);
    preserveNote = legato && envelope.isActive();
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

    if (! preserveNote)
    {
        velocityGain = yup::jlimit (0.0f, 1.0f, velocity);
        for (auto& oscillator : oscillators)
            oscillator.reset (0.0);

        envelope.setParameters (context.envelopes[0]);
        envelope.noteOn();
        modulationEnvelope.setParameters (context.envelopes[1]);
        modulationEnvelope.noteOn();
        filter.reset();

        for (std::size_t index = 0; index < lfos.size(); ++index)
            if (context.lfos[index].retrigger)
                lfos[index].reset();
    }

    preserveNote = false;
    hasPlayed = true;
    glideSeconds = 0.0;
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        envelope.noteOff();
        modulationEnvelope.noteOff();
        return;
    }

    if (! preserveNote)
    {
        if (envelope.isActive())
        {
            tailScratch.clear();
            renderNextBlock (tailScratch, 0, tailLength);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < tailLength; ++sample)
                {
                    const auto fade = 0.5 + 0.5 * std::cos (yup::MathConstants<double>::pi
                                                          * sample / (tailLength - 1));
                    tailBuffer.setSample (channel, sample, tailScratch.getSample (channel, sample) * static_cast<float> (fade));
                }
            tailPosition = 0;
        }
        envelope.noteOffImmediate();
        modulationEnvelope.noteOffImmediate();
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

    envelope.setParameters (context.envelopes[0]);
    modulationEnvelope.setParameters (context.envelopes[1]);

    for (int offset = 0; offset < numSamples;)
    {
        const auto count = yup::jmin (numSamples - offset, static_cast<int> (mixLeft.size()), SynthExample::controlChunk);
        if (count <= 0)
            return;
        renderChunk (outputBuffer, startSample + offset, count);
        offset += count;
    }
}

void SynthVoice::renderChunk (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept
{
    yup::FloatVectorOperations::clear (mixLeft.data(), numSamples);
    yup::FloatVectorOperations::clear (mixRight.data(), numSamples);

    if (isVoiceActive())
    {
        // The envelopes and LFOs are per voice, so the routings are applied here, once per
        // control chunk, from the values each source holds at the top of the chunk.
        for (std::size_t index = 0; index < lfos.size(); ++index)
        {
            lfos[index].setShape (context.lfos[index].shape);
            lfos[index].setFrequency (context.lfos[index].rate);
            lfos[index].setPhaseOffset (context.lfos[index].phase);
        }

        auto patch = context.patch;
        const std::array<float, 5> sources { envelope.getLevel(), modulationEnvelope.getLevel(), lfos[0].getValue(), lfos[1].getValue(), context.modWheel };
        applyModulation (patch, context.modulation, sources);

        for (auto& lfo : lfos)
            lfo.skip (numSamples);

        const auto note = pitch.skip (numSamples) + bend.skip (numSamples);
        const auto frequency = midiNoteToFrequency (note);
        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            const auto slot = static_cast<std::size_t> (index);
            const auto& values = patch.oscillators[slot];
            auto& level = levels[slot];
            const auto detuned = frequency * std::exp2 (values.octave + values.detuneSemitones / 12.0);
            oscillators[slot].renderBlock (oscLeft.data(), oscRight.data(), numSamples, values, detuned,
                                           context.modulation.hasSpectrumRoute (index));
            level.setTargetValue (values.level);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto gain = level.getNextValue();
                mixLeft[static_cast<std::size_t> (sample)] += oscLeft[static_cast<std::size_t> (sample)] * gain;
                mixRight[static_cast<std::size_t> (sample)] += oscRight[static_cast<std::size_t> (sample)] * gain;
            }
        }

        filter.process (mixLeft.data(), mixRight.data(), numSamples, patch.filter, note);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto gain = envelope.getNextValue() * velocityGain;
        modulationEnvelope.getNextValue();
        auto left = mixLeft[static_cast<std::size_t> (sample)] * gain;
        auto right = mixRight[static_cast<std::size_t> (sample)] * gain;
        if (tailPosition < tailLength)
        {
            left += tailBuffer.getSample (0, tailPosition);
            right += tailBuffer.getSample (1, tailPosition++);
        }

        if (outputBuffer.getNumChannels() == 1)
            outputBuffer.addSample (0, startSample + sample, (left + right) * 0.5f);
        else
        {
            outputBuffer.addSample (0, startSample + sample, left);
            outputBuffer.addSample (1, startSample + sample, right);
        }
    }

    if (! envelope.isActive())
        clearCurrentNote();
}
