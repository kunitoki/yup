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
#include <utility>

//==============================================================================
/**
    A pad and a pulsing drone in D minor, shaped by where the pointer is, for the Layers demo.

    The pad is a held chord of detuned saws with a sine sub, through a resonant
    low pass. Under it a low D pulses on a sixteenth clock, each pulse a short
    filter pluck, with accents and octave jumps like an analog sequencer.

    Across (x, 0 to 1) the sound goes from dark to bright: the pad filter opens and
    the plucks get snappier and more resonant. Down (y, 0 to 1) it goes from calm to
    busy: the pad grows from root and fifth to a minor ninth chord with wider
    detune, and the pulse fills in from quarters to accented sixteenths.

    setPosition() is called on the message thread; everything else runs on the
    audio thread.
*/
class LayersSound : public SoundSource
{
public:
    //==============================================================================
    /** Where the pointer is, from 0 to 1 on both axes; the sound glides there. */
    void setPosition (float x, float y)
    {
        targetX.store (std::clamp (x, 0.0f, 1.0f), std::memory_order_relaxed);
        targetY.store (std::clamp (y, 0.0f, 1.0f), std::memory_order_relaxed);
    }

    //==============================================================================
    void prepare (double newSampleRate) override
    {
        sampleRate = static_cast<float> (newSampleRate);
        x = targetX.load (std::memory_order_relaxed);
        y = targetY.load (std::memory_order_relaxed);

        // Spread starting phases, so the detuned pairs do not begin in unison and sweep together.
        for (std::size_t k = 0; k < padPhases.size(); ++k)
            padPhases[k] = std::fmod (0.381966f * static_cast<float> (k), 1.0f);

        subPhase = 0.0f;
        cutoffBreath.prepare (newSampleRate);
        cutoffBreath.setFrequency (0.08f);
        for (auto& filter : padFilters)
            filter.reset();

        pulse.phase = pulse.octavePhase = 0.0f;
        pulse.level = pulse.peak = pulse.envelope = 0.0f;
        pulse.attackSamples = 0;
        pulse.filter.reset();
        step = 0;
        samplesToNextStep = 0.0f;
    }

    void render (float* left, float* right, int numSamples) override
    {
        const auto glide = 1.0f - std::exp (-1.0f / (0.5f * sampleRate));
        const auto stepSamples = sampleRate * 60.0f / (bpm * 4.0f);
        const auto wantedX = targetX.load (std::memory_order_relaxed);
        const auto wantedY = targetY.load (std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            x += (wantedX - x) * glide;
            y += (wantedY - y) * glide;

            if (samplesToNextStep <= 0.0f)
            {
                playStep();
                samplesToNextStep += stepSamples;
            }
            samplesToNextStep -= 1.0f;

            const auto [padLeft, padRight] = nextPad();
            const auto drone = nextPulse();

            left[i] += padLeft + drone;
            right[i] += padRight + drone;
        }
    }

private:
    //==============================================================================
    static constexpr float bpm = 100.0f;
    static constexpr int numPadNotes = 5;

    /** D3, A3, D4, then the minor third F4 and the ninth E5 that the lower half of the panel brings in. */
    static constexpr std::array<int, numPadNotes> padNotes { 50, 57, 62, 65, 76 };

    static float frequencyOf (float midiNote) { return 440.0f * std::exp2 ((midiNote - 69.0f) / 12.0f); }

