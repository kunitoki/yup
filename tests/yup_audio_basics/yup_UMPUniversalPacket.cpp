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

#include <sstream>
#include <string>

using namespace yup;
using namespace yup::ump;

TEST (UMPUniversalPacketTests, Constructors)
{
    {
        UniversalPacket p;

        EXPECT_EQ (p.data[0], 0u);
        EXPECT_EQ (p.data[1], 0u);
        EXPECT_EQ (p.data[2], 0u);
        EXPECT_EQ (p.data[3], 0u);

        EXPECT_EQ (p.getSize(), 1u);
        EXPECT_EQ (p.getType(), PacketType::utility);
        EXPECT_EQ (p.getGroup(), 0u);
        EXPECT_EQ (p.getStatus(), 0u);
        EXPECT_EQ (p.getByte2(), 0u);
        EXPECT_EQ (p.getByte3(), 0u);
        EXPECT_EQ (p.getByte4(), 0u);
    }

    {
        UniversalPacket p { 0x12345678u };
        EXPECT_EQ (p.data[0], 0x12345678u);
        EXPECT_EQ (p.data[1], 0u);
        EXPECT_EQ (p.data[2], 0u);
        EXPECT_EQ (p.data[3], 0u);
    }

    {
        UniversalPacket p { 1933467u };
        EXPECT_EQ (p.data[0], 1933467u);
        EXPECT_EQ (p.data[1], 0u);
        EXPECT_EQ (p.data[2], 0u);
        EXPECT_EQ (p.data[3], 0u);
    }

    {
        UniversalPacket p { 0x21004567u, 0x145938aau };
        EXPECT_EQ (p.data[0], 0x21004567u);
        EXPECT_EQ (p.data[1], 0x145938aau);
        EXPECT_EQ (p.data[2], 0u);
        EXPECT_EQ (p.data[3], 0u);
    }

    {
        UniversalPacket p { 0xbc352890u, 0x9d2a445cu, 77777u };
        EXPECT_EQ (p.data[0], 0xbc352890u);
        EXPECT_EQ (p.data[1], 0x9d2a445cu);
        EXPECT_EQ (p.data[2], 77777u);
        EXPECT_EQ (p.data[3], 0u);
    }

    {
        UniversalPacket p { 0x54abcdefu, 0x12345678u, 0xabadbabeu, 0xcdcdcdcdu };
        EXPECT_EQ (p.data[0], 0x54abcdefu);
        EXPECT_EQ (p.data[1], 0x12345678u);
        EXPECT_EQ (p.data[2], 0xabadbabeu);
        EXPECT_EQ (p.data[3], 0xcdcdcdcdu);
    }
}

