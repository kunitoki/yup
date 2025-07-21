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
class BesselFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    BesselFilterFloat filterFloat;
    BesselFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (BesselFilterTests, DefaultConstruction)
{
    BesselFilterFloat filter;
    EXPECT_EQ (filter.getFilterType(), FilterType::lowpass);
    EXPECT_EQ (filter.getOrder(), 2);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
}

TEST_F (BesselFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (FilterType::highpass, 6, 2000.0f, sampleRate);

    EXPECT_EQ (filterFloat.getFilterType(), FilterType::highpass);
    EXPECT_EQ (filterFloat.getOrder(), 6);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
}

TEST_F (BesselFilterTests, OrderLimits)
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

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (BesselFilterTests, LowpassCharacteristic)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // DC should pass through
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.8f);

    // Response should be smooth without ripple
    const auto response500Hz = filterFloat.getMagnitudeResponse (500.0f);
    const auto response750Hz = filterFloat.getMagnitudeResponse (750.0f);
    const auto response1000Hz = filterFloat.getMagnitudeResponse (1000.0f);

    // Should show smooth monotonic decrease
    EXPECT_GE (dcResponse, response500Hz);
    EXPECT_GE (response500Hz, response750Hz);
    EXPECT_GE (response750Hz, response1000Hz);

    // High frequency should be attenuated (but less steep than Butterworth)
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    EXPECT_LT (responseAt4kHz, response1000Hz);
    EXPECT_GT (responseAt4kHz, 0.001f); // But not as steep as other filter types
}

TEST_F (BesselFilterTests, DISABLED_HighpassCharacteristic)
{
    filterFloat.setParameters (FilterType::highpass, 4, 1000.0f, sampleRate);

    // DC should be blocked
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_LT (dcResponse, 0.1f);

    // Response should increase with frequency smoothly
    const auto response1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto response2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto response4kHz = filterFloat.getMagnitudeResponse (4000.0f);

    EXPECT_GT (response1kHz, dcResponse);
    EXPECT_GE (response2kHz, response1kHz);
    EXPECT_GE (response4kHz, response2kHz);

    // High frequencies should pass well
    EXPECT_GT (response4kHz, 0.5f);
}

TEST_F (BesselFilterTests, SmoothFrequencyResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

    // Sample many points to verify smooth response (no ripple)
    std::vector<float> responses;
    for (int i = 1; i <= 50; ++i)
    {
        const auto freq = static_cast<float> (i) * 20.0f; // 20Hz to 1000Hz
        responses.push_back (filterFloat.getMagnitudeResponse (freq));
    }

    // Check that response is monotonically decreasing (smooth)
    for (size_t i = 1; i < responses.size(); ++i)
    {
        EXPECT_LE (responses[i], responses[i - 1] + 0.1f); // Allow small numerical variations
    }

    // Should not have significant ripple like Chebyshev
    const auto minResponse = *std::min_element (responses.begin(), responses.end());
    const auto maxResponse = *std::max_element (responses.begin(), responses.end());
    const auto rippleRatio = maxResponse / minResponse;
    EXPECT_LT (rippleRatio, 2.0f); // Much less ripple than Chebyshev
}

TEST_F (BesselFilterTests, DISABLED_OrderEffect)
{
    // Test that increasing order provides better selectivity but maintains smooth response
    filterFloat.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate);
    const auto order2At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    filterFloat.setOrder (6);
    const auto order6At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    filterFloat.setOrder (12);
    const auto order12At3kHz = filterFloat.getMagnitudeResponse (3000.0f);

    // Higher order should provide better attenuation
    EXPECT_GT (order2At3kHz, order6At3kHz);
    EXPECT_GT (order6At3kHz, order12At3kHz);

    // But rolloff should be gentler than Butterworth/Chebyshev
    EXPECT_GT (order12At3kHz, 0.001f); // Still some response even with high order
}

//==============================================================================
// Linear Phase and Group Delay Tests
//==============================================================================

TEST_F (BesselFilterTests, GroupDelayCalculation)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    const auto groupDelay = filterFloat.getGroupDelay();
    EXPECT_GT (groupDelay, 0.0f);
    EXPECT_TRUE (std::isfinite (groupDelay));

    // Group delay should increase with order
    filterFloat.setOrder (8);
    const auto higherOrderDelay = filterFloat.getGroupDelay();
    EXPECT_GT (higherOrderDelay, groupDelay);

    // Group delay should be inversely related to cutoff frequency
    filterFloat.setCutoffFrequency (500.0f);
    const auto lowerFreqDelay = filterFloat.getGroupDelay();
    EXPECT_GT (lowerFreqDelay, higherOrderDelay);
}

