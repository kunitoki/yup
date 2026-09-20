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
    Synchronization flavors of the alias-free oscillator method.

    Each flavor corresponds to one of the three time-domain constructions in
    Roth, Keller, Castaneda and Studer, "Alias-Free Oscillator Synchronization
    via Additive Synthesis" (DAFx26).

    @see SyncSpectralResampler
*/
enum class SyncMode
{
    none,     /**< No synchronization, the follower is passed through untouched */
    hard,     /**< Hard sync: s (t) = r (t + P / 2) on [-P / 2, P / 2), period P */
    mirrored, /**< Mirrored sync: s (t) = r (P - |t|) on [-P, P), period 2 P */
    pulsar    /**< Pulsar sync: one follower period in a window of width 1, period P */
};

//==============================================================================
/**
    Rewrites the Fourier coefficients of a free-running follower into the ones of
    a synchronized oscillator.

    For a follower with period ratio P = T_lead / T_follow, the synchronized
    waveform is periodic again and its Fourier coefficients are a linear
    transform of the follower's: this class applies that transform (a "spectral
    resampling" of the follower's spectrum) so the result can be synthesized
    additively. Because only harmonics below Nyquist are synthesized, the output
    is alias-free by construction.

    Both the follower and the output are FourierSeries objects, so the caller
    controls the follower bandwidth and how many output harmonics are wanted
    (Remark 6 of the paper allows N_out != N_in). The transform cannot create
    harmonics that the follower does not have: bandlimiting the follower to N
    harmonics caps the spectrum of the synchronized waveform at N * P (Remark 13).

    The transform costs O (N_in * N_out) multiply-accumulates with O (N_in)
    transcendental calls, using FloatVectorOperations and, where helpful,
    SIMDRegister. It is allocation-free once prepare() has been called, so it can
    run on the audio thread, but a synthesizer normally calls it once per block
    instead of once per sample.

    The absolute phase of the output follows the paper's pre-rotation: relative to
    the raw time-domain definitions above, the output is the same waveform delayed
    by half a period of the output fundamental, i.e. its coefficients carry an
    extra factor (-1)^n.

    @tparam CoeffType  Precision used for the transform (default double).

    @see FourierSeries, SyncOscillator
*/
template <typename CoeffType = double>
class SyncSpectralResampler
{
public:
    //==============================================================================
    /** Creates an unprepared resampler. Call prepare() before transform(). */
    SyncSpectralResampler() = default;

    //==============================================================================
    /**
        Allocates the scratch storage for series of up to maxHarmonics harmonics.

        This is the only allocating method, so it must be called outside of the
        audio callback.
    */
    void prepare (int maxHarmonics)
    {
        jassert (maxHarmonics > 0);

        const auto count = static_cast<std::size_t> (jmax (1, maxHarmonics));

        firstWeight.assign (count, CoeffType (0));
        secondWeight.assign (count, CoeffType (0));
        argumentSquared.assign (count, CoeffType (0));
        triggerSine.assign (count, CoeffType (0));
        triggerCosine.assign (count, CoeffType (0));
        inverseArgument.assign (count, CoeffType (0));
        weight.assign (count, CoeffType (0));

        resonantIndices.reserve (count);
        resonantHarmonics.reserve (count);

        rotated.resize (jmax (1, maxHarmonics));
    }

