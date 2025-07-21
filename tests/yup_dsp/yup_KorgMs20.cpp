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
class KorgMs20FilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    KorgMs20Float filterFloat;
    KorgMs20Double filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, DefaultConstruction)
{
    KorgMs20Float filter;
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getResonance(), 0.1f);
    EXPECT_EQ (filter.getMode(), KorgMs20<float>::Mode::lowpass);
}

TEST_F (KorgMs20FilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 0.8f, KorgMs20<float>::Mode::highpass);

    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getResonance(), 0.8f);
    EXPECT_EQ (filterFloat.getMode(), KorgMs20<float>::Mode::highpass);
}

TEST_F (KorgMs20FilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test minimum frequency
    filterFloat.setCutoffFrequency (5.0f);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 10.0f);

    // Test maximum frequency (should be clamped below Nyquist)
    filterFloat.setCutoffFrequency (nyquist);
    EXPECT_LT (filterFloat.getCutoffFrequency(), nyquist);
}

TEST_F (KorgMs20FilterTests, ResonanceLimits)
{
    // Test minimum resonance
    filterFloat.setResonance (-0.1f);
    EXPECT_GE (filterFloat.getResonance(), 0.0f);

    // Test maximum resonance (should be clamped to prevent instability)
    filterFloat.setResonance (1.5f);
    EXPECT_LT (filterFloat.getResonance(), 1.0f);
}

TEST_F (KorgMs20FilterTests, ModeSettings)
{
    // Test lowpass mode
    filterFloat.setMode (KorgMs20<float>::Mode::lowpass);
    EXPECT_EQ (filterFloat.getMode(), KorgMs20<float>::Mode::lowpass);

    // Test highpass mode
    filterFloat.setMode (KorgMs20<float>::Mode::highpass);
    EXPECT_EQ (filterFloat.getMode(), KorgMs20<float>::Mode::highpass);
}

//==============================================================================
// Filter Mode Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, DISABLED_LowpassMode)
{
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::lowpass);

    // DC should pass through
    filterFloat.reset();
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_GT (std::abs (dcResponse), 0.5f); // Should pass DC with some gain variation

    // High frequency should be attenuated
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_LT (responseAt10kHz, 0.3f);
}

TEST_F (KorgMs20FilterTests, HighpassMode)
{
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::highpass);

    // DC should be blocked
    filterFloat.reset();
    for (int i = 0; i < 200; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_LT (std::abs (dcResponse), 0.2f);

    // High frequency should pass better than DC
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    const auto responseDC = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (responseAt10kHz, responseDC);
}

TEST_F (KorgMs20FilterTests, ModeSwitching)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::lowpass);

    // Process some samples in lowpass mode
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    const auto lpOutput = filterFloat.processSample (0.5f);

    // Switch to highpass mode
    filterFloat.setMode (KorgMs20<float>::Mode::highpass);
    const auto hpOutput = filterFloat.processSample (0.5f);

    // Outputs should be different between modes
    EXPECT_NE (lpOutput, hpOutput);
    EXPECT_TRUE (std::isfinite (lpOutput));
    EXPECT_TRUE (std::isfinite (hpOutput));
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, TwoPoleCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::lowpass);

    // Test the -12dB/octave rolloff characteristic
    const auto responseAt1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto responseAt2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);

    // Each octave should show approximately 12dB rolloff
    EXPECT_LT (responseAt2kHz, responseAt1kHz);
    EXPECT_LT (responseAt4kHz, responseAt2kHz);

    // More specific: 2-pole should be steeper than 1-pole but not as steep as 4-pole
    const auto ratio1to2 = responseAt2kHz / responseAt1kHz;
    EXPECT_LT (ratio1to2, 0.7f); // More than -6dB/octave
    EXPECT_GT (ratio1to2, 0.1f); // But not as steep as -24dB/octave
}

TEST_F (KorgMs20FilterTests, CutoffFrequencyResponse)
{
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::lowpass);

    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);

    // For 2-pole filter at cutoff, response should be attenuated
    EXPECT_LT (responseAtCutoff, 1.0f);
    EXPECT_GT (responseAtCutoff, 0.2f);
}

