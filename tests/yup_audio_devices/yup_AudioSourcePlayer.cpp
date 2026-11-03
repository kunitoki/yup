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

#include <gtest/gtest.h>

#include <yup_audio_devices/yup_audio_devices.h>

using namespace yup;

class AudioSourcePlayerTests : public ::testing::Test
{
protected:
    struct CountingSource : AudioSource
    {
        void prepareToPlay (int, double) override {}

        void releaseResources() override {}

        void getNextAudioBlock (const AudioSourceChannelInfo& info) override
        {
            ++numBlocks;
            info.clearActiveBufferRegion();
        }

        int numBlocks = 0;
    };

    void callback()
    {
        float* outputs[] = { output.data() };
        player.audioDeviceIOCallbackWithContext (nullptr, 0, outputs, 1, (int) output.size(), {});
    }

    CountingSource source;
    AudioSourcePlayer player;
    std::vector<float> output = std::vector<float> (64, 1.0f);
};

TEST_F (AudioSourcePlayerTests, PullsFromSourceWhilePrepared)
{
    player.setSource (&source);
    player.prepareToPlay (44100.0, 64);

    callback();

    EXPECT_EQ (1, source.numBlocks);
    player.setSource (nullptr);
}

TEST_F (AudioSourcePlayerTests, TrailingCallbackAfterStopOutputsSilenceWithoutPullingSource)
{
    player.setSource (&source);
    player.prepareToPlay (44100.0, 64);
    player.audioDeviceStopped();

    callback();

    EXPECT_EQ (0, source.numBlocks);

    for (const auto sample : output)
        EXPECT_EQ (0.0f, sample);

    player.setSource (nullptr);
}
