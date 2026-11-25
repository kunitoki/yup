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

#include <yup_audio_basics/yup_audio_basics.h>

#include <array>
#include <cmath>

//==============================================================================
/** The oscillator shapes the instrument can play. Keep the order stable: the
    values are the automation values of the "waveform" parameter.
*/
enum class ExampleWaveform
{
    sine = 0,
    saw = 1,
    square = 2
};

//==============================================================================
/** One sounding note: an oscillator plus a linear attack/release envelope.

    A voice is silent until noteOn() starts it and keeps sounding, releasing, until
    its envelope reaches zero. renderSample() advances it by a single frame.
*/
class ExampleVoice
{
public:
    ExampleVoice() = default;

    /** Prepares the voice for a sample rate, clearing any sound. */
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        reset();
    }

    /** Silences the voice immediately. */
    void reset() noexcept
    {
        stage = Stage::idle;
        phase = 0.0;
        level = 0.0f;
        noteNumber = -1;
    }

    /** Returns true when the voice is producing (or finishing) sound. */
    bool isActive() const noexcept { return stage != Stage::idle; }

    /** Returns the MIDI note the voice is playing, or -1 when idle. */
    int getNoteNumber() const noexcept { return noteNumber; }

    /** Starts a note, entering the attack stage. */
    void noteOn (int midiNoteNumber, float velocity) noexcept
    {
        noteNumber = midiNoteNumber;
        velocityGain = velocity;
        phase = 0.0;
        phaseIncrement = yup::MidiMessage::getMidiNoteInHertz (midiNoteNumber) / sampleRate;
        stage = Stage::attack;
    }

    /** Releases the note, entering the release stage. */
    void noteOff() noexcept
    {
        if (stage != Stage::idle)
            stage = Stage::release;
    }

    /** Sets the attack and release times, in seconds. */
    void setEnvelope (float attackSeconds, float releaseSeconds) noexcept
    {
        attackRate = rateForTime (attackSeconds);
        releaseRate = rateForTime (releaseSeconds);
    }

    /** Selects the oscillator shape. */
    void setWaveform (ExampleWaveform newWaveform) noexcept { waveform = newWaveform; }

    /** Advances the envelope and returns the next mono sample. */
    float renderSample() noexcept
    {
        if (stage == Stage::idle)
            return 0.0f;

        if (stage == Stage::attack)
        {
            level += attackRate;

            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::sustain;
            }
        }
        else if (stage == Stage::release)
        {
            level -= releaseRate;

            if (level <= 0.0f)
            {
                reset();
                return 0.0f;
            }
        }

        const auto sample = oscillator();

        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;

        return sample * level * velocityGain;
    }

private:
    enum class Stage
    {
        idle,
        attack,
        sustain,
        release
    };

    float oscillator() const noexcept
    {
        switch (waveform)
        {
            case ExampleWaveform::saw:
                return static_cast<float> (2.0 * phase - 1.0);

            case ExampleWaveform::square:
                return phase < 0.5 ? 1.0f : -1.0f;

            case ExampleWaveform::sine:
            default:
                return static_cast<float> (std::sin (phase * yup::MathConstants<double>::twoPi));
        }
    }

    float rateForTime (float timeSeconds) const noexcept
    {
        const auto samples = static_cast<double> (timeSeconds) * sampleRate;
        return samples > 1.0 ? static_cast<float> (1.0 / samples) : 1.0f;
    }

    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float level = 0.0f;
    float velocityGain = 1.0f;
    float attackRate = 0.01f;
    float releaseRate = 0.001f;
    int noteNumber = -1;
    Stage stage = Stage::idle;
    ExampleWaveform waveform = ExampleWaveform::sine;
};

//==============================================================================
/** A fixed pool of voices that turns MIDI notes into audio.

    The synth owns the polyphony: it picks a free voice for each note on, stealing
    one in turn when all are busy, releases the matching voices on note off, and
    sums every voice into the output.
*/
class ExampleSynth
{
public:
    /** The number of simultaneous voices. */
    static constexpr int numVoices = 16;

    ExampleSynth() = default;

    /** Prepares every voice for a sample rate. */
    void prepare (double sampleRate) noexcept
    {
        for (auto& voice : voices)
            voice.prepare (sampleRate);
    }

    /** Silences every voice. */
    void reset() noexcept
    {
        for (auto& voice : voices)
            voice.reset();
    }

    /** Applies the attack and release times, in seconds, to every voice. */
    void setEnvelope (float attackSeconds, float releaseSeconds) noexcept
    {
        for (auto& voice : voices)
            voice.setEnvelope (attackSeconds, releaseSeconds);
    }

    /** Selects the oscillator shape for every voice. */
    void setWaveform (ExampleWaveform waveform) noexcept
    {
        for (auto& voice : voices)
            voice.setWaveform (waveform);
    }

    /** Starts a note on a free voice, stealing one in turn when all are busy. */
    void noteOn (int midiNoteNumber, float velocity) noexcept
    {
        for (auto& voice : voices)
        {
            if (! voice.isActive())
            {
                voice.noteOn (midiNoteNumber, velocity);
                return;
            }
        }

        voices[stealIndex].noteOn (midiNoteNumber, velocity);
        stealIndex = (stealIndex + 1) % voices.size();
    }

    /** Releases every voice currently playing a note. */
    void noteOff (int midiNoteNumber) noexcept
    {
        for (auto& voice : voices)
            if (voice.isActive() && voice.getNoteNumber() == midiNoteNumber)
                voice.noteOff();
    }

    /** Releases every sounding voice. */
    void allNotesOff() noexcept
    {
        for (auto& voice : voices)
            voice.noteOff();
    }

    /** Returns the summed output of every voice for the next frame. */
    float renderSample() noexcept
    {
        float sample = 0.0f;
        for (auto& voice : voices)
            sample += voice.renderSample();

        return sample;
    }

private:
    std::array<ExampleVoice, numVoices> voices;
    std::size_t stealIndex = 0;
};
