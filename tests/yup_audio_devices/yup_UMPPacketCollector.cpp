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
