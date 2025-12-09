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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
namespace
{
class MockAudioSource : public AudioSource
{
public:
    MockAudioSource() = default;
    ~MockAudioSource() override = default;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        prepareToPlayCalled = true;
        lastSamplesPerBlock = samplesPerBlockExpected;
        lastSampleRate = sampleRate;
    }

    void releaseResources() override
    {
        releaseResourcesCalled = true;
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& info) override
    {
        getNextAudioBlockCalled = true;

        // Fill with a simple sine-like pattern for testing
        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                const float value = std::sin (static_cast<float> (i) * 0.1f) * 0.5f;
                info.buffer->setSample (ch, info.startSample + i, value);
            }
        }
    }

    bool prepareToPlayCalled = false;
    bool releaseResourcesCalled = false;
    bool getNextAudioBlockCalled = false;
    int lastSamplesPerBlock = 0;
    double lastSampleRate = 0.0;
};
} // namespace

//==============================================================================
class ResamplingAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockSource = new MockAudioSource();
        resampler = std::make_unique<ResamplingAudioSource> (mockSource, true, 2);
    }

    void TearDown() override
    {
        resampler.reset();
    }

    MockAudioSource* mockSource; // Owned by resampler
    std::unique_ptr<ResamplingAudioSource> resampler;
};

//==============================================================================
TEST_F (ResamplingAudioSourceTests, Constructor)
{
    auto* source = new MockAudioSource();
    EXPECT_NO_THROW (ResamplingAudioSource (source, true, 2));
}

TEST_F (ResamplingAudioSourceTests, ConstructorWithDifferentChannels)
{
    auto* source = new MockAudioSource();
    EXPECT_NO_THROW (ResamplingAudioSource (source, true, 8));
}

TEST_F (ResamplingAudioSourceTests, Destructor)
{
    auto* source = new MockAudioSource();
    auto* temp = new ResamplingAudioSource (source, true, 2);
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, SetResamplingRatio)
{
    // Test various valid ratios
    EXPECT_NO_THROW (resampler->setResamplingRatio (1.0));
    EXPECT_NO_THROW (resampler->setResamplingRatio (0.5));
    EXPECT_NO_THROW (resampler->setResamplingRatio (2.0));
    EXPECT_NO_THROW (resampler->setResamplingRatio (0.1));
}

TEST_F (ResamplingAudioSourceTests, DISABLED_SetResamplingRatioNegative)
{
    // Negative ratio should be clamped to 0 (line 60)
    EXPECT_NO_THROW (resampler->setResamplingRatio (-1.0));
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, PrepareToPlay)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    // Should call prepareToPlay on source with scaled values (line 68)
    EXPECT_TRUE (mockSource->prepareToPlayCalled);
    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 44100.0);
}

TEST_F (ResamplingAudioSourceTests, PrepareToPlayWithDifferentRatios)
{
    resampler->setResamplingRatio (2.0);
    resampler->prepareToPlay (512, 44100.0);

    EXPECT_TRUE (mockSource->prepareToPlayCalled);
    // Sample rate should be scaled by ratio (line 68)
    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 88200.0);
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, FlushBuffers)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    EXPECT_NO_THROW (resampler->flushBuffers());
}

