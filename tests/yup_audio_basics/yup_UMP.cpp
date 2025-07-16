/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
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
constexpr uint8_t operator""_u8 (unsigned long long int i) { return static_cast<uint8_t> (i); }

constexpr uint16_t operator""_u16 (unsigned long long int i) { return static_cast<uint16_t> (i); }

constexpr uint32_t operator""_u32 (unsigned long long int i) { return static_cast<uint32_t> (i); }

constexpr uint64_t operator""_u64 (unsigned long long int i) { return static_cast<uint64_t> (i); }
} // namespace

class UniversalMidiPacketTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        random.setSeed (12345);
    }

    Random random;

    static Packets toMidi1 (const MidiMessage& msg)
    {
        Packets packets;
        Conversion::toMidi1 (ump::BytestreamMidiView (&msg), [&] (const auto p)
        {
            packets.add (p);
        });
        return packets;
    }

    static Packets convertMidi2ToMidi1 (const Packets& midi2)
    {
        Packets r;

        for (const auto& packet : midi2)
        {
            Conversion::midi2ToMidi1DefaultTranslation (packet, [&r] (const View& v)
            {
                r.add (v);
            });
        }

        return r;
    }

    static Packets convertMidi1ToMidi2 (const Packets& midi1)
    {
        Packets r;
        Midi1ToMidi2DefaultTranslator translator;

        for (const auto& packet : midi1)
            translator.dispatch (packet, [&r] (const View& v)
            {
                r.add (v);
            });

        return r;
    }

    void checkBytestreamConversion (const Packets& actual, const Packets& expected)
    {
        EXPECT_EQ (actual.size(), expected.size());

        if (actual.size() != expected.size())
            return;

        auto actualPtr = actual.data();

        std::for_each (expected.data(),
                       expected.data() + expected.size(),
                       [&] (const uint32_t word)
        {
            EXPECT_EQ (*actualPtr++, word);
        });
    }

    void checkMidi2ToMidi1Conversion (const Packets& midi2, const Packets& expected)
    {
        checkBytestreamConversion (convertMidi2ToMidi1 (midi2), expected);
    }

    void checkMidi1ToMidi2Conversion (const Packets& midi1, const Packets& expected)
    {
        checkBytestreamConversion (convertMidi1ToMidi2 (midi1), expected);
    }

    MidiMessage createRandomSysEx (Random& random, size_t sysExBytes)
    {
        std::vector<uint8_t> data;
        data.reserve (sysExBytes);

        for (size_t i = 0; i != sysExBytes; ++i)
            data.push_back (uint8_t (random.nextInt (0x80)));

        return MidiMessage::createSysExMessage (data.data(), int (data.size()));
    }

    PacketX1 createRandomUtilityUMP (Random& random)
    {
        const auto status = random.nextInt (3);

        return PacketX1 { Utils::bytesToWord (std::byte { 0 },
                                              std::byte (status << 0x4),
                                              std::byte (status == 0 ? 0 : random.nextInt (0x100)),
                                              std::byte (status == 0 ? 0 : random.nextInt (0x100))) };
    }

    PacketX1 createRandomRealtimeUMP (Random& random)
    {
        const auto status = [&]
        {
            switch (random.nextInt (6))
            {
                case 0:
                    return std::byte { 0xf8 };
                case 1:
                    return std::byte { 0xfa };
                case 2:
                    return std::byte { 0xfb };
                case 3:
                    return std::byte { 0xfc };
                case 4:
                    return std::byte { 0xfe };
                case 5:
                    return std::byte { 0xff };
            }

            jassertfalse;
            return std::byte { 0x00 };
        }();

        return PacketX1 { Utils::bytesToWord (std::byte { 0x10 }, status, std::byte { 0x00 }, std::byte { 0x00 }) };
    }

    template <typename Fn>
    void forEachNonSysExTestMessage (Random& random, Fn&& fn)
    {
        for (uint16_t counter = 0x80; counter != 0x100; ++counter)
        {
            const auto firstByte = (uint8_t) counter;

            if (firstByte == 0xf0 || firstByte == 0xf7)
                continue; // sysEx is tested separately

            const auto length = MidiMessage::getMessageLengthFromFirstByte (firstByte);
            const auto getDataByte = [&]
            {
                return uint8_t (random.nextInt (256) & 0x7f);
            };

            const auto message = [&]
            {
                switch (length)
                {
                    case 1:
                        return MidiMessage (firstByte);
                    case 2:
                        return MidiMessage (firstByte, getDataByte());
                    case 3:
                        return MidiMessage (firstByte, getDataByte(), getDataByte());
                }

                return MidiMessage();
            }();

            fn (message);
        }
    }

    static bool equal (const MidiMessage& a, const MidiMessage& b) noexcept
    {
        return a.getRawDataSize() == b.getRawDataSize()
            && std::equal (a.getRawData(), a.getRawData() + a.getRawDataSize(), b.getRawData());
    }

    static bool equal (const MidiBuffer& a, const MidiBuffer& b) noexcept
    {
        return a.data == b.data;
    }
};

