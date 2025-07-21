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

//==============================================================================
class DspMathTests : public ::testing::Test
{
protected:
    static constexpr double tolerance = 1e-6;
};

TEST_F (DspMathTests, FrequencyConversion)
{
    const double sampleRate = 44100.0;
    const double frequency = 1000.0;

    const auto omega = DspMath::frequencyToAngular (frequency, sampleRate);
    const auto backToFreq = DspMath::angularToFrequency (omega, sampleRate);

    EXPECT_NEAR (backToFreq, frequency, tolerance);
}

TEST_F (DspMathTests, QToBandwidthConversion)
{
    const double q = 0.707;
    const auto bandwidth = DspMath::qToBandwidth (q);
    const auto backToQ = DspMath::bandwidthToQ (bandwidth);

    EXPECT_NEAR (backToQ, q, tolerance);
}

TEST_F (DspMathTests, DecibelConversion)
{
    const double gainDb = 6.0;
    const auto linearGain = DspMath::dbToGain (gainDb);
    const auto backToDb = DspMath::gainToDb (linearGain);

    EXPECT_NEAR (backToDb, gainDb, tolerance);
    EXPECT_NEAR (linearGain, 2.0, tolerance); // 6dB = 2x gain
}

#if 0 // TODO - Disable the tests for now until they are moved to their own files

//==============================================================================
class BiquadTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        biquad.prepare (sampleRate, blockSize);
    }

    BiquadFloat biquad;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-5f;
};

TEST_F (BiquadTests, Initialization)
{
    EXPECT_NO_THROW (biquad.reset());
    EXPECT_NO_THROW (biquad.prepare (sampleRate, blockSize));
}

TEST_F (BiquadTests, ProcessSample)
{
    // Test with unity gain coefficients (should pass through)
    BiquadCoefficients<double> coeffs (1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    biquad.setCoefficients (coeffs);

    const float input = 0.5f;
    const auto output = biquad.processSample (input);

    EXPECT_NEAR (output, input, tolerance);
}

TEST_F (BiquadTests, ProcessBlock)
{
    // Test with unity gain coefficients
    BiquadCoefficients<double> coeffs (1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    biquad.setCoefficients (coeffs);

    const int numSamples = 32;
    std::vector<float> input (numSamples, 0.5f);
    std::vector<float> output (numSamples);

    biquad.processBlock (input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
        EXPECT_NEAR (output[i], input[i], tolerance);
}

TEST_F (BiquadTests, TopologyComparison)
{
    // Compare Direct Form I and II with same coefficients
    BiquadFloat df1 (Biquad<float>::Topology::directFormI);
    BiquadFloat df2 (Biquad<float>::Topology::directFormII);

    df1.prepare (sampleRate, blockSize);
    df2.prepare (sampleRate, blockSize);

    // Simple lowpass coefficients
    BiquadCoefficients<double> coeffs (0.1, 0.2, 0.1, 1.0, -0.5, 0.1);
    df1.setCoefficients (coeffs);
    df2.setCoefficients (coeffs);

    const int numSamples = 100;
    std::vector<float> input (numSamples);
    std::vector<float> output1 (numSamples);
    std::vector<float> output2 (numSamples);

    // Generate impulse response
    for (int i = 0; i < numSamples; ++i)
        input[i] = (i == 0) ? 1.0f : 0.0f;

    df1.processBlock (input.data(), output1.data(), numSamples);
    df2.processBlock (input.data(), output2.data(), numSamples);

    // Outputs should be very similar (within numerical precision)
    for (int i = 0; i < numSamples; ++i)
        EXPECT_NEAR (output1[i], output2[i], tolerance * 10); // Slightly higher tolerance for different topologies
}

//==============================================================================
class StateVariableFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        svf.prepare (sampleRate, blockSize);
    }

    StateVariableFilterFloat svf;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-4f;
};

TEST_F (StateVariableFilterTests, Initialization)
{
    EXPECT_NO_THROW (svf.setParameters (1000.0f, 0.707f, sampleRate));
    EXPECT_EQ (svf.getCutoffFrequency(), 1000.0f);
    EXPECT_EQ (svf.getQFactor(), 0.707f);
}

TEST_F (StateVariableFilterTests, AllOutputsSimultaneous)
{
    svf.setParameters (1000.0f, 0.707f, sampleRate);

    const float input = 1.0f;
    const auto outputs = svf.processAllOutputs (input);

    // Basic sanity checks
    EXPECT_TRUE (std::isfinite (outputs.lowpass));
    EXPECT_TRUE (std::isfinite (outputs.bandpass));
    EXPECT_TRUE (std::isfinite (outputs.highpass));
    EXPECT_TRUE (std::isfinite (outputs.notch));
}

TEST_F (StateVariableFilterTests, ModeSelection)
{
    svf.setParameters (1000.0f, 0.707f, sampleRate);

    const float input = 1.0f;

    svf.setMode (StateVariableFilter<float>::Mode::lowpass);
    const auto lpOutput = svf.processSample (input);

    svf.reset();
    svf.setMode (StateVariableFilter<float>::Mode::highpass);
    const auto hpOutput = svf.processSample (input);

    // Different modes should produce different outputs for transient input
    EXPECT_NE (lpOutput, hpOutput);
}

//==============================================================================
class ButterworthFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    ButterworthFilterFloat filter;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-4f;
};

