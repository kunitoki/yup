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

#include <vector>

using namespace yup;

//==============================================================================
class LFOTests : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 48000.0;

    /** 3 kHz at 48 kHz is a 16-sample period with an exactly representable increment. */
    static constexpr double frequency = 3000.0;
    static constexpr int period = 16;

    void SetUp() override
    {
        lfo.prepare (sampleRate);
        lfo.setFrequency (frequency);
    }

    LFO<double> lfo;
};

//==============================================================================
TEST_F (LFOTests, SineStartsAtZeroAndPeaksAtQuarterPeriod)
{
    lfo.setShape (LFO<double>::Shape::sine);

    EXPECT_NEAR (0.0, lfo.getValue(), 1e-12);
    EXPECT_NEAR (1.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (-1.0, lfo.skip (period / 4), 1e-12);
}

TEST_F (LFOTests, PeriodMatchesFrequency)
{
    lfo.setShape (LFO<double>::Shape::sawtooth);

    lfo.skip (period * 10);

    EXPECT_NEAR (0.0, lfo.getPhase(), 1e-12);
    EXPECT_NEAR (-1.0, lfo.getValue(), 1e-12);
}

TEST_F (LFOTests, TriangleIsBipolarAndSymmetric)
{
    lfo.setShape (LFO<double>::Shape::triangle);

    EXPECT_NEAR (-1.0, lfo.getValue(), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (1.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 4), 1e-12);
}

TEST_F (LFOTests, SquareIsHighForFirstHalf)
{
    lfo.setShape (LFO<double>::Shape::square);

    EXPECT_DOUBLE_EQ (1.0, lfo.getValue());
    EXPECT_DOUBLE_EQ (1.0, lfo.skip (period / 2 - 1));
    EXPECT_DOUBLE_EQ (-1.0, lfo.skip (1));
    EXPECT_DOUBLE_EQ (-1.0, lfo.skip (period / 2 - 1));
    EXPECT_DOUBLE_EQ (1.0, lfo.skip (1));
}

TEST_F (LFOTests, SampleAndHoldHoldsWithinACycleAndChangesAcrossWraps)
{
    lfo.setShape (LFO<double>::Shape::sampleAndHold);

    const auto first = lfo.getValue();

    EXPECT_GE (first, -1.0);
    EXPECT_LE (first, 1.0);
    EXPECT_DOUBLE_EQ (first, lfo.skip (5));
    EXPECT_DOUBLE_EQ (first, lfo.skip (5));

    const auto second = lfo.skip (period);

    EXPECT_NE (first, second);
    EXPECT_DOUBLE_EQ (second, lfo.skip (3));
}

TEST_F (LFOTests, SampleAndHoldIsDeterministicPerSeed)
{
    LFO<double> a, b;

    a.prepare (sampleRate);
    b.prepare (sampleRate);
    a.setShape (LFO<double>::Shape::sampleAndHold);
    b.setShape (LFO<double>::Shape::sampleAndHold);
    a.setFrequency (frequency);
    b.setFrequency (frequency);
    a.setSeed (7u);
    b.setSeed (7u);

    EXPECT_DOUBLE_EQ (a.getValue(), b.getValue());

    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ (a.skip (period), b.skip (period));
}

TEST_F (LFOTests, PhaseOffsetShiftsTheWaveform)
{
    lfo.setShape (LFO<double>::Shape::sine);
    lfo.setPhaseOffset (0.25);

    EXPECT_NEAR (1.0, lfo.getValue(), 1e-12);
    EXPECT_NEAR (0.0, lfo.getPhase(), 1e-12);
    EXPECT_NEAR (0.25, lfo.getPhaseOffset(), 1e-12);
}

TEST_F (LFOTests, SkipEqualsRepeatedProcessSample)
{
    LFO<double> stepped;
    stepped.prepare (sampleRate);
    stepped.setFrequency (frequency);
    stepped.setShape (LFO<double>::Shape::triangle);
    lfo.setShape (LFO<double>::Shape::triangle);

    for (int i = 0; i < 333; ++i)
        stepped.processSample();

    EXPECT_NEAR (stepped.getValue(), lfo.skip (333), 1e-9);
}

TEST_F (LFOTests, ProcessBlockWritesConsecutiveSamples)
{
    LFO<double> stepped;
    stepped.prepare (sampleRate);
    stepped.setFrequency (frequency);

    std::vector<double> block (64);
    lfo.processBlock (block.data(), 64);

    for (auto value : block)
        EXPECT_NEAR (stepped.processSample(), value, 1e-12);
}

TEST_F (LFOTests, ResetRestartsThePhase)
{
    lfo.skip (77);
    lfo.reset (0.5);

    EXPECT_NEAR (0.5, lfo.getPhase(), 1e-12);
}

TEST_F (LFOTests, ZeroFrequencyHolds)
{
    lfo.setFrequency (0.0);
    lfo.setShape (LFO<double>::Shape::sawtooth);

    EXPECT_NEAR (-1.0, lfo.skip (1000), 1e-12);
    EXPECT_DOUBLE_EQ (0.0, lfo.getFrequency());
}

TEST_F (LFOTests, NegativeFrequencyIsClampedToZero)
{
    lfo.setFrequency (-5.0);

    EXPECT_DOUBLE_EQ (0.0, lfo.getFrequency());
}
