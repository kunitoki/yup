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
constexpr double tolerance = 1e-6;
constexpr float toleranceF = 1e-5f;
constexpr double sampleRate = 44100.0;
constexpr int blockSize = 256;
} // namespace

//==============================================================================
class NotchFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    NotchFilterFloat filterFloat;
    NotchFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (NotchFilterTests, DefaultConstruction)
{
    NotchFilterFloat filter;
    EXPECT_EQ (filter.getAlgorithm(), NotchFilter<float>::Algorithm::allpass);
    EXPECT_FLOAT_EQ (filter.getFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getDepth(), 0.9f);
    EXPECT_FLOAT_EQ (filter.getBoost(), 0.0f);
}

TEST_F (NotchFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 0.5f, sampleRate, NotchFilter<float>::Algorithm::biquad);

    EXPECT_EQ (filterFloat.getAlgorithm(), NotchFilter<float>::Algorithm::biquad);
    EXPECT_FLOAT_EQ (filterFloat.getFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getDepth(), 0.5f);
}

TEST_F (NotchFilterTests, DepthLimits)
{
    // Test minimum depth
    filterFloat.setDepth (-0.1f);
    EXPECT_GE (filterFloat.getDepth(), 0.0f);

    // Test maximum depth
    filterFloat.setDepth (1.5f);
    EXPECT_LE (filterFloat.getDepth(), 1.0f);

    // Test valid range
    filterFloat.setDepth (0.7f);
    EXPECT_FLOAT_EQ (filterFloat.getDepth(), 0.7f);
}

TEST_F (NotchFilterTests, BoostLimits)
{
    // Test minimum boost
    filterFloat.setBoost (-1.5f);
    EXPECT_GE (filterFloat.getBoost(), -1.0f);

    // Test maximum boost
    filterFloat.setBoost (1.5f);
    EXPECT_LE (filterFloat.getBoost(), 1.0f);

    // Test valid range
    filterFloat.setBoost (0.3f);
    EXPECT_FLOAT_EQ (filterFloat.getBoost(), 0.3f);
}

TEST_F (NotchFilterTests, AlgorithmSwitching)
{
    filterFloat.setAlgorithm (NotchFilter<float>::Algorithm::allpass);
    EXPECT_EQ (filterFloat.getAlgorithm(), NotchFilter<float>::Algorithm::allpass);

    filterFloat.setAlgorithm (NotchFilter<float>::Algorithm::biquad);
    EXPECT_EQ (filterFloat.getAlgorithm(), NotchFilter<float>::Algorithm::biquad);

    filterFloat.setAlgorithm (NotchFilter<float>::Algorithm::cutboost);
    EXPECT_EQ (filterFloat.getAlgorithm(), NotchFilter<float>::Algorithm::cutboost);
}

//==============================================================================
// Notch Characteristic Tests - Allpass Algorithm
//==============================================================================

TEST_F (NotchFilterTests, AllpassNotchCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.9f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Response at notch frequency should be deeply attenuated
    const auto notchResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (notchResponse, 0.2f);

    // Response away from notch should be relatively unaffected
    const auto response500Hz = filterFloat.getMagnitudeResponse (500.0f);
    const auto response2000Hz = filterFloat.getMagnitudeResponse (2000.0f);

    EXPECT_GT (response500Hz, 0.7f);
    EXPECT_GT (response2000Hz, 0.7f);

    // Should show characteristic notch shape
    const auto responseNear = filterFloat.getMagnitudeResponse (900.0f);
    EXPECT_GT (responseNear, notchResponse);
    EXPECT_LT (responseNear, response500Hz);
}

TEST_F (NotchFilterTests, DISABLED_AllpassDepthEffect)
{
    // Test shallow notch
    filterFloat.setParameters (1000.0f, 0.3f, sampleRate, NotchFilter<float>::Algorithm::allpass);
    const auto shallowNotchResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // Test deep notch
    filterFloat.setDepth (0.9f);
    const auto deepNotchResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // Deep notch should provide more attenuation
    EXPECT_LT (deepNotchResponse, shallowNotchResponse);
    EXPECT_GT (shallowNotchResponse, 0.5f); // Shallow should be less attenuated
    EXPECT_LT (deepNotchResponse, 0.3f);    // Deep should be well attenuated
}