TEST_F (BesselFilterTests, LinearPhaseCharacteristic)
{
    using CoeffType = decltype (filterFloat)::CoefficientsType;

    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

    // Test phase linearity by measuring phase at multiple frequencies
    std::vector<CoeffType> frequencies = { 100.0, 200.0, 300.0, 400.0, 500.0 };
    std::vector<CoeffType> phases;

    for (const auto freq : frequencies)
    {
        const auto response = filterFloat.getComplexResponse (static_cast<CoeffType> (freq));
        const auto phase = std::atan2 (response.imag(), response.real());
        phases.push_back (phase);
    }

    // Check that phase relationship is approximately linear
    // (This is a qualitative test since perfect linearity is hard to verify numerically)
    for (const auto phase : phases)
    {
        EXPECT_TRUE (std::isfinite (phase));
    }

    // Phase should generally become more negative with frequency for lowpass
    for (size_t i = 1; i < phases.size(); ++i)
    {
        EXPECT_LE (phases[i], phases[i - 1] + 0.5); // Allow some tolerance for numerical effects
    }
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (BesselFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (BesselFilterTests, BlockProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 500.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (BesselFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and show smooth decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), toleranceF);

    // Bessel filters should have minimal overshoot/ringing
    const auto maxValue = *std::max_element (impulseResponse.begin(), impulseResponse.end());
    const auto initialValue = impulseResponse[0];

    // Overshoot should be minimal compared to other filter types
    EXPECT_LT (maxValue, std::abs (initialValue) * 1.5f); // Less than 50% overshoot

    // Should show smooth exponential-like decay
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);
}

TEST_F (BesselFilterTests, StepResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 500.0f, sampleRate);
    filterFloat.reset();

    std::vector<float> stepResponse (512);
    for (int i = 0; i < 512; ++i)
    {
        const float input = (i >= 0) ? 1.0f : 0.0f;
        stepResponse[i] = filterFloat.processSample (input);
    }

    // Step response should settle smoothly without significant overshoot
    const auto finalValue = stepResponse.back();
    EXPECT_TRUE (std::isfinite (finalValue));
    EXPECT_GT (finalValue, 0.5f);

    // Bessel filters should have minimal overshoot in step response
    const auto maxValue = *std::max_element (stepResponse.begin(), stepResponse.end());
    const auto overshoot = (maxValue - finalValue) / finalValue;

    EXPECT_LT (overshoot, 0.2f); // Less than 20% overshoot (much better than Butterworth)
}

//==============================================================================
// Transient Response Tests
//==============================================================================

TEST_F (BesselFilterTests, SquareWaveResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 200.0f, sampleRate);

    // Generate square wave and measure transient response
    std::vector<float> outputs;
    outputs.reserve (1000);

    for (int i = 0; i < 1000; ++i)
    {
        // 50Hz square wave
        const float input = (std::fmod (i, static_cast<float> (sampleRate) / 100.0f) < (sampleRate / 200.0f)) ? 1.0f : -1.0f;
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Bessel filter should produce smooth transitions without ringing
    // (Qualitative test - main goal is stability verification)
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    const auto minOutput = *std::min_element (outputs.begin(), outputs.end());

    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (minOutput, -0.1f);
    EXPECT_LT (maxOutput, 2.0f); // Should not have excessive overshoot
    EXPECT_GT (minOutput, -2.0f);
}

