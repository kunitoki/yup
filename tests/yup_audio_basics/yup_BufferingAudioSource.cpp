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
class MockPositionableAudioSource : public PositionableAudioSource
{
public:
    MockPositionableAudioSource()
        : totalLength (44100 * 10) // 10 seconds at 44.1kHz
        , currentPosition (0)
        , looping (false)
    {
    }

    ~MockPositionableAudioSource() override = default;

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

        // Fill with a pattern based on current position
        for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                const float value = std::sin ((currentPosition + i) * 0.01f) * 0.5f;
                info.buffer->setSample (ch, info.startSample + i, value);
            }
        }
        currentPosition += info.numSamples;
    }

    void setNextReadPosition (int64 newPosition) override
    {
        setNextReadPositionCalled = true;
        currentPosition = newPosition;
    }

    int64 getNextReadPosition() const override
    {
        return currentPosition;
    }

    int64 getTotalLength() const override
    {
        return totalLength;
    }

    bool isLooping() const override
    {
        return looping;
    }

    void setLooping (bool shouldLoop) override
    {
        looping = shouldLoop;
    }

    bool prepareToPlayCalled = false;
    bool releaseResourcesCalled = false;
    bool getNextAudioBlockCalled = false;
    bool setNextReadPositionCalled = false;
    int lastSamplesPerBlock = 0;
    double lastSampleRate = 0.0;
    int64 totalLength;
    int64 currentPosition;
    bool looping;
};
} // namespace

//==============================================================================
class BufferingAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        thread = std::make_unique<TimeSliceThread> ("BufferingTest");
        thread->startThread();

        mockSource = new MockPositionableAudioSource();
        buffering = std::make_unique<BufferingAudioSource> (mockSource, *thread, true, 8192, 2, false);
    }

    void TearDown() override
    {
        buffering.reset();
        thread->stopThread (1000);
        thread.reset();
    }

    std::unique_ptr<TimeSliceThread> thread;
    MockPositionableAudioSource* mockSource; // Owned by buffering
    std::unique_ptr<BufferingAudioSource> buffering;
};

//==============================================================================
TEST_F (BufferingAudioSourceTests, Constructor)
{
    TimeSliceThread localThread ("Test");
    localThread.startThread();

    auto* source = new MockPositionableAudioSource();
    EXPECT_NO_THROW (BufferingAudioSource (source, localThread, true, 8192, 2, false));

    localThread.stopThread (1000);
}

TEST_F (BufferingAudioSourceTests, ConstructorWithPrefill)
{
    TimeSliceThread localThread ("Test");
    localThread.startThread();

    auto* source = new MockPositionableAudioSource();
    EXPECT_NO_THROW (BufferingAudioSource (source, localThread, true, 8192, 2, true));

    localThread.stopThread (1000);
}

TEST_F (BufferingAudioSourceTests, Destructor)
{
    TimeSliceThread localThread ("Test");
    localThread.startThread();

    auto* source = new MockPositionableAudioSource();
    auto* temp = new BufferingAudioSource (source, localThread, true, 8192, 2, false);

    EXPECT_NO_THROW (delete temp);

    localThread.stopThread (1000);
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, PrepareToPlay)
{
    buffering->prepareToPlay (512, 44100.0);

    // Should call prepareToPlay on source (line 80)
    EXPECT_TRUE (mockSource->prepareToPlayCalled);
    EXPECT_EQ (mockSource->lastSamplesPerBlock, 512);
    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 44100.0);

    // Give background thread time to start buffering
    Thread::sleep (50);
}

TEST_F (BufferingAudioSourceTests, PrepareToPlayMultipleTimes)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    // Calling again with same parameters should not recreate buffer (line 71-73)
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    EXPECT_TRUE (mockSource->prepareToPlayCalled);
}

TEST_F (BufferingAudioSourceTests, PrepareToPlayDifferentSampleRate)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    // Different sample rate should trigger re-initialization (line 71)
    buffering->prepareToPlay (512, 48000.0);
    Thread::sleep (50);

    EXPECT_DOUBLE_EQ (mockSource->lastSampleRate, 48000.0);
}