//==============================================================================
// Notch Characteristic Tests - Biquad Algorithm
//==============================================================================

TEST_F (NotchFilterTests, DISABLED_BiquadNotchCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::biquad);

    // Response at notch frequency should be attenuated
    const auto notchResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (notchResponse, 0.3f);

    // Response away from notch should pass through
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (10000.0f);

    EXPECT_GT (dcResponse, 0.7f);
    EXPECT_GT (highFreqResponse, 0.7f);

    // Should show notch bandwidth
    const auto responseNear1 = filterFloat.getMagnitudeResponse (800.0f);
    const auto responseNear2 = filterFloat.getMagnitudeResponse (1250.0f);

    EXPECT_GT (responseNear1, notchResponse);
    EXPECT_GT (responseNear2, notchResponse);
}

TEST_F (NotchFilterTests, BiquadDepthEffect)
{
    // Test different depths
    std::vector<float> depths = { 0.2f, 0.5f, 0.8f };
    std::vector<float> responses;

    for (const auto depth : depths)
    {
        filterFloat.setParameters (1000.0f, depth, sampleRate, NotchFilter<float>::Algorithm::biquad);
        responses.push_back (filterFloat.getMagnitudeResponse (1000.0f));
    }

    // Higher depth should provide more attenuation
    EXPECT_GT (responses[0], responses[1]); // 0.2 > 0.5
    EXPECT_GT (responses[1], responses[2]); // 0.5 > 0.8
}

//==============================================================================
// Cut/Boost Algorithm Tests
//==============================================================================

TEST_F (NotchFilterTests, CutBoostNotchMode)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::cutboost);
    filterFloat.setBoost (-0.5f); // Negative boost = cut/notch

    // Should create a notch when boost is negative
    const auto notchResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);

    EXPECT_LT (notchResponse, sideResponse);
    EXPECT_LT (notchResponse, 0.8f);
}

TEST_F (NotchFilterTests, DISABLED_CutBoostPeakMode)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::cutboost);
    filterFloat.setBoost (0.5f); // Positive boost = peak

    // Should create a peak when boost is positive
    const auto peakResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);

    EXPECT_GT (peakResponse, sideResponse);
    EXPECT_GT (peakResponse, 1.0f);
}

TEST_F (NotchFilterTests, DISABLED_CutBoostNeutralMode)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::cutboost);
    filterFloat.setBoost (0.0f); // Zero boost = neutral

    // Should have minimal effect when boost is zero
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);

    EXPECT_NEAR (centerResponse, sideResponse, 0.2f);
    EXPECT_NEAR (centerResponse, 1.0f, 0.3f);
}

//==============================================================================
// Bandwidth Tests
//==============================================================================

TEST_F (NotchFilterTests, BandwidthEstimation)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    const auto bandwidth = filterFloat.getBandwidth3dB();
    EXPECT_GT (bandwidth, 0.0f);
    EXPECT_LT (bandwidth, 1000.0f); // Should be reasonable fraction of center frequency

    // Shallower notch should have wider bandwidth
    filterFloat.setDepth (0.3f);
    const auto wideBandwidth = filterFloat.getBandwidth3dB();
    EXPECT_GT (wideBandwidth, bandwidth);
}

