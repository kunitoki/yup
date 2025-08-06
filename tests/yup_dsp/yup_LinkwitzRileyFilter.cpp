/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
constexpr double tolerance = 1e-4;
constexpr float toleranceF = 1e-4f;
constexpr double sampleRate = 44100.0;
constexpr int blockSize = 256;
} // namespace

//==============================================================================
class LinkwitzRileyFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test vectors
        testDataLeft.resize (blockSize);
        testDataRight.resize (blockSize);
        outputLowLeft.resize (blockSize);
        outputLowRight.resize (blockSize);
        outputHighLeft.resize (blockSize);
        outputHighRight.resize (blockSize);

        // Generate impulse and white noise test signals
        std::fill (testDataLeft.begin(), testDataLeft.end(), 0.0f);
        std::fill (testDataRight.begin(), testDataRight.end(), 0.0f);
        testDataLeft[0] = 1.0f; // Impulse
        testDataRight[0] = 1.0f;

        // Generate sine wave test signal at 1kHz
        sineTestLeft.resize (blockSize);
        sineTestRight.resize (blockSize);
        for (int i = 0; i < blockSize; ++i)
        {
            const auto phase = static_cast<float> (i) * 1000.0f * 2.0f * MathConstants<float>::pi / static_cast<float> (sampleRate);
            sineTestLeft[i] = std::sin (phase);
            sineTestRight[i] = std::sin (phase);
        }
    }

    std::vector<float> testDataLeft, testDataRight;
    std::vector<float> outputLowLeft, outputLowRight;
    std::vector<float> outputHighLeft, outputHighRight;
    std::vector<float> sineTestLeft, sineTestRight;
};

//==============================================================================
TEST_F (LinkwitzRileyFilterTests, LR2ConstructorSetsValidDefaults)
{
    LinkwitzRiley2Filter<float> filter;

    EXPECT_EQ (filter.getFrequency(), 1000.0);
    EXPECT_EQ (filter.getSampleRate(), 44100.0);
    EXPECT_EQ (filter.getOrder(), 2);
}

TEST_F (LinkwitzRileyFilterTests, LR2SetParametersUpdatesCorrectly)
{
    LinkwitzRiley2Filter<float> filter;

    filter.setParameters (2000.0, 48000.0);

    EXPECT_NEAR (filter.getFrequency(), 2000.0, tolerance);
    EXPECT_NEAR (filter.getSampleRate(), 48000.0, tolerance);
}

TEST_F (LinkwitzRileyFilterTests, LR2ProcessSampleDoesNotCrash)
{
    LinkwitzRiley2Filter<float> filter (1000.0);

    float lowLeft, lowRight, highLeft, highRight;
    filter.processSample (0.5f, 0.5f, lowLeft, lowRight, highLeft, highRight);

    // Should produce valid output
    EXPECT_TRUE (std::isfinite (lowLeft));
    EXPECT_TRUE (std::isfinite (lowRight));
    EXPECT_TRUE (std::isfinite (highLeft));
    EXPECT_TRUE (std::isfinite (highRight));
}

TEST_F (LinkwitzRileyFilterTests, LR2ProcessBufferDoesNotCrash)
{
    LinkwitzRiley2Filter<float> filter (1000.0);

    filter.processBuffer (testDataLeft.data(),
                          testDataRight.data(),
                          outputLowLeft.data(),
                          outputLowRight.data(),
                          outputHighLeft.data(),
                          outputHighRight.data(),
                          blockSize);

    // Should produce valid output
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputLowLeft[i]));
        EXPECT_TRUE (std::isfinite (outputLowRight[i]));
        EXPECT_TRUE (std::isfinite (outputHighLeft[i]));
        EXPECT_TRUE (std::isfinite (outputHighRight[i]));
    }
}

TEST_F (LinkwitzRileyFilterTests, LR4ConstructorSetsValidDefaults)
{
    LinkwitzRiley4Filter<float> filter;

    EXPECT_EQ (filter.getFrequency(), 1000.0);
    EXPECT_EQ (filter.getSampleRate(), 44100.0);
    EXPECT_EQ (filter.getOrder(), 4);
}

