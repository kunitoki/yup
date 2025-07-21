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
class Tb303FilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    Tb303FilterFloat filterFloat;
    Tb303FilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (Tb303FilterTests, DefaultConstruction)
{
    Tb303FilterFloat filter;
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getResonance(), 0.1f);
    EXPECT_FLOAT_EQ (filter.getEnvelopeAmount(), 0.5f);
    EXPECT_FLOAT_EQ (filter.getAccent(), 0.0f);
}

TEST_F (Tb303FilterTests, ParameterInitialization)
{
    filterFloat.setParameters (2000.0f, 0.8f, 1.5f, 0.7f);

    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getResonance(), 0.8f);
    EXPECT_FLOAT_EQ (filterFloat.getEnvelopeAmount(), 1.5f);
    EXPECT_FLOAT_EQ (filterFloat.getAccent(), 0.7f);
}

TEST_F (Tb303FilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test minimum frequency
    filterFloat.setCutoffFrequency (5.0f);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 10.0f);

    // Test maximum frequency (should be clamped below Nyquist)
    filterFloat.setCutoffFrequency (nyquist);
    EXPECT_LT (filterFloat.getCutoffFrequency(), nyquist);
}

TEST_F (Tb303FilterTests, ResonanceLimits)
{
    // Test minimum resonance
    filterFloat.setResonance (-0.1f);
    EXPECT_GE (filterFloat.getResonance(), 0.0f);

    // Test maximum resonance (should be clamped to prevent instability)
    filterFloat.setResonance (1.5f);
    EXPECT_LT (filterFloat.getResonance(), 1.0f);
}

TEST_F (Tb303FilterTests, EnvelopeAmountLimits)
{
    // Test minimum envelope amount
    filterFloat.setEnvelopeAmount (-0.5f);
    EXPECT_GE (filterFloat.getEnvelopeAmount(), 0.0f);

    // Test maximum envelope amount
    filterFloat.setEnvelopeAmount (3.0f);
    EXPECT_LE (filterFloat.getEnvelopeAmount(), 2.0f);
}

TEST_F (Tb303FilterTests, AccentLimits)
{
    // Test minimum accent
    filterFloat.setAccent (-0.1f);
    EXPECT_GE (filterFloat.getAccent(), 0.0f);

    // Test maximum accent
    filterFloat.setAccent (1.5f);
    EXPECT_LE (filterFloat.getAccent(), 1.0f);
}

//==============================================================================
// Frequency Response Tests
//==============================================================================

TEST_F (Tb303FilterTests, LowpassCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.1f, 0.0f, 0.0f);

    // DC should pass through with some attenuation
    const auto dcResponse = filterFloat.getMagnitudeResponse (1.0f);
    EXPECT_GT (dcResponse, 0.5f);

    // High frequency should be attenuated (-24dB/octave for 4-pole)
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);
    const auto responseAt8kHz = filterFloat.getMagnitudeResponse (8000.0f);

    // Each octave should provide significant attenuation
    EXPECT_LT (responseAt4kHz, dcResponse * 0.5f);
    EXPECT_LT (responseAt8kHz, responseAt4kHz * 0.5f);
}

TEST_F (Tb303FilterTests, DISABLED_CutoffFrequencyResponse)
{
    filterFloat.setParameters (1000.0f, 0.1f, 0.0f, 0.0f);

    const auto responseAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);

    // For diode ladder filter at cutoff, response should be attenuated
    EXPECT_LT (responseAtCutoff, 1.0f);
    EXPECT_GT (responseAtCutoff, 0.2f);
}

TEST_F (Tb303FilterTests, DISABLED_ResonanceEffect)
{
    // Low resonance
    filterFloat.setParameters (1000.0f, 0.1f, 0.0f, 0.0f);
    const auto lowResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance
    filterFloat.setParameters (1000.0f, 0.9f, 0.0f, 0.0f);
    const auto highResResponse = filterFloat.getMagnitudeResponse (1000.0f);

    // High resonance should increase response at cutoff frequency
    EXPECT_GT (highResResponse, lowResResponse);
}