TEST_F (NotchFilterTests, NotchSharpness)
{
    filterFloat.setParameters (1000.0f, 0.9f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Measure response at multiple frequencies around the notch
    std::vector<float> frequencies = { 900.0f, 950.0f, 1000.0f, 1050.0f, 1100.0f };
    std::vector<float> responses;

    for (const auto freq : frequencies)
    {
        responses.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Should show characteristic notch shape
    EXPECT_GT (responses[0], responses[1]); // 900 > 950
    EXPECT_GT (responses[1], responses[2]); // 950 > 1000 (center)
    EXPECT_LT (responses[2], responses[3]); // 1000 < 1050
    EXPECT_LT (responses[3], responses[4]); // 1050 < 1100

    // Center should be minimum
    const auto minResponse = *std::min_element (responses.begin(), responses.end());
    EXPECT_FLOAT_EQ (minResponse, responses[2]); // Center frequency
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (NotchFilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (NotchFilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 0.7f, sampleRate, NotchFilter<float>::Algorithm::biquad);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal at notch frequency
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }

    // Output should be significantly attenuated
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto inputRMS = calculateRMS (input);
    const auto outputRMS = calculateRMS (output);

    EXPECT_LT (outputRMS, inputRMS * 0.5f); // Should be significantly attenuated
}

TEST_F (NotchFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));

    // Should show characteristic ringing at notch frequency
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);

    // Check for overall stability
    for (const auto sample : impulseResponse)
    {
        EXPECT_TRUE (std::isfinite (sample));
    }
}

//==============================================================================
// Algorithm Comparison Tests
//==============================================================================

TEST_F (NotchFilterTests, DISABLED_AlgorithmComparison)
{
    // Test all three algorithms with same parameters
    const float freq = 1000.0f;
    const float depth = 0.8f;

    NotchFilterFloat allpassFilter, biquadFilter, cutboostFilter;

    allpassFilter.prepare (sampleRate, blockSize);
    biquadFilter.prepare (sampleRate, blockSize);
    cutboostFilter.prepare (sampleRate, blockSize);

    allpassFilter.setParameters (freq, depth, sampleRate, NotchFilter<float>::Algorithm::allpass);
    biquadFilter.setParameters (freq, depth, sampleRate, NotchFilter<float>::Algorithm::biquad);
    cutboostFilter.setParameters (freq, depth, sampleRate, NotchFilter<float>::Algorithm::cutboost);
    cutboostFilter.setBoost (-0.5f); // Set to notch mode

    // All should create notches at the target frequency
    const auto allpassNotch = allpassFilter.getMagnitudeResponse (freq);
    const auto biquadNotch = biquadFilter.getMagnitudeResponse (freq);
    const auto cutboostNotch = cutboostFilter.getMagnitudeResponse (freq);

    EXPECT_LT (allpassNotch, 0.5f);
    EXPECT_LT (biquadNotch, 0.5f);
    EXPECT_LT (cutboostNotch, 1.0f); // May be less deep due to boost setting

    // All should preserve frequencies away from notch
    const auto allpassSide = allpassFilter.getMagnitudeResponse (500.0f);
    const auto biquadSide = biquadFilter.getMagnitudeResponse (500.0f);
    const auto cutboostSide = cutboostFilter.getMagnitudeResponse (500.0f);

    EXPECT_GT (allpassSide, 0.7f);
    EXPECT_GT (biquadSide, 0.7f);
    EXPECT_GT (cutboostSide, 0.7f);
}

