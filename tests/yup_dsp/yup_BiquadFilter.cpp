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
class BiquadFilterTests : public ::testing::Test
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

    BiquadFloat filterFloat;
    BiquadDouble filterDouble;
    std::vector<float> testData;
    std::vector<float> outputData;
    std::vector<double> doubleTestData;
    std::vector<double> doubleOutputData;
};

TEST_F (BiquadFilterTests, DefaultConstruction)
{
    BiquadFloat filter;
    EXPECT_EQ (filter.getTopology(), BiquadFloat::Topology::directFormII);

    // Default coefficients should be a pass-through (b0=1, others=0)
    auto coeffs = filter.getCoefficients();
    EXPECT_FLOAT_EQ (coeffs.b0, 1.0f);
    EXPECT_FLOAT_EQ (coeffs.b1, 0.0f);
    EXPECT_FLOAT_EQ (coeffs.b2, 0.0f);
    EXPECT_FLOAT_EQ (coeffs.a1, 0.0f);
    EXPECT_FLOAT_EQ (coeffs.a2, 0.0f);
}

TEST_F (BiquadFilterTests, TopologyConstruction)
{
    BiquadFloat filter1 (BiquadFloat::Topology::directFormI);
    BiquadFloat filter2 (BiquadFloat::Topology::directFormII);
    BiquadFloat filter3 (BiquadFloat::Topology::transposedDirectFormII);

    EXPECT_EQ (filter1.getTopology(), BiquadFloat::Topology::directFormI);
    EXPECT_EQ (filter2.getTopology(), BiquadFloat::Topology::directFormII);
    EXPECT_EQ (filter3.getTopology(), BiquadFloat::Topology::transposedDirectFormII);
}

TEST_F (BiquadFilterTests, CoefficientSetAndGet)
{
    BiquadCoefficients<double> coeffs (1.0, 0.5, 0.25, 1.0, -0.5, 0.125);

    filterFloat.setCoefficients (coeffs);
    auto retrievedCoeffs = filterFloat.getCoefficients();

    EXPECT_DOUBLE_EQ (retrievedCoeffs.b0, 1.0);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.b1, 0.5);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.b2, 0.25);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.a1, -0.5);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.a2, 0.125);
}

TEST_F (BiquadFilterTests, TopologySwitch)
{
    // Set initial topology
    filterFloat.setTopology (BiquadFloat::Topology::directFormI);
    EXPECT_EQ (filterFloat.getTopology(), BiquadFloat::Topology::directFormI);

    // Switch topology
    filterFloat.setTopology (BiquadFloat::Topology::transposedDirectFormII);
    EXPECT_EQ (filterFloat.getTopology(), BiquadFloat::Topology::transposedDirectFormII);
}

TEST_F (BiquadFilterTests, CoefficientNormalization)
{
    // Create coefficients with a0 != 1
    BiquadCoefficients<double> coeffs (2.0, 1.0, 0.5, 2.0, -1.0, 0.25);

    filterFloat.setCoefficients (coeffs);
    auto normalizedCoeffs = filterFloat.getCoefficients();

    // After normalization, a0 should be 1.0 and others scaled appropriately
    EXPECT_DOUBLE_EQ (normalizedCoeffs.a0, 1.0);
    EXPECT_DOUBLE_EQ (normalizedCoeffs.b0, 1.0);   // 2.0/2.0
    EXPECT_DOUBLE_EQ (normalizedCoeffs.b1, 0.5);   // 1.0/2.0
    EXPECT_DOUBLE_EQ (normalizedCoeffs.b2, 0.25);  // 0.5/2.0
    EXPECT_DOUBLE_EQ (normalizedCoeffs.a1, -0.5);  // -1.0/2.0
    EXPECT_DOUBLE_EQ (normalizedCoeffs.a2, 0.125); // 0.25/2.0
}

