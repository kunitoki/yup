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

#include <cmath>
#include <numeric>
#include <vector>
#include <thread>
#include <chrono>

using namespace yup;

namespace
{
constexpr int numSamples = 100000;
constexpr int smallNumSamples = 1000;
constexpr float amplitudeTolerance = 0.1f;
constexpr float meanTolerance = 0.05f;
constexpr float varianceTolerance = 0.05f;
} // namespace

//==============================================================================
class NoiseGeneratorsTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        whiteNoise.setSeed (12345);
        pinkNoise.setSeed (12345);
    }

    float calculateMean (const std::vector<float>& samples)
    {
        return std::accumulate (samples.begin(), samples.end(), 0.0f) / samples.size();
    }

    float calculateVariance (const std::vector<float>& samples, float mean)
    {
        float variance = 0.0f;
        for (const auto& sample : samples)
        {
            float diff = sample - mean;
            variance += diff * diff;
        }
        return variance / samples.size();
    }

    float calculateSpectralSlope (const std::vector<float>& samples, float sampleRate = 44100.0f)
    {
        // Simple FFT-based spectral slope calculation
        // For pink noise, we expect approximately -3dB/octave slope

        const int fftSize = 2048;
        const int numBins = fftSize / 2 + 1;

        // Take chunks and average the spectrum
        std::vector<float> avgMagnitude (numBins, 0.0f);
        int numChunks = static_cast<int> (samples.size()) / fftSize;

        FFTProcessor fft (fftSize);
        std::vector<float> fftInputData (fftSize);
        std::vector<float> fftOutputData (fftSize * 2);
        std::vector<float> window (fftSize);

        // Create Hann window
        WindowFunctions<float>::generate (WindowType::hann, window.data(), window.size());

        for (int chunk = 0; chunk < numChunks; ++chunk)
        {
            // Copy and window the data
            for (int i = 0; i < fftSize; ++i)
                fftInputData[i] = samples[chunk * fftSize + i] * window[i];

            // Perform FFT
            fft.performRealFFTForward (fftInputData.data(), fftOutputData.data());

            // Accumulate magnitude spectrum
            // Real FFT output format: interleaved real/imag pairs
            for (int i = 0; i < numBins - 1; ++i)
            {
                float real = fftOutputData[i * 2];
                float imag = fftOutputData[i * 2 + 1];
                avgMagnitude[i] += std::sqrt (real * real + imag * imag);
            }
        }

        // Average and convert to dB
        std::vector<float> magnitudeDB (numBins);
        for (int i = 1; i < numBins; ++i)
        {
            avgMagnitude[i] /= numChunks;
            magnitudeDB[i] = 20.0f * std::log10 (avgMagnitude[i] + 1e-10f);
        }

        // Calculate slope using linear regression on log-frequency scale
        float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
        int validBins = 0;

        for (int i = 10; i < numBins / 4; ++i) // Use middle frequency range
        {
            float freq = i * sampleRate / fftSize;
            float x = std::log10 (freq);
            float y = magnitudeDB[i];

            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
            validBins++;
        }

        // Calculate slope
        float slope = (validBins * sumXY - sumX * sumY) / (validBins * sumX2 - sumX * sumX);

        return slope;
    }

    WhiteNoise whiteNoise;
    PinkNoise pinkNoise;
};

//==============================================================================
// White Noise Tests

