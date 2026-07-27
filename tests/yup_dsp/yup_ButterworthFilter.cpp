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

//==============================================================================
class ButterworthFilterTests : public ::testing::Test
{
protected:
    static constexpr double tolerance = 1e-4;
    static constexpr float toleranceF = 1e-4f;
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 256;

    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);

        // Initialize test vectors
        testData.resize (blockSize);
        outputData.resize (blockSize);
        doubleTestData.resize (blockSize);
        doubleOutputData.resize (blockSize);

        // Fill with test pattern - impulse followed by sine wave
        for (int i = 0; i < blockSize; ++i)
        {
            testData[i] = (i == 0) ? 1.0f : 0.1f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
            doubleTestData[i] = (i == 0) ? 1.0 : 0.1 * std::sin (2.0 * MathConstants<double>::pi * 1000.0 * i / sampleRate);
        }
    }

    ButterworthFilter<float> filterFloat;
    ButterworthFilter<double> filterDouble;
    std::vector<float> testData;
    std::vector<float> outputData;
    std::vector<double> doubleTestData;
    std::vector<double> doubleOutputData;
};

//==============================================================================
TEST_F (ButterworthFilterTests, DefaultConstruction)
{
    ButterworthFilter<float> filter;
    EXPECT_EQ (filter.getMode(), FilterMode::lowpass);
    EXPECT_EQ (filter.getOrder(), 2);
    EXPECT_FLOAT_EQ (filter.getFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getSecondaryFrequency(), 2000.0f);
}

TEST_F (ButterworthFilterTests, ParameterizedConstruction)
{
    ButterworthFilter<float> filter (FilterMode::highpass, 4, 500.0f);
    EXPECT_EQ (filter.getMode(), FilterMode::highpass);
    EXPECT_EQ (filter.getOrder(), 4);
    EXPECT_FLOAT_EQ (filter.getFrequency(), 500.0f);
}

TEST_F (ButterworthFilterTests, SupportedModes)
{
    auto supportedModes = filterFloat.getSupportedModes();
    EXPECT_TRUE (supportedModes.test (FilterMode::lowpass));
    EXPECT_TRUE (supportedModes.test (FilterMode::highpass));
    EXPECT_TRUE (supportedModes.test (FilterMode::bandpass));
    EXPECT_TRUE (supportedModes.test (FilterMode::bandstop));
    EXPECT_TRUE (supportedModes.test (FilterMode::allpass));
}

TEST_F (ButterworthFilterTests, ParameterSetting)
{
    filterFloat.setParameters (FilterMode::bandpass, 8, 1000.0f, 2000.0f, sampleRate);

    EXPECT_EQ (filterFloat.getMode(), FilterMode::bandpass);
    EXPECT_EQ (filterFloat.getOrder(), 8);
    EXPECT_FLOAT_EQ (filterFloat.getFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getSecondaryFrequency(), 2000.0f);
}

TEST_F (ButterworthFilterTests, OrderCorrection)
{
    // Test that odd orders get corrected to next even value
    filterFloat.setOrder (5);
    EXPECT_EQ (filterFloat.getOrder(), 6);

    filterFloat.setOrder (3);
    EXPECT_EQ (filterFloat.getOrder(), 4);

    filterFloat.setOrder (1);
    EXPECT_EQ (filterFloat.getOrder(), 2); // Minimum order is 2
}

TEST_F (ButterworthFilterTests, LowpassFrequencyResponse)
{
    filterFloat.setParameters (FilterMode::lowpass, 4, 1000.0f, 0.0f, sampleRate);

    // DC response should be close to 1.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_NEAR (dcResponse, 1.0, 0.1);

    // Cutoff frequency response should be about -3dB per 2nd order section
    auto cutoffResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    EXPECT_LT (cutoffResponse, 1.0);
    EXPECT_GT (cutoffResponse, 0.1);

    // High frequency should be heavily attenuated for 4th order
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_LT (highFreqResponse, 0.1);
}

TEST_F (ButterworthFilterTests, HighpassFrequencyResponse)
{
    filterFloat.setParameters (FilterMode::highpass, 4, 1000.0f, 0.0f, sampleRate);

    // DC response should be close to 0.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_LT (dcResponse, 0.1);

    // High frequency should pass
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_GT (highFreqResponse, 0.5);
}