TEST_F (ButterworthFilterTests, Initialization)
{
    EXPECT_NO_THROW (filter.setParameters (FilterType::lowpass, 4, 1000.0f, sampleRate));
    EXPECT_EQ (filter.getOrder(), 4);
    EXPECT_EQ (filter.getCutoffFrequency(), 1000.0f);
    EXPECT_EQ (filter.getFilterType(), FilterType::lowpass);
}

TEST_F (ButterworthFilterTests, OrderValidation)
{
    filter.setParameters (FilterType::lowpass, 25, 1000.0f, sampleRate); // Should clamp to 20
    EXPECT_EQ (filter.getOrder(), 20);

    filter.setParameters (FilterType::lowpass, -5, 1000.0f, sampleRate); // Should clamp to 1
    EXPECT_EQ (filter.getOrder(), 1);
}

TEST_F (ButterworthFilterTests, FrequencyResponse)
{
    filter.setParameters (FilterType::lowpass, 2, 1000.0f, sampleRate);

    // Test frequency response at cutoff frequency
    const auto responseAtCutoff = filter.getMagnitudeResponse (1000.0f);

    // Butterworth filter should be approximately -3dB at cutoff
    const auto expectedMagnitude = std::pow (10.0f, -3.0f / 20.0f); // -3dB in linear
    EXPECT_NEAR (responseAtCutoff, expectedMagnitude, 0.1f);
}

//==============================================================================
class RbjFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    RbjFilterFloat filter;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-4f;
};

TEST_F (RbjFilterTests, PeakingFilter)
{
    filter.setParameters (RbjFilter<float>::Type::peaking, 1000.0f, 0.707f, 6.0f, sampleRate);

    EXPECT_EQ (filter.getFrequency(), 1000.0f);
    EXPECT_EQ (filter.getQ(), 0.707f);
    EXPECT_EQ (filter.getGain(), 6.0f);
    EXPECT_EQ (filter.getType(), RbjFilter<float>::Type::peaking);
}

TEST_F (RbjFilterTests, FrequencyResponsePeaking)
{
    filter.setParameters (RbjFilter<float>::Type::peaking, 1000.0f, 1.0f, 6.0f, sampleRate);

    // At center frequency, peaking filter should provide the specified gain
    const auto responseAtCenter = filter.getMagnitudeResponse (1000.0f);
    const auto expectedGain = DspMath::dbToGain (6.0f);

    EXPECT_NEAR (responseAtCenter, expectedGain, 0.1f);
}

//==============================================================================
class LinkwitzRileyFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    LinkwitzRileyFilterFloat filter;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-3f;
};

TEST_F (LinkwitzRileyFilterTests, Initialization)
{
    EXPECT_NO_THROW (filter.setParameters (4, 1000.0f, sampleRate));
    EXPECT_EQ (filter.getOrder(), 4);
    EXPECT_EQ (filter.getCrossoverFrequency(), 1000.0f);
}

TEST_F (LinkwitzRileyFilterTests, EvenOrderEnforcement)
{
    filter.setParameters (3, 1000.0f, sampleRate); // Should become 4
    EXPECT_EQ (filter.getOrder(), 4);

    filter.setParameters (5, 1000.0f, sampleRate); // Should become 6
    EXPECT_EQ (filter.getOrder(), 6);
}

TEST_F (LinkwitzRileyFilterTests, PerfectReconstruction)
{
    filter.setParameters (4, 1000.0f, sampleRate);

    // Test perfect reconstruction at various frequencies
    const std::vector<float> testFrequencies = { 100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };

    for (const auto freq : testFrequencies)
    {
        const auto summedMagnitude = filter.verifySummedResponse (freq);

        // Should be close to 1.0 (0dB) for perfect reconstruction
        EXPECT_NEAR (summedMagnitude, 1.0f, 0.1f);
    }
}

