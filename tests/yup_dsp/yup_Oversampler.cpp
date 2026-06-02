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
class OversamplerTest : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 44100.0;
    static constexpr int maxChannels = 2;
    static constexpr int blockSize = 256;

    void SetUp() override
    {
        os2x.prepare (sampleRate, maxChannels, blockSize);
        os4x.prepare (sampleRate, maxChannels, blockSize);
    }

    float calculateRMS (const float* data, int numSamples) const
    {
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sum += data[i] * data[i];
        return std::sqrt (sum / static_cast<float> (numSamples));
    }

    void fillSine (std::vector<float>& buf, float frequency, float amplitude = 1.0f) const
    {
        for (std::size_t i = 0; i < buf.size(); ++i)
        {
            buf[i] = amplitude * std::sin (MathConstants<float>::twoPi * frequency * static_cast<float> (i) / static_cast<float> (sampleRate));
        }
    }

    void fillDC (std::vector<float>& buf, float value) const
    {
        std::fill (buf.begin(), buf.end(), value);
    }

    Oversampler<float, 2, 8> os2x;
    Oversampler<float, 4, 8> os4x;
};

//==============================================================================
TEST_F (OversamplerTest, DefaultConstructionDoesNotCrash)
{
    Oversampler<float, 2, 8> os;
    EXPECT_EQ (os.getOversampledNumSamples(), 0);
    EXPECT_EQ (os.getLatencyInSamples(), 16);
    EXPECT_EQ (os.getOversampledChannelData (0), nullptr);
}

TEST_F (OversamplerTest, PrepareAllocatesOversampledBuffer)
{
    EXPECT_EQ (os2x.getOversampledNumSamples(), 0);

    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    EXPECT_EQ (os2x.getOversampledNumSamples(), blockSize * 2);
    EXPECT_NE (os2x.getOversampledChannelData (0), nullptr);
}

TEST_F (OversamplerTest, LatencyReturnsCorrectValue)
{
    EXPECT_EQ (os2x.getLatencyInSamples(), 16); // 2 * SincRadius = 2 * 8
    EXPECT_EQ (os4x.getLatencyInSamples(), 16);
}

TEST_F (OversamplerTest, ResetClearsOversampledSize)
{
    std::vector<float> ch0 (blockSize, 1.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    ASSERT_EQ (os2x.getOversampledNumSamples(), blockSize * 2);

    os2x.reset();
    EXPECT_EQ (os2x.getOversampledNumSamples(), 0);
}

TEST_F (OversamplerTest, UpsampleDCSignalHasCorrectMagnitude)
{
    constexpr float dcValue = 0.5f;
    std::vector<float> ch0 (blockSize, dcValue);
    const float* inputPtrs[] = { ch0.data() };

    // Warm up the filter (several blocks to flush transient)
    for (int b = 0; b < 5; ++b)
        os2x.upsample (inputPtrs, 1, blockSize);

    const float* outData = os2x.getOversampledChannelData (0);
    ASSERT_NE (outData, nullptr);

    // Second half of the block should be at steady state
    const int halfSize = blockSize; // = blockSize * 2 / 2
    float rms = calculateRMS (outData + halfSize, halfSize);
    EXPECT_NEAR (rms, dcValue, 0.05f);
}

TEST_F (OversamplerTest, ProcessOversampledBlockCallbackReceivesCorrectSize)
{
    std::vector<float> ch0 (blockSize, 0.0f), ch1 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data(), ch1.data() };
    os2x.upsample (inputPtrs, 2, blockSize);

    int callbackChannels = 0;
    int callbackSamples = 0;
    os2x.processOversampledBlock ([&] (auto& buf)
    {
        callbackChannels = buf.getNumChannels();
        callbackSamples = buf.getNumSamples();
    });

    EXPECT_EQ (callbackChannels, maxChannels);
    EXPECT_EQ (callbackSamples, blockSize * 2);
}

