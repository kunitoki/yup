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

namespace
{
constexpr double combSampleRate = 48000.0;
constexpr int combBlockSize = 128;

template <typename SampleType>
void expectFiniteCombBuffer (const std::vector<SampleType>& buffer)
{
    for (auto sample : buffer)
        EXPECT_TRUE (std::isfinite (sample));
}
} // namespace

//==============================================================================

TEST (CombFilterTests, DefaultConstructorInitializes)
{
    CombFilter<float> filter;

    EXPECT_NO_THROW (filter.prepare (combSampleRate, combBlockSize));
    EXPECT_GT (filter.getFrequency(), 0.0f);
    EXPECT_GE (filter.getDelayInSamples(), 1.0f);
}

TEST (CombFilterTests, ProducesExpectedFeedForwardDelayTap)
{
    CombFilter<float> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (static_cast<float> (combSampleRate / 8.0), 0.0f, 0.0f, combSampleRate);
    filter.reset();

    std::vector<float> input (combBlockSize, 0.0f);
    std::vector<float> output (combBlockSize, 0.0f);
    input[0] = 1.0f;

    filter.processBlock (input.data(), output.data(), combBlockSize);

    EXPECT_FLOAT_EQ (output[0], 1.0f);
    EXPECT_NEAR (output[8], 0.5f, 1e-5f);
    expectFiniteCombBuffer (output);
}

TEST (CombFilterTests, FeedbackAndSaturationRemainFinite)
{
    CombFilter<float> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (440.0f, 1.0f, 1.0f, combSampleRate);
    filter.setSignalRange (2.0f);

    std::vector<float> input (combBlockSize, 0.25f);
    std::vector<float> output (combBlockSize, 0.0f);

    filter.processBlock (input.data(), output.data(), combBlockSize);

    expectFiniteCombBuffer (output);
}

TEST (CombFilterTests, SetParametersFromNoteUpdatesDelay)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParametersFromNote (69.0, 0.5, 0.0, combSampleRate);

    EXPECT_NEAR (filter.getFrequency(), 440.0, 1e-9);
    EXPECT_NEAR (filter.getDelayInSamples(), combSampleRate / 440.0, 1e-9);
}

TEST (CombFilterTests, ResetRestoresDeterministicState)
{
    CombFilter<float> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (660.0f, 0.5f, 0.2f, combSampleRate);

    std::vector<float> input (combBlockSize, 0.0f);
    std::vector<float> firstOutput (combBlockSize, 0.0f);
    std::vector<float> secondOutput (combBlockSize, 0.0f);

    for (int i = 0; i < combBlockSize; ++i)
        input[static_cast<std::size_t> (i)] = (i % 5 == 0) ? 0.5f : -0.125f;

    filter.processBlock (input.data(), firstOutput.data(), combBlockSize);
    filter.reset();
    filter.processBlock (input.data(), secondOutput.data(), combBlockSize);

    for (int i = 0; i < combBlockSize; ++i)
        EXPECT_FLOAT_EQ (firstOutput[static_cast<std::size_t> (i)], secondOutput[static_cast<std::size_t> (i)]);
}

TEST (CombFilterTests, ComplexResponseIsFinite)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (440.0, 0.7, 0.3, combSampleRate);

    const auto response = filter.getComplexResponse (1000.0);

    EXPECT_TRUE (std::isfinite (response.real()));
    EXPECT_TRUE (std::isfinite (response.imag()));
}

TEST (CombFilterTests, ComplexResponseMatchesIntegerDelayFeedForwardComb)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (combSampleRate / 8.0, 0.0, 0.0, combSampleRate);

    EXPECT_NEAR (std::abs (filter.getComplexResponse (0.0)), 1.5, 1e-9);
    EXPECT_NEAR (std::abs (filter.getComplexResponse (combSampleRate / 16.0)), 0.5, 1e-9);
}

TEST (CombFilterTests, ComplexResponseIncludesFeedbackResonance)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (combSampleRate / 8.0, 0.81, 0.0, combSampleRate);

    EXPECT_NEAR (std::abs (filter.getComplexResponse (combSampleRate / 8.0)), 6.0, 1e-9);
}

TEST (CombFilterTests, MagnitudeResponseRangeEnvelopesTheTeeth)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (441.0, 0.9, 0.0, combSampleRate);

    const auto spacing = combSampleRate / filter.getDelayInSamples();

    for (int tooth = 1; tooth <= 20; ++tooth)
    {
        auto peak = 0.0;
        auto trough = 1.0e9;

        for (int step = -500; step <= 500; ++step)
        {
            const auto frequency = spacing * (tooth + step / 1000.0);
            const auto magnitude = filter.getMagnitudeResponse (frequency);
            const auto range = filter.getMagnitudeResponseRange (frequency);

            EXPECT_GE (magnitude, range.getStart() - 1.0e-9);
            EXPECT_LE (magnitude, range.getEnd() + 1.0e-9);
            peak = jmax (peak, magnitude);
            trough = jmin (trough, magnitude);
        }

        EXPECT_NEAR (peak, filter.getMagnitudeResponseRange (spacing * tooth).getEnd(), 0.02 * peak);
        EXPECT_NEAR (trough, filter.getMagnitudeResponseRange (spacing * (tooth + 0.5)).getStart(), 0.02 * trough);
    }
}

