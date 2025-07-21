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
class EllipticFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    EllipticFilterFloat filterFloat;
    EllipticFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (EllipticFilterTests, DefaultConstruction)
{
    EllipticFilterFloat filter;
    EXPECT_EQ (filter.getFilterType(), FilterType::lowpass);
    EXPECT_EQ (filter.getOrder(), 2);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getPassbandRipple(), 0.5f);
    EXPECT_FLOAT_EQ (filter.getStopbandAttenuation(), 40.0f);
}

TEST_F (EllipticFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (FilterType::highpass, 6, 2000.0f, sampleRate, 1.0f, 60.0f);

    EXPECT_EQ (filterFloat.getFilterType(), FilterType::highpass);
    EXPECT_EQ (filterFloat.getOrder(), 6);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getPassbandRipple(), 1.0f);
    EXPECT_FLOAT_EQ (filterFloat.getStopbandAttenuation(), 60.0f);
}

TEST_F (EllipticFilterTests, OrderLimits)
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

TEST_F (EllipticFilterTests, PassbandRippleLimits)
{
    // Test minimum ripple
    filterFloat.setPassbandRipple (0.005f);
    EXPECT_GE (filterFloat.getPassbandRipple(), 0.01f);

    // Test maximum ripple
    filterFloat.setPassbandRipple (15.0f);
    EXPECT_LE (filterFloat.getPassbandRipple(), 10.0f);

    // Test valid range
    filterFloat.setPassbandRipple (2.0f);
    EXPECT_FLOAT_EQ (filterFloat.getPassbandRipple(), 2.0f);
}

TEST_F (EllipticFilterTests, StopbandAttenuationLimits)
{
    // Test minimum attenuation
    filterFloat.setStopbandAttenuation (10.0f);
    EXPECT_GE (filterFloat.getStopbandAttenuation(), 20.0f);

    // Test maximum attenuation
    filterFloat.setStopbandAttenuation (150.0f);
    EXPECT_LE (filterFloat.getStopbandAttenuation(), 120.0f);

    // Test valid range
    filterFloat.setStopbandAttenuation (80.0f);
    EXPECT_FLOAT_EQ (filterFloat.getStopbandAttenuation(), 80.0f);
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (EllipticFilterTests, DISABLED_LowpassCharacteristic)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f, 60.0f);

    // DC should pass through with some ripple
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.5f);

    // Response should show passband ripple
    const auto response500Hz = filterFloat.getMagnitudeResponse (500.0f);
    const auto response750Hz = filterFloat.getMagnitudeResponse (750.0f);

    EXPECT_TRUE (std::isfinite (response500Hz));
    EXPECT_TRUE (std::isfinite (response750Hz));

    // High frequency should be heavily attenuated (steeper than other filter types)
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    const auto responseAt8kHz = filterFloat.getMagnitudeResponse (8000.0f);

    // Should show very steep rolloff characteristic of elliptic filters
    const auto rolloffRatio = responseAt8kHz / responseAt4kHz;
    EXPECT_LT (rolloffRatio, 0.3f); // Much steeper than Butterworth/Bessel

    // Stopband should meet attenuation requirements
    EXPECT_LT (responseAt4kHz, 0.1f); // Strong attenuation in stopband
}

TEST_F (EllipticFilterTests, DISABLED_HighpassCharacteristic)
{
    filterFloat.setParameters (FilterType::highpass, 4, 1000.0f, sampleRate, 1.0f, 60.0f);

    // DC should be strongly blocked
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_LT (dcResponse, 0.01f);

    // High frequency should pass with some ripple
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (responseAt10kHz, 0.3f);

    // Low frequency should show steep attenuation
    const auto responseAt250Hz = filterFloat.getMagnitudeResponse (250.0f);
    const auto responseAt125Hz = filterFloat.getMagnitudeResponse (125.0f);

    const auto rolloffRatio = responseAt125Hz / responseAt250Hz;
    EXPECT_LT (rolloffRatio, 0.3f); // Very steep rolloff
}

