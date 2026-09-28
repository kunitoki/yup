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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

#include <functional>

using namespace yup;

//==============================================================================
class SyncSpectralResamplerTests : public ::testing::Test
{
protected:
    // Number of midpoint samples used to integrate the coefficients of one period
    // of a waveform. The constructions are made of at most a handful of harmonics,
    // so the integration error stays below 1e-7 except for the pulsar
    // discontinuity, which is below 1e-4.
    static constexpr int referenceSamples = 1 << 15;
    static constexpr int followerHarmonics = 8;

    static double evaluateSeries (const FourierSeries<double>& series, double t)
    {
        auto value = series.getDC();

        for (int n = 1; n <= series.getNumHarmonics(); ++n)
            value += series.getCosine (n) * std::cos (MathConstants<double>::twoPi * n * t)
                   + series.getSine (n) * std::sin (MathConstants<double>::twoPi * n * t);

        return value;
    }

    static double normalizedSinc (double x)
    {
        return x == 0.0 ? 1.0 : std::sin (MathConstants<double>::pi * x) / (MathConstants<double>::pi * x);
    }

    static double normalizedVersinc (double x)
    {
        return x == 0.0 ? 0.0 : (1.0 - std::cos (MathConstants<double>::pi * x)) / (MathConstants<double>::pi * x);
    }

    static FourierSeries<double> makeFollower (Waveform waveform = Waveform::sawtooth)
    {
        return FourierSeries<double>::create (waveform, followerHarmonics);
    }

    //==============================================================================
    /** Pre-rotation of the paper: the coefficients of r (t - shift). */
    struct RotatedCoefficients
    {
        std::vector<double> cosine;
        std::vector<double> sine;
    };

    static RotatedCoefficients preRotate (const FourierSeries<double>& follower, double shift)
    {
        const auto count = follower.getNumHarmonics();

        RotatedCoefficients rotated { std::vector<double> (static_cast<std::size_t> (count) + 1, 0.0),
                                      std::vector<double> (static_cast<std::size_t> (count) + 1, 0.0) };

        for (int n = 1; n <= count; ++n)
        {
            const auto angle = MathConstants<double>::twoPi * shift * n;
            const auto index = static_cast<std::size_t> (n);

            rotated.cosine[index] = follower.getCosine (n) * std::cos (angle) - follower.getSine (n) * std::sin (angle);
            rotated.sine[index] = follower.getCosine (n) * std::sin (angle) + follower.getSine (n) * std::cos (angle);
        }

        return rotated;
    }

    /**
        Independent scalar implementation of the paper's transform, written with the
        sinc and versinc kernels as published. It is the reference the vectorized,
        dot-product implementation is checked against, so it deliberately repeats the
        pre-rotation instead of calling into the class.
    */
    static FourierSeries<double> scalarReferenceTransform (const FourierSeries<double>& follower, double ratio, SyncMode mode, int numOutputHarmonics)
    {
        FourierSeries<double> output (numOutputHarmonics);

        if (mode == SyncMode::none)
        {
            output.copyFrom (follower);
            return output;
        }

        const auto shift = mode == SyncMode::hard ? -ratio / 2.0
                         : mode == SyncMode::mirrored ? -ratio
                                                      : -0.5;
        const auto rotated = preRotate (follower, shift);

        double dc = 0.0;

        if (mode == SyncMode::hard)
        {
            for (int k = 1; k <= follower.getNumHarmonics(); ++k)
                dc += rotated.cosine[static_cast<std::size_t> (k)] * normalizedSinc (k * ratio);
        }
        else if (mode == SyncMode::mirrored)
        {
            for (int k = 1; k <= follower.getNumHarmonics(); ++k)
                dc += rotated.cosine[static_cast<std::size_t> (k)] * normalizedSinc (2.0 * k * ratio)
                    - rotated.sine[static_cast<std::size_t> (k)] * normalizedVersinc (2.0 * k * ratio);
        }

        output.setDC (dc);

        for (int n = 1; n <= numOutputHarmonics; ++n)
        {
            double a = 0.0;
            double b = 0.0;

            for (int k = 1; k <= follower.getNumHarmonics(); ++k)
            {
                const auto index = static_cast<std::size_t> (k);
                const auto a_k = rotated.cosine[index];
                const auto b_k = rotated.sine[index];

                if (mode == SyncMode::hard)
                {
                    a += a_k * (normalizedSinc (n - k * ratio) + normalizedSinc (n + k * ratio));
                    b += b_k * (normalizedSinc (n - k * ratio) - normalizedSinc (n + k * ratio));
                }
                else if (mode == SyncMode::mirrored)
                {
                    a += a_k * (normalizedSinc (n - 2.0 * k * ratio) + normalizedSinc (n + 2.0 * k * ratio))
                       + b_k * (normalizedVersinc (n - 2.0 * k * ratio) - normalizedVersinc (n + 2.0 * k * ratio));
                }
                else
                {
                    const auto q = n / ratio;

                    a += (a_k / ratio) * (normalizedSinc (q - k) + normalizedSinc (q + k));
                    b += (b_k / ratio) * (normalizedSinc (q - k) - normalizedSinc (q + k));
                }
            }

            output.setHarmonic (n, a, b);
        }

        return output;
    }

