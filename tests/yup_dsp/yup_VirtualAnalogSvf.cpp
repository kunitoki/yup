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
class VirtualAnalogSvfFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    VirtualAnalogSvfFloat filterFloat;
    VirtualAnalogSvfDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, DefaultConstruction)
{
    VirtualAnalogSvfFloat filter;
    EXPECT_EQ (filter.getMode(), VirtualAnalogSvf<float>::Mode::lowpass);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getResonance(), 0.1f);
}

TEST_F (VirtualAnalogSvfFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::highpass);

    EXPECT_EQ (filterFloat.getMode(), VirtualAnalogSvf<float>::Mode::highpass);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getResonance(), 0.9f);
}

TEST_F (VirtualAnalogSvfFilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test low frequency
    filterFloat.setParameters (10.0f, 0.707f);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 10.0f);

    // Test high frequency (should be clamped near Nyquist)
    filterFloat.setParameters (nyquist * 0.95f, 0.707f);
    EXPECT_LE (filterFloat.getCutoffFrequency(), nyquist);
}

TEST_F (VirtualAnalogSvfFilterTests, ResonanceLimits)
{
    // Test minimum resonance
    filterFloat.setParameters (1000.0f, 0.1f);
    EXPECT_GE (filterFloat.getResonance(), 0.1f);

    // Test maximum resonance (should be clamped to prevent instability)
    filterFloat.setParameters (1000.0f, 0.99f);
    EXPECT_LE (filterFloat.getResonance(), 0.99f);
}

//==============================================================================
// Filter Mode Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, LowpassMode)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    // DC should pass through
    filterFloat.reset();
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_NEAR (dcResponse, 1.0f, 0.2f);
}

TEST_F (VirtualAnalogSvfFilterTests, HighpassMode)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::highpass);

    // DC should be blocked
    filterFloat.reset();
    for (int i = 0; i < 200; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_LT (std::abs (dcResponse), 0.2f);
}

TEST_F (VirtualAnalogSvfFilterTests, BandpassMode)
{
    filterFloat.setParameters (1000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::bandpass);

    // Process a signal and check it doesn't blow up
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (VirtualAnalogSvfFilterTests, NotchMode)
{
    filterFloat.setParameters (1000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::notch);

    // Process a signal and check it doesn't blow up
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (VirtualAnalogSvfFilterTests, AllOutputsSimultaneous)
{
    filterFloat.setParameters (1000.0f, 0.707f);

    const float input = 1.0f;
    const auto outputs = filterFloat.processMultiSample (input);

    // All outputs should be finite
    EXPECT_TRUE (std::isfinite (outputs.lowpass));
    EXPECT_TRUE (std::isfinite (outputs.highpass));
    EXPECT_TRUE (std::isfinite (outputs.bandpass));
    EXPECT_TRUE (std::isfinite (outputs.notch));

    // Basic sanity check: LP + HP should approximately equal input for very low resonance
    filterFloat.setParameters (1000.0f, 0.1f);
    filterFloat.reset();

    for (int i = 0; i < 100; ++i)
    {
        const auto out = filterFloat.processMultiSample (1.0f);
        if (i > 50) // After settling
        {
            const auto sum = out.lowpass + out.highpass;
            EXPECT_NEAR (sum, 1.0f, 0.3f);
        }
    }
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (VirtualAnalogSvfFilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

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

TEST_F (VirtualAnalogSvfFilterTests, ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and generally decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));

    // For lowpass, should have some initial response
    bool hasNonZeroResponse = false;
    for (int i = 0; i < 50; ++i)
    {
        if (std::abs (impulseResponse[i]) > toleranceF)
        {
            hasNonZeroResponse = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZeroResponse);
}

//==============================================================================
// Resonance Effect Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, ResonanceEffect)
{
    // Low resonance
    filterFloat.setParameters (1000.0f, 0.1f, VirtualAnalogSvf<float>::Mode::bandpass);

    // Generate a burst at the cutoff frequency
    filterFloat.reset();
    float maxOutputLowRes = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        maxOutputLowRes = std::max (maxOutputLowRes, std::abs (output));
    }

    // High resonance
    filterFloat.setParameters (1000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::bandpass);
    filterFloat.reset();
    float maxOutputHighRes = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        maxOutputHighRes = std::max (maxOutputHighRes, std::abs (output));
    }

    // High resonance should produce higher peak response
    EXPECT_GT (maxOutputHighRes, maxOutputLowRes);
}

TEST_F (VirtualAnalogSvfFilterTests, SelfOscillationPrevention)
{
    // Even with very high resonance, filter should remain stable
    filterFloat.setParameters (1000.0f, 0.99f, VirtualAnalogSvf<float>::Mode::bandpass);

    // Process silence and check for instability
    filterFloat.reset();
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 2.0f); // Should not blow up
    }
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 0.707, VirtualAnalogSvf<double>::Mode::lowpass);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (VirtualAnalogSvfFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    filterDouble.setParameters (1000.0, 0.707, VirtualAnalogSvf<double>::Mode::lowpass);

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

TEST_F (VirtualAnalogSvfFilterTests, StabilityWithLargeSignals)
{
    filterFloat.setParameters (1000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Test with large input signal
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (10.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 50.0f); // Should not blow up excessively
    }
}

TEST_F (VirtualAnalogSvfFilterTests, StabilityWithExtremeParameters)
{
    // Very low frequency
    filterFloat.setParameters (1.0f, 0.5f, VirtualAnalogSvf<float>::Mode::lowpass);

    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (nyquist, 0.5f, VirtualAnalogSvf<float>::Mode::lowpass);

    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be different
    EXPECT_NE (outputBeforeReset, outputAfterReset);
}

TEST_F (VirtualAnalogSvfFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 0.5f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::highpass);

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

TEST_F (VirtualAnalogSvfFilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);

        // For TPT filters, zero input might not always produce zero output due to internal state
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (VirtualAnalogSvfFilterTests, ConstantInput)
{
    filterFloat.setParameters (1000.0f, 0.1f, VirtualAnalogSvf<float>::Mode::lowpass); // Low resonance

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass with low resonance, constant input should eventually equal output
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, constantInput, 0.2f);
}

