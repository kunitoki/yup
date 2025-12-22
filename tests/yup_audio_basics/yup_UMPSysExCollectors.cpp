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
#include <vector>

using namespace yup;
using namespace yup::ump;

namespace
{
struct SysEx7TestCase
{
    std::string description;
    std::vector<UniversalPacket> packets;
    SysEx7 sysex;
};

struct SysEx8TestCase
{
    std::string description;
    std::vector<UniversalPacket> packets;
    uint8_t streamId;
    SysEx8 sysex;
};

const std::vector<SysEx7TestCase> sysEx7TestCases {
    { "empty sysex",
      { UniversalPacket { 0x30010000u, 0x00000000u } },
      SysEx7 {} },
    { "one byte manufacturer only",
      { UniversalPacket { 0x31010400u, 0x00000000u } },
      SysEx7 { Manufacturer::moog } },
    { "three byte manufacturer only",
      { UniversalPacket { 0x35030021u, 0x09000000u } },
      SysEx7 { Manufacturer::nativeInstruments } },
    { "three byte manufacturer, complete message",
      { UniversalPacket { 0x31050002u, 0x0d152600u } },
      SysEx7 { Manufacturer::google, { 0x15, 0x26 } } },
    { "one byte manufacturer, complete message",
      { UniversalPacket { 0x3f060905u, 0x04030201u } },
      SysEx7 { Manufacturer::midi9, { 5, 4, 3, 2, 1 } } },
    { "two messages",
      { UniversalPacket { 0x3a164e09u, 0x08070605u },
        UniversalPacket { 0x3a340403u, 0x02010000u } },
      SysEx7 { Manufacturer::teac, { 9, 8, 7, 6, 5, 4, 3, 2, 1 } } },
    { "three messages",
      { UniversalPacket { 0x36160021u, 0x1d112233u },
        UniversalPacket { 0x36264455u, 0x66771829u },
        UniversalPacket { 0x36353a4bu, 0x5c6d7c00u } },
      SysEx7 { Manufacturer::ableton,
               { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7c } } },
    { "four messages",
      { UniversalPacket { 0x31160020u, 0x6b112233u },
        UniversalPacket { 0x31264455u, 0x66771829u },
        UniversalPacket { 0x31263a4bu, 0x5c6d7c0fu },
        UniversalPacket { 0x31311000u, 0x00000000u } },
      SysEx7 { Manufacturer::arturia,
               { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7c, 0x0f, 0x10 } } }
};

const std::vector<SysEx8TestCase> sysEx8TestCases {
    { "empty sysex",
      { UniversalPacket { 0x50034400u, 0x00000000u } },
      0x44,
      SysEx8 {} },
    { "one byte manufacturer only",
      { UniversalPacket { 0x51033400u, 0x04000000u } },
      0x34,
      SysEx8 { Manufacturer::moog } },
    { "three byte manufacturer only",
      { UniversalPacket { 0x550300a1u, 0x09000000u } },
      0x00,
      SysEx8 { Manufacturer::nativeInstruments } },
    { "three byte manufacturer, complete packet",
      { UniversalPacket { 0x5106aa82u, 0x0d85a600u } },
      0xaa,
      SysEx8 { Manufacturer::google, { 0x85, 0xa6, 0x00 } } },
    { "one byte manufacturer, complete packet",
      { UniversalPacket { 0x5f089300u, 0x09f5e4d3u, 0xc2b10000u, 0 } },
      0x93,
      SysEx8 { Manufacturer::midi9, { 0xf5, 0xe4, 0xd3, 0xc2, 0xb1 } } },
    { "two packets",
      { UniversalPacket { 0x5a1e3900u, 0x4e090807u, 0x06050403u, 0x020100ffu },
        UniversalPacket { 0x5a3639eeu, 0xddccbbaau } },
      0x39,
      SysEx8 { Manufacturer::teac, { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa } } },
    { "three packets",
      { UniversalPacket { 0x561ec3a1u, 0x1d112233u, 0x44556677u, 0x18293a4bu },
        UniversalPacket { 0x562ec35cu, 0x6d7c8b9au, 0xa9b8c7d6u, 0xe5f40312u },
        UniversalPacket { 0x5632c331u, 0 } },
      0xc3,
      SysEx8 { Manufacturer::ableton,
               { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7c, 0x8b, 0x9a, 0xa9, 0xb8, 0xc7, 0xd6, 0xe5, 0xf4, 0x03, 0x12, 0x31 } } },
    { "lots of data",
      { UniversalPacket { 0x511e83a1u, 0x09112233u, 0x44556677u, 0x8899aabbu },
        UniversalPacket { 0x512e83ccu, 0xddeeff18u, 0x293a4b5cu, 0x6d7e8f90u },
        UniversalPacket { 0x512e83a1u, 0xb2c3d4e5u, 0xf6e7d8f0u, 0xe1d2c3b4u },
        UniversalPacket { 0x512e83a5u, 0x96877869u, 0x5a4b3c2du, 0x1e0f420fu },
        UniversalPacket { 0x51328310u, 0x00000000u } },
      0x83,
      SysEx8 { Manufacturer::nativeInstruments,
               { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x18, 0x29, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x90, 0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0xe7, 0xd8, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f, 0x42, 0x0f, 0x10 } } }
};
} // namespace