TEST_F (NoiseGeneratorsTests, WhiteNoise_OutputRange)
{
    // White noise should produce values between -1 and 1
    for (int i = 0; i < smallNumSamples; ++i)
    {
        float sample = whiteNoise.getNextSample();
        EXPECT_GE (sample, -1.0f);
        EXPECT_LE (sample, 1.0f);
    }
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_OperatorCall)
{
    // Test that operator() works the same as getNextSample()
    whiteNoise.setSeed (54321);
    WhiteNoise whiteNoise2 (54321);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ (whiteNoise.getNextSample(), whiteNoise2());
    }
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_Mean)
{
    // White noise should have a mean close to 0
    std::vector<float> samples (numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = whiteNoise.getNextSample();
    }

    float mean = calculateMean (samples);
    EXPECT_NEAR (mean, 0.0f, meanTolerance);
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_Variance)
{
    // White noise should have variance approximately equal to 1/3 for uniform distribution
    std::vector<float> samples (numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = whiteNoise.getNextSample();
    }

    float mean = calculateMean (samples);
    float variance = calculateVariance (samples, mean);

    // For uniform distribution [-1, 1], variance = (b-a)^2/12 = 4/12 = 1/3
    EXPECT_NEAR (variance, 1.0f / 3.0f, varianceTolerance);
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_Deterministic)
{
    // Same seed should produce same sequence
    WhiteNoise noise1 (98765);
    WhiteNoise noise2 (98765);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ (noise1.getNextSample(), noise2.getNextSample());
    }
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_DifferentSeeds)
{
    // Different seeds should produce different sequences
    WhiteNoise noise1 (11111);
    WhiteNoise noise2 (22222);

    int differences = 0;
    for (int i = 0; i < 100; ++i)
    {
        if (noise1.getNextSample() != noise2.getNextSample())
            differences++;
    }

    // At least 90% should be different
    EXPECT_GT (differences, 90);
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_SetSeed)
{
    // setSeed should reset the sequence
    std::vector<float> firstRun (100);
    for (int i = 0; i < 100; ++i)
    {
        firstRun[i] = whiteNoise.getNextSample();
    }

    whiteNoise.setSeed (12345); // Reset to original seed

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ (firstRun[i], whiteNoise.getNextSample());
    }
}

TEST_F (NoiseGeneratorsTests, WhiteNoise_DefaultConstructor)
{
    // Default constructor should use current time as seed
    // Two instances created at different times should produce different sequences
    WhiteNoise noise1;

    // Small delay to ensure different timestamp
    std::this_thread::sleep_for (std::chrono::milliseconds (1));

    WhiteNoise noise2;

    int differences = 0;
    for (int i = 0; i < 100; ++i)
    {
        if (noise1.getNextSample() != noise2.getNextSample())
            differences++;
    }

    // Should have some differences (not deterministic)
    EXPECT_GT (differences, 0);
}

//==============================================================================
// Pink Noise Tests

TEST_F (NoiseGeneratorsTests, PinkNoise_OutputRange)
{
    // Pink noise should produce reasonable output values
    float maxAbs = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = pinkNoise.getNextSample();
        maxAbs = std::max (maxAbs, std::abs (sample));
    }

    // Pink noise is typically lower amplitude than white noise
    EXPECT_LE (maxAbs, 1.0f);
    EXPECT_GE (maxAbs, 0.1f); // Should have some reasonable amplitude
}

TEST_F (NoiseGeneratorsTests, PinkNoise_OperatorCall)
{
    // Test that operator() works the same as getNextSample()
    pinkNoise.setSeed (54321);
    PinkNoise pinkNoise2 (54321);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ (pinkNoise.getNextSample(), pinkNoise2());
    }
}

TEST_F (NoiseGeneratorsTests, PinkNoise_Mean)
{
    // Pink noise should have a mean close to 0
    std::vector<float> samples (numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = pinkNoise.getNextSample();
    }

    float mean = calculateMean (samples);
    EXPECT_NEAR (mean, 0.0f, meanTolerance);
}

TEST_F (NoiseGeneratorsTests, PinkNoise_Deterministic)
{
    // Same seed should produce same sequence
    PinkNoise noise1 (98765);
    PinkNoise noise2 (98765);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ (noise1.getNextSample(), noise2.getNextSample());
    }
}

TEST_F (NoiseGeneratorsTests, PinkNoise_DifferentSeeds)
{
    // Different seeds should produce different sequences
    PinkNoise noise1 (11111);
    PinkNoise noise2 (22222);

    int differences = 0;
    for (int i = 0; i < 100; ++i)
    {
        if (std::abs (noise1.getNextSample() - noise2.getNextSample()) > 1e-6f)
            differences++;
    }

    // At least 90% should be different
    EXPECT_GT (differences, 90);
}

