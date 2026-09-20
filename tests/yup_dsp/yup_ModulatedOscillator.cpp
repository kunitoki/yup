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

class ModulatedOscillatorTests : public ::testing::Test
{
protected:
    using Oscillator = ModulatedOscillator<double>;
    std::array<FourierSeries<double>, 2> frames {
        FourierSeries<double>::create (Waveform::sine, 16),
        FourierSeries<double>::create (Waveform::cosine, 16)
    };
    WaveformBank<double> bank;
    static constexpr double sampleRate = 48000.0;

    void SetUp() override { bank.prepare ({ frames.data(), frames.size() }); }

    static Oscillator::Parameters modulation (int index)
    {
        const auto t = index / (4.0 * sampleRate);
        Oscillator::Parameters p;
        p.frequency = 1100.0;
        p.linearFM = 1700.0 * std::sin (MathConstants<double>::twoPi * 137.0 * t);
        p.phaseModulation = 0.1 * std::sin (MathConstants<double>::twoPi * 251.0 * t);
        p.morph = 0.5 + 0.5 * std::sin (MathConstants<double>::twoPi * 89.0 * t);
        p.phaseDistortion = 0.5 + 0.3 * std::sin (MathConstants<double>::twoPi * 73.0 * t);
        p.syncFrequency = 731.0;
        return p;
    }
};

TEST_F (ModulatedOscillatorTests, SignedFrequencyMatchesAnalyticSineAfterLatency)
{
    for (const auto frequency : { -1000.0, 0.0, 1000.0 })
    {
        Oscillator oscillator;
        oscillator.prepare (sampleRate, 512, bank);
        Oscillator::Parameters p;
        p.frequency = frequency;
        std::array<double, 512> output {};
        ASSERT_TRUE (oscillator.processBlock (output.data(), 512, p));

        for (int i = 64; i < 512; ++i)
            EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * frequency * (i - oscillator.getLatencyInSamples()) / sampleRate),
                         output[static_cast<std::size_t> (i)], 0.002);
    }
}

TEST_F (ModulatedOscillatorTests, PhaseModulationDoesNotChangeTheAccumulator)
{
    Oscillator oscillator;
    oscillator.prepare (sampleRate, 64, bank);
    Oscillator::Parameters p;
    p.frequency = 0.0;
    p.phaseModulation = 0.25;
    std::array<double, 64> output {};
    ASSERT_TRUE (oscillator.processBlock (output.data(), 64, p));
    EXPECT_EQ (0.0, oscillator.getPhase());
    EXPECT_NEAR (1.0, output.back(), 1e-5);
}

TEST_F (ModulatedOscillatorTests, FractionalSyncKeepsThePostResetRemainder)
{
    Oscillator oscillator;
    oscillator.prepare (sampleRate, 100, bank);
    Oscillator::Parameters p;
    p.frequency = 1100.0;
    p.syncFrequency = 731.0;
    std::array<double, 100> output {};
    ASSERT_TRUE (oscillator.processBlock (output.data(), 100, p));

    const auto leaderCycles = 100.0 * p.syncFrequency / sampleRate;
    const auto expected = (leaderCycles - std::floor (leaderCycles)) * p.frequency / p.syncFrequency;
    EXPECT_NEAR (expected - std::floor (expected), oscillator.getPhase(), 1e-12);
}

TEST_F (ModulatedOscillatorTests, ModulationIsIndependentOfBlockPartition)
{
    Oscillator whole;
    Oscillator split;
    whole.prepare (sampleRate, 256, bank);
    split.prepare (sampleRate, 256, bank);
    std::array<double, 256> expected {};
    std::array<double, 256> actual {};
    ASSERT_TRUE (whole.processModulatedBlock (expected.data(), 256, modulation));

    int offset = 0;
    for (const auto size : { 1, 3, 17, 64, 171 })
    {
        ASSERT_TRUE (split.processModulatedBlock (actual.data() + offset, size,
                                                  [offset] (int index) { return modulation (offset * 4 + index); }));
        offset += size;
    }

    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        EXPECT_TRUE (std::isfinite (actual[i]));
        EXPECT_NEAR (expected[i], actual[i], 1e-12);
    }
}

TEST_F (ModulatedOscillatorTests, ResetReproducesTheSameModulatedOutput)
{
    Oscillator oscillator;
    oscillator.prepare (sampleRate, 256, bank);
    std::array<double, 256> first {};
    std::array<double, 256> second {};
    oscillator.processModulatedBlock (first.data(), 256, modulation);
    oscillator.reset();
    oscillator.processModulatedBlock (second.data(), 256, modulation);
    EXPECT_EQ (first, second);
}

TEST_F (ModulatedOscillatorTests, RejectsInvalidBlocksWithoutAdvancingState)
{
    Oscillator oscillator;
    oscillator.prepare (sampleRate, 16, bank);
    std::array<double, 17> output {};
    Oscillator::Parameters p;
    EXPECT_FALSE (oscillator.processBlock (output.data(), 17, p));
    EXPECT_FALSE (oscillator.processBlock (output.data(), 0, p));
    EXPECT_FALSE (oscillator.processBlock (nullptr, 16, p));
    EXPECT_EQ (0.0, oscillator.getPhase());
}