TEST_F (NotchFilterTests, PhaseCharacteristics)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Test phase response at various frequencies
    std::vector<float> frequencies = { 500.0f, 1000.0f, 2000.0f };

    for (const auto freq : frequencies)
    {
        const auto response = filterFloat.getComplexResponse (freq);
        const auto phase = std::atan2 (response.imag(), response.real());

        EXPECT_TRUE (std::isfinite (phase));

        // Phase should be continuous (no huge jumps)
        EXPECT_GT (phase, -MathConstants<float>::pi - 0.1f);
        EXPECT_LT (phase, MathConstants<float>::pi + 0.1f);
    }
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (NotchFilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 0.8, sampleRate, NotchFilter<double>::Algorithm::allpass);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (NotchFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::biquad);
    filterDouble.setParameters (1000.0, 0.8, sampleRate, NotchFilter<double>::Algorithm::biquad);

    const int numSamples = 50;
    std::vector<float> inputF (numSamples, 0.1f);
    std::vector<double> inputD (numSamples, 0.1);
    std::vector<float> outputF (numSamples);
    std::vector<double> outputD (numSamples);

    filterFloat.processBlock (inputF.data(), outputF.data(), numSamples);
    filterDouble.processBlock (inputD.data(), outputD.data(), numSamples);

    // Results should be similar within reasonable tolerance
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR (outputF[i], static_cast<float> (outputD[i]), 1e-3f);
    }
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F (NotchFilterTests, DISABLED_StabilityAllAlgorithms)
{
    std::vector<NotchFilter<float>::Algorithm> algorithms = {
        NotchFilter<float>::Algorithm::allpass,
        NotchFilter<float>::Algorithm::biquad,
        NotchFilter<float>::Algorithm::cutboost
    };

    for (const auto alg : algorithms)
    {
        filterFloat.setParameters (1000.0f, 0.9f, sampleRate, alg);

        // Test stability with various signals
        for (int i = 0; i < 1000; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
            EXPECT_LT (std::abs (output), 10.0f);
        }
    }
}

TEST_F (NotchFilterTests, DISABLED_ExtremeParameterStability)
{
    // Test with extreme depth
    filterFloat.setParameters (1000.0f, 1.0f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Test with very low frequency
    filterFloat.setParameters (10.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::biquad);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Test with very high frequency
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (nyquist, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (NotchFilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (NotchFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 0.5f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, 0.9f, sampleRate, NotchFilter<float>::Algorithm::biquad);

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

TEST_F (NotchFilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 0.8f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (NotchFilterTests, DISABLED_ConstantInput)
{
    filterFloat.setParameters (1000.0f, 0.6f, sampleRate, NotchFilter<float>::Algorithm::biquad);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For constant input, notch filter should eventually pass it through
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be close to input value (DC should pass)
    EXPECT_NEAR (output, constantInput, 0.2f);
}

//==============================================================================
// Application Scenario Tests
//==============================================================================

TEST_F (NotchFilterTests, DISABLED_HumRemovalScenario)
{
    // Simulate 50Hz hum removal
    filterFloat.setParameters (50.0f, 0.9f, sampleRate, NotchFilter<float>::Algorithm::allpass);

    // Create signal with 50Hz hum + audio content
    std::vector<float> outputs;
    outputs.reserve (1000);

    for (int i = 0; i < 1000; ++i)
    {
        const float audioSignal = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 440.0f * i / static_cast<float> (sampleRate));
        const float hum = 0.3f * std::sin (2.0f * MathConstants<float>::pi * 50.0f * i / static_cast<float> (sampleRate));
        const float input = audioSignal + hum;

        outputs.push_back (filterFloat.processSample (input));
    }

    // Should remove 50Hz while preserving 440Hz
    const auto response50Hz = filterFloat.getMagnitudeResponse (50.0f);
    const auto response440Hz = filterFloat.getMagnitudeResponse (440.0f);

    EXPECT_LT (response50Hz, 0.3f);  // 50Hz should be attenuated
    EXPECT_GT (response440Hz, 0.7f); // 440Hz should pass through

    // All outputs should be finite
    for (const auto output : outputs)
    {
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (NotchFilterTests, DISABLED_ParametricEQScenario)
{
    // Test cut/boost algorithm as parametric EQ
    filterFloat.setParameters (1000.0f, 0.7f, sampleRate, NotchFilter<float>::Algorithm::cutboost);

    // Test cutting
    filterFloat.setBoost (-0.6f);
    const auto cutResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (cutResponse, 0.8f);

    // Test boosting
    filterFloat.setBoost (0.6f);
    const auto boostResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_GT (boostResponse, 1.1f);

    // Both should be stable
    for (int i = 0; i < 100; ++i)
    {
        const auto output1 = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output1));
    }

    filterFloat.setBoost (-0.6f);
    for (int i = 0; i < 100; ++i)
    {
        const auto output2 = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output2));
    }
}
