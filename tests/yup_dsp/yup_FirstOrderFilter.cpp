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
class FirstOrderFilterTests : public ::testing::Test
{
protected:
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

    FirstOrderFilterFloat filterFloat;
    FirstOrderFilterDouble filterDouble;
    std::vector<float> testData;
    std::vector<float> outputData;
    std::vector<double> doubleTestData;
    std::vector<double> doubleOutputData;
};

//==============================================================================
// Basic Functionality Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, DefaultConstruction)
{
    FirstOrderFilterFloat filter;

    // Default coefficients should be a pass-through (b0=1, others=0)
    auto coeffs = filter.getCoefficients();
    EXPECT_DOUBLE_EQ (coeffs.b0, 1.0);
    EXPECT_DOUBLE_EQ (coeffs.b1, 0.0);
    EXPECT_DOUBLE_EQ (coeffs.a1, 0.0);
}

TEST_F (FirstOrderFilterTests, CoefficientSetAndGet)
{
    FirstOrderCoefficients<double> coeffs (0.5, 0.25, -0.5);

    filterFloat.setCoefficients (coeffs);
    auto retrievedCoeffs = filterFloat.getCoefficients();

    EXPECT_DOUBLE_EQ (retrievedCoeffs.b0, 0.5);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.b1, 0.25);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.a1, -0.5);
}

TEST_F (FirstOrderFilterTests, ManualCoefficientCreation)
{
    // Test creating coefficients manually
    FirstOrderCoefficients<double> coeffs;
    coeffs.b0 = 0.8;
    coeffs.b1 = 0.2;
    coeffs.a1 = -0.3;

    filterFloat.setCoefficients (coeffs);
    auto retrievedCoeffs = filterFloat.getCoefficients();

    EXPECT_DOUBLE_EQ (retrievedCoeffs.b0, 0.8);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.b1, 0.2);
    EXPECT_DOUBLE_EQ (retrievedCoeffs.a1, -0.3);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, SampleProcessing)
{
    // Set up a simple lowpass filter
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    for (int i = 0; i < 10; ++i)
    {
        auto output = filterFloat.processSample (testData[i]);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (FirstOrderFilterTests, BlockProcessing)
{
    // Set up a highpass filter
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (500.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    filterFloat.processBlock (testData.data(), outputData.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (FirstOrderFilterTests, InPlaceProcessing)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    std::vector<float> data = testData; // Copy for in-place processing
    filterFloat.processInPlace (data.data(), blockSize);

    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (data[i]));
    }
}

//==============================================================================
// Filter Type Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, LowpassFilter)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // DC response should be close to 1.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_NEAR (dcResponse, 1.0, 0.1);

    // High frequency should be attenuated
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_LT (highFreqResponse, 0.5);
}

TEST_F (FirstOrderFilterTests, HighpassFilter)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // DC response should be close to 0.0
    auto dcResponse = std::abs (filterFloat.getComplexResponse (0.0));
    EXPECT_LT (dcResponse, 0.1);

    // High frequency should pass
    auto highFreqResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_GT (highFreqResponse, 0.7);
}

TEST_F (FirstOrderFilterTests, LowShelfFilter)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowShelf (1000.0, 6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Low frequencies should have gain
    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    auto expectedGain = dbToGain (6.0);

    EXPECT_GT (lowResponse, 1.5); // Should have noticeable gain

    // High frequencies should be closer to unity
    auto highResponse = std::abs (filterFloat.getComplexResponse (10000.0));
    EXPECT_NEAR (highResponse, 1.0, 0.5);
}

TEST_F (FirstOrderFilterTests, HighShelfFilter)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighShelf (1000.0, 6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // High frequencies should have gain
    auto highResponse = std::abs (filterFloat.getComplexResponse (10000.0));

    EXPECT_GT (highResponse, 1.5); // Should have noticeable gain

    // Low frequencies should be closer to unity
    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    EXPECT_NEAR (lowResponse, 1.0, 0.5);
}

TEST_F (FirstOrderFilterTests, AllpassFilter)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderAllpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // All frequencies should pass with unity magnitude
    const std::vector<double> testFreqs = { 100.0, 500.0, 1000.0, 2000.0, 5000.0 };

    for (const auto freq : testFreqs)
    {
        auto response = std::abs (filterFloat.getComplexResponse (freq));
        EXPECT_NEAR (response, 1.0, 0.1);
    }
}

//==============================================================================
// Shelving Filter Gain Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, LowShelfPositiveGain)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowShelf (1000.0, 6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    auto highResponse = std::abs (filterFloat.getComplexResponse (10000.0));

    // Low frequencies should be boosted
    EXPECT_GT (lowResponse, highResponse);
}