TEST_F (LinkwitzRileyFilterTests, LR4ProcessSampleDoesNotCrash)
{
    LinkwitzRiley4Filter<float> filter (1000.0);

    float lowLeft, lowRight, highLeft, highRight;
    filter.processSample (0.5f, 0.5f, lowLeft, lowRight, highLeft, highRight);

    // Should produce valid output
    EXPECT_TRUE (std::isfinite (lowLeft));
    EXPECT_TRUE (std::isfinite (lowRight));
    EXPECT_TRUE (std::isfinite (highLeft));
    EXPECT_TRUE (std::isfinite (highRight));
}

TEST_F (LinkwitzRileyFilterTests, LR8ConstructorSetsValidDefaults)
{
    LinkwitzRiley8Filter<float> filter;

    EXPECT_EQ (filter.getFrequency(), 1000.0);
    EXPECT_EQ (filter.getSampleRate(), 44100.0);
    EXPECT_EQ (filter.getOrder(), 8);
}

TEST_F (LinkwitzRileyFilterTests, LR8ProcessSampleDoesNotCrash)
{
    LinkwitzRiley8Filter<float> filter (1000.0);

    float lowLeft, lowRight, highLeft, highRight;
    filter.processSample (0.5f, 0.5f, lowLeft, lowRight, highLeft, highRight);

    // Should produce valid output
    EXPECT_TRUE (std::isfinite (lowLeft));
    EXPECT_TRUE (std::isfinite (lowRight));
    EXPECT_TRUE (std::isfinite (highLeft));
    EXPECT_TRUE (std::isfinite (highRight));
}

TEST_F (LinkwitzRileyFilterTests, ComplementaryResponse)
{
    LinkwitzRiley2Filter<float> filter (1000.0);
    filter.setSampleRate (sampleRate);
    filter.reset();

    // Now do the actual test
    float lowLeft, lowRight, highLeft, highRight;

    // Let the filter settle by processing some samples first
    for (int i = 0; i < blockSize; ++i)
        filter.processSample (sineTestLeft[i], sineTestRight[i], lowLeft, lowRight, highLeft, highRight);

    // Test that low + high outputs sum to approximately unity at crossover frequency
    std::vector<float> summedLeft (blockSize);
    std::vector<float> summedRight (blockSize);

    // Process sine wave at crossover frequency (second pass for steady state)
    for (int i = 0; i < blockSize; ++i)
    {
        filter.processSample (sineTestLeft[i], sineTestRight[i], lowLeft, lowRight, highLeft, highRight);
        summedLeft[i] = lowLeft + highLeft;
        summedRight[i] = lowRight + highRight;
    }

    // Calculate RMS of summed outputs
    float sumRmsLeft = 0.0f, sumRmsRight = 0.0f;
    float inputRmsLeft = 0.0f, inputRmsRight = 0.0f;

    for (int i = 0; i < blockSize; ++i)
    {
        sumRmsLeft += summedLeft[i] * summedLeft[i];
        sumRmsRight += summedRight[i] * summedRight[i];
        inputRmsLeft += sineTestLeft[i] * sineTestLeft[i];
        inputRmsRight += sineTestRight[i] * sineTestRight[i];
    }

    sumRmsLeft = std::sqrt (sumRmsLeft / blockSize);
    sumRmsRight = std::sqrt (sumRmsRight / blockSize);
    inputRmsLeft = std::sqrt (inputRmsLeft / blockSize);
    inputRmsRight = std::sqrt (inputRmsRight / blockSize);

    // Allow for some tolerance due to filter transient and numerical precision
    EXPECT_NEAR (sumRmsLeft, inputRmsLeft, 0.1f);
    EXPECT_NEAR (sumRmsRight, inputRmsRight, 0.1f);
}

