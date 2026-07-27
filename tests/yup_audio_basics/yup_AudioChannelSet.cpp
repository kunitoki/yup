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

// =============================================================================
// Operator Tests
// =============================================================================

TEST_F (AudioChannelSetTest, InequalityOperator)
{
    auto stereo = AudioChannelSet::stereo();
    auto mono = AudioChannelSet::mono();

    EXPECT_TRUE (stereo != mono);
    EXPECT_FALSE (stereo != stereo);
}

TEST_F (AudioChannelSetTest, LessThanOperator)
{
    auto mono = AudioChannelSet::mono();
    auto stereo = AudioChannelSet::stereo();

    // operator< compares the underlying channels bitmask
    EXPECT_FALSE (mono < stereo);
    EXPECT_TRUE (stereo < mono);
    EXPECT_FALSE (stereo < stereo);
}

// =============================================================================
// Channel Name Tests
// =============================================================================

TEST_F (AudioChannelSetTest, GetChannelTypeName)
{
    // Test standard channel names
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::left), "Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::right), "Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::centre), "Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::LFE), "LFE");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::leftSurround), "Left Surround");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::rightSurround), "Right Surround");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::leftCentre), "Left Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::rightCentre), "Right Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::centreSurround), "Centre Surround");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::leftSurroundRear), "Left Surround Rear");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::rightSurroundRear), "Right Surround Rear");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topMiddle), "Top Middle");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topFrontLeft), "Top Front Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topFrontCentre), "Top Front Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topFrontRight), "Top Front Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topRearLeft), "Top Rear Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topRearCentre), "Top Rear Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topRearRight), "Top Rear Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::wideLeft), "Wide Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::wideRight), "Wide Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::LFE2), "LFE 2");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::leftSurroundSide), "Left Surround Side");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::rightSurroundSide), "Right Surround Side");

    // Test ambisonic channels
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicW), "Ambisonic W");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicX), "Ambisonic X");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicY), "Ambisonic Y");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicZ), "Ambisonic Z");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicACN4), "Ambisonic 4");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicACN15), "Ambisonic 15");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::ambisonicACN63), "Ambisonic 63");

    // Test top/bottom channels
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topSideLeft), "Top Side Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::topSideRight), "Top Side Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomFrontLeft), "Bottom Front Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomFrontCentre), "Bottom Front Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomFrontRight), "Bottom Front Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::proximityLeft), "Proximity Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::proximityRight), "Proximity Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomSideLeft), "Bottom Side Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomSideRight), "Bottom Side Right");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomRearLeft), "Bottom Rear Left");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomRearCentre), "Bottom Rear Centre");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::bottomRearRight), "Bottom Rear Right");

    // Test discrete channels
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::discreteChannel0), "Discrete 1");
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (static_cast<AudioChannelSet::ChannelType> (AudioChannelSet::discreteChannel0 + 5)), "Discrete 6");

    // Test unknown channel
    EXPECT_EQ (AudioChannelSet::getChannelTypeName (AudioChannelSet::unknown), "Unknown");
}

TEST_F (AudioChannelSetTest, GetAbbreviatedChannelTypeName)
{
    // Test standard abbreviated names
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::left), "L");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::right), "R");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::centre), "C");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::LFE), "Lfe");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::leftSurround), "Ls");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::rightSurround), "Rs");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::leftCentre), "Lc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::rightCentre), "Rc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::centreSurround), "Cs");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::leftSurroundRear), "Lrs");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::rightSurroundRear), "Rrs");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topMiddle), "Tm");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topFrontLeft), "Tfl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topFrontCentre), "Tfc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topFrontRight), "Tfr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topRearLeft), "Trl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topRearCentre), "Trc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topRearRight), "Trr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::wideLeft), "Wl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::wideRight), "Wr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::LFE2), "Lfe2");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::leftSurroundSide), "Lss");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::rightSurroundSide), "Rss");

    // Test ambisonic abbreviations
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::ambisonicACN0), "ACN0");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::ambisonicACN10), "ACN10");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::ambisonicACN63), "ACN63");

    // Test top/bottom abbreviations
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topSideLeft), "Tsl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::topSideRight), "Tsr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomFrontLeft), "Bfl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomFrontCentre), "Bfc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomFrontRight), "Bfr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::proximityLeft), "Pl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::proximityRight), "Pr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomSideLeft), "Bsl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomSideRight), "Bsr");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomRearLeft), "Brl");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomRearCentre), "Brc");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::bottomRearRight), "Brr");

    // Test discrete channels
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::discreteChannel0), "1");
    EXPECT_EQ (AudioChannelSet::getAbbreviatedChannelTypeName (static_cast<AudioChannelSet::ChannelType> (AudioChannelSet::discreteChannel0 + 9)), "10");

    // Test unknown channel
    EXPECT_TRUE (AudioChannelSet::getAbbreviatedChannelTypeName (AudioChannelSet::unknown).isEmpty());
}

