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

#if 0
using namespace yup;

namespace
{
void writeBytes(OutputStream& os, const std::vector<uint8>& bytes)
{
    for (const auto& byte : bytes)
        os.writeByte((char)byte);
}

template <typename Fn>
MidiMessageSequence parseSequence(Fn&& fn)
{
    MemoryOutputStream os;
    fn(os);

    return MidiFileHelpers::readTrack(reinterpret_cast<const uint8*>(os.getData()),
                                    (int)os.getDataSize());
}

template <typename Fn>
Optional<MidiFileHelpers::HeaderDetails> parseHeader(Fn&& fn)
{
    MemoryOutputStream os;
    fn(os);

    return MidiFileHelpers::parseMidiHeader(reinterpret_cast<const uint8*>(os.getData()),
                                          os.getDataSize());
}

template <typename Fn>
Optional<MidiFile> parseFile(Fn&& fn)
{
    MemoryOutputStream os;
    fn(os);

    MemoryInputStream is(os.getData(), os.getDataSize(), false);
    MidiFile mf;

    int fileType = 0;

    if (mf.readFrom(is, true, &fileType))
        return mf;

    return {};
}
} // namespace

class MidiFileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data if needed
    }
};

TEST_F(MidiFileTest, ReadTrackRespectsRunningStatus)
{
    const auto sequence = parseSequence([](OutputStream& os)
    {
        MidiFileHelpers::writeVariableLengthInt(os, 100);
        writeBytes(os, { 0x90, 0x40, 0x40 });
        MidiFileHelpers::writeVariableLengthInt(os, 200);
        writeBytes(os, { 0x40, 0x40 });
        MidiFileHelpers::writeVariableLengthInt(os, 300);
        writeBytes(os, { 0xff, 0x2f, 0x00 });
    });

    EXPECT_EQ(sequence.getNumEvents(), 3);
    EXPECT_TRUE(sequence.getEventPointer(0)->message.isNoteOn());
    EXPECT_TRUE(sequence.getEventPointer(1)->message.isNoteOn());
    EXPECT_TRUE(sequence.getEventPointer(2)->message.isEndOfTrackMetaEvent());
}

TEST_F(MidiFileTest, ReadTrackReturnsAvailableMessagesIfInputIsTruncated)
{
    {
        const auto sequence = parseSequence([](OutputStream& os)
        {
            // Incomplete delta time
            writeBytes(os, { 0xff });
        });

        EXPECT_EQ(sequence.getNumEvents(), 0);
    }

    {
        const auto sequence = parseSequence([](OutputStream& os)
        {
            // Complete delta with no following event
            MidiFileHelpers::writeVariableLengthInt(os, 0xffff);
        });

        EXPECT_EQ(sequence.getNumEvents(), 0);
    }

    {
        const auto sequence = parseSequence([](OutputStream& os)
        {
            // Complete delta with malformed following event
            MidiFileHelpers::writeVariableLengthInt(os, 0xffff);
            writeBytes(os, { 0x90, 0x40 });
        });

        EXPECT_EQ(sequence.getNumEvents(), 1);
        EXPECT_TRUE(sequence.getEventPointer(0)->message.isNoteOff());
        EXPECT_EQ(sequence.getEventPointer(0)->message.getNoteNumber(), 0x40);
        EXPECT_EQ(sequence.getEventPointer(0)->message.getVelocity(), (uint8)0x00);
    }
}

TEST_F(MidiFileTest, HeaderParsingWorks)
{
    {
        // No data
        const auto header = parseHeader([](OutputStream&) {});
        EXPECT_FALSE(header.hasValue());
    }

    {
        // Invalid initial byte
        const auto header = parseHeader([](OutputStream& os)
        {
            writeBytes(os, { 0xff });
        });

        EXPECT_FALSE(header.hasValue());
    }

    {
        // Type block, but no header data
        const auto header = parseHeader([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd' });
        });

        EXPECT_FALSE(header.hasValue());
    }

    {
        // Well-formed header, but track type is 0 and channels != 1
        const auto header = parseHeader([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 16, 0, 1 });
        });

        EXPECT_FALSE(header.hasValue());
    }

    {
        // Well-formed header, but track type is 5
        const auto header = parseHeader([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 5, 0, 16, 0, 1 });
        });

        EXPECT_FALSE(header.hasValue());
    }

    {
        // Well-formed header
        const auto header = parseHeader([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 16, 0, 1 });
        });

        EXPECT_TRUE(header.hasValue());

        EXPECT_EQ(header->fileType, (short)1);
        EXPECT_EQ(header->numberOfTracks, (short)16);
        EXPECT_EQ(header->timeFormat, (short)1);
        EXPECT_EQ((int)header->bytesRead, 14);
    }
}

TEST_F(MidiFileTest, ReadFromStream)
{
    {
        // Empty input
        const auto file = parseFile([](OutputStream&) {});
        EXPECT_FALSE(file.hasValue());
    }

    {
        // Malformed header
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd' });
        });

        EXPECT_FALSE(file.hasValue());
    }

    {
        // Header, no channels
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 0, 0, 1 });
        });

        EXPECT_TRUE(file.hasValue());
        EXPECT_EQ(file->getNumTracks(), 0);
    }

    {
        // Header, one malformed channel
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 1 });
            writeBytes(os, { 'M', 'T', 'r', '?' });
        });

        EXPECT_FALSE(file.hasValue());
    }

    {
        // Header, one channel with malformed message
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 1 });
            writeBytes(os, { 'M', 'T', 'r', 'k', 0, 0, 0, 1, 0xff });
        });

        EXPECT_TRUE(file.hasValue());
        EXPECT_EQ(file->getNumTracks(), 1);
        EXPECT_EQ(file->getTrack(0)->getNumEvents(), 0);
    }

    {
        // Header, one channel with incorrect length message
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 1 });
            writeBytes(os, { 'M', 'T', 'r', 'k', 0x0f, 0, 0, 0, 0xff });
        });

        EXPECT_FALSE(file.hasValue());
    }

    {
        // Header, one channel, all well-formed
        const auto file = parseFile([](OutputStream& os)
        {
            writeBytes(os, { 'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 1, 0, 1, 0, 1 });
            writeBytes(os, { 'M', 'T', 'r', 'k', 0, 0, 0, 4 });

            MidiFileHelpers::writeVariableLengthInt(os, 0x0f);
            writeBytes(os, { 0x80, 0x00, 0x00 });
        });

        EXPECT_TRUE(file.hasValue());
        EXPECT_EQ(file->getNumTracks(), 1);

        auto& track = *file->getTrack(0);
        EXPECT_EQ(track.getNumEvents(), 1);
        EXPECT_TRUE(track.getEventPointer(0)->message.isNoteOff());
        EXPECT_EQ(track.getEventPointer(0)->message.getTimeStamp(), (double)0x0f);
    }
}

#endif
