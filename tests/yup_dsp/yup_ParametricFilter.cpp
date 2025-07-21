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
class ParametricFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    ParametricFilterFloat filterFloat;
    ParametricFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (ParametricFilterTests, DefaultConstruction)
{
    ParametricFilterFloat filter;
    EXPECT_EQ (filter.getType(), ParametricFilter<float>::Type::bell);
    EXPECT_FLOAT_EQ (filter.getFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getGain(), 0.0f);
    EXPECT_FLOAT_EQ (filter.getQ(), 1.0f);
    EXPECT_FALSE (filter.isBoosting());
    EXPECT_FALSE (filter.isCutting());
}

TEST_F (ParametricFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::bell);
    EXPECT_FLOAT_EQ (filterFloat.getFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getGain(), 6.0f);
    EXPECT_FLOAT_EQ (filterFloat.getQ(), 2.0f);
    EXPECT_TRUE (filterFloat.isBoosting());
    EXPECT_FALSE (filterFloat.isCutting());
}

TEST_F (ParametricFilterTests, GainLimits)
{
    // Test minimum gain
    filterFloat.setGain (-50.0f);
    EXPECT_GE (filterFloat.getGain(), -40.0f);

    // Test maximum gain
    filterFloat.setGain (50.0f);
    EXPECT_LE (filterFloat.getGain(), 40.0f);

    // Test valid range
    filterFloat.setGain (12.0f);
    EXPECT_FLOAT_EQ (filterFloat.getGain(), 12.0f);
}

TEST_F (ParametricFilterTests, DISABLED_QLimits)
{
    // Test minimum Q
    filterFloat.setQ (0.05f);
    EXPECT_GE (filterFloat.getQ(), 0.1f);

    // Test valid range
    filterFloat.setQ (5.0f);
    EXPECT_FLOAT_EQ (filterFloat.getQ(), 5.0f);
}

TEST_F (ParametricFilterTests, BandwidthConversion)
{
    // Test Q to bandwidth conversion
    filterFloat.setQ (1.0f);
    const auto bandwidth1 = filterFloat.getBandwidth();
    EXPECT_GT (bandwidth1, 0.0f);

    filterFloat.setQ (2.0f);
    const auto bandwidth2 = filterFloat.getBandwidth();
    EXPECT_LT (bandwidth2, bandwidth1); // Higher Q = narrower bandwidth

    // Test bandwidth to Q conversion
    filterFloat.setBandwidth (1.0f);
    const auto q1 = filterFloat.getQ();

    filterFloat.setBandwidth (2.0f);
    const auto q2 = filterFloat.getQ();
    EXPECT_LT (q2, q1); // Wider bandwidth = lower Q
}

TEST_F (ParametricFilterTests, TypeSwitching)
{
    filterFloat.setType (ParametricFilter<float>::Type::bell);
    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::bell);

    filterFloat.setType (ParametricFilter<float>::Type::lowShelf);
    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::lowShelf);

    filterFloat.setType (ParametricFilter<float>::Type::highShelf);
    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::highShelf);

    filterFloat.setType (ParametricFilter<float>::Type::notch);
    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::notch);

    filterFloat.setType (ParametricFilter<float>::Type::cutBoost);
    EXPECT_EQ (filterFloat.getType(), ParametricFilter<float>::Type::cutBoost);
}

//==============================================================================
// Bell Filter Tests
//==============================================================================

TEST_F (ParametricFilterTests, BellBoostCharacteristic)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // Response at center frequency should be boosted
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expectedGain = DspMath::dbToGain (6.0f);
    EXPECT_NEAR (centerResponse, expectedGain, 0.2f);

    // Response away from center should be unaffected
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);
    EXPECT_NEAR (sideResponse, 1.0f, 0.1f);

    // Should show bell-shaped response
    const auto response900Hz = filterFloat.getMagnitudeResponse (900.0f);
    const auto response1100Hz = filterFloat.getMagnitudeResponse (1100.0f);

    EXPECT_GT (response900Hz, sideResponse);
    EXPECT_GT (response1100Hz, sideResponse);
    EXPECT_LT (response900Hz, centerResponse);
    EXPECT_LT (response1100Hz, centerResponse);
}