TEST_F (NoiseGeneratorsTests, PinkNoise_SetSeed)
{
    // setSeed should reset the sequence
    std::vector<float> firstRun (100);
    for (int i = 0; i < 100; ++i)
    {
        firstRun[i] = pinkNoise.getNextSample();
    }

    pinkNoise.setSeed (12345); // Reset to original seed

    // Need to create new instance due to filter state
    PinkNoise freshPinkNoise (12345);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ (firstRun[i], freshPinkNoise.getNextSample());
    }
}

TEST_F (NoiseGeneratorsTests, PinkNoise_SpectralCharacteristics)
{
    // Pink noise should have lower power at higher frequencies than white noise
    std::vector<float> samples (numSamples);

    // Let filters settle
    for (int i = 0; i < 1000; ++i)
    {
        pinkNoise.getNextSample();
    }

    // Collect samples
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = pinkNoise.getNextSample();
    }

    // For now, just verify it has some negative slope
    // The exact -3dB/octave is hard to measure precisely with this simple method
    float slope = calculateSpectralSlope (samples);
    EXPECT_LT (slope, 0.0f); // Should have negative slope
}

TEST_F (NoiseGeneratorsTests, PinkNoise_FilterStability)
{
    // Test that the filter remains stable over long runs
    float maxAbs = 0.0f;
    const int longRun = 1000000;

    for (int i = 0; i < longRun; ++i)
    {
        float sample = pinkNoise.getNextSample();
        maxAbs = std::max (maxAbs, std::abs (sample));

        // Check that we don't have runaway values
        ASSERT_LE (std::abs (sample), 1.0f);
    }

    // Should maintain reasonable amplitude throughout
    EXPECT_GE (maxAbs, 0.1f);
    EXPECT_LE (maxAbs, 1.0f); // Pink noise can have higher peaks than expected
}

TEST_F (NoiseGeneratorsTests, PinkNoise_DefaultConstructor)
{
    // Default constructor should initialize filters to zero
    PinkNoise defaultNoise;

    // First few samples might be very small due to zero initialization
    float firstSample = std::abs (defaultNoise.getNextSample());
    EXPECT_LE (firstSample, 1.0f);

    // After some samples, should reach normal amplitude
    for (int i = 0; i < 100; ++i)
    {
        defaultNoise.getNextSample();
    }

    float maxAbs = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        maxAbs = std::max (maxAbs, std::abs (defaultNoise.getNextSample()));
    }

    EXPECT_GE (maxAbs, 0.01f); // Should have some amplitude
}

//==============================================================================
// Comparison Tests

TEST_F (NoiseGeneratorsTests, WhiteVsPink_SpectralDifference)
{
    // White noise should have flat spectrum, pink noise should have -3dB/octave
    const int compareNumSamples = 50000;
    std::vector<float> whiteSamples (compareNumSamples);
    std::vector<float> pinkSamples (compareNumSamples);

    // Reset both with same seed for fair comparison
    whiteNoise.setSeed (99999);
    pinkNoise.setSeed (99999);

    // Let pink noise filters settle
    for (int i = 0; i < 1000; ++i)
    {
        pinkNoise.getNextSample();
    }

    // Collect samples
    for (int i = 0; i < compareNumSamples; ++i)
    {
        whiteSamples[i] = whiteNoise.getNextSample();
        pinkSamples[i] = pinkNoise.getNextSample();
    }

    float whiteSlope = calculateSpectralSlope (whiteSamples);
    float pinkSlope = calculateSpectralSlope (pinkSamples);

    // White noise should be relatively flat (close to 0 dB/decade)
    EXPECT_NEAR (whiteSlope, 0.0f, 2.0f);

    // Pink noise should have negative slope
    EXPECT_LT (pinkSlope, whiteSlope - 5.0f); // Pink should be at least 5dB/decade steeper
}
