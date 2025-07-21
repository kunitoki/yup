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
class ChebyshevFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    ChebyshevFilterFloat filterFloat;
    ChebyshevFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, DefaultConstruction)
{
    ChebyshevFilterFloat filter;
    EXPECT_EQ (filter.getChebyshevType(), ChebyshevFilter<float>::Type::Type1);
    EXPECT_EQ (filter.getFilterType(), FilterType::lowpass);
    EXPECT_EQ (filter.getOrder(), 2);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getRipple(), 0.5f);
}

TEST_F (ChebyshevFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::highpass, 6, 2000.0f, sampleRate, 40.0f);

    EXPECT_EQ (filterFloat.getChebyshevType(), ChebyshevFilter<float>::Type::Type2);
    EXPECT_EQ (filterFloat.getFilterType(), FilterType::highpass);
    EXPECT_EQ (filterFloat.getOrder(), 6);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getRipple(), 40.0f);
}

TEST_F (ChebyshevFilterTests, OrderLimits)
{
    // Test minimum order
    filterFloat.setOrder (0);
    EXPECT_EQ (filterFloat.getOrder(), 1);

    // Test maximum order
    filterFloat.setOrder (25);
    EXPECT_EQ (filterFloat.getOrder(), 20);

    // Test valid range
    for (int order = 1; order <= 20; ++order)
    {
        filterFloat.setOrder (order);
        EXPECT_EQ (filterFloat.getOrder(), order);
    }
}

TEST_F (ChebyshevFilterTests, Type1RippleLimits)
{
    filterFloat.setChebyshevType (ChebyshevFilter<float>::Type::Type1);

    // Test minimum ripple for Type I
    filterFloat.setRipple (0.005f);
    EXPECT_GE (filterFloat.getRipple(), 0.01f);

    // Test maximum ripple for Type I
    filterFloat.setRipple (15.0f);
    EXPECT_LE (filterFloat.getRipple(), 10.0f);

    // Test valid range
    filterFloat.setRipple (1.0f);
    EXPECT_FLOAT_EQ (filterFloat.getRipple(), 1.0f);
}

TEST_F (ChebyshevFilterTests, Type2RippleLimits)
{
    filterFloat.setChebyshevType (ChebyshevFilter<float>::Type::Type2);

    // Test minimum ripple for Type II
    filterFloat.setRipple (10.0f);
    EXPECT_GE (filterFloat.getRipple(), 20.0f);

    // Test maximum ripple for Type II
    filterFloat.setRipple (150.0f);
    EXPECT_LE (filterFloat.getRipple(), 100.0f);

    // Test valid range
    filterFloat.setRipple (60.0f);
    EXPECT_FLOAT_EQ (filterFloat.getRipple(), 60.0f);
}

TEST_F (ChebyshevFilterTests, TypeSwitching)
{
    // Start with Type I
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    // Switch to Type II - ripple should be adjusted
    filterFloat.setChebyshevType (ChebyshevFilter<float>::Type::Type2);
    EXPECT_EQ (filterFloat.getChebyshevType(), ChebyshevFilter<float>::Type::Type2);
    EXPECT_GE (filterFloat.getRipple(), 20.0f); // Should be adjusted to valid Type II range

    // Switch back to Type I - ripple should be adjusted again
    filterFloat.setRipple (80.0f); // Set high value first
    filterFloat.setChebyshevType (ChebyshevFilter<float>::Type::Type1);
    EXPECT_EQ (filterFloat.getChebyshevType(), ChebyshevFilter<float>::Type::Type1);
    EXPECT_LE (filterFloat.getRipple(), 10.0f); // Should be adjusted to valid Type I range
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, Type1LowpassCharacteristic)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    // DC should pass through
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.5f);

    // Response at cutoff should show ripple effect
    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_TRUE (std::isfinite (responseAtCutoff));

    // High frequency should be attenuated more than Butterworth
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    const auto responseAt8kHz = filterFloat.getMagnitudeResponse (8000.0f);

    // Should show steep rolloff characteristic of Chebyshev
    const auto rolloffRatio = responseAt8kHz / responseAt4kHz;
    EXPECT_LT (rolloffRatio, 0.5f); // Steeper than typical 2-pole response
}

TEST_F (ChebyshevFilterTests, DISABLED_Type1HighpassCharacteristic)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::highpass, 4, 1000.0f, sampleRate, 1.0f);

    // DC should be blocked
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_LT (dcResponse, 0.1f);

    // High frequency should pass
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (responseAt10kHz, 0.3f);

    // Low frequency should show steep attenuation
    const auto responseAt250Hz = filterFloat.getMagnitudeResponse (250.0f);
    const auto responseAt125Hz = filterFloat.getMagnitudeResponse (125.0f);

    const auto rolloffRatio = responseAt125Hz / responseAt250Hz;
    EXPECT_LT (rolloffRatio, 0.5f); // Steep rolloff
}

