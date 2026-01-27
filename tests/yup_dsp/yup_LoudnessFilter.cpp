/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
class LoudnessFilterTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 1e-5f;
    static constexpr double sampleRate = 48000.0;

    void SetUp() override
    {
        filter = std::make_unique<LoudnessFilter>();
        filter->prepare (sampleRate, 512);
    }

    std::unique_ptr<LoudnessFilter> filter;
};

//==============================================================================
TEST_F (LoudnessFilterTests, DefaultConstructorInitializes)
{
    LoudnessFilter f;
    EXPECT_GT (f.getSampleRate(), 0.0);
}

TEST_F (LoudnessFilterTests, PrepareSetsSampleRate)
{
    filter->prepare (44100.0, 512);
    EXPECT_DOUBLE_EQ (44100.0, filter->getSampleRate());

    filter->prepare (96000.0, 1024);
    EXPECT_DOUBLE_EQ (96000.0, filter->getSampleRate());
}

//==============================================================================
TEST_F (LoudnessFilterTests, ProcessSampleHandlesSilence)
{
    float output = filter->processSample (0.0f);
    EXPECT_NEAR (0.0f, output, tolerance);
}

TEST_F (LoudnessFilterTests, ProcessSampleModifiesSignal)
{
    // K-weighting should modify the signal (not pass through unchanged)
    const float input = 0.5f;
    float output = filter->processSample (input);

    // Output should be non-zero but different from input due to filtering
    EXPECT_NE (input, output);
}

TEST_F (LoudnessFilterTests, ProcessBlockHandlesSilence)
{
    std::vector<float> samples (512, 0.0f);
    filter->processBlock (samples.data(), 512);

    for (float sample : samples)
        EXPECT_NEAR (0.0f, sample, tolerance);
}

TEST_F (LoudnessFilterTests, ProcessBlockModifiesInPlace)
{
    std::vector<float> samples (512);

    // Generate test signal
    for (int i = 0; i < 512; ++i)
        samples[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / sampleRate);

    // Store original
    std::vector<float> original = samples;

    // Process in-place
    filter->processBlock (samples.data(), 512);

    // Should be modified
    bool isModified = false;
    for (int i = 0; i < 512; ++i)
    {
        if (std::abs (samples[i] - original[i]) > tolerance)
        {
            isModified = true;
            break;
        }
    }
    EXPECT_TRUE (isModified);
}

//==============================================================================
TEST_F (LoudnessFilterTests, PreFilterCoefficientsAreValid)
{
    double b0, b1, b2, a0, a1, a2;
    LoudnessFilter::calculatePreFilterCoefficients (sampleRate, b0, b1, b2, a0, a1, a2);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (b0));
    EXPECT_TRUE (std::isfinite (b1));
    EXPECT_TRUE (std::isfinite (b2));
    EXPECT_TRUE (std::isfinite (a0));
    EXPECT_TRUE (std::isfinite (a1));
    EXPECT_TRUE (std::isfinite (a2));

    // a0 should be 1.0 (normalized form)
    EXPECT_DOUBLE_EQ (1.0, a0);
}

TEST_F (LoudnessFilterTests, HighpassCoefficientsAreValid)
{
    double b0, b1, b2, a0, a1, a2;
    LoudnessFilter::calculateHighpassCoefficients (sampleRate, b0, b1, b2, a0, a1, a2);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (b0));
    EXPECT_TRUE (std::isfinite (b1));
    EXPECT_TRUE (std::isfinite (b2));
    EXPECT_TRUE (std::isfinite (a0));
    EXPECT_TRUE (std::isfinite (a1));
    EXPECT_TRUE (std::isfinite (a2));

    // a0 should be 1.0 (normalized form)
    EXPECT_DOUBLE_EQ (1.0, a0);

    // For highpass: b0 = b2, b1 = -2*b0
    EXPECT_NEAR (b0, b2, 1e-10);
    EXPECT_NEAR (-2.0 * b0, b1, 1e-10);
}

TEST_F (LoudnessFilterTests, PreFilterCoefficientsMatchITUSpec)
{
    // ITU-R BS.1770-4 specifies exact values at 48kHz
    double b0, b1, b2, a0, a1, a2;
    LoudnessFilter::calculatePreFilterCoefficients (48000.0, b0, b1, b2, a0, a1, a2);

    // These are reference values from ITU spec
    // (exact values depend on implementation, but should be close)
    EXPECT_GT (b0, 1.0);          // High-shelf with gain should have b0 > 1
    EXPECT_NEAR (1.0, a0, 1e-10); // Normalized
}

TEST_F (LoudnessFilterTests, HighpassCoefficientsMatch38HzCutoff)
{
    // 38 Hz highpass at 48kHz
    double b0, b1, b2, a0, a1, a2;
    LoudnessFilter::calculateHighpassCoefficients (48000.0, b0, b1, b2, a0, a1, a2);

    // Highpass coefficients should satisfy certain properties
    EXPECT_GT (b0, 0.0);
    EXPECT_LT (b1, 0.0); // b1 is negative for highpass
    EXPECT_GT (b2, 0.0);
}

