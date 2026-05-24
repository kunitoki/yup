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
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 128;

template <typename SampleType>
void expectFiniteBuffer (const std::vector<SampleType>& buffer)
{
    for (auto sample : buffer)
        EXPECT_TRUE (std::isfinite (sample));
}
} // namespace

//==============================================================================

TEST (CombFilterTests, DefaultConstructorInitializes)
{
    CombFilter<float> filter;

    EXPECT_NO_THROW (filter.prepare (sampleRate, blockSize));
    EXPECT_GT (filter.getFrequency(), 0.0f);
    EXPECT_GE (filter.getDelayInSamples(), 1.0f);
}

TEST (CombFilterTests, ProducesExpectedFeedForwardDelayTap)
{
    CombFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (static_cast<float> (sampleRate / 8.0), 0.0f, 0.0f, sampleRate);
    filter.reset();

    std::vector<float> input (blockSize, 0.0f);
    std::vector<float> output (blockSize, 0.0f);
    input[0] = 1.0f;

    filter.processBlock (input.data(), output.data(), blockSize);

    EXPECT_FLOAT_EQ (output[0], 1.0f);
    EXPECT_NEAR (output[8], 0.5f, 1e-5f);
    expectFiniteBuffer (output);
}

TEST (CombFilterTests, FeedbackAndSaturationRemainFinite)
{
    CombFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (440.0f, 1.0f, 1.0f, sampleRate);
    filter.setSignalRange (2.0f);

    std::vector<float> input (blockSize, 0.25f);
    std::vector<float> output (blockSize, 0.0f);

    filter.processBlock (input.data(), output.data(), blockSize);

    expectFiniteBuffer (output);
}

TEST (CombFilterTests, SetParametersFromNoteUpdatesDelay)
{
    CombFilter<double> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParametersFromNote (69.0, 0.5, 0.0, sampleRate);

    EXPECT_NEAR (filter.getFrequency(), 440.0, 1e-9);
    EXPECT_NEAR (filter.getDelayInSamples(), sampleRate / 440.0, 1e-9);
}

TEST (CombFilterTests, ResetRestoresDeterministicState)
{
    CombFilter<float> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (660.0f, 0.5f, 0.2f, sampleRate);

    std::vector<float> input (blockSize, 0.0f);
    std::vector<float> firstOutput (blockSize, 0.0f);
    std::vector<float> secondOutput (blockSize, 0.0f);

    for (int i = 0; i < blockSize; ++i)
        input[static_cast<std::size_t> (i)] = (i % 5 == 0) ? 0.5f : -0.125f;

    filter.processBlock (input.data(), firstOutput.data(), blockSize);
    filter.reset();
    filter.processBlock (input.data(), secondOutput.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
        EXPECT_FLOAT_EQ (firstOutput[static_cast<std::size_t> (i)], secondOutput[static_cast<std::size_t> (i)]);
}

TEST (CombFilterTests, ComplexResponseIsFinite)
{
    CombFilter<double> filter;
    filter.prepare (sampleRate, blockSize);
    filter.setParameters (440.0, 0.7, 0.3, sampleRate);

    const auto response = filter.getComplexResponse (1000.0);

    EXPECT_TRUE (std::isfinite (response.real()));
    EXPECT_TRUE (std::isfinite (response.imag()));
}
