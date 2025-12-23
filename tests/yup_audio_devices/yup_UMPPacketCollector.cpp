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

#include <yup_audio_devices/yup_audio_devices.h>

using namespace yup;
using namespace yup::ump;

TEST (UMPPacketCollectorTests, CollectsPacketsIntoBlocks)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    const auto packet = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001 + 0.005;
    collector.addPacketToQueue (View (packet.data), timeStamp);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (buffer.getNumEvents(), 1);
}

TEST (UMPPacketCollectorTests, HandlesKeyboardStateCallbacks)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_2_0, 1);
    collector.reset (1000.0);

    collector.handleNoteOn (nullptr, 1, 64, 0.5f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (buffer.getNumEvents(), 1);
}

TEST (UMPPacketCollectorTests, HandlesNoteOffMidi1)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    collector.handleNoteOff (nullptr, 2, 60, 0.6f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (buffer.getNumEvents(), 1);
}

TEST (UMPPacketCollectorTests, HandlesNoteOffMidi2)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_2_0, 1);
    collector.reset (1000.0);

    collector.handleNoteOff (nullptr, 3, 72, 0.3f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (buffer.getNumEvents(), 1);
}

TEST (UMPPacketCollectorTests, PacketReceivedDirectly)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    const auto packet = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001 + 0.005;
    collector.packetReceived (View (packet.data), timeStamp);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
}

TEST (UMPPacketCollectorTests, MultiplePacketsInBlock)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    const auto packet1 = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    const auto packet2 = makeMidi1NoteOnMessage (0, 0, 64, Velocity { uint7_t { 90 } });
    const auto packet3 = makeMidi1NoteOnMessage (0, 0, 67, Velocity { uint7_t { 80 } });

    auto baseTime = Time::getMillisecondCounterHiRes() * 0.001;
    collector.addPacketToQueue (View (packet1.data), baseTime + 0.001);
    collector.addPacketToQueue (View (packet2.data), baseTime + 0.002);
    collector.addPacketToQueue (View (packet3.data), baseTime + 0.003);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 128);

    EXPECT_EQ (buffer.getNumEvents(), 3);
}

TEST (UMPPacketCollectorTests, EnsureStorageAllocated)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    collector.ensureStorageAllocated (1024);

    const auto packet = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001 + 0.005;
    collector.addPacketToQueue (View (packet.data), timeStamp);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_FALSE (buffer.isEmpty());
}

TEST (UMPPacketCollectorTests, ScalesPacketTimingForLargeBlocks)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    auto baseTime = Time::getMillisecondCounterHiRes() * 0.001;

    for (int i = 0; i < 100; ++i)
    {
        const auto packet = makeMidi1NoteOnMessage (0, 0, 60 + (i % 12), Velocity { uint7_t { 100 } });
        collector.addPacketToQueue (View (packet.data), baseTime + i * 0.001);
    }

    std::this_thread::sleep_for (std::chrono::milliseconds (10));

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_GT (buffer.getNumEvents(), 0);
}

TEST (UMPPacketCollectorTests, ClearsOldPackets)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    auto baseTime = Time::getMillisecondCounterHiRes() * 0.001;
    const auto packet = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    collector.addPacketToQueue (View (packet.data), baseTime - 2.0);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_GE (buffer.getNumEvents(), 0);
}

TEST (UMPPacketCollectorTests, EmptyBufferOnNoPackets)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_TRUE (buffer.isEmpty());
}

TEST (UMPPacketCollectorTests, VelocityConversionMidi1)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    collector.handleNoteOn (nullptr, 1, 60, 0.0f);
    collector.handleNoteOn (nullptr, 1, 61, 0.5f);
    collector.handleNoteOn (nullptr, 1, 62, 1.0f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_EQ (buffer.getNumEvents(), 3);
}

TEST (UMPPacketCollectorTests, VelocityConversionMidi2)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_2_0, 0);
    collector.reset (1000.0);

    collector.handleNoteOn (nullptr, 1, 60, 0.0f);
    collector.handleNoteOn (nullptr, 1, 61, 0.5f);
    collector.handleNoteOn (nullptr, 1, 62, 1.0f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_EQ (buffer.getNumEvents(), 3);
}

TEST (UMPPacketCollectorTests, DifferentGroupsInConstructor)
{
    UMPPacketCollector collector1 (PacketProtocol::MIDI_1_0, 0);
    UMPPacketCollector collector2 (PacketProtocol::MIDI_1_0, 5);

    collector1.reset (1000.0);
    collector2.reset (1000.0);

    collector1.handleNoteOn (nullptr, 1, 60, 0.8f);
    collector2.handleNoteOn (nullptr, 1, 60, 0.8f);

    UMPPacketBuffer buffer1, buffer2;
    collector1.removeNextBlockOfPackets (buffer1, 64);
    collector2.removeNextBlockOfPackets (buffer2, 64);

    EXPECT_FALSE (buffer1.isEmpty());
    EXPECT_FALSE (buffer2.isEmpty());
}

TEST (UMPPacketCollectorTests, ResetClearsIncomingPackets)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    const auto packet = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    const auto timeStamp = Time::getMillisecondCounterHiRes() * 0.001 + 0.005;
    collector.addPacketToQueue (View (packet.data), timeStamp);

    collector.reset (2000.0);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 64);

    EXPECT_TRUE (buffer.isEmpty());
}

TEST (UMPPacketCollectorTests, HandlesSmallBlockSizes)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    collector.handleNoteOn (nullptr, 1, 60, 0.8f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 1);

    EXPECT_FALSE (buffer.isEmpty());
}

TEST (UMPPacketCollectorTests, HandlesLargeBlockSizes)
{
    UMPPacketCollector collector (PacketProtocol::MIDI_1_0, 0);
    collector.reset (1000.0);

    collector.handleNoteOn (nullptr, 1, 60, 0.8f);

    UMPPacketBuffer buffer;
    collector.removeNextBlockOfPackets (buffer, 8192);

    EXPECT_FALSE (buffer.isEmpty());
}