TEST_F (AudioChannelSetTest, GetChannelTypeFromAbbreviation)
{
    // Test standard abbreviations
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("L"), AudioChannelSet::left);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("R"), AudioChannelSet::right);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("C"), AudioChannelSet::centre);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Lfe"), AudioChannelSet::LFE);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Ls"), AudioChannelSet::leftSurround);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Rs"), AudioChannelSet::rightSurround);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Lfe2"), AudioChannelSet::LFE2);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Lss"), AudioChannelSet::leftSurroundSide);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Rss"), AudioChannelSet::rightSurroundSide);

    // Test ambisonic abbreviations
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("W"), AudioChannelSet::ambisonicW);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("X"), AudioChannelSet::ambisonicX);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Y"), AudioChannelSet::ambisonicY);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Z"), AudioChannelSet::ambisonicZ);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("ACN0"), AudioChannelSet::ambisonicACN0);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("ACN15"), AudioChannelSet::ambisonicACN15);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("ACN63"), AudioChannelSet::ambisonicACN63);

    // Test discrete channels (numeric)
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("1"), AudioChannelSet::discreteChannel0);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("5"), static_cast<AudioChannelSet::ChannelType> (AudioChannelSet::discreteChannel0 + 4));

    // Test top/bottom abbreviations
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Tsl"), AudioChannelSet::topSideLeft);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Tsr"), AudioChannelSet::topSideRight);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Bfl"), AudioChannelSet::bottomFrontLeft);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Bfc"), AudioChannelSet::bottomFrontCentre);
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("Brr"), AudioChannelSet::bottomRearRight);

    // Test unknown abbreviation
    EXPECT_EQ (AudioChannelSet::getChannelTypeFromAbbreviation ("XYZ"), AudioChannelSet::unknown);
}

// =============================================================================
// Speaker Arrangement String Tests
// =============================================================================

TEST_F (AudioChannelSetTest, GetSpeakerArrangementAsString)
{
    auto stereo = AudioChannelSet::stereo();
    EXPECT_EQ (stereo.getSpeakerArrangementAsString(), "L R");

    auto surround51 = AudioChannelSet::create5point1();
    EXPECT_EQ (surround51.getSpeakerArrangementAsString(), "L R C Lfe Ls Rs");

    auto mono = AudioChannelSet::mono();
    EXPECT_EQ (mono.getSpeakerArrangementAsString(), "C");
}

TEST_F (AudioChannelSetTest, FromAbbreviatedString)
{
    auto stereo = AudioChannelSet::fromAbbreviatedString ("L R");
    EXPECT_EQ (stereo, AudioChannelSet::stereo());

    auto surround51 = AudioChannelSet::fromAbbreviatedString ("L R C Lfe Ls Rs");
    EXPECT_EQ (surround51, AudioChannelSet::create5point1());

    auto mono = AudioChannelSet::fromAbbreviatedString ("C");
    EXPECT_EQ (mono, AudioChannelSet::mono());

    // Test with unknown abbreviations (should be ignored)
    auto partial = AudioChannelSet::fromAbbreviatedString ("L XYZ R");
    EXPECT_EQ (partial, AudioChannelSet::stereo());
}

// =============================================================================
// Description Tests
// =============================================================================

