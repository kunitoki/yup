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

TEST (Midi1ByteStreamParserTests, RunningStatus)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0x90);
    parser.feed (0x3c);
    parser.feed (0x7f);
    parser.feed (0x40);
    parser.feed (0x60);

    ASSERT_EQ (received.size(), 2u);
    EXPECT_TRUE (packetEquals (makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOn), 0, 0x3c, 0x7f), received[0]));
    EXPECT_TRUE (packetEquals (makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOn), 0, 0x40, 0x60), received[1]));
}

TEST (Midi1ByteStreamParserTests, ProgramChangeAndChannelPressure)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xc5);
    parser.feed (0x42);
    parser.feed (0xd7);
    parser.feed (0x55);

    ASSERT_EQ (received.size(), 2u);
    EXPECT_TRUE (packetEquals (makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::programChange), 5, 0x42, 0), received[0]));
    EXPECT_TRUE (packetEquals (makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::channelPressure), 7, 0x55, 0), received[1]));
}

TEST (Midi1ByteStreamParserTests, SystemRealtimeInterleaved)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0x90);
    parser.feed (0xf8);
    parser.feed (0x3c);
    parser.feed (0xfa);
    parser.feed (0x7f);

    ASSERT_EQ (received.size(), 3u);
    EXPECT_TRUE (packetEquals (makeSystemMessage (0, Status (SystemStatus::clock)), received[0]));
    EXPECT_TRUE (packetEquals (makeSystemMessage (0, Status (SystemStatus::start)), received[1]));
    EXPECT_TRUE (packetEquals (makeMidi1ChannelVoiceMessage (0, Status (Midi1ChannelVoiceStatus::noteOn), 0, 0x3c, 0x7f), received[2]));
}

TEST (Midi1ByteStreamParserTests, IgnoredRealtimeBytes)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xf9);
    parser.feed (0xfd);
    parser.feed (0xf8);

    ASSERT_EQ (received.size(), 1u);
    EXPECT_TRUE (packetEquals (makeSystemMessage (0, Status (SystemStatus::clock)), received[0]));
}

TEST (Midi1ByteStreamParserTests, TuneRequest)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xf6);

    ASSERT_EQ (received.size(), 1u);
    EXPECT_TRUE (packetEquals (makeSystemMessage (0, Status (SystemStatus::tuneRequest)), received[0]));
}

TEST (Midi1ByteStreamParserTests, SysExCompleteWithPackets)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xf0);
    parser.feed (0x7e);
    parser.feed (0xf7);

    ASSERT_EQ (received.size(), 1u);
    EXPECT_EQ (received[0].getType(), PacketType::data);
    EXPECT_EQ (received[0].getStatus() & 0xf0, uint8_t (DataStatus::sysex7Complete));
}

TEST (Midi1ByteStreamParserTests, SysExContinueWithPackets)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xf0);
    for (int i = 0; i < 7; ++i)
        parser.feed (0x01 + i);
    parser.feed (0xf7);

    ASSERT_GE (received.size(), 1u);
    EXPECT_EQ (received[0].getType(), PacketType::data);
}

TEST (Midi1ByteStreamParserTests, SysExWithCallback)
{
    std::vector<SysEx7> receivedSysEx;
    std::vector<UniversalPacket> receivedPackets;

    Midi1ByteStreamParser parser (
        [&] (UniversalPacket p)
    {
        receivedPackets.push_back (p);
    },
        [&] (const SysEx7& sysex)
    {
        receivedSysEx.push_back (sysex);
    });

    parser.feed (0xf0);
    parser.feed (0x7e);
    parser.feed (0x00);
    parser.feed (0x01);
    parser.feed (0x02);
    parser.feed (0xf7);

    ASSERT_EQ (receivedSysEx.size(), 1u);
    EXPECT_EQ (receivedSysEx[0].manufacturerId, 0x7e0000u);
    EXPECT_EQ (receivedSysEx[0].data.size(), 3u);
}

TEST (Midi1ByteStreamParserTests, SysExWithCallbackThreeByteManufacturerId)
{
    std::vector<SysEx7> receivedSysEx;

    Midi1ByteStreamParser parser (
        [] (UniversalPacket) {},
        [&] (const SysEx7& sysex)
    {
        receivedSysEx.push_back (sysex);
    });

    parser.feed (0xf0);
    parser.feed (0x00);
    parser.feed (0x01);
    parser.feed (0x02);
    parser.feed (0x03);
    parser.feed (0xf7);

    ASSERT_EQ (receivedSysEx.size(), 1u);
    EXPECT_EQ (receivedSysEx[0].manufacturerId, 0x000102u);
    EXPECT_EQ (receivedSysEx[0].data.size(), 1u);
    EXPECT_EQ (receivedSysEx[0].data[0], 0x03);
}

