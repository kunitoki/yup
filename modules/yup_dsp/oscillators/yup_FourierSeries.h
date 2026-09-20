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
    Waveforms produced by the convenient series presets.

    @see FourierSeries::setWaveform
*/
enum class Waveform
{
    sine,     /**< Single sine harmonic, b1 = 1 */
    cosine,   /**< Single cosine harmonic, a1 = 1 */
    sawtooth, /**< Bandlimited sawtooth, b_n = -2 (-1)^n / (pi n) */
    square,   /**< Bandlimited square, b_n = 4 / (pi n) for odd n */
    triangle, /**< Bandlimited triangle, b_n = -8 (-1)^k / (pi n)^2 for n = 2k - 1 */
    pulse     /**< Bandlimited pulse, a_n = 1 / N */
};

//==============================================================================
/**
    Number of harmonics of a fundamental that fit strictly below the Nyquist
    frequency.

    A harmonic n is audible when n * frequency < sampleRate / 2, so the returned
    limit is ceil (sampleRate / (2 * frequency)) - 1, clamped to
    [0, maxHarmonics].

    @param frequency     Oscillator fundamental frequency in Hz.
    @param sampleRate    Sample rate in Hz.
    @param maxHarmonics  Number of harmonics the caller is able to synthesize.
*/
template <typename FloatType>
int getNyquistHarmonicLimit (FloatType frequency, double sampleRate, int maxHarmonics) noexcept
{
    if (maxHarmonics <= 0 || sampleRate <= 0.0)
        return 0;

    const auto fundamental = static_cast<double> (frequency);

    if (! (fundamental > 0.0))
        return maxHarmonics;

    const auto limit = sampleRate / (2.0 * fundamental);

    if (limit > static_cast<double> (maxHarmonics))
        return maxHarmonics;

    return jlimit (0, maxHarmonics, static_cast<int> (std::ceil (limit)) - 1);
}

//==============================================================================
/**
    Fourier series of a periodic waveform.

    The series stores the real coefficients of
    @code
    r (t) = dc + sum (n = 1..N) (cosine[n] * cos (2 pi n t) + sine[n] * sin (2 pi n t))
    @endcode
    where t runs over one period. It is the coefficient container shared by every
    oscillator in this folder: additive synthesis reads the coefficients
    directly, wavetable synthesis renders them with an inverse FFT, and the
    synchronizing transform rewrites them.

    Coefficients are stored contiguously (index 0 is harmonic 1) so the
    oscillator classes can load them with SIMD loads.

    @tparam CoeffType  Precision used for the coefficients (default double).

    @see AdditiveOscillator, WavetableOscillator, SyncSpectralResampler
*/
template <typename CoeffType = double>
class FourierSeries
{
public:
    //==============================================================================
    /** Creates an empty series with no harmonics. */
    FourierSeries() = default;

    /** Creates a series with a fixed number of harmonics, all zero. */
    explicit FourierSeries (int numHarmonics)
    {
        resize (numHarmonics);
    }

    /** Resizes the series, zero-filling every coefficient. */
    void resize (int numHarmonics)
    {
        jassert (numHarmonics >= 0);

        const auto count = static_cast<std::size_t> (jmax (0, numHarmonics));

        cosine.assign (count, CoeffType (0));
        sine.assign (count, CoeffType (0));
        phasorCosine.assign (count, CoeffType (0));
        phasorSine.assign (count, CoeffType (0));
        dc = CoeffType (0);
    }

    //==============================================================================
    /** Returns the number of stored harmonics. */
    int getNumHarmonics() const noexcept { return static_cast<int> (cosine.size()); }

    /** Sets every coefficient to zero, keeping the current size. */
    void clear() noexcept
    {
        dc = CoeffType (0);

        FloatVectorOperations::clear (cosine.data(), getNumHarmonics());
        FloatVectorOperations::clear (sine.data(), getNumHarmonics());
    }

    //==============================================================================
    /** Returns the DC (harmonic 0) coefficient. */
    CoeffType getDC() const noexcept { return dc; }

    /** Sets the DC (harmonic 0) coefficient. */
    void setDC (CoeffType newDC) noexcept { dc = newDC; }

