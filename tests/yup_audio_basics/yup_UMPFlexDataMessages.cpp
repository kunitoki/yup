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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

#include <string>

using namespace yup;
using namespace yup::ump;

TEST (UMPFlexDataMessagesTests, ConstructorsAndView)
{
    FlexDataMessage m (2);
    EXPECT_EQ (m.getType(), PacketType::flexData);
    EXPECT_EQ (m.getGroup(), 2u);

    auto view = FlexDataMessageView { m };
    EXPECT_EQ (view.getGroup(), 2u);
}

TEST (UMPFlexDataMessagesTests, MakeTextMessage)
{
    auto msg = makeFlexDataTextMessage (1,
                                        PacketFormat::complete,
                                        PacketAddress::group,
                                        0,
                                        0x01,
                                        0x02,
                                        "Hello");

    EXPECT_EQ (msg.getFormat(), PacketFormat::complete);
    EXPECT_EQ (FlexDataMessage::getPayloadAsString (msg), "Hello");
}

TEST (UMPFlexDataMessagesTests, SetTempoAndTimeSignature)
{
    auto tempo = makeSetTempoMessage (3, 0x11223344u);
    EXPECT_EQ (tempo.getGroup(), 3u);
    EXPECT_EQ (tempo.data[1], 0x11223344u);

    auto timeSig = makeSetTimeSignatureMessage (4, 7, 8, 12);
    EXPECT_EQ (timeSig.getGroup(), 4u);
    EXPECT_EQ (timeSig.getByte (4), 7u);
    EXPECT_EQ (timeSig.getByte (5), 8u);
    EXPECT_EQ (timeSig.getByte (6), 12u);
}

TEST (UMPFlexDataMessagesTests, SetMetronomeAndKeySignature)
{
    auto metro = makeSetMetronomeMessage (1, 24, 1, 2, 3, 4, 5);
    EXPECT_EQ (metro.getByte (4), 24u);
    EXPECT_EQ (metro.getByte (5), 1u);
    EXPECT_EQ (metro.getByte (6), 2u);
    EXPECT_EQ (metro.getByte (7), 3u);

    auto key = makeSetKeySignatureMessage (2, PacketAddress::group, 0, 3, 5);
    EXPECT_EQ (key.getByte (4), 0x35u);
}

TEST (UMPFlexDataMessagesTests, SetChord)
{
    auto chord = makeSetChordMessage (3, PacketAddress::group, 0, 0x11223344u, 0x55667788u, 0x99aabbccu);
    EXPECT_EQ (chord.data[1], 0x11223344u);
    EXPECT_EQ (chord.data[2], 0x55667788u);
    EXPECT_EQ (chord.data[3], 0x99aabbccu);
}

TEST (UMPFlexDataMessagesTests, IsFlexDataMessage)
{
    FlexDataMessage m (2);
    EXPECT_TRUE (isFlexDataMessage (m));
}

TEST (UMPFlexDataMessagesTests, TextMessageWithEmptyStringProducesEmptyPayload)
{
    auto msg = makeFlexDataTextMessage (0, PacketFormat::complete, PacketAddress::group, 0, 0x01, 0x02, "");
    EXPECT_EQ (FlexDataMessage::getPayloadAsString (msg), "");
}

TEST (UMPFlexDataMessagesTests, SetTempoGroupIsPreserved)
{
    auto tempo = makeSetTempoMessage (5, 500000u);
    EXPECT_EQ (tempo.getGroup(), 5u);
    EXPECT_EQ (tempo.data[1], 500000u);
}