TEST (UMPUniversalPacketTests, Equality)
{
    {
        UniversalPacket p1;
        UniversalPacket p2 { 0x12345678u };
        UniversalPacket p3 { 1933467u };
        UniversalPacket p4 { 0x31004567u, 0x145938aau };
        UniversalPacket p5 { 0x12345678u };

        EXPECT_EQ (p1, p1);
        EXPECT_NE (p1, p2);
        EXPECT_NE (p1, p3);
        EXPECT_NE (p1, p4);
        EXPECT_NE (p1, p5);

        EXPECT_NE (p2, p1);
        EXPECT_EQ (p2, p2);
        EXPECT_NE (p2, p3);
        EXPECT_NE (p2, p4);
        EXPECT_EQ (p2, p5);

        EXPECT_NE (p3, p1);
        EXPECT_NE (p3, p2);
        EXPECT_EQ (p3, p3);
        EXPECT_NE (p3, p4);
        EXPECT_NE (p3, p5);

        EXPECT_NE (p4, p1);
        EXPECT_NE (p4, p2);
        EXPECT_NE (p4, p3);
        EXPECT_EQ (p4, p4);
        EXPECT_NE (p4, p5);

        EXPECT_NE (p5, p1);
        EXPECT_EQ (p5, p2);
        EXPECT_NE (p5, p3);
        EXPECT_NE (p5, p4);
        EXPECT_EQ (p5, p5);
    }

    {
        UniversalPacket p1 { 0xba345678u, 99u, 55u, 42u };
        UniversalPacket p2 { 0xba345678u, 99u, 55u, 43u };
        UniversalPacket p3 { 0xba345679u, 99u, 55u, 42u };
        UniversalPacket p4 { 0xba345678u, 98u, 55u, 42u };
        UniversalPacket p5 { 0xba345678u, 99u, 56u, 42u };

        EXPECT_EQ (p1, p1);
        EXPECT_EQ (p1, p2);
        EXPECT_NE (p1, p3);
        EXPECT_NE (p1, p4);
        EXPECT_NE (p1, p5);

        EXPECT_EQ (p2, p1);
        EXPECT_EQ (p2, p2);
        EXPECT_NE (p2, p3);
        EXPECT_NE (p2, p4);
        EXPECT_NE (p2, p5);

        EXPECT_NE (p3, p1);
        EXPECT_NE (p3, p2);
        EXPECT_EQ (p3, p3);
        EXPECT_NE (p3, p4);
        EXPECT_NE (p3, p5);

        EXPECT_NE (p4, p1);
        EXPECT_NE (p4, p2);
        EXPECT_NE (p4, p3);
        EXPECT_EQ (p4, p4);
        EXPECT_NE (p4, p5);

        EXPECT_NE (p5, p1);
        EXPECT_NE (p5, p2);
        EXPECT_NE (p5, p3);
        EXPECT_NE (p5, p4);
        EXPECT_EQ (p5, p5);
    }
}

TEST (UMPUniversalPacketTests, EqualityIgnoresUnusedWords)
{
    UniversalPacket p1 { 0x12345678u, 1000 };
    UniversalPacket p2 { 0x12345678u, 1001 };
    UniversalPacket px { 0x12345677u, 1000 };

    EXPECT_EQ (p1, p2);
    EXPECT_NE (p1, px);

    UniversalPacket p3 { 0x31004567u, 0x145938aau, 2939 };
    UniversalPacket p4 { 0x31004567u, 0x145938aau, 55 };
    UniversalPacket py { 0x31004568u, 0x145938aau, 2939 };
    UniversalPacket pz { 0x31004567u, 0x5938aa12u, 2939 };

    EXPECT_EQ (p3, p4);
    EXPECT_NE (p3, py);
    EXPECT_NE (p3, pz);
}

TEST (UMPUniversalPacketTests, ByteAccess)
{
    UniversalPacket p { 0x11223344u, 0x55667788u, 0x99aabbccu, 0xddeeff00u };

    EXPECT_EQ (p.getByte (0), 0x11u);
    EXPECT_EQ (p.getByte (3), 0x44u);
    EXPECT_EQ (p.getByte (4), 0x55u);
    EXPECT_EQ (p.getByte (15), 0x00u);

    p.setByte (0, 0xaa);
    p.setByte (15, 0xbb);

    EXPECT_EQ (p.getByte (0), 0xaau);
    EXPECT_EQ (p.getByte (15), 0xbbu);
}

TEST (UMPUniversalPacketTests, Type)
{
    UniversalPacket p0 { 0x011f1234u };
    EXPECT_EQ (p0.getType(), PacketType::utility);

    UniversalPacket p1 { 0x15f12345u };
    EXPECT_EQ (p1.getType(), PacketType::system);

    UniversalPacket p2 { 0x2cbf8765u };
    EXPECT_EQ (p2.getType(), PacketType::midi1ChannelVoice);

    UniversalPacket p3 { 0x34317f03u, 0x12345678u };
    EXPECT_EQ (p3.getType(), PacketType::data);

    UniversalPacket p4 { 0x41937788u, 0x81114080u };
    EXPECT_EQ (p4.getType(), PacketType::midi2ChannelVoice);

    UniversalPacket p5 { 0x59937788u, 0x11111111u, 0x22222222u, 0x22222222u };
    EXPECT_EQ (p5.getType(), PacketType::extendedData);

    p0.setType (PacketType::midi1ChannelVoice);
    EXPECT_EQ (p0.getType(), PacketType::midi1ChannelVoice);

    p5.setType (PacketType::midi2ChannelVoice);
    EXPECT_EQ (p5.getType(), PacketType::midi2ChannelVoice);
}

