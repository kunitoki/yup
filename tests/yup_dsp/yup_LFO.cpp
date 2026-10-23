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

TEST_F (LFOTests, SawDownFallsFromOneToMinusOne)
{
    lfo.setShape (LFO<double>::Shape::sawDown);

    EXPECT_NEAR (1.0, lfo.getValue(), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 2), 1e-12);
    EXPECT_NEAR (-0.75, lfo.skip (period / 2 - 2), 1e-12);
}

TEST_F (LFOTests, SmoothRandomIsContinuousAndBounded)
{
    lfo.setShape (LFO<double>::Shape::smoothRandom);
    lfo.setFrequency (100.0);

    auto previous = lfo.getValue();
    auto maxJump = 0.0;
    auto minimum = previous;
    auto maximum = previous;

    for (int i = 0; i < 48000; ++i)
    {
        const auto value = lfo.processSample();

        maxJump = jmax (maxJump, std::abs (value - previous));
        minimum = jmin (minimum, value);
        maximum = jmax (maximum, value);
        previous = value;
    }

    EXPECT_GE (minimum, -1.0);
    EXPECT_LE (maximum, 1.0);
    EXPECT_GT (maximum - minimum, 0.5);

    // A full swing of 1.6 spread over a 480 sample cycle, with the spline's overshoot.
    EXPECT_LT (maxJump, 0.02);
}

TEST_F (LFOTests, SmoothRandomIsDeterministicPerSeed)
{
    LFO<double> a, b;

    for (auto* l : { &a, &b })
    {
        l->prepare (sampleRate);
        l->setFrequency (frequency);
        l->setShape (LFO<double>::Shape::smoothRandom);
        l->setSeed (11u);
    }

    for (int i = 0; i < 100; ++i)
        EXPECT_DOUBLE_EQ (a.processSample(), b.processSample());
}

TEST_F (LFOTests, CustomShapePlaysTheCurveAsOneBipolarCycle)
{
    BreakpointCurve<double> curve;
    curve.addPoint (0.0, 0.0);
    curve.addPoint (0.5, 1.0);
    curve.addPoint (1.0, 0.0);

    lfo.setShape (LFO<double>::Shape::custom);
    lfo.setCustomCurve (&curve);

    EXPECT_NEAR (-1.0, lfo.getValue(), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (1.0, lfo.skip (period / 4), 1e-12);
    EXPECT_NEAR (0.0, lfo.skip (period / 4), 1e-12);
}

TEST_F (LFOTests, CustomShapeWithoutCurveIsSilent)
{
    lfo.setShape (LFO<double>::Shape::custom);

    EXPECT_EQ (0.0, lfo.getValue());
    EXPECT_EQ (0.0, lfo.skip (3));
}

TEST_F (LFOTests, SmoothingSlewsASquare)
{
    constexpr double smoothingSeconds = 0.001;

    lfo.setFrequency (10.0);
    lfo.setShape (LFO<double>::Shape::square);
    lfo.setSmoothing (smoothingSeconds);
    lfo.reset();

    EXPECT_NEAR (1.0, lfo.getValue(), 1e-12);

    // Half a 10 Hz period is 2400 samples; the edge happens on the last of them.
    for (int i = 0; i < 2400; ++i)
        lfo.processSample();

    const auto tau = static_cast<int> (smoothingSeconds * sampleRate);
    auto previous = lfo.getValue();
    auto maxJump = 0.0;

    for (int i = 0; i < tau; ++i)
    {
        const auto value = lfo.processSample();
        maxJump = jmax (maxJump, std::abs (value - previous));
        previous = value;
    }

    // About one time constant after the edge, 63% of the swing of 2 has been covered.
    EXPECT_NEAR (1.0 - 2.0 * 0.632, lfo.getValue(), 0.03);
    EXPECT_LT (maxJump, 2.0 / tau);
}

TEST_F (LFOTests, SkipWithSmoothingFollowsTheSameTimeConstant)
{
    LFO<double> stepped;
    stepped.prepare (sampleRate);

    for (auto* l : { &lfo, &stepped })
    {
        l->setFrequency (5.0);
        l->setShape (LFO<double>::Shape::sine);
        l->setSmoothing (0.002);
        l->reset();
    }

    for (int block = 0; block < 100; ++block)
    {
        for (int i = 0; i < 32; ++i)
            stepped.processSample();

        // skip() holds the waveform over the block, so it leads by about half a block.
        EXPECT_NEAR (stepped.getValue(), lfo.skip (32), 0.02) << block;
    }
}

TEST_F (LFOTests, SmoothingOffLeavesTheRawWaveform)
{
    lfo.setShape (LFO<double>::Shape::square);
    lfo.setSmoothing (0.0);

    EXPECT_DOUBLE_EQ (1.0, lfo.getValue());
    EXPECT_DOUBLE_EQ (-1.0, lfo.skip (period / 2));
}

TEST_F (LFOTests, FreeRunningIgnoresNoteOn)
{
    lfo.setShape (LFO<double>::Shape::sawtooth);
    lfo.setRetriggerMode (LFO<double>::RetriggerMode::free);
    lfo.setStartPhase (0.5);
    lfo.skip (3);

    lfo.noteOn();

    EXPECT_NEAR (3.0 / period, lfo.getPhase(), 1e-12);
}

TEST_F (LFOTests, RetriggerRestartsAtTheStartPhase)
{
    lfo.setShape (LFO<double>::Shape::sawtooth);
    lfo.setRetriggerMode (LFO<double>::RetriggerMode::retrigger);
    lfo.setStartPhase (0.25);
    lfo.skip (5);

    lfo.noteOn();

    EXPECT_NEAR (0.25, lfo.getPhase(), 1e-12);
    EXPECT_NEAR (-0.5, lfo.getValue(), 1e-12);

    // Retriggering keeps going after the first cycle.
    EXPECT_NEAR (-0.5, lfo.skip (period), 1e-9);
}

TEST_F (LFOTests, OneShotHoldsTheEndOfTheCycle)
{
    lfo.setShape (LFO<double>::Shape::sawtooth);
    lfo.setRetriggerMode (LFO<double>::RetriggerMode::oneShot);
    lfo.noteOn();

    EXPECT_NEAR (0.0, lfo.skip (period / 2), 1e-12);
    EXPECT_NEAR (1.0, lfo.skip (period), 1e-9);
    EXPECT_NEAR (1.0, lfo.skip (period * 3), 1e-9);

    lfo.noteOn();
    EXPECT_NEAR (-1.0, lfo.getValue(), 1e-12);
}