TEST_F (ParametricFilterTests, BellCutCharacteristic)
{
    filterFloat.setParameters (1000.0f, -6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // Response at center frequency should be cut
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expectedGain = DspMath::dbToGain (-6.0f);
    EXPECT_NEAR (centerResponse, expectedGain, 0.2f);

    // Response away from center should be unaffected
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);
    EXPECT_NEAR (sideResponse, 1.0f, 0.1f);

    // Should show inverted bell-shaped response
    const auto response900Hz = filterFloat.getMagnitudeResponse (900.0f);
    const auto response1100Hz = filterFloat.getMagnitudeResponse (1100.0f);

    EXPECT_LT (response900Hz, sideResponse);
    EXPECT_LT (response1100Hz, sideResponse);
    EXPECT_GT (response900Hz, centerResponse);
    EXPECT_GT (response1100Hz, centerResponse);
}

TEST_F (ParametricFilterTests, BellQEffect)
{
    // Test narrow Q
    filterFloat.setParameters (1000.0f, 6.0f, 5.0f, sampleRate, ParametricFilter<float>::Type::bell);
    const auto narrowResponse800Hz = filterFloat.getMagnitudeResponse (800.0f);

    // Test wide Q
    filterFloat.setQ (0.5f);
    const auto wideResponse800Hz = filterFloat.getMagnitudeResponse (800.0f);

    // Wide Q should affect frequencies further from center more than narrow Q
    EXPECT_GT (wideResponse800Hz, narrowResponse800Hz);
}

//==============================================================================
// Low Shelf Filter Tests
//==============================================================================

TEST_F (ParametricFilterTests, DISABLED_LowShelfBoostCharacteristic)
{
    filterFloat.setParameters (1000.0f, 6.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::lowShelf);

    // Low frequencies should be boosted
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto expectedGain = DspMath::dbToGain (6.0f);

    EXPECT_NEAR (dcResponse, expectedGain, 0.3f);
    EXPECT_NEAR (lowFreqResponse, expectedGain, 0.3f);

    // High frequencies should be unaffected
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_NEAR (highFreqResponse, 1.0f, 0.2f);

    // Transition should occur around shelf frequency
    const auto transitionResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_GT (transitionResponse, 1.0f);
    EXPECT_LT (transitionResponse, expectedGain);
}

TEST_F (ParametricFilterTests, DISABLED_LowShelfCutCharacteristic)
{
    filterFloat.setParameters (1000.0f, -6.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::lowShelf);

    // Low frequencies should be cut
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto expectedGain = DspMath::dbToGain (-6.0f);

    EXPECT_NEAR (dcResponse, expectedGain, 0.3f);
    EXPECT_NEAR (lowFreqResponse, expectedGain, 0.3f);

    // High frequencies should be unaffected
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_NEAR (highFreqResponse, 1.0f, 0.2f);
}

//==============================================================================
// High Shelf Filter Tests
//==============================================================================

TEST_F (ParametricFilterTests, DISABLED_HighShelfBoostCharacteristic)
{
    filterFloat.setParameters (5000.0f, 6.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::highShelf);

    // High frequencies should be boosted
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (15000.0f);
    const auto expectedGain = DspMath::dbToGain (6.0f);
    EXPECT_NEAR (highFreqResponse, expectedGain, 0.3f);

    // Low frequencies should be unaffected
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (100.0f);
    EXPECT_NEAR (dcResponse, 1.0f, 0.2f);
    EXPECT_NEAR (lowFreqResponse, 1.0f, 0.2f);

    // Transition should occur around shelf frequency
    const auto transitionResponse = filterFloat.getMagnitudeResponse (5000.0f);
    EXPECT_GT (transitionResponse, 1.0f);
    EXPECT_LT (transitionResponse, expectedGain);
}