TEST_F (AudioChannelSetTest, GetDescription)
{
    // disabled() is a discrete layout with 0 channels
    EXPECT_EQ (AudioChannelSet::disabled().getDescription(), "Discrete #0");
    EXPECT_EQ (AudioChannelSet::mono().getDescription(), "Mono");
    EXPECT_EQ (AudioChannelSet::stereo().getDescription(), "Stereo");
    EXPECT_EQ (AudioChannelSet::createLCR().getDescription(), "LCR");
    EXPECT_EQ (AudioChannelSet::createLRS().getDescription(), "LRS");
    EXPECT_EQ (AudioChannelSet::createLCRS().getDescription(), "LCRS");
    EXPECT_EQ (AudioChannelSet::create5point0().getDescription(), "5.0 Surround");
    EXPECT_EQ (AudioChannelSet::create5point0point2().getDescription(), "5.0.2 Surround");
    EXPECT_EQ (AudioChannelSet::create5point0point4().getDescription(), "5.0.4 Surround");
    EXPECT_EQ (AudioChannelSet::create5point1().getDescription(), "5.1 Surround");
    EXPECT_EQ (AudioChannelSet::create5point1point2().getDescription(), "5.1.2 Surround");
    EXPECT_EQ (AudioChannelSet::create5point1point4().getDescription(), "5.1.4 Surround");
    EXPECT_EQ (AudioChannelSet::create6point0().getDescription(), "6.0 Surround");
    EXPECT_EQ (AudioChannelSet::create6point1().getDescription(), "6.1 Surround");
    EXPECT_EQ (AudioChannelSet::create6point0Music().getDescription(), "6.0 (Music) Surround");
    EXPECT_EQ (AudioChannelSet::create6point1Music().getDescription(), "6.1 (Music) Surround");
    EXPECT_EQ (AudioChannelSet::create7point0().getDescription(), "7.0 Surround");
    EXPECT_EQ (AudioChannelSet::create7point1().getDescription(), "7.1 Surround");
    EXPECT_EQ (AudioChannelSet::create7point0SDDS().getDescription(), "7.0 Surround SDDS");
    EXPECT_EQ (AudioChannelSet::create7point1SDDS().getDescription(), "7.1 Surround SDDS");
    EXPECT_EQ (AudioChannelSet::create7point0point2().getDescription(), "7.0.2 Surround");
    EXPECT_EQ (AudioChannelSet::create7point0point4().getDescription(), "7.0.4 Surround");
    EXPECT_EQ (AudioChannelSet::create7point0point6().getDescription(), "7.0.6 Surround");
    EXPECT_EQ (AudioChannelSet::create7point1point2().getDescription(), "7.1.2 Surround");
    EXPECT_EQ (AudioChannelSet::create7point1point4().getDescription(), "7.1.4 Surround");
    EXPECT_EQ (AudioChannelSet::create7point1point6().getDescription(), "7.1.6 Surround");
    EXPECT_EQ (AudioChannelSet::create9point0point4().getDescription(), "9.0.4 Surround");
    EXPECT_EQ (AudioChannelSet::create9point1point4().getDescription(), "9.1.4 Surround");
    EXPECT_EQ (AudioChannelSet::create9point0point6().getDescription(), "9.0.6 Surround");
    EXPECT_EQ (AudioChannelSet::create9point1point6().getDescription(), "9.1.6 Surround");
    EXPECT_EQ (AudioChannelSet::quadraphonic().getDescription(), "Quadraphonic");
    EXPECT_EQ (AudioChannelSet::pentagonal().getDescription(), "Pentagonal");
    EXPECT_EQ (AudioChannelSet::hexagonal().getDescription(), "Hexagonal");
    EXPECT_EQ (AudioChannelSet::octagonal().getDescription(), "Octagonal");

    // Test discrete layout
    EXPECT_EQ (AudioChannelSet::discreteChannels (4).getDescription(), "Discrete #4");

    // Test ambisonic descriptions
    EXPECT_EQ (AudioChannelSet::ambisonic (0).getDescription(), "0th Order Ambisonics");
    EXPECT_EQ (AudioChannelSet::ambisonic (1).getDescription(), "1st Order Ambisonics");
    EXPECT_EQ (AudioChannelSet::ambisonic (2).getDescription(), "2nd Order Ambisonics");
    EXPECT_EQ (AudioChannelSet::ambisonic (3).getDescription(), "3rd Order Ambisonics");
    EXPECT_EQ (AudioChannelSet::ambisonic (4).getDescription(), "4th Order Ambisonics");
}

// =============================================================================
// Channel Access Tests
// =============================================================================

TEST_F (AudioChannelSetTest, GetTypeOfChannel)
{
    auto stereo = AudioChannelSet::stereo();
    EXPECT_EQ (stereo.getTypeOfChannel (0), AudioChannelSet::left);
    EXPECT_EQ (stereo.getTypeOfChannel (1), AudioChannelSet::right);

    auto surround51 = AudioChannelSet::create5point1();
    EXPECT_EQ (surround51.getTypeOfChannel (0), AudioChannelSet::left);
    EXPECT_EQ (surround51.getTypeOfChannel (1), AudioChannelSet::right);
    EXPECT_EQ (surround51.getTypeOfChannel (2), AudioChannelSet::centre);
    EXPECT_EQ (surround51.getTypeOfChannel (3), AudioChannelSet::LFE);
    EXPECT_EQ (surround51.getTypeOfChannel (4), AudioChannelSet::leftSurround);
    EXPECT_EQ (surround51.getTypeOfChannel (5), AudioChannelSet::rightSurround);
}

