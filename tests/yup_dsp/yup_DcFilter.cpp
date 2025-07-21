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
class DcFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    DcFilterFloat filterFloat;
    DcFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (DcFilterTests, DefaultConstruction)
{
    DcFilterFloat filter;
    EXPECT_EQ (filter.getMode(), DcFilter<float>::Mode::Default);
    EXPECT_GT (filter.getCoefficient(), 0.9f);
    EXPECT_LT (filter.getCoefficient(), 1.0f);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 20.0f);
}

TEST_F (DcFilterTests, ModeInitialization)
{
    DcFilterFloat slowFilter (DcFilter<float>::Mode::Slow);
    DcFilterFloat defaultFilter (DcFilter<float>::Mode::Default);
    DcFilterFloat fastFilter (DcFilter<float>::Mode::Fast);

    slowFilter.prepare (sampleRate, blockSize);
    defaultFilter.prepare (sampleRate, blockSize);
    fastFilter.prepare (sampleRate, blockSize);

    EXPECT_EQ (slowFilter.getMode(), DcFilter<float>::Mode::Slow);
    EXPECT_EQ (defaultFilter.getMode(), DcFilter<float>::Mode::Default);
    EXPECT_EQ (fastFilter.getMode(), DcFilter<float>::Mode::Fast);

    // Different modes should have different cutoff frequencies
    EXPECT_DOUBLE_EQ (slowFilter.getCutoffFrequency(), 5.0);
    EXPECT_DOUBLE_EQ (defaultFilter.getCutoffFrequency(), 20.0);
    EXPECT_DOUBLE_EQ (fastFilter.getCutoffFrequency(), 50.0);
}

TEST_F (DcFilterTests, CustomCutoffFrequency)
{
    filterFloat.setCutoffFrequency (10.0);
    EXPECT_DOUBLE_EQ (filterFloat.getCutoffFrequency(), 10.0);

    // Test frequency limits
    filterFloat.setCutoffFrequency (0.05);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 0.1);

    const auto nyquist = static_cast<double> (sampleRate) * 0.5;
    filterFloat.setCutoffFrequency (nyquist);
    EXPECT_LT (filterFloat.getCutoffFrequency(), nyquist);

    // Test returning to default
    filterFloat.useDefaultCutoff();
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 20.0f);
}

TEST_F (DcFilterTests, ModeChanging)
{
    filterFloat.setMode (DcFilter<float>::Mode::Slow);
    EXPECT_EQ (filterFloat.getMode(), DcFilter<float>::Mode::Slow);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 5.0f);

    filterFloat.setMode (DcFilter<float>::Mode::Fast);
    EXPECT_EQ (filterFloat.getMode(), DcFilter<float>::Mode::Fast);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 50.0f);
}

//==============================================================================
// DC Removal Tests
//==============================================================================

TEST_F (DcFilterTests, RemovesDcOffset)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Test with constant DC offset
    const float dcOffset = 0.5f;
    float output = 0.0f;

    // Process DC signal - should gradually remove the offset
    for (int i = 0; i < 1000; ++i)
    {
        output = filterFloat.processSample (dcOffset);
    }

    // After sufficient time, DC should be mostly removed
    EXPECT_LT (std::abs (output), 0.05f);
}

TEST_F (DcFilterTests, PreservesAcSignal)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Test with AC signal at 100Hz (well above cutoff)
    const float frequency = 100.0f;
    const float amplitude = 0.8f;

    std::vector<float> input, output;
    input.reserve (1000);
    output.reserve (1000);

    // Generate and process AC signal
    for (int i = 0; i < 1000; ++i)
    {
        const float sample = amplitude * std::sin (2.0f * MathConstants<float>::pi * frequency * i / static_cast<float> (sampleRate));
        input.push_back (sample);
        output.push_back (filterFloat.processSample (sample));
    }

    // Calculate RMS of input and output (after settling)
    auto calculateRMS = [] (const std::vector<float>& signal, int startIdx = 100)
    {
        float sum = 0.0f;
        const int count = static_cast<int> (signal.size()) - startIdx;
        for (int i = startIdx; i < static_cast<int> (signal.size()); ++i)
        {
            sum += signal[i] * signal[i];
        }
        return std::sqrt (sum / count);
    };

    const auto inputRMS = calculateRMS (input);
    const auto outputRMS = calculateRMS (output);

    // RMS should be preserved for frequencies well above cutoff
    EXPECT_NEAR (outputRMS, inputRMS, 0.1f * inputRMS);
}