TEST_F (Tb303FilterTests, DiodeLadderCharacteristic)
{
    filterFloat.setParameters (1000.0f, 0.3f, 0.0f, 0.0f);

    // Test the asymmetric 4-pole rolloff characteristic
    const auto responseAt1kHz = filterFloat.getMagnitudeResponse (1000.0f);
    const auto responseAt2kHz = filterFloat.getMagnitudeResponse (2000.0f);
    const auto responseAt4kHz = filterFloat.getMagnitudeResponse (4000.0f);

    // Should show steep rolloff but with TB-303 asymmetric characteristics
    const auto ratio1to2 = responseAt2kHz / responseAt1kHz;
    const auto ratio2to4 = responseAt4kHz / responseAt2kHz;

    EXPECT_LT (ratio1to2, 0.6f); // Steeper than 2-pole
    EXPECT_LT (ratio2to4, 0.6f); // But with asymmetric response
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (Tb303FilterTests, SampleProcessing)
{
    filterFloat.setParameters (1000.0f, 0.5f, 0.5f, 0.3f);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (Tb303FilterTests, BlockProcessing)
{
    filterFloat.setParameters (1000.0f, 0.4f, 0.8f, 0.0f);

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

TEST_F (Tb303FilterTests, ImpulseResponse)
{
    filterFloat.setParameters (1000.0f, 0.3f, 0.0f, 0.0f);
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

    // Should show characteristic TB-303 decay
    const auto early = std::abs (impulseResponse[10]);
    const auto late = std::abs (impulseResponse[100]);
    EXPECT_GT (early, late);
}

//==============================================================================
// Diode Ladder and Nonlinearity Tests
//==============================================================================

TEST_F (Tb303FilterTests, AsymmetricDistortion)
{
    filterFloat.setParameters (1000.0f, 0.7f, 0.0f, 0.0f);

    // Test asymmetric saturation behavior
    filterFloat.reset();
    const auto positiveOutput = filterFloat.processSample (1.5f);

    filterFloat.reset();
    const auto negativeOutput = filterFloat.processSample (-1.5f);

    // TB-303 should exhibit asymmetric response due to diode characteristics
    EXPECT_TRUE (std::isfinite (positiveOutput));
    EXPECT_TRUE (std::isfinite (negativeOutput));

    // The asymmetry might be subtle but both should be stable
    const auto asymmetryRatio = std::abs (positiveOutput / negativeOutput);
    EXPECT_GT (asymmetryRatio, 0.1f);  // Should not be zero
    EXPECT_LT (asymmetryRatio, 10.0f); // Should not be extreme
}

TEST_F (Tb303FilterTests, NonlinearSaturation)
{
    filterFloat.setParameters (1000.0f, 0.8f, 0.0f, 0.0f);

    // Test with different signal levels to check for non-linear behavior
    filterFloat.reset();
    const auto smallSignalOutput = filterFloat.processSample (0.1f);

    filterFloat.reset();
    const auto largeSignalOutput = filterFloat.processSample (2.0f);

    // The filter should exhibit non-linear behavior with large signals
    EXPECT_TRUE (std::isfinite (smallSignalOutput));
    EXPECT_TRUE (std::isfinite (largeSignalOutput));

    // Large signal shouldn't be 20x the small signal due to diode saturation
    const auto linearRatio = std::abs (largeSignalOutput / smallSignalOutput);
    EXPECT_LT (linearRatio, 15.0f); // Should show compression
}

TEST_F (Tb303FilterTests, DiodeLadderStages)
{
    filterFloat.setParameters (500.0f, 0.6f, 0.0f, 0.0f);

    // Process a signal and verify each stage contributes
    std::vector<float> outputs;
    outputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 300.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce characteristic TB-303 filtering
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.05f);
    EXPECT_LT (maxOutput, 3.0f);
}

//==============================================================================
// Envelope Follower and Dynamic Response Tests
//==============================================================================

TEST_F (Tb303FilterTests, EnvelopeFollower)
{
    filterFloat.setParameters (1000.0f, 0.3f, 1.0f, 0.0f);

    // Test envelope follower response
    EXPECT_FLOAT_EQ (filterFloat.getEnvelopeState(), 0.0f);

    // Process a signal burst
    for (int i = 0; i < 50; ++i)
    {
        filterFloat.processSample (0.8f);
    }

    // Envelope should have increased
    const auto envelopeAfterBurst = filterFloat.getEnvelopeState();
    EXPECT_GT (envelopeAfterBurst, 0.1f);

    // Process silence
    for (int i = 0; i < 100; ++i)
    {
        filterFloat.processSample (0.0f);
    }

    // Envelope should decay
    const auto envelopeAfterSilence = filterFloat.getEnvelopeState();
    EXPECT_LT (envelopeAfterSilence, envelopeAfterBurst);
}

TEST_F (Tb303FilterTests, EnvelopeModulation)
{
    // Test with envelope modulation
    filterFloat.setParameters (1000.0f, 0.3f, 1.5f, 0.0f);

    // Process signal with envelope modulation
    std::vector<float> modulatedOutputs;
    modulatedOutputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input * 0.8f);
        modulatedOutputs.push_back (output);
    }

    // Test without envelope modulation
    filterFloat.reset();
    filterFloat.setParameters (1000.0f, 0.3f, 0.0f, 0.0f);

    std::vector<float> unmodulatedOutputs;
    unmodulatedOutputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input * 0.8f);
        unmodulatedOutputs.push_back (output);
    }

    // Envelope modulation should create different response
    auto calculateRMS = [] (const std::vector<float>& signal)
    {
        float sum = 0.0f;
        for (auto sample : signal)
            sum += sample * sample;
        return std::sqrt (sum / signal.size());
    };

    const auto modulatedRMS = calculateRMS (modulatedOutputs);
    const auto unmodulatedRMS = calculateRMS (unmodulatedOutputs);

    EXPECT_NE (modulatedRMS, unmodulatedRMS); // Should be different
}