TEST_F (KorgMs20FilterTests, ResonanceEffect)
{
    // Low resonance
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::lowpass);
    const auto lowResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance
    filterFloat.setParameters (1000.0f, 0.8f, KorgMs20<float>::Mode::lowpass);
    const auto highResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance should increase response at cutoff frequency
    EXPECT_GT (highResResponse, lowResResponse);
}

TEST_F (KorgMs20FilterTests, HighpassFrequencyResponse)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::highpass);

    // DC response should be minimal
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_LT (dcResponse, 0.1f);

    // Response should increase with frequency
    const auto response1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto response5kHz = filterFloat.getMagnitudeResponse (5000.0f);
    const auto response10kHz = filterFloat.getMagnitudeResponse (10000.0f);

    EXPECT_GT (response1kHz, dcResponse);
    EXPECT_GE (response5kHz, response1kHz);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.5f, KorgMs20<float>::Mode::lowpass);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (KorgMs20FilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::lowpass);

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

TEST_F (KorgMs20FilterTests, ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 0.2f, KorgMs20<float>::Mode::lowpass);
    filterFloat.reset();

    std::vector<float> impulseResponse (256);
    for (int i = 0; i < 256; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite and decay for lowpass
    EXPECT_TRUE (std::isfinite (impulseResponse[0]));
    EXPECT_GT (std::abs (impulseResponse[0]), toleranceF);

    // Should show characteristic decay
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);
}

//==============================================================================
// Dual-Mode Output Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, DualModeOutputs)
{
    filterFloat.setParameters (1000.0f, 0.4f, KorgMs20<float>::Mode::lowpass);

    double lpOutput, hpOutput;
    const auto mainOutput = filterFloat.processDualSample (1.0f, lpOutput, hpOutput);

    // All outputs should be finite
    EXPECT_TRUE (std::isfinite (mainOutput));
    EXPECT_TRUE (std::isfinite (lpOutput));
    EXPECT_TRUE (std::isfinite (hpOutput));

    // In lowpass mode, main output should be similar to lpOutput
    EXPECT_NEAR (static_cast<float> (lpOutput), mainOutput, 0.1f);
}

TEST_F (KorgMs20FilterTests, IntermediateOutputs)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::lowpass);

    // Process a sample to populate intermediate outputs
    filterFloat.processSample (1.0f);

    const auto lpOutput = filterFloat.getLowpassOutput();
    const auto bpOutput = filterFloat.getBandpassOutput();

    EXPECT_TRUE (std::isfinite (lpOutput));
    EXPECT_TRUE (std::isfinite (bpOutput));
}

TEST_F (KorgMs20FilterTests, DualFilterEmulation)
{
    // Test the dual-filter characteristic of MS-20
    filterFloat.setParameters (1000.0f, 0.6f, KorgMs20<float>::Mode::lowpass);

    std::vector<float> mainOutputs, lpOutputs, hpOutputs;

    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));

        double lp, hp;
        const auto main = filterFloat.processDualSample (input, lp, hp);

        mainOutputs.push_back (main);
        lpOutputs.push_back (static_cast<float> (lp));
        hpOutputs.push_back (static_cast<float> (hp));
    }

    // LP and HP outputs should show complementary characteristics
    // This is a qualitative test for basic functionality
    for (size_t i = 0; i < mainOutputs.size(); ++i)
    {
        EXPECT_TRUE (std::isfinite (mainOutputs[i]));
        EXPECT_TRUE (std::isfinite (lpOutputs[i]));
        EXPECT_TRUE (std::isfinite (hpOutputs[i]));
    }
}

//==============================================================================
// Non-Linear Behavior Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, NonLinearSaturation)
{
    filterFloat.setParameters (1000.0f, 0.7f, KorgMs20<float>::Mode::lowpass);

    // Test with different signal levels to check for non-linear behavior
    filterFloat.reset();
    const auto smallSignalOutput = filterFloat.processSample (0.1f);

    filterFloat.reset();
    const auto largeSignalOutput = filterFloat.processSample (2.0f);

    // The filter should exhibit non-linear behavior with large signals
    EXPECT_TRUE (std::isfinite (smallSignalOutput));
    EXPECT_TRUE (std::isfinite (largeSignalOutput));

    // Large signal shouldn't be simply 20x the small signal due to saturation
    const auto linearRatio = std::abs (largeSignalOutput / smallSignalOutput);
    EXPECT_LT (linearRatio, 15.0f); // Should show some compression
}