    //==============================================================================
    /** One of the three time-domain constructions of the paper. */
    struct TimeDomainConstruction
    {
        std::function<double (double)> waveform;
        double start = 0.0;
        double period = 1.0;
    };

    static TimeDomainConstruction makeConstruction (SyncMode mode, double ratio, const FourierSeries<double>& follower)
    {
        const auto value = [&follower] (double t) { return evaluateSeries (follower, t); };

        if (mode == SyncMode::hard)
            return { [value, ratio] (double t) { return value (t + ratio / 2); }, -ratio / 2, ratio };

        if (mode == SyncMode::mirrored)
            return { [value, ratio] (double t) { return value (ratio - std::abs (t)); }, -ratio, 2 * ratio };

        return { [value] (double t) { return std::abs (t) < 0.5 ? value (t + 0.5) : 0.0; }, -ratio / 2, ratio };
    }

    /**
        Integrates the Fourier coefficients of one period of a time-domain construction.

        The integral is taken over the period the construction is defined on, and the
        result carries the (-1)^n factor of the paper's phase frame: relative to the
        raw construction the transform fixes the phase of the output at half an output
        period, which is where the sinc kernels come from.
    */
    static FourierSeries<double> integrateConstruction (const TimeDomainConstruction& construction, int numHarmonics)
    {
        std::vector<double> cosine (static_cast<std::size_t> (numHarmonics) + 1, 0.0);
        std::vector<double> sine (static_cast<std::size_t> (numHarmonics) + 1, 0.0);
        std::vector<double> phasorCosine (static_cast<std::size_t> (numHarmonics), 0.0);
        std::vector<double> phasorSine (static_cast<std::size_t> (numHarmonics), 0.0);

        const auto step = construction.period / referenceSamples;

        for (int m = 0; m < referenceSamples; ++m)
        {
            const auto t = construction.start + (m + 0.5) * step;
            const auto value = construction.waveform (t);

            cosine[0] += value;

            fillHarmonicPhasors (phasorCosine.data(), phasorSine.data(), numHarmonics, MathConstants<double>::twoPi * (t - construction.start) / construction.period);

            for (int n = 1; n <= numHarmonics; ++n)
            {
                const auto index = static_cast<std::size_t> (n - 1);

                cosine[static_cast<std::size_t> (n)] += value * phasorCosine[index];
                sine[static_cast<std::size_t> (n)] += value * phasorSine[index];
            }
        }

        FourierSeries<double> integrated (numHarmonics);

        integrated.setDC (cosine[0] / referenceSamples);

        for (int n = 1; n <= numHarmonics; ++n)
        {
            const auto index = static_cast<std::size_t> (n);
            const auto frame = (n % 2 == 0) ? 1.0 : -1.0;

            integrated.setHarmonic (n,
                                    frame * 2.0 * cosine[index] / referenceSamples,
                                    frame * 2.0 * sine[index] / referenceSamples);
        }

        return integrated;
    }

    static FourierSeries<double> runTransform (const FourierSeries<double>& follower, double ratio, SyncMode mode, int numOutputHarmonics)
    {
        SyncSpectralResampler<double> resampler;
        resampler.prepare (jmax (numOutputHarmonics, follower.getNumHarmonics()));

        FourierSeries<double> output (numOutputHarmonics);
        resampler.transform (follower, ratio, mode, output, numOutputHarmonics);

        return output;
    }

