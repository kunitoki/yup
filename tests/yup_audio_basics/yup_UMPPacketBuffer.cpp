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

TEST (UMPPacketBufferTests, ClearEntireBuffer)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 5);
    buffer.addEvent (p2.data, p2.getSize(), 10);

    EXPECT_FALSE (buffer.isEmpty());
    buffer.clear();
    EXPECT_TRUE (buffer.isEmpty());
}

TEST (UMPPacketBufferTests, ClearRangeRemovesEvents)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };
    const UniversalPacket p3 { 0x40000000u, 0x22222222u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);
    buffer.addEvent (p3.data, p3.getSize(), 12);

    buffer.clear (7, 6);

    EXPECT_EQ (buffer.getNumEvents(), 1);
    EXPECT_EQ (buffer.getFirstEventTime(), 2);
    EXPECT_EQ (buffer.getLastEventTime(), 2);
}

TEST (UMPPacketBufferTests, ClearRangeAtStart)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);

    buffer.clear (0, 5);

    EXPECT_EQ (buffer.getNumEvents(), 1);
    EXPECT_EQ (buffer.getFirstEventTime(), 8);
}

TEST (UMPPacketBufferTests, ClearRangeAtEnd)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);

    buffer.clear (7, 10);

    EXPECT_EQ (buffer.getNumEvents(), 1);
    EXPECT_EQ (buffer.getLastEventTime(), 2);
}

TEST (UMPPacketBufferTests, SwapWith)
{
    UMPPacketBuffer buffer1;
    UMPPacketBuffer buffer2;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer1.addEvent (p1.data, p1.getSize(), 5);
    buffer2.addEvent (p2.data, p2.getSize(), 10);

    buffer1.swapWith (buffer2);

    EXPECT_EQ (buffer1.getNumEvents(), 1);
    EXPECT_EQ (buffer1.getFirstEventTime(), 10);
    EXPECT_EQ (buffer2.getNumEvents(), 1);
    EXPECT_EQ (buffer2.getFirstEventTime(), 5);
}

TEST (UMPPacketBufferTests, EnsureSize)
{
    UMPPacketBuffer buffer;
    buffer.ensureSize (1024);

    const UniversalPacket p1 { 0x20000000u };
    EXPECT_TRUE (buffer.addEvent (p1.data, p1.getSize(), 0));
}

TEST (UMPPacketBufferTests, AddEventWithView)
{
    UMPPacketBuffer buffer;

    const UniversalPacket packet { 0x40000000u, 0x12345678u };
    EXPECT_TRUE (buffer.addEvent (View (packet.data), 5));

    EXPECT_EQ (buffer.getNumEvents(), 1);
    EXPECT_EQ (buffer.getFirstEventTime(), 5);
}

TEST (UMPPacketBufferTests, AddEventRejectsInvalidData)
{
    UMPPacketBuffer buffer;

    EXPECT_FALSE (buffer.addEvent (nullptr, 1, 0));
    EXPECT_FALSE (buffer.addEvent ((uint32_t*) 0x1, 0, 0));
    EXPECT_FALSE (buffer.addEvent ((uint32_t*) 0x1, 5, 0));
}

TEST (UMPPacketBufferTests, AddEventsFromOtherBuffer)
{
    UMPPacketBuffer buffer1;
    UMPPacketBuffer buffer2;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };
    const UniversalPacket p3 { 0x40000000u, 0x22222222u };

    buffer1.addEvent (p1.data, p1.getSize(), 2);
    buffer1.addEvent (p2.data, p2.getSize(), 8);
    buffer1.addEvent (p3.data, p3.getSize(), 12);

    buffer2.addEvents (buffer1, 0, -1, 0);

    EXPECT_EQ (buffer2.getNumEvents(), 3);
}

TEST (UMPPacketBufferTests, AddEventsWithRange)
{
    UMPPacketBuffer buffer1;
    UMPPacketBuffer buffer2;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };
    const UniversalPacket p3 { 0x40000000u, 0x22222222u };

    buffer1.addEvent (p1.data, p1.getSize(), 2);
    buffer1.addEvent (p2.data, p2.getSize(), 8);
    buffer1.addEvent (p3.data, p3.getSize(), 12);

    buffer2.addEvents (buffer1, 5, 10, 0);

    EXPECT_EQ (buffer2.getNumEvents(), 2);
    EXPECT_EQ (buffer2.getFirstEventTime(), 8);
    EXPECT_EQ (buffer2.getLastEventTime(), 12);
}

