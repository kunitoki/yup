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

namespace yup
{

//==============================================================================
/**
    Exact additive oscillator.

    Synthesizes a FourierSeries at a given fundamental frequency, using only the
    harmonics that stay strictly below Nyquist, so the output never aliases. It is
    the reference renderer of this folder: whatever the wavetable oscillator
    produces should match it, and a synchronizing transform can be verified
    against it as well.

    Synthesis is band-unlimited on purpose - the caller decides how many harmonics
    the series holds - and each sample costs a phase increment plus one multiply
    accumulate per active harmonic, evaluated laneCount harmonics at a time in
    SIMDRegister lanes. Two transcendental calls per sample are needed to rotate
    the harmonic phasors, and no per-harmonic state is kept, so there is no
    long-term drift.

    @tparam SampleType  Type for the synthesized samples (float or double).
    @tparam CoeffType   Type for the coefficients and phase math (default double).

    @see FourierSeries, WavetableOscillator, SyncOscillator
*/
template <typename SampleType, typename CoeffType = double>
class AdditiveOscillator
{
public:
    //==============================================================================
    /** Number of harmonics evaluated per SIMD step. */
    static constexpr int laneCount = std::is_same_v<CoeffType, float> ? 8 : 4;

    /** SIMD register type used for the harmonic accumulation. */
    using Register = SIMDRegister<CoeffType, laneCount>;

    //==============================================================================
    /** Default constructor. Call prepare() before processing. */
    AdditiveOscillator() = default;

    //==============================================================================
    /**
        Allocates the internal series and recomputes the harmonic limit.

        @param sampleRate      Sample rate in Hz.
        @param maxHarmonics    Maximum number of harmonics to store and synthesize.
    */
    void prepare (double sampleRate, int maxHarmonics)
    {
        jassert (sampleRate > 0.0);
        jassert (maxHarmonics >= 0);

        this->sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        series.resize (maxHarmonics);

        reset();
    }

    /** Resets the oscillator phase to zero. */
    void reset() noexcept
    {
        phase = 0.0;
    }

    //==============================================================================
    /** Sets the fundamental frequency in Hz. */
    void setFrequency (CoeffType newFrequency) noexcept
    {
        const auto sanitized = jmax (newFrequency, static_cast<CoeffType> (0));

        if (! approximatelyEqual (frequency, sanitized))
        {
            frequency = sanitized;
            updateActiveHarmonics();
        }
    }

    /** Returns the fundamental frequency in Hz. */
    CoeffType getFrequency() const noexcept { return frequency; }

    /** Sets the phase, normalized to one period. */
    void setPhase (CoeffType newPhase) noexcept
    {
        phase = static_cast<double> (newPhase - std::floor (newPhase));
    }

    /** Returns the phase, normalized to one period. */
    CoeffType getPhase() const noexcept { return static_cast<CoeffType> (phase); }

    //==============================================================================
    /** Copies a series into the oscillator, without rendering anything yet. */
    void setSeries (const FourierSeries<CoeffType>& newSeries) noexcept
    {
        series.copyFrom (newSeries);
        updateActiveHarmonics();
    }

    /** Fills the series with one of the convenient presets. */
    void setWaveform (Waveform waveform) noexcept
    {
        series.setWaveform (waveform);
        updateActiveHarmonics();
    }

    /** Returns the series being synthesized. */
    const FourierSeries<CoeffType>& getSeries() const noexcept { return series; }

    /** Selects whether the series' DC coefficient is synthesized (off by default). */
    void setIncludeDC (bool shouldIncludeDC) noexcept { includeDC = shouldIncludeDC; }

    /** Returns whether the DC coefficient is synthesized. */
    bool getIncludeDC() const noexcept { return includeDC; }

    /** Returns the number of harmonics currently synthesized. */
    int getNumActiveHarmonics() const noexcept { return activeHarmonics; }

    /** Returns the number of harmonics the oscillator can synthesize. */
    int getNumHarmonics() const noexcept { return series.getNumHarmonics(); }

    //==============================================================================
    /** Synthesizes one sample. */
    SampleType processSample() noexcept
    {
        const auto value = activeHarmonics > 0 ? synthesizeSample() : CoeffType (0);

        phase += static_cast<double> (frequency) / sampleRate;
        phase -= std::floor (phase);

        return static_cast<SampleType> (value);
    }

    /** Synthesizes a block of samples. */
    void processBlock (SampleType* output, int numSamples) noexcept
    {
        if (output == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

private:
    //==============================================================================
    void updateActiveHarmonics() noexcept
    {
        activeHarmonics = getNyquistHarmonicLimit (frequency, sampleRate, series.getNumHarmonics());
    }

    CoeffType synthesizeSample() noexcept
    {
        const auto theta = MathConstants<CoeffType>::twoPi * static_cast<CoeffType> (phase);
        const auto stepCosine = std::cos (theta);
        const auto stepSine = std::sin (theta);

        CoeffType laneCosine[laneCount] = {};
        CoeffType laneSine[laneCount] = {};

        auto cosine = stepCosine;
        auto sine = stepSine;

        for (int lane = 0; lane < laneCount; ++lane)
        {
            laneCosine[lane] = cosine;
            laneSine[lane] = sine;

            if (lane + 1 < laneCount)
            {
                const auto nextCosine = cosine * stepCosine - sine * stepSine;
                const auto nextSine = sine * stepCosine + cosine * stepSine;

                cosine = nextCosine;
                sine = nextSine;
            }
        }

        auto laneCosines = Register::loadUnaligned (laneCosine);
        auto laneSines = Register::loadUnaligned (laneSine);
        const auto blockCosines = Register::broadcast (cosine);
        const auto blockSines = Register::broadcast (sine);

        const auto* cosineCoefficients = series.getCosineCoefficients().data();
        const auto* sineCoefficients = series.getSineCoefficients().data();

        auto accumulator = Register::zero();

        int harmonic = 1;

        while (harmonic + laneCount - 1 <= activeHarmonics)
        {
            const auto index = static_cast<std::size_t> (harmonic - 1);

            accumulator = accumulator
                              .mulAdd (Register::loadUnaligned (cosineCoefficients + index), laneCosines)
                              .mulAdd (Register::loadUnaligned (sineCoefficients + index), laneSines);

            const auto nextCosines = laneCosines * blockCosines - laneSines * blockSines;
            const auto nextSines = laneSines * blockCosines + laneCosines * blockSines;

            laneCosines = nextCosines;
            laneSines = nextSines;

            harmonic += laneCount;
        }

        auto value = accumulator.sum();

        for (auto remaining = activeHarmonics - harmonic + 1, lane = 0; remaining > 0; --remaining, ++lane)
        {
            const auto index = static_cast<std::size_t> (harmonic - 1 + lane);

            value += cosineCoefficients[index] * laneCosines[lane]
                   + sineCoefficients[index] * laneSines[lane];
        }

        if (includeDC)
            value += series.getDC();

        return value;
    }

    //==============================================================================
    FourierSeries<CoeffType> series;
    double sampleRate = 44100.0;
    double phase = 0.0;
    CoeffType frequency = static_cast<CoeffType> (440);
    int activeHarmonics = 0;
    bool includeDC = false;
};

//==============================================================================
/** @name Convenience type aliases */
using AdditiveOscillatorFloat = AdditiveOscillator<float>;   /**< float samples, double coefficients */
using AdditiveOscillatorDouble = AdditiveOscillator<double>; /**< double samples and coefficients */

} // namespace yup