TEST (UMPUniversalPacketTests, Size)
{
    EXPECT_EQ (UniversalPacket { 0x011f1234u }.getSize(), 1u);
    EXPECT_EQ (UniversalPacket { 0x15f12345u }.getSize(), 1u);
    EXPECT_EQ (UniversalPacket { 0x2cbf8765u }.getSize(), 1u);
    EXPECT_EQ ((UniversalPacket { 0x34317f03u, 0x12345678u }.getSize()), 2u);
    EXPECT_EQ ((UniversalPacket { 0x41937788u, 0x81114080u }.getSize()), 2u);
    EXPECT_EQ ((UniversalPacket { 0x59937788u, 0x11111111u, 0x22222222u, 0x22222222u }.getSize()), 4u);

    EXPECT_EQ (UniversalPacket { 0x60000000u }.getSize(), 1u);
    EXPECT_EQ (UniversalPacket { 0x70000000u }.getSize(), 1u);
    EXPECT_EQ (UniversalPacket { 0x80000000u }.getSize(), 2u);
    EXPECT_EQ (UniversalPacket { 0x90000000u }.getSize(), 2u);
    EXPECT_EQ (UniversalPacket { 0xa0000000u }.getSize(), 2u);
    EXPECT_EQ (UniversalPacket { 0xb0000000u }.getSize(), 3u);
    EXPECT_EQ (UniversalPacket { 0xc0000000u }.getSize(), 3u);
    EXPECT_EQ (UniversalPacket { 0xd0000000u }.getSize(), 4u);
    EXPECT_EQ (UniversalPacket { 0xe0000000u }.getSize(), 4u);
    EXPECT_EQ (UniversalPacket { 0xf0000000u }.getSize(), 4u);
}

TEST (UMPUniversalPacketTests, Group)
{
    UniversalPacket p0 { 0x011f1234u };
    EXPECT_EQ (p0.getGroup(), 1u);

    UniversalPacket p1 { 0x15f12345u };
    EXPECT_EQ (p1.getGroup(), 5u);

    UniversalPacket p2 { 0x2cbf8765u };
    EXPECT_EQ (p2.getGroup(), 12u);

    UniversalPacket p3 { 0x34317f03u, 0x12345678u };
    EXPECT_EQ (p3.getGroup(), 4u);

    UniversalPacket p4 { 0x41937788u, 0x81114080u };
    EXPECT_EQ (p4.getGroup(), 1u);

    UniversalPacket p5 { 0x59937788u, 0x11111111u, 0x22222222u, 0x22222222u };
    EXPECT_EQ (p5.getGroup(), 9u);

    p1.setGroup (4);
    EXPECT_EQ (p1.getGroup(), 4u);

    p3.setGroup (0x0f);
    EXPECT_EQ (p3.getGroup(), 15u);
}