TEST_F (LinkwitzRileyFilterTests, CrossoverFrequencyResponse)
{
    filter.setParameters (4, 1000.0f, sampleRate);

    // At crossover frequency, both LP and HP should be -6dB
    const auto lpResponse = std::abs (filter.getLowpassResponse (1000.0f));
    const auto hpResponse = std::abs (filter.getHighpassResponse (1000.0f));

    const auto expected6dB = std::pow (10.0f, -6.0f / 20.0f); // -6dB in linear

    EXPECT_NEAR (lpResponse, expected6dB, 0.1f);
    EXPECT_NEAR (hpResponse, expected6dB, 0.1f);
}

//==============================================================================
class FirstOrderFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    FirstOrderFilterFloat filter;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-4f;
};

TEST_F (FirstOrderFilterTests, LowpassConfiguration)
{
    EXPECT_NO_THROW (filter.makeLowpass (1000.0f, sampleRate));

    // Test that it processes without throwing
    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirstOrderFilterTests, HighpassConfiguration)
{
    EXPECT_NO_THROW (filter.makeHighpass (1000.0f, sampleRate));

    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirstOrderFilterTests, ShelfFilters)
{
    EXPECT_NO_THROW (filter.makeLowShelf (1000.0f, 6.0f, sampleRate));
    EXPECT_NO_THROW (filter.makeHighShelf (1000.0f, -3.0f, sampleRate));

    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

//==============================================================================
class FirFilterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        filter.prepare (sampleRate, blockSize);
    }

    FirFilterFloat filter;
    double sampleRate = 44100.0;
    int blockSize = 256;
    static constexpr float tolerance = 1e-4f;
};

TEST_F (FirFilterTests, Initialization)
{
    EXPECT_NO_THROW (filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate));
    EXPECT_EQ (filter.getType(), FirFilter<float>::Type::lowpass);
    EXPECT_EQ (filter.getLength(), 64);
    EXPECT_EQ (filter.getCutoffFrequency(), 1000.0f);
}

TEST_F (FirFilterTests, LowpassResponse)
{
    filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    // Test that it processes without throwing
    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirFilterTests, HighpassResponse)
{
    filter.setParameters (FirFilter<float>::Type::highpass, 64, 1000.0f, sampleRate);

    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirFilterTests, BandpassResponse)
{
    filter.setBandParameters (FirFilter<float>::Type::bandpass, 128, 500.0f, 2000.0f, sampleRate);

    EXPECT_EQ (filter.getType(), FirFilter<float>::Type::bandpass);
    EXPECT_EQ (filter.getCutoffFrequency(), 500.0f);
    EXPECT_EQ (filter.getSecondCutoffFrequency(), 2000.0f);

    const float input = 0.5f;
    const auto output = filter.processSample (input);
    EXPECT_TRUE (std::isfinite (output));
}

TEST_F (FirFilterTests, FilterLength)
{
    filter.setParameters (FirFilter<float>::Type::lowpass, 32, 1000.0f, sampleRate);
    EXPECT_EQ (filter.getLength(), 32);
    EXPECT_EQ (filter.getCoefficients().size(), 32);

    filter.setParameters (FirFilter<float>::Type::lowpass, 128, 1000.0f, sampleRate);
    EXPECT_EQ (filter.getLength(), 128);
    EXPECT_EQ (filter.getCoefficients().size(), 128);
}

TEST_F (FirFilterTests, KaiserBetaParameter)
{
    filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 3.0f);
    EXPECT_EQ (filter.getKaiserBeta(), 3.0f);

    filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate, 9.0f);
    EXPECT_EQ (filter.getKaiserBeta(), 9.0f);
}

TEST_F (FirFilterTests, LinearPhaseProperty)
{
    filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const auto& coeffs = filter.getCoefficients();
    const int length = filter.getLength();

    // FIR filters should have symmetric coefficients for linear phase
    for (int i = 0; i < length / 2; ++i)
    {
        EXPECT_NEAR (coeffs[static_cast<size_t> (i)],
                     coeffs[static_cast<size_t> (length - 1 - i)],
                     tolerance);
    }
}

TEST_F (FirFilterTests, BlockProcessing)
{
    filter.setParameters (FirFilter<float>::Type::lowpass, 64, 1000.0f, sampleRate);

    const int numSamples = 32;
    std::vector<float> input (numSamples, 0.1f);
    std::vector<float> output (numSamples);

    EXPECT_NO_THROW (filter.processBlock (input.data(), output.data(), numSamples));

    for (int i = 0; i < numSamples; ++i)
        EXPECT_TRUE (std::isfinite (output[i]));
}

#endif
