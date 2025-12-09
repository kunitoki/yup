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
class ToneGeneratorAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        source = std::make_unique<ToneGeneratorAudioSource>();
    }

    void TearDown() override
    {
        source.reset();
    }

    std::unique_ptr<ToneGeneratorAudioSource> source;
};

//==============================================================================
TEST_F (ToneGeneratorAudioSourceTests, Constructor)
{
    EXPECT_NO_THROW (ToneGeneratorAudioSource());
}

TEST_F (ToneGeneratorAudioSourceTests, Destructor)
{
    auto* temp = new ToneGeneratorAudioSource();
    EXPECT_NO_THROW (delete temp);
}

//==============================================================================
TEST_F (ToneGeneratorAudioSourceTests, SetAmplitude)
{
    EXPECT_NO_THROW (source->setAmplitude (0.5f));
    EXPECT_NO_THROW (source->setAmplitude (0.0f));
    EXPECT_NO_THROW (source->setAmplitude (1.0f));
    EXPECT_NO_THROW (source->setAmplitude (2.0f));
}

TEST_F (ToneGeneratorAudioSourceTests, SetFrequency)
{
    EXPECT_NO_THROW (source->setFrequency (440.0));
    EXPECT_NO_THROW (source->setFrequency (1000.0));
    EXPECT_NO_THROW (source->setFrequency (20.0));
    EXPECT_NO_THROW (source->setFrequency (20000.0));
}

//==============================================================================
TEST_F (ToneGeneratorAudioSourceTests, PrepareToPlay)
{
    EXPECT_NO_THROW (source->prepareToPlay (512, 44100.0));
    EXPECT_NO_THROW (source->prepareToPlay (1024, 48000.0));
}

TEST_F (ToneGeneratorAudioSourceTests, ReleaseResources)
{
    source->prepareToPlay (512, 44100.0);
    EXPECT_NO_THROW (source->releaseResources());
}

//==============================================================================
TEST_F (ToneGeneratorAudioSourceTests, GetNextAudioBlockInitializesPhase)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (1000.0);
    source->setAmplitude (0.5f);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    // First call should initialize phasePerSample (line 82-83)
    source->getNextAudioBlock (info);

    // Verify audio was generated
    bool hasNonZero = false;
    for (int i = 0; i < info.numSamples; ++i)
    {
        if (buffer.getSample (0, i) != 0.0f)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ToneGeneratorAudioSourceTests, GeneratesSineWave)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (440.0);
    source->setAmplitude (1.0f);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 0;
    info.numSamples = 512;

    source->getNextAudioBlock (info);

    // Check that both channels have the same content
    for (int i = 0; i < info.numSamples; ++i)
    {
        EXPECT_FLOAT_EQ (buffer.getSample (0, i), buffer.getSample (1, i));
    }

    // Check amplitude is within expected range
    float maxValue = 0.0f;
    for (int i = 0; i < info.numSamples; ++i)
    {
        maxValue = jmax (maxValue, std::abs (buffer.getSample (0, i)));
    }
    EXPECT_LE (maxValue, 1.0f);
    EXPECT_GT (maxValue, 0.5f); // Should be close to 1.0 for a full cycle
}

TEST_F (ToneGeneratorAudioSourceTests, GeneratesWithDifferentAmplitudes)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (1000.0);

    // Test with different amplitudes
    for (float amp : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
    {
        source->setAmplitude (amp);
        source->prepareToPlay (512, 44100.0); // Reset phase

        AudioBuffer<float> buffer (1, 512);
        buffer.clear();

        AudioSourceChannelInfo info;
        info.buffer = &buffer;
        info.startSample = 0;
        info.numSamples = 512;

        source->getNextAudioBlock (info);

        float maxValue = 0.0f;
        for (int i = 0; i < info.numSamples; ++i)
        {
            maxValue = jmax (maxValue, std::abs (buffer.getSample (0, i)));
        }

        if (amp == 0.0f)
        {
            EXPECT_FLOAT_EQ (maxValue, 0.0f);
        }
        else
        {
            EXPECT_LE (maxValue, amp);
        }
    }
}

TEST_F (ToneGeneratorAudioSourceTests, GeneratesWithDifferentFrequencies)
{
    source->prepareToPlay (512, 44100.0);
    source->setAmplitude (1.0f);

    for (double freq : { 100.0, 440.0, 1000.0, 5000.0 })
    {
        source->setFrequency (freq);
        source->prepareToPlay (512, 44100.0); // Reset phase

        AudioBuffer<float> buffer (1, 512);
        buffer.clear();

        AudioSourceChannelInfo info;
        info.buffer = &buffer;
        info.startSample = 0;
        info.numSamples = 512;

        source->getNextAudioBlock (info);

        // Just verify audio was generated
        bool hasNonZero = false;
        for (int i = 0; i < info.numSamples; ++i)
        {
            if (buffer.getSample (0, i) != 0.0f)
            {
                hasNonZero = true;
                break;
            }
        }
        EXPECT_TRUE (hasNonZero);
    }
}

TEST_F (ToneGeneratorAudioSourceTests, GeneratesWithMultipleChannels)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (1000.0);
    source->setAmplitude (0.5f);

    // Test with various channel counts
    for (int numChannels = 1; numChannels <= 8; ++numChannels)
    {
        AudioBuffer<float> buffer (numChannels, 256);
        buffer.clear();

        AudioSourceChannelInfo info;
        info.buffer = &buffer;
        info.startSample = 0;
        info.numSamples = 256;

        source->getNextAudioBlock (info);

        // All channels should have identical content (line 90-91)
        for (int ch = 1; ch < numChannels; ++ch)
        {
            for (int i = 0; i < info.numSamples; ++i)
            {
                EXPECT_FLOAT_EQ (buffer.getSample (0, i), buffer.getSample (ch, i));
            }
        }
    }
}

TEST_F (ToneGeneratorAudioSourceTests, GeneratesWithStartSampleOffset)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (1000.0);
    source->setAmplitude (0.5f);

    AudioBuffer<float> buffer (2, 512);
    buffer.clear();

    AudioSourceChannelInfo info;
    info.buffer = &buffer;
    info.startSample = 100;
    info.numSamples = 256;

    source->getNextAudioBlock (info);

    // Check that samples before startSample are still zero
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_FLOAT_EQ (buffer.getSample (0, i), 0.0f);
        EXPECT_FLOAT_EQ (buffer.getSample (1, i), 0.0f);
    }

    // Check that samples at startSample are non-zero
    bool hasNonZero = false;
    for (int i = 100; i < 356; ++i)
    {
        if (buffer.getSample (0, i) != 0.0f)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ToneGeneratorAudioSourceTests, PhaseAccumulatesAcrossCalls)
{
    source->prepareToPlay (512, 44100.0);
    source->setFrequency (1000.0);
    source->setAmplitude (1.0f);

    AudioBuffer<float> buffer1 (1, 256);
    AudioBuffer<float> buffer2 (1, 256);

    AudioSourceChannelInfo info1;
    info1.buffer = &buffer1;
    info1.startSample = 0;
    info1.numSamples = 256;

    AudioSourceChannelInfo info2;
    info2.buffer = &buffer2;
    info2.startSample = 0;
    info2.numSamples = 256;

    // Get two consecutive blocks
    source->getNextAudioBlock (info1);
    source->getNextAudioBlock (info2);

    // The phase should continue from first block to second
    // Last sample of first block should not equal first sample of second block
    EXPECT_NE (buffer1.getSample (0, 255), buffer2.getSample (0, 0));
}