TEST_F (EllipticFilterTests, DISABLED_PassbandRipple)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 3.0f, 60.0f);

    // Sample multiple points in passband to detect ripple
    std::vector<float> passbandResponse;
    for (int i = 1; i <= 20; ++i)
    {
        const auto freq = static_cast<float> (i) * 40.0f; // 40Hz to 800Hz
        passbandResponse.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Elliptic filters should show equiripple in passband
    const auto minResponse = *std::min_element (passbandResponse.begin(), passbandResponse.end());
    const auto maxResponse = *std::max_element (passbandResponse.begin(), passbandResponse.end());

    EXPECT_GT (maxResponse, minResponse); // Should have ripple variation

    // Ripple should be approximately within specified dB range
    const auto rippleDB = 20.0f * std::log10 (maxResponse / minResponse);
    EXPECT_LT (rippleDB, 6.0f); // Should be reasonable compared to specified 3dB
}

TEST_F (EllipticFilterTests, DISABLED_StopbandRipple)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    // Sample multiple points in stopband to detect ripple/notches
    std::vector<float> stopbandResponse;
    for (int i = 20; i <= 100; ++i)
    {
        const auto freq = static_cast<float> (i) * 100.0f; // 2kHz to 10kHz
        stopbandResponse.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Elliptic filters should show equiripple in stopband with notches
    const auto minResponse = *std::min_element (stopbandResponse.begin(), stopbandResponse.end());
    const auto maxResponse = *std::max_element (stopbandResponse.begin(), stopbandResponse.end());

    EXPECT_GT (maxResponse, minResponse); // Should have ripple/notch variation

    // Should have finite transmission zeros (notches)
    int notchCount = 0;
    for (size_t i = 1; i < stopbandResponse.size() - 1; ++i)
    {
        if (stopbandResponse[i] < stopbandResponse[i - 1] && stopbandResponse[i] < stopbandResponse[i + 1])
        {
            if (stopbandResponse[i] < maxResponse * 0.1f) // Significant notch
                notchCount++;
        }
    }

    EXPECT_GT (notchCount, 0); // Should have some notches from transmission zeros
}

TEST_F (EllipticFilterTests, DISABLED_OrderEffect)
{
    // Test that increasing order provides steeper rolloff
    filterFloat.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate, 1.0f, 60.0f);
    const auto order2At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    filterFloat.setOrder (6);
    const auto order6At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    filterFloat.setOrder (12);
    const auto order12At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    // Higher order should provide much better attenuation (steepest possible)
    EXPECT_GT (order2At3kHz, order6At3kHz);
    EXPECT_GT (order6At3kHz, order12At3kHz);

    // Elliptic should provide the steepest rolloff
    EXPECT_LT (order12At3kHz, 0.001f); // Very strong attenuation with high order
}

//==============================================================================
// Elliptic-Specific Characteristics Tests
//==============================================================================

TEST_F (EllipticFilterTests, SelectivityFactor)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    const auto selectivity = filterFloat.getSelectivityFactor();
    EXPECT_GT (selectivity, 0.0f);
    EXPECT_LT (selectivity, 1.0f);
    EXPECT_TRUE (std::isfinite (selectivity));

    // Higher stopband attenuation should decrease selectivity factor
    filterFloat.setStopbandAttenuation (80.0f);
    const auto higherAttenSelectivity = filterFloat.getSelectivityFactor();
    EXPECT_LT (higherAttenSelectivity, selectivity);

    // Higher passband ripple should increase selectivity factor
    filterFloat.setPassbandRipple (3.0f);
    const auto higherRippleSelectivity = filterFloat.getSelectivityFactor();
    EXPECT_GT (higherRippleSelectivity, higherAttenSelectivity);
}

TEST_F (EllipticFilterTests, TransitionBandwidth)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    const auto transitionBW = filterFloat.getTransitionBandwidth();
    EXPECT_GT (transitionBW, 0.0f);
    EXPECT_LT (transitionBW, 1.0f);
    EXPECT_TRUE (std::isfinite (transitionBW));

    // Higher order should provide narrower transition bandwidth
    filterFloat.setOrder (12);
    const auto higherOrderTransitionBW = filterFloat.getTransitionBandwidth();
    EXPECT_LT (higherOrderTransitionBW, transitionBW);

    // Elliptic filters should have the narrowest transition bandwidth
    EXPECT_LT (transitionBW, 0.5f); // Should be quite narrow
}