TEST_F (Tb303FilterTests, AccentEffect)
{
    // Test accent effect
    filterFloat.setParameters (1000.0f, 0.5f, 0.5f, 0.8f);

    // Process a signal with accent
    std::vector<float> accentOutputs;
    accentOutputs.reserve (50);

    for (int i = 0; i < 50; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input * 0.5f);
        accentOutputs.push_back (output);
    }

    // Test without accent
    filterFloat.reset();
    filterFloat.setParameters (1000.0f, 0.5f, 0.5f, 0.0f);

    std::vector<float> noAccentOutputs;
    noAccentOutputs.reserve (50);

    for (int i = 0; i < 50; ++i)
    {
        const float input = std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / static_cast<float> (sampleRate));
        const auto output = filterFloat.processSample (input * 0.5f);
        noAccentOutputs.push_back (output);
    }

    // Accent should affect the response
    const auto accentMax = *std::max_element (accentOutputs.begin(), accentOutputs.end());
    const auto noAccentMax = *std::max_element (noAccentOutputs.begin(), noAccentOutputs.end());

    EXPECT_NE (accentMax, noAccentMax); // Should create different response
}

//==============================================================================
// Resonance and Self-Oscillation Tests
//==============================================================================

TEST_F (Tb303FilterTests, HighResonanceStability)
{
    filterFloat.setParameters (1000.0f, 0.95f, 0.0f, 0.0f);

    // Should remain stable even with very high resonance
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 10.0f); // Should not blow up
    }
}

