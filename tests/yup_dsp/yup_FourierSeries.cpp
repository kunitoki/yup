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

using namespace yup;

//==============================================================================
class FourierSeriesTests : public ::testing::Test
{
protected:
    static double sawtoothCoefficient (int harmonic)
    {
        return -2.0 * (harmonic % 2 == 0 ? 1.0 : -1.0) / (MathConstants<double>::pi * harmonic);
    }

    static double triangleCoefficient (int harmonic)
    {
        const auto order = (harmonic + 1) / 2;
        const auto sign = (order % 2 == 0) ? -1.0 : 1.0;

        return -8.0 * sign / (MathConstants<double>::pi * MathConstants<double>::pi * harmonic * harmonic);
    }

    static constexpr int presetHarmonics = 16;

    FourierSeries<double> series { presetHarmonics };
};

TEST_F (FourierSeriesTests, ConstructorSizesAndZeroes)
{
    EXPECT_EQ (presetHarmonics, series.getNumHarmonics());
    EXPECT_EQ (0.0, series.getDC());

    for (int n = 1; n <= presetHarmonics; ++n)
    {
        EXPECT_EQ (0.0, series.getCosine (n));
        EXPECT_EQ (0.0, series.getSine (n));
    }
}

TEST_F (FourierSeriesTests, ResizeZeroFills)
{
    series.setHarmonic (1, 1.0, 2.0);
    series.setDC (0.5);

    series.resize (32);

    EXPECT_EQ (32, series.getNumHarmonics());
    EXPECT_EQ (0.0, series.getDC());
    EXPECT_EQ (0.0, series.getCosine (1));
    EXPECT_EQ (0.0, series.getSine (1));
}

TEST_F (FourierSeriesTests, SineAndCosinePresets)
{
    series.setWaveform (Waveform::sine);
    EXPECT_EQ (0.0, series.getCosine (1));
    EXPECT_EQ (1.0, series.getSine (1));

    for (int n = 2; n <= presetHarmonics; ++n)
        EXPECT_EQ (0.0, series.getSine (n));

    series.setWaveform (Waveform::cosine);
    EXPECT_EQ (1.0, series.getCosine (1));
    EXPECT_EQ (0.0, series.getSine (1));
}

TEST_F (FourierSeriesTests, SawtoothPresetMatchesAnalyticFormula)
{
    series.setWaveform (Waveform::sawtooth);

    for (int n = 1; n <= presetHarmonics; ++n)
    {
        EXPECT_NEAR (sawtoothCoefficient (n), series.getSine (n), 1e-15);
        EXPECT_EQ (0.0, series.getCosine (n));
    }
}

TEST_F (FourierSeriesTests, SquarePresetMatchesAnalyticFormula)
{
    series.setWaveform (Waveform::square);

    for (int n = 1; n <= presetHarmonics; ++n)
    {
        const auto expected = (n % 2 == 1) ? 4.0 / (MathConstants<double>::pi * n) : 0.0;

        EXPECT_NEAR (expected, series.getSine (n), 1e-15);
    }
}

TEST_F (FourierSeriesTests, TrianglePresetMatchesAnalyticFormula)
{
    series.setWaveform (Waveform::triangle);

    for (int n = 1; n <= presetHarmonics; ++n)
    {
        const auto expected = (n % 2 == 1) ? triangleCoefficient (n) : 0.0;

        EXPECT_NEAR (expected, series.getSine (n), 1e-15);
    }
}

TEST_F (FourierSeriesTests, PulsePresetMatchesAnalyticFormula)
{
    series.setWaveform (Waveform::pulse);

    for (int n = 1; n <= presetHarmonics; ++n)
        EXPECT_NEAR (1.0 / presetHarmonics, series.getCosine (n), 1e-15);
}

TEST_F (FourierSeriesTests, CreateReturnsSizedPresets)
{
    const auto created = FourierSeries<double>::create (Waveform::sine, 8);

    EXPECT_EQ (8, created.getNumHarmonics());
    EXPECT_EQ (1.0, created.getSine (1));
}

TEST_F (FourierSeriesTests, MagnitudeCombinesBothCoefficients)
{
    series.setHarmonic (3, 3.0, 4.0);

    EXPECT_NEAR (5.0, series.getMagnitude (3), 1e-15);
    EXPECT_EQ (0.0, series.getMagnitude (4));
}

