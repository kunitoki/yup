/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

namespace
{
AudioBuffer<float> getTestBuffer (int length)
{
    AudioBuffer<float> buffer { 2, length };

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            buffer.setSample (channel, sample, jmap ((float) sample, 0.0f, (float) length, -1.0f, 1.0f));

    return buffer;
}

AudioBuffer<float> getShortBuffer() { return getTestBuffer (5); }

AudioBuffer<float> getLongBuffer() { return getTestBuffer (1000); }

void play (MemoryAudioSource& source, AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();
    source.getNextAudioBlock (info);
}

bool isSilent (const AudioBuffer<float>& b)
{
    for (int channel = 0; channel < b.getNumChannels(); ++channel)
        if (b.findMinMax (channel, 0, b.getNumSamples()) != Range<float> {})
            return false;

    return true;
}
} // namespace

class MemoryAudioSourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bufferToFill.setSize (2, blockSize);
        channelInfo = std::make_unique<AudioSourceChannelInfo> (bufferToFill);
    }

    constexpr static int blockSize = 512;
    AudioBuffer<float> bufferToFill;
    std::unique_ptr<AudioSourceChannelInfo> channelInfo;
};

TEST_F (MemoryAudioSourceTests, ZeroLengthBufferProducesSilence)
{
    for (const bool enableLooping : { false, true })
    {
        AudioBuffer<float> buffer;
        MemoryAudioSource source { buffer, true, false };
        source.setLooping (enableLooping);
        source.prepareToPlay (blockSize, 44100.0);

        for (int i = 0; i < 2; ++i)
        {
            play (source, *channelInfo);
            EXPECT_TRUE (isSilent (bufferToFill));
        }
    }
}

TEST_F (MemoryAudioSourceTests, ShortBufferWithoutLoopingPlayedOnceAndSilence)
{
    auto buffer = getShortBuffer();
    MemoryAudioSource source { buffer, true, false };
    source.setLooping (false);
    source.prepareToPlay (blockSize, 44100.0);

    play (source, *channelInfo);

    auto copy = buffer;
    copy.setSize (buffer.getNumChannels(), blockSize, true, true, false);

    EXPECT_TRUE (bufferToFill == copy);

    play (source, *channelInfo);

    EXPECT_TRUE (isSilent (bufferToFill));
}

TEST_F (MemoryAudioSourceTests, ShortBufferWithLoopingPlayedMultipleTimes)
{
    auto buffer = getShortBuffer();
    MemoryAudioSource source { buffer, true, false };
    source.setLooping (true);
    source.prepareToPlay (blockSize, 44100.0);

    play (source, *channelInfo);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        EXPECT_EQ (bufferToFill.getSample (0, sample + buffer.getNumSamples()), buffer.getSample (0, sample));

    EXPECT_FALSE (isSilent (bufferToFill));
}

TEST_F (MemoryAudioSourceTests, LongBufferWithoutLoopingPlayedOnce)
{
    auto buffer = getLongBuffer();
    MemoryAudioSource source { buffer, true, false };
    source.setLooping (false);
    source.prepareToPlay (blockSize, 44100.0);

    play (source, *channelInfo);

    auto copy = buffer;
    copy.setSize (buffer.getNumChannels(), blockSize, true, true, false);

    EXPECT_TRUE (bufferToFill == copy);

    for (int i = 0; i < 10; ++i)
        play (source, *channelInfo);

    EXPECT_TRUE (isSilent (bufferToFill));
}

TEST_F (MemoryAudioSourceTests, LongBufferWithLoopingPlayedMultipleTimes)
{
    auto buffer = getLongBuffer();
    MemoryAudioSource source { buffer, true, false };
    source.setLooping (true);
    source.prepareToPlay (blockSize, 44100.0);

    for (int i = 0; i < 100; ++i)
    {
        play (source, *channelInfo);
        EXPECT_EQ (bufferToFill.getSample (0, 0), buffer.getSample (0, (i * blockSize) % buffer.getNumSamples()));
    }
}