TEST_F (DcFilterTests, RemovesDcFromAcPlusDc)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Test with AC signal + DC offset
    const float frequency = 200.0f;
    const float acAmplitude = 0.5f;
    const float dcOffset = 0.3f;

    std::vector<float> outputs;
    outputs.reserve (2000);

    // Process AC + DC signal
    for (int i = 0; i < 2000; ++i)
    {
        const float acComponent = acAmplitude * std::sin (2.0f * MathConstants<float>::pi * frequency * i / static_cast<float> (sampleRate));
        const float sample = acComponent + dcOffset;
        outputs.push_back (filterFloat.processSample (sample));
    }

    // Calculate average of latter half (after settling)
    float average = 0.0f;
    const int startIdx = 1000;
    for (int i = startIdx; i < static_cast<int> (outputs.size()); ++i)
    {
        average += outputs[i];
    }
    average /= (outputs.size() - startIdx);

    // Average should be close to zero (DC removed)
    EXPECT_LT (std::abs (average), 0.05f);

    // Peak-to-peak should be approximately preserved
    const auto minVal = *std::min_element (outputs.begin() + startIdx, outputs.end());
    const auto maxVal = *std::max_element (outputs.begin() + startIdx, outputs.end());
    const auto peakToPeak = maxVal - minVal;
    const auto expectedPeakToPeak = 2.0f * acAmplitude;

    EXPECT_NEAR (peakToPeak, expectedPeakToPeak, 0.2f * expectedPeakToPeak);
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (DcFilterTests, HighpassCharacteristic)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // DC should be blocked
    const auto dcResponse = filterFloat.getMagnitudeResponse (0.1f);
    EXPECT_LT (dcResponse, 0.1f);

    // Very low frequency should be attenuated
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (5.0f);
    EXPECT_LT (lowFreqResponse, 0.5f);

    // Frequency at cutoff should be somewhat attenuated
    const auto cutoffResponse = filterFloat.getMagnitudeResponse (20.0f);
    EXPECT_GT (cutoffResponse, 0.3f);
    EXPECT_LT (cutoffResponse, 0.9f);

    // High frequency should pass through
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (1000.0f);
    EXPECT_GT (highFreqResponse, 0.9f);
}

TEST_F (DcFilterTests, ModeFrequencyResponse)
{
    // Test that different modes have different response characteristics
    DcFilterFloat slowFilter (DcFilter<float>::Mode::Slow);
    DcFilterFloat fastFilter (DcFilter<float>::Mode::Fast);

    slowFilter.prepare (sampleRate, blockSize);
    fastFilter.prepare (sampleRate, blockSize);

    const float testFreq = 10.0f;
    const auto slowResponse = slowFilter.getMagnitudeResponse (testFreq);
    const auto fastResponse = fastFilter.getMagnitudeResponse (testFreq);

    // Fast mode should attenuate low frequencies more than slow mode
    EXPECT_LT (fastResponse, slowResponse);

    // Both should be finite and positive
    EXPECT_TRUE (std::isfinite (slowResponse));
    EXPECT_TRUE (std::isfinite (fastResponse));
    EXPECT_GT (slowResponse, 0.0f);
    EXPECT_GT (fastResponse, 0.0f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (DcFilterTests, SampleProcessing)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (DcFilterTests, BlockProcessing)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal with DC offset
    for (int i = 0; i < numSamples; ++i)
    {
        input[i] = 0.3f + 0.5f * std::sin (2.0f * MathConstants<float>::pi * 440.0f * i / static_cast<float> (sampleRate));
    }

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (DcFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should start positive and decay
    EXPECT_GT (impulseResponse[0], 0.0f);
    EXPECT_GT (impulseResponse[1], 0.0f);

    // Should show exponential decay characteristic of single-pole filter
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);

    // Should eventually settle near zero
    EXPECT_LT (std::abs (impulseResponse.back()), 0.01f);
}

TEST_F (DcFilterTests, DISABLED_StepResponse)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);
    filterFloat.reset();

    std::vector<float> stepResponse (1000);
    for (int i = 0; i < 1000; ++i)
    {
        const float input = 1.0f; // Unit step
        stepResponse[i] = filterFloat.processSample (input);
    }

    // Step response should start high and decay to zero (DC blocking)
    EXPECT_GT (stepResponse[0], 0.5f);

    // Should decay exponentially
    const auto early = stepResponse[10];
    const auto middle = stepResponse[100];
    const auto late = stepResponse[500];

    EXPECT_GT (early, middle);
    EXPECT_GT (middle, late);

    // Should settle near zero (DC component removed)
    EXPECT_LT (std::abs (stepResponse.back()), 0.05f);
}

//==============================================================================
// Denormal Protection Tests
//==============================================================================