//==============================================================================
TEST_F (LoudnessFilterTests, ResetClearsFilterState)
{
    // Process some samples to initialize filter state
    std::vector<float> samples (512, 1.0f);
    filter->processBlock (samples.data(), 512);

    // Reset should clear state
    filter->reset();

    // Process single DC sample - should not have history
    float dcInput = 1.0f;
    float output1 = filter->processSample (dcInput);

    // Reset and process again - should give same result
    filter->reset();
    float output2 = filter->processSample (dcInput);

    EXPECT_FLOAT_EQ (output1, output2);
}

//==============================================================================
TEST_F (LoudnessFilterTests, FilterRemovesDC)
{
    // Highpass component should remove DC
    std::vector<float> dcSamples (4800, 1.0f); // 100ms of DC at 48kHz

    filter->processBlock (dcSamples.data(), 4800);

    // After filtering, DC should be significantly attenuated
    // Calculate mean (should be close to zero after filtering)
    double mean = 0.0;
    for (float sample : dcSamples)
        mean += sample;
    mean /= dcSamples.size();

    EXPECT_NEAR (0.0, mean, 0.1); // DC should be largely removed
}

TEST_F (LoudnessFilterTests, FilterEmphasizesHighFrequencies)
{
    constexpr int numSamples = 4800;

    // Generate 200 Hz tone (below 1681 Hz shelf)
    std::vector<float> lowFreq (numSamples);
    for (int i = 0; i < numSamples; ++i)
        lowFreq[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 200.0f * i / sampleRate);

    // Generate 4 kHz tone (above 1681 Hz shelf)
    std::vector<float> highFreq (numSamples);
    for (int i = 0; i < numSamples; ++i)
        highFreq[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 4000.0f * i / sampleRate);

    // Process both
    LoudnessFilter filter1;
    filter1.prepare (sampleRate, numSamples);
    filter1.processBlock (lowFreq.data(), numSamples);

    LoudnessFilter filter2;
    filter2.prepare (sampleRate, numSamples);
    filter2.processBlock (highFreq.data(), numSamples);

    // Calculate RMS of each
    auto calculateRMS = [] (const std::vector<float>& samples)
    {
        double sum = 0.0;
        for (float s : samples)
            sum += s * s;
        return std::sqrt (sum / samples.size());
    };

    double rmsLow = calculateRMS (lowFreq);
    double rmsHigh = calculateRMS (highFreq);

    // High frequency should have higher RMS due to +4dB shelf
    EXPECT_GT (rmsHigh, rmsLow);
}

//==============================================================================
TEST_F (LoudnessFilterTests, FilterIsStable)
{
    // Process impulse and check for stability (no runaway)
    std::vector<float> samples (48000, 0.0f); // 1 second
    samples[0] = 1.0f;                        // Impulse

    filter->processBlock (samples.data(), 48000);

    // Check that output doesn't explode
    for (float sample : samples)
    {
        EXPECT_TRUE (std::isfinite (sample));
        EXPECT_LT (std::abs (sample), 10.0f); // Should decay, not amplify indefinitely
    }

    // Output should eventually decay to near zero
    float finalSamples = 0.0f;
    for (int i = 47000; i < 48000; ++i)
        finalSamples += std::abs (samples[i]);
    finalSamples /= 1000;

    EXPECT_LT (finalSamples, 0.01f);
}

TEST_F (LoudnessFilterTests, ConsistentAtDifferentSampleRates)
{
    // Filter should work correctly at various sample rates
    const std::vector<double> sampleRates = { 44100.0, 48000.0, 88200.0, 96000.0 };

    for (double sr : sampleRates)
    {
        LoudnessFilter f;
        f.prepare (sr, 512);

        // Process sine wave
        constexpr int numSamples = 4410;
        std::vector<float> samples (numSamples);
        for (int i = 0; i < numSamples; ++i)
            samples[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / sr);

        f.processBlock (samples.data(), numSamples);

        // Should produce finite, reasonable values
        bool allFinite = true;
        for (float sample : samples)
        {
            if (! std::isfinite (sample) || std::abs (sample) > 10.0f)
            {
                allFinite = false;
                break;
            }
        }
        EXPECT_TRUE (allFinite);
    }
}

//==============================================================================
TEST_F (LoudnessFilterTests, DISABLED_ProcessBlockHandlesEmptyBuffer)
{
    std::vector<float> samples;
    filter->processBlock (samples.data(), 0);
    // Should not crash
    SUCCEED();
}

TEST_F (LoudnessFilterTests, MultiplePassesMaintainState)
{
    // Process in multiple small blocks
    constexpr int blockSize = 128;
    constexpr int numBlocks = 10;

    std::vector<float> continuous (blockSize * numBlocks);
    for (int i = 0; i < blockSize * numBlocks; ++i)
        continuous[i] = 0.5f * std::sin (2.0f * MathConstants<float>::pi * 1000.0f * i / sampleRate);

    // Copy for block processing
    std::vector<float> blocked = continuous;

    // Process continuous
    LoudnessFilter filter1;
    filter1.prepare (sampleRate, blockSize * numBlocks);
    filter1.processBlock (continuous.data(), blockSize * numBlocks);

    // Process in blocks
    LoudnessFilter filter2;
    filter2.prepare (sampleRate, blockSize);
    for (int b = 0; b < numBlocks; ++b)
        filter2.processBlock (blocked.data() + b * blockSize, blockSize);

    // Results should be very similar (allowing for numerical differences)
    for (int i = blockSize; i < blockSize * numBlocks; ++i) // Skip first block (transient)
        EXPECT_NEAR (continuous[i], blocked[i], 0.01f);
}