TEST_F (ModulatedOscillatorTests, FloatCoefficientsAndOutputRemainFiniteAtExtremeControls)
{
    std::array<FourierSeries<float>, 1> source { FourierSeries<float>::create (Waveform::sawtooth, 32) };
    WaveformBank<float, float> floatBank;
    floatBank.prepare ({ source.data(), source.size() });
    ModulatedOscillator<float, 2, 8, float> oscillator;
    oscillator.prepare (sampleRate, 128, floatBank);
    std::array<float, 128> output {};
    for (const auto breakpoint : { 0.0, 0.5, 1.0 })
    {
        decltype (oscillator)::Parameters p;
        p.frequency = -30000.0;
        p.exponentialFM = 2.0;
        p.phaseDistortion = breakpoint;
        p.syncFrequency = 30000.0;
        ASSERT_TRUE (oscillator.processBlock (output.data(), 128, p));
        for (const auto value : output)
            EXPECT_TRUE (std::isfinite (value));
    }
}

TEST_F (ModulatedOscillatorTests, SyncAndPhaseDistortionApproachAnIndependentHighRateReference)
{
    constexpr int count = 1024;
    constexpr int radius = 32;
    constexpr double carrier = 3000.0;
    constexpr double leader = 1700.0;
    constexpr double breakpoint = 0.2;
    constexpr double morph = 0.3;

    ModulatedOscillator<double, 4, radius> oscillator;
    oscillator.prepare (sampleRate, count, bank);
    decltype (oscillator)::Parameters p;
    p.frequency = carrier;
    p.syncFrequency = leader;
    p.phaseDistortion = breakpoint;
    p.morph = morph;

    std::array<double, count> actual {};
    ASSERT_TRUE (oscillator.processBlock (actual.data(), count, p));

    const auto continuousWaveform = [] (double time)
    {
        auto phase = carrier * (time - std::floor (time * leader) / leader);
        phase -= std::floor (phase);
        const auto distorted = phase < breakpoint ? 0.5 * phase / breakpoint
                                                  : 0.5 + 0.5 * (phase - breakpoint) / (1.0 - breakpoint);
        const auto angle = MathConstants<double>::twoPi * distorted;
        return (1.0 - morph) * std::sin (angle) + morph * std::cos (angle);
    };

    const auto renderReference = [&]<int factor>()
    {
        Oversampler<double, factor, radius> decimator;
        decimator.prepare (sampleRate, 1, count);
        decimator.beginGeneration (1, count);
        auto* internal = decimator.getOversampledChannelData (0);
        for (int i = 0; i < count * factor; ++i)
            internal[i] = continuousWaveform (i / (sampleRate * factor));

        std::array<double, count> result {};
        double* channels[] = { result.data() };
        decimator.downsample (channels, 1, count);
        return result;
    };

    const auto reference = renderReference.template operator()<64>();
    const auto coarserReference = renderReference.template operator()<32>();
    double convergenceError = 0.0;

    double correctedError = 0.0;
    double naiveError = 0.0;
    for (int i = 2 * radius; i < count; ++i)
    {
        const auto expected = reference[static_cast<std::size_t> (i)];
        const auto convergence = coarserReference[static_cast<std::size_t> (i)] - expected;
        convergenceError += convergence * convergence;
        const auto corrected = actual[static_cast<std::size_t> (i)] - expected;
        const auto naive = continuousWaveform ((i - radius) / sampleRate) - expected;
        correctedError += corrected * corrected;
        naiveError += naive * naive;
    }

    EXPECT_LT (std::sqrt (convergenceError / (count - 2 * radius)), 0.01);
    EXPECT_LT (std::sqrt (correctedError / (count - 2 * radius)), 0.04);
    EXPECT_LT (correctedError, naiveError);
}

TEST_F (ModulatedOscillatorTests, AudioRatePhaseModulationMatchesAnalyticReference)
{
    constexpr int count = 1024;
    Oscillator oscillator;
    oscillator.prepare (sampleRate, count, bank);
    std::array<double, count> output {};
    ASSERT_TRUE (oscillator.processModulatedBlock (output.data(), count, [] (int i)
    {
        Oscillator::Parameters p;
        p.frequency = 2000.0;
        p.phaseModulation = 0.1 * std::sin (MathConstants<double>::twoPi * 1000.0 * i / (sampleRate * 4));
        return p;
    }));

    for (int i = 64; i < count; ++i)
    {
        const auto time = (i - oscillator.getLatencyInSamples()) / sampleRate;
        const auto phase = 2000.0 * time + 0.1 * std::sin (MathConstants<double>::twoPi * 1000.0 * time);
        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * phase), output[static_cast<std::size_t> (i)], 0.003);
    }
}

TEST_F (ModulatedOscillatorTests, ThroughZeroFMApproachesTheIntegratedFrequencyReference)
{
    constexpr int count = 2048;
    Oscillator oscillator;
    oscillator.prepare (sampleRate, count, bank);
    std::array<double, count> output {};
    ASSERT_TRUE (oscillator.processModulatedBlock (output.data(), count, [] (int i)
    {
        Oscillator::Parameters p;
        p.frequency = 200.0;
        p.linearFM = 400.0 * std::sin (MathConstants<double>::twoPi * 137.0 * i / (sampleRate * 4));
        return p;
    }));

    for (int i = 64; i < count; ++i)
    {
        const auto time = (i - oscillator.getLatencyInSamples()) / sampleRate;
        const auto phase = 200.0 * time + 400.0 * (1.0 - std::cos (MathConstants<double>::twoPi * 137.0 * time))
                                                   / (MathConstants<double>::twoPi * 137.0);
        EXPECT_NEAR (std::sin (MathConstants<double>::twoPi * phase), output[static_cast<std::size_t> (i)], 0.01);
    }
}
