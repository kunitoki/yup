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

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

//==============================================================================
/**
    A tine electric piano playing one chord per key of the Keys demo.

    Each note is two operator FM: a sine carrier phase modulated by a sine at the
    same pitch, bright on the attack and mellow as the modulation decays, plus a
    short bell overtone for the tine, added rather than modulating so it cannot
    spray sidebands. The chords are strummed a little,
    ring out over each other and are released softly; a slow autopan gives the
    classic stage piano wobble.

    playChord() and releaseChord() are called on the message thread; everything
    else runs on the audio thread.
*/
class KeysSound : public SoundSource
{
public:
    //==============================================================================
    /** Starts the chord of a key, counted in semitones from the lowest C of the keyboard. */
    void playChord (int key)
    {
        if (yup::isPositiveAndBelow (key, 32))
            pendingOn.fetch_or (1u << key, std::memory_order_release);
    }

    /** Lets the chord of a key ring out. */
    void releaseChord (int key)
    {
        if (yup::isPositiveAndBelow (key, 32))
            pendingOff.fetch_or (1u << key, std::memory_order_release);
    }

    //==============================================================================
    void prepare (double newSampleRate) override
    {
        sampleRate = static_cast<float> (newSampleRate);
        voices = {};
        autopan.prepare (newSampleRate);
        autopan.setFrequency (autopanHz);
        clock = 0;
        sineOfCycles (0.0f); // builds the table here rather than in the middle of a note
    }

    void render (float* left, float* right, int numSamples) override
    {
        const auto off = pendingOff.exchange (0u, std::memory_order_acquire);
        const auto on = pendingOn.exchange (0u, std::memory_order_acquire);

        for (int key = 0; key < 32; ++key)
        {
            if ((off & (1u << key)) != 0)
                release (key);

            if ((on & (1u << key)) != 0)
                startChord (key);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            auto l = 0.0f, r = 0.0f;

            for (auto& voice : voices)
            {
                if (! voice.active)
                    continue;

                const auto sample = voice.next();
                l += sample * voice.leftGain;
                r += sample * voice.rightGain;
            }

            // Autopan: the sound swings gently from side to side.
            const auto swing = autopanDepth * autopan.processSample();

            left[i] += l * (1.0f - swing);
            right[i] += r * (1.0f + swing);
        }
    }

private:
    //==============================================================================
    static constexpr int maxVoices = 40;
    static constexpr int stealAbove = 28; // spare voices for the next chords while old ones fade
    static constexpr float autopanHz = 3.6f;
    static constexpr float autopanDepth = 0.3f;
    static constexpr float strumSeconds = 0.012f;

    //==============================================================================
    struct Voice
    {
        float next()
        {
            if (delay > 0)
            {
                --delay;
                return 0.0f;
            }

            const auto modulation = index * sineOfCycles (modPhase);
            const auto body = sineOfCycles (phase + modulation * (1.0f / yup::MathConstants<float>::twoPi));
            const auto out = (body + tineLevel * sineOfCycles (tinePhase)) * envelope * attack;

            phase += increment;
            phase -= std::floor (phase);
            modPhase += increment;
            modPhase -= std::floor (modPhase);
            tinePhase += tineIncrement;
            tinePhase -= std::floor (tinePhase);

            attack = std::min (1.0f, attack + attackStep);
            envelope *= stealing ? stealCoefficient : released ? releaseCoefficient : decayCoefficient;
            index = indexFloor + (index - indexFloor) * indexCoefficient;
            tineLevel *= tineCoefficient;

            if (envelope < 1.0e-4f)
                active = false;

            return out;
        }

        bool active = false, released = false, stealing = false;
        int key = -1, delay = 0;
        std::uint32_t startedAt = 0;
        float phase = 0.0f, modPhase = 0.0f, tinePhase = 0.0f;
        float increment = 0.0f, tineIncrement = 0.0f;
        float envelope = 0.0f, attack = 0.0f, attackStep = 0.0f;
        float decayCoefficient = 1.0f, releaseCoefficient = 1.0f, stealCoefficient = 1.0f;
        float index = 0.0f, indexFloor = 0.0f, indexCoefficient = 1.0f;
        float tineLevel = 0.0f, tineCoefficient = 1.0f;
        float leftGain = 0.0f, rightGain = 0.0f;
    };