    //==============================================================================
    /**
        Runs the synchronization transform.

        @param follower            Follower coefficients, resized to the follower bandwidth.
        @param periodRatio         P = T_lead / T_follow, must be positive.
        @param mode                Synchronization flavor to apply.
        @param output              Destination series, already sized to the wanted
                                   number of harmonics and no larger than the
                                   maxHarmonics given to prepare().
        @param numOutputHarmonics  Number of output harmonics to compute (-1 for
                                   all of them). The remaining ones are zeroed
                                   without reallocating, which lets a caller
                                   follow a changing Nyquist limit.
    */
    void transform (const FourierSeries<CoeffType>& follower,
                    CoeffType periodRatio,
                    SyncMode mode,
                    FourierSeries<CoeffType>& output,
                    int numOutputHarmonics = -1) noexcept
    {
        jassert (periodRatio > CoeffType (0));
        jassert (follower.getNumHarmonics() <= rotated.getNumHarmonics());
        jassert (output.getNumHarmonics() <= rotated.getNumHarmonics());

        if (mode == SyncMode::none)
        {
            output.copyFrom (follower);
            return;
        }

        const auto maxHarmonics = rotated.getNumHarmonics();
        if (maxHarmonics <= 0)
            return;

        const auto ratio = jmax (periodRatio, static_cast<CoeffType> (1e-9));
        const auto numOut = numOutputHarmonics < 0 ? output.getNumHarmonics()
                                                   : jlimit (0, output.getNumHarmonics(), numOutputHarmonics);
        const auto numFollower = jmin (follower.getNumHarmonics(), maxHarmonics);

        output.clear();

        rotated.copyFrom (follower);
        rotated.timeShift (getPreRotation (mode, ratio));

        if (numFollower <= 0)
            return;

        switch (mode)
        {
            case SyncMode::hard: transformHard (ratio, numFollower, output, numOut); break;
            case SyncMode::mirrored: transformMirrored (ratio, numFollower, output, numOut); break;
            case SyncMode::pulsar: transformPulsar (ratio, numFollower, output, numOut); break;
            case SyncMode::none: break;
        }
    }

    //==============================================================================
    /** Returns the ratio between the output fundamental and the leader frequency.

        Mirrored sync produces a waveform with period 2 * T_lead, so it sounds an
        octave below the leader; every other mode has the leader's period.
    */
    static constexpr CoeffType getFundamentalScale (SyncMode mode) noexcept
    {
        return mode == SyncMode::mirrored ? static_cast<CoeffType> (0.5) : static_cast<CoeffType> (1);
    }

    /** Returns the pre-rotation time shift applied to the follower, in follower periods. */
    static constexpr CoeffType getPreRotation (SyncMode mode, CoeffType periodRatio) noexcept
    {
        return mode == SyncMode::hard ? -periodRatio / static_cast<CoeffType> (2)
             : mode == SyncMode::mirrored ? -periodRatio
                                          : static_cast<CoeffType> (-0.5);
    }

    /** Returns how many output harmonics are needed to keep the follower's bandwidth.

        The follower's top harmonic sits at N * P times the leader frequency, and
        the output fundamental is the leader frequency scaled by
        getFundamentalScale(), hence ceil (N * P / scale). The caller is expected
        to clamp the result to its own harmonic budget, in which case the upper
        harmonics of the follower are truncated.
    */
    static int getRecommendedOutputHarmonics (int numFollowerHarmonics, CoeffType periodRatio, SyncMode mode) noexcept
    {
        const auto scale = static_cast<double> (getFundamentalScale (mode));
        const auto count = static_cast<double> (jmax (0, numFollowerHarmonics)) * static_cast<double> (periodRatio) / scale;

        return static_cast<int> (std::ceil (count));
    }