TEST (UMPUniversalPacketTests, Status)
{
    UniversalPacket p0 { 0x011f1234u };
    EXPECT_EQ (p0.getStatus(), 0x1fu);

    UniversalPacket p1 { 0x15f12345u };
    EXPECT_EQ (p1.getStatus(), 0xf1u);

    UniversalPacket p2 { 0x2cbf8765u };
    EXPECT_EQ (p2.getStatus(), 0xbfu);

    UniversalPacket p3 { 0x34317f03u, 0x12345678u };
    EXPECT_EQ (p3.getStatus(), 0x31u);

    UniversalPacket p4 { 0x41937788u, 0x81114080u };
    EXPECT_EQ (p4.getStatus(), 0x93u);

    UniversalPacket p5 { 0x599a7788u, 0x11111111u, 0x22222222u, 0x22222222u };
    EXPECT_EQ (p5.getStatus(), 0x9au);

    p2.setByte (1, 0xabu);
    EXPECT_EQ (p2.getStatus(), 0xabu);

    p4.setByte (1, 0x01u);
    EXPECT_EQ (p4.getStatus(), 0x01u);
}

TEST (UMPUniversalPacketTests, Bytes)
{
    UniversalPacket p { 0x599a7788u, 0x4f95a278u, 0x34317f03u, 0x011f1234u };

    EXPECT_EQ (p.getByte (0), 0x59u);
    EXPECT_EQ (p.getByte (1), 0x9au);
    EXPECT_EQ (p.getByte2(), 0x9au);
    EXPECT_EQ (p.getByte (2), 0x77u);
    EXPECT_EQ (p.getByte3(), 0x77u);
    EXPECT_EQ (p.getByte (3), 0x88u);
    EXPECT_EQ (p.getByte4(), 0x88u);

    EXPECT_EQ (p.getByte (4), 0x4fu);
    EXPECT_EQ (p.getByte (5), 0x95u);
    EXPECT_EQ (p.getByte (6), 0xa2u);
    EXPECT_EQ (p.getByte (7), 0x78u);

    EXPECT_EQ (p.getByte (8), 0x34u);
    EXPECT_EQ (p.getByte (9), 0x31u);
    EXPECT_EQ (p.getByte (10), 0x7fu);
    EXPECT_EQ (p.getByte (11), 0x03u);

    EXPECT_EQ (p.getByte (12), 0x01u);
    EXPECT_EQ (p.getByte (13), 0x1fu);
    EXPECT_EQ (p.getByte (14), 0x12u);
    EXPECT_EQ (p.getByte (15), 0x34u);

    for (uint8_t i = 0; i < 16; ++i)
    {
        const auto sevenBits = uint7_t (p.getByte (i) & 0x7f);
        EXPECT_EQ (p.getByte7Bit (i), sevenBits);

        p.setByte (i, uint8_t (i + 1));
        EXPECT_EQ (p.getByte (i), uint8_t (i + 1));

        p.setByte7Bit (i, uint8_t (0x80u + i));
        EXPECT_EQ (p.getByte (i), i);
    }
}

TEST (UMPUniversalPacketTests, Channel)
{
    UniversalPacket p0 { 0x011f1234u };
    EXPECT_FALSE (p0.hasChannel());

    UniversalPacket p1 { 0x15f12345u };
    EXPECT_FALSE (p1.hasChannel());

    UniversalPacket p2 { 0x2cbf8765u };
    EXPECT_TRUE (p2.hasChannel());
    EXPECT_EQ (p2.getChannel(), 15u);

    UniversalPacket p3 { 0x34317f03u, 0x12345678u };
    EXPECT_FALSE (p3.hasChannel());

    UniversalPacket p4 { 0x41937788u, 0x81114080u };
    EXPECT_TRUE (p4.hasChannel());
    EXPECT_EQ (p4.getChannel(), 3u);

    UniversalPacket p5 { 0xd99a7788u, 0x11111111u, 0x22222222u, 0x22222222u };
    EXPECT_TRUE (p5.hasChannel());
    EXPECT_EQ (p5.getChannel(), 10u);

    p2.setByte (1, 0x11u);
    EXPECT_EQ (p2.getChannel(), 1u);

    p4.setByte (1, 0xdeu);
    EXPECT_EQ (p4.getChannel(), 14u);

    EXPECT_FALSE (UniversalPacket { 0x60000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0x70000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0x80000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0x90000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0xa0000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0xb0000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0xc0000000u }.hasChannel());
    EXPECT_TRUE (UniversalPacket { 0xd0000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0xe0000000u }.hasChannel());
    EXPECT_FALSE (UniversalPacket { 0xf0000000u }.hasChannel());
}

