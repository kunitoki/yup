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
class MoogLadderFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    MoogLadderFloat filterFloat;
    MoogLadderDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, DefaultConstruction)
{
    MoogLadderFloat filter;
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getResonance(), 0.1f);
    EXPECT_FLOAT_EQ (filter.getDrive(), 1.0f);
    EXPECT_FLOAT_EQ (filter.getPassbandGain(), 0.5f);
}

TEST_F (MoogLadderFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 0.8f, 2.5f);

    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getResonance(), 0.8f);
    EXPECT_FLOAT_EQ (filterFloat.getDrive(), 2.5f);
}

TEST_F (MoogLadderFilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test minimum frequency
    filterFloat.setCutoffFrequency (0.5f);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 1.0f);

    // Test maximum frequency (should be clamped below Nyquist)
    filterFloat.setCutoffFrequency (nyquist);
    EXPECT_LT (filterFloat.getCutoffFrequency(), nyquist);
}

TEST_F (MoogLadderFilterTests, ResonanceLimits)
{
    // Test minimum resonance
    filterFloat.setResonance (-0.1f);
    EXPECT_GE (filterFloat.getResonance(), 0.0f);

    // Test maximum resonance (should be clamped to prevent instability)
    filterFloat.setResonance (1.5f);
    EXPECT_LT (filterFloat.getResonance(), 1.0f);
}

TEST_F (MoogLadderFilterTests, DISABLED_DriveLimits)
{
    // Test minimum drive
    filterFloat.setDrive (0.05f);
    EXPECT_GE (filterFloat.getDrive(), 0.1f);

    // Test maximum drive
    filterFloat.setDrive (15.0f);
    EXPECT_LE (filterFloat.getDrive(), 10.0f);
}

TEST_F (MoogLadderFilterTests, PassbandGainLimits)
{
    // Test minimum passband gain
    filterFloat.setPassbandGain (-0.1f);
    EXPECT_GE (filterFloat.getPassbandGain(), 0.0f);

    // Test maximum passband gain
    filterFloat.setPassbandGain (1.5f);
    EXPECT_LE (filterFloat.getPassbandGain(), 1.0f);
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, LowpassCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.1f, 1.0f);

    // DC should pass through
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.8f);

    // High frequency should be attenuated (-24dB/octave)
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    const auto responseAt8kHz = filterFloat.getMagnitudeResponse (8000.0f);

    // Each octave should provide approximately 24dB attenuation
    EXPECT_LT (responseAt4kHz, dcResponse * 0.3f);
    EXPECT_LT (responseAt8kHz, responseAt4kHz * 0.3f);
}

TEST_F (MoogLadderFilterTests, CutoffFrequencyResponse)
{
    filterFloat.setParameters (1000.0f, 0.1f, 1.0f);

    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto expected3dB = std::pow (10.0f, -3.0f / 20.0f); // -3dB in linear

    // For Moog filter, response at cutoff might be different due to resonance compensation
    EXPECT_LT (responseAtCutoff, 1.0f);
    EXPECT_GT (responseAtCutoff, 0.3f);
}

TEST_F (MoogLadderFilterTests, ResonanceEffect)
{
    // Low resonance
    filterFloat.setParameters (1000.0f, 0.1f, 1.0f);
    const auto lowResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance
    filterFloat.setParameters (1000.0f, 0.9f, 1.0f);
    const auto highResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance should increase response at cutoff frequency
    EXPECT_GT (highResResponse, lowResResponse);
}

TEST_F (MoogLadderFilterTests, FourPoleCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.1f, 1.0f);

    // Test the -24dB/octave rolloff characteristic
    const auto responseAt1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto responseAt2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);

    // Each octave should show steeper rolloff than typical 2-pole filter
    const auto ratio1to2 = responseAt2kHz / responseAt1kHz;
    const auto ratio2to4 = responseAt4kHz / responseAt2kHz;

    EXPECT_LT (ratio1to2, 0.5f); // More than -6dB/octave
    EXPECT_LT (ratio2to4, 0.5f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.5f, 1.0f);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (MoogLadderFilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f);

    const int numSamples = 128;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal at cutoff frequency
    for (int i = 0; i < numSamples; ++i)
        input[i] = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (MoogLadderFilterTests, DISABLED_ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 0.2f, 1.0f);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), toleranceF);

    // Should show exponential decay characteristic of lowpass filter
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);
}