TEST (UMPPacketBufferTests, AddEventsWithSampleDelta)
{
    UMPPacketBuffer buffer1;
    UMPPacketBuffer buffer2;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer1.addEvent (p1.data, p1.getSize(), 2);
    buffer1.addEvent (p2.data, p2.getSize(), 8);

    buffer2.addEvents (buffer1, 0, -1, 10);

    EXPECT_EQ (buffer2.getNumEvents(), 2);
    EXPECT_EQ (buffer2.getFirstEventTime(), 12);
    EXPECT_EQ (buffer2.getLastEventTime(), 18);
}

TEST (UMPPacketBufferTests, IteratorPreIncrement)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);

    auto it = buffer.begin();
    EXPECT_EQ ((*it).samplePosition, 2);

    ++it;
    EXPECT_EQ ((*it).samplePosition, 8);
}

TEST (UMPPacketBufferTests, IteratorPostIncrement)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);

    auto it = buffer.begin();
    auto oldIt = it++;

    EXPECT_EQ ((*oldIt).samplePosition, 2);
    EXPECT_EQ ((*it).samplePosition, 8);
}

TEST (UMPPacketBufferTests, IteratorDereference)
{
    UMPPacketBuffer buffer;

    const UniversalPacket packet { 0x40000000u, 0x12345678u };
    buffer.addEvent (packet.data, packet.getSize(), 5);

    auto it = buffer.begin();
    auto meta = *it;

    EXPECT_EQ (meta.samplePosition, 5);
    EXPECT_EQ (meta.numWords, 2);
    EXPECT_EQ (meta.getView()[0], packet.data[0]);
    EXPECT_EQ (meta.getView()[1], packet.data[1]);
}

TEST (UMPPacketBufferTests, GetFirstEventTimeOnEmpty)
{
    UMPPacketBuffer buffer;
    EXPECT_EQ (buffer.getFirstEventTime(), 0);
}

TEST (UMPPacketBufferTests, GetLastEventTimeOnEmpty)
{
    UMPPacketBuffer buffer;
    EXPECT_EQ (buffer.getLastEventTime(), 0);
}

TEST (UMPPacketBufferTests, GetNumEventsOnEmpty)
{
    UMPPacketBuffer buffer;
    EXPECT_EQ (buffer.getNumEvents(), 0);
}

TEST (UMPPacketBufferTests, FindNextSamplePositionAtStart)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    buffer.addEvent (p1.data, p1.getSize(), 5);

    auto it = buffer.findNextSamplePosition (0);
    ASSERT_NE (it, buffer.end());
    EXPECT_EQ ((*it).samplePosition, 5);
}

TEST (UMPPacketBufferTests, FindNextSamplePositionBeyondEnd)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    buffer.addEvent (p1.data, p1.getSize(), 5);

    auto it = buffer.findNextSamplePosition (100);
    EXPECT_EQ (it, buffer.end());
}

TEST (UMPPacketBufferTests, RangeBasedForLoop)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 2);
    buffer.addEvent (p2.data, p2.getSize(), 8);

    int count = 0;
    for (const auto& meta : buffer)
    {
        EXPECT_GE (meta.samplePosition, 0);
        ++count;
    }

    EXPECT_EQ (count, 2);
}

TEST (UMPPacketBufferTests, AddMultipleEventsAtSameSample)
{
    UMPPacketBuffer buffer;

    const UniversalPacket p1 { 0x20000000u };
    const UniversalPacket p2 { 0x40000000u, 0x11111111u };

    buffer.addEvent (p1.data, p1.getSize(), 5);
    buffer.addEvent (p2.data, p2.getSize(), 5);

    EXPECT_EQ (buffer.getNumEvents(), 2);
    EXPECT_EQ (buffer.getFirstEventTime(), 5);
    EXPECT_EQ (buffer.getLastEventTime(), 5);
}