    /** Returns the tolerance used to detect the removable singularities of the transform. */
    static constexpr CoeffType getResonanceEpsilon() noexcept
    {
        return std::is_same_v<CoeffType, float> ? static_cast<CoeffType> (1e-3)
                                                : static_cast<CoeffType> (1e-6);
    }

private:
    //==============================================================================
    void transformHard (CoeffType ratio, int numFollower, FourierSeries<CoeffType>& output, int numOut) noexcept
    {
        const auto* followerCosine = rotated.getCosineCoefficients().data();
        const auto* followerSine = rotated.getSineCoefficients().data();
        const auto pi = MathConstants<CoeffType>::pi;

        resonantIndices.clear();
        resonantHarmonics.clear();

        for (int k = 1; k <= numFollower; ++k)
        {
            const auto index = static_cast<std::size_t> (k - 1);
            const auto argument = ratio * static_cast<CoeffType> (k);
            const auto trigger = std::sin (pi * argument);
            const auto rounded = std::round (argument);

            argumentSquared[index] = argument * argument;
            triggerSine[index] = trigger;
            firstWeight[index] = followerCosine[index] * trigger * argument;
            secondWeight[index] = followerSine[index] * trigger;
            inverseArgument[index] = CoeffType (1) / (pi * argument);

            if (std::abs (argument - rounded) < getResonanceEpsilon())
            {
                resonantIndices.push_back (static_cast<int> (index));
                resonantHarmonics.push_back (static_cast<int> (rounded));
            }
        }

        FloatVectorOperations::multiply (weight.data(), followerCosine, triggerSine.data(), numFollower);
        output.setDC (rotated.getDC() + FloatVectorOperations::dotProduct (weight.data(), inverseArgument.data(), numFollower));

        const auto numResonant = static_cast<int> (resonantIndices.size());

        for (int n = 1; n <= numOut; ++n)
        {
            const auto nSquared = static_cast<CoeffType> (n) * static_cast<CoeffType> (n);

            FloatVectorOperations::fill (weight.data(), nSquared, numFollower);
            FloatVectorOperations::subtract (weight.data(), argumentSquared.data(), numFollower);
            FloatVectorOperations::copyWithDividend (weight.data(), weight.data(), CoeffType (1), numFollower);

            for (int i = 0; i < numResonant; ++i)
                weight[static_cast<std::size_t> (resonantIndices[static_cast<std::size_t> (i)])] = CoeffType (0);

            const auto sign = (n % 2 == 0) ? static_cast<CoeffType> (-1) : static_cast<CoeffType> (1);

            auto a = sign * (CoeffType (2) / pi) * FloatVectorOperations::dotProduct (firstWeight.data(), weight.data(), numFollower);
            auto b = sign * (CoeffType (2 * n) / pi) * FloatVectorOperations::dotProduct (secondWeight.data(), weight.data(), numFollower);

            for (int i = 0; i < numResonant; ++i)
            {
                if (n != resonantHarmonics[static_cast<std::size_t> (i)])
                    continue;

                const auto index = static_cast<std::size_t> (resonantIndices[static_cast<std::size_t> (i)]);
                a += followerCosine[index];
                b += followerSine[index];
            }

            output.setHarmonic (n, a, b);
        }
    }

    //==============================================================================
    void transformMirrored (CoeffType ratio, int numFollower, FourierSeries<CoeffType>& output, int numOut) noexcept
    {
        const auto* followerCosine = rotated.getCosineCoefficients().data();
        const auto* followerSine = rotated.getSineCoefficients().data();
        const auto pi = MathConstants<CoeffType>::pi;

        resonantIndices.clear();
        resonantHarmonics.clear();

        for (int k = 1; k <= numFollower; ++k)
        {
            const auto index = static_cast<std::size_t> (k - 1);
            const auto argument = static_cast<CoeffType> (2) * ratio * static_cast<CoeffType> (k);
            const auto rounded = std::round (argument);

            argumentSquared[index] = argument * argument;
            triggerSine[index] = std::sin (pi * argument);
            triggerCosine[index] = std::cos (pi * argument);
            firstWeight[index] = followerSine[index] * argument;
            secondWeight[index] = -(followerCosine[index] * triggerSine[index] + followerSine[index] * triggerCosine[index]) * argument;
            inverseArgument[index] = CoeffType (1) / (pi * argument);
            weight[index] = followerCosine[index] * triggerSine[index] - followerSine[index] * (CoeffType (1) - triggerCosine[index]);

            if (std::abs (argument - rounded) < getResonanceEpsilon())
            {
                resonantIndices.push_back (static_cast<int> (index));
                resonantHarmonics.push_back (static_cast<int> (rounded));
            }
        }

        output.setDC (rotated.getDC() + FloatVectorOperations::dotProduct (weight.data(), inverseArgument.data(), numFollower));

        const auto numResonant = static_cast<int> (resonantIndices.size());

        for (int n = 1; n <= numOut; ++n)
        {
            const auto nSquared = static_cast<CoeffType> (n) * static_cast<CoeffType> (n);

            FloatVectorOperations::fill (weight.data(), nSquared, numFollower);
            FloatVectorOperations::subtract (weight.data(), argumentSquared.data(), numFollower);
            FloatVectorOperations::copyWithDividend (weight.data(), weight.data(), CoeffType (1), numFollower);

            for (int i = 0; i < numResonant; ++i)
                weight[static_cast<std::size_t> (resonantIndices[static_cast<std::size_t> (i)])] = CoeffType (0);

            const auto alternating = (n % 2 == 0) ? static_cast<CoeffType> (1) : static_cast<CoeffType> (-1);

            auto a = (CoeffType (2) / pi)
                   * (FloatVectorOperations::dotProduct (firstWeight.data(), weight.data(), numFollower)
                      + alternating * FloatVectorOperations::dotProduct (secondWeight.data(), weight.data(), numFollower));

            for (int i = 0; i < numResonant; ++i)
            {
                const auto harmonic = resonantHarmonics[static_cast<std::size_t> (i)];
                const auto index = static_cast<std::size_t> (resonantIndices[static_cast<std::size_t> (i)]);

                if (n == harmonic)
                {
                    a += followerCosine[index];
                }
                else if (((n + harmonic) % 2) != 0)
                {
                    // The mirrored weight vector has a removable pole at |n| = 2 k P whose
                    // numerator only vanishes in the n == harmonic case, so the contribution
                    // of a resonant harmonic has to be added back explicitly here.
                    const auto m = static_cast<CoeffType> (harmonic);

                    a += static_cast<CoeffType> (4) * m * followerSine[index] / (pi * (nSquared - m * m));
                }
            }

            output.setHarmonic (n, a, CoeffType (0));
        }
    }

