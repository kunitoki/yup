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
#include <cmath>
#include <utility>

//==============================================================================
/**
    A slot machine reel of presets that play, for the Reel demo.

    Every row passing by ticks like a ratchet, higher and brighter the faster the
    reel spins. Selecting a
    preset plays a four note phrase on its sound: six presets from one small voice
    (FM or detuned saws through a resonant low pass), the number of the preset
    moving the phrase to a nearby key. Under it all a low pad holds the key of the
    selected preset, gliding to each new one, so the phrases sit on one ground.

    tick() and playPreset() are called on the message thread; everything
    else runs on the audio thread.
*/
class ReelSound : public SoundSource
{
public:
    //==============================================================================
    static constexpr int numPresetKinds = 6;

    /** A row went by, at a speed in rows per second. */
    void tick (float rowsPerSecond) { post ({ Event::tick, rowsPerSecond, 0 }); }

    /** Plays the phrase of a preset row, fading out the one playing. */
    void playPreset (int row) { post ({ Event::preset, 0.0f, row }); }

    //==============================================================================
    void prepare (double newSampleRate) override
    {
        sampleRate = static_cast<float> (newSampleRate);

        for (auto& voice : voices)
            voice.reset();

        for (auto& click : clicks)
            click.level = 0.0f;

        phraseNote = phraseLength;
        pad.prepare (sampleRate, phraseKey);

        // Events posted while the demo was closed are stale.
        int start1, size1, start2, size2;
        events.prepareToRead (maxEvents, start1, size1, start2, size2);
        events.finishedRead (size1 + size2);

        sineOfCycles (0.0f); // builds the table here rather than in the middle of a note
    }

    void render (float* left, float* right, int numSamples) override
    {
        handleEvents();

        for (int i = 0; i < numSamples; ++i)
        {
            if (phraseNote < phraseLength && --samplesToNextNote <= 0.0f)
                playPhraseNote();

            auto l = 0.0f, r = 0.0f;

            for (auto& voice : voices)
            {
                const auto sample = voice.next();
                l += sample * voice.leftGain;
                r += sample * voice.rightGain;
            }

            auto mechanics = 0.0f;
            for (auto& click : clicks)
                mechanics += click.next();

            const auto [padLeft, padRight] = pad.next();

            left[i] += l + mechanics + padLeft;
            right[i] += r + mechanics + padRight;
        }
    }

private:
    //==============================================================================
    struct Event
    {
        enum Kind
        {
            tick,
            preset
        };

        Kind kind = tick;
        float value = 0.0f;
        int row = 0;
    };

    static constexpr int maxEvents = 128;
    static constexpr int phraseLength = 4;
    static constexpr float phraseStepSeconds = 0.22f;

    void post (const Event& event)
    {
        int start1, size1, start2, size2;
        events.prepareToWrite (1, start1, size1, start2, size2);
        if (size1 + size2 == 0)
            return;

        pendingEvents[(std::size_t) (size1 > 0 ? start1 : start2)] = event;
        events.finishedWrite (1);
    }

    void handleEvents()
    {
        int start1, size1, start2, size2;
        events.prepareToRead (maxEvents, start1, size1, start2, size2);

        for (int i = 0; i < size1 + size2; ++i)
        {
            const auto& event = pendingEvents[(std::size_t) (i < size1 ? start1 + i : start2 + i - size1)];

            if (event.kind == Event::tick)
                startClick (event.value);
            else
                startPhrase (event.row);
        }

        events.finishedRead (size1 + size2);
    }

    //==============================================================================
    /** A ratchet click: a short decaying ping, pitched and brightened by the spin speed. */
    struct Click
    {
        float next()
        {
            if (level < 1.0e-5f)
                return 0.0f;

            const auto out = sineOfCycles (phase) * level;
            phase += increment;
            phase -= std::floor (phase);
            level *= decay;
            return out;
        }

        float phase = 0.0f, increment = 0.0f, level = 0.0f, decay = 0.0f;
    };

    void startClick (float rowsPerSecond)
    {
        const auto speed = std::clamp (rowsPerSecond / 40.0f, 0.0f, 1.0f);

        auto& click = clicks[(std::size_t) nextClick];
        nextClick = (nextClick + 1) % static_cast<int> (clicks.size());

        click.phase = 0.0f;
        click.increment = (1400.0f + 1800.0f * speed) / sampleRate;
        click.level = 0.035f + 0.03f * speed;
        click.decay = std::exp (-1.0f / (0.006f * sampleRate));
    }

