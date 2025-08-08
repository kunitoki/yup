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
class RbjFilterTests : public ::testing::Test
{
protected:
    static constexpr double tolerance = 1e-6;
    static constexpr float toleranceF = 1e-5f;
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 256;

    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    RbjFilterFloat filterFloat;
    RbjFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (RbjFilterTests, DefaultConstruction)
{
    RbjFilterFloat filter;
    EXPECT_EQ (filter.getMode(), FilterMode::lowpass);
    EXPECT_FLOAT_EQ (filter.getFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getQ(), 0.707f);
    EXPECT_FLOAT_EQ (filter.getGain(), 0.0f);
}

TEST_F (RbjFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (FilterMode::peak, 2000.0f, 1.5f, 6.0f, sampleRate);

    EXPECT_EQ (filterFloat.getMode(), FilterMode::peak);
    EXPECT_FLOAT_EQ (filterFloat.getFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getQ(), 1.5f);
    EXPECT_FLOAT_EQ (filterFloat.getGain(), 6.0f);
}

TEST_F (RbjFilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test near-zero frequency
    filterFloat.setParameters (FilterMode::lowpass, 1.0f, 0.707f, 0.0f, sampleRate);
    EXPECT_GE (filterFloat.getFrequency(), 1.0f);

    // Test near-Nyquist frequency
    filterFloat.setParameters (FilterMode::lowpass, nyquist * 0.99f, 0.707f, 0.0f, sampleRate);
    EXPECT_LE (filterFloat.getFrequency(), nyquist);
}

TEST_F (RbjFilterTests, QFactorLimits)
{
    // Test minimum Q
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.01f, 0.0f, sampleRate);
    EXPECT_GE (filterFloat.getQ(), 0.01f);

    // Test very high Q
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 100.0f, 0.0f, sampleRate);
    EXPECT_LE (filterFloat.getQ(), 100.0f);
}

//==============================================================================
// Filter Type Tests
//==============================================================================

TEST_F (RbjFilterTests, LowpassFilter)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    // DC should pass through
    filterFloat.reset();
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_NEAR (dcResponse, 1.0f, 0.1f);

    // High frequency should be attenuated
    const auto responseAt5kHz = filterFloat.getMagnitudeResponse (5000.0f);
    EXPECT_LT (responseAt5kHz, 0.5f);
}

TEST_F (RbjFilterTests, HighpassFilter)
{
    filterFloat.setParameters (FilterMode::highpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    // DC should be blocked
    filterFloat.reset();
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_LT (std::abs (dcResponse), 0.1f);

    // High frequency should pass
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (responseAt10kHz, 0.7f);
}

TEST_F (RbjFilterTests, BandpassFilter)
{
    filterFloat.setParameters (FilterMode::bandpass, 1000.0f, 2.0f, 0.0f, sampleRate);

    // Center frequency should have good response
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_GT (centerResponse, 0.5f);

    // Frequencies far from center should be attenuated
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_LT (lowResponse, 0.3f);
    EXPECT_LT (highResponse, 0.3f);
}

TEST_F (RbjFilterTests, BandstopFilter)
{
    filterFloat.setParameters (FilterMode::bandstop, 1000.0f, 2.0f, 0.0f, sampleRate);

    // Center frequency should be attenuated
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (centerResponse, 0.3f);

    // Frequencies away from center should pass
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (lowResponse, 0.7f);
    EXPECT_GT (highResponse, 0.7f);
}

TEST_F (RbjFilterTests, PeakingFilter)
{
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 1.0f, 6.0f, sampleRate);

    // At center frequency, should provide the specified gain
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expectedGain = dbToGain (6.0f);

    EXPECT_NEAR (centerResponse, expectedGain, 0.2f);

    // Far from center, should be close to unity gain
    const auto farResponse = filterFloat.getMagnitudeResponse (100.0f);
    EXPECT_NEAR (farResponse, 1.0f, 0.2f);
}

TEST_F (RbjFilterTests, LowShelfFilter)
{
    filterFloat.setParameters (FilterMode::lowshelf, 1000.0f, 0.707f, 6.0f, sampleRate);

    // Low frequencies should have the specified gain
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto expectedGain = dbToGain (6.0f);

    EXPECT_NEAR (lowResponse, expectedGain, 0.3f);

    // High frequencies should be close to unity
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_NEAR (highResponse, 1.0f, 0.2f);
}

TEST_F (RbjFilterTests, HighShelfFilter)
{
    filterFloat.setParameters (FilterMode::highshelf, 1000.0f, 0.707f, 6.0f, sampleRate);

    // High frequencies should have the specified gain
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    const auto expectedGain = dbToGain (6.0f);

    EXPECT_NEAR (highResponse, expectedGain, 0.3f);

    // Low frequencies should be close to unity
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    EXPECT_NEAR (lowResponse, 1.0f, 0.2f);
}