    static void expectMatchesTimeDomain (SyncMode mode, double ratio, const FourierSeries<double>& follower, int numOutputHarmonics, double tolerance)
    {
        const auto reference = integrateConstruction (makeConstruction (mode, ratio, follower), numOutputHarmonics);
        const auto transformed = runTransform (follower, ratio, mode, numOutputHarmonics);

        EXPECT_NEAR (reference.getDC(), transformed.getDC(), tolerance) << "mode " << (int) mode << " P " << ratio;

        for (int n = 1; n <= numOutputHarmonics; ++n)
        {
            EXPECT_NEAR (reference.getCosine (n), transformed.getCosine (n), tolerance) << "cosine " << n << " mode " << (int) mode << " P " << ratio;
            EXPECT_NEAR (reference.getSine (n), transformed.getSine (n), tolerance) << "sine " << n << " mode " << (int) mode << " P " << ratio;
        }
    }
};

//==============================================================================
TEST_F (SyncSpectralResamplerTests, NoneModePassesTheFollowerThrough)
{
    auto follower = makeFollower();
    follower.setDC (0.25);

    FourierSeries<double> output (32);
    output.setWaveform (Waveform::pulse);
    SyncSpectralResampler<double> resampler;
    resampler.prepare (64);

    resampler.transform (follower, 1.375, SyncMode::none, output, 32);

    EXPECT_EQ (32, output.getNumHarmonics());
    EXPECT_EQ (follower.getDC(), output.getDC());

    for (int n = 1; n <= follower.getNumHarmonics(); ++n)
    {
        EXPECT_EQ (follower.getCosine (n), output.getCosine (n));
        EXPECT_EQ (follower.getSine (n), output.getSine (n));
    }

    for (int n = follower.getNumHarmonics() + 1; n <= 32; ++n)
    {
        EXPECT_EQ (0.0, output.getCosine (n));
        EXPECT_EQ (0.0, output.getSine (n));
    }
}

TEST_F (SyncSpectralResamplerTests, HardSyncOfSineWithIntegerRatioRemapsTheHarmonic)
{
    const auto follower = makeFollower (Waveform::sine);

    // With P = 2 the hard sync of a sine repeats it every two follower periods,
    // which is the second harmonic of the leader frequency.
    const auto output = runTransform (follower, 2.0, SyncMode::hard, 16);

    EXPECT_NEAR (1.0, output.getSine (2), 1e-12);
    EXPECT_NEAR (0.0, output.getDC(), 1e-12);

    for (int n = 1; n <= 16; ++n)
    {
        if (n == 2)
            continue;

        EXPECT_NEAR (0.0, output.getCosine (n), 1e-12);
        EXPECT_NEAR (0.0, output.getSine (n), 1e-12);
    }
}

TEST_F (SyncSpectralResamplerTests, HardSyncOfSineWithFractionalRatioStaysFinite)
{
    for (const auto ratio : { 0.75, 1.375, 2.5, 1.5, 2.0 / 3.0 })
    {
        const auto output = runTransform (makeFollower (Waveform::sine), ratio, SyncMode::hard, 16);

        for (int n = 1; n <= 16; ++n)
        {
            EXPECT_TRUE (std::isfinite (output.getCosine (n)));
            EXPECT_TRUE (std::isfinite (output.getSine (n)));
        }
    }
}

TEST_F (SyncSpectralResamplerTests, MirroredOutputOnlyHasCosineCoefficients)
{
    const auto follower = makeFollower();

    for (const auto ratio : { 0.75, 1.375, 2.5, 1.5, 2.0 })
    {
        const auto output = runTransform (follower, ratio, SyncMode::mirrored, 24);

        for (int n = 1; n <= 24; ++n)
            EXPECT_EQ (0.0, output.getSine (n)) << "harmonic " << n << " ratio " << ratio;
    }
}

TEST_F (SyncSpectralResamplerTests, PulsarOutputHasNoDC)
{
    auto follower = makeFollower();

    follower.setDC (0.5);

    EXPECT_EQ (0.0, runTransform (follower, 1.375, SyncMode::pulsar, 16).getDC());
}

//==============================================================================
TEST_F (SyncSpectralResamplerTests, MatchesTheTimeDomainConstruction)
{
    const auto follower = makeFollower();

    for (const auto ratio : { 0.75, 1.375, 2.5 })
    {
        expectMatchesTimeDomain (SyncMode::hard, ratio, follower, 8, 1e-3);
        expectMatchesTimeDomain (SyncMode::mirrored, ratio, follower, 8, 1e-3);
    }

    // The pulsar construction needs P >= 1: below that the follower period is
    // shorter than the pulse width, a case the paper leaves unanalysed.
    for (const auto ratio : { 1.375, 2.5, 3.0 })
        expectMatchesTimeDomain (SyncMode::pulsar, ratio, follower, 8, 1e-3);
}

