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

using namespace yup;
using namespace yup::ump;

TEST (UMPExtendedDataMessagesTests, ExtendedDataMessageConstructors)
{
    {
        constexpr ExtendedDataMessage m;
        EXPECT_EQ (m.getType(), PacketType::extendedData);
        EXPECT_EQ (m.getGroup(), 0u);
    }

    {
        constexpr ExtendedDataMessage m { Status (ExtendedDataStatus::sysex8End) };
        EXPECT_EQ (m.getType(), PacketType::extendedData);
        EXPECT_EQ (m.getGroup(), 0u);
        EXPECT_EQ (m.getStatus(), Status (ExtendedDataStatus::sysex8End));
    }
}

TEST (UMPExtendedDataMessagesTests, SysEx8PacketConstructors)
{
    {
        constexpr SysEx8Packet m;
        EXPECT_EQ (m.getType(), PacketType::extendedData);
        EXPECT_EQ (m.getGroup(), 0u);
        EXPECT_EQ (m.getFormat(), PacketFormat::complete);
        EXPECT_EQ (m.getPayloadSize(), 0u);
        EXPECT_TRUE (isExtendedDataMessage (m));
    }

    {
        constexpr SysEx8Packet m { Status (ExtendedDataStatus::sysex8End), 0xac, 0x0c };
        EXPECT_EQ (m.getGroup(), 0x0c);
        EXPECT_EQ (m.getFormat(), PacketFormat::end);
        EXPECT_EQ (m.getStreamId(), 0xac);
        EXPECT_TRUE (isExtendedDataMessage (m));
    }
}

TEST (UMPExtendedDataMessagesTests, SysEx8PacketHelpers)
{
    auto m = makeSysEx8CompletePacket (0);
    EXPECT_EQ (m.getFormat(), PacketFormat::complete);
    EXPECT_EQ (m.getStreamId(), 0u);

    m.setStreamId (99);
    EXPECT_EQ (m.getStreamId(), 99u);

    m.addPayloadByte (0x12);
    EXPECT_EQ (m.getPayloadSize(), 1u);
    EXPECT_EQ (m.getPayloadByte (0), 0x12);
}

TEST (UMPExtendedDataMessagesTests, SysEx8PacketFormats)
{
    EXPECT_EQ (makeSysEx8CompletePacket (0).getFormat(), PacketFormat::complete);
    EXPECT_EQ (makeSysEx8StartPacket (0).getFormat(), PacketFormat::start);
    EXPECT_EQ (makeSysEx8ContinuePacket (0).getFormat(), PacketFormat::cont);
    EXPECT_EQ (makeSysEx8EndPacket (0).getFormat(), PacketFormat::end);
}

TEST (UMPExtendedDataMessagesTests, SysEx8MultiplePayloadBytesAreStoredCorrectly)
{
    auto m = makeSysEx8CompletePacket (0);
    const uint8_t bytes[] = { 0x01, 0x23, 0x45, 0x67, 0x89 };
    for (auto b : bytes)
        m.addPayloadByte (b);

    EXPECT_EQ (m.getPayloadSize(), 5u);
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ (m.getPayloadByte (i), bytes[i]);
}

TEST (UMPExtendedDataMessagesTests, SysEx8MaxPayloadIs13Bytes)
{
    auto m = makeSysEx8CompletePacket (0);
    for (int i = 0; i < 13; ++i)
        m.addPayloadByte (static_cast<uint8_t> (i + 1));

    EXPECT_EQ (m.getPayloadSize(), 13u);
    for (int i = 0; i < 13; ++i)
        EXPECT_EQ (m.getPayloadByte (static_cast<size_t> (i)), static_cast<uint8_t> (i + 1));
}

TEST (UMPExtendedDataMessagesTests, SysEx8GroupIsPreservedAcrossPacketTypes)
{
    const uint8_t streamId = 42;
    EXPECT_EQ (makeSysEx8StartPacket (streamId, 3).getGroup(), 3u);
    EXPECT_EQ (makeSysEx8ContinuePacket (streamId, 3).getGroup(), 3u);
    EXPECT_EQ (makeSysEx8EndPacket (streamId, 3).getGroup(), 3u);
}