TEST_F (ParametricFilterTests, DISABLED_HighShelfCutCharacteristic)
{
    filterFloat.setParameters (5000.0f, -6.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::highShelf);

    // High frequencies should be cut
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (15000.0f);
    const auto expectedGain = DspMath::dbToGain (-6.0f);
    EXPECT_NEAR (highFreqResponse, expectedGain, 0.3f);

    // Low frequencies should be unaffected
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_NEAR (dcResponse, 1.0f, 0.2f);
}

//==============================================================================
// Notch Filter Tests
//==============================================================================

TEST_F (ParametricFilterTests, NotchCharacteristic)
{
    filterFloat.setParameters (1000.0f, -20.0f, 5.0f, sampleRate, ParametricFilter<float>::Type::notch);

    // Response at notch frequency should be deeply attenuated
    const auto notchResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (notchResponse, 0.2f);

    // Response away from notch should be unaffected
    const auto sideResponse1 = filterFloat.getMagnitudeResponse (500.0f);
    const auto sideResponse2 = filterFloat.getMagnitudeResponse (2000.0f);

    EXPECT_NEAR (sideResponse1, 1.0f, 0.1f);
    EXPECT_NEAR (sideResponse2, 1.0f, 0.1f);

    // Should show characteristic notch shape
    const auto response900Hz = filterFloat.getMagnitudeResponse (900.0f);
    const auto response1100Hz = filterFloat.getMagnitudeResponse (1100.0f);

    EXPECT_GT (response900Hz, notchResponse);
    EXPECT_GT (response1100Hz, notchResponse);
}

TEST_F (ParametricFilterTests, NotchQEffect)
{
    // Test narrow notch
    filterFloat.setParameters (1000.0f, -20.0f, 10.0f, sampleRate, ParametricFilter<float>::Type::notch);
    const auto narrowResponse950Hz = filterFloat.getMagnitudeResponse (950.0f);

    // Test wide notch
    filterFloat.setQ (1.0f);
    const auto wideResponse950Hz = filterFloat.getMagnitudeResponse (950.0f);

    // Wide Q should affect frequencies further from center more than narrow Q
    EXPECT_LT (wideResponse950Hz, narrowResponse950Hz);
}

//==============================================================================
// Cut/Boost Filter Tests
//==============================================================================

TEST_F (ParametricFilterTests, DISABLED_CutBoostAlgorithmBoost)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::cutBoost);

    // Should create a boost at center frequency
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_GT (centerResponse, 1.0f);

    // Should have minimal effect away from center
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);
    EXPECT_NEAR (sideResponse, 1.0f, 0.3f);
}

TEST_F (ParametricFilterTests, DISABLED_CutBoostAlgorithmCut)
{
    filterFloat.setParameters (1000.0f, -6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::cutBoost);

    // Should create a cut at center frequency
    const auto centerResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_LT (centerResponse, 1.0f);

    // Should have minimal effect away from center
    const auto sideResponse = filterFloat.getMagnitudeResponse (500.0f);
    EXPECT_NEAR (sideResponse, 1.0f, 0.3f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (ParametricFilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ParametricFilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal at center frequency
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }

    // Output should be boosted compared to input
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto inputRMS = calculateRMS (input);
    const auto outputRMS = calculateRMS (output);

    EXPECT_GT (outputRMS, inputRMS); // Should be boosted
}

TEST_F (ParametricFilterTests, ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));

    // Should show some ringing at center frequency
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
// Filter Type Comparison Tests
//==============================================================================