TEST_F (OversamplerTest, UpsampleThenDownsamplePreservesLowFrequencySine)
{
    constexpr float frequency = 440.0f; // A4 - well below Nyquist/4
    std::vector<float> input (blockSize);
    fillSine (input, frequency);

    std::vector<float> output (blockSize, 0.0f);
    const float* inPtrs[] = { input.data() };
    float* outPtrs[] = { output.data() };

    // Warm up to flush latency
    for (int b = 0; b < 10; ++b)
    {
        os2x.upsample (inPtrs, 1, blockSize);
        os2x.downsample (outPtrs, 1, blockSize);
    }

    // Compare RMS energy - should be preserved
    float rmsIn = calculateRMS (input.data(), blockSize);
    float rmsOut = calculateRMS (output.data(), blockSize);
    EXPECT_NEAR (rmsOut, rmsIn, rmsIn * 0.1f); // within 10%
}

TEST_F (OversamplerTest, DecimationFiltersOversampledDomainHighFrequency)
{
    // Upsample silence to get a clean oversampled buffer
    std::vector<float> silence (blockSize, 0.0f);
    const float* silPtrs[] = { silence.data() };

    for (int b = 0; b < 5; ++b)
        os2x.upsample (silPtrs, 1, blockSize);

    // Inject a tone ABOVE original Nyquist directly into the oversampled buffer.
    // This simulates distortion harmonics created at the elevated sample rate.
    // The decimation filter should attenuate this before downsampling.
    const double oversampledRate = sampleRate * 2.0;
    const double highFreq = sampleRate * 0.6; // above original Nyquist (22050 Hz)
    float* oversampledData = os2x.getOversampledChannelData (0);
    const int oversampledLen = os2x.getOversampledNumSamples();
    constexpr float injectedAmplitude = 0.5f;

    for (int i = 0; i < oversampledLen; ++i)
        oversampledData[i] += injectedAmplitude * static_cast<float> (std::sin (MathConstants<double>::twoPi * highFreq * static_cast<double> (i) / oversampledRate));

    std::vector<float> output (blockSize, 0.0f);
    float* outPtrs[] = { output.data() };
    os2x.downsample (outPtrs, 1, blockSize);

    float rmsOut = calculateRMS (output.data(), blockSize);

    // The anti-aliasing filter should substantially reduce the injected tone
    EXPECT_LT (rmsOut, injectedAmplitude * 0.5f);
}

TEST_F (OversamplerTest, OversampledChannelDataNotNullAfterUpsample)
{
    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os2x.upsample (inputPtrs, 1, blockSize);

    EXPECT_NE (os2x.getOversampledChannelData (0), nullptr);
    EXPECT_EQ (os2x.getOversampledChannelData (1), nullptr);  // channel 1 not prepared
    EXPECT_EQ (os2x.getOversampledChannelData (-1), nullptr); // invalid index
}

TEST_F (OversamplerTest, FourXOversamplerHasCorrectOutputSize)
{
    std::vector<float> ch0 (blockSize, 0.0f);
    const float* inputPtrs[] = { ch0.data() };
    os4x.upsample (inputPtrs, 1, blockSize);

    EXPECT_EQ (os4x.getOversampledNumSamples(), blockSize * 4);
}

//==============================================================================
TEST (OversamplerTypeAliasTest, TypeAliasesCompile)
{
    Oversampler2xFloat a;
    Oversampler4xFloat b;
    Oversampler8xFloat c;
    Oversampler2xDouble d;
    Oversampler4xDouble e;

    // Prepare briefly to confirm the types are usable
    a.prepare (44100.0, 1, 64);
    b.prepare (44100.0, 1, 64);
    c.prepare (44100.0, 1, 64);
    d.prepare (44100.0, 1, 64);
    e.prepare (44100.0, 1, 64);

    SUCCEED();
}

} // namespace yup::test
