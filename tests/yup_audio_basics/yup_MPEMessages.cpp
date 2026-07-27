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

namespace
{
void extractRawBinaryData (const MidiBuffer& midiBuffer, const uint8* bufferToCopyTo, std::size_t maxBytes)
{
    std::size_t pos = 0;

    for (const auto metadata : midiBuffer)
    {
        const uint8* data = metadata.data;
        std::size_t dataSize = (std::size_t) metadata.numBytes;

        if (pos + dataSize > maxBytes)
            return;

        std::memcpy ((void*) (bufferToCopyTo + pos), data, dataSize);
        pos += dataSize;
    }
}

void testMidiBuffer (MidiBuffer& buffer, const uint8* expectedBytes, int expectedBytesSize)
{
    uint8 actualBytes[128] = { 0 };
    extractRawBinaryData (buffer, actualBytes, sizeof (actualBytes));

    EXPECT_EQ (std::memcmp (actualBytes, expectedBytes, (std::size_t) expectedBytesSize), 0);
}
} // namespace

class MPEMessagesTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup common test data if needed
    }
};

TEST_F (MPEMessagesTest, AddZoneLower)
{
    MidiBuffer buffer = MPEMessages::setLowerZone (7);

    const uint8 expectedBytes[] = {
        0xb0, 0x64, 0x06, 0xb0, 0x65, 0x00, 0xb0, 0x06, 0x07, // set up zone
        0xb1,
        0x64,
        0x00,
        0xb1,
        0x65,
        0x00,
        0xb1,
        0x06,
        0x30, // per-note pbrange (default = 48)
        0xb0,
        0x64,
        0x00,
        0xb0,
        0x65,
        0x00,
        0xb0,
        0x06,
        0x02 // master pbrange (default = 2)
    };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}

TEST_F (MPEMessagesTest, AddZoneUpper)
{
    MidiBuffer buffer = MPEMessages::setUpperZone (5, 96, 0);

    const uint8 expectedBytes[] = {
        0xbf, 0x64, 0x06, 0xbf, 0x65, 0x00, 0xbf, 0x06, 0x05, // set up zone
        0xbe,
        0x64,
        0x00,
        0xbe,
        0x65,
        0x00,
        0xbe,
        0x06,
        0x60, // per-note pbrange (custom)
        0xbf,
        0x64,
        0x00,
        0xbf,
        0x65,
        0x00,
        0xbf,
        0x06,
        0x00 // master pbrange (custom)
    };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}

TEST_F (MPEMessagesTest, SetPerNotePitchbendRange)
{
    MidiBuffer buffer = MPEMessages::setLowerZonePerNotePitchbendRange (96);

    const uint8 expectedBytes[] = { 0xb1, 0x64, 0x00, 0xb1, 0x65, 0x00, 0xb1, 0x06, 0x60 };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}

TEST_F (MPEMessagesTest, SetMasterPitchbendRange)
{
    MidiBuffer buffer = MPEMessages::setUpperZoneMasterPitchbendRange (60);

    const uint8 expectedBytes[] = { 0xbf, 0x64, 0x00, 0xbf, 0x65, 0x00, 0xbf, 0x06, 0x3c };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}

TEST_F (MPEMessagesTest, ClearAllZones)
{
    MidiBuffer buffer = MPEMessages::clearAllZones();

    const uint8 expectedBytes[] = {
        0xb0, 0x64, 0x06, 0xb0, 0x65, 0x00, 0xb0, 0x06, 0x00, // clear lower zone
        0xbf,
        0x64,
        0x06,
        0xbf,
        0x65,
        0x00,
        0xbf,
        0x06,
        0x00 // clear upper zone
    };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}

TEST_F (MPEMessagesTest, SetCompleteState)
{
    MPEZoneLayout layout;

    layout.setLowerZone (7, 96, 0);
    layout.setUpperZone (7);

    MidiBuffer buffer = MPEMessages::setZoneLayout (layout);

    const uint8 expectedBytes[] = {
        0xb0, 0x64, 0x06, 0xb0, 0x65, 0x00, 0xb0, 0x06, 0x00, // clear lower zone
        0xbf,
        0x64,
        0x06,
        0xbf,
        0x65,
        0x00,
        0xbf,
        0x06,
        0x00, // clear upper zone
        0xb0,
        0x64,
        0x06,
        0xb0,
        0x65,
        0x00,
        0xb0,
        0x06,
        0x07, // set lower zone
        0xb1,
        0x64,
        0x00,
        0xb1,
        0x65,
        0x00,
        0xb1,
        0x06,
        0x60, // per-note pbrange (custom)
        0xb0,
        0x64,
        0x00,
        0xb0,
        0x65,
        0x00,
        0xb0,
        0x06,
        0x00, // master pbrange (custom)
        0xbf,
        0x64,
        0x06,
        0xbf,
        0x65,
        0x00,
        0xbf,
        0x06,
        0x07, // set upper zone
        0xbe,
        0x64,
        0x00,
        0xbe,
        0x65,
        0x00,
        0xbe,
        0x06,
        0x30, // per-note pbrange (default = 48)
        0xbf,
        0x64,
        0x00,
        0xbf,
        0x65,
        0x00,
        0xbf,
        0x06,
        0x02 // master pbrange (default = 2)
    };

    testMidiBuffer (buffer, expectedBytes, sizeof (expectedBytes));
}