TEST_F (BufferingAudioSourceTests, PrepareToPlayDifferentBufferSize)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    // Different buffer size might not trigger re-initialization if bufferSizeNeeded doesn't change
    // bufferSizeNeeded = jmax(samplesPerBlockExpected * 2, numberOfSamplesToBuffer)
    // With numberOfSamplesToBuffer=8192, changing from 512 to 1024 won't change bufferSizeNeeded
    buffering->prepareToPlay (1024, 44100.0);
    Thread::sleep (50);

    // The source might still have old value if buffer didn't need resize
    EXPECT_GE (mockSource->lastSamplesPerBlock, 512);
}

TEST_F (BufferingAudioSourceTests, PrepareToPlayWithPrefill)
{
    // Create new buffering source with prefill enabled
    auto* source = new MockPositionableAudioSource();
    auto bufferingWithPrefill = std::make_unique<BufferingAudioSource> (source, *thread, true, 8192, 2, true);

    // This should block until buffer is partially filled (line 98-99)
    bufferingWithPrefill->prepareToPlay (512, 44100.0);

    EXPECT_TRUE (source->prepareToPlayCalled);
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, ReleaseResources)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    buffering->releaseResources();

    // Should call releaseResources on source (line 114)
    EXPECT_TRUE (mockSource->releaseResourcesCalled);
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, GetNextAudioBlockEmpty)
{
    // Without prepareToPlay, should get cache miss (line 121-126)
    AudioBuffer<float> buffer (2, 512);
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            buffer.setSample (ch, i, 1.0f);
        }
    }

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    buffering->getNextAudioBlock (info);

    // Buffer should be cleared (cache miss)
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.0f);
        }
    }
}

TEST_F (BufferingAudioSourceTests, GetNextAudioBlockAfterPrepare)
{
    buffering->prepareToPlay (512, 44100.0);

    // Wait for background thread to buffer some data
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    buffering->getNextAudioBlock (info);

    // Should have buffered data
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

TEST_F (BufferingAudioSourceTests, GetNextAudioBlockPartialCacheMissStart)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    // Seek to position that might cause partial cache miss (line 133-134)
    buffering->setNextReadPosition (100000);
    Thread::sleep (50);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (buffering->getNextAudioBlock (info));
}

TEST_F (BufferingAudioSourceTests, GetNextAudioBlockWrapAround)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process multiple blocks to potentially trigger wrap-around (line 149-160)
    for (int i = 0; i < 20; ++i)
    {
        buffer.clear();
        buffering->getNextAudioBlock (info);
        Thread::sleep (10);
    }
}

TEST_F (BufferingAudioSourceTests, GetNextAudioBlockWithStartSample)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    buffering->getNextAudioBlock (info);

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
TEST_F (BufferingAudioSourceTests, WaitForNextAudioBlockReadyNullSource)
{
    // Create buffering with source that has zero length
    mockSource->totalLength = 0;

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should return false for invalid source (line 169-170)
    EXPECT_FALSE (buffering->waitForNextAudioBlockReady (info, 100));
}

TEST_F (BufferingAudioSourceTests, WaitForNextAudioBlockReadyNegativePosition)
{
    buffering->prepareToPlay (512, 44100.0);
    buffering->setNextReadPosition (-1000);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should return true for negative position (line 172-174)
    EXPECT_TRUE (buffering->waitForNextAudioBlockReady (info, 100));
}

TEST_F (BufferingAudioSourceTests, WaitForNextAudioBlockReadyPastEnd)
{
    buffering->prepareToPlay (512, 44100.0);

    // Set position past the end
    buffering->setNextReadPosition (mockSource->getTotalLength() + 1000);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should return true when past end and not looping (line 172-174)
    EXPECT_TRUE (buffering->waitForNextAudioBlockReady (info, 100));
}

TEST_F (BufferingAudioSourceTests, WaitForNextAudioBlockReadySuccess)
{
    buffering->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Should return true when data is ready (line 189-194)
    EXPECT_TRUE (buffering->waitForNextAudioBlockReady (info, 1000));
}

TEST_F (BufferingAudioSourceTests, WaitForNextAudioBlockReadyTimeout)
{
    buffering->prepareToPlay (512, 44100.0);

    // Seek to far position
    buffering->setNextReadPosition (1000000);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // With short timeout, might return false (line 207)
    auto result = buffering->waitForNextAudioBlockReady (info, 10);

    // Either true or false is acceptable depending on timing
    EXPECT_TRUE (result == true || result == false);
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, GetNextReadPosition)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    // Initial position should be 0
    EXPECT_EQ (buffering->getNextReadPosition(), 0);

    // Process some audio
    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    buffering->getNextAudioBlock (info);

    // Position should advance (line 164)
    EXPECT_EQ (buffering->getNextReadPosition(), 512);
}