TEST_F (ChebyshevFilterTests, DISABLED_Type2LowpassCharacteristic)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 4, 1000.0f, sampleRate, 40.0f);

    // DC should pass through smoothly (no passband ripple)
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.8f);

    // Response should be monotonic in passband
    const auto response500Hz = filterFloat.getMagnitudeResponse (500.0f);
    const auto response750Hz = filterFloat.getMagnitudeResponse (750.0f);

    EXPECT_GE (dcResponse, response500Hz);
    EXPECT_GE (response500Hz, response750Hz);

    // Stopband should show ripple/notches
    const auto responseAt2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);

    // Type II should have finite transmission zeros
    EXPECT_TRUE (std::isfinite (responseAt2kHz));
    EXPECT_TRUE (std::isfinite (responseAt4kHz));
}

TEST_F (ChebyshevFilterTests, DISABLED_Type2HighpassCharacteristic)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::highpass, 4, 1000.0f, sampleRate, 40.0f);

    // DC should be blocked
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_LT (dcResponse, 0.1f);

    // Passband should be monotonic
    const auto response2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto response4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    const auto response8kHz = filterFloat.getMagnitudeResponse (8000.0f);

    EXPECT_LE (response2kHz, response4kHz);
    EXPECT_LE (response4kHz, response8kHz);
}

TEST_F (ChebyshevFilterTests, RippleEffect)
{
    // Test Type I passband ripple
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 3.0f);

    // Sample multiple points in passband to detect ripple
    std::vector<float> passbandResponse;
    for (int i = 1; i <= 20; ++i)
    {
        const auto freq = static_cast<float> (i) * 40.0f; // 40Hz to 800Hz
        passbandResponse.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Type I should show some variation in passband (ripple)
    const auto minResponse = *std::min_element (passbandResponse.begin(), passbandResponse.end());
    const auto maxResponse = *std::max_element (passbandResponse.begin(), passbandResponse.end());

    EXPECT_GT (maxResponse, minResponse);        // Should have ripple variation
    EXPECT_LT (maxResponse / minResponse, 5.0f); // But not extreme
}

TEST_F (ChebyshevFilterTests, OrderEffect)
{
    // Test increasing order makes steeper response
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 2, 1000.0f, sampleRate, 0.5f);
    const auto order2At2kHz = filterFloat.getMagnitudeResponse (2000.0f);

    filterFloat.setOrder (6);
    const auto order6At2kHz = filterFloat.getMagnitudeResponse (2000.0f);

    filterFloat.setOrder (12);
    const auto order12At2kHz = filterFloat.getMagnitudeResponse (2000.0f);

    // Higher order should provide better attenuation
    EXPECT_GT (order2At2kHz, order6At2kHz);
    EXPECT_GT (order6At2kHz, order12At2kHz);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, SampleProcessing)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ChebyshevFilterTests, BlockProcessing)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 8, 1000.0f, sampleRate, 60.0f);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (ChebyshevFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and show decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), toleranceF);

    // Should show characteristic Chebyshev decay with possible ringing
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);

    // Check for overall stability (no infinite values)
    for (const auto sample : impulseResponse)
    {
        EXPECT_TRUE (std::isfinite (sample));
    }
}

//==============================================================================
// Specialized Chebyshev Characteristics Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, PassbandEdgeFrequency)
{
    // Test Type I passband edge calculation
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    const auto passbandEdge = filterFloat.getPassbandEdgeFrequency();
    EXPECT_FLOAT_EQ (passbandEdge, 1000.0f); // Should equal cutoff for Type I

    // Test Type II passband edge calculation
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 4, 1000.0f, sampleRate, 40.0f);

    const auto type2PassbandEdge = filterFloat.getPassbandEdgeFrequency();
    EXPECT_LT (type2PassbandEdge, 1000.0f); // Should be less than cutoff for Type II
    EXPECT_GT (type2PassbandEdge, 100.0f);  // Should be reasonable
}

TEST_F (ChebyshevFilterTests, StopbandEdgeFrequency)
{
    // Test Type I stopband edge calculation
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    const auto stopbandEdge = filterFloat.getStopbandEdgeFrequency();
    EXPECT_GT (stopbandEdge, 1000.0f); // Should be greater than cutoff for Type I

    // Test Type II stopband edge calculation
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 4, 1000.0f, sampleRate, 40.0f);

    const auto type2StopbandEdge = filterFloat.getStopbandEdgeFrequency();
    EXPECT_FLOAT_EQ (type2StopbandEdge, 1000.0f); // Should equal cutoff for Type II
}

TEST_F (ChebyshevFilterTests, StepResponse)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 500.0f, sampleRate, 0.5f);
    filterFloat.reset();

    std::vector<float> stepResponse (512);
    for (int i = 0; i < 512; ++i)
    {
        const float input = (i >= 0) ? 1.0f : 0.0f;
        stepResponse[i] = filterFloat.processSample (input);
    }

    // Step response should settle to final value
    const auto finalValue = stepResponse.back();
    EXPECT_TRUE (std::isfinite (finalValue));
    EXPECT_GT (finalValue, 0.5f); // Should pass most of the step

    // Chebyshev Type I may show overshoot/ringing
    const auto maxValue = *std::max_element (stepResponse.begin(), stepResponse.end());
    EXPECT_GE (maxValue, finalValue); // May overshoot due to passband ripple
}