TEST (SysEx7CollectorTests, CollectsPacketSequences)
{
    for (const auto& entry : sysEx7TestCases)
    {
        bool outputGenerated = false;
        SysEx7Collector collector ([&] (const SysEx7& sx)
        {
            outputGenerated = true;
            EXPECT_EQ (sx, entry.sysex) << entry.description;
        });

        for (const auto& packet : entry.packets)
            collector.feed (packet);

        EXPECT_TRUE (outputGenerated) << entry.description;
    }
}

TEST (SysEx7CollectorTests, RoundTripPacketization)
{
    const SysEx7 sysex { Manufacturer::nativeInstruments, { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 } };
    const auto packets = asSysEx7Packets (sysex);

    bool outputGenerated = false;
    SysEx7Collector collector ([&] (const SysEx7& sx)
    {
        outputGenerated = true;
        EXPECT_EQ (sx, sysex);
    });

    for (const auto& packet : packets)
        collector.feed (packet);

    EXPECT_TRUE (outputGenerated);
}

TEST (SysEx8CollectorTests, CollectsPacketSequences)
{
    for (const auto& entry : sysEx8TestCases)
    {
        bool outputGenerated = false;
        SysEx8Collector collector ([&] (const SysEx8& sx, uint8_t streamId)
        {
            outputGenerated = true;
            EXPECT_EQ (sx, entry.sysex) << entry.description;
            EXPECT_EQ (streamId, entry.streamId) << entry.description;
        });

        for (const auto& packet : entry.packets)
            collector.feed (packet);

        EXPECT_TRUE (outputGenerated) << entry.description;
    }
}

TEST (SysEx8CollectorTests, RoundTripPacketization)
{
    const SysEx8 sysex { Manufacturer::google, { 0x11, 0x22, 0x33, 0x44, 0x55 } };
    const uint8_t streamId = 0x7f;
    const auto packets = asSysEx8Packets (sysex, streamId);

    bool outputGenerated = false;
    SysEx8Collector collector ([&] (const SysEx8& sx, uint8_t stream)
    {
        outputGenerated = true;
        EXPECT_EQ (sx, sysex);
        EXPECT_EQ (stream, streamId);
    });

    for (const auto& packet : packets)
        collector.feed (packet);

    EXPECT_TRUE (outputGenerated);
}

TEST (SysEx7CollectorTests, HonorsMaxSize)
{
    bool outputGenerated = false;
    SysEx7Collector collector ([&] (const SysEx7&)
    {
        outputGenerated = true;
    });
    collector.setMaxSysExDataSize (12);

    const std::vector<UniversalPacket> packets {
        UniversalPacket { 0x30167e12u, 0x34567809u },
        UniversalPacket { 0x30261a2bu, 0x3c4d5e6fu },
        UniversalPacket { 0x30320000u, 0 }
    };

    for (const auto& packet : packets)
        collector.feed (packet);

    EXPECT_FALSE (outputGenerated);
}

TEST (SysEx8CollectorTests, StreamIdSwitchTriggersNewMessage)
{
    bool outputGenerated = false;
    SysEx8Collector collector ([&] (const SysEx8&, uint8_t)
    {
        outputGenerated = true;
    });

    collector.feed (UniversalPacket { 0x55130f80u, 0x3b000000u });
    EXPECT_EQ (collector.getStreamId(), 0x0fu);

    collector.feed (UniversalPacket { 0x55310f44u, 0x00000000u });
    EXPECT_TRUE (outputGenerated);
}
