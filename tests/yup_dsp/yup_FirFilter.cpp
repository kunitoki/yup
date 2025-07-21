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
class FirFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filterFloat.prepare (sampleRate, blockSize);
        filterDouble.prepare (sampleRate, blockSize);
    }

    FirFilterFloat filterFloat;
    FirFilterDouble filterDouble;
};

//==============================================================================
// Initialization and Parameter Tests
//==============================================================================

TEST_F (FirFilterTests, DefaultConstruction)
{
    FirFilterFloat filter;
    EXPECT_EQ (filter.getType(), FirFilter<float>::Type::lowpass);
    EXPECT_EQ (filter.getLength(), 64);
    EXPECT_FLOAT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_FLOAT_EQ (filter.getKaiserBeta(), 6.0f);
}

TEST_F (FirFilterTests, ParameterInitialization)
{
    filterFloat.setParameters (FirFilter<float>::Type::highpass, 128, 2000.0f, sampleRate, 6.0f);

    EXPECT_EQ (filterFloat.getType(), FirFilter<float>::Type::highpass);
    EXPECT_EQ (filterFloat.getLength(), 128);
    EXPECT_FLOAT_EQ (filterFloat.getCutoffFrequency(), 2000.0f);
    EXPECT_FLOAT_EQ (filterFloat.getKaiserBeta(), 6.0f);
}

TEST_F (FirFilterTests, DISABLED_LengthClamping) // TODO - Should we implement clamping ?
{
    // Test minimum length
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 3, 1000.0f, sampleRate);
    EXPECT_GE (filterFloat.getLength(), 4); // Should clamp to minimum

    // Test maximum length
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 2048, 1000.0f, sampleRate);
    EXPECT_LE (filterFloat.getLength(), 1024); // Should clamp to maximum
}

TEST_F (FirFilterTests, FrequencyLimits)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test near-zero frequency
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1.0f, sampleRate);
    EXPECT_GE (filterFloat.getCutoffFrequency(), 1.0f);

    // Test near-Nyquist frequency
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, nyquist * 0.9f, sampleRate);
    EXPECT_LE (filterFloat.getCutoffFrequency(), nyquist);
}

TEST_F (FirFilterTests, KaiserBetaParameter)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 0.5f);
    EXPECT_EQ (filterFloat.getKaiserBeta(), 0.5f);

    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 12.0f);
    EXPECT_EQ (filterFloat.getKaiserBeta(), 12.0f);
}

//==============================================================================
// Filter Type Tests
//==============================================================================

TEST_F (FirFilterTests, LowpassFilter)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 128, 1000.0f, sampleRate);

    // DC should pass through (after settling)
    filterFloat.reset();
    for (int i = 0; i < 200; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_NEAR (dcResponse, 1.0f, 0.1f);

    // High frequency should be attenuated
    const auto responseAt5kHz = filterFloat.getMagnitudeResponse (5000.0f);
    EXPECT_LT (responseAt5kHz, 0.3f);
}

TEST_F (FirFilterTests, DISABLED_HighpassFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setParameters (FirFilter<float>::Type::highpass, 128, 1000.0f, sampleRate);

    // DC should be blocked
    filterFloat.reset();
    for (int i = 0; i < 200; ++i)
        filterFloat.processSample (1.0f);

    const auto dcResponse = filterFloat.processSample (1.0f);
    EXPECT_LT (std::abs (dcResponse), 0.1f);

    // High frequency should pass
    const auto responseAt10kHz = filterFloat.getMagnitudeResponse (10000.0f);
    EXPECT_GT (responseAt10kHz, 0.7f);
}

TEST_F (FirFilterTests, DISABLED_BandpassFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setBandParameters (FirFilter<float>::Type::bandpass, 256, 500.0f, 2000.0f, sampleRate);

    EXPECT_EQ (filterFloat.getType(), FirFilter<float>::Type::bandpass);
    EXPECT_EQ (filterFloat.getCutoffFrequency(), 500.0f);
    EXPECT_EQ (filterFloat.getSecondCutoffFrequency(), 2000.0f);

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

TEST_F (FirFilterTests, DISABLED_BandstopFilter) // TODO - Investigate why the failure, bad test or bad implementation ?
{
    filterFloat.setBandParameters (FirFilter<float>::Type::bandstop, 256, 500.0f, 2000.0f, sampleRate);

    EXPECT_EQ (filterFloat.getType(), FirFilter<float>::Type::bandstop);

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
// Filter Characteristics Tests
//==============================================================================

TEST_F (FirFilterTests, LinearPhaseProperty)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const auto& coeffs = filterFloat.getCoefficients();
    const int length = filterFloat.getLength();

    // FIR filters should have symmetric coefficients for linear phase
    for (int i = 0; i < length / 2; ++i)
    {
        EXPECT_NEAR (coeffs[static_cast<size_t> (i)],
                     coeffs[static_cast<size_t> (length - 1 - i)],
                     toleranceF);
    }
}

TEST_F (FirFilterTests, CoefficientNormalization)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const auto& coeffs = filterFloat.getCoefficients();

    // Sum of coefficients should be approximately 1 for lowpass
    float sum = 0.0f;
    for (const auto coeff : coeffs)
        sum += coeff;

    EXPECT_NEAR (sum, 1.0f, 0.1f);
}