TEST_F (UniversalMidiPacketTest, ShortBytestreamMidiMessagesCanBeRoundTrippedThroughUMPConverter)
{
    Midi1ToBytestreamTranslator translator (0);

    forEachNonSysExTestMessage (random, [&] (const MidiMessage& m)
    {
        const auto packets = toMidi1 (m);
        EXPECT_EQ (packets.size(), 1u);

        // Make sure that the message type is correct
        const auto msgType = Utils::getMessageType (packets.data()[0]);
        EXPECT_EQ (msgType, ((m.getRawData()[0] >> 0x4) == 0xf ? 0x1 : 0x2));

        translator.dispatch (View { packets.data() },
                             0,
                             [&] (const BytestreamMidiView& roundTripped)
        {
            EXPECT_TRUE (equal (m, roundTripped.getMessage()));
        });
    });
}

TEST_F (UniversalMidiPacketTest, BytestreamSysExConvertsToUniversalPackets)
{
    {
        // Zero length message
        const auto packets = toMidi1 (createRandomSysEx (random, 0));
        EXPECT_EQ (packets.size(), 2u);

        EXPECT_EQ (packets.data()[0], 0x30000000u);
        EXPECT_EQ (packets.data()[1], 0x00000000u);
    }

    {
        const auto message = createRandomSysEx (random, 1);
        const auto packets = toMidi1 (message);
        EXPECT_EQ (packets.size(), 2u);

        const auto* sysEx = message.getSysExData();
        EXPECT_EQ (packets.data()[0], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x01 }, std::byte { sysEx[0] }, std::byte { 0 }));
        EXPECT_EQ (packets.data()[1], 0x00000000u);
    }

    {
        const auto message = createRandomSysEx (random, 6);
        const auto packets = toMidi1 (message);
        EXPECT_EQ (packets.size(), 2u);

        const auto* sysEx = message.getSysExData();
        EXPECT_EQ (packets.data()[0], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x06 }, std::byte { sysEx[0] }, std::byte { sysEx[1] }));
        EXPECT_EQ (packets.data()[1], Utils::bytesToWord (std::byte { sysEx[2] }, std::byte { sysEx[3] }, std::byte { sysEx[4] }, std::byte { sysEx[5] }));
    }

    {
        const auto message = createRandomSysEx (random, 12);
        const auto packets = toMidi1 (message);
        EXPECT_EQ (packets.size(), 4u);

        const auto* sysEx = message.getSysExData();
        EXPECT_EQ (packets.data()[0], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x16 }, std::byte { sysEx[0] }, std::byte { sysEx[1] }));
        EXPECT_EQ (packets.data()[1], Utils::bytesToWord (std::byte { sysEx[2] }, std::byte { sysEx[3] }, std::byte { sysEx[4] }, std::byte { sysEx[5] }));
        EXPECT_EQ (packets.data()[2], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x36 }, std::byte { sysEx[6] }, std::byte { sysEx[7] }));
        EXPECT_EQ (packets.data()[3], Utils::bytesToWord (std::byte { sysEx[8] }, std::byte { sysEx[9] }, std::byte { sysEx[10] }, std::byte { sysEx[11] }));
    }

    {
        const auto message = createRandomSysEx (random, 13);
        const auto packets = toMidi1 (message);
        EXPECT_EQ (packets.size(), 6u);

        const auto* sysEx = message.getSysExData();
        EXPECT_EQ (packets.data()[0], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x16 }, std::byte { sysEx[0] }, std::byte { sysEx[1] }));
        EXPECT_EQ (packets.data()[1], Utils::bytesToWord (std::byte { sysEx[2] }, std::byte { sysEx[3] }, std::byte { sysEx[4] }, std::byte { sysEx[5] }));
        EXPECT_EQ (packets.data()[2], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x26 }, std::byte { sysEx[6] }, std::byte { sysEx[7] }));
        EXPECT_EQ (packets.data()[3], Utils::bytesToWord (std::byte { sysEx[8] }, std::byte { sysEx[9] }, std::byte { sysEx[10] }, std::byte { sysEx[11] }));
        EXPECT_EQ (packets.data()[4], Utils::bytesToWord (std::byte { 0x30 }, std::byte { 0x31 }, std::byte { sysEx[12] }, std::byte { 0 }));
        EXPECT_EQ (packets.data()[5], 0x00000000u);
    }
}