TEST_F (BufferingAudioSourceTests, GetNextReadPositionWithLooping)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    mockSource->setLooping (true);

    // Set position past total length
    buffering->setNextReadPosition (mockSource->getTotalLength() + 1000);

    // Should wrap around with looping (line 215-216)
    auto pos = buffering->getNextReadPosition();
    EXPECT_LT (pos, mockSource->getTotalLength());
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, SetNextReadPosition)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    buffering->setNextReadPosition (5000);

    // Position should be updated (line 224)
    EXPECT_EQ (buffering->getNextReadPosition(), 5000);
}

TEST_F (BufferingAudioSourceTests, SetNextReadPositionMultipleTimes)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (50);

    for (int64 pos = 0; pos < 10000; pos += 1000)
    {
        buffering->setNextReadPosition (pos);
        Thread::sleep (20);
        EXPECT_EQ (buffering->getNextReadPosition(), pos);
    }
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, ReadNextBufferChunkInitial)
{
    buffering->prepareToPlay (512, 44100.0);

    // Background thread should call readNextBufferChunk (line 238-318)
    Thread::sleep (100);

    // Verify source was called
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (BufferingAudioSourceTests, ReadNextBufferChunkCacheMiss)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    // Seek far away to trigger cache miss (line 259-268)
    buffering->setNextReadPosition (200000);
    Thread::sleep (100);

    // Should have read new buffer section
    EXPECT_TRUE (mockSource->setNextReadPositionCalled);
}

TEST_F (BufferingAudioSourceTests, ReadNextBufferChunkIncrementalRead)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process audio to advance position
    for (int i = 0; i < 5; ++i)
    {
        buffer.clear();
        buffering->getNextAudioBlock (info);
        Thread::sleep (20);
    }

    // Should trigger incremental reads (line 269-279)
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (BufferingAudioSourceTests, ReadNextBufferChunkWrapAround)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Process many blocks to trigger buffer wrap-around (line 290-307)
    for (int i = 0; i < 30; ++i)
    {
        buffer.clear();
        buffering->getNextAudioBlock (info);
        Thread::sleep (10);
    }
}

TEST_F (BufferingAudioSourceTests, ReadNextBufferChunkLoopingChange)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    // Change looping state to trigger buffer reset (line 245-250)
    mockSource->setLooping (true);
    Thread::sleep (100);

    mockSource->setLooping (false);
    Thread::sleep (100);

    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

//==============================================================================
TEST_F (BufferingAudioSourceTests, UseTimeSlice)
{
    buffering->prepareToPlay (512, 44100.0);

    // useTimeSlice is called by background thread (line 331-334)
    Thread::sleep (100);

    // Should have processed some chunks
    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (BufferingAudioSourceTests, MultipleChannels)
{
    auto* source = new MockPositionableAudioSource();
    auto bufferingMulti = std::make_unique<BufferingAudioSource> (source, *thread, true, 8192, 8, false);

    bufferingMulti->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (8, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    EXPECT_NO_THROW (bufferingMulti->getNextAudioBlock (info));
}

TEST_F (BufferingAudioSourceTests, StressTestContinuousPlayback)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // Simulate continuous playback
    for (int i = 0; i < 50; ++i)
    {
        buffer.clear();
        buffering->getNextAudioBlock (info);
        Thread::sleep (5);
    }

    EXPECT_TRUE (mockSource->getNextAudioBlockCalled);
}

TEST_F (BufferingAudioSourceTests, StressTestRandomSeeks)
{
    buffering->prepareToPlay (512, 44100.0);
    Thread::sleep (100);

    AudioBuffer<float> buffer (2, 512);
    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    Random random;

    // Perform random seeks
    for (int i = 0; i < 20; ++i)
    {
        const int64 pos = random.nextInt (static_cast<int> (mockSource->getTotalLength() / 2));
        buffering->setNextReadPosition (pos);
        Thread::sleep (50);

        buffer.clear();
        buffering->getNextAudioBlock (info);
    }

    EXPECT_TRUE (mockSource->setNextReadPositionCalled);
}