TEST (Midi1ByteStreamParserTests, SysExInterruptedByStatus)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0xf0);
    parser.feed (0x7e);
    parser.feed (0x90);
    parser.feed (0x3c);
    parser.feed (0x7f);

    EXPECT_GT (received.size(), 0u);
    auto lastPacket = received.back();
    EXPECT_EQ (lastPacket.getType(), PacketType::midi1ChannelVoice);
}

TEST (Midi1ByteStreamParserTests, ResetClearsState)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser ([&] (UniversalPacket p)
    {
        received.push_back (p);
    });

    parser.feed (0x90);
    parser.feed (0x3c);
    parser.reset();

    parser.feed (0x7f);

    EXPECT_TRUE (received.empty());
}

TEST (Midi1ByteStreamParserTests, CallbacksDisabled)
{
    std::vector<UniversalPacket> received;
    Midi1ByteStreamParser parser (
        [&] (UniversalPacket p)
    {
        received.push_back (p);
    },
        {},
        false);

    parser.feed (0xf8);
    parser.feed (0x90);
    parser.feed (0x3c);
    parser.feed (0x7f);

    EXPECT_TRUE (received.empty());
}

TEST (Midi1ByteStreamParserTests, GetMidi1ByteStreamSizeEdgeCases)
{
    EXPECT_EQ (getMidi1ByteStreamSize (UniversalPacket {}), 0u);

    auto invalidData = UniversalPacket {};
    invalidData.setByte (1, 0x37);
    EXPECT_EQ (getMidi1ByteStreamSize (invalidData), 0u);

    auto sysexPacket = makeSysEx7StartPacket (0);
    sysexPacket.setByte (1, uint8_t (DataStatus::sysex7Complete) + 5);
    EXPECT_EQ (getMidi1ByteStreamSize (sysexPacket), 5u);
}

TEST (Midi1ByteStreamParserTests, FromMidi1ByteStreamInvalidStatus)
{
    auto packet = fromMidi1ByteStream (0x7f, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::utility);

    packet = fromMidi1ByteStream (0xf0, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::utility);

    packet = fromMidi1ByteStream (0xf7, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::utility);

    packet = fromMidi1ByteStream (0xf9, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::utility);

    packet = fromMidi1ByteStream (0xfd, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::utility);
}

TEST (Midi1ByteStreamParserTests, FromMidi1ByteStreamValidSystemAndChannel)
{
    auto packet = fromMidi1ByteStream (0xf8, 0, 0);
    EXPECT_EQ (packet.getType(), PacketType::system);
    EXPECT_EQ (packet.getStatus(), uint8_t (SystemStatus::clock));

    packet = fromMidi1ByteStream (0x90, 0x3c, 0x7f);
    EXPECT_EQ (packet.getType(), PacketType::midi1ChannelVoice);
    EXPECT_EQ (packet.getStatus(), 0x90);
}

TEST (Midi1ByteStreamParserTests, ToMidi1ByteStreamSysExPackets)
{
    uint8_t result[8] = {};

    auto completePacket = makeSysEx7CompletePacket (0);
    completePacket.addPayloadByte (0x7e);
    completePacket.addPayloadByte (0x00);
    completePacket.addPayloadByte (0x01);
    auto size = toMidi1ByteStream (completePacket, result);
    ASSERT_EQ (size, 5u);
    EXPECT_EQ (result[0], 0xf0);
    EXPECT_EQ (result[4], 0xf7);

    std::fill (std::begin (result), std::end (result), 0);
    auto startPacket = makeSysEx7StartPacket (0);
    startPacket.addPayloadByte (0x7e);
    startPacket.addPayloadByte (0x00);
    size = toMidi1ByteStream (startPacket, result);
    ASSERT_EQ (size, 3u);
    EXPECT_EQ (result[0], 0xf0);

    std::fill (std::begin (result), std::end (result), 0);
    auto endPacket = makeSysEx7EndPacket (0);
    endPacket.addPayloadByte (0x01);
    size = toMidi1ByteStream (endPacket, result);
    ASSERT_EQ (size, 2u);
    EXPECT_EQ (result[1], 0xf7);
}

TEST (Midi1ByteStreamParserTests, ToMidi1ByteStreamUnsupportedType)
{
    uint8_t result[8] = {};

    auto utilityPacket = UniversalPacket {};
    utilityPacket.setByte (0, uint8_t (PacketType::utility) << 4);
    auto size = toMidi1ByteStream (utilityPacket, result);
    EXPECT_EQ (size, 0u);
}