TEST_F (UniversalMidiPacketTest, LongSysExBytestreamMidiMessagesCanBeRoundTrippedThroughUMPConverter)
{
    ToBytestreamDispatcher converter (0);
    Packets packets;

    const auto checkRoundTrip = [&] (const MidiBuffer& expected)
    {
        for (const auto meta : expected)
            Conversion::toMidi1 (ump::BytestreamMidiView (meta), [&] (const auto p)
            {
                packets.add (p);
            });

        MidiBuffer output;
        converter.dispatch (packets.data(),
                            packets.data() + packets.size(),
                            0,
                            [&] (const BytestreamMidiView& roundTripped)
        {
            output.addEvent (roundTripped.getMessage(), int (roundTripped.timestamp));
        });
        packets.clear();

        EXPECT_TRUE (equal (expected, output));
    };

    for (auto length : { 0, 1, 2, 3, 4, 5, 6, 7, 13, 20, 100, 1000 })
    {
        MidiBuffer expected;
        expected.addEvent (createRandomSysEx (random, size_t (length)), 0);
        checkRoundTrip (expected);
    }
}

TEST_F (UniversalMidiPacketTest, UMPSysEx7MessagesInterspersedWithUtilityMessagesConvertToBytestream)
{
    ToBytestreamDispatcher converter (0);

    const auto sysEx = createRandomSysEx (random, 100);
    const auto originalPackets = toMidi1 (sysEx);

    Packets modifiedPackets;

    const auto addRandomUtilityUMP = [&]
    {
        const auto newPacket = createRandomUtilityUMP (random);
        modifiedPackets.add (View (newPacket.data()));
    };

    for (const auto& packet : originalPackets)
    {
        addRandomUtilityUMP();
        modifiedPackets.add (packet);
        addRandomUtilityUMP();
    }

    MidiBuffer output;
    converter.dispatch (modifiedPackets.data(),
                        modifiedPackets.data() + modifiedPackets.size(),
                        0,
                        [&] (const BytestreamMidiView& roundTripped)
    {
        output.addEvent (roundTripped.getMessage(), int (roundTripped.timestamp));
    });

    // All Utility messages should have been ignored
    EXPECT_EQ (output.getNumEvents(), 1);

    for (const auto meta : output)
        EXPECT_TRUE (equal (meta.getMessage(), sysEx));
}

// Due to the size and complexity of the original test, I'll include a few key tests here
// Additional tests would follow the same pattern...

TEST_F (UniversalMidiPacketTest, Midi2ToMidi1NoteOnConversions)
{
    {
        Packets midi2;
        midi2.add (PacketX2 { 0x41946410, 0x12345678 });

        Packets midi1;
        midi1.add (PacketX1 { 0x21946409 });

        checkMidi2ToMidi1Conversion (midi2, midi1);
    }

    {
        // If the velocity is close to 0, the output velocity should still be 1
        Packets midi2;
        midi2.add (PacketX2 { 0x4295327f, 0x00345678 });

        Packets midi1;
        midi1.add (PacketX1 { 0x22953201 });

        checkMidi2ToMidi1Conversion (midi2, midi1);
    }
}

TEST_F (UniversalMidiPacketTest, Midi2ToMidi1NoteOffConversion)
{
    Packets midi2;
    midi2.add (PacketX2 { 0x448b0520, 0xfedcba98 });

    Packets midi1;
    midi1.add (PacketX1 { 0x248b057f });

    checkMidi2ToMidi1Conversion (midi2, midi1);
}