TEST_F (ButterworthFilterTests, BandpassFrequencyResponse)
{
    filterFloat.setParameters (FilterMode::bandpass, 4, 800.0f, 1200.0f, sampleRate);

    // DC and high frequency should be attenuated
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (20000.0));

    EXPECT_LT (dcResponse, 0.1);
    EXPECT_LT (highFreqResponse, 0.1);

    // Center frequency should pass
    auto centerFreq = std::sqrt (800.0f * 1200.0f);
    auto centerResponse = std::abs (filterFloat.getComplexResponse (centerFreq));
    EXPECT_GT (centerResponse, 0.3);
}

TEST_F (ButterworthFilterTests, BandstopFrequencyResponse)
{
    filterFloat.setParameters (FilterMode::bandstop, 4, 800.0f, 1200.0f, sampleRate);

    // DC and high frequency should pass
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (20000.0));

    EXPECT_GT (dcResponse, 0.5);
    EXPECT_GT (highFreqResponse, 0.5);

    // Center frequency should be attenuated
    auto centerFreq = std::sqrt (800.0f * 1200.0f);
    auto centerResponse = std::abs (filterFloat.getComplexResponse (centerFreq));
    EXPECT_LT (centerResponse, 0.5);
}

TEST_F (ButterworthFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FilterMode::lowpass, 2, 1000.0f, 0.0f, sampleRate);

    for (int i = 0; i < 10; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ButterworthFilterTests, BlockProcessing)
{
    filterFloat.setParameters (FilterMode::bandpass, 4, 800.0f, 1200.0f, sampleRate);

    filterFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (ButterworthFilterTests, HighOrderStability)
{
    // Test high order filters that were previously unstable with ZPK approach
    filterFloat.setParameters (FilterMode::lowpass, 16, 1000.0f, 0.0f, sampleRate);

    // Process a longer sequence to test stability
    std::vector<float> longTestData (1000);
    for (int i = 0; i < 1000; ++i)
    {
        longTestData[i] = 0.1f * std::sin (2.0f * MathConstants<float>::pi * 500.0f * i / static_cast<float> (sampleRate));
    }

    for (int i = 0; i < 1000; ++i)
    {
        auto output = filterFloat.processSample (longTestData[i]);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (ButterworthFilterTests, ParameterAutomation)
{
    filterFloat.setParameters (FilterMode::lowpass, 8, 1000.0f, 0.0f, sampleRate);

    // Simulate parameter automation like in real use
    for (int sweep = 0; sweep < 100; ++sweep)
    {
        float freq = 500.0f + 1500.0f * sweep / 100.0f;
        filterFloat.setFrequency (freq);

        // Process a few samples at each frequency
        for (int i = 0; i < 10; ++i)
        {
            auto output = filterFloat.processSample (testData[i % blockSize]);
            EXPECT_TRUE (std::isfinite (output));
        }
    }
}

TEST_F (ButterworthFilterTests, StateReset)
{
    filterFloat.setParameters (FilterMode::lowpass, 4, 1000.0f, 0.0f, sampleRate);

    // Process some samples to build up internal state
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (1.0f);

    auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, the output should be closer to zero
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
}

TEST_F (ButterworthFilterTests, PolesAndZeros)
{
    filterDouble.setParameters (FilterMode::lowpass, 4, 1000.0, 0.0, sampleRate);

    std::vector<std::complex<double>> poles, zeros;
    filterDouble.getPolesZeros (poles, zeros);

    // A 4th-order filter should have 4 poles
    EXPECT_EQ (poles.size(), 4u);

    // For a stable filter, all poles should be inside the unit circle
    for (const auto& pole : poles)
    {
        EXPECT_LT (std::abs (pole), 1.0 + tolerance);
    }
}

TEST_F (ButterworthFilterTests, BandpassPolesAndZeros)
{
    filterDouble.setParameters (FilterMode::bandpass, 4, 800.0, 1200.0, sampleRate);

    std::vector<std::complex<double>> poles, zeros;
    filterDouble.getPolesZeros (poles, zeros);

    // Bandpass should have both poles and zeros
    EXPECT_GT (poles.size(), 0u);
    EXPECT_GT (zeros.size(), 0u);

    // All poles should be stable
    for (const auto& pole : poles)
    {
        EXPECT_LT (std::abs (pole), 1.0 + tolerance);
    }
}

TEST_F (ButterworthFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (FilterMode::lowpass, 4, 1000.0f, 0.0f, sampleRate);
    filterDouble.setParameters (FilterMode::lowpass, 4, 1000.0, 0.0, sampleRate);

    std::vector<float> outputFloat (blockSize);
    std::vector<double> outputDouble (blockSize);

    filterFloat.processBlock (testData.data(), outputFloat.data(), blockSize);
    filterDouble.processBlock (doubleTestData.data(), outputDouble.data(), blockSize);

    // Results should be close but not identical due to precision differences
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (outputFloat[i], static_cast<float> (outputDouble[i]), 1e-3f);
    }
}

TEST_F (ButterworthFilterTests, ZeroInput)
{
    filterFloat.setParameters (FilterMode::bandpass, 8, 800.0f, 1200.0f, sampleRate);

    for (int i = 0; i < 100; ++i)
    {
        auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ButterworthFilterTests, ImpulseResponse)
{
    filterFloat.setParameters (FilterMode::lowpass, 4, 1000.0f, 0.0f, sampleRate);
    filterFloat.reset();

    std::vector<float> impulseResponse (128);
    for (int i = 0; i < 128; ++i)
    {
        float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and should eventually decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));

    // Check that the response eventually settles (last samples should be smaller than peak)
    float maxResponse = 0.0f;
    for (int i = 0; i < 64; ++i)
        maxResponse = std::max (maxResponse, std::abs (impulseResponse[i]));

    float finalResponse = std::abs (impulseResponse[127]);
    EXPECT_LT (finalResponse, maxResponse * 0.1f); // Final response should be much smaller than peak
}

TEST_F (ButterworthFilterTests, ParameterValidation)
{
    // Test that invalid parameters are handled gracefully
    EXPECT_NO_THROW (filterFloat.setParameters (FilterMode::bandpass, 2, 100.0f, 200.0f, sampleRate));

    // Test frequency order for bandpass
    EXPECT_NO_THROW (filterFloat.setParameters (FilterMode::bandpass, 2, 200.0f, 100.0f, sampleRate));
    // The filter should internally handle this correctly
}

TEST_F (ButterworthFilterTests, ModeChanges)
{
    // Test switching between different filter modes
    filterFloat.setParameters (FilterMode::lowpass, 4, 1000.0f, 0.0f, sampleRate);

    // Process some data
    for (int i = 0; i < 10; ++i)
        filterFloat.processSample (testData[i]);

    // Change to highpass
    filterFloat.setMode (FilterMode::highpass);

    // Should still work
    for (int i = 0; i < 10; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ButterworthFilterTests, ExtremeCoefficientStability)
{
    // Test with extreme frequency ranges
    filterFloat.setParameters (FilterMode::lowpass, 8, 10.0f, 0.0f, sampleRate); // Very low frequency

    for (int i = 0; i < 50; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }

    filterFloat.setParameters (FilterMode::lowpass, 8, 20000.0f, 0.0f, sampleRate); // High frequency

    for (int i = 0; i < 50; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ButterworthFilterTests, AllpassPhaseResponse)
{
    filterFloat.setParameters (FilterMode::allpass, 2, 1000.0f, 0.0f, sampleRate);

    // Allpass should have unity magnitude response across all frequencies
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    auto response1k = std::abs (filterFloat.getComplexResponse (1000.0));
    auto response5k = std::abs (filterFloat.getComplexResponse (5000.0));
    auto highResponse = std::abs (filterFloat.getComplexResponse (15000.0));

    // All should be close to 1.0 for a proper allpass filter
    EXPECT_NEAR (dcResponse, 1.0, 0.15);
    EXPECT_NEAR (response1k, 1.0, 0.15);
    EXPECT_NEAR (response5k, 1.0, 0.15);
    EXPECT_NEAR (highResponse, 1.0, 0.15);
}

TEST_F (ButterworthFilterTests, CascadeStructure)
{
    // Test that the filter properly creates the expected number of biquad sections
    filterFloat.setParameters (FilterMode::lowpass, 8, 1000.0f, 0.0f, sampleRate);

    // An 8th order filter should have 4 biquad sections
    EXPECT_EQ (filterFloat.getNumSections(), 4u);

    filterFloat.setParameters (FilterMode::lowpass, 6, 1000.0f, 0.0f, sampleRate);

    // A 6th order filter should have 3 biquad sections
    EXPECT_EQ (filterFloat.getNumSections(), 3u);
}