    //==============================================================================
    /** The chord of each key in C major: maj9 on the major degrees, m9 or m7 on the minor ones,
        9 on the dominant, half diminished on the leading tone; the black keys borrow the colors
        of the parallel minor. Intervals above the root, which plays in the bass. */
    static std::array<int, 3> chordIntervals (int pitchClass)
    {
        static constexpr std::array<std::array<int, 3>, 12> chords { {
            { 4, 11, 14 }, // C maj9
            { 4, 11, 14 }, // Db maj9
            { 3, 10, 14 }, // D m9
            { 4, 11, 14 }, // Eb maj9
            { 3, 7, 10 },  // E m7
            { 4, 11, 14 }, // F maj9
            { 3, 6, 10 },  // F# m7b5
            { 4, 10, 14 }, // G 9
            { 4, 11, 14 }, // Ab maj9
            { 3, 10, 14 }, // A m9
            { 4, 10, 14 }, // Bb 9
            { 3, 6, 10 },  // B m7b5
        } };

        return chords[(std::size_t) pitchClass];
    }

    void startChord (int key)
    {
        const auto pitchClass = key % 12;
        const auto octave = key / 12;
        const auto root = 36 + pitchClass + 12 * octave; // the lowest C of the keyboard plays C2

        makeRoom();
        startNote (key, root, 0.7f, 0);

        // The upper voices stay in one compact octave over middle C, an octave higher for the
        // upper keys, so neighbouring chords move little and blend.
        int step = 1;
        for (const auto interval : chordIntervals (pitchClass))
        {
            auto note = root + interval;
            while (note < 55 + 12 * octave)
                note += 12;
            while (note >= 70 + 12 * octave)
                note -= 12;

            startNote (key, note, 0.85f, step++);
        }
    }

    void startNote (int key, int midiNote, float velocity, int strumStep)
    {
        auto* voice = freeVoice();

        const auto frequency = 440.0f * std::exp2 ((static_cast<float> (midiNote) - 69.0f) / 12.0f);
        const auto belowMiddle = 261.63f / frequency;
        const auto perSample = [this] (float seconds) { return std::exp (-1.0f / (seconds * sampleRate)); };

        *voice = {};
        voice->active = true;
        voice->key = key;
        voice->startedAt = ++clock;
        voice->delay = static_cast<int> (strumSeconds * static_cast<float> (strumStep) * sampleRate);
        voice->increment = frequency / sampleRate;
        voice->tineIncrement = frequency * 7.0f < 0.4f * sampleRate ? frequency * 7.0f / sampleRate : 0.0f;

        voice->envelope = 0.1f * velocity * std::pow (belowMiddle, 0.15f);
        voice->attackStep = 1.0f / (0.002f * sampleRate);
        voice->decayCoefficient = perSample (std::clamp (2.2f * std::pow (belowMiddle, 0.4f), 0.6f, 5.0f));
        voice->releaseCoefficient = perSample (0.35f);
        voice->stealCoefficient = perSample (0.015f);

        voice->index = 1.6f * velocity;
        voice->indexFloor = 0.35f * velocity;
        voice->indexCoefficient = perSample (0.35f);
        voice->tineLevel = voice->tineIncrement > 0.0f ? 0.12f * velocity : 0.0f;
        voice->tineCoefficient = perSample (0.07f);

        // Low notes a little to the left, high ones to the right.
        const auto pan = 0.5f + std::clamp ((static_cast<float> (midiNote) - 60.0f) / 48.0f, -0.25f, 0.25f);
        voice->leftGain = std::cos (pan * yup::MathConstants<float>::halfPi);
        voice->rightGain = std::sin (pan * yup::MathConstants<float>::halfPi);
    }

    void release (int key)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.key == key)
                voice.released = true;
        }
    }

    /** Fades out the oldest voices quickly while too many ring, so the next chords find free ones. */
    void makeRoom()
    {
        for (;;)
        {
            int ringing = 0;
            Voice* oldest = nullptr;

            for (auto& voice : voices)
            {
                if (! voice.active || voice.stealing)
                    continue;

                ++ringing;
                if (oldest == nullptr || voice.startedAt < oldest->startedAt)
                    oldest = &voice;
            }

            if (ringing <= stealAbove || oldest == nullptr)
                return;

            oldest->stealing = true;
        }
    }

    Voice* freeVoice()
    {
        Voice* oldest = &voices[0];

        for (auto& voice : voices)
        {
            if (! voice.active)
                return &voice;

            if (voice.startedAt < oldest->startedAt)
                oldest = &voice;
        }

        return oldest;
    }

    //==============================================================================
    std::atomic<std::uint32_t> pendingOn { 0 }, pendingOff { 0 };

    // Audio thread only
    std::array<Voice, maxVoices> voices {};
    float sampleRate = 44100.0f;
    yup::LFO<float> autopan;
    std::uint32_t clock = 0;
};