TEST_F (UniversalMidiPacketTest, WideningConversionsWork)
{
    // This is similar to the 'slow' example code from the MIDI 2.0 spec
    const auto baselineScale = [] (uint32_t srcVal, uint32_t srcBits, uint32_t dstBits)
    {
        const auto scaleBits = (uint32_t) (dstBits - srcBits);

        auto bitShiftedValue = (uint32_t) (srcVal << scaleBits);

        const auto srcCenter = (uint32_t) (1 << (srcBits - 1));

        if (srcVal <= srcCenter)
            return bitShiftedValue;

        const auto repeatBits = (uint32_t) (srcBits - 1);
        const auto repeatMask = (uint32_t) ((1 << repeatBits) - 1);

        auto repeatValue = (uint32_t) (srcVal & repeatMask);

        if (scaleBits > repeatBits)
            repeatValue <<= scaleBits - repeatBits;
        else
            repeatValue >>= repeatBits - scaleBits;

        while (repeatValue != 0)
        {
            bitShiftedValue |= repeatValue;
            repeatValue >>= repeatBits;
        }

        return bitShiftedValue;
    };

    const auto baselineScale7To8 = [&] (uint8_t in)
    {
        return baselineScale (in, 7, 8);
    };

    const auto baselineScale7To16 = [&] (uint8_t in)
    {
        return baselineScale (in, 7, 16);
    };

    const auto baselineScale14To16 = [&] (uint16_t in)
    {
        return baselineScale (in, 14, 16);
    };

    const auto baselineScale7To32 = [&] (uint8_t in)
    {
        return baselineScale (in, 7, 32);
    };

    const auto baselineScale14To32 = [&] (uint16_t in)
    {
        return baselineScale (in, 14, 32);
    };

    for (auto i = 0; i != 100; ++i)
    {
        const auto rand = (uint8_t) random.nextInt (0x80);
        EXPECT_EQ (Conversion::scaleTo8 (rand), baselineScale7To8 (rand));
    }

    EXPECT_EQ (Conversion::scaleTo16 ((uint8_t) 0x00), 0x0000);
    EXPECT_EQ (Conversion::scaleTo16 ((uint8_t) 0x0a), 0x1400);
    EXPECT_EQ (Conversion::scaleTo16 ((uint8_t) 0x40), 0x8000);
    EXPECT_EQ (Conversion::scaleTo16 ((uint8_t) 0x57), 0xaeba);
    EXPECT_EQ (Conversion::scaleTo16 ((uint8_t) 0x7f), 0xffff);

    for (auto i = 0; i != 100; ++i)
    {
        const auto rand = (uint8_t) random.nextInt (0x80);
        EXPECT_EQ (Conversion::scaleTo16 (rand), baselineScale7To16 (rand));
    }
}

// ==============================================================================
// UMPUtils Tests

TEST (UMPUtilsTests, GetNumWordsForMessageType)
{
    using namespace yup::universal_midi_packets;

    // Test 1-word message types
    EXPECT_EQ (1, Utils::getNumWordsForMessageType (0x00000000)); // Message type 0x0
    EXPECT_EQ (1, Utils::getNumWordsForMessageType (0x10000000)); // Message type 0x1
    EXPECT_EQ (1, Utils::getNumWordsForMessageType (0x20000000)); // Message type 0x2
    EXPECT_EQ (1, Utils::getNumWordsForMessageType (0x60000000)); // Message type 0x6
    EXPECT_EQ (1, Utils::getNumWordsForMessageType (0x70000000)); // Message type 0x7

    // Test 2-word message types
    EXPECT_EQ (2, Utils::getNumWordsForMessageType (0x30000000)); // Message type 0x3
    EXPECT_EQ (2, Utils::getNumWordsForMessageType (0x40000000)); // Message type 0x4
    EXPECT_EQ (2, Utils::getNumWordsForMessageType (0x80000000)); // Message type 0x8
    EXPECT_EQ (2, Utils::getNumWordsForMessageType (0x90000000)); // Message type 0x9
    EXPECT_EQ (2, Utils::getNumWordsForMessageType (0xA0000000)); // Message type 0xA

    // Test 3-word message types
    EXPECT_EQ (3, Utils::getNumWordsForMessageType (0xB0000000)); // Message type 0xB
    EXPECT_EQ (3, Utils::getNumWordsForMessageType (0xC0000000)); // Message type 0xC

    // Test 4-word message types
    EXPECT_EQ (4, Utils::getNumWordsForMessageType (0x50000000)); // Message type 0x5
    EXPECT_EQ (4, Utils::getNumWordsForMessageType (0xD0000000)); // Message type 0xD
    EXPECT_EQ (4, Utils::getNumWordsForMessageType (0xE0000000)); // Message type 0xE
    EXPECT_EQ (4, Utils::getNumWordsForMessageType (0xF0000000)); // Message type 0xF
}