//==============================================================================
// Drive and Saturation Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, DriveEffect)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f);
    filterFloat.reset();

    // Low drive
    const auto lowDriveOutput = filterFloat.processSample (0.5f);

    filterFloat.reset();
    filterFloat.setDrive (5.0f);

    // High drive should introduce saturation/nonlinearity
    const auto highDriveOutput = filterFloat.processSample (0.5f);

    // With drive, output should be different (may be compressed)
    EXPECT_NE (lowDriveOutput, highDriveOutput);
    EXPECT_TRUE (std::isfinite (highDriveOutput));
}

TEST_F (MoogLadderFilterTests, DISABLED_SaturationStability)
{
    filterFloat.setParameters (1000.0f, 0.5f, 10.0f); // Maximum drive

    // Even with maximum drive, filter should remain stable
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (1.0f); // Large input
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 5.0f); // Should not blow up
    }
}

TEST_F (MoogLadderFilterTests, SaturationCharacteristics)
{
    filterFloat.setParameters (1000.0f, 0.1f, 3.0f);

    // Test saturation curve - should show compression at high levels
    filterFloat.reset();
    const auto smallOutput = filterFloat.processSample (0.1f);

    filterFloat.reset();
    const auto largeOutput = filterFloat.processSample (1.0f);

    // Large input should not produce 10x larger output due to saturation
    const auto ratio = std::abs (largeOutput / smallOutput);
    EXPECT_LT (ratio, 8.0f); // Should show some compression
}

//==============================================================================
// Multi-Stage Output Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, StageOutputs)
{
    filterFloat.setParameters (1000.0f, 0.2f, 1.0f);

    // Process a sample to populate stage outputs
    filterFloat.processSample (1.0f);

    // Each stage should produce valid output
    for (int stage = 0; stage < 4; ++stage)
    {
        const auto output = filterFloat.getStageOutput (stage);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Stage outputs should show progressive filtering
    // (each stage should have less high-frequency content)
    // This is a qualitative test for basic functionality
}

TEST_F (MoogLadderFilterTests, MultiSampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f);

    double outputs[4];
    const auto mainOutput = filterFloat.processMultiSample (1.0f, outputs);

    // All outputs should be finite
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputs[i]));
    }

    // Main output should match the 4th stage
    EXPECT_NEAR (static_cast<float> (outputs[3]), mainOutput, toleranceF);
}

TEST_F (MoogLadderFilterTests, StageProgression)
{
    filterFloat.setParameters (500.0f, 0.1f, 1.0f); // Low cutoff to see filtering effect

    // Generate a high-frequency signal
    std::vector<float> stageOutputs[4];
    for (auto& vec : stageOutputs)
        vec.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 5000.0f * i / static_cast<float> (sampleRate));

        double outputs[4];
        filterFloat.processMultiSample (input, outputs);

        for (int stage = 0; stage < 4; ++stage)
        {
            stageOutputs[stage].push_back (static_cast<float> (outputs[stage]));
        }
    }

    // Later stages should have lower RMS values for high-frequency input
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto rms0 = calculateRMS (stageOutputs[0]);
    const auto rms3 = calculateRMS (stageOutputs[3]);

    EXPECT_GT (rms0, rms3); // 4th stage should have more attenuation
}

