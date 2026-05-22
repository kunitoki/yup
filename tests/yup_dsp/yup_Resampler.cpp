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

#include <cmath>
#include <vector>

namespace yup::test
{

//==============================================================================
class ResamplerTest : public ::testing::Test
{
protected:
    static constexpr double sourceSampleRate = 44100.0;
    static constexpr double targetSampleRate = 48000.0;
    static constexpr int maxChannels = 2;
    static constexpr int blockSize = 256;

    void SetUp() override
    {
        resamplerUp.prepare (sourceSampleRate, targetSampleRate, maxChannels, blockSize);
        resamplerDown.prepare (targetSampleRate, sourceSampleRate, maxChannels, blockSize);
    }

    float calculateRMS (const float* data, int numSamples) const
    {
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sum += data[i] * data[i];
        return std::sqrt (sum / static_cast<float> (numSamples));
    }

    void fillSine (std::vector<float>& buf, double frequency, double sr, float amplitude = 1.0f) const
    {
        for (std::size_t i = 0; i < buf.size(); ++i)
        {
            buf[i] = amplitude * static_cast<float> (std::sin (MathConstants<double>::twoPi * frequency * static_cast<double> (i) / sr));
        }
    }

    void fillDC (std::vector<float>& buf, float value) const
    {
        std::fill (buf.begin(), buf.end(), value);
    }

    // Max output buffer size for a 44100 -> 48000 conversion
    static constexpr int maxOutputSize = static_cast<int> (blockSize * 48000.0 / 44100.0) + 2;

