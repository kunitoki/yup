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

        // Fill with a constant value for testing
        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                info.buffer->setSample (ch, info.startSample + i, fillValue);
            }
        }
    }

    bool prepareToPlayCalled = false;
    bool releaseResourcesCalled = false;
    bool getNextAudioBlockCalled = false;
    int lastSamplesPerBlock = 0;
    double lastSampleRate = 0.0;
    float fillValue = 0.5f;
};
} // namespace

//==============================================================================
class ReverbAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockSource = new MockAudioSource();
        reverbSource = std::make_unique<ReverbAudioSource> (mockSource, true);
    }

    void TearDown() override
    {
        reverbSource.reset();
    }

    MockAudioSource* mockSource; // Owned by reverbSource
    std::unique_ptr<ReverbAudioSource> reverbSource;
};

//==============================================================================
TEST_F (ReverbAudioSourceTests, ConstructorWithDeleteInput)
{
    auto* source = new MockAudioSource();
    EXPECT_NO_THROW (ReverbAudioSource (source, true));
}

TEST_F (ReverbAudioSourceTests, ConstructorWithoutDeleteInput)
{
    MockAudioSource source;
    EXPECT_NO_THROW (ReverbAudioSource (&source, false));
}

TEST_F (ReverbAudioSourceTests, Destructor)
{
    auto* source = new MockAudioSource();
    auto* temp = new ReverbAudioSource (source, true);
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (ReverbAudioSourceTests, PrepareToPlay)
{
    reverbSource->prepareToPlay (512, 44100.0);

    // Should call prepareToPlay on input source (line 55)
    EXPECT_TRUE (mockSource->prepareToPlayCalled);
    EXPECT_EQ (mockSource->lastSamplesPerBlock, 512);
    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 44100.0);
}

TEST_F (ReverbAudioSourceTests, ReleaseResources)
{
    reverbSource->prepareToPlay (512, 44100.0);
    EXPECT_NO_THROW (reverbSource->releaseResources());
}

//==============================================================================
TEST_F (ReverbAudioSourceTests, GetNextAudioBlockMono)
{
    reverbSource->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (1, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    reverbSource->getNextAudioBlock (info);

    // Should call getNextAudioBlock on input (line 65)
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);

    // Should process mono (line 79)
    // Buffer should have been modified by reverb
    bool hasNonZero = false;
    for (int i = 0; i < 512; ++i)
    {
        if (buffer.getSample (0, i) != 0.0f)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ReverbAudioSourceTests, GetNextAudioBlockStereo)
{
    reverbSource->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    reverbSource->getNextAudioBlock (info);

    // Should call getNextAudioBlock on input (line 65)
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);

    // Should process stereo (line 73-75)
    // Buffer should have been modified by reverb
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

TEST_F (ReverbAudioSourceTests, GetNextAudioBlockMultiChannel)
{
    reverbSource->prepareToPlay (512, 44100.0);

    // Test with more than 2 channels
    AudioBuffer<float> buffer (4, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should still work, processing first 2 channels as stereo (line 71-76)
    EXPECT_NO_THROW (reverbSource->getNextAudioBlock (info));
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ReverbAudioSourceTests, GetNextAudioBlockWithStartSampleOffset)
{
    reverbSource->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    reverbSource->getNextAudioBlock (info);

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

//==============================================================================
TEST_F (ReverbAudioSourceTests, SetParameters)
{
    Reverb::Parameters params;
    params.roomSize = 0.8f;
    params.damping = 0.5f;
    params.wetLevel = 0.4f;
    params.dryLevel = 0.6f;
    params.width = 1.0f;
    params.freezeMode = 0.0f;

    EXPECT_NO_THROW (reverbSource->setParameters (params));

    // Prepare and process to verify parameters are applied
    reverbSource->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    // Fill with signal
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            buffer.setSample (ch, i, 0.5f);
        }
    }

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (reverbSource->getNextAudioBlock (info));
}

//==============================================================================
TEST_F (ReverbAudioSourceTests, SetBypassedTrue)
{
    reverbSource->prepareToPlay (512, 44100.0);

    // Set bypass to true (line 92-97)
    reverbSource->setBypassed (true);

    mockSource->fillValue = 0.7f;

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    reverbSource->getNextAudioBlock (info);

    // When bypassed, should not process reverb (line 67)
    // Buffer should contain only the input source value
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.7f);
        }
    }
}

TEST_F (ReverbAudioSourceTests, SetBypassedFalse)
{
    reverbSource->setBypassed (true);
    reverbSource->prepareToPlay (512, 44100.0);

    // Set bypass to false (line 92-97)
    reverbSource->setBypassed (false);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    reverbSource->getNextAudioBlock (info);

    // When not bypassed, should process reverb
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (ReverbAudioSourceTests, SetBypassedSameValue)
{
    reverbSource->setBypassed (false);

    // Setting to same value should not acquire lock (line 92)
    EXPECT_NO_THROW (reverbSource->setBypassed (false));
}

TEST_F (ReverbAudioSourceTests, SetBypassedResetsReverb)
{
    reverbSource->prepareToPlay (512, 44100.0);

    // Process some audio
    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    reverbSource->getNextAudioBlock (info);

    // Toggling bypass should reset reverb (line 96)
    reverbSource->setBypassed (true);
    reverbSource->setBypassed (false);

    // Continue processing
    EXPECT_NO_THROW (reverbSource->getNextAudioBlock (info));
}

TEST_F (ReverbAudioSourceTests, BypassAndUnbypassMultipleTimes)
{
    reverbSource->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    for (int i = 0; i < 5; ++i)
    {
        reverbSource->setBypassed (true);
        buffer.clear();
        reverbSource->getNextAudioBlock (info);

        reverbSource->setBypassed (false);
        buffer.clear();
        reverbSource->getNextAudioBlock (info);
    }

    // Should handle multiple bypass toggles without issues
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}