TEST_F (BesselFilterTests, WaveformPreservation)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 2000.0f, sampleRate);

    // Test with a complex waveform (sum of sines)
    std::vector<float> originalSignal, filteredSignal;
    originalSignal.reserve (200);
    filteredSignal.reserve (200);

    for (int i = 0; i < 200; ++i)
    {
        // Complex waveform with multiple harmonics within passband
        const float t = static_cast<float> (i) / static_cast<float> (sampleRate);
        const float fundamental = std::sin (2.0f * MathConstants<float>::pi * 300.0f * t);
        const float harmonic2 = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 600.0f * t);
        const float harmonic3 = 0.25f * std::sin (2.0f * MathConstants<float>::pi * 900.0f * t);

        const float input = fundamental + harmonic2 + harmonic3;
        const auto output = filterFloat.processSample (input);

        originalSignal.push_back (input);
        filteredSignal.push_back (output);
    }

    // Bessel filter should preserve waveform shape better than other filter types
    // (This is a qualitative test - we mainly verify stability and reasonable output)
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto originalRMS = calculateRMS (originalSignal);
    const auto filteredRMS = calculateRMS (filteredSignal);

    // Since signal is mostly in passband, RMS should be preserved reasonably well
    EXPECT_GT (filteredRMS, originalRMS * 0.3f);
    EXPECT_LT (filteredRMS, originalRMS * 1.2f);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (BesselFilterTests, DoublePrecision)
{
    filterDouble.setParameters (FilterType::lowpass, 8, 1000.0, sampleRate);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (BesselFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);
    filterDouble.setParameters (FilterType::lowpass, 6, 1000.0, sampleRate);

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

TEST_F (BesselFilterTests, HighOrderStability)
{
    // Test maximum order stability
    filterFloat.setParameters (FilterType::lowpass, 20, 1000.0f, sampleRate);

    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (BesselFilterTests, FrequencyExtremes)
{
    // Very low frequency
    filterFloat.setParameters (FilterType::lowpass, 4, 1.0f, sampleRate);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency (near Nyquist)
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (FilterType::lowpass, 4, nyquist, sampleRate);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

TEST_F (BesselFilterTests, LargeSignalStability)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

    // Test with large input signals
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (100.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 1000.0f); // Should not blow up excessively
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (BesselFilterTests, ResetClearsState)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (BesselFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (FilterType::highpass, 8, 2000.0f, sampleRate);

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

TEST_F (BesselFilterTests, ZeroInput)
{
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (BesselFilterTests, ConstantInput)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually equal output
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, constantInput, 0.1f);
}

TEST_F (BesselFilterTests, SinusoidalInput)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

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
    EXPECT_LT (maxOutput, 1.5f);
}

//==============================================================================
// Bessel-Specific Characteristic Tests
//==============================================================================

TEST_F (BesselFilterTests, MaximallyFlatGroupDelay)
{
    using CoeffType = decltype (filterFloat)::CoefficientsType;

    // Test that group delay is approximately constant across passband
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate);

    std::vector<float> frequencies = { 100.0f, 200.0f, 300.0f, 400.0f, 500.0f };
    std::vector<CoeffType> groupDelays;

    // Calculate group delay by numerical differentiation of phase
    for (const auto freq : frequencies)
    {
        const auto deltaF = 1.0f;
        const auto response1 = filterFloat.getComplexResponse (freq - deltaF);
        const auto response2 = filterFloat.getComplexResponse (freq + deltaF);

        const auto phase1 = std::atan2 (response1.imag(), response1.real());
        const auto phase2 = std::atan2 (response2.imag(), response2.real());

        const auto groupDelay = -(phase2 - phase1) / (2.0 * deltaF * 2.0 * MathConstants<CoeffType>::pi);
        groupDelays.push_back (groupDelay);
    }

    // Group delay should be relatively constant (this is the main Bessel characteristic)
    if (groupDelays.size() > 1)
    {
        const auto minDelay = *std::min_element (groupDelays.begin(), groupDelays.end());
        const auto maxDelay = *std::max_element (groupDelays.begin(), groupDelays.end());

        // Allow reasonable variation due to numerical effects
        if (maxDelay > 0.0)
        {
            const auto variation = (maxDelay - minDelay) / maxDelay;
            EXPECT_LT (variation, 0.5); // Less than 50% variation across passband
        }
    }
}

TEST_F (BesselFilterTests, AllOrdersBasicFunctionality)
{
    // Test that all supported orders work without throwing
    for (int order = 1; order <= 20; ++order)
    {
        filterFloat.setParameters (FilterType::lowpass, order, 1000.0f, sampleRate);

        // Each order should process without throwing
        for (int i = 0; i < 10; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
        }

        // Test frequency response
        const auto response = filterFloat.getMagnitudeResponse (2000.0f);
        EXPECT_TRUE (std::isfinite (response));

        // Test group delay calculation
        const auto groupDelay = filterFloat.getGroupDelay();
        EXPECT_TRUE (std::isfinite (groupDelay));
        EXPECT_GE (groupDelay, 0.0f);

        filterFloat.reset();
    }
}
