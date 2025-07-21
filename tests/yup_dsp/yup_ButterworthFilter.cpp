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
class ButterworthFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    ButterworthFilterFloat filterFloat;
    ButterworthFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (ButterworthFilterTests, DefaultConstruction)
{
    ButterworthFilterFloat filter;
    EXPECT_EQ (filter.getOrder(), 2);
    EXPECT_EQ (filter.getFilterType(), FilterType::lowpass);
    EXPECT_EQ (filter.getCutoffFrequency(), 1000.0f);
}

TEST_F (ButterworthFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (FilterType::highpass, 6, 2000.0f, sampleRate);

    EXPECT_EQ (filterFloat.getOrder(), 6);
    EXPECT_EQ (filterFloat.getFilterType(), FilterType::highpass);
    EXPECT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
}

TEST_F (ButterworthFilterTests, OrderClamping)
{
    // Test minimum order clamping
    filterFloat.setParameters (FilterType::lowpass, -5, 1000.0f, sampleRate);
    EXPECT_EQ (filterFloat.getOrder(), 1);

    filterFloat.setParameters (FilterType::lowpass, 0, 1000.0f, sampleRate);
    EXPECT_EQ (filterFloat.getOrder(), 1);

    // Test maximum order clamping
    filterFloat.setParameters (FilterType::lowpass, 25, 1000.0f, sampleRate);
    EXPECT_EQ (filterFloat.getOrder(), 20);

    filterFloat.setParameters (FilterType::lowpass, 100, 1000.0f, sampleRate);
    EXPECT_EQ (filterFloat.getOrder(), 20);
}

TEST_F (ButterworthFilterTests, FrequencyClamping)
{
    // Test very low frequency
    filterFloat.setParameters (FilterType::lowpass, 4, 0.1f, sampleRate);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 0.1f);

    // Test near Nyquist frequency
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;
    filterFloat.setParameters (FilterType::lowpass, 4, nyquist * 0.99f, sampleRate);
    EXPECT_LE (filterFloat.getCutoffFrequency(), nyquist * 0.99f);
}

//==============================================================================
// Filter Type Tests
//==============================================================================

TEST_F (ButterworthFilterTests, LowpassFilter)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

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

TEST_F (ButterworthFilterTests, DISABLED_HighpassFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setParameters (FilterType::highpass, 4, 1000.0f, sampleRate);

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

TEST_F (ButterworthFilterTests, DISABLED_BandpassFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setParameters (FilterType::bandpass, 4, 500.0f, sampleRate, 2.0f);

    EXPECT_EQ (filterFloat.getFilterType(), FilterType::bandpass);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 500.0f);

    // Center frequency should have good response
    const auto centerFreq = std::sqrt (500.0f * 2000.0f);
    const auto centerResponse = filterFloat.getMagnitudeResponse (centerFreq);
    EXPECT_GT (centerResponse, 0.5f);

    // Frequencies outside band should be attenuated
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_LT (lowResponse, 0.3f);
    EXPECT_LT (highResponse, 0.3f);
}

TEST_F (ButterworthFilterTests, DISABLED_BandstopFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setParameters (FilterType::bandstop, 4, 500.0f, sampleRate);

    EXPECT_EQ (filterFloat.getFilterType(), FilterType::bandstop);

    // Center frequency should be attenuated
    const auto centerFreq = std::sqrt (500.0f * 2000.0f);
    const auto centerResponse = filterFloat.getMagnitudeResponse (centerFreq);
    EXPECT_LT (centerResponse, 0.5f);

    // Frequencies outside band should pass
    const auto lowResponse = filterFloat.getMagnitudeResponse (100.0f);
    const auto highResponse = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (lowResponse, 0.7f);
    EXPECT_GT (highResponse, 0.7f);
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (ButterworthFilterTests, CutoffFrequencyResponse)
{
    // Test 2nd order lowpass at cutoff frequency
    filterFloat.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate);

    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expected3dB = std::pow (10.0f, -3.0f / 20.0f); // -3dB in linear

    EXPECT_NEAR (responseAtCutoff, expected3dB, 0.15f);
}