TEST_F (RbjFilterTests, AllpassFilter)
{
    filterFloat.setParameters (FilterMode::allpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    // All frequencies should pass with unity magnitude
    const std::vector<float> testFreqs = { 100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };

    for (const auto freq : testFreqs)
    {
        const auto response = filterFloat.getMagnitudeResponse (freq);
        EXPECT_NEAR (response, 1.0f, 0.1f);
    }
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (RbjFilterTests, CutoffFrequencyResponse)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expected3dB = std::pow (10.0f, -3.0f / 20.0f); // -3dB in linear

    EXPECT_NEAR (responseAtCutoff, expected3dB, 0.15f);
}

TEST_F (RbjFilterTests, QFactorEffect)
{
    // Test low Q (broad response)
    filterFloat.setParameters (FilterMode::bandpass, 1000.0f, 0.5f, 0.0f, sampleRate);
    const auto lowQResponse = filterFloat.getMagnitudeResponse (1414.0f); // sqrt(2) * 1000

    // Test high Q (narrow response)
    filterFloat.setParameters (FilterMode::bandpass, 1000.0f, 5.0f, 0.0f, sampleRate);
    const auto highQResponse = filterFloat.getMagnitudeResponse (1414.0f);

    // High Q should have more attenuation away from center
    EXPECT_LT (highQResponse, lowQResponse);
}

TEST_F (RbjFilterTests, GainParameterEffect)
{
    // Positive gain
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 1.0f, 6.0f, sampleRate);
    const auto positiveGainResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // Negative gain
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 1.0f, -6.0f, sampleRate);
    const auto negativeGainResponse = filterFloat.getMagnitudeResponse (1000.0f);

    EXPECT_GT (positiveGainResponse, 1.0f);
    EXPECT_LT (negativeGainResponse, 1.0f);

    // They should be approximately reciprocals
    const auto product = positiveGainResponse * negativeGainResponse;
    EXPECT_NEAR (product, 1.0f, 0.2f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (RbjFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (RbjFilterTests, BlockProcessing)
{
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 1.0f, 3.0f, sampleRate);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (RbjFilterTests, ImpulseResponse)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);
    filterFloat.reset();

    std::vector<float> impulseResponse (128);
    for (int i = 0; i < 128; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), std::abs (impulseResponse[50]));
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (RbjFilterTests, DoublePrecision)
{
    filterDouble.setParameters (FilterMode::peak, 1000.0, 0.707, 6.0, sampleRate);

    const double smallSignal = 1e-10;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (RbjFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);
    filterDouble.setParameters (FilterMode::lowpass, 1000.0, 0.707, 0.0, sampleRate);

    const int numSamples = 100;
    std::vector<float> inputF (numSamples, 0.1f);
    std::vector<double> inputD (numSamples, 0.1);
    std::vector<float> outputF (numSamples);
    std::vector<double> outputD (numSamples);

    filterFloat.processBlock (inputF.data(), outputF.data(), numSamples);
    filterDouble.processBlock (inputD.data(), outputD.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR (outputF[i], static_cast<float> (outputD[i]), 1e-4f);
    }
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F (RbjFilterTests, StabilityWithHighQ)
{
    // Very high Q can cause instability
    filterFloat.setParameters (FilterMode::bandpass, 1000.0f, 50.0f, 0.0f, sampleRate);

    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (RbjFilterTests, StabilityWithExtremeGain)
{
    // Very high gain
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 0.707f, 40.0f, sampleRate);

    const auto output1 = filterFloat.processSample (0.001f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very negative gain
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 0.707f, -40.0f, sampleRate);

    const auto output2 = filterFloat.processSample (0.001f);
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (RbjFilterTests, ResetClearsState)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
}

TEST_F (RbjFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (FilterMode::peak, 2000.0f, 2.0f, 6.0f, sampleRate);

    // Should continue processing without issues
    for (int i = 0; i < 50; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Edge Case Tests
//==============================================================================

TEST_F (RbjFilterTests, ZeroInput)
{
    filterFloat.setParameters (FilterMode::peak, 1000.0f, 1.0f, 6.0f, sampleRate);

    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_EQ (output, 0.0f);
    }
}

TEST_F (RbjFilterTests, ConstantInputLowpass)
{
    filterFloat.setParameters (FilterMode::lowpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually equal output
    for (int i = 0; i < 1000; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, constantInput, 0.1f);
}

TEST_F (RbjFilterTests, ConstantInputHighpass)
{
    filterFloat.setParameters (FilterMode::highpass, 1000.0f, 0.707f, 0.0f, sampleRate);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For highpass, constant input should eventually go to zero
    for (int i = 0; i < 1000; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, 0.0f, 0.1f);
}

TEST_F (RbjFilterTests, SinusoidalInput)
{
    filterFloat.setParameters (FilterMode::bandpass, 1000.0f, 2.0f, 0.0f, sampleRate);

    // Test with sinusoid at center frequency
    const float freq = 1000.0f;
    float maxOutput = 0.0f;

    for (int i = 0; i < 1000; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * freq * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        maxOutput = std::max (maxOutput, std::abs (output));
    }

    // Should have reasonable output for center frequency
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 2.0f);
}

//==============================================================================
// All Filter Types Comprehensive Test
//==============================================================================

TEST_F (RbjFilterTests, AllFilterTypesBasicFunctionality)
{
    const std::vector<FilterModeType> allTypes = {
        FilterMode::lowpass,
        FilterMode::highpass,
        FilterMode::bandpass,
        FilterMode::bandstop,
        FilterMode::peak,
        FilterMode::lowshelf,
        FilterMode::highshelf,
        FilterMode::allpass
    };

    for (const auto type : allTypes)
    {
        filterFloat.setParameters (type, 1000.0f, 0.707f, 3.0f, sampleRate);

        // Each type should process without throwing
        for (int i = 0; i < 10; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
        }

        filterFloat.reset();
    }
}