    //==============================================================================
    /** A low, soft pad gluing the phrases together: root, fifth and octave of the key of the selected
        preset, on detuned saws through a slowly moving low pass, gliding to each new key. */
    struct Pad
    {
        static constexpr std::array<int, 3> intervals { 0, 7, 12 };

        void prepare (float newSampleRate, int key)
        {
            sampleRate = newSampleRate;
            setKey (key);
            root = targetRoot;

            for (std::size_t k = 0; k < phases.size(); ++k)
                phases[k] = std::fmod (0.381966f * static_cast<float> (k), 1.0f);

            for (auto& filter : filters)
                filter.reset();

            breath.prepare (sampleRate);
            breath.setFrequency (0.06f);
        }

        /** Moves to the key of a phrase, folded into the octave from E2. */
        void setKey (int key)
        {
            targetRoot = static_cast<float> (40 + ((key - 40) % 12 + 12) % 12);
        }

        std::pair<float, float> next()
        {
            root += (targetRoot - root) * (1.0f - std::exp (-1.0f / (0.4f * sampleRate)));

            std::array<float, 2> sides {};
            for (std::size_t n = 0; n < intervals.size(); ++n)
            {
                const auto frequency = 440.0f * std::exp2 ((root + static_cast<float> (intervals[n]) - 69.0f) / 12.0f);

                for (std::size_t side = 0; side < sides.size(); ++side)
                {
                    auto& phase = phases[n * 2 + side];
                    const auto increment = frequency * (side == 0 ? 0.997f : 1.003f) / sampleRate;

                    phase += increment;
                    phase -= std::floor (phase);
                    sides[side] += (n == 0 ? 1.0f : 0.7f) * polyBlepSaw (phase, increment);
                }
            }

            const auto cutoff = 520.0f * (1.0f + 0.25f * breath.processSample());

            for (std::size_t side = 0; side < sides.size(); ++side)
            {
                filters[side].setParameters (yup::FilterMode::lowpass, cutoff, 0.7, 0.0, sampleRate);
                sides[side] = level * filters[side].processSample (sides[side]);
            }

            return { sides[0], sides[1] };
        }

        static constexpr float level = 0.045f;

        std::array<float, intervals.size() * 2> phases {};
        std::array<yup::VAStateVariableFilter<float>, 2> filters;
        yup::LFO<float> breath;
        float sampleRate = 44100.0f;
        float root = 45.0f, targetRoot = 45.0f;
    };

    //==============================================================================
    /** What makes one preset kind sound like its name. */
    struct Preset
    {
        bool fm;                // FM pair, or two detuned saws
        float ratio, index;     // FM modulator ratio and depth in radians
        float attack, decay;    // seconds
        float cutoff, q;        // low pass, in Hz
        float level;
        std::array<int, phraseLength> phrase; // semitones above the key
        int octave;                           // MIDI note of the key, before the preset number moves it
    };

    static const Preset& presetFor (int kind)
    {
        static const std::array<Preset, numPresetKinds> presets { {
            { false, 1.0f, 0.0f, 0.12f, 1.6f, 1800.0f, 0.8f, 0.05f, { 0, 7, 12, 16 }, 57 },  // glass pad
            { true, 3.5f, 2.2f, 0.002f, 1.4f, 6000.0f, 0.7f, 0.08f, { 0, 12, 7, 19 }, 48 },  // deep bell
            { true, 1.0f, 1.8f, 0.002f, 0.25f, 4000.0f, 0.7f, 0.1f, { 0, 3, 7, 10 }, 60 },   // soft pluck
            { false, 1.0f, 0.0f, 0.25f, 2.2f, 600.0f, 1.2f, 0.07f, { 0, 7, 0, 5 }, 45 },     // warm drone
            { false, 1.0f, 0.0f, 0.01f, 0.5f, 3200.0f, 3.0f, 0.045f, { 12, 15, 19, 22 }, 60 }, // bright lead
            { true, 1.0f, 0.9f, 0.003f, 0.9f, 900.0f, 0.7f, 0.1f, { 0, 4, 7, 11 }, 52 },     // dark keys
        } };

        return presets[(std::size_t) (kind % numPresetKinds)];
    }