    //==============================================================================
    void transformPulsar (CoeffType ratio, int numFollower, FourierSeries<CoeffType>& output, int numOut) noexcept
    {
        const auto* followerCosine = rotated.getCosineCoefficients().data();
        const auto* followerSine = rotated.getSineCoefficients().data();
        const auto pi = MathConstants<CoeffType>::pi;

        for (int k = 1; k <= numFollower; ++k)
        {
            const auto index = static_cast<std::size_t> (k - 1);
            const auto sign = (k % 2 == 0) ? static_cast<CoeffType> (1) : static_cast<CoeffType> (-1);

            argumentSquared[index] = static_cast<CoeffType> (k * k);
            firstWeight[index] = sign * followerCosine[index];
            secondWeight[index] = sign * static_cast<CoeffType> (k) * followerSine[index];
        }

        // The paper's pulsar transform drops the follower's DC term.
        output.setDC (CoeffType (0));

        for (int n = 1; n <= numOut; ++n)
        {
            const auto q = static_cast<CoeffType> (n) / ratio;
            const auto rounded = std::round (q);
            const auto isResonant = std::abs (q - rounded) < getResonanceEpsilon()
                                 && rounded >= CoeffType (1)
                                 && rounded <= static_cast<CoeffType> (numFollower);
            const auto resonantIndex = isResonant ? static_cast<std::size_t> (rounded) - 1 : 0;

            FloatVectorOperations::fill (weight.data(), q * q, numFollower);
            FloatVectorOperations::subtract (weight.data(), argumentSquared.data(), numFollower);
            FloatVectorOperations::copyWithDividend (weight.data(), weight.data(), CoeffType (1), numFollower);

            if (isResonant)
                weight[resonantIndex] = CoeffType (0);

            const auto sine = std::sin (pi * q);

            auto a = (CoeffType (2) * q * sine / (pi * ratio)) * FloatVectorOperations::dotProduct (firstWeight.data(), weight.data(), numFollower);
            auto b = (CoeffType (2) * sine / (pi * ratio)) * FloatVectorOperations::dotProduct (secondWeight.data(), weight.data(), numFollower);

            if (isResonant)
            {
                a += followerCosine[resonantIndex] / ratio;
                b += followerSine[resonantIndex] / ratio;
            }

            output.setHarmonic (n, a, b);
        }
    }

    //==============================================================================
    FourierSeries<CoeffType> rotated;
    std::vector<CoeffType> firstWeight;
    std::vector<CoeffType> secondWeight;
    std::vector<CoeffType> argumentSquared;
    std::vector<CoeffType> triggerSine;
    std::vector<CoeffType> triggerCosine;
    std::vector<CoeffType> inverseArgument;
    std::vector<CoeffType> weight;
    std::vector<int> resonantIndices;
    std::vector<int> resonantHarmonics;
};

} // namespace yup
