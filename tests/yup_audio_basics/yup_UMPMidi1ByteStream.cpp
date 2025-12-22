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

namespace
{
bool packetEquals (const UniversalPacket& a, const UniversalPacket& b)
{
    return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2] && a.data[3] == b.data[3];
}
} // namespace

TEST (Midi1ByteStreamParserTests, ConstructorAndCallbacks)
{
    bool invoked = false;
    Midi1ByteStreamParser parser ([&] (UniversalPacket)
    {
        invoked = true;
    });
    EXPECT_TRUE (parser.callbacksEnabled());

    parser.feed (0xf8);
    EXPECT_TRUE (invoked);

    parser.enableCallbacks (false);
    invoked = false;
    parser.feed (0xfa);
    EXPECT_FALSE (invoked);
}

TEST (Midi1ByteStreamParserTests, GroupHandling)
{
    int calls = 0;
    Midi1ByteStreamParser parser (
        [&] (UniversalPacket packet)
    {
        ++calls;
        EXPECT_EQ (Group { 9 }, packet.getGroup());
    });

    parser.setGroup (9);
    parser.feed (0xfa);
    EXPECT_EQ (1, calls);
}

TEST (Midi1ByteStreamParserTests, SystemMessages)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket packet)
    {
        received.push_back (packet);
    });

    const uint8_t bytes[] = { 0xf8, 0xf1, 0x09, 0xfa, 0xf2, 0x11, 0x44, 0xfb, 0xf3, 0x75, 0xfc };
    for (const auto b : bytes)
        parser.feed (b);

    std::vector<UniversalPacket> expected {
        makeSystemMessage (0, Status (SystemStatus::clock)),
        makeSystemMessage (0, Status (SystemStatus::mtcQuarterFrame), 0x09),
        makeSystemMessage (0, Status (SystemStatus::start)),
        makeSystemMessage (0, Status (SystemStatus::songPosition), 0x11, 0x44),
        makeSystemMessage (0, Status (SystemStatus::cont)),
        makeSystemMessage (0, Status (SystemStatus::songSelect), 0x75),
        makeSystemMessage (0, Status (SystemStatus::stop))
    };

    ASSERT_EQ (expected.size(), received.size());
    for (size_t i = 0; i < expected.size(); ++i)
        EXPECT_TRUE (packetEquals (expected[i], received[i]));
}

TEST (Midi1ByteStreamParserTests, ChannelVoiceMessages)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket packet)
    {
        received.push_back (packet);
    });

    const uint8_t bytes[] = { 0x83, 0x45, 0x6e, 0x9e, 0x30, 0x7f, 0xaa, 0x44, 0x03, 0x77, 0xb0, 0x07, 0x70 };
    for (const auto b : bytes)
        parser.feed (b);

    std::vector<UniversalPacket> expected {
        makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOff), 3, 0x45, 0x6e),
        makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOn), 14, 0x30, 0x7f),
        makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::polyPressure), 10, 0x44, 0x03),
        makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::controlChange), 0, 0x07, 0x70)
    };

    ASSERT_EQ (expected.size(), received.size());
    for (size_t i = 0; i < expected.size(); ++i)
        EXPECT_TRUE (packetEquals (expected[i], received[i]));
}

TEST (Midi1ByteStreamParserTests, Midi1ByteStreamRoundTrip)
{
    const auto packet = makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOn), 1, 60, 90);

    uint8_t bytes[8] = {};
    const auto size = toMidi1ByteStream (packet, bytes);
    EXPECT_EQ (size, 3u);

    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });
    parser.feed (bytes, size);

    ASSERT_EQ (received.size(), 1u);
    EXPECT_TRUE (packetEquals (packet, received.front()));
}