TEST_F (BiquadFilterTests, SampleProcessing)
{
    // Set up a simple lowpass filter
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);

    for (int i = 0; i < 10; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (BiquadFilterTests, BlockProcessing)
{
    // Set up a bandpass filter
    auto coeffs = FilterDesigner<double>::designRbjBandpass (1000.0, 2.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    filterFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (BiquadFilterTests, InPlaceProcessing)
{
    auto coeffs = FilterDesigner<double>::designRbjHighpass (500.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);

    std::vector<float> data = testData; // Copy for in-place processing
    filterFloat.processInPlace (data.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (data[i]));
    }
}

TEST_F (BiquadFilterTests, TopologyEquivalence)
{
    // Test that all topologies produce equivalent results for the same coefficients
    auto coeffs = FilterDesigner<double>::designRbjPeak (1000.0, 1.0, 6.0, sampleRate);

    BiquadFloat filter1 (BiquadFloat::Topology::directFormI);
    BiquadFloat filter2 (BiquadFloat::Topology::directFormII);
    BiquadFloat filter3 (BiquadFloat::Topology::transposedDirectFormII);

    filter1.prepare (sampleRate, blockSize);
    filter2.prepare (sampleRate, blockSize);
    filter3.prepare (sampleRate, blockSize);

    filter1.setCoefficients (coeffs);
    filter2.setCoefficients (coeffs);
    filter3.setCoefficients (coeffs);

    std::vector<float> output1 (blockSize);
    std::vector<float> output2 (blockSize);
    std::vector<float> output3 (blockSize);

    filter1.processBlock (testData.data(), output1.data(), blockSize);
    filter2.processBlock (testData.data(), output2.data(), blockSize);
    filter3.processBlock (testData.data(), output3.data(), blockSize);

    // All topologies should produce nearly identical results
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (output1[i], output2[i], toleranceF);
        EXPECT_NEAR (output2[i], output3[i], toleranceF);
    }
}

TEST_F (BiquadFilterTests, StateReset)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Process some samples to build up internal state
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (1.0f);

    auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, the output should be closer to zero
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
}

TEST_F (BiquadFilterTests, FrequencyResponse)
{
    // Test lowpass filter response
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // DC response should be close to 1.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_NEAR (dcResponse, 1.0, 0.1);

    // Cutoff frequency response should be about -3dB (0.707)
    auto cutoffResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    EXPECT_NEAR (cutoffResponse, 0.707, 0.1);

    // High frequency should be attenuated
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_LT (highFreqResponse, 0.5);
}

TEST_F (BiquadFilterTests, HighpassFrequencyResponse)
{
    auto coeffs = FilterDesigner<double>::designRbjHighpass (1000.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // DC response should be close to 0.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_LT (dcResponse, 0.1);

    // High frequency should pass
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_GT (highFreqResponse, 0.7);
}

TEST_F (BiquadFilterTests, PolesAndZeros)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    filterDouble.setCoefficients (coeffs);

    std::vector<std::complex<double>> poles, zeros;
    filterDouble.getPolesZeros (poles, zeros);

    // A second-order filter should have at most 2 poles and 2 zeros
    EXPECT_LE (poles.size(), 2u);
    EXPECT_LE (zeros.size(), 2u);

    // For a stable filter, all poles should be inside the unit circle
    for (const auto& pole : poles)
    {
        EXPECT_LT (std::abs (pole), 1.0 + tolerance);
    }
}

TEST_F (BiquadFilterTests, FloatVsDoublePrecision)
{
    auto coeffs = FilterDesigner<double>::designRbjPeak (1000.0, 1.0, 3.0, sampleRate);

    filterFloat.setCoefficients (coeffs);
    filterDouble.setCoefficients (coeffs);

    std::vector<float> outputFloat (blockSize);
    std::vector<double> outputDouble (blockSize);

    filterFloat.processBlock (testData.data(), outputFloat.data(), blockSize);
    filterDouble.processBlock (doubleTestData.data(), outputDouble.data(), blockSize);

    // Results should be close but not identical due to precision differences
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_NEAR (outputFloat[i], static_cast<float> (outputDouble[i]), 1e-4f);
    }
}

TEST_F (BiquadFilterTests, ZeroInput)
{
    auto coeffs = FilterDesigner<double>::designRbjPeak (1000.0, 1.0, 6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    for (int i = 0; i < 100; ++i)
    {
        auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (BiquadFilterTests, ImpulseResponse)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, sampleRate);
    filterFloat.setCoefficients (coeffs);
    filterFloat.reset();

    std::vector<float> impulseResponse (128);
    for (int i = 0; i < 128; ++i)
    {
        float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay over time
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), std::abs (impulseResponse[50]));
}

TEST_F (BiquadFilterTests, HighQStability)
{
    // Test with very high Q factor
    auto coeffs = FilterDesigner<double>::designRbjBandpass (1000.0, 50.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    for (int i = 0; i < 1000; ++i)
    {
        auto output = filterFloat.processSample (0.01f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (BiquadFilterTests, ExtremeCoefficientValues)
{
    // Test with very small coefficients
    BiquadCoefficients<double> smallCoeffs (1e-6, 1e-7, 1e-8, 1.0, 1e-6, 1e-7);
    filterFloat.setCoefficients (smallCoeffs);

    auto output = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output));
}
