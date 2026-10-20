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
    Water through a small mixer, for the Ripple demo.

    Every ring on the panel is a drop: a short sine gliding up, pitched on a minor
    pentatonic scale by where the ring lands, louder for stronger rings. Under the
    drops a quiet drone breathes slowly. The panel's controls then mix it for real:
    the four faders are low, mid, high and air bands of an EQ (flat in the middle),
    drive saturates, space sends it into a small reverb and power fades it all in
    or out.

    The setters and drop() are called on the message thread; everything else runs
    on the audio thread.
*/
class RippleSound : public SoundSource
{
public:
    //==============================================================================
    static constexpr int numBands = 4;

    /** Drops a drop where a ring starts: x across the panel from 0 to 1, strength from 0 to 1. */
    void drop (float x, float strength)
    {
        int start1, size1, start2, size2;
        drops.prepareToWrite (1, start1, size1, start2, size2);
        if (size1 + size2 == 0)
            return;

        pendingDrops[(std::size_t) (size1 > 0 ? start1 : start2)] = { std::clamp (x, 0.0f, 1.0f), std::clamp (strength, 0.0f, 1.0f) };
        drops.finishedWrite (1);
    }

    /** Sets one EQ band from 0 to 1, 0.5 being flat: low, mid, high, air. */
    void setBand (int band, float value)
    {
        if (yup::isPositiveAndBelow (band, numBands))
            bands[(std::size_t) band].store (value, std::memory_order_relaxed);
    }

    void setDrive (float value) { drive.store (value, std::memory_order_relaxed); }

    void setSpace (float value) { space.store (value, std::memory_order_relaxed); }

    void setPower (bool isOn) { power.store (isOn, std::memory_order_relaxed); }

    //==============================================================================
    void prepare (double newSampleRate) override
    {
        sampleRate = static_cast<float> (newSampleRate);
        voices = {};
        nextVoice = 0;
        dronePhases = {};
        for (std::size_t k = 0; k < droneBreaths.size(); ++k)
        {
            droneBreaths[k].prepare (newSampleRate);
            droneBreaths[k].setFrequency (breathRates[k]);
            droneBreaths[k].setPhaseOffset (0.25f * static_cast<float> (k));
        }
        for (auto& filter : filters)
            filter.prepare (newSampleRate, 4096);
        reverb.reset (sampleRate);
        powerGain = power.load (std::memory_order_relaxed) ? 1.0f : 0.0f;

        for (std::size_t i = 0; i < smoothedBands.size(); ++i)
            smoothedBands[i] = bands[i].load (std::memory_order_relaxed);
        smoothedDrive = drive.load (std::memory_order_relaxed);
        smoothedSpace = space.load (std::memory_order_relaxed);

        // Drops dropped while the demo was closed are stale.
        int start1, size1, start2, size2;
        drops.prepareToRead (maxPendingDrops, start1, size1, start2, size2);
        drops.finishedRead (size1 + size2);
        sineOfCycles (0.0f); // builds the table here rather than in the middle of a drop
    }

    void render (float* left, float* right, int numSamples) override
    {
        startDrops();
        updateMixer (numSamples);

        const auto powerTarget = power.load (std::memory_order_relaxed) ? 1.0f : 0.0f;
        const auto powerFollow = 1.0f - std::exp (-1.0f / (0.08f * sampleRate));
        const auto driveGain = 1.0f + 8.0f * smoothedDrive * smoothedDrive;
        const auto driveMakeup = 1.0f / std::sqrt (driveGain);
        const auto wet = 0.7f * smoothedSpace;
        const auto dry = 1.0f - 0.35f * smoothedSpace;
        const auto feedback = 0.62f + 0.32f * smoothedSpace;

        for (int i = 0; i < numSamples; ++i)
        {
            auto mono = nextDrone();
            for (auto& voice : voices)
                mono += voice.next();

            for (auto& filter : filters)
                mono = filter.processSample (mono);

            mono = driveClipper.processSample (mono * driveGain) * driveMakeup;

            powerGain += (powerTarget - powerGain) * powerFollow;
            mono *= powerGain;

            const auto [reverbLeft, reverbRight] = reverb.process (mono, feedback);
            left[i] += dry * mono + wet * reverbLeft;
            right[i] += dry * mono + wet * reverbRight;
        }
    }

private:
    //==============================================================================
    static constexpr int numVoices = 16;
    static constexpr int maxPendingDrops = 64;
    static constexpr std::array<float, 4> breathRates { 0.07f, 0.11f, 0.13f, 0.23f }; // drone partials, in Hz
    static constexpr float maxGlide = 1.6f; // how far up a drop glides, as a frequency ratio