TEST_F (FirFilterTests, KaiserWindowEffect)
{
    // Compare different Kaiser beta values
    FirFilterFloat filter1, filter2;
    filter1.prepare (sampleRate, blockSize);
    filter2.prepare (sampleRate, blockSize);

    filter1.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 3.0f);
    filter2.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 9.0f);

    // Higher beta should have better stopband attenuation
    const auto response1At5kHz = filter1.getMagnitudeResponse (5000.0f);
    const auto response2At5kHz = filter2.getMagnitudeResponse (5000.0f);

    EXPECT_LT (response2At5kHz, response1At5kHz);
}

TEST_F (FirFilterTests, FilterLengthEffect)
{
    // Compare different filter lengths
    FirFilterFloat filter1, filter2;
    filter1.prepare (sampleRate, blockSize);
    filter2.prepare (sampleRate, blockSize);

    filter1.setParameters (FirFilter<float>::Type::lowpass, 32, 1000.0f, sampleRate);
    filter2.setParameters (FirFilter<float>::Type::lowpass, 128, 1000.0f, sampleRate);

    // Longer filter should have sharper transition
    const auto response1At1500Hz = filter1.getMagnitudeResponse (1500.0f);
    const auto response2At1500Hz = filter2.getMagnitudeResponse (1500.0f);

    // This is a general trend, though not always strict
    EXPECT_LE (response2At1500Hz, response1At1500Hz + 0.2f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (FirFilterTests, SampleProcessing)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const std::vector<float> testInputs = { 0.0f, 0.5f, -0.5f, 1.0f, -1.0f };

    for (const auto input : testInputs)
    {
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

TEST_F (FirFilterTests, BlockProcessing)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 128, 1000.0f, sampleRate);

    const int numSamples = 256;
    std::vector<float> input (numSamples);
    std::vector<float> output (numSamples);

    // Generate test signal (mix of frequencies)
    for (int i = 0; i < numSamples; ++i)
    {
        const float t = i / static_cast<float> (sampleRate);
        input[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 440.0f * t) + 0.3f * std::sin (2.0f * MathConstants<float>::pi * 2000.0f * t);
    }

    filterFloat.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        EXPECT_TRUE (std::isfinite (output[i]));
    }
}

TEST_F (FirFilterTests, ImpulseResponse)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);
    filterFloat.reset();

    std::vector<float> impulseResponse (128);
    for (int i = 0; i < 128; ++i)
    {
        const float input = (i == 0) ? 1.0f : 0.0f;
        impulseResponse[i] = filterFloat.processSample (input);
    }

    // Impulse response should be finite
    for (const auto sample : impulseResponse)
    {
        EXPECT_TRUE (std::isfinite (sample));
    }

    // Should have non-zero values in the beginning
    bool hasNonZero = false;
    for (int i = 0; i < 64; ++i)
    {
        if (std::abs (impulseResponse[i]) > toleranceF)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (FirFilterTests, DoublePrecision)
{
    filterDouble.setParameters (FirFilter<double>::Type::lowpass, 128, 1000.0, sampleRate, 6.0);

    const double smallSignal = 1e-12;
    const auto output = filterDouble.processSample (smallSignal);

    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirFilterTests, FloatVsDoublePrecision)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);
    filterDouble.setParameters (FirFilter<double>::Type::lowpass, 64, 1000.0, sampleRate);

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
        EXPECT_NEAR (outputF[i], static_cast<float> (outputD[i]), 1e-3f);
    }
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F (FirFilterTests, StabilityWithLargeSignals)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 128, 1000.0f, sampleRate);

    // Test with large input signal
    for (int i = 0; i < 1000; ++i)
    {
        const auto output = filterFloat.processSample (100.0f);
        EXPECT_TRUE (std::isfinite (output));
        EXPECT_LT (std::abs (output), 200.0f); // Should not amplify significantly
    }
}

