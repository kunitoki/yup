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
class AdditiveOscillatorTests : public ::testing::Test
{
protected:
    /** Scalar evaluation of the harmonics the oscillator is expected to synthesize. */
    static double harmonicReference (const FourierSeries<double>& series, int numHarmonics, double frequency, double phase)
    {
        double value = 0.0;

        for (int n = 1; n <= numHarmonics; ++n)
        {
            if (n * frequency >= testSampleRate / 2)
                break;

            const auto angle = MathConstants<double>::twoPi * n * phase;

            value += series.getCosine (n) * std::cos (angle)
                   + series.getSine (n) * std::sin (angle);
        }

        return value;
    }

    static AdditiveOscillator<double> makeOscillator (int numHarmonics, Waveform waveform, double frequency)
    {
        AdditiveOscillator<double> oscillator;

        oscillator.prepare (testSampleRate, numHarmonics);
        oscillator.setWaveform (waveform);
        oscillator.setFrequency (frequency);

        return oscillator;
    }

    static constexpr double testSampleRate = 48000.0;
};

TEST_F (AdditiveOscillatorTests, SineMatchesAnalyticSine)
{
    auto oscillator = makeOscillator (64, Waveform::sine, 1000.0);

    const auto increment = 1000.0 / testSampleRate;

    for (int i = 0; i < 480; ++i)
        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * increment * i), oscillator.processSample(), 1e-9) << i;
}

TEST_F (AdditiveOscillatorTests, HarmonicLimitFollowsNyquist)
{
    auto oscillator = makeOscillator (512, Waveform::sawtooth, 10000.0);

    EXPECT_EQ (2, oscillator.getNumActiveHarmonics());

    oscillator.setFrequency (30000.0);
    EXPECT_EQ (0, oscillator.getNumActiveHarmonics());
    EXPECT_EQ (0.0, oscillator.processSample());

    oscillator.setFrequency (1000.0);
    EXPECT_EQ (23, oscillator.getNumActiveHarmonics());

    oscillator.setFrequency (1.0e-6);
    EXPECT_EQ (512, oscillator.getNumActiveHarmonics());
}

TEST_F (AdditiveOscillatorTests, BlockMatchesSampleBySample)
{
    auto blockOscillator = makeOscillator (32, Waveform::sawtooth, 1500.0);
    auto sampleOscillator = makeOscillator (32, Waveform::sawtooth, 1500.0);

    std::vector<double> buffer (256);

    blockOscillator.processBlock (buffer.data(), 256);

    for (int i = 0; i < 256; ++i)
        EXPECT_EQ (buffer[static_cast<std::size_t> (i)], sampleOscillator.processSample()) << i;
}

TEST_F (AdditiveOscillatorTests, PhaseWrapsAndCanBeSet)
{
    auto oscillator = makeOscillator (8, Waveform::sine, 1000.0);

    oscillator.setPhase (-0.25);
    EXPECT_NEAR (0.75, oscillator.getPhase(), 1e-15);

    oscillator.setPhase (1.25);
    EXPECT_NEAR (0.25, oscillator.getPhase(), 1e-15);

    auto reference = makeOscillator (8, Waveform::sine, 1000.0);

    reference.setPhase (0.25);

    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR (reference.processSample(), oscillator.processSample(), 1e-15) << i;

    EXPECT_TRUE (oscillator.getPhase() >= 0.0 && oscillator.getPhase() < 1.0);
}

TEST_F (AdditiveOscillatorTests, SimdLanesMatchScalarReference)
{
    for (const auto numHarmonics : { 13, 37 })
    {
        const auto series = FourierSeries<double>::create (Waveform::sawtooth, numHarmonics);
        auto oscillator = makeOscillator (numHarmonics, Waveform::sawtooth, 1000.0);

        for (int i = 0; i < 64; ++i)
        {
            const auto phase = std::fmod (1000.0 / testSampleRate * i, 1.0);

            EXPECT_NEAR (harmonicReference (series, numHarmonics, 1000.0, phase), oscillator.processSample(), 1e-9) << i;
        }
    }
}