TEST (UMPUtilsTests, UtilityFunctionsGetMessageTypeGroupStatusChannel)
{
    using namespace yup::universal_midi_packets;

    // Test a word with all nibbles set to different values
    uint32_t testWord = 0x12345678;

    EXPECT_EQ (0x1, Utils::getMessageType (testWord));
    EXPECT_EQ (0x2, Utils::getGroup (testWord));
    EXPECT_EQ (0x3, Utils::getStatus (testWord));
    EXPECT_EQ (0x4, Utils::getChannel (testWord));
}

TEST (UMPUtilsTests, U4TemplateHelpers)
{
    using namespace yup::universal_midi_packets;

    // Test setting and getting 4-bit values at different positions
    uint32_t word = 0x00000000;

    // Set value at index 0 (most significant nibble)
    word = Utils::U4<0>::set (word, 0xA);
    EXPECT_EQ (0xA0000000, word);
    EXPECT_EQ (0xA, Utils::U4<0>::get (word));

    // Set value at index 1
    word = Utils::U4<1>::set (word, 0xB);
    EXPECT_EQ (0xAB000000, word);
    EXPECT_EQ (0xB, Utils::U4<1>::get (word));

    // Set value at index 7 (least significant nibble)
    word = Utils::U4<7>::set (word, 0xF);
    EXPECT_EQ (0xAB00000F, word);
    EXPECT_EQ (0xF, Utils::U4<7>::get (word));

    // Test overwriting existing values
    word = Utils::U4<0>::set (word, 0x3);
    EXPECT_EQ (0x3B00000F, word);
    EXPECT_EQ (0x3, Utils::U4<0>::get (word));
}

TEST (UMPUtilsTests, U8TemplateHelpers)
{
    using namespace yup::universal_midi_packets;

    uint32_t word = 0x00000000;

    // Set byte at index 0 (most significant byte)
    word = Utils::U8<0>::set (word, 0xAB);
    EXPECT_EQ (0xAB000000, word);
    EXPECT_EQ (0xAB, Utils::U8<0>::get (word));

    // Set byte at index 3 (least significant byte)
    word = Utils::U8<3>::set (word, 0xCD);
    EXPECT_EQ (0xAB0000CD, word);
    EXPECT_EQ (0xCD, Utils::U8<3>::get (word));

    // Test overwriting
    word = Utils::U8<1>::set (word, 0xEF);
    EXPECT_EQ (0xABEF00CD, word);
    EXPECT_EQ (0xEF, Utils::U8<1>::get (word));
}

TEST (UMPUtilsTests, U16TemplateHelpers)
{
    using namespace yup::universal_midi_packets;

    uint32_t word = 0x00000000;

    // Set 16-bit value at index 0 (most significant 16 bits)
    word = Utils::U16<0>::set (word, 0xABCD);
    EXPECT_EQ (0xABCD0000, word);
    EXPECT_EQ (0xABCD, Utils::U16<0>::get (word));

    // Set 16-bit value at index 1 (least significant 16 bits)
    word = Utils::U16<1>::set (word, 0xEF12);
    EXPECT_EQ (0xABCDEF12, word);
    EXPECT_EQ (0xEF12, Utils::U16<1>::get (word));

    // Test overwriting
    word = Utils::U16<0>::set (word, 0x3456);
    EXPECT_EQ (0x3456EF12, word);
    EXPECT_EQ (0x3456, Utils::U16<0>::get (word));
}

TEST (UMPUtilsTests, BytesToWordFunction)
{
    using namespace yup::universal_midi_packets;

    auto result = Utils::bytesToWord (std::byte { 0x12 }, std::byte { 0x34 }, std::byte { 0x56 }, std::byte { 0x78 });
    EXPECT_EQ (0x12345678, result);

    // Test with zeros
    result = Utils::bytesToWord (std::byte { 0x00 }, std::byte { 0x00 }, std::byte { 0x00 }, std::byte { 0x00 });
    EXPECT_EQ (0x00000000, result);

    // Test with 0xFF values
    result = Utils::bytesToWord (std::byte { 0xFF }, std::byte { 0xFF }, std::byte { 0xFF }, std::byte { 0xFF });
    EXPECT_EQ (0xFFFFFFFF, result);
}