TEST_F (FourierSeriesTests, CopyFromZeroesUnusedHarmonics)
{
    FourierSeries<double> destination (32);

    destination.setWaveform (Waveform::square);
    series.setWaveform (Waveform::sawtooth);
    destination.copyFrom (series);

    for (int n = 1; n <= presetHarmonics; ++n)
    {
        EXPECT_NEAR (sawtoothCoefficient (n), destination.getSine (n), 1e-15);
        EXPECT_EQ (0.0, destination.getCosine (n));
    }

    for (int n = presetHarmonics + 1; n <= 32; ++n)
        EXPECT_EQ (0.0, destination.getSine (n));

    EXPECT_EQ (32, destination.getNumHarmonics());
}

TEST_F (FourierSeriesTests, TimeShiftTurnsSineIntoCosine)
{
    series.resize (1);
    series.setWaveform (Waveform::sine);

    // r (t + 0.25) = sin (2 pi (t + 0.25)) = cos (2 pi t)
    series.timeShift (-0.25);

    EXPECT_NEAR (1.0, series.getCosine (1), 1e-15);
    EXPECT_NEAR (0.0, series.getSine (1), 1e-15);
}

TEST_F (FourierSeriesTests, TimeShiftByHalfPeriodInvertsTheSeries)
{
    series.resize (4);
    series.setWaveform (Waveform::sawtooth);

    const auto before = series.getSine (1);

    series.timeShift (0.5);

    EXPECT_NEAR (-before, series.getSine (1), 1e-14);
}

TEST_F (FourierSeriesTests, TimeShiftKeepsMagnitudesAndIsReversible)
{
    series.setWaveform (Waveform::sawtooth);

    const auto cosineBefore = series.getCosine (3);
    const auto sineBefore = series.getSine (3);

    series.timeShift (-0.125);
    EXPECT_NEAR (std::sqrt (cosineBefore * cosineBefore + sineBefore * sineBefore), series.getMagnitude (3), 1e-14);

    series.timeShift (0.125);
    EXPECT_NEAR (cosineBefore, series.getCosine (3), 1e-14);
    EXPECT_NEAR (sineBefore, series.getSine (3), 1e-14);
}

TEST_F (FourierSeriesTests, SetFromCycleRecoversSine)
{
    constexpr int numSamples = 64;

    std::vector<double> cycle (numSamples);

    for (int m = 0; m < numSamples; ++m)
        cycle[static_cast<std::size_t> (m)] = std::sin (MathConstants<double>::twoPi * m / numSamples);

    series.setFromCycle (Span<const double> (cycle.data(), cycle.size()));

    EXPECT_NEAR (1.0, series.getSine (1), 1e-12);
    EXPECT_NEAR (0.0, series.getCosine (1), 1e-12);
    EXPECT_NEAR (0.0, series.getDC(), 1e-12);

    for (int n = 2; n <= presetHarmonics; ++n)
    {
        EXPECT_NEAR (0.0, series.getCosine (n), 1e-12);
        EXPECT_NEAR (0.0, series.getSine (n), 1e-12);
    }
}

TEST_F (FourierSeriesTests, SetFromCycleRecoversDCAndFirstHarmonic)
{
    constexpr int numSamples = 32;

    std::vector<double> cycle (numSamples);

    for (int m = 0; m < numSamples; ++m)
    {
        const auto angle = MathConstants<double>::twoPi * m / numSamples;

        cycle[static_cast<std::size_t> (m)] = 0.25 + 0.5 * std::cos (angle) + 0.75 * std::sin (angle);
    }

    series.setFromCycle (Span<const double> (cycle.data(), cycle.size()));

    EXPECT_NEAR (0.25, series.getDC(), 1e-12);
    EXPECT_NEAR (0.5, series.getCosine (1), 1e-12);
    EXPECT_NEAR (0.75, series.getSine (1), 1e-12);
}

TEST_F (FourierSeriesTests, SetFromCycleHandlesFloatCycles)
{
    constexpr int numSamples = 16;

    std::vector<float> cycle (numSamples);

    for (int m = 0; m < numSamples; ++m)
        cycle[static_cast<std::size_t> (m)] = std::cos (static_cast<float> (MathConstants<double>::twoPi * m / numSamples));

    series.setFromCycle (Span<const float> (cycle.data(), cycle.size()));

    EXPECT_NEAR (1.0, series.getCosine (1), 1e-6);
    EXPECT_NEAR (0.0, series.getSine (1), 1e-6);
}