TEST_F (AdditiveOscillatorTests, ManyHarmonicsStayAccurate)
{
    constexpr int numHarmonics = 512;

    const auto series = FourierSeries<double>::create (Waveform::sawtooth, numHarmonics);
    auto oscillator = makeOscillator (numHarmonics, Waveform::sawtooth, 50.0);

    EXPECT_EQ (479, oscillator.getNumActiveHarmonics());

    for (int i = 0; i < 32; ++i)
    {
        const auto phase = std::fmod (50.0 / testSampleRate * i, 1.0);

        EXPECT_NEAR (harmonicReference (series, numHarmonics, 50.0, phase), oscillator.processSample(), 1e-9) << i;
    }
}

TEST_F (AdditiveOscillatorTests, DCCoefficientIsOptional)
{
    FourierSeries<double> series (8);

    series.setDC (0.5);

    AdditiveOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 8);
    oscillator.setSeries (series);
    oscillator.setFrequency (0.0);

    EXPECT_EQ (0.0, oscillator.processSample());

    oscillator.setIncludeDC (true);
    EXPECT_EQ (0.5, oscillator.processSample());
}

TEST_F (AdditiveOscillatorTests, SeriesChangesTakeEffectImmediately)
{
    const auto sawtooth = FourierSeries<double>::create (Waveform::sawtooth, 16);
    const auto square = FourierSeries<double>::create (Waveform::square, 16);

    AdditiveOscillator<double> oscillator;
    oscillator.prepare (testSampleRate, 16);
    oscillator.setSeries (sawtooth);
    oscillator.setFrequency (500.0);
    oscillator.setPhase (0.25);

    const auto sawtoothSample = oscillator.processSample();

    oscillator.setPhase (0.25);
    oscillator.setSeries (square);

    const auto squareSample = oscillator.processSample();

    EXPECT_NEAR (harmonicReference (sawtooth, 16, 500.0, 0.25), sawtoothSample, 1e-12);
    EXPECT_NEAR (harmonicReference (square, 16, 500.0, 0.25), squareSample, 1e-12);
    EXPECT_NE (sawtoothSample, squareSample);
}

TEST_F (AdditiveOscillatorTests, FloatInstantiationBehavesLikeDouble)
{
    AdditiveOscillator<float> oscillator;

    oscillator.prepare (testSampleRate, 32);
    oscillator.setWaveform (Waveform::sawtooth);
    oscillator.setFrequency (1000.0);

    const auto series = FourierSeries<float>::create (Waveform::sawtooth, 32);

    for (int i = 0; i < 64; ++i)
    {
        double expected = 0.0;

        for (int n = 1; n <= 32; ++n)
        {
            if (n * 1000.0 >= testSampleRate / 2)
                break;

            const auto angle = MathConstants<float>::twoPi * n * static_cast<float> (std::fmod (1000.0 / testSampleRate * i, 1.0));

            expected += series.getCosine (n) * std::cos (angle) + series.getSine (n) * std::sin (angle);
        }

        EXPECT_NEAR (expected, oscillator.processSample(), 1e-3) << i;
    }
}

TEST_F (AdditiveOscillatorTests, FrequencyAndPhaseChangesRefreshThePhasorRecurrence)
{
    auto oscillator = makeOscillator (13, Waveform::sawtooth, 440.0);
    for (int i = 0; i < 4096; ++i)
    {
        if (i % 127 == 0)
            oscillator.setFrequency (220.0 + (i % 7) * 113.0);
        if (i % 191 == 0)
            oscillator.setPhase (0.173);

        const auto expected = harmonicReference (oscillator.getSeries(), 13, oscillator.getFrequency(), oscillator.getPhase());
        EXPECT_NEAR (expected, oscillator.processSample(), 1e-10);
    }
}

TEST_F (AdditiveOscillatorTests, DCSurvivesWhenNoHarmonicFitsBelowNyquist)
{
    auto oscillator = makeOscillator (1, Waveform::sine, 24000.0);
    FourierSeries<double> series (1);
    series.setDC (0.25);
    oscillator.setSeries (series);
    oscillator.setIncludeDC (true);
    EXPECT_EQ (0, oscillator.getNumActiveHarmonics());
    EXPECT_EQ (0.25, oscillator.processSample());
}