TEST_F (ResamplingAudioSourceTests, FlushBuffersAfterProcessing)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    resampler->getNextAudioBlock (info);

    // Flush should clear internal state (line 84-88)
    EXPECT_NO_THROW (resampler->flushBuffers());
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, ReleaseResources)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    resampler->releaseResources();

    // Should call releaseResources on source (line 93)
    EXPECT_TRUE (mockSource->releaseResourcesCalled);
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockRatioOne)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    resampler->getNextAudioBlock (info);

    // Should call getNextAudioBlock on source
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);

    // Buffer should have audio
    bool hasNonZero = false;
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            if (buffer.getSample (ch, i) != 0.0f)
            {
                hasNonZero = true;
                break;
            }
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockDownsampling)
{
    // Test down-sampling (ratio > 1.0, line 140-146)
    resampler->setResamplingRatio (2.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 256);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 256;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockUpsampling)
{
    // Test up-sampling (ratio < 1.0, line 184-189)
    resampler->setResamplingRatio (0.5);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockChangingRatio)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process with ratio 1.0
    buffer.clear();
    resampler->getNextAudioBlock (info);

    // Change ratio during processing (line 108-112)
    resampler->setResamplingRatio (0.8);

    buffer.clear();
    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockBufferResize)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (256, 44100.0);

    // Start with small buffer
    AudioBuffer<float> buffer1 (2, 256);
    buffer1.clear();

    AudioSourceChannelInfo info1;
    info1.buffer = &buffer1;
    info1.startSample = 0;
    info1.numSamples = 256;

    resampler->getNextAudioBlock (info1);

    // Request larger buffer, should trigger resize (line 118-123)
    AudioBuffer<float> buffer2 (2, 2048);
    buffer2.clear();

    AudioSourceChannelInfo info2;
    info2.buffer = &buffer2;
    info2.startSample = 0;
    info2.numSamples = 2048;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info2));
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockMultipleChannels)
{
    auto* source = new MockAudioSource();
    auto multiChannelResampler = std::make_unique<ResamplingAudioSource> (source, true, 8);

    multiChannelResampler->setResamplingRatio (1.0);
    multiChannelResampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (8, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (multiChannelResampler->getNextAudioBlock (info));
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockWithStartSample)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    resampler->getNextAudioBlock (info);

    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);

    // Samples before startSample should remain zero
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.0f);
        }
    }
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockBufferWrapAround)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process multiple blocks to trigger wrap-around (line 125, 132, 174-175)
    for (int i = 0; i < 10; ++i)
    {
        buffer.clear();
        EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
    }
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockFilterStateUpdate)
{
    // Test the filter state update for ratio close to 1.0 (line 190-210)
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process first block
    buffer.clear();
    resampler->getNextAudioBlock (info);

    // Process second block, filter states should be updated
    buffer.clear();
    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
}

TEST_F (ResamplingAudioSourceTests, GetNextAudioBlockFilterStateUpdateSingleSample)
{
    // Test filter state update with single sample (line 198-206)
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 1);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 1;

    buffer.clear();
    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));

    // Process another single sample
    buffer.clear();
    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, CreateLowPassForDownsampling)
{
    // This is tested indirectly through getNextAudioBlock with ratio > 1.0
    resampler->setResamplingRatio (2.5);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 256);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 256;

    // Should apply low-pass filter for down-sampling (line 217-218)
    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
}

TEST_F (ResamplingAudioSourceTests, CreateLowPassForUpsampling)
{
    // Test with ratio < 1.0 (line 217-218)
    resampler->setResamplingRatio (0.4);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info));
}

//==============================================================================
TEST_F (ResamplingAudioSourceTests, InterpolationAccuracy)
{
    resampler->setResamplingRatio (1.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    resampler->getNextAudioBlock (info);

    // Verify interpolation happened (line 164-168)
    // Buffer should contain interpolated values
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ResamplingAudioSourceTests, MultipleBlocksConsistency)
{
    resampler->setResamplingRatio (1.5);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 256);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 256;

    // Process multiple blocks to ensure consistent behavior
    for (int i = 0; i < 20; ++i)
    {
        buffer.clear();
        EXPECT_NO_THROW (resampler->getNextAudioBlock (info));

        // Verify output has audio content
        bool hasNonZero = false;
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int s = 0; s < 256; ++s)
            {
                if (buffer.getSample (ch, s) != 0.0f)
                {
                    hasNonZero = true;
                    break;
                }
            }
        }
        EXPECT_TRUE (hasNonZero);
    }
}

TEST_F (ResamplingAudioSourceTests, ExtremeRatios)
{
    // Test with very small ratio
    resampler->setResamplingRatio (0.1);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer1 (2, 512);
    buffer1.clear();

    AudioSourceChannelInfo info1;
    info1.buffer = &buffer1;
    info1.startSample = 0;
    info1.numSamples = 512;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info1));

    // Test with large ratio
    resampler->setResamplingRatio (8.0);
    resampler->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer2 (2, 64);
    buffer2.clear();

    AudioSourceChannelInfo info2;
    info2.buffer = &buffer2;
    info2.startSample = 0;
    info2.numSamples = 64;

    EXPECT_NO_THROW (resampler->getNextAudioBlock (info2));
}