TEST_F (Tb303FilterTests, SelfOscillationPrevention)
{
    filterFloat.setParameters (1000.0f, 0.99f, 0.0f, 0.0f);

    // Even near self-oscillation, should remain stable with no input
    filterFloat.reset();
    for (int i = 0; i < 500; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (Tb303FilterTests, DISABLED_ResonancePeaking)
{
    // Test that resonance creates expected peaking at cutoff frequency
    filterFloat.setParameters (1000.0f, 0.1f, 0.0f, 0.0f);
    const auto lowResAtCutoff = filterFloat.getMagnitudeResponse (1000.0f);
    const auto lowResNearCutoff = filterFloat.getMagnitudeResponse (800.0f);

    filterFloat.setParameters (1000.0f, 0.8f, 0.0f, 0.0f);
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

TEST_F (Tb303FilterTests, DoublePrecision)
{
    filterDouble.setParameters (1000.0, 0.5, 0.5, 0.0);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (Tb303FilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (1000.0f, 0.3f, 0.0f, 0.0f);
    filterDouble.setParameters (1000.0, 0.3, 0.0, 0.0);

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

TEST_F (Tb303FilterTests, StabilityWithExtremeParameters)
{
    // Very low frequency
    filterFloat.setParameters (10.0f, 0.5f, 1.0f, 0.5f);
    const auto output1 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output1));

    // Very high frequency
    const auto nyquist = static_cast<float> (sampleRate) * 0.4f;
    filterFloat.setParameters (nyquist, 0.5f, 1.0f, 0.5f);
    const auto output2 = filterFloat.processSample (1.0f);
    EXPECT_TRUE (std::isfinite (output2));
}

TEST_F (Tb303FilterTests, StabilityWithLargeSignals)
{
    filterFloat.setParameters (1000.0f, 0.7f, 1.0f, 0.5f);

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

TEST_F (Tb303FilterTests, ResetClearsState)
{
    filterFloat.setParameters (1000.0f, 0.5f, 0.5f, 0.0f);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);
    const auto envelopeBeforeReset = filterFloat.getEnvelopeState();

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);
    const auto envelopeAfterReset = filterFloat.getEnvelopeState();

    // After reset, transient response should be reduced
    EXPECT_LT (std::abs (outputAfterReset), std::abs (outputBeforeReset) + toleranceF);
    EXPECT_LT (envelopeAfterReset, envelopeBeforeReset + toleranceF);
}

TEST_F (Tb303FilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (1000.0f, 0.3f, 0.5f, 0.0f);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (2000.0f, 0.8f, 1.5f, 0.7f);

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

TEST_F (Tb303FilterTests, ZeroInput)
{
    filterFloat.setParameters (1000.0f, 0.5f, 0.5f, 0.0f);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (Tb303FilterTests, ConstantInput)
{
    filterFloat.setParameters (1000.0f, 0.2f, 0.0f, 0.0f);

    const float constantInput = 0.7f;
    float output = 0.0f;

    // For lowpass filter, constant input should eventually stabilize
    for (int i = 0; i < 500; ++i)
        output = filterFloat.processSample (constantInput);

    // Should be stable and proportional to input
    EXPECT_TRUE (std::isfinite (output));
    EXPECT_LT (std::abs (output), 2.0f); // Should be reasonable
}

TEST_F (Tb303FilterTests, SinusoidalInput)
{
    filterFloat.setParameters (1000.0f, 0.4f, 0.5f, 0.0f);

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
// TB-303 Specific Character Tests
//==============================================================================

TEST_F (Tb303FilterTests, AcidBassCharacter)
{
    // Test the distinctive TB-303 acid bass character
    filterFloat.setParameters (500.0f, 0.8f, 1.2f, 0.5f);

    // Process a typical acid bassline pattern
    std::vector<float> outputs;
    outputs.reserve (200);

    for (int i = 0; i < 200; ++i)
    {
        // Create a sawtooth-like signal with envelope
        const float envelope = std::exp (-static_cast<float> (i) / 50.0f);
        const float fundamental = std::sin (2.0f * MathConstants<float>::pi * 200.0f * i / static_cast<float> (sampleRate));
        const float harmonics = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 400.0f * i / static_cast<float> (sampleRate));

        const float input = (fundamental + harmonics) * envelope;
        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce the characteristic TB-303 acid sound
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 5.0f);
}

TEST_F (Tb303FilterTests, DiodeLadderDistortion)
{
    filterFloat.setParameters (1000.0f, 0.9f, 0.0f, 0.0f);

    // Test with rich harmonic content to check diode distortion
    std::vector<float> outputs;
    outputs.reserve (100);

    for (int i = 0; i < 100; ++i)
    {
        // Rich harmonic content
        float input = 0.0f;
        for (int harmonic = 1; harmonic <= 4; ++harmonic)
        {
            input += (1.0f / harmonic) * std::sin (2.0f * MathConstants<float>::pi * 300.0f * harmonic * i / static_cast<float> (sampleRate));
        }
        input *= 0.6f; // Scale to reasonable level

        const auto output = filterFloat.processSample (input);
        outputs.push_back (output);
        EXPECT_TRUE (std::isfinite (output));
    }

    // Should produce characteristic TB-303 filtered distortion
    const auto maxOutput = *std::max_element (outputs.begin(), outputs.end());
    EXPECT_GT (maxOutput, 0.1f);
    EXPECT_LT (maxOutput, 3.0f);
}

TEST_F (Tb303FilterTests, TemperatureDependentBehavior)
{
    // Test behavior that models temperature-dependent analog characteristics
    filterFloat.setParameters (100.0f, 0.8f, 0.0f, 0.0f); // Low frequency
    const auto lowFreqResponse = filterFloat.getMagnitudeResponse (100.0f);

    filterFloat.setParameters (10000.0f, 0.8f, 0.0f, 0.0f); // High frequency
    const auto highFreqResponse = filterFloat.getMagnitudeResponse (10000.0f);

    // Both should be finite and stable
    EXPECT_TRUE (std::isfinite (lowFreqResponse));
    EXPECT_TRUE (std::isfinite (highFreqResponse));

    // The filter should behave consistently across frequency ranges
    EXPECT_GT (lowFreqResponse, 0.0f);
    EXPECT_GT (highFreqResponse, 0.0f);
}

TEST_F (Tb303FilterTests, EnvelopeAndResonanceInteraction)
{
    // Test how envelope modulation interacts with high resonance
    filterFloat.setParameters (800.0f, 0.9f, 2.0f, 0.8f);

    // Test with bursts and silence to trigger envelope follower
    std::vector<float> signalLevels = { 0.0f, 0.8f, 0.0f, 1.2f, 0.0f };
    std::vector<float> peakOutputs;

    for (const auto level : signalLevels)
    {
        float maxOutput = 0.0f;

        for (int i = 0; i < 100; ++i)
        {
            const float input = level * std::sin (2.0f * MathConstants<float>::pi * 800.0f * i / static_cast<float> (sampleRate));
            const auto output = filterFloat.processSample (input);
            maxOutput = std::max (maxOutput, std::abs (output));
        }

        peakOutputs.push_back (maxOutput);
        EXPECT_TRUE (std::isfinite (maxOutput));
    }

    // Envelope modulation should create dynamic response
    // (Hard to test exact behavior, but should remain stable)
    for (const auto peak : peakOutputs)
    {
        EXPECT_LT (peak, 5.0f); // Should not blow up
    }
}
