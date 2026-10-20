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
#include <initializer_list>

//==============================================================================
/**
    A four operator FM pluck playing a two bar loop through a ping-pong delay, for the Facet demo.

    The operators are sines at 1, 2, 3 and 4 times the note, stacked 4 into 3 into 2
    into 1, and operator 1 is heard. The four amounts are: how much operator 1
    modulates itself (feedback), and how much 2 modulates 1, 3 modulates 2 and 4
    modulates 3. Each note's modulation decays with it, which makes the pluck.

    setAmount() is called on the message thread; everything else runs on the audio thread.
*/
class FacetSound : public SoundSource
{
public:
    //==============================================================================
    static constexpr int numAmounts = 4;

    /** Sets one of the four FM amounts, from 0 to 1: 0 is operator 1 feedback, then 2 into 1, 3 into 2, 4 into 3. */
    void setAmount (int which, float amount)
    {
        if (yup::isPositiveAndBelow (which, numAmounts))
            amounts[(std::size_t) which].store (std::clamp (amount, 0.0f, 1.0f), std::memory_order_relaxed);
    }

    //==============================================================================
    void prepare (double newSampleRate) override
    {
        sampleRate = static_cast<float> (newSampleRate);
        voices = {};
        nextVoice = 0;
        step = 0;
        samplesToNextStep = 0.0f;
        delayLeft.clear();
        delayRight.clear();

        for (auto& filter : damping)
        {
            filter.setParameters (yup::FilterMode::lowpass, 3500.0, 0.0, newSampleRate);
            filter.reset();
        }

        for (std::size_t i = 0; i < smoothed.size(); ++i)
            smoothed[i] = amounts[i].load (std::memory_order_relaxed);

        sineOfCycles (0.0f); // builds the table here rather than in the middle of a note
    }

    void render (float* left, float* right, int numSamples) override
    {
        const auto stepSamples = sampleRate * 60.0f / (bpm * 4.0f);
        const auto delaySamples = std::clamp (static_cast<int> (stepSamples * 3.0f), 1, delaySize); // a dotted eighth
        const auto smoothing = 1.0f - std::exp (-1.0f / (0.02f * sampleRate));

        std::array<float, numAmounts> targets;
        for (std::size_t i = 0; i < targets.size(); ++i)
            targets[i] = amounts[i].load (std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            if (samplesToNextStep <= 0.0f)
            {
                playStep();
                samplesToNextStep += stepSamples;
            }
            samplesToNextStep -= 1.0f;

            for (std::size_t k = 0; k < smoothed.size(); ++k)
                smoothed[k] += (targets[k] - smoothed[k]) * smoothing;

            // Knob to modulation depth in radians: squared, so the low end of each knob is fine grained.
            const auto feedback = 1.1f * smoothed[0];
            const auto depth2 = 5.0f * smoothed[1] * smoothed[1];
            const auto depth3 = 5.0f * smoothed[2] * smoothed[2];
            const auto depth4 = 5.0f * smoothed[3] * smoothed[3];

            auto dry = 0.0f;
            for (auto& voice : voices)
                dry += voice.next (feedback, depth2, depth3, depth4);

            // Ping-pong: the left line feeds the right one and back, each repeat a little darker.
            // A line's newest sample sits at its last index, one sample ago.
            const auto echoLeft = delayLeft[delaySize - delaySamples];
            const auto echoRight = delayRight[delaySize - delaySamples];

            delayLeft.push (dry + damping[1].processSample (echoRight) * delayFeedback);
            delayRight.push (damping[0].processSample (echoLeft) * delayFeedback);

            left[i] += dry + delayMix * echoLeft;
            right[i] += dry + delayMix * echoRight;
        }
    }

private:
    //==============================================================================
    static constexpr float bpm = 112.0f;
    static constexpr int numVoices = 6;
    static constexpr int delaySize = 65536; // over a second at 48 kHz
    static constexpr float delayFeedback = 0.5f;
    static constexpr float delayMix = 0.3f;
    static constexpr int rest = -1;