    //==============================================================================
    /** A drop: a sine gliding quickly up a fifth or so while it dies away. */
    struct Voice
    {
        float next()
        {
            if (level < 1.0e-5f)
                return 0.0f;

            const auto out = sineOfCycles (phase) * level * std::min (1.0f, attack);

            phase += increment * glide;
            phase -= std::floor (phase);
            glide += (maxGlide - glide) * glideFollow;
            attack += attackStep;
            level *= decayCoefficient;

            return out;
        }

        float phase = 0.0f, increment = 0.0f, glide = 1.0f, glideFollow = 0.0f;
        float level = 0.0f, attack = 0.0f, attackStep = 0.0f, decayCoefficient = 0.0f;
    };

    void startDrops()
    {
        static constexpr std::array<int, 5> scale { 0, 3, 5, 7, 10 }; // minor pentatonic

        int start1, size1, start2, size2;
        drops.prepareToRead (maxPendingDrops, start1, size1, start2, size2);

        const auto start = [this] (const Drop& drop)
        {
            const auto degree = std::min (static_cast<int> (drop.x * 11.0f), 10);
            const auto note = 57 + 12 * (degree / 5) + scale[(std::size_t) (degree % 5)]; // A3 to A5
            const auto frequency = 440.0f * std::exp2 ((static_cast<float> (note) - 69.0f) / 12.0f);

            auto& voice = voices[(std::size_t) nextVoice];
            nextVoice = (nextVoice + 1) % numVoices;

            voice.phase = 0.0f;
            voice.increment = frequency / sampleRate;
            voice.glide = 1.0f;
            voice.glideFollow = 1.0f - std::exp (-1.0f / (0.018f * sampleRate));
            voice.level = 0.025f + 0.065f * drop.strength;
            voice.attack = 0.0f;
            voice.attackStep = 1.0f / (0.001f * sampleRate);
            voice.decayCoefficient = std::exp (-1.0f / ((0.09f - 0.03f * drop.x) * sampleRate));
        };

        for (int i = 0; i < size1; ++i)
            start (pendingDrops[(std::size_t) (start1 + i)]);
        for (int i = 0; i < size2; ++i)
            start (pendingDrops[(std::size_t) (start2 + i)]);

        drops.finishedRead (size1 + size2);
    }

    //==============================================================================
    /** A low A and E breathing at unrelated slow rates, and a faint high shimmer. */
    float nextDrone()
    {
        static constexpr std::array<float, 4> frequencies { 110.0f, 164.81f, 220.4f, 1760.0f };
        static constexpr std::array<float, 4> levels { 0.03f, 0.022f, 0.016f, 0.003f };

        auto out = 0.0f;
        for (std::size_t k = 0; k < frequencies.size(); ++k)
        {
            dronePhases[k] += frequencies[k] / sampleRate;
            dronePhases[k] -= std::floor (dronePhases[k]);

            const auto breath = 0.6f + 0.4f * droneBreaths[k].processSample();
            out += sineOfCycles (dronePhases[k]) * levels[k] * breath;
        }

        return out;
    }

    //==============================================================================
    /** Follows the controls once per block, smoothly, and redesigns the EQ from them. */
    void updateMixer (int numSamples)
    {
        static constexpr std::array<float, numBands> frequencies { 180.0f, 1000.0f, 4500.0f, 12000.0f };
        const std::array<yup::FilterModeType, numBands> shapes { yup::FilterMode::lowshelf, yup::FilterMode::peak, yup::FilterMode::highshelf, yup::FilterMode::highshelf };

        const auto follow = 1.0f - std::exp (-static_cast<float> (numSamples) / (0.03f * sampleRate));

        for (std::size_t i = 0; i < smoothedBands.size(); ++i)
        {
            smoothedBands[i] += (bands[i].load (std::memory_order_relaxed) - smoothedBands[i]) * follow;
            filters[i].setParameters (shapes[i], frequencies[i], 0.75, (smoothedBands[i] - 0.5f) * 24.0f, sampleRate);
        }

        smoothedDrive += (drive.load (std::memory_order_relaxed) - smoothedDrive) * follow;
        smoothedSpace += (space.load (std::memory_order_relaxed) - smoothedSpace) * follow;
    }

