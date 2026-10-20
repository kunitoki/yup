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
    What the demos hear of the sound they play.

    The audio thread pushes the output in; every half window a short FFT measures
    the energy of the low, mid and high bands and the hits (sudden rises of energy,
    from a spectral flux). Alongside it, a long window tuned to the notes of the keys
    demo, C3 to C5, measures how loud each of those notes is: a short FFT cannot tell
    neighbouring low notes apart. The demos read the result once per frame, without
    locking.
*/
class AudioAnalysis
{
public:
    //==============================================================================
    static constexpr int fftSize = 256;
    static constexpr int firstNote = 48; // C3, the lowest key of the keys demo
    static constexpr int numNotes = 25;  // up to C5

    /** One reading: levels from 0 (silent, -60 dB) to 1 (full scale), and a count of the hits so far. */
    struct Levels
    {
        float low = 0.0f, mid = 0.0f, high = 0.0f;
        std::array<float, numNotes> notes {}; // from C3 up, one per semitone
        std::uint32_t hits = 0;
    };

    //==============================================================================
    AudioAnalysis()
        : fft (fftSize)
    {
        yup::WindowFunctions<float>::generate (yup::WindowType::hann, window.data(), window.size());
        yup::WindowFunctions<float>::generate (yup::WindowType::hann, noteWindow.data(), noteWindow.size());
    }

    //==============================================================================
    /** Feeds a block of the output, called on the audio thread. */
    void push (const float* left, const float* right, int numSamples, double sampleRate)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto mono = 0.5f * (left[i] + (right != nullptr ? right[i] : left[i]));
            history.push (mono);
            noteHistory.push (mono);

            if (++samplesSinceAnalysis >= fftSize / 2)
            {
                samplesSinceAnalysis = 0;
                analyse (static_cast<float> (sampleRate));
            }

            if (++samplesSinceNotes >= noteHop)
            {
                samplesSinceNotes = 0;
                analyseNotes (static_cast<float> (sampleRate));
            }
        }
    }

    //==============================================================================
    /** The latest reading, for the message thread. */
    Levels read() const
    {
        Levels levels;
        levels.low = low.load (std::memory_order_relaxed);
        levels.mid = mid.load (std::memory_order_relaxed);
        levels.high = high.load (std::memory_order_relaxed);
        levels.hits = hits.load (std::memory_order_relaxed);

        for (std::size_t i = 0; i < notes.size(); ++i)
            levels.notes[i] = notes[i].load (std::memory_order_relaxed);

        return levels;
    }

private:
    //==============================================================================
    static constexpr int numBins = fftSize / 2 + 1;

    /** -60 dB to 0 dB of a sine's amplitude, as 0 to 1. */
    static float toLevel (float amplitude)
    {
        return std::clamp ((20.0f * std::log10 (amplitude + 1.0e-6f) + 60.0f) / 60.0f, 0.0f, 1.0f);
    }

    void analyse (float sampleRate)
    {
        for (int i = 0; i < fftSize; ++i)
            windowed[(std::size_t) i] = history[i] * window[(std::size_t) i];

        fft.performRealFFTForward (windowed.data(), spectrumData.data());

        // A sine of amplitude A peaks at A N / 4 through a Hann window.
        std::array<float, numBins> magnitudes;
        for (std::size_t k = 0; k < magnitudes.size(); ++k)
            magnitudes[k] = std::hypot (spectrumData[2 * k], spectrumData[2 * k + 1]) * (4.0f / fftSize);

        const auto binHz = sampleRate / fftSize;
        const auto bandLevel = [&] (float fromHz, float toHz)
        {
            auto peak = 0.0f;
            for (std::size_t k = 1; k < magnitudes.size(); ++k)
            {
                const auto hz = static_cast<float> (k) * binHz;
                if (hz >= fromHz && hz < toHz)
                    peak = std::max (peak, magnitudes[k]);
            }

            return toLevel (peak);
        };

        low.store (bandLevel (0.0f, 250.0f), std::memory_order_relaxed);
        mid.store (bandLevel (250.0f, 2000.0f), std::memory_order_relaxed);
        high.store (bandLevel (2000.0f, 20000.0f), std::memory_order_relaxed);

        // A hit is a rise of the summed log spectrum well above its recent average.
        auto flux = 0.0f;
        for (std::size_t k = 1; k < magnitudes.size(); ++k)
        {
            const auto level = toLevel (magnitudes[k]);
            flux += std::max (0.0f, level - previous[k]);
            previous[k] = level;
        }

        hopsSinceHit = std::min (hopsSinceHit + 1, minimumHopsBetweenHits);
        if (flux > 1.8f * fluxAverage + 0.6f && hopsSinceHit >= minimumHopsBetweenHits)
        {
            hits.fetch_add (1, std::memory_order_relaxed);
            hopsSinceHit = 0;
        }

        fluxAverage += (flux - fluxAverage) * 0.1f;
    }

    /** One Goertzel filter per note over the long window: the amplitude of each key's own pitch. */
    void analyseNotes (float sampleRate)
    {
        for (int i = 0; i < noteWindowSize; ++i)
            noteWindowed[(std::size_t) i] = noteHistory[i] * noteWindow[(std::size_t) i];

        for (std::size_t n = 0; n < notes.size(); ++n)
        {
            const auto hz = 440.0f * std::exp2 ((static_cast<float> (firstNote + static_cast<int> (n)) - 69.0f) / 12.0f);
            const auto coefficient = 2.0f * std::cos (yup::MathConstants<float>::twoPi * hz / sampleRate);

            auto s1 = 0.0f, s2 = 0.0f;
            for (const auto x : noteWindowed)
            {
                const auto s0 = x + coefficient * s1 - s2;
                s2 = s1;
                s1 = s0;
            }

            // Like an FFT bin: a sine of amplitude A comes out at A N / 4 through a Hann window.
            const auto power = std::max (0.0f, s1 * s1 + s2 * s2 - coefficient * s1 * s2);
            notes[n].store (toLevel (std::sqrt (power) * (4.0f / noteWindowSize)), std::memory_order_relaxed);
        }
    }

    //==============================================================================
    static constexpr int minimumHopsBetweenHits = 12; // about 30 ms at 48 kHz
    static constexpr int noteWindowSize = 16384;     // about a third of a second at 48 kHz: C3 and C#3 a few bins apart
    static constexpr int noteHop = 2048;

    yup::FFTProcessor<float> fft;
    std::array<float, fftSize> window {}, windowed {};
    std::array<float, fftSize * 2 + 2> spectrumData {};
    yup::CircularBuffer<float, fftSize> history;
    std::array<float, numBins> previous {};
    float fluxAverage = 0.0f;
    int samplesSinceAnalysis = 0;

    std::array<float, noteWindowSize> noteWindow {}, noteWindowed {};
    yup::CircularBuffer<float, noteWindowSize> noteHistory;
    int samplesSinceNotes = 0;
    int hopsSinceHit = minimumHopsBetweenHits;

    std::atomic<float> low { 0.0f }, mid { 0.0f }, high { 0.0f };
    std::array<std::atomic<float>, numNotes> notes {};
    std::atomic<std::uint32_t> hits { 0 };
};