TEST_F (DcFilterTests, DenormalProtection)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Process very small signals that could cause denormals
    for (int i = 0; i < 1000; ++i)
    {
        const float input = 1e-30f * std::sin (2.0f * MathConstants<float>::pi * 100.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);

        EXPECT_TRUE (std::isfinite (output));
        EXPECT_FALSE (std::isnan (output));
    }

    // Process silence - should handle denormals gracefully
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_FALSE (std::isnan (output));
    }
}

//==============================================================================
// Coefficient Tests
//==============================================================================

TEST_F (DcFilterTests, CoefficientLimits)
{
    // Test that coefficient stays in valid range for various cutoff frequencies
    std::vector<float> testFrequencies = { 0.1f, 1.0f, 5.0f, 20.0f, 100.0f, 1000.0f };

    for (const auto freq : testFrequencies)
    {
        filterFloat.setCutoffFrequency (freq);
        const auto coeff = filterFloat.getCoefficient();

        EXPECT_GE (coeff, 0.5f);
        EXPECT_LT (coeff, 1.0f);
        EXPECT_TRUE (std::isfinite (coeff));
    }
}

TEST_F (DcFilterTests, CoefficientModeConsistency)
{
    DcFilterFloat slowFilter (DcFilter<float>::Mode::Slow);
    DcFilterFloat defaultFilter (DcFilter<float>::Mode::Default);
    DcFilterFloat fastFilter (DcFilter<float>::Mode::Fast);

    slowFilter.prepare (sampleRate, blockSize);
    defaultFilter.prepare (sampleRate, blockSize);
    fastFilter.prepare (sampleRate, blockSize);

    const auto slowCoeff = slowFilter.getCoefficient();
    const auto defaultCoeff = defaultFilter.getCoefficient();
    const auto fastCoeff = fastFilter.getCoefficient();

    // Slower cutoff should have higher coefficient (closer to 1)
    EXPECT_GT (slowCoeff, defaultCoeff);
    EXPECT_GT (defaultCoeff, fastCoeff);

    // All should be in valid range
    EXPECT_GT (slowCoeff, 0.9f);
    EXPECT_GT (defaultCoeff, 0.9f);
    EXPECT_GT (fastCoeff, 0.9f);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (DcFilterTests, DoublePrecision)
{
    filterDouble.setMode (DcFilter<double>::Mode::Default);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (DcFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);
    filterDouble.setMode (DcFilter<double>::Mode::Default);

    const int numSamples = 100;
    std::vector<float> inputF (numSamples);
    std::vector<double> inputD (numSamples);
    std::vector<float> outputF (numSamples);
    std::vector<double> outputD (numSamples);

    // Generate test signal with DC offset
    for (int i = 0; i < numSamples; ++i)
    {
        const auto value = 0.2 + 0.3 * std::sin (2.0 * MathConstants<double>::pi * 200.0 * i / sampleRate);
        inputF[i] = static_cast<float> (value);
        inputD[i] = value;
    }

    filterFloat.processBlock (inputF.data(), outputF.data(), numSamples);
    filterDouble.processBlock (inputD.data(), outputD.data(), numSamples);

    // Results should be similar within reasonable tolerance
    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_NEAR (outputF[i], static_cast<float> (outputD[i]), 1e-4f);
    }
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F (DcFilterTests, LargeSignalStability)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Test with large input signals
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (100.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 1000.0f); // Should not blow up
    }
}

TEST_F (DcFilterTests, VariableSampleRateStability)
{
    std::vector<double> testSampleRates = { 8000.0, 16000.0, 44100.0, 48000.0, 96000.0, 192000.0 };

    for (const auto sr : testSampleRates)
    {
        filterFloat.prepare (sr, blockSize);

        // Test processing at this sample rate
        for (int i = 0; i < 100; ++i)
        {
            const float input = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 100.0f * i / static_cast<float> (sr));
            const auto output = filterFloat.processSample (input);
            EXPECT_TRUE (std::isfinite (output));
        }

        // Coefficient should be valid
        const auto coeff = filterFloat.getCoefficient();
        EXPECT_GT (coeff, 0.5f);
        EXPECT_LT (coeff, 1.0f);
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (DcFilterTests, ResetClearsState)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Build up state with DC signal
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, response to zero input should be much smaller
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
    EXPECT_LT (std::abs (outputAfterReset), 0.01f);
}

