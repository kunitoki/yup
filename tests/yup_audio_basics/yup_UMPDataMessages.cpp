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

TEST (UMPDataMessagesTests, DataMessageConstructors)
{
    {
        constexpr DataMessage m;
        EXPECT_EQ (m.getType(), PacketType::data);
        EXPECT_EQ (m.getGroup(), 0u);
        EXPECT_EQ (m.getStatus(), Status (DataStatus::sysex7Complete));
    }

    {
        constexpr DataMessage m { Status (DataStatus::sysex7End) };
        EXPECT_EQ (m.getType(), PacketType::data);
        EXPECT_EQ (m.getGroup(), 0u);
        EXPECT_EQ (m.getStatus(), Status (DataStatus::sysex7End));
    }
}

TEST (UMPDataMessagesTests, SysEx7PacketConstructors)
{
    {
        constexpr SysEx7Packet p;
        EXPECT_EQ (p.getType(), PacketType::data);
        EXPECT_EQ (p.getGroup(), 0u);
        EXPECT_EQ (p.getStatus(), Status (DataStatus::sysex7Complete));
        EXPECT_EQ (p.getFormat(), PacketFormat::complete);
        EXPECT_EQ (p.getPayloadSize(), 0u);
    }

    {
        constexpr SysEx7Packet p { Status (DataStatus::sysex7End), 9 };
        EXPECT_EQ (p.getType(), PacketType::data);
        EXPECT_EQ (p.getGroup(), 9u);
        EXPECT_EQ (p.getStatus(), Status (DataStatus::sysex7End));
        EXPECT_EQ (p.getFormat(), PacketFormat::end);
        EXPECT_EQ (p.getPayloadSize(), 0u);
    }
}

TEST (UMPDataMessagesTests, MakeSysEx7Packets)
{
    {
        constexpr auto m = makeSysEx7CompletePacket();
        EXPECT_EQ (m.getFormat(), PacketFormat::complete);
        EXPECT_EQ (m.getPayloadSize(), 0u);
    }

    {
        constexpr auto m = makeSysEx7StartPacket (0xf);
        EXPECT_EQ (m.getGroup(), 0xf);
        EXPECT_EQ (m.getFormat(), PacketFormat::start);
    }

    {
        constexpr auto m = makeSysEx7ContinuePacket (9);
        EXPECT_EQ (m.getGroup(), 9u);
        EXPECT_EQ (m.getFormat(), PacketFormat::cont);
    }

    {
        constexpr auto m = makeSysEx7EndPacket (1);
        EXPECT_EQ (m.getGroup(), 1u);
        EXPECT_EQ (m.getFormat(), PacketFormat::end);
    }
}

TEST (UMPDataMessagesTests, SysEx7PacketPayloadHelpers)
{
    auto m = makeSysEx7CompletePacket();
    EXPECT_EQ (m.getPayloadSize(), 0u);

    m.addPayloadByte (0x12);
    m.addPayloadByte (0x34);

    EXPECT_EQ (m.getPayloadSize(), 2u);
    EXPECT_EQ (m.getPayloadByte (0), 0x12);
    EXPECT_EQ (m.getPayloadByte (1), 0x34);
}

TEST (UMPDataMessagesTests, SysEx7PacketSetPayloadByte)
{
    auto m = makeSysEx7CompletePacket();
    m.addPayloadByte (0x00);
    m.addPayloadByte (0x00);
    m.setPayloadByte (0, 0x2B);
    m.setPayloadByte (1, 0x4D);

    EXPECT_EQ (m.getPayloadByte (0), 0x2Bu);
    EXPECT_EQ (m.getPayloadByte (1), 0x4Du);
}

TEST (UMPDataMessagesTests, SysEx7MaxPayloadIs6Bytes)
{
    auto m = makeSysEx7CompletePacket();
    for (int i = 0; i < 6; ++i)
        m.addPayloadByte (static_cast<uint8_t> (i));

    EXPECT_EQ (m.getPayloadSize(), 6u);

    // Verify each byte was stored correctly
    for (int i = 0; i < 6; ++i)
        EXPECT_EQ (m.getPayloadByte (static_cast<size_t> (i)), static_cast<uint8_t> (i));
}

TEST (UMPDataMessagesTests, SysEx7GroupIsPreservedInStart)
{
    auto start = makeSysEx7StartPacket (7);
    auto cont = makeSysEx7ContinuePacket (7);
    auto end = makeSysEx7EndPacket (7);

    EXPECT_EQ (start.getGroup(), 7u);
    EXPECT_EQ (cont.getGroup(), 7u);
    EXPECT_EQ (end.getGroup(), 7u);
}