    static float smoothStep (float edge0, float edge1, float value)
    {
        const auto t = std::clamp ((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    //==============================================================================
    std::pair<float, float> nextPad()
    {
        // Each note is two saws detuned apart, one to each side; the lower half widens the detune.
        const auto detuneCents = 4.0f + 14.0f * y;
        const auto spread = std::exp2 (detuneCents / 1200.0f);
        const std::array<float, numPadNotes> gains { 1.0f, 0.9f, 0.8f, 0.75f * smoothStep (0.25f, 0.6f, y), 0.55f * smoothStep (0.55f, 0.95f, y) };

        auto l = 0.0f, r = 0.0f;
        for (std::size_t n = 0; n < padNotes.size(); ++n)
        {
            const auto frequency = frequencyOf (static_cast<float> (padNotes[n]));

            for (std::size_t side = 0; side < 2; ++side)
            {
                auto& phase = padPhases[n * 2 + side];
                const auto increment = frequency * (side == 0 ? 1.0f / spread : spread) / sampleRate;

                phase += increment;
                phase -= std::floor (phase);

                const auto value = gains[n] * polyBlepSaw (phase, increment);
                (side == 0 ? l : r) += value;
            }
        }

        // A slow breath on the cutoff keeps the held chord alive.
        const auto cutoff = 250.0f * std::exp2 (4.3f * x) * (1.0f + 0.15f * cutoffBreath.processSample());
        const auto q = 1.0f / (1.4f - 0.9f * x * x);

        subPhase += frequencyOf (38.0f) / sampleRate;
        subPhase -= std::floor (subPhase);
        const auto sub = 0.05f * sineOfCycles (subPhase);

        for (auto& filter : padFilters)
            filter.setParameters (yup::FilterMode::lowpass, cutoff, q, 0.0, sampleRate);

        return { padLevel * padFilters[0].processSample (l) + sub,
                 padLevel * padFilters[1].processSample (r) + sub };
    }

    //==============================================================================
    /** The pulsing drone: one voice whose level and filter are plucked at every step it plays. */
    struct Pulse
    {
        float phase = 0.0f, octavePhase = 0.0f, increment = 0.0f;
        float level = 0.0f, peak = 0.0f, envelope = 0.0f;
        int attackSamples = 0;
        yup::VAStateVariableFilter<float> filter;
    };

    void playStep()
    {
        static constexpr std::array<float, 16> accents { 1.0f, 0.45f, 0.6f, 0.45f, 0.85f, 0.45f, 0.6f, 0.5f, 0.95f, 0.45f, 0.6f, 0.45f, 0.8f, 0.5f, 0.65f, 0.55f };
        static constexpr std::array<int, 16> octaves { 0, 0, 12, 0, 0, 12, 0, 0, 0, 0, 12, 0, 0, 12, 0, 12 };

        const auto index = (std::size_t) step;
        step = (step + 1) % 16;

        // Quarters at the top, eighths from a third of the way down, sixteenths from two thirds.
        const auto plays = index % 4 == 0 || (y > 0.33f && index % 2 == 0) || y > 0.66f;
        if (! plays)
            return;

        const auto jump = y > 0.5f ? octaves[index] : 0;
        pulse.increment = frequencyOf (static_cast<float> (38 + jump)) / sampleRate; // D2, or D3 on a jump
        pulse.peak = 0.13f * accents[index];
        pulse.attackSamples = static_cast<int> (0.003f * sampleRate);
        pulse.envelope = 1.0f;
    }

    float nextPulse()
    {
        // Picks up from wherever the last pluck left the level, so pulses never click.
        if (pulse.attackSamples > 0)
        {
            pulse.level += (pulse.peak - pulse.level) / static_cast<float> (pulse.attackSamples);
            --pulse.attackSamples;
        }
        else
        {
            pulse.level *= std::exp (-1.0f / ((0.22f - 0.1f * x) * sampleRate));
        }

        pulse.envelope *= std::exp (-1.0f / ((0.09f - 0.05f * x) * sampleRate));

        if (pulse.level < 1.0e-5f && pulse.attackSamples == 0)
            return 0.0f;

        pulse.phase += pulse.increment;
        pulse.phase -= std::floor (pulse.phase);
        pulse.octavePhase += 2.0f * pulse.increment;
        pulse.octavePhase -= std::floor (pulse.octavePhase);

        const auto raw = polyBlepSaw (pulse.phase, pulse.increment) + 0.5f * polyBlepSaw (pulse.octavePhase, 2.0f * pulse.increment);
        const auto cutoff = 120.0f * std::exp2 (3.5f * x) * (1.0f + (2.0f + 10.0f * x) * pulse.envelope);
        pulse.filter.setParameters (yup::FilterMode::lowpass, cutoff, 1.0f / (1.4f - 0.9f * x), 0.0, sampleRate);

        return pulse.level * pulse.filter.processSample (raw);
    }

    //==============================================================================
    static constexpr float padLevel = 0.035f;

    std::atomic<float> targetX { 0.5f }, targetY { 0.5f };

    // Audio thread only
    std::array<float, numPadNotes * 2> padPhases {};
    std::array<yup::VAStateVariableFilter<float>, 2> padFilters; // one per side, so the detuned pairs stay apart
    Pulse pulse;
    float sampleRate = 44100.0f;
    float x = 0.5f, y = 0.5f;
    yup::LFO<float> cutoffBreath;
    float subPhase = 0.0f;
    float samplesToNextStep = 0.0f;
    int step = 0;
};