    void startPhrase (int row)
    {
        // The number of the preset moves its phrase around the key.
        static constexpr std::array<int, 8> keyShifts { 0, 2, -3, 5, -2, 3, -5, 7 };

        for (auto& voice : voices)
            voice.fadeOut (sampleRate);

        phraseKind = row % numPresetKinds;
        phraseKey = presetFor (phraseKind).octave + keyShifts[(std::size_t) ((row / numPresetKinds) % 8)];
        pad.setKey (phraseKey);
        phraseNote = 0;
        samplesToNextNote = 0.0f;
    }

    void playPhraseNote()
    {
        const auto& preset = presetFor (phraseKind);
        const auto note = phraseKey + preset.phrase[(std::size_t) phraseNote];

        auto& voice = voices[(std::size_t) nextVoice];
        nextVoice = (nextVoice + 1) % static_cast<int> (voices.size());
        voice.start (preset, note, phraseNote, sampleRate);

        ++phraseNote;
        samplesToNextNote = phraseStepSeconds * sampleRate;
    }

    //==============================================================================
    /** One note of a preset, through its own low pass. */
    struct Voice
    {
        void reset()
        {
            level = 0.0f;
            filter.reset();
        }

        void start (const Preset& newPreset, int note, int position, float sampleRate)
        {
            preset = &newPreset;
            increment = 440.0f * std::exp2 ((static_cast<float> (note) - 69.0f) / 12.0f) / sampleRate;
            attackStep = 1.0f / std::max (1.0f, preset->attack * sampleRate);
            decay = std::exp (-1.0f / (preset->decay * sampleRate));
            index = preset->index;
            indexDecay = std::exp (-1.0f / (0.3f * preset->decay * sampleRate));
            attack = 0.0f;
            level = preset->level;
            fading = false;

            // The four notes of a phrase spread across the stereo field.
            const auto pan = 0.3f + 0.4f * static_cast<float> (position) / (phraseLength - 1);
            leftGain = std::cos (pan * yup::MathConstants<float>::halfPi);
            rightGain = std::sin (pan * yup::MathConstants<float>::halfPi);

            filter.setParameters (yup::FilterMode::lowpass, preset->cutoff, preset->q, 0.0, sampleRate);
        }

        /** Hurries the note out, for a new phrase. */
        void fadeOut (float sampleRate)
        {
            if (level > 0.0f)
            {
                fading = true;
                fadeDecay = std::exp (-1.0f / (0.05f * sampleRate));
            }
        }

        float next()
        {
            if (preset == nullptr || level < 1.0e-5f)
                return 0.0f;

            float raw;
            if (preset->fm)
            {
                raw = sineOfCycles (phase + index * sineOfCycles (modPhase) * (1.0f / yup::MathConstants<float>::twoPi));
                modPhase += increment * preset->ratio;
                modPhase -= std::floor (modPhase);
                index *= indexDecay;
            }
            else
            {
                raw = 0.5f * (polyBlepSaw (phase, increment) + polyBlepSaw (detunedPhase, increment * 1.006f));
                detunedPhase += increment * 1.006f;
                detunedPhase -= std::floor (detunedPhase);
            }

            phase += increment;
            phase -= std::floor (phase);

            attack = std::min (1.0f, attack + attackStep);
            level *= fading ? fadeDecay : decay;

            return filter.processSample (raw) * level * attack;
        }

        const Preset* preset = nullptr;
        float phase = 0.0f, modPhase = 0.0f, detunedPhase = 0.0f, increment = 0.0f;
        float level = 0.0f, attack = 0.0f, attackStep = 0.0f, decay = 1.0f, fadeDecay = 1.0f;
        float index = 0.0f, indexDecay = 1.0f;
        float leftGain = 0.7f, rightGain = 0.7f;
        bool fading = false;
        yup::VAStateVariableFilter<float> filter;
    };

    //==============================================================================
    yup::AbstractFifo events { maxEvents };
    std::array<Event, maxEvents> pendingEvents {};

    // Audio thread only
    std::array<Voice, 8> voices;
    std::array<Click, 12> clicks {};
    Pad pad;
    float sampleRate = 44100.0f;
    float samplesToNextNote = 0.0f;
    int nextVoice = 0, nextClick = 0;
    int phraseKind = 0, phraseKey = 57, phraseNote = phraseLength;
};