TEST (UMPUniversalPacketTests, Reset)
{
    UniversalPacket p { 0x50123456u, 0x789abcdeu, 0xfedcba98u, 0x76543210u };

    p.reset();

    EXPECT_EQ (p.data[0], 0u);
    EXPECT_EQ (p.data[1], 0u);
    EXPECT_EQ (p.data[2], 0u);
    EXPECT_EQ (p.data[3], 0u);
}

TEST (UMPUniversalPacketTests, IsUtilityMessage)
{
    for (uint32_t t = 0; t < 0x10; ++t)
    {
        UniversalPacket p { t << 28u };
        EXPECT_EQ (p.isUtilityMessage(), t == 0);
    }
}

TEST (UMPUniversalPacketTests, IsSystemMessage)
{
    for (uint32_t t = 0; t < 0x10; ++t)
    {
        UniversalPacket p { t << 28u };
        EXPECT_EQ (p.isSystemMessage(), t == 1);
    }
}

TEST (UMPUniversalPacketTests, IsChannelVoiceMessage)
{
    for (uint32_t t = 0; t < 0x10; ++t)
    {
        UniversalPacket p { t << 28u };
        EXPECT_EQ (p.isChannelVoiceMessage(), t == 2 || t == 4);
    }
}

TEST (UMPUniversalPacketTests, IsDataMessage)
{
    for (uint32_t t = 0; t < 0x10; ++t)
    {
        UniversalPacket p { t << 28u };
        EXPECT_EQ (p.isDataMessage(), t == 3 || t == 5);
    }
}

TEST (UMPUniversalPacketTests, IsMidi1ProtocolMessage)
{
    for (uint32_t t = 0; t < 0x10; ++t)
    {
        for (uint32_t s = 0; s < 0xff; ++s)
        {
            UniversalPacket p { (t << 28u) | (s << 16u) };

            bool expected = false;
            if (t == 1)
            {
                if (p.getStatus() > 0xf0)
                {
                    switch (p.getStatus())
                    {
                        case 0xf4:
                        case 0xf5:
                        case 0xf7:
                        case 0xf9:
                        case 0xfd:
                            expected = false;
                            break;
                        default:
                            expected = true;
                            break;
                    }
                }
            }
            else if (t == 2)
            {
                expected = p.getStatus() >= 0x80 && p.getStatus() < 0xf0;
            }

            EXPECT_EQ (p.isMidi1ProtocolMessage(), expected);
        }
    }
}

TEST (UMPUniversalPacketTests, StreamOperators)
{
    UniversalPacket p { 0x50123456u, 0x789abcdeu, 0xfedcba98u, 0x76543210u };

    {
        std::stringstream stream;
        stream << p << '\n';

        UniversalPacket out;
        stream >> out;
        EXPECT_EQ (out, p);
        EXPECT_TRUE (stream.good());
    }

    {
        std::stringstream stream;
        stream << p;

        std::string w1, w2, w3, w4;
        stream >> w1 >> w2 >> w3 >> w4;
        EXPECT_EQ (w1, "50123456");
        EXPECT_EQ (w2, "789abcde");
        EXPECT_EQ (w3, "fedcba98");
        EXPECT_EQ (w4, "76543210");
    }

    {
        std::stringstream stream;
        stream << "invaliddata";

        UniversalPacket out;
        stream >> out;
        EXPECT_FALSE (stream.good());
        EXPECT_EQ (out, UniversalPacket {});
    }

    {
        std::stringstream stream;
        stream << "50123456p1";

        UniversalPacket out;
        stream >> out;
        EXPECT_FALSE (stream.good());
        EXPECT_EQ (out, UniversalPacket { 0x50123456u });
    }
}