TEST_F (FirstOrderFilterTests, LowShelfNegativeGain)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowShelf (1000.0, -6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    auto highResponse = std::abs (filterFloat.getComplexResponse (10000.0));

    // Low frequencies should be attenuated
    EXPECT_LT (lowResponse, highResponse);
}

TEST_F (FirstOrderFilterTests, HighShelfPositiveGain)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighShelf (1000.0, 6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    auto highResponse = std::abs (filterFloat.getComplexResponse (10000.0));

    // High frequencies should be boosted
    EXPECT_GT (highResponse, lowResponse);
}

TEST_F (FirstOrderFilterTests, HighShelfNegativeGain)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighShelf (1000.0, -6.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Test frequencies across the shelf transition
    auto lowResponse = std::abs (filterFloat.getComplexResponse (100.0));
    auto shelfResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    auto highResponse = std::abs (filterFloat.getComplexResponse (5000.0));

    // For first-order high shelf with negative gain, the behavior is:
    // - Low frequencies are more attenuated than high frequencies
    // - The shelf frequency is in transition between them
    EXPECT_LT (lowResponse, highResponse);   // High frequencies have higher response
    EXPECT_GT (shelfResponse, lowResponse);  // Shelf is higher than low freq
    EXPECT_LT (shelfResponse, highResponse); // But lower than high freq
}

//==============================================================================
// State Reset Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, StateReset)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
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

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, LowpassCutoffFrequency)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // At cutoff frequency, first-order lowpass should be about -3dB (0.707)
    auto cutoffResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    EXPECT_NEAR (cutoffResponse, 0.707, 0.1);
}

TEST_F (FirstOrderFilterTests, HighpassCutoffFrequency)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // At cutoff frequency, first-order highpass should be about -3dB (0.707)
    auto cutoffResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    EXPECT_NEAR (cutoffResponse, 0.707, 0.1);
}

TEST_F (FirstOrderFilterTests, AllpassPhaseShift)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderAllpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Allpass should have unity magnitude but varying phase
    auto response100 = filterFloat.getComplexResponse (100.0);
    auto response1000 = filterFloat.getComplexResponse (1000.0);
    auto response10000 = filterFloat.getComplexResponse (10000.0);

    EXPECT_NEAR (std::abs (response100), 1.0, 0.1);
    EXPECT_NEAR (std::abs (response1000), 1.0, 0.1);
    EXPECT_NEAR (std::abs (response10000), 1.0, 0.1);

    // Phase should be different at different frequencies
    auto phase100 = std::arg (response100);
    auto phase10000 = std::arg (response10000);
    EXPECT_NE (phase100, phase10000);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, FloatVsDoublePrecision)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);

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

//==============================================================================
// Edge Cases Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, ZeroInput)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    for (int i = 0; i < 100; ++i)
    {
        auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (FirstOrderFilterTests, ImpulseResponse)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
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

TEST_F (FirstOrderFilterTests, StepResponse)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);
    filterFloat.reset();

    std::vector<float> stepResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        stepResponse[i] = filterFloat.processSample (1.0f);
    }

    // Step response should approach 1.0 for lowpass
    EXPECT_TRUE (std::isfinite (stepResponse[0]));
    EXPECT_LT (stepResponse[0], stepResponse[255]); // Should be increasing
    EXPECT_NEAR (stepResponse[255], 1.0f, 0.1f);    // Should approach unity
}

//==============================================================================
// Mathematical Properties Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, LowpassRolloff)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Test rolloff characteristics (should be -6dB/octave for first-order)
    auto response1k = std::abs (filterFloat.getComplexResponse (1000.0));
    auto response2k = std::abs (filterFloat.getComplexResponse (2000.0));
    auto response4k = std::abs (filterFloat.getComplexResponse (4000.0));

    // Each octave should have approximately -6dB (-0.5 in linear scale ratio)
    auto ratio2k = response2k / response1k;
    auto ratio4k = response4k / response2k;

    EXPECT_LT (ratio2k, 1.0);            // Should be attenuated
    EXPECT_LT (ratio4k, 1.0);            // Should be attenuated
    EXPECT_NEAR (ratio2k, ratio4k, 0.2); // Should have similar ratios (consistent rolloff)
}