    /** Returns the cosine coefficient of a 1-based harmonic. */
    CoeffType getCosine (int harmonic) const noexcept
    {
        jassert (isPositiveAndBelow (harmonic - 1, getNumHarmonics()));

        return cosine[static_cast<std::size_t> (harmonic - 1)];
    }

    /** Returns the sine coefficient of a 1-based harmonic. */
    CoeffType getSine (int harmonic) const noexcept
    {
        jassert (isPositiveAndBelow (harmonic - 1, getNumHarmonics()));

        return sine[static_cast<std::size_t> (harmonic - 1)];
    }

    /** Sets both coefficients of a 1-based harmonic. */
    void setHarmonic (int harmonic, CoeffType newCosine, CoeffType newSine) noexcept
    {
        jassert (isPositiveAndBelow (harmonic - 1, getNumHarmonics()));

        cosine[static_cast<std::size_t> (harmonic - 1)] = newCosine;
        sine[static_cast<std::size_t> (harmonic - 1)] = newSine;
    }

    /** Returns the magnitude of a 1-based harmonic. */
    CoeffType getMagnitude (int harmonic) const noexcept
    {
        const auto a = getCosine (harmonic);
        const auto b = getSine (harmonic);

        return std::sqrt (a * a + b * b);
    }

    /** Returns the contiguous cosine coefficients, index 0 being harmonic 1. */
    Span<const CoeffType> getCosineCoefficients() const noexcept
    {
        return { cosine.data(), cosine.size() };
    }

    /** Returns the contiguous sine coefficients, index 0 being harmonic 1. */
    Span<const CoeffType> getSineCoefficients() const noexcept
    {
        return { sine.data(), sine.size() };
    }

    //==============================================================================
    /**
        Fills the existing harmonics with one of the convenient presets.

        The number of harmonics is not changed, so resize() first to pick the
        bandwidth of the preset.
    */
    void setWaveform (Waveform waveform) noexcept
    {
        clear();

        const auto numHarmonics = getNumHarmonics();
        if (numHarmonics <= 0)
            return;

        const auto pi = MathConstants<CoeffType>::pi;

        switch (waveform)
        {
            case Waveform::sine:
                setHarmonic (1, CoeffType (0), CoeffType (1));
                break;

            case Waveform::cosine:
                setHarmonic (1, CoeffType (1), CoeffType (0));
                break;

            case Waveform::sawtooth:
                for (int n = 1; n <= numHarmonics; ++n)
                    sine[static_cast<std::size_t> (n - 1)] = static_cast<CoeffType> ((n % 2 == 0) ? -2 : 2) / (pi * static_cast<CoeffType> (n));
                break;

            case Waveform::square:
                for (int n = 1; n <= numHarmonics; n += 2)
                    sine[static_cast<std::size_t> (n - 1)] = static_cast<CoeffType> (4) / (pi * static_cast<CoeffType> (n));
                break;

            case Waveform::triangle:
                for (int k = 1; 2 * k - 1 <= numHarmonics; ++k)
                {
                    const auto n = 2 * k - 1;
                    const auto sign = (k % 2 == 0) ? static_cast<CoeffType> (-1) : static_cast<CoeffType> (1);

                    sine[static_cast<std::size_t> (n - 1)] = static_cast<CoeffType> (-8) * sign / (pi * pi * static_cast<CoeffType> (n * n));
                }
                break;

            case Waveform::pulse:
                for (int n = 1; n <= numHarmonics; ++n)
                    cosine[static_cast<std::size_t> (n - 1)] = CoeffType (1) / static_cast<CoeffType> (numHarmonics);
                break;
        }
    }