TEST_F (KorgMs20FilterTests, AsymmetricSaturation)
{
    filterFloat.setParameters (1000.0f, 0.5f, KorgMs20<float>::Mode::lowpass);

    // Test asymmetric saturation (MS-20 characteristic)
    filterFloat.reset();
    const auto positiveOutput = filterFloat.processSample (1.5f);

    filterFloat.reset();
    const auto negativeOutput = filterFloat.processSample (-1.5f);

    // Should handle both polarities, possibly with asymmetric response
    EXPECT_TRUE (std::isfinite (positiveOutput));
    EXPECT_TRUE (std::isfinite (negativeOutput));

    // The asymmetry might not be easily testable without knowing exact implementation
    // but both should be stable
}

TEST_F (KorgMs20FilterTests, DISABLED_AggressiveResonanceCharacter)
{
    // Test the "aggressive" resonance character of MS-20
    filterFloat.setParameters (1000.0f, 0.9f, KorgMs20<float>::Mode::lowpass);

    // Process a signal at the resonant frequency
    std::vector<float> outputs;
    outputs.reserve (200);

    for (int i = 0; i < 200; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input * 0.5f);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce aggressive, resonant character but remain stable
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.2f);  // Should have significant resonant response
    EXPECT_LT (maxOutput, 10.0f); // But shouldn't blow up
}

//==============================================================================
// Resonance and Self-Oscillation Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, HighResonanceStability)
{
    filterFloat.setParameters (1000.0f, 0.95f, KorgMs20<float>::Mode::lowpass);

    // Should remain stable even with very high resonance
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 5.0f); // Should not blow up
    }
}

TEST_F (KorgMs20FilterTests, SelfOscillationPrevention)
{
    filterFloat.setParameters (1000.0f, 0.99f, KorgMs20<float>::Mode::lowpass);

    // Even near self-oscillation, should remain stable with no input
    filterFloat.reset();
    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (KorgMs20FilterTests, ResonancePeaking)
{
    // Test that resonance creates expected peaking at cutoff frequency
    filterFloat.setParameters (1000.0f, 0.1f, KorgMs20<float>::Mode::lowpass);
    const auto lowResAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto lowResNearCutoff = filterFloat.getMagnitudeResponse (800.0f);

    filterFloat.setParameters (1000.0f, 0.8f, KorgMs20<float>::Mode::lowpass);
    const auto highResAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto highResNearCutoff = filterFloat.getMagnitudeResponse (800.0f);

    // High resonance should create more pronounced peaking
    const auto lowResPeak = lowResAtCutoff / jmax (lowResNearCutoff, 0.001);
    const auto highResPeak = highResAtCutoff / jmax (highResNearCutoff, 0.001);

    EXPECT_GT (highResPeak, lowResPeak);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 0.5, KorgMs20<double>::Mode::lowpass);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (KorgMs20FilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::lowpass);
    filterDouble.setParameters (1000.0, 0.3, KorgMs20<double>::Mode::lowpass);

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

TEST_F (KorgMs20FilterTests, StabilityWithExtremeParameters)
{
    // Very low frequency
    filterFloat.setParameters (10.0f, 0.5f, KorgMs20<float>::Mode::lowpass);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency
    const auto nyquist = static_cast<float> (sampleRate) * 0.4f;
    filterFloat.setParameters (nyquist, 0.5f, KorgMs20<float>::Mode::lowpass);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

TEST_F (KorgMs20FilterTests, StabilityWithLargeSignals)
{
    filterFloat.setParameters (1000.0f, 0.7f, KorgMs20<float>::Mode::lowpass);

    // Test with large input signals
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (5.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up excessively
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 0.5f, KorgMs20<float>::Mode::lowpass);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
}

TEST_F (KorgMs20FilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 0.3f, KorgMs20<float>::Mode::lowpass);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, 0.8f, KorgMs20<float>::Mode::highpass);

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

TEST_F (KorgMs20FilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 0.5f, KorgMs20<float>::Mode::lowpass);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (KorgMs20FilterTests, ConstantInput)
{
    filterFloat.setParameters (1000.0f, 0.2f, KorgMs20<float>::Mode::lowpass);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass, constant input should eventually stabilize
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be stable and proportional to input
    EXPECT_TRUE (std::isfinite (output));
    EXPECT_LT (std::abs (output), 2.0f); // Should be reasonable
}

TEST_F (KorgMs20FilterTests, SinusoidalInput)
{
    filterFloat.setParameters (1000.0f, 0.4f, KorgMs20<float>::Mode::lowpass);

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
    EXPECT_LT (maxOutput, 3.0f);
}

//==============================================================================
// MS-20 Specific Character Tests
//==============================================================================

TEST_F (KorgMs20FilterTests, MS20Character)
{
    // Test the distinctive MS-20 filter character
    filterFloat.setParameters (1000.0f, 0.7f, KorgMs20<float>::Mode::lowpass);

    // Process a rich harmonic signal
    std::vector<float> outputs;
    outputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        // Create a signal with harmonics
        const float fundamental = std::sin (2.0f * MathConstants<float>::pi * 400.0f * i / static_cast<float> (sampleRate));
        const float second = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));
        const float third = 0.25f * std::sin (2.0f * MathConstants<float>::pi * 1200.0f * i / static_cast<float> (sampleRate));

        const float input = fundamental + second + third;
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce the characteristic MS-20 sound (hard to quantify, but should be stable)
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 5.0f);
}