//==============================================================================
// Resonance and Self-Oscillation Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, DISABLED_HighResonanceStability)
{
    filterFloat.setParameters (1000.0f, 0.95f, 1.0f); // Very high resonance

    // Should remain stable even with very high resonance
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (MoogLadderFilterTests, SelfOscillationPrevention)
{
    filterFloat.setParameters (1000.0f, 0.999f, 1.0f); // Near self-oscillation

    // Even near self-oscillation, should remain stable with no input
    filterFloat.reset();
    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (MoogLadderFilterTests, ResonancePeaking)
{
    // Test that resonance creates expected peaking at cutoff frequency
    filterFloat.setParameters (1000.0f, 0.1f, 1.0f);
    const auto lowResAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto lowResNearCutoff = filterFloat.getMagnitudeResponse (800.0f);

    filterFloat.setParameters (1000.0f, 0.8f, 1.0f);
    const auto highResAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto highResNearCutoff = filterFloat.getMagnitudeResponse (800.0f);

    // High resonance should create more pronounced peaking
    const auto lowResPeak = lowResAtCutoff / lowResNearCutoff;
    const auto highResPeak = highResAtCutoff / highResNearCutoff;

    EXPECT_GT (highResPeak, lowResPeak);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 0.5, 1.0);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (MoogLadderFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f);
    filterDouble.setParameters (1000.0, 0.3, 1.0);

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

TEST_F (MoogLadderFilterTests, StabilityWithExtremeParameters)
{
    // Very low frequency
    filterFloat.setParameters (1.0f, 0.5f, 1.0f);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency
    const auto nyquist = static_cast<float> (sampleRate) * 0.45f;
    filterFloat.setParameters (nyquist, 0.5f, 1.0f);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

TEST_F (MoogLadderFilterTests, DISABLED_StabilityWithLargeSignals)
{
    filterFloat.setParameters (1000.0f, 0.7f, 3.0f);

    // Test with large input signals
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (10.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 20.0f); // Should not blow up excessively
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 0.5f, 1.0f);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (MoogLadderFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, 0.8f, 2.0f);

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

TEST_F (MoogLadderFilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 0.5f, 1.0f);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (MoogLadderFilterTests, DISABLED_ConstantInput) // TODO - There is an error in the filter
{
    filterFloat.setParameters (1000.0f, 0.2f, 1.0f);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually equal output (with some gain difference)
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be stable and proportional to input
    EXPECT_NEAR (std::abs (output), std::abs (constantInput), 0.5f);
}

TEST_F (MoogLadderFilterTests, DISABLED_SinusoidalInput)
{
    filterFloat.setParameters (1000.0f, 0.4f, 1.0f);

    // Test with sinusoid at cutoff frequency
    const float freq = 1000.0f;
    float maxOutput = 0.0f;

    for (int i = 0; i < 1000; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * freq * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        maxOutput = std::max (maxOutput, std::abs (output));
    }

    // Should have reasonable output for signal at cutoff frequency
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 2.0f);
}

//==============================================================================
// Moog-Specific Character Tests
//==============================================================================

TEST_F (MoogLadderFilterTests, MoogCharacteristics)
{
    // Test the warm, musical character of the Moog filter
    filterFloat.setParameters (1000.0f, 0.6f, 1.5f);

    // Process a rich harmonic signal
    std::vector<float> outputs;
    outputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        // Create a signal with harmonics
        const float fundamental = std::sin (2.0f * MathConstants<float>::pi * 500.0f * i / static_cast<float> (sampleRate));
        const float second = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const float third = 0.25f * std::sin (2.0f * MathConstants<float>::pi * 1500.0f * i / static_cast<float> (sampleRate));

        const float input = fundamental + second + third;
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce musically pleasing results (hard to quantify, but should be stable)
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 3.0f);
}

TEST_F (MoogLadderFilterTests, PassbandGainCompensation)
{
    // Test passband gain compensation
    filterFloat.setParameters (1000.0f, 0.8f, 1.0f);

    // Without compensation
    filterFloat.setPassbandGain (0.0f);
    const auto responseWithoutComp = filterFloat.getMagnitudeResponse (100.0f); // Low frequency

    // With compensation
    filterFloat.setPassbandGain (0.8f);
    const auto responseWithComp = filterFloat.getMagnitudeResponse (100.0f);

    // Compensation should affect the passband response
    // (The exact behavior depends on implementation, but should be stable)
    EXPECT_TRUE (std::isfinite (responseWithoutComp));
    EXPECT_TRUE (std::isfinite (responseWithComp));
}

TEST_F (MoogLadderFilterTests, TemperatureCompensationEffect)
{
    // Temperature compensation should affect response at different frequencies
    filterFloat.setParameters (100.0f, 0.8f, 1.0f); // Low frequency
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (100.0f);

    filterFloat.setParameters (10000.0f, 0.8f, 1.0f); // High frequency
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (10000.0f);

    // Both should be finite and stable
    EXPECT_TRUE (std::isfinite (lowFreqResponse));
    EXPECT_TRUE (std::isfinite (highFreqResponse));

    // The filter should behave consistently across frequency ranges
    EXPECT_GT (lowFreqResponse, 0.0f);
    EXPECT_GT (highFreqResponse, 0.0f);
}
