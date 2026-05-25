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