    //==============================================================================
    /** A small stereo reverb: two allpass diffusers into four delay lines, damped by one pole low passes
        and mixed by a Hadamard matrix. */
    struct Reverb
    {
        static constexpr int lineSize = 8192;     // over 80 ms at 96 kHz
        static constexpr int diffuserSize = 1024; // over 10 ms at 96 kHz
        static constexpr std::array<float, 4> lineSeconds { 0.0297f, 0.0371f, 0.0411f, 0.0437f };
        static constexpr std::array<float, 2> diffuserSeconds { 0.0053f, 0.0079f };

        /** Clears the lines in place: a cleared copy would not fit the audio thread's stack. */
        void reset (float newSampleRate)
        {
            sampleRate = newSampleRate;

            for (auto& line : lines)
                line.clear();
            for (auto& line : diffusers)
                line.clear();

            for (auto& filter : damping)
            {
                filter.setParameters (yup::FilterMode::lowpass, 4000.0, 0.0, sampleRate);
                filter.reset();
            }
        }

        std::pair<float, float> process (float input, float feedback)
        {
            // A line's newest sample sits at its last index, one sample ago.
            const auto delayed = [this] (const auto& line, int size, float seconds)
            {
                return line[size - std::clamp (static_cast<int> (seconds * sampleRate), 1, size)];
            };

            for (std::size_t d = 0; d < diffusers.size(); ++d)
            {
                const auto echo = delayed (diffusers[d], diffuserSize, diffuserSeconds[d]);
                const auto x = input + 0.6f * echo;
                diffusers[d].push (x);
                input = echo - 0.6f * x;
            }

            std::array<float, 4> outs, damped;
            for (std::size_t k = 0; k < lines.size(); ++k)
            {
                outs[k] = delayed (lines[k], lineSize, lineSeconds[k]);
                damped[k] = damping[k].processSample (outs[k]);
            }

            const auto& d = damped;
            const std::array<float, 4> mixed { d[0] + d[1] + d[2] + d[3], d[0] - d[1] + d[2] - d[3], d[0] + d[1] - d[2] - d[3], d[0] - d[1] - d[2] + d[3] };

            for (std::size_t k = 0; k < lines.size(); ++k)
                lines[k].push (input + 0.5f * feedback * mixed[k]);

            return { outs[0] + outs[2], outs[1] + outs[3] };
        }

        std::array<yup::CircularBuffer<float, lineSize>, 4> lines;
        std::array<yup::CircularBuffer<float, diffuserSize>, 2> diffusers;
        std::array<yup::FirstOrderFilter<float>, 4> damping;
        float sampleRate = 44100.0f;
    };

    //==============================================================================
    struct Drop
    {
        float x = 0.0f, strength = 0.0f;
    };

    yup::AbstractFifo drops { maxPendingDrops };
    std::array<Drop, maxPendingDrops> pendingDrops {};
    std::array<std::atomic<float>, numBands> bands {};
    std::atomic<float> drive { 0.0f }, space { 0.0f };
    std::atomic<bool> power { true };

    // Audio thread only
    std::array<Voice, numVoices> voices {};
    std::array<yup::RbjFilter<float>, numBands> filters;
    yup::SoftClipper<float> driveClipper { 1.0, 0.3 }; // soft knee from a third of full scale
    std::array<float, numBands> smoothedBands {};
    std::array<float, 4> dronePhases {};
    Reverb reverb;
    float sampleRate = 44100.0f;
    std::array<yup::LFO<float>, 4> droneBreaths;
    float powerGain = 1.0f;
    float smoothedDrive = 0.0f, smoothedSpace = 0.0f;
    int nextVoice = 0;
};