TEST_F (EllipticFilterTests, DISABLED_SteepestRolloff)
{
    // Compare elliptic rolloff with theoretical expectations
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 1.0f, 80.0f);

    // Test rolloff steepness by measuring attenuation over small frequency range
    const auto response1500Hz = filterFloat.getMagnitudeResponse (1500.0f);
    const auto response2000Hz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto response3000Hz = filterFloat.getMagnitudeResponse (3000.0f);

    // Should show very steep transition
    const auto rolloff1 = response2000Hz / response1500Hz;
    const auto rolloff2 = response3000Hz / response2000Hz;

    EXPECT_LT (rolloff1, 0.5f); // Steep transition
    EXPECT_LT (rolloff2, 0.3f); // Even steeper

    // Should achieve specified stopband attenuation
    EXPECT_LT (response3000Hz, 0.01f); // -40dB or better for 80dB specification
}

TEST_F (EllipticFilterTests, AllpassConfiguration)
{
    filterFloat.setParameters (FilterType::allpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    // Allpass should have unit magnitude response
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    const auto response1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto response5kHz = filterFloat.getMagnitudeResponse (5000.0f);

    // All frequencies should pass with approximately unit gain
    EXPECT_NEAR (dcResponse, 1.0f, 0.2f);
    EXPECT_NEAR (response1kHz, 1.0f, 0.2f);
    EXPECT_NEAR (response5kHz, 1.0f, 0.2f);

    // But phase should vary (this is the purpose of elliptic allpass)
    const auto dcPhase = std::arg (filterFloat.getComplexResponse (1.0f));
    const auto phase1kHz = std::arg (filterFloat.getComplexResponse (1000.0f));
    const auto phase5kHz = std::arg (filterFloat.getComplexResponse (5000.0f));

    EXPECT_TRUE (std::isfinite (dcPhase));
    EXPECT_TRUE (std::isfinite (phase1kHz));
    EXPECT_TRUE (std::isfinite (phase5kHz));

    // Phase should change significantly across frequency
    const auto phaseRange = std::abs (phase5kHz - dcPhase);
    EXPECT_GT (phaseRange, 1.0); // Should have significant phase variation
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (EllipticFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f, 60.0f);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (EllipticFilterTests, DISABLED_BlockProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 2.0f, 80.0f);

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

TEST_F (EllipticFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);
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

    // Elliptic filters may show ringing due to passband/stopband ripple
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);

    // Check for overall stability (no infinite values)
    for (const auto sample : impulseResponse)
    {
        EXPECT_TRUE (std::isfinite (sample));
    }
}

TEST_F (EllipticFilterTests, DISABLED_StepResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 500.0f, sampleRate, 1.0f, 60.0f);
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
    EXPECT_GT (finalValue, 0.5f);

    // Elliptic filters may show overshoot and ringing due to ripple
    const auto maxValue = *std::max_element (stepResponse.begin(), stepResponse.end());
    EXPECT_GE (maxValue, finalValue); // May overshoot

    // But should remain stable
    EXPECT_LT (maxValue, finalValue * 3.0f); // Should not be excessive
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (EllipticFilterTests, DISABLED_DoublePrecision)
{
    filterDouble.setParameters (FilterType::lowpass, 8, 1000.0, sampleRate, 1.0, 80.0);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (EllipticFilterTests, DISABLED_FloatVsDoublePrecision)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);
    filterDouble.setParameters (FilterType::lowpass, 6, 1000.0, sampleRate, 1.0, 60.0);

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

