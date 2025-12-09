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
class MixerAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mixer = std::make_unique<MixerAudioSource>();
    }

    void TearDown() override
    {
        mixer.reset();
    }

    std::unique_ptr<MixerAudioSource> mixer;
};

//==============================================================================
TEST_F (MixerAudioSourceTests, Constructor)
{
    EXPECT_NO_THROW (MixerAudioSource());
}

TEST_F (MixerAudioSourceTests, Destructor)
{
    auto* temp = new MixerAudioSource();
    auto* source = new MockAudioSource();
    temp->addInputSource (source, true);

    // Destructor should call removeAllInputs which releases resources
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (MixerAudioSourceTests, AddInputSourceWithNull)
{
    // Should not crash with null (line 57)
    EXPECT_NO_THROW (mixer->addInputSource (nullptr, false));
}

TEST_F (MixerAudioSourceTests, AddInputSourceWithoutDelete)
{
    MockAudioSource source;
    mixer->addInputSource (&source, false);

    // Source should not be prepared if mixer hasn't been prepared yet (line 68)
    EXPECT_FALSE (source.prepareToPlayCalled);
}

TEST_F (MixerAudioSourceTests, AddInputSourceAfterPrepare)
{
    mixer->prepareToPlay (512, 44100.0);

    MockAudioSource source;
    mixer->addInputSource (&source, false);

    // Source should be prepared if mixer was already prepared (line 68-69)
    EXPECT_TRUE (source.prepareToPlayCalled);
    EXPECT_EQ (source.lastSamplesPerBlock, 512);
    EXPECT_DOUBLE_EQ (source.lastSampleRate, 44100.0);
}

TEST_F (MixerAudioSourceTests, AddInputSourceWithDelete)
{
    auto* source = new MockAudioSource();
    mixer->addInputSource (source, true);

    // Cleanup will happen in mixer destructor or removeInputSource
}

TEST_F (MixerAudioSourceTests, AddDuplicateInput)
{
    MockAudioSource source;
    mixer->addInputSource (&source, false);

    // Adding same source again should be ignored (line 57)
    mixer->addInputSource (&source, false);
}

//==============================================================================
TEST_F (MixerAudioSourceTests, RemoveInputSourceWithNull)
{
    // Should not crash with null (line 80)
    EXPECT_NO_THROW (mixer->removeInputSource (nullptr));
}

TEST_F (MixerAudioSourceTests, RemoveNonExistentInput)
{
    MockAudioSource source;
    // Should return early if input not found (line 88-89)
    EXPECT_NO_THROW (mixer->removeInputSource (&source));
    EXPECT_FALSE (source.releaseResourcesCalled);
}

TEST_F (MixerAudioSourceTests, RemoveInputSourceWithoutDelete)
{
    MockAudioSource source;
    mixer->addInputSource (&source, false);
    mixer->removeInputSource (&source);

    // Should call releaseResources (line 98)
    EXPECT_TRUE (source.releaseResourcesCalled);
}

TEST_F (MixerAudioSourceTests, RemoveInputSourceWithDelete)
{
    auto* source = new MockAudioSource();
    mixer->addInputSource (source, true);

    // Should delete the source (line 91-92)
    mixer->removeInputSource (source);

    // Source is deleted, we can't check it anymore but no crash means success
}

//==============================================================================
TEST_F (MixerAudioSourceTests, RemoveAllInputsEmpty)
{
    EXPECT_NO_THROW (mixer->removeAllInputs());
}

TEST_F (MixerAudioSourceTests, RemoveAllInputsWithoutDelete)
{
    MockAudioSource source1;
    MockAudioSource source2;

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (&source2, false);

    mixer->removeAllInputs();

    // removeAllInputs only calls releaseResources on inputs marked for deletion (line 109-117)
    // Inputs without delete flag don't get releaseResources called
    EXPECT_FALSE (source1.releaseResourcesCalled);
    EXPECT_FALSE (source2.releaseResourcesCalled);
}

TEST_F (MixerAudioSourceTests, RemoveAllInputsWithDelete)
{
    auto* source1 = new MockAudioSource();
    auto* source2 = new MockAudioSource();

    mixer->addInputSource (source1, true);
    mixer->addInputSource (source2, true);

    // Should delete and release all sources (line 109-111, 116-117)
    EXPECT_NO_THROW (mixer->removeAllInputs());
}

TEST_F (MixerAudioSourceTests, RemoveAllInputsMixed)
{
    MockAudioSource source1;
    auto* source2 = new MockAudioSource();

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (source2, true);

    mixer->removeAllInputs();

    // Only inputs marked for deletion get releaseResources called
    EXPECT_FALSE (source1.releaseResourcesCalled);
}

//==============================================================================
TEST_F (MixerAudioSourceTests, PrepareToPlay)
{
    MockAudioSource source1;
    MockAudioSource source2;

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (&source2, false);

    mixer->prepareToPlay (1024, 48000.0);

    // Should prepare all inputs (line 129-130)
    EXPECT_TRUE (source1.prepareToPlayCalled);
    EXPECT_TRUE (source2.prepareToPlayCalled);
    EXPECT_EQ (source1.lastSamplesPerBlock, 1024);
    EXPECT_DOUBLE_EQ (source1.lastSampleRate, 48000.0);
}

//==============================================================================
TEST_F (MixerAudioSourceTests, ReleaseResources)
{
    MockAudioSource source1;
    MockAudioSource source2;

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (&source2, false);

    mixer->prepareToPlay (512, 44100.0);
    mixer->releaseResources();

    // Should release all inputs (line 137-138)
    EXPECT_TRUE (source1.releaseResourcesCalled);
    EXPECT_TRUE (source2.releaseResourcesCalled);
}

//==============================================================================
TEST_F (MixerAudioSourceTests, GetNextAudioBlockWithNoInputs)
{
    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    // Fill with non-zero to test clearing
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

    mixer->getNextAudioBlock (info);

    // Should clear the buffer when no inputs (line 172)
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.0f);
        }
    }
}