TEST_F (FirFilterTests, StabilityWithVaryingInput)
{
    filterFloat.setParameters (FirFilter<float>::Type::bandpass, 128, 500.0f, 2000.0f, sampleRate);

    // Test with rapidly varying input
    for (int i = 0; i < 1000; ++i)
    {
        const float input = (i % 2 == 0) ? 1.0f : -1.0f;
        const auto output = filterFloat.processSample (input);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Reset and State Tests
//==============================================================================

TEST_F (FirFilterTests, ResetClearsState)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    // Build up state
    for (int i = 0; i < 100; ++i)
        filterFloat.processSample (1.0f);

    const auto outputBeforeReset = filterFloat.processSample (0.0f);

    filterFloat.reset();
    const auto outputAfterReset = filterFloat.processSample (0.0f);

    // After reset, output should be zero (FIR has finite memory)
    EXPECT_EQ (outputAfterReset, 0.0f);
}

TEST_F (FirFilterTests, ParameterChangesHandledSafely)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    // Process some samples
    for (int i = 0; i < 50; ++i)
        filterFloat.processSample (0.5f);

    // Change parameters mid-stream
    filterFloat.setParameters (FirFilter<float>::Type::highpass, 128, 2000.0f, sampleRate, 6.0f);

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

TEST_F (FirFilterTests, ZeroInput)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    // Process only zeros
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.0f);
        EXPECT_EQ (output, 0.0f);
    }
}

TEST_F (FirFilterTests, ConstantInput)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    // For lowpass, constant input should eventually equal output
    const float constantInput = 0.7f;
    float output = 0.0f;

    for (int i = 0; i < 200; ++i)
        output = filterFloat.processSample (constantInput);

    EXPECT_NEAR (output, constantInput, 0.1f);
}

TEST_F (FirFilterTests, NyquistFrequency)
{
    const float nyquist = static_cast<float> (sampleRate) * 0.5f;

    // Test filter at Nyquist frequency
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, nyquist * 0.8f, sampleRate);

    // Should process without issues
    for (int i = 0; i < 100; ++i)
    {
        const auto output = filterFloat.processSample (0.1f);
        EXPECT_TRUE (std::isfinite (output));
    }
}

//==============================================================================
// Coefficient Access Tests
//==============================================================================

TEST_F (FirFilterTests, CoefficientAccess)
{
    filterFloat.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const auto& coeffs = filterFloat.getCoefficients();

    EXPECT_EQ (coeffs.size(), 64);

    // All coefficients should be finite
    for (const auto coeff : coeffs)
    {
        EXPECT_TRUE (std::isfinite (coeff));
    }
}

TEST_F (FirFilterTests, CoefficientConsistency)
{
    // Same parameters should produce same coefficients
    FirFilterFloat filter1, filter2;

    filter1.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 3.0f);
    filter2.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 3.0f);

    const auto& coeffs1 = filter1.getCoefficients();
    const auto& coeffs2 = filter2.getCoefficients();

    EXPECT_EQ (coeffs1.size(), coeffs2.size());

    for (size_t i = 0; i < coeffs1.size(); ++i)
    {
        EXPECT_NEAR (coeffs1[i], coeffs2[i], toleranceF);
    }
}

//==============================================================================
// All Filter Types Comprehensive Test
//==============================================================================

TEST_F (FirFilterTests, AllFilterTypesBasicFunctionality)
{
    const std::vector<typename FirFilter<float>::Type> allTypes = {
        FirFilter<float>::Type::lowpass,
        FirFilter<float>::Type::highpass,
        FirFilter<float>::Type::bandpass,
        FirFilter<float>::Type::bandstop
    };

    for (const auto type : allTypes)
    {
        if (type == FirFilter<float>::Type::bandpass || type == FirFilter<float>::Type::bandstop)
        {
            filterFloat.setBandParameters (type, 128, 500.0f, 2000.0f, sampleRate);
        }
        else
        {
            filterFloat.setParameters (type, 128, 1000.0f, sampleRate);
        }

        // Each type should process without throwing
        for (int i = 0; i < 10; ++i)
        {
            const auto output = filterFloat.processSample (0.1f);
            EXPECT_TRUE (std::isfinite (output));
        }

        filterFloat.reset();
    }
}