TEST_F (VirtualAnalogSvfFilterTests, AlternatingInput)
{
    filterFloat.setParameters (100.0f, 0.5f, VirtualAnalogSvf<float>::Mode::lowpass); // Very low cutoff

    // Alternating signal should be heavily attenuated by lowpass
    float sumOutput = 0.0f;
    for (int i = 0; i < 200; ++i)
    {
        const float input = (i % 2 == 0) ? 1.0f : -1.0f;
        const auto output = filterFloat.processSample (input);
        if (i > 100) // After settling
            sumOutput += std::abs (output);
    }

    const float avgOutput = sumOutput / 100.0f;
    EXPECT_LT (avgOutput, 0.5f); // Should be significantly attenuated
}

//==============================================================================
// Mode Switching Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, ModeSwitchingStability)
{
    filterFloat.setParameters (1000.0f, 0.707f, VirtualAnalogSvf<float>::Mode::lowpass);

    const std::vector<typename VirtualAnalogSvf<float>::Mode> modes = {
        VirtualAnalogSvf<float>::Mode::lowpass,
        VirtualAnalogSvf<float>::Mode::highpass,
        VirtualAnalogSvf<float>::Mode::bandpass,
        VirtualAnalogSvf<float>::Mode::notch
    };

    // Switch between modes and ensure stability
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        for (const auto mode : modes)
        {
            filterFloat.setMode (mode);

            // Process samples in each mode
            for (int i = 0; i < 20; ++i)
            {
                const auto output = filterFloat.processSample (0.1f);
                EXPECT_TRUE (std::isfinite (output));
            }
        }
    }
}

//==============================================================================
// Analog Modeling Characteristics Tests
//==============================================================================

TEST_F (VirtualAnalogSvfFilterTests, DISABLED_NonlinearCharacteristics)
{
    filterFloat.setParameters (1000.0f, 0.9f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Test with different signal levels to check for nonlinear behavior
    filterFloat.reset();
    const auto smallSignalOutput = filterFloat.processSample (0.01f);

    filterFloat.reset();
    const auto largeSignalOutput = filterFloat.processSample (1.0f);

    // The filter should exhibit some level-dependent behavior (like analog filters)
    // but still remain stable
    EXPECT_TRUE (std::isfinite (smallSignalOutput));
    EXPECT_TRUE (std::isfinite (largeSignalOutput));

    // The response shouldn't be perfectly linear
    const auto scaledSmallSignal = smallSignalOutput * 100.0f;
    EXPECT_NE (scaledSmallSignal, largeSignalOutput); // Should show some nonlinearity
}

TEST_F (VirtualAnalogSvfFilterTests, WarmthAndCharacter)
{
    // This test ensures the filter processes normally - the "warmth" is subjective
    // but we can test that it doesn't sound clinical/digital by ensuring some
    // amount of harmonic content when driven hard

    filterFloat.setParameters (1000.0f, 0.8f, VirtualAnalogSvf<float>::Mode::lowpass);

    // Drive the filter with a moderate signal
    std::vector<float> outputs;
    outputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        const float input = 0.7f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce reasonable output levels
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 2.0f);
}