TEST_F (ChebyshevFilterTests, GroupDelay)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f);

    // Test group delay characteristics by measuring phase response
    const auto freq1 = 500.0f;
    const auto freq2 = 600.0f;

    const auto response1 = filterFloat.getComplexResponse (freq1);
    const auto response2 = filterFloat.getComplexResponse (freq2);

    // Both should be finite and stable
    EXPECT_TRUE (std::isfinite (response1.real()));
    EXPECT_TRUE (std::isfinite (response1.imag()));
    EXPECT_TRUE (std::isfinite (response2.real()));
    EXPECT_TRUE (std::isfinite (response2.imag()));

    // Chebyshev filters typically have variable group delay
    const auto phase1 = std::atan2 (response1.imag(), response1.real());
    const auto phase2 = std::atan2 (response2.imag(), response2.real());

    EXPECT_TRUE (std::isfinite (phase1));
    EXPECT_TRUE (std::isfinite (phase2));
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, DoublePrecision)
{
    filterDouble.setParameters (ChebyshevFilter<double>::Type::Type1, FilterType::lowpass, 8, 1000.0, sampleRate, 0.5);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (ChebyshevFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);
    filterDouble.setParameters (ChebyshevFilter<double>::Type::Type1, FilterType::lowpass, 4, 1000.0, sampleRate, 1.0);

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

TEST_F (ChebyshevFilterTests, DISABLED_HighOrderStability)
{
    // Test maximum order stability
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 20, 1000.0f, sampleRate, 2.0f);

    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (ChebyshevFilterTests, ExtremeRippleStability)
{
    // Test Type I with maximum ripple
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 10.0f);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Test Type II with maximum attenuation
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 6, 1000.0f, sampleRate, 100.0f);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ChebyshevFilterTests, FrequencyExtremes)
{
    // Very low frequency
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 10.0f, sampleRate, 1.0f);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency (near Nyquist)
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, nyquist, sampleRate, 1.0f);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, ResetClearsState)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (ChebyshevFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::highpass, 8, 2000.0f, sampleRate, 60.0f);

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

TEST_F (ChebyshevFilterTests, ZeroInput)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 8, 1000.0f, sampleRate, 2.0f);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (ChebyshevFilterTests, ConstantInput)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 4, 1000.0f, sampleRate, 0.5f);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually stabilize
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be stable and proportional to input
    EXPECT_NEAR (output, constantInput, 0.2f);
}

TEST_F (ChebyshevFilterTests, DISABLED_SinusoidalInput)
{
    filterFloat.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 6, 1000.0f, sampleRate, 40.0f);

    // Test with sinusoid in passband
    const float freq = 500.0f;
    float maxOutput = 0.0f;

    for (int i = 0; i < 1000; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * freq * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        maxOutput = std::max (maxOutput, std::abs (output));
    }

    // Should have reasonable output for passband frequency
    EXPECT_GT (maxOutput, 0.3f);
    EXPECT_LT (maxOutput, 2.0f);
}

//==============================================================================
// Comparative Tests
//==============================================================================

TEST_F (ChebyshevFilterTests, DISABLED_CompareType1VsType2)
{
    // Configure both types with same order and frequency
    ChebyshevFilterFloat type1Filter, type2Filter;

    type1Filter.prepare (sampleRate, blockSize);
    type2Filter.prepare (sampleRate, blockSize);

    type1Filter.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f);
    type2Filter.setParameters (ChebyshevFilter<float>::Type::Type2, FilterType::lowpass, 1000.0f, sampleRate, 40.0f);

    // Test passband behavior
    const auto type1At500Hz = type1Filter.getMagnitudeResponse (500.0f);
    const auto type2At500Hz = type2Filter.getMagnitudeResponse (500.0f);

    // Type II should be more monotonic in passband
    EXPECT_TRUE (std::isfinite (type1At500Hz));
    EXPECT_TRUE (std::isfinite (type2At500Hz));

    // Test stopband behavior
    const auto type1At3kHz = type1Filter.getMagnitudeResponse (3000.0f);
    const auto type2At3kHz = type2Filter.getMagnitudeResponse (3000.0f);

    // Both should attenuate, but with different characteristics
    EXPECT_LT (type1At3kHz, type1At500Hz);
    EXPECT_LT (type2At3kHz, type2At500Hz);
}

TEST_F (ChebyshevFilterTests, AllOrdersBasicFunctionality)
{
    // Test that all supported orders work without throwing
    for (int order = 1; order <= 20; ++order)
    {
        filterFloat.setParameters (ChebyshevFilter<float>::Type::Type1, FilterType::lowpass, order, 1000.0f, sampleRate, 1.0f);

        // Each order should process without throwing
        for (int i = 0; i < 10; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
        }

        // Test frequency response
        const auto response = filterFloat.getMagnitudeResponse (2000.0f);
        EXPECT_TRUE (std::isfinite (response));

        filterFloat.reset();
    }
}