TEST_F (DcFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change mode mid-stream
    filterFloat.setMode (DcFilter<float>::Mode::Fast);

    // Should continue processing without issues
    for (int i = 0; i < 50; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Change to custom cutoff
    filterFloat.setCutoffFrequency (15.0f);

    for (int i = 0; i < 50; ++i)
    {
        const auto output = filterFloat.processSample (0.5f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Edge Case Tests
//==============================================================================

TEST_F (DcFilterTests, ZeroInput)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), toleranceF); // Should decay to zero
    }
}

TEST_F (DcFilterTests, AlternatingSignal)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Test with alternating +1/-1 signal (no DC component)
    std::vector<float> outputs;
    outputs.reserve (200);

    for (int i = 0; i < 200; ++i)
    {
        const float input = (i % 2 == 0) ? 1.0f : -1.0f;
        outputs.push_back (filterFloat.processSample (input));
    }

    // Calculate average (should be near zero since no DC)
    float average = 0.0f;
    const int startIdx = 50; // Skip initial transient
    for (int i = startIdx; i < static_cast<int> (outputs.size()); ++i)
    {
        average += outputs[i];
    }
    average /= (outputs.size() - startIdx);

    EXPECT_LT (std::abs (average), 0.1f);
}

//==============================================================================
// Application Scenario Tests
//==============================================================================

TEST_F (DcFilterTests, DISABLED_AudioProcessingScenario)
{
    filterFloat.setMode (DcFilter<float>::Mode::Default);

    // Simulate audio processing scenario with various frequencies and DC
    const float dcOffset = 0.1f;
    const std::vector<float> frequencies = { 50.0f, 100.0f, 440.0f, 1000.0f, 5000.0f };

    for (const auto freq : frequencies)
    {
        filterFloat.reset();

        std::vector<float> outputs;
        outputs.reserve (1000);

        // Process sinusoid with DC offset
        for (int i = 0; i < 1000; ++i)
        {
            const float acComponent = 0.5f * std::sin (2.0f * MathConstants<float>::pi * freq * i / static_cast<float> (sampleRate));
            const float input = acComponent + dcOffset;
            outputs.push_back (filterFloat.processSample (input));
        }

        // Calculate average of latter half (DC should be removed)
        float average = 0.0f;
        const int startIdx = 500;
        for (int i = startIdx; i < static_cast<int> (outputs.size()); ++i)
        {
            average += outputs[i];
        }
        average /= (outputs.size() - startIdx);

        // DC should be mostly removed
        EXPECT_LT (std::abs (average), 0.05f);

        // All outputs should be finite
        for (const auto output : outputs)
        {
            EXPECT_TRUE (std::isfinite (output));
        }
    }
}

TEST_F (DcFilterTests, ModeComparisonScenario)
{
    // Test all three modes with the same input
    DcFilterFloat slowFilter (DcFilter<float>::Mode::Slow);
    DcFilterFloat defaultFilter (DcFilter<float>::Mode::Default);
    DcFilterFloat fastFilter (DcFilter<float>::Mode::Fast);

    slowFilter.prepare (sampleRate, blockSize);
    defaultFilter.prepare (sampleRate, blockSize);
    fastFilter.prepare (sampleRate, blockSize);

    // Test with low frequency signal (20Hz) + DC
    const float freq = 20.0f;
    const float dcOffset = 0.3f;

    std::vector<float> slowOutputs, defaultOutputs, fastOutputs;

    for (int i = 0; i < 2000; ++i)
    {
        const float acComponent = 0.4f * std::sin (2.0f * MathConstants<float>::pi * freq * i / static_cast<float> (sampleRate));
        const float input = acComponent + dcOffset;

        slowOutputs.push_back (slowFilter.processSample (input));
        defaultOutputs.push_back (defaultFilter.processSample (input));
        fastOutputs.push_back (fastFilter.processSample (input));
    }

    // Calculate RMS of latter half for each mode
    auto calculateRMS = [] (const std::vector<float>& signal, int startIdx = 1000)
    {
        float sum = 0.0f;
        const int count = static_cast<int> (signal.size()) - startIdx;
        for (int i = startIdx; i < static_cast<int> (signal.size()); ++i)
        {
            sum += signal[i] * signal[i];
        }
        return std::sqrt (sum / count);
    };

    const auto slowRMS = calculateRMS (slowOutputs);
    const auto defaultRMS = calculateRMS (defaultOutputs);
    const auto fastRMS = calculateRMS (fastOutputs);

    // Slow mode should preserve more of the 20Hz signal than fast mode
    EXPECT_GT (slowRMS, fastRMS);

    // All should be reasonable values
    EXPECT_GT (slowRMS, 0.1f);
    EXPECT_GT (defaultRMS, 0.1f);
    EXPECT_GT (fastRMS, 0.05f);
}