TEST_F (SyncSpectralResamplerTests, MatchesTheTimeDomainConstructionForMixedSpectra)
{
    FourierSeries<double> follower (followerHarmonics);

    follower.setDC (0.2);
    follower.setHarmonic (1, 0.7, 0.9);
    follower.setHarmonic (2, -0.3, 0.4);
    follower.setHarmonic (3, 0.15, -0.25);
    follower.setHarmonic (5, 0.0, 0.35);

    for (const auto ratio : { 1.375, 2.0 })
    {
        expectMatchesTimeDomain (SyncMode::hard, ratio, follower, 8, 1e-3);
        expectMatchesTimeDomain (SyncMode::mirrored, ratio, follower, 8, 1e-3);
    }

    // The pulsar transform drops the follower's DC term, so its construction has
    // to be DC free to be comparable.
    follower.setDC (0.0);

    for (const auto ratio : { 1.375, 2.0 })
        expectMatchesTimeDomain (SyncMode::pulsar, ratio, follower, 8, 1e-3);
}

TEST_F (SyncSpectralResamplerTests, MatchesTheTimeDomainConstructionAtResonantRatios)
{
    // These ratios put 2 k P or k P right on top of an output harmonic, which is
    // where the published kernels have removable singularities.
    const auto follower = makeFollower();

    for (const auto ratio : { 1.0, 2.0, 3.0, 1.5, 2.0 / 3.0 })
    {
        expectMatchesTimeDomain (SyncMode::hard, ratio, follower, 8, 1e-3);
        expectMatchesTimeDomain (SyncMode::mirrored, ratio, follower, 8, 1e-3);
    }

    for (const auto ratio : { 1.0, 2.0, 3.0, 1.5, 7.0 / 3.0 })
        expectMatchesTimeDomain (SyncMode::pulsar, ratio, follower, 8, 1e-3);
}

TEST_F (SyncSpectralResamplerTests, MatchesTheScalarSincReference)
{
    // Guards the vectorized dot-product rewrite against the published kernels, for
    // a ratio that does not touch any removable singularity.
    for (const auto mode : { SyncMode::hard, SyncMode::mirrored, SyncMode::pulsar })
    {
        for (const auto waveform : { Waveform::sawtooth, Waveform::square, Waveform::triangle })
        {
            const auto follower = makeFollower (waveform);
            const auto reference = scalarReferenceTransform (follower, 1.375, mode, 24);
            const auto transformed = runTransform (follower, 1.375, mode, 24);

            EXPECT_NEAR (reference.getDC(), transformed.getDC(), 1e-12);

            for (int n = 1; n <= 24; ++n)
            {
                EXPECT_NEAR (reference.getCosine (n), transformed.getCosine (n), 1e-12) << "cosine " << n << " mode " << (int) mode;
                EXPECT_NEAR (reference.getSine (n), transformed.getSine (n), 1e-12) << "sine " << n << " mode " << (int) mode;
            }
        }
    }
}

TEST_F (SyncSpectralResamplerTests, MatchesTheScalarSincReferenceAtResonances)
{
    for (const auto mode : { SyncMode::hard, SyncMode::mirrored, SyncMode::pulsar })
    {
        for (const auto ratio : { 1.0, 2.0, 1.5, 0.5, 2.0 / 3.0 })
        {
            const auto follower = makeFollower();
            const auto reference = scalarReferenceTransform (follower, ratio, mode, 24);
            const auto transformed = runTransform (follower, ratio, mode, 24);

            for (int n = 1; n <= 24; ++n)
            {
                EXPECT_NEAR (reference.getCosine (n), transformed.getCosine (n), 1e-9) << "cosine " << n << " mode " << (int) mode << " P " << ratio;
                EXPECT_NEAR (reference.getSine (n), transformed.getSine (n), 1e-9) << "sine " << n << " mode " << (int) mode << " P " << ratio;
            }
        }
    }
}

//==============================================================================
TEST_F (SyncSpectralResamplerTests, NumOutputHarmonicsZeroesTheTail)
{
    const auto follower = makeFollower();

    FourierSeries<double> output (32);
    SyncSpectralResampler<double> resampler;
    resampler.prepare (64);

    resampler.transform (follower, 1.375, SyncMode::hard, output, 8);

    for (int n = 9; n <= 32; ++n)
    {
        EXPECT_EQ (0.0, output.getCosine (n));
        EXPECT_EQ (0.0, output.getSine (n));
    }

    for (int n = 1; n <= 8; ++n)
        EXPECT_TRUE (std::isfinite (output.getSine (n)));
}