    /** Two bars of sixteenths over A minor then F major, with velocities: 1 is an accent. */
    static constexpr std::array<int, 32> pattern { {
        45, rest, 57, 52, rest, 60, 57, 55, 45, rest, 64, 62, 60, rest, 55, 52,
        41, rest, 53, 60, rest, 57, 53, 52, 41, rest, 60, 57, 55, rest, 52, 48,
    } };

    static constexpr std::array<float, 16> accents { {
        1.0f, 0.0f, 0.6f, 0.5f, 0.0f, 0.8f, 0.55f, 0.5f, 0.9f, 0.0f, 0.65f, 0.55f, 0.75f, 0.0f, 0.6f, 0.5f,
    } };

    //==============================================================================
    struct Voice
    {
        float next (float feedback, float depth2, float depth3, float depth4)
        {
            if (level < 1.0e-5f && ! attacking)
                return 0.0f;

            constexpr auto toCycles = 1.0f / yup::MathConstants<float>::twoPi;

            const auto o4 = sineOfCycles (phase4);
            const auto o3 = sineOfCycles (phase3 + depth4 * modulation * o4 * toCycles);
            const auto o2 = sineOfCycles (phase2 + depth3 * modulation * o3 * toCycles);
            const auto o1 = sineOfCycles (phase1 + (depth2 * modulation * o2 + feedback * 0.5f * (last1 + last2)) * toCycles);

            last2 = last1;
            last1 = o1;

            for (auto* phase : { &phase1, &phase2, &phase3, &phase4 })
                *phase -= std::floor (*phase);

            phase1 += increment;
            phase2 += 2.0f * increment;
            phase3 += 3.0f * increment;
            phase4 += 4.0f * increment;

            if (attacking)
            {
                level += (peak - level) * attackCoefficient;
                attacking = --attackSamples > 0;
            }
            else
            {
                level *= decayCoefficient;
            }

            modulation = modulationFloor + (modulation - modulationFloor) * modulationCoefficient;

            return o1 * level;
        }

        float phase1 = 0.0f, phase2 = 0.0f, phase3 = 0.0f, phase4 = 0.0f;
        float last1 = 0.0f, last2 = 0.0f;
        float increment = 0.0f;
        float level = 0.0f, peak = 0.0f, attackCoefficient = 0.0f, decayCoefficient = 0.0f;
        float modulation = 0.0f, modulationFloor = 0.0f, modulationCoefficient = 0.0f;
        int attackSamples = 0;
        bool attacking = false;
    };

    void playStep()
    {
        const auto index = (std::size_t) step;
        step = (step + 1) % static_cast<int> (pattern.size());

        const auto note = pattern[index];
        if (note == rest)
            return;

        const auto velocity = accents[index % accents.size()];
        const auto perSample = [this] (float seconds) { return std::exp (-1.0f / (seconds * sampleRate)); };

        // Phases and level carry on from the note the voice played before, so reusing it never clicks.
        auto& voice = voices[(std::size_t) nextVoice];
        nextVoice = (nextVoice + 1) % numVoices;

        voice.increment = 440.0f * std::exp2 ((static_cast<float> (note) - 69.0f) / 12.0f) / sampleRate;
        voice.peak = 0.16f * (0.4f + 0.6f * velocity);
        voice.attacking = true;
        voice.attackSamples = static_cast<int> (0.006f * sampleRate);
        voice.attackCoefficient = 1.0f - perSample (0.0015f);
        voice.decayCoefficient = perSample (note < 50 ? 0.32f : 0.2f);
        voice.modulation = 0.5f + 0.5f * velocity;
        voice.modulationFloor = 0.25f;
        voice.modulationCoefficient = perSample (0.12f);
    }

    //==============================================================================
    std::array<std::atomic<float>, numAmounts> amounts {};

    // Audio thread only
    std::array<float, numAmounts> smoothed {};
    std::array<Voice, numVoices> voices {};
    yup::CircularBuffer<float, delaySize> delayLeft, delayRight;
    std::array<yup::FirstOrderFilter<float>, 2> damping; // darkens every repeat
    float sampleRate = 44100.0f;
    float samplesToNextStep = 0.0f;
    int nextVoice = 0;
    int step = 0;
};