    Resampler<float, 8> resamplerUp;   // 44100 -> 48000
    Resampler<float, 8> resamplerDown; // 48000 -> 44100
};

//==============================================================================
TEST_F (ResamplerTest, DefaultConstructionDoesNotCrash)
{
    Resampler<float, 8> r;
    EXPECT_EQ (r.getLatencyInSamples(), 8);
}

TEST_F (ResamplerTest, LatencyReturnsCorrectValue)
{
    EXPECT_EQ (resamplerUp.getLatencyInSamples(), 8); // SincRadius = 8
    EXPECT_EQ (resamplerDown.getLatencyInSamples(), 8);
}

TEST_F (ResamplerTest, UpsampleProducesMoreOutputThanInput)
{
    std::vector<float> input (blockSize, 0.0f);
    std::vector<float> output (maxOutputSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    int produced = resamplerUp.resample (inPtrs, outPtrs, 1, blockSize);

    // 44100 -> 48000: expect approx blockSize * 48000/44100 output samples
    const int expectedApprox = static_cast<int> (blockSize * targetSampleRate / sourceSampleRate);
    EXPECT_GT (produced, 0);
    EXPECT_NEAR (produced, expectedApprox, 2);
}

TEST_F (ResamplerTest, DownsampleProducesFewerOutputThanInput)
{
    std::vector<float> input (blockSize, 0.0f);
    std::vector<float> output (blockSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    int produced = resamplerDown.resample (inPtrs, outPtrs, 1, blockSize);

    // 48000 -> 44100: expect fewer output samples than input
    const int expectedApprox = static_cast<int> (blockSize * sourceSampleRate / targetSampleRate);
    EXPECT_GT (produced, 0);
    EXPECT_NEAR (produced, expectedApprox, 2);
}

TEST_F (ResamplerTest, ResampleDCPreservesMagnitude)
{
    constexpr float dcValue = 0.5f;
    std::vector<float> input (blockSize);
    fillDC (input, dcValue);

    std::vector<float> output (maxOutputSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    // Warm up to flush transient
    for (int b = 0; b < 10; ++b)
        resamplerUp.resample (inPtrs, outPtrs, 1, blockSize);

    int produced = resamplerUp.resample (inPtrs, outPtrs, 1, blockSize);
    ASSERT_GT (produced, 0);

    float rms = calculateRMS (output.data(), produced);
    EXPECT_NEAR (rms, dcValue, 0.05f);
}

TEST_F (ResamplerTest, ResampleSinePreservesFrequencyContent)
{
    constexpr double frequency = 440.0; // A4
    std::vector<float> input (blockSize);
    fillSine (input, frequency, sourceSampleRate);

    std::vector<float> output (maxOutputSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    // Warm up
    for (int b = 0; b < 10; ++b)
        resamplerUp.resample (inPtrs, outPtrs, 1, blockSize);

    int produced = resamplerUp.resample (inPtrs, outPtrs, 1, blockSize);
    ASSERT_GT (produced, 0);

    float rmsIn = calculateRMS (input.data(), blockSize);
    float rmsOut = calculateRMS (output.data(), produced);

    EXPECT_NEAR (rmsOut, rmsIn, rmsIn * 0.1f); // within 10%
}

TEST_F (ResamplerTest, ResetRestoresCleanState)
{
    std::vector<float> input (blockSize);
    fillSine (input, 440.0, sourceSampleRate);

    std::vector<float> output1 (maxOutputSize, 0.0f);
    std::vector<float> output2 (maxOutputSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs1[] = { output1.data() };
    float* outPtrs2[] = { output2.data() };

    // Run a few blocks, then reset, then run again from beginning
    int produced1 = resamplerUp.resample (inPtrs, outPtrs1, 1, blockSize);

    resamplerUp.reset();

    int produced2 = resamplerUp.resample (inPtrs, outPtrs2, 1, blockSize);

    // After reset, first block output should match the very first run
    EXPECT_EQ (produced1, produced2);
    EXPECT_NEAR (output1[0], output2[0], 1e-4f);
}

TEST_F (ResamplerTest, TwoChannelResamplingProducesConsistentOutput)
{
    std::vector<float> ch0 (blockSize), ch1 (blockSize);
    fillSine (ch0, 440.0, sourceSampleRate, 0.7f);
    fillSine (ch1, 880.0, sourceSampleRate, 0.5f);

    std::vector<float> out0 (maxOutputSize, 0.0f);
    std::vector<float> out1 (maxOutputSize, 0.0f);
    const float* inPtrs[] = { ch0.data(), ch1.data() };
    float* outPtrs[] = { out0.data(), out1.data() };

    // Warm up
    for (int b = 0; b < 5; ++b)
        resamplerUp.resample (inPtrs, outPtrs, 2, blockSize);

    int produced = resamplerUp.resample (inPtrs, outPtrs, 2, blockSize);
    ASSERT_GT (produced, 0);

    float rms0 = calculateRMS (out0.data(), produced);
    float rms1 = calculateRMS (out1.data(), produced);

    // Both channels should have non-trivial output
    EXPECT_GT (rms0, 0.1f);
    EXPECT_GT (rms1, 0.1f);

    // RMS should be roughly proportional to input amplitude
    EXPECT_GT (rms0, rms1);
}

TEST_F (ResamplerTest, IdentityResamplePreservesSignal)
{
    // Same input and output rate should be a near-identity operation
    Resampler<float, 8> identity;
    identity.prepare (44100.0, 44100.0, 1, blockSize);

    std::vector<float> input (blockSize);
    fillSine (input, 440.0, 44100.0);

    std::vector<float> output (blockSize + 4, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    // Warm up
    for (int b = 0; b < 10; ++b)
        identity.resample (inPtrs, outPtrs, 1, blockSize);

    int produced = identity.resample (inPtrs, outPtrs, 1, blockSize);
    ASSERT_GT (produced, 0);

    float rmsIn = calculateRMS (input.data(), blockSize);
    float rmsOut = calculateRMS (output.data(), produced);
    EXPECT_NEAR (rmsOut, rmsIn, rmsIn * 0.05f); // within 5%
}

//==============================================================================
TEST (ResamplerTypeAliasTest, TypeAliasesCompile)
{
    ResamplerFloat a;
    ResamplerDouble b;

    a.prepare (44100.0, 48000.0, 1, 64);
    b.prepare (44100.0, 48000.0, 1, 64);

    SUCCEED();
}

} // namespace yup::test