    //==============================================================================
    /**
        Analyses one cycle of a waveform into the existing harmonics.

        The cycle is expected to cover exactly one period of the waveform, sampled
        at evenly spaced points in [0, 1). This is an offline transform: it costs
        O (numSamples * numHarmonics) and is not meant to run on the audio thread,
        although it does not allocate.

        @param oneCycle  Evenly spaced samples of a single waveform period.
    */
    template <typename SampleType>
    void setFromCycle (Span<const SampleType> oneCycle) noexcept
    {
        clear();

        const auto numSamples = static_cast<int> (oneCycle.size());
        const auto numHarmonics = getNumHarmonics();

        if (numSamples <= 0 || numHarmonics <= 0)
            return;

        const auto measure = static_cast<CoeffType> (numSamples);
        CoeffType sum = CoeffType (0);

        for (int m = 0; m < numSamples; ++m)
            sum += static_cast<CoeffType> (oneCycle[static_cast<std::size_t> (m)]);

        dc = sum / measure;

        for (int n = 1; n <= numHarmonics; ++n)
        {
            const auto angle = MathConstants<CoeffType>::twoPi * static_cast<CoeffType> (n) / measure;
            const auto stepCosine = std::cos (angle);
            const auto stepSine = std::sin (angle);

            CoeffType phasorCosine = CoeffType (1);
            CoeffType phasorSine = CoeffType (0);
            CoeffType accumulatedCosine = CoeffType (0);
            CoeffType accumulatedSine = CoeffType (0);

            for (int m = 0; m < numSamples; ++m)
            {
                const auto sample = static_cast<CoeffType> (oneCycle[static_cast<std::size_t> (m)]);

                accumulatedCosine += sample * phasorCosine;
                accumulatedSine += sample * phasorSine;

                if ((m + 1) % harmonicPhasorReseedInterval == 0)
                {
                    const auto seedAngle = angle * static_cast<CoeffType> (m + 1);
                    phasorCosine = std::cos (seedAngle);
                    phasorSine = std::sin (seedAngle);
                }
                else
                {
                    const auto nextCosine = phasorCosine * stepCosine - phasorSine * stepSine;
                    const auto nextSine = phasorSine * stepCosine + phasorCosine * stepSine;

                    phasorCosine = nextCosine;
                    phasorSine = nextSine;
                }
            }

            cosine[static_cast<std::size_t> (n - 1)] = CoeffType (2) * accumulatedCosine / measure;
            sine[static_cast<std::size_t> (n - 1)] = CoeffType (2) * accumulatedSine / measure;
        }
    }

    //==============================================================================
    /**
        Rotates the phase of the whole series, so that it describes r (t - shift).

        Writing the series as r (t), the shifted series is r (t - normalizedShift),
        with the shift expressed as a fraction of the period. For a sine series
        this is the "pre-rotation" step of the alias-free synchronization method,
        where it moves the leader's period boundary onto the follower's zero phase.

        @param normalizedShift  Time shift in periods (negative values shift forward).
    */
    void timeShift (CoeffType normalizedShift) noexcept
    {
        const auto numHarmonics = getNumHarmonics();
        if (numHarmonics <= 0)
            return;

        fillHarmonicPhasors (phasorCosine.data(), phasorSine.data(), numHarmonics, MathConstants<CoeffType>::twoPi * normalizedShift);

        for (int n = 1; n <= numHarmonics; ++n)
        {
            const auto index = static_cast<std::size_t> (n - 1);
            const auto a = cosine[index];
            const auto b = sine[index];
            const auto c = phasorCosine[index];
            const auto s = phasorSine[index];

            cosine[index] = a * c - b * s;
            sine[index] = a * s + b * c;
        }
    }

    //==============================================================================
    /**
        Copies another series into this one, keeping the current storage.

        Harmonics beyond the source size are zeroed, so the copy never leaves
        stale coefficients behind and never allocates.
    */
    void copyFrom (const FourierSeries& other) noexcept
    {
        const auto numHarmonics = getNumHarmonics();
        const auto numToCopy = jmin (numHarmonics, other.getNumHarmonics());

        FloatVectorOperations::copy (cosine.data(), other.cosine.data(), numToCopy);
        FloatVectorOperations::copy (sine.data(), other.sine.data(), numToCopy);
        FloatVectorOperations::clear (cosine.data() + numToCopy, numHarmonics - numToCopy);
        FloatVectorOperations::clear (sine.data() + numToCopy, numHarmonics - numToCopy);

        dc = other.dc;
    }

    //==============================================================================
    /** Creates a series of a given size filled with one of the presets. */
    static FourierSeries create (Waveform waveform, int numHarmonics)
    {
        FourierSeries result (numHarmonics);
        result.setWaveform (waveform);

        return result;
    }

private:
    //==============================================================================
    CoeffType dc = CoeffType (0);
    std::vector<CoeffType> cosine;
    std::vector<CoeffType> sine;
    std::vector<CoeffType> phasorCosine;
    std::vector<CoeffType> phasorSine;
};

} // namespace yup
