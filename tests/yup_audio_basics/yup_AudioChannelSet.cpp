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

#include <yup_audio_basics/yup_audio_basics.h>
#include <gtest/gtest.h>

using namespace yup;

namespace
{
AudioChannelSet channelSetFromMask (uint64 mask)
{
    Array<AudioChannelSet::ChannelType> channels;
    for (int bit = 0; bit <= 62; ++bit)
        if ((mask & (1ull << bit)) != 0)
            channels.add (static_cast<AudioChannelSet::ChannelType> (bit));

    return AudioChannelSet::channelSetWithChannels (channels);
}
} // namespace

class AudioChannelSetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        max = AudioChannelSet::maxChannelsOfNamedLayout;
    }

    void checkAmbisonic (uint64 mask, int order, const char* layoutName)
    {
        auto expected = AudioChannelSet::ambisonic (order);
        auto numChannels = expected.size();

        EXPECT_EQ (numChannels, BigInteger ((int64) mask).countNumberOfSetBits());
        EXPECT_EQ (channelSetFromMask (mask), expected);

        EXPECT_EQ (order, expected.getAmbisonicOrder());
        EXPECT_EQ (expected.getDescription(), layoutName);

        auto layouts = AudioChannelSet::channelSetsWithNumberOfChannels (numChannels);
        EXPECT_TRUE (layouts.contains (expected));

        for (auto layout : layouts)
            EXPECT_EQ (layout.getAmbisonicOrder(), (layout == expected ? order : -1));
    }

    int max;
};

TEST_F (AudioChannelSetTest, MaxChannelsOfNamedLayoutIsNonDiscrete)
{
    EXPECT_GE (AudioChannelSet::channelSetsWithNumberOfChannels (max).size(), 2);
}

TEST_F (AudioChannelSetTest, ChannelSetsWithNumberOfChannelsReturnsCorrectSpeakerCount)
{
    for (auto ch = 1; ch <= max; ++ch)
    {
        auto channelSets = AudioChannelSet::channelSetsWithNumberOfChannels (ch);

        for (auto set : channelSets)
            EXPECT_EQ (set.size(), ch);
    }
}

TEST_F (AudioChannelSetTest, Ambisonics)
{
    uint64 mask = 0;

    mask |= (1ull << AudioChannelSet::ambisonicACN0);
    checkAmbisonic (mask, 0, "0th Order Ambisonics");

    mask |= (1ull << AudioChannelSet::ambisonicACN1) | (1ull << AudioChannelSet::ambisonicACN2) | (1ull << AudioChannelSet::ambisonicACN3);
    checkAmbisonic (mask, 1, "1st Order Ambisonics");

    mask |= (1ull << AudioChannelSet::ambisonicACN4) | (1ull << AudioChannelSet::ambisonicACN5) | (1ull << AudioChannelSet::ambisonicACN6)
          | (1ull << AudioChannelSet::ambisonicACN7) | (1ull << AudioChannelSet::ambisonicACN8);
    checkAmbisonic (mask, 2, "2nd Order Ambisonics");

    mask |= (1ull << AudioChannelSet::ambisonicACN9) | (1ull << AudioChannelSet::ambisonicACN10) | (1ull << AudioChannelSet::ambisonicACN11)
          | (1ull << AudioChannelSet::ambisonicACN12) | (1ull << AudioChannelSet::ambisonicACN13) | (1ull << AudioChannelSet::ambisonicACN14)
          | (1ull << AudioChannelSet::ambisonicACN15);
    checkAmbisonic (mask, 3, "3rd Order Ambisonics");

    mask |= (1ull << AudioChannelSet::ambisonicACN16) | (1ull << AudioChannelSet::ambisonicACN17) | (1ull << AudioChannelSet::ambisonicACN18)
          | (1ull << AudioChannelSet::ambisonicACN19) | (1ull << AudioChannelSet::ambisonicACN20) | (1ull << AudioChannelSet::ambisonicACN21)
          | (1ull << AudioChannelSet::ambisonicACN22) | (1ull << AudioChannelSet::ambisonicACN23) | (1ull << AudioChannelSet::ambisonicACN24);
    checkAmbisonic (mask, 4, "4th Order Ambisonics");

    mask |= (1ull << AudioChannelSet::ambisonicACN25) | (1ull << AudioChannelSet::ambisonicACN26) | (1ull << AudioChannelSet::ambisonicACN27)
          | (1ull << AudioChannelSet::ambisonicACN28) | (1ull << AudioChannelSet::ambisonicACN29) | (1ull << AudioChannelSet::ambisonicACN30)
          | (1ull << AudioChannelSet::ambisonicACN31) | (1ull << AudioChannelSet::ambisonicACN32) | (1ull << AudioChannelSet::ambisonicACN33)
          | (1ull << AudioChannelSet::ambisonicACN34) | (1ull << AudioChannelSet::ambisonicACN35);
    checkAmbisonic (mask, 5, "5th Order Ambisonics");
}