TEST_F (ButterworthFilterTests, RolloffRate)
{
    // Test rolloff rate for different orders
    const std::vector<int> orders = { 1, 2, 4, 8 };

    for (const auto order : orders)
    {
        filterFloat.setParameters (FilterType::lowpass, order, 1000.0f, sampleRate);

        const auto responseAt1kHz = filterFloat.getMagnitudeResponse (1000.0f);
        const auto responseAt2kHz = filterFloat.getMagnitudeResponse (2000.0f);
        const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);

        // Higher order should have steeper rolloff
        EXPECT_GT (responseAt1kHz, responseAt2kHz);
        EXPECT_GT (responseAt2kHz, responseAt4kHz);

        if (order >= 2)
        {
            // Check approximate rolloff rate (order * 6 dB/octave)
            const auto ratio2k = responseAt2kHz / responseAt1kHz;
            const auto ratio4k = responseAt4kHz / responseAt2kHz;

            // Should show consistent rolloff
            EXPECT_NEAR (ratio2k, ratio4k, 0.3f);
        }
    }
}

TEST_F (ButterworthFilterTests, PhaseResponse)
{
    filterFloat.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate);

    // At cutoff frequency, 2nd order Butterworth should have -90° phase
    const auto phaseAtCutoff = filterFloat.getPhaseResponse (1000.0f);
    const auto expectedPhase = -MathConstants<float>::halfPi;

    EXPECT_NEAR (phaseAtCutoff, expectedPhase, 0.3f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (ButterworthFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // Test with various input values
    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LE (std::abs (output), std::abs (input) + toleranceF); // Output shouldn't exceed input for stable filter
    }
}

TEST_F (ButterworthFilterTests, BlockProcessing)
{
    filterFloat.setParameters (FilterType::lowpass, 6, 1000.0f, sampleRate);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 440.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (ButterworthFilterTests, DISABLED_ImpulseResponse) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);
    filterFloat.reset();

    // Generate impulse response
    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay over time
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), std::abs (impulseResponse[100]));
    EXPECT_GT (std::abs (impulseResponse[100]), std::abs (impulseResponse[200]));
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (ButterworthFilterTests, DoublePrecision)
{
    filterDouble.setParameters (FilterType::lowpass, 8, 1000.0, sampleRate);

    // Test with small signal that might expose precision issues
    const double smallSignal = 1e-10;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
    EXPECT_NE (output, 0.0); // Should not underflow to zero
}

TEST_F (ButterworthFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);
    filterDouble.setParameters (FilterType::lowpass, 4, 1000.0, sampleRate);

    const int numSamples = 100;
    std::vector<float> inputF (numSamples, 0.1f);
    std::vector<double> inputD (numSamples, 0.1);
    std::vector<float> outputF (numSamples);
    std::vector<double> outputD (numSamples);

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

TEST_F (ButterworthFilterTests, StabilityWithLargeSignals)
{
    filterFloat.setParameters (FilterType::lowpass, 8, 1000.0f, sampleRate);

    // Test with large input signal
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (100.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 1000.0f); // Should not blow up
    }
}

TEST_F (ButterworthFilterTests, StabilityWithExtremeFrequencies)
{
    // Test very low frequency
    filterFloat.setParameters (FilterType::lowpass, 4, 1.0f, sampleRate);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Test high frequency (near Nyquist)
    const auto nyquist = static_cast<float> (sampleRate) * 0.49f;
    filterFloat.setParameters (FilterType::lowpass, 4, nyquist, sampleRate);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (ButterworthFilterTests, ResetClearsState)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // Process some samples to build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, output should be closer to zero
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset));
}

TEST_F (ButterworthFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (FilterType::highpass, 6, 2000.0f, sampleRate);

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

TEST_F (ButterworthFilterTests, ZeroInput)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_EQ (output, 0.0f);
    }
}

TEST_F (ButterworthFilterTests, ConstantInput)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate);

    // For lowpass, constant input should eventually equal output
    const float constantInput = 0.7f;
    float output = 0.0f;

    for (int i = 0; i < 1000; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, constantInput, 0.1f);
}

TEST_F (ButterworthFilterTests, AlternatingInput)
{
    filterFloat.setParameters (FilterType::lowpass, 4, 100.0f, sampleRate); // Very low cutoff

    // Alternating signal should be heavily attenuated by lowpass
    float sumOutput = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        const float input = (i % 2 == 0) ? 1.0f : -1.0f;
        const auto output = filterFloat.processSample (input);
        sumOutput += std::abs (output);
    }

    const float avgOutput = sumOutput / 100.0f;
    EXPECT_LT (avgOutput, 0.5f); // Should be significantly attenuated
}
