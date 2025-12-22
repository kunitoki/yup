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

TEST (UMPPacketBufferTests, AddEventOrdersBySamplePosition)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x40000000u, 0x12345678u };
    const UniversalPacket p2 { 0x20000000u };

    EXPECT_TRUE (buffer.addEvent (p1.data, p1.getSize(), 10));
    EXPECT_TRUE (buffer.addEvent (p2.data, p2.getSize(), 5));

    EXPECT_EQ (buffer.getNumEvents(), 2);
    EXPECT_EQ (buffer.getFirstEventTime(), 5);
    EXPECT_EQ (buffer.getLastEventTime(), 10);

    auto it = buffer.begin();
    const auto first = *it;
    EXPECT_EQ (first.samplePosition, 5);
    EXPECT_EQ (first.numWords, 1);
    EXPECT_EQ (first.getView()[0], p2.data[0]);

    ++it;
    const auto second = *it;
    EXPECT_EQ (second.samplePosition, 10);
    EXPECT_EQ (second.numWords, 2);
    EXPECT_EQ (second.getView()[0], p1.data[0]);
    EXPECT_EQ (second.getView()[1], p1.data[1]);
}

TEST (UMPPacketBufferTests, ConstructorAddsSinglePacket)
{
    const UniversalPacket packet { 0x40000000u, 0xabcdef01u };
    UMPPacketBuffer buffer { View (packet.data) };

    EXPECT_EQ (buffer.getNumEvents(), 1);
    EXPECT_EQ (buffer.getFirstEventTime(), 0);

    const auto meta = *buffer.begin();
    EXPECT_EQ (meta.samplePosition, 0);
    EXPECT_EQ (meta.numWords, 2);
    EXPECT_EQ (meta.getView()[0], packet.data[0]);
    EXPECT_EQ (meta.getView()[1], packet.data[1]);
}

TEST (UMPPacketBufferTests, FindNextSamplePosition)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };
    const UniversalPacket p3 { 0x40000000u, 0x22222222u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);
    buffer.addEvent (p3.data, p3.getSize(), 12);

    auto it = buffer.findNextSamplePosition (9);
    ASSERT_NE (it, buffer.end());
    EXPECT_EQ ((*it).samplePosition, 12);
}