TEST_F (AudioChannelSetTest, GetChannelIndexForType)
{
    auto surround51 = AudioChannelSet::create5point1();
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::left), 0);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::right), 1);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::centre), 2);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::LFE), 3);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::leftSurround), 4);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::rightSurround), 5);

    // Test channel not in set
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::topMiddle), -1);
}

// =============================================================================
// Channel Manipulation Tests
// =============================================================================

TEST_F (AudioChannelSetTest, RemoveChannel)
{
    auto surround51 = AudioChannelSet::create5point1();
    EXPECT_EQ (surround51.size(), 6);

    surround51.removeChannel (AudioChannelSet::LFE);
    EXPECT_EQ (surround51.size(), 5);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::LFE), -1);

    surround51.removeChannel (AudioChannelSet::centre);
    EXPECT_EQ (surround51.size(), 4);
    EXPECT_EQ (surround51.getChannelIndexForType (AudioChannelSet::centre), -1);
}

// =============================================================================
// Factory Method Tests
// =============================================================================

TEST_F (AudioChannelSetTest, CanonicalChannelSet)
{
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (1), AudioChannelSet::mono());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (2), AudioChannelSet::stereo());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (3), AudioChannelSet::createLCR());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (4), AudioChannelSet::quadraphonic());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (5), AudioChannelSet::create5point0());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (6), AudioChannelSet::create5point1());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (7), AudioChannelSet::create7point0());
    EXPECT_EQ (AudioChannelSet::canonicalChannelSet (8), AudioChannelSet::create7point1());

    // For channel counts without canonical layouts, should return discrete
    auto discrete10 = AudioChannelSet::canonicalChannelSet (10);
    EXPECT_TRUE (discrete10.isDiscreteLayout());
    EXPECT_EQ (discrete10.size(), 10);
}

TEST_F (AudioChannelSetTest, NamedChannelSet)
{
    EXPECT_EQ (AudioChannelSet::namedChannelSet (1), AudioChannelSet::mono());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (2), AudioChannelSet::stereo());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (3), AudioChannelSet::createLCR());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (4), AudioChannelSet::quadraphonic());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (5), AudioChannelSet::create5point0());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (6), AudioChannelSet::create5point1());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (7), AudioChannelSet::create7point0());
    EXPECT_EQ (AudioChannelSet::namedChannelSet (8), AudioChannelSet::create7point1());

    // For channel counts without named layouts, should return disabled (empty)
    auto empty = AudioChannelSet::namedChannelSet (10);
    EXPECT_EQ (empty.size(), 0);
    EXPECT_EQ (empty, AudioChannelSet::disabled());
}

// =============================================================================
// Wave Channel Mask Tests
// =============================================================================

TEST_F (AudioChannelSetTest, FromWaveChannelMask)
{
    // Test stereo (left + right = bits 0 and 1)
    auto stereo = AudioChannelSet::fromWaveChannelMask (0x3);
    EXPECT_EQ (stereo.size(), 2);

    // Test 5.1 (L, R, C, LFE, Ls, Rs)
    auto surround51 = AudioChannelSet::fromWaveChannelMask (0x3F);
    EXPECT_EQ (surround51.size(), 6);

    // Test empty
    auto empty = AudioChannelSet::fromWaveChannelMask (0x0);
    EXPECT_EQ (empty.size(), 0);
}

TEST_F (AudioChannelSetTest, GetWaveChannelMask)
{
    // Test stereo
    auto stereo = AudioChannelSet::stereo();
    EXPECT_EQ (stereo.getWaveChannelMask(), 0x3);

    // Test 5.1
    auto surround51 = AudioChannelSet::create5point1();
    EXPECT_EQ (surround51.getWaveChannelMask(), 0x3F);

    // Test mono (centre channel)
    auto mono = AudioChannelSet::mono();
    EXPECT_EQ (mono.getWaveChannelMask(), 0x4);

    // Test disabled
    auto disabled = AudioChannelSet::disabled();
    EXPECT_EQ (disabled.getWaveChannelMask(), 0x0);

    // Test set with channel beyond topRearRight (should return -1)
    AudioChannelSet highChannel;
    highChannel.addChannel (AudioChannelSet::ambisonicACN10);
    EXPECT_EQ (highChannel.getWaveChannelMask(), -1);
}