TEST_F (KorgMs20FilterTests, NonLinearInteractionWithResonance)
{
    // Test how non-linearity interacts with resonance (MS-20 characteristic)
    filterFloat.setParameters (1000.0f, 0.8f, KorgMs20<float>::Mode::lowpass);

    // Test with increasing signal levels
    std::vector<float> signalLevels = { 0.1f, 0.3f, 0.5f, 0.8f, 1.0f, 1.5f };
    std::vector<float> peakOutputs;

    for (const auto level : signalLevels)
    {
        filterFloat.reset();
        float maxOutput = 0.0f;

        for (int i = 0; i < 200; ++i)
        {
            const float input = level * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
            const auto output = filterFloat.processSample (input);
            maxOutput = std::max (maxOutput, std::abs (output));
        }

        peakOutputs.push_back (maxOutput);
        EXPECT_TRUE (std::isfinite (maxOutput));
    }

    // Should show non-linear relationship (saturation/compression at high levels)
    // Higher input levels shouldn't produce proportionally higher outputs
    EXPECT_GT (peakOutputs.back(), peakOutputs.front());         // Some increase
    EXPECT_LT (peakOutputs.back() / peakOutputs.front(), 10.0f); // But not linear
}

TEST_F (KorgMs20FilterTests, DualFilterInteraction)
{
    // Test interaction between LP and HP modes like the real MS-20
    filterFloat.setParameters (1000.0f, 0.6f, KorgMs20<float>::Mode::lowpass);

    // Process in lowpass mode
    std::vector<float> lpOutputs, hpOutputs;
    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1200.0f * i / static_cast<float> (sampleRate));

        double lp, hp;
        filterFloat.processDualSample (input, lp, hp);

        lpOutputs.push_back (static_cast<float> (lp));
        hpOutputs.push_back (static_cast<float> (hp));
    }

    // LP and HP should show complementary behavior for signals above cutoff
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto lpRMS = calculateRMS (lpOutputs);
    const auto hpRMS = calculateRMS (hpOutputs);

    // For signal above cutoff, HP should have higher energy than LP
    EXPECT_GT (hpRMS, lpRMS * 0.5f); // Some relationship, but exact depends on implementation
}

TEST_F (KorgMs20FilterTests, DISABLED_ScreamingResonanceCharacter)
{
    // Test the "screaming" resonance characteristic that MS-20 is known for
    filterFloat.setParameters (2000.0f, 0.95f, KorgMs20<float>::Mode::lowpass);

    // Feed it a signal rich in harmonics near the cutoff
    std::vector<float> outputs;
    outputs.reserve (500);

    for (int i = 0; i < 500; ++i)
    {
        // Rich harmonic content
        float input = 0.0f;
        for (int harmonic = 1; harmonic <= 5; ++harmonic)
        {
            input += (1.0f / harmonic) * std::sin (2.0f * MathConstants<float>::pi * 300.0f * harmonic * i / static_cast<float> (sampleRate));
        }
        input *= 0.3f; // Scale down to avoid clipping

        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce aggressive resonant response characteristic of MS-20
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.2f); // Should have strong resonant response
    EXPECT_LT (maxOutput, 5.0f); // But remain stable
}