TEST_F (ParametricFilterTests, DISABLED_FilterTypeComparison)
{
    const float freq = 1000.0f;
    const float gain = 6.0f;
    const float Q = 2.0f;

    // Test all filter types with same parameters
    ParametricFilterFloat bellFilter, shelfFilter, notchFilter, cutboostFilter;

    bellFilter.prepare (sampleRate, blockSize);
    shelfFilter.prepare (sampleRate, blockSize);
    notchFilter.prepare (sampleRate, blockSize);
    cutboostFilter.prepare (sampleRate, blockSize);

    bellFilter.setParameters (freq, gain, Q, sampleRate, ParametricFilter<float>::Type::bell);
    shelfFilter.setParameters (freq, gain, Q, sampleRate, ParametricFilter<float>::Type::lowShelf);
    notchFilter.setParameters (freq, -20.0f, Q, sampleRate, ParametricFilter<float>::Type::notch);
    cutboostFilter.setParameters (freq, gain, Q, sampleRate, ParametricFilter<float>::Type::cutBoost);

    // Test response at center frequency
    const auto bellResponse = bellFilter.getMagnitudeResponse (freq);
    const auto shelfResponse = shelfFilter.getMagnitudeResponse (freq);
    const auto notchResponse = notchFilter.getMagnitudeResponse (freq);
    const auto cutboostResponse = cutboostFilter.getMagnitudeResponse (freq);

    // Bell and cutboost should boost at center frequency
    EXPECT_GT (bellResponse, 1.0f);
    EXPECT_GT (cutboostResponse, 1.0f);

    // Shelf should boost at center frequency (transition region)
    EXPECT_GT (shelfResponse, 1.0f);

    // Notch should cut at center frequency
    EXPECT_LT (notchResponse, 0.5f);

    // All should be stable
    EXPECT_TRUE (std::isfinite (bellResponse));
    EXPECT_TRUE (std::isfinite (shelfResponse));
    EXPECT_TRUE (std::isfinite (notchResponse));
    EXPECT_TRUE (std::isfinite (cutboostResponse));
}

//==============================================================================
// Gain and Q Interaction Tests
//==============================================================================

TEST_F (ParametricFilterTests, GainQInteraction)
{
    // Test various gain and Q combinations
    std::vector<float> gains = { -12.0f, -6.0f, 0.0f, 6.0f, 12.0f };
    std::vector<float> qs = { 0.5f, 1.0f, 2.0f, 5.0f };

    for (const auto gain : gains)
    {
        for (const auto q : qs)
        {
            filterFloat.setParameters (1000.0f, gain, q, sampleRate, ParametricFilter<float>::Type::bell);

            const auto response = filterFloat.getMagnitudeResponse (1000.0f);
            EXPECT_TRUE (std::isfinite (response));

            if (gain > 0.0f)
            {
                EXPECT_GT (response, 1.0f); // Should boost
            }
            else if (gain < 0.0f)
            {
                EXPECT_LT (response, 1.0f); // Should cut
            }
            else
            {
                EXPECT_NEAR (response, 1.0f, 0.1f); // Should be neutral
            }
        }
    }
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (ParametricFilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 6.0, 2.0, sampleRate, ParametricFilter<double>::Type::bell);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (ParametricFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);
    filterDouble.setParameters (1000.0, 6.0, 2.0, sampleRate, ParametricFilter<double>::Type::bell);

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

TEST_F (ParametricFilterTests, StabilityAllTypes)
{
    std::vector<ParametricFilter<float>::Type> types = {
        ParametricFilter<float>::Type::bell,
        ParametricFilter<float>::Type::lowShelf,
        ParametricFilter<float>::Type::highShelf,
        ParametricFilter<float>::Type::notch,
        ParametricFilter<float>::Type::cutBoost
    };

    for (const auto type : types)
    {
        filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, type);

        // Test stability with various signals
        for (int i = 0; i < 1000; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
            EXPECT_LT (std::abs (output), 10.0f);
        }
    }
}