TEST (CombFilterTests, FractionalDelayKeepsTheTeethHeight)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);

    for (const auto delay : { 8.0, 8.25, 8.5, 8.75 })
    {
        filter.setParameters (combSampleRate / delay, 0.81, 0.0, combSampleRate);

        EXPECT_NEAR (filter.getMagnitudeResponseRange (15000.0).getEnd(), 6.0, 0.03);
        EXPECT_NEAR (filter.getMagnitudeResponseRange (15000.0).getStart(), 1.0 - 0.5 / 1.9, 0.002);
    }
}

TEST (CombFilterTests, FractionalDelayProcessingMatchesResponse)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (combSampleRate / 8.5, 0.81, 0.0, combSampleRate);
    filter.reset();

    constexpr double frequency = 15000.0;
    constexpr double amplitude = 0.01;
    constexpr int settleSamples = 4800;
    constexpr int measureSamples = 16000;

    double sumOfSquares = 0.0;

    for (int i = 0; i < settleSamples + measureSamples; ++i)
    {
        const auto output = filter.processSample (amplitude * std::sin (MathConstants<double>::twoPi * frequency * i / combSampleRate));

        if (i >= settleSamples)
            sumOfSquares += output * output;
    }

    const auto measuredGain = std::sqrt (2.0 * sumOfSquares / measureSamples) / amplitude;

    EXPECT_NEAR (measuredGain, filter.getMagnitudeResponse (frequency), 0.01 * measuredGain);
}

TEST (CombFilterTests, SteppedLowFrequencySweepStaysSmooth)
{
    // Settles on the start frequency, sweeps to the end one updating every 16 samples like a
    // synth would, then holds it, returning the output's second difference as roughness.
    const auto measureRoughness = [] (double startFrequency, double endFrequency, double feedback, int sweepSamples)
    {
        CombFilter<double> filter;
        filter.prepare (combSampleRate, combBlockSize);
        filter.setParameters (startFrequency, feedback, 0.0, combSampleRate);
        filter.reset();

        constexpr int settleSamples = 4800;
        constexpr int holdSamples = 2400;
        double previous = 0.0;
        double beforePrevious = 0.0;
        double sumOfSquares = 0.0;

        for (int i = 0; i < settleSamples + sweepSamples + holdSamples; ++i)
        {
            if (i % 16 == 0)
            {
                const auto progress = jlimit (0.0, 1.0, (i - settleSamples) / static_cast<double> (sweepSamples));
                filter.setParameters (startFrequency * std::pow (endFrequency / startFrequency, progress), feedback, 0.0, combSampleRate);
            }

            const auto output = filter.processSample (0.3 * std::sin (MathConstants<double>::twoPi * 220.0 * i / combSampleRate));
            const auto secondDifference = output - 2.0 * previous + beforePrevious;

            if (i >= settleSamples)
                sumOfSquares += secondDifference * secondDifference;

            beforePrevious = previous;
            previous = output;
        }

        return std::sqrt (sumOfSquares / (sweepSamples + holdSamples));
    };

    EXPECT_LT (measureRoughness (40.0, 160.0, 0.9, 4800), 0.001);
    EXPECT_LT (measureRoughness (160.0, 40.0, 0.9, 4800), 0.001);
    EXPECT_LT (measureRoughness (20.0, 100.0, 0.0, 480), 0.0008);
    EXPECT_LT (measureRoughness (100.0, 20.0, 0.0, 480), 0.0008);
}

TEST (CombFilterTests, SlowDelayModulationHasNoZipperNoise)
{
    CombFilter<double> filter;
    filter.prepare (combSampleRate, combBlockSize);
    filter.setParameters (combSampleRate / 1200.0, 0.0, 0.0, combSampleRate);
    filter.reset();

    constexpr int numSamples = 9600;
    const auto omega = MathConstants<double>::twoPi * 10000.0 / combSampleRate;

    // The delayed copy of a sinusoid satisfies z[n] = 2 cos (omega) z[n - 1] - z[n - 2]
    // while its delay drifts slowly; a glitch in the read breaks the recurrence.
    double previous = 0.0;
    double beforePrevious = 0.0;
    double worstResidual = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        filter.setParameters (combSampleRate / (1200.0 - 4.0 * i / numSamples), 0.0, 0.0, combSampleRate);

        const auto input = 0.3 * std::sin (omega * i);
        const auto delayed = 2.0 * (filter.processSample (input) - input);

        if (i >= 2400)
            worstResidual = jmax (worstResidual, std::abs (delayed - 2.0 * std::cos (omega) * previous + beforePrevious));

        beforePrevious = previous;
        previous = delayed;
    }

    EXPECT_LT (worstResidual, 0.002);
}