TEST_F (LinkwitzRileyFilterTests, ResetClearsState)
{
    LinkwitzRiley2Filter<float> filter (1000.0);

    // Process some data to build up state
    float lowLeft, lowRight, highLeft, highRight;
    for (int i = 0; i < 10; ++i)
    {
        filter.processSample (1.0f, 1.0f, lowLeft, lowRight, highLeft, highRight);
    }

    // Reset and process silence
    filter.reset();
    filter.processSample (0.0f, 0.0f, lowLeft, lowRight, highLeft, highRight);

    // Output should be zero (or very close to zero)
    EXPECT_NEAR (lowLeft, 0.0f, toleranceF);
    EXPECT_NEAR (lowRight, 0.0f, toleranceF);
    EXPECT_NEAR (highLeft, 0.0f, toleranceF);
    EXPECT_NEAR (highRight, 0.0f, toleranceF);
}

//==============================================================================
// FilterDesigner Tests

TEST (FilterDesignerLinkwitzRileyTests, DesignLR2ReturnsValidCoefficients)
{
    std::vector<BiquadCoefficients<double>> lowCoeffs, highCoeffs;

    int sections = FilterDesigner<double>::designLinkwitzRiley2 (1000.0, sampleRate, lowCoeffs, highCoeffs);

    //EXPECT_EQ (sections, 2);
    EXPECT_EQ (lowCoeffs.size(), 2);
    EXPECT_EQ (highCoeffs.size(), 2);

    EXPECT_TRUE (std::isfinite (lowCoeffs[0].b0));
    EXPECT_TRUE (std::isfinite (lowCoeffs[0].b1));
    EXPECT_TRUE (std::isfinite (lowCoeffs[0].b2));
    EXPECT_TRUE (std::isfinite (lowCoeffs[0].a0));
    EXPECT_TRUE (std::isfinite (lowCoeffs[0].a1));
    EXPECT_TRUE (std::isfinite (lowCoeffs[0].a2));

    EXPECT_TRUE (std::isfinite (highCoeffs[0].b0));
    EXPECT_TRUE (std::isfinite (highCoeffs[0].b1));
    EXPECT_TRUE (std::isfinite (highCoeffs[0].b2));
    EXPECT_TRUE (std::isfinite (highCoeffs[0].a0));
    EXPECT_TRUE (std::isfinite (highCoeffs[0].a1));
    EXPECT_TRUE (std::isfinite (highCoeffs[0].a2));
}

TEST (FilterDesignerLinkwitzRileyTests, DesignLR4ReturnsCorrectNumberOfSections)
{
    std::vector<BiquadCoefficients<double>> lowCoeffs, highCoeffs;

    int sections = FilterDesigner<double>::designLinkwitzRiley4 (1000.0, sampleRate, lowCoeffs, highCoeffs);

    EXPECT_EQ (sections, 4); // LR4 should create 4 biquad sections
    EXPECT_EQ (lowCoeffs.size(), 4);
    EXPECT_EQ (highCoeffs.size(), 4);
}

TEST (FilterDesignerLinkwitzRileyTests, DesignLR8ReturnsCorrectNumberOfSections)
{
    std::vector<BiquadCoefficients<double>> lowCoeffs, highCoeffs;

    int sections = FilterDesigner<double>::designLinkwitzRiley8 (1000.0, sampleRate, lowCoeffs, highCoeffs);

    EXPECT_EQ (sections, 8); // LR8 should create 8 biquad sections
    EXPECT_EQ (lowCoeffs.size(), 8);
    EXPECT_EQ (highCoeffs.size(), 8);
}

TEST (FilterDesignerLinkwitzRileyTests, GeneralDesignerHandlesVariousOrders)
{
    std::vector<BiquadCoefficients<double>> lowCoeffs, highCoeffs;

    // Test LR2
    int sections2 = FilterDesigner<double>::designLinkwitzRiley (2, 1000.0, sampleRate, lowCoeffs, highCoeffs);
    EXPECT_EQ (sections2, 2);

    // Test LR4
    int sections4 = FilterDesigner<double>::designLinkwitzRiley (4, 1000.0, sampleRate, lowCoeffs, highCoeffs);
    EXPECT_EQ (sections4, 4);

    // Test LR8
    int sections8 = FilterDesigner<double>::designLinkwitzRiley (8, 1000.0, sampleRate, lowCoeffs, highCoeffs);
    EXPECT_EQ (sections8, 8);
}