TEST_F (ParametricFilterTests, ExtremeParameterStability)
{
    // Test with maximum gain and high Q
    filterFloat.setParameters (1000.0f, 40.0f, 10.0f, sampleRate, ParametricFilter<float>::Type::bell);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 100.0f);
    }

    // Test with minimum gain and high Q
    filterFloat.setParameters (1000.0f, -40.0f, 10.0f, sampleRate, ParametricFilter<float>::Type::bell);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (ParametricFilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (ParametricFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, -12.0f, 5.0f, sampleRate, ParametricFilter<float>::Type::notch);

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

TEST_F (ParametricFilterTests, ZeroGainBypass)
{
    filterFloat.setParameters (1000.0f, 0.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // With zero gain, filter should act as bypass
    std::vector<float> testFrequencies = { 100.0f, 1000.0f, 5000.0f };

    for (const auto freq : testFrequencies)
    {
        const auto response = filterFloat.getMagnitudeResponse (freq);
        EXPECT_NEAR (response, 1.0f, 0.1f);
    }
}

TEST_F (ParametricFilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Application Scenario Tests
//==============================================================================

TEST_F (ParametricFilterTests, DISABLED_MultibandEQScenario)
{
    // Simulate a 3-band parametric EQ
    ParametricFilterFloat lowFilter, midFilter, highFilter;

    lowFilter.prepare (sampleRate, blockSize);
    midFilter.prepare (sampleRate, blockSize);
    highFilter.prepare (sampleRate, blockSize);

    // Bass boost, mid cut, treble boost
    lowFilter.setParameters (100.0f, 3.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::lowShelf);
    midFilter.setParameters (1000.0f, -6.0f, 2.0f, sampleRate, ParametricFilter<float>::Type::bell);
    highFilter.setParameters (8000.0f, 4.0f, 1.0f, sampleRate, ParametricFilter<float>::Type::highShelf);

    // Test with broadband signal
    for (int i = 0; i < 1000; ++i)
    {
        const float input = 0.1f * std::sin (2.0f * MathConstants<float>::pi * 100.0f * i / static_cast<float> (sampleRate)) + 0.1f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate)) + 0.1f * std::sin (2.0f * MathConstants<float>::pi * 8000.0f * i / static_cast<float> (sampleRate));

        // Process through all three filters in series
        auto output = lowFilter.processSample (input);
        output = midFilter.processSample (output);
        output = highFilter.processSample (output);

        EXPECT_TRUE (std::isfinite (output));
    }

    // Verify frequency responses
    const auto lowResponse = lowFilter.getMagnitudeResponse (100.0f);
    const auto midResponse = midFilter.getMagnitudeResponse (1000.0f);
    const auto highResponse = highFilter.getMagnitudeResponse (8000.0f);

    EXPECT_GT (lowResponse, 1.0f);  // Bass boosted
    EXPECT_LT (midResponse, 1.0f);  // Mids cut
    EXPECT_GT (highResponse, 1.0f); // Treble boosted
}

TEST_F (ParametricFilterTests, FeedbackSuppressionScenario)
{
    // Use notch filter to suppress feedback at specific frequency
    filterFloat.setParameters (2400.0f, -30.0f, 20.0f, sampleRate, ParametricFilter<float>::Type::notch);

    // Test with signal containing feedback frequency
    std::vector<float> outputs;
    outputs.reserve (500);

    for (int i = 0; i < 500; ++i)
    {
        // Mix of audio (440Hz) and feedback (2400Hz)
        const float audioSignal = 0.3f * std::sin (2.0f * MathConstants<float>::pi * 440.0f * i / static_cast<float> (sampleRate));
        const float feedbackSignal = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 2400.0f * i / static_cast<float> (sampleRate));
        const float input = audioSignal + feedbackSignal;

        outputs.push_back (filterFloat.processSample (input));
    }

    // Verify frequency responses
    const auto audioResponse = filterFloat.getMagnitudeResponse (440.0f);
    const auto feedbackResponse = filterFloat.getMagnitudeResponse (2400.0f);

    EXPECT_NEAR (audioResponse, 1.0f, 0.1f); // Audio preserved
    EXPECT_LT (feedbackResponse, 0.1f);      // Feedback suppressed

    // All outputs should be finite
    for (const auto output : outputs)
    {
        EXPECT_TRUE (std::isfinite (output));
    }
}