TEST_F (MixerAudioSourceTests, GetNextAudioBlockWithSingleInput)
{
    MockAudioSource source;
    source.fillValue = 0.3f;

    mixer->addInputSource (&source, false);
    mixer->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    mixer->getNextAudioBlock (info);

    // Should just call first input directly (line 152)
    EXPECT_TRUE (source.getNextAudioBlockCalled);

    // Buffer should contain the source's value
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.3f);
        }
    }
}

TEST_F (MixerAudioSourceTests, GetNextAudioBlockWithMultipleInputs)
{
    MockAudioSource source1;
    MockAudioSource source2;
    MockAudioSource source3;

    source1.fillValue = 0.2f;
    source2.fillValue = 0.3f;
    source3.fillValue = 0.1f;

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (&source2, false);
    mixer->addInputSource (&source3, false);

    mixer->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    mixer->getNextAudioBlock (info);

    // Should mix all inputs (line 154-168)
    EXPECT_TRUE (source1.getNextAudioBlockCalled);
    EXPECT_TRUE (source2.getNextAudioBlockCalled);
    EXPECT_TRUE (source3.getNextAudioBlockCalled);

    // Buffer should contain sum of all sources
    const float expectedSum = 0.2f + 0.3f + 0.1f;
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 512; ++i)
        {
            EXPECT_NEAR (buffer.getSample (ch, i), expectedSum, 0.0001f);
        }
    }
}

TEST_F (MixerAudioSourceTests, GetNextAudioBlockWithStartSampleOffset)
{
    MockAudioSource source;
    source.fillValue = 0.5f;

    mixer->addInputSource (&source, false);
    mixer->prepareToPlay (512, 44100.0);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    mixer->getNextAudioBlock (info);

    // Check that samples before startSample are still zero
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 100; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.0f);
        }
    }

    // Check that samples at startSample have the expected value
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 100; i < 356; ++i)
        {
            EXPECT_FLOAT_EQ (buffer.getSample (ch, i), 0.5f);
        }
    }
}

TEST_F (MixerAudioSourceTests, GetNextAudioBlockResizesTempBuffer)
{
    MockAudioSource source1;
    MockAudioSource source2;

    source1.fillValue = 0.3f;
    source2.fillValue = 0.4f;

    mixer->addInputSource (&source1, false);
    mixer->addInputSource (&source2, false);

    mixer->prepareToPlay (512, 44100.0);

    // Test with different buffer sizes to trigger temp buffer resize (line 156-157)
    AudioBuffer<float> buffer1 (4, 256);
    buffer1.clear();

    AudioSourceChannelInfo info1;
    info1.buffer = &buffer1;
    info1.startSample = 0;
    info1.numSamples = 256;

    mixer->getNextAudioBlock (info1);

    // Now test with larger size
    AudioBuffer<float> buffer2 (4, 1024);
    buffer2.clear();

    AudioSourceChannelInfo info2;
    info2.buffer = &buffer2;
    info2.startSample = 0;
    info2.numSamples = 1024;

    EXPECT_NO_THROW (mixer->getNextAudioBlock (info2));
}