TEST_F (EllipticFilterTests, DISABLED_HighOrderStability)
{
    // Test maximum order stability
    filterFloat.setParameters (FilterType::lowpass, 20, 1000.0f, sampleRate, 2.0f, 100.0f);

    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (EllipticFilterTests, DISABLED_ExtremeParameterStability)
{
    // Test with maximum ripple and attenuation
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 10.0f, 120.0f);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Test with minimum ripple and attenuation
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 0.01f, 20.0f);

    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (EllipticFilterTests, DISABLED_FrequencyExtremes)
{
    // Very low frequency
    filterFloat.setParameters (FilterType::lowpass, 4, 10.0f, sampleRate, 1.0f, 60.0f);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency (near Nyquist)
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (FilterType::lowpass, 4, nyquist, sampleRate, 1.0f, 60.0f);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (EllipticFilterTests, ResetClearsState)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (EllipticFilterTests, DISABLED_ParameterChangesHandledSafely)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f, 60.0f);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (FilterType::highpass, 8, 2000.0f, sampleRate, 2.0f, 80.0f);

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

TEST_F (EllipticFilterTests, ZeroInput)
{
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 2.0f, 80.0f);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (EllipticFilterTests, DISABLED_ConstantInput)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate, 1.0f, 60.0f);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually stabilize
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be stable and proportional to input (may have some error due to ripple)
    EXPECT_NEAR (output, constantInput, 0.3f);
}

TEST_F (EllipticFilterTests, DISABLED_SinusoidalInput)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

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
// Comparative Performance Tests
//==============================================================================

TEST_F (EllipticFilterTests, DISABLED_CompareWithOtherFilterTypes)
{
    // Test that elliptic provides steepest rolloff for same order
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate, 1.0f, 60.0f);

    // Test stopband attenuation at 3kHz
    const auto ellipticAt3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    // Elliptic should provide better stopband attenuation than other filter types
    // (This is qualitative since we don't have other filters in this test)
    EXPECT_LT (ellipticAt3kHz, 0.01f); // Should be very well attenuated

    // Test transition sharpness
    const auto responseAt1200Hz = filterFloat.getMagnitudeResponse (1200.0f);
    const auto responseAt1800Hz = filterFloat.getMagnitudeResponse (1800.0f);

    const auto transitionRatio = responseAt1800Hz / responseAt1200Hz;
    EXPECT_LT (transitionRatio, 0.2f); // Very sharp transition
}

TEST_F (EllipticFilterTests, DISABLED_AllOrdersBasicFunctionality)
{
    // Test that all supported orders work without throwing
    for (int order = 1; order <= 20; ++order)
    {
        filterFloat.setParameters (FilterType::lowpass, order, 1000.0f, sampleRate, 1.0f, 60.0f);

        // Each order should process without throwing
        for (int i = 0; i < 10; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
        }

        // Test frequency response
        const auto response = filterFloat.getMagnitudeResponse (2000.0f);
        EXPECT_TRUE (std::isfinite (response));

        // Test selectivity factor calculation
        const auto selectivity = filterFloat.getSelectivityFactor();
        EXPECT_TRUE (std::isfinite (selectivity));
        EXPECT_GT (selectivity, 0.0f);

        // Test transition bandwidth calculation
        const auto transitionBW = filterFloat.getTransitionBandwidth();
        EXPECT_TRUE (std::isfinite (transitionBW));
        EXPECT_GT (transitionBW, 0.0f);

        filterFloat.reset();
    }
}

TEST_F (EllipticFilterTests, DISABLED_OptimalFrequencySelectivity)
{
    // Test that elliptic filter provides optimal frequency selectivity
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate, 1.0f, 80.0f);

    // Measure selectivity by testing multiple frequency points
    std::vector<float> frequencies = { 800.0f, 900.0f, 1000.0f, 1100.0f, 1200.0f, 1400.0f, 1600.0f, 2000.0f };
    std::vector<float> responses;

    for (const auto freq : frequencies)
    {
        responses.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Should show very sharp transition around cutoff
    const auto passbandLevel = responses[0];     // 800Hz
    const auto stopbandLevel = responses.back(); // 2000Hz

    const auto selectivityRatio = stopbandLevel / passbandLevel;
    EXPECT_LT (selectivityRatio, 0.001f); // Very sharp selectivity for elliptic

    // Should have monotonic decrease in transition region
    for (size_t i = 4; i < responses.size(); ++i) // From 1200Hz onwards
    {
        EXPECT_LE (responses[i], responses[i - 1] + 0.1f); // Generally decreasing
    }
}