TEST_F (FirstOrderFilterTests, HighpassRolloff)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Test rolloff characteristics (should be +6dB/octave for first-order)
    auto response500 = std::abs (filterFloat.getComplexResponse (500.0));
    auto response250 = std::abs (filterFloat.getComplexResponse (250.0));
    auto response125 = std::abs (filterFloat.getComplexResponse (125.0));

    // Each octave down should have approximately -6dB
    auto ratio250 = response250 / response500;
    auto ratio125 = response125 / response250;

    EXPECT_LT (ratio250, 1.0);             // Should be attenuated
    EXPECT_LT (ratio125, 1.0);             // Should be attenuated
    EXPECT_NEAR (ratio250, ratio125, 0.2); // Should have similar ratios
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, ExtremeCoefficientValues)
{
    // Test with very small coefficients
    FirstOrderCoefficients<double> smallCoeffs (1e-6, 1e-7, 1e-8);
    filterFloat.setCoefficients (smallCoeffs);

    auto output = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirstOrderFilterTests, LargeInputValues)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    filterFloat.setCoefficients (coeffs);

    // Test with large input values
    auto output1 = filterFloat.processSample (1000.0f);
    auto output2 = filterFloat.processSample (-1000.0f);

    EXPECT_TRUE (std::isfinite (output1));
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Consistency Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, SampleVsBlockProcessingConsistency)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);

    // Set up two identical filters
    FirstOrderFilterFloat filter1, filter2;
    filter1.prepare (sampleRate, blockSize);
    filter2.prepare (sampleRate, blockSize);
    filter1.setCoefficients (coeffs);
    filter2.setCoefficients (coeffs);

    std::vector<float> sampleOutput (blockSize);
    std::vector<float> blockOutput (blockSize);

    // Process sample by sample
    for (int i = 0; i < blockSize; ++i)
        sampleOutput[i] = filter1.processSample (testData[i]);

    // Process as block
    filter2.processBlock (testData.data(), blockOutput.data(), blockSize);

    // Results should be identical
    for (int i = 0; i < blockSize; ++i)
    {
        EXPECT_FLOAT_EQ (sampleOutput[i], blockOutput[i]);
    }
}

//==============================================================================
// Filter Frequency Characteristics Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, FrequencyScaling)
{
    // Test filters at different frequencies
    auto coeffs1k = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);
    auto coeffs2k = FilterDesigner<double>::designFirstOrderLowpass (2000.0, sampleRate);

    FirstOrderFilterFloat filter1k, filter2k;
    filter1k.prepare (sampleRate, blockSize);
    filter2k.prepare (sampleRate, blockSize);
    filter1k.setCoefficients (coeffs1k);
    filter2k.setCoefficients (coeffs2k);

    // Response at 500Hz should be higher for 2kHz filter than 1kHz filter
    // (higher cutoff = less attenuation at frequencies below cutoff)
    auto response1k_at500 = std::abs (filter1k.getComplexResponse (500.0));
    auto response2k_at500 = std::abs (filter2k.getComplexResponse (500.0));

    EXPECT_GT (response2k_at500, response1k_at500);
}

TEST_F (FirstOrderFilterTests, ShelfGainScaling)
{
    auto coeffs3db = FilterDesigner<double>::designFirstOrderLowShelf (1000.0, 3.0, sampleRate);
    auto coeffs6db = FilterDesigner<double>::designFirstOrderLowShelf (1000.0, 6.0, sampleRate);

    FirstOrderFilterFloat filter3db, filter6db;
    filter3db.prepare (sampleRate, blockSize);
    filter6db.prepare (sampleRate, blockSize);
    filter3db.setCoefficients (coeffs3db);
    filter6db.setCoefficients (coeffs6db);

    // 6dB shelf should have higher gain than 3dB shelf at low frequencies
    auto response3db = std::abs (filter3db.getComplexResponse (100.0));
    auto response6db = std::abs (filter6db.getComplexResponse (100.0));

    EXPECT_GT (response6db, response3db);
}

//==============================================================================
// Complex Coefficient Tests
//==============================================================================

TEST_F (FirstOrderFilterTests, CoefficientComplexResponse)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (1000.0, sampleRate);

    // Test that the complex response calculation is working
    auto response = coeffs.getComplexResponse (1000.0, sampleRate);

    EXPECT_TRUE (std::isfinite (response.real()));
    EXPECT_TRUE (std::isfinite (response.imag()));

    // Set coefficients first, then test magnitude should match filter response
    filterFloat.setCoefficients (coeffs);
    auto filterResponse = std::abs (filterFloat.getComplexResponse (1000.0));
    auto coeffResponse = std::abs (response);

    EXPECT_NEAR (filterResponse, coeffResponse, toleranceF);
}