TEST_F (SyncSpectralResamplerTests, RecommendedOutputHarmonicsCoversTheFollowerBandwidth)
{
    EXPECT_EQ (22, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (16, 1.375, SyncMode::hard));
    EXPECT_EQ (22, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (16, 1.375, SyncMode::pulsar));
    EXPECT_EQ (44, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (16, 1.375, SyncMode::mirrored));
    EXPECT_EQ (64, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (16, 4.0, SyncMode::hard));
    EXPECT_EQ (16, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (16, 1.0, SyncMode::hard));
    EXPECT_EQ (0, SyncSpectralResampler<double>::getRecommendedOutputHarmonics (0, 1.0, SyncMode::hard));
}

TEST_F (SyncSpectralResamplerTests, FundamentalScaleIsHalvedForMirroring)
{
    EXPECT_EQ (1.0, SyncSpectralResampler<double>::getFundamentalScale (SyncMode::none));
    EXPECT_EQ (1.0, SyncSpectralResampler<double>::getFundamentalScale (SyncMode::hard));
    EXPECT_EQ (0.5, SyncSpectralResampler<double>::getFundamentalScale (SyncMode::mirrored));
    EXPECT_EQ (1.0, SyncSpectralResampler<double>::getFundamentalScale (SyncMode::pulsar));
}

//==============================================================================
TEST_F (SyncSpectralResamplerTests, FloatCoefficientsAgreeWithDouble)
{
    const auto followerDouble = makeFollower (Waveform::sawtooth);

    FourierSeries<float> followerFloat (followerHarmonics);

    for (int n = 1; n <= followerHarmonics; ++n)
        followerFloat.setHarmonic (n, static_cast<float> (followerDouble.getCosine (n)), static_cast<float> (followerDouble.getSine (n)));

    SyncSpectralResampler<float> resamplerFloat;
    resamplerFloat.prepare (32);

    FourierSeries<float> outputFloat (32);
    resamplerFloat.transform (followerFloat, 1.375f, SyncMode::mirrored, outputFloat, 24);

    const auto outputDouble = runTransform (followerDouble, 1.375, SyncMode::mirrored, 24);

    for (int n = 1; n <= 24; ++n)
    {
        EXPECT_NEAR (outputDouble.getCosine (n), outputFloat.getCosine (n), 1e-4) << "harmonic " << n;
        EXPECT_EQ (0.0f, outputFloat.getSine (n));
    }
}

TEST_F (SyncSpectralResamplerTests, LargeFollowerStaysFinite)
{
    const auto follower = FourierSeries<double>::create (Waveform::sawtooth, 512);

    const auto output = runTransform (follower, 1.375, SyncMode::hard, 512);

    for (int n = 1; n <= 512; ++n)
    {
        EXPECT_TRUE (std::isfinite (output.getCosine (n)));
        EXPECT_TRUE (std::isfinite (output.getSine (n)));
        EXPECT_LT (std::abs (output.getSine (n)), 10.0);
    }
}

TEST_F (SyncSpectralResamplerTests, RepeatedTransformsDoNotAccumulateState)
{
    const auto follower = makeFollower();

    FourierSeries<double> output (32);
    SyncSpectralResampler<double> resampler;
    resampler.prepare (64);

    resampler.transform (follower, 1.375, SyncMode::hard, output, 32);

    const auto first = output.getSine (3);

    for (int i = 0; i < 8; ++i)
        resampler.transform (follower, 1.375, SyncMode::hard, output, 32);

    EXPECT_EQ (first, output.getSine (3));
}

TEST_F (SyncSpectralResamplerTests, MatchesTheScalarReferenceForShortFollowers)
{
    for (const auto count : { 1, 3, 5, 13 })
    {
        FourierSeries<double> follower (count);
        for (int n = 1; n <= count; ++n)
            follower.setHarmonic (n, 0.3 / n, (n % 2 == 0 ? -0.7 : 0.7) / n);

        for (const auto mode : { SyncMode::hard, SyncMode::mirrored, SyncMode::pulsar })
        {
            for (const auto ratio : { 1.0, 1.375, 2.0 })
            {
                const auto expected = scalarReferenceTransform (follower, ratio, mode, 24);
                const auto actual = runTransform (follower, ratio, mode, 24);
                EXPECT_NEAR (expected.getDC(), actual.getDC(), 1e-12);
                for (int n = 1; n <= 24; ++n)
                {
                    EXPECT_NEAR (expected.getCosine (n), actual.getCosine (n), 1e-10);
                    EXPECT_NEAR (expected.getSine (n), actual.getSine (n), 1e-10);
                }
            }
        }
    }
}