//==============================================================================
class NyquistHarmonicLimitTests : public ::testing::Test
{
};

TEST_F (NyquistHarmonicLimitTests, ExcludesTheExactNyquistBoundaryAtCapacity)
{
    EXPECT_EQ (0, getNyquistHarmonicLimit (24000.0, 48000.0, 1));
    EXPECT_EQ (7, getNyquistHarmonicLimit (3000.0, 48000.0, 8));
    EXPECT_EQ (8, getNyquistHarmonicLimit (2999.0, 48000.0, 8));
}

TEST_F (NyquistHarmonicLimitTests, FollowsSampleRateAndFrequency)
{
    EXPECT_EQ (2, getNyquistHarmonicLimit (10000.0, 48000.0, 512));
    EXPECT_EQ (0, getNyquistHarmonicLimit (30000.0, 48000.0, 512));
    EXPECT_EQ (23, getNyquistHarmonicLimit (1000.0, 48000.0, 512));
    EXPECT_EQ (7, getNyquistHarmonicLimit (1000.0, 48000.0, 7));
    EXPECT_EQ (0, getNyquistHarmonicLimit (1000.0, 48000.0, 0));
}

TEST_F (NyquistHarmonicLimitTests, UsesStrictlyBelowNyquistHarmonics)
{
    // Exactly 24 kHz at 48 kHz is Nyquist, so harmonic 23 is the last audible one.
    EXPECT_EQ (22, getNyquistHarmonicLimit (1000.0, 48000.0, 4096) - 1);
}

//==============================================================================
class HarmonicPhasorTests : public ::testing::Test
{
protected:
    static void compareWithTrigonometry (const std::vector<double>& cosines, const std::vector<double>& sines, double angle, double tolerance)
    {
        for (int k = 1; k <= static_cast<int> (cosines.size()); ++k)
        {
            const auto index = static_cast<std::size_t> (k - 1);

            EXPECT_NEAR (std::cos (angle * k), cosines[index], tolerance);
            EXPECT_NEAR (std::sin (angle * k), sines[index], tolerance);
        }
    }

    static void compareWithTrigonometry (const std::vector<float>& cosines, const std::vector<float>& sines, double angle, double tolerance)
    {
        for (int k = 1; k <= static_cast<int> (cosines.size()); ++k)
        {
            const auto index = static_cast<std::size_t> (k - 1);

            EXPECT_NEAR (std::cos (angle * k), cosines[index], tolerance);
            EXPECT_NEAR (std::sin (angle * k), sines[index], tolerance);
        }
    }

    static constexpr int count = 4096;
};

TEST_F (HarmonicPhasorTests, MatchesTrigonometryInDouble)
{
    constexpr double angle = 0.12217304763960307; // 7 degrees

    std::vector<double> cosines (count);
    std::vector<double> sines (count);

    fillHarmonicPhasors (cosines.data(), sines.data(), count, angle);

    compareWithTrigonometry (cosines, sines, angle, 1e-12);
}

TEST_F (HarmonicPhasorTests, MatchesTrigonometryAcrossReseedBoundaries)
{
    constexpr int reseededCount = 8 * harmonicPhasorReseedInterval;

    std::vector<double> cosines (reseededCount);
    std::vector<double> sines (reseededCount);

    const auto angle = MathConstants<double>::twoPi / reseededCount;

    fillHarmonicPhasors (cosines.data(), sines.data(), reseededCount, angle);

    compareWithTrigonometry (cosines, sines, angle, 1e-12);
}

TEST_F (HarmonicPhasorTests, MatchesTrigonometryInFloat)
{
    const auto angle = static_cast<float> (MathConstants<double>::twoPi / count);

    std::vector<float> cosines (count);
    std::vector<float> sines (count);

    fillHarmonicPhasors (cosines.data(), sines.data(), count, angle);

    compareWithTrigonometry (cosines, sines, static_cast<double> (angle), 2e-5);
}

TEST_F (HarmonicPhasorTests, IgnoresEmptyRanges)
{
    std::vector<double> cosines (1, -1.0);
    std::vector<double> sines (1, -1.0);

    fillHarmonicPhasors (cosines.data(), sines.data(), 0, 1.0);

    EXPECT_EQ (-1.0, cosines[0]);
    EXPECT_EQ (-1.0, sines[0]);
}
