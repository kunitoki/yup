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

#include <gtest/gtest.h>

#include <juce_audio_basics/juce_audio_basics.h>

using namespace juce;

namespace {
const std::vector<uint8> metaEvents[] {
    // Format is 0xff, followed by a 'kind' byte, followed by a variable-length
    // 'data-length' value, followed by that many data bytes
    { 0xff, 0x00, 0x02, 0x00, 0x00 },                   // Sequence number
    { 0xff, 0x01, 0x00 },                               // Text event
    { 0xff, 0x02, 0x00 },                               // Copyright notice
    { 0xff, 0x03, 0x00 },                               // Track name
    { 0xff, 0x04, 0x00 },                               // Instrument name
    { 0xff, 0x05, 0x00 },                               // Lyric
    { 0xff, 0x06, 0x00 },                               // Marker
    { 0xff, 0x07, 0x00 },                               // Cue point
    { 0xff, 0x20, 0x01, 0x00 },                         // Channel prefix
    { 0xff, 0x2f, 0x00 },                               // End of track
    { 0xff, 0x51, 0x03, 0x01, 0x02, 0x03 },             // Set tempo
    { 0xff, 0x54, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05 }, // SMPTE offset
    { 0xff, 0x58, 0x04, 0x01, 0x02, 0x03, 0x04 },       // Time signature
    { 0xff, 0x59, 0x02, 0x01, 0x02 },                   // Key signature
    { 0xff, 0x7f, 0x00 },                               // Sequencer-specific
};
} // namespace

TEST (MidiMessageTests, ReadVariableLengthValueShouldReturnCompatibleResults)
{
    using std::begin;
    using std::end;

    const std::vector<uint8> inputs[] {
        { 0x00 },
        { 0x40 },
        { 0x7f },
        { 0x81, 0x00 },
        { 0xc0, 0x00 },
        { 0xff, 0x7f },
        { 0x81, 0x80, 0x00 },
        { 0xc0, 0x80, 0x00 },
        { 0xff, 0xff, 0x7f },
        { 0x81, 0x80, 0x80, 0x00 },
        { 0xc0, 0x80, 0x80, 0x00 },
        { 0xff, 0xff, 0xff, 0x7f }
    };

    const int outputs[] {
        0x00,
        0x40,
        0x7f,
        0x80,
        0x2000,
        0x3fff,
        0x4000,
        0x100000,
        0x1fffff,
        0x200000,
        0x8000000,
        0xfffffff,
    };

    EXPECT_EQ (std::distance (begin (inputs), end (inputs)),
                    std::distance (begin (outputs), end (outputs)));

    size_t index = 0;

    for (const auto& input : inputs)
    {
        auto copy = input;

        while (copy.size() < 16)
            copy.push_back (0);

        const auto result = MidiMessage::readVariableLengthValue (copy.data(),
                                                                    (int) copy.size());

        EXPECT_TRUE (result.isValid());
        EXPECT_EQ (result.value, outputs[index]);
        EXPECT_EQ (result.bytesUsed, (int) inputs[index].size());

        ++index;
    }
}

TEST (MidiMessageTests, ReadVariableLengthValueShouldReturnZeroWithTruncatedInput)
{
    for (size_t i = 0; i != 16; ++i)
    {
        std::vector<uint8> input;
        input.resize (i, 0xFF);

        const auto result = MidiMessage::readVariableLengthValue (input.data(),
                                                                    (int) input.size());

        EXPECT_TRUE (! result.isValid());
        EXPECT_EQ (result.value, 0);
        EXPECT_EQ (result.bytesUsed, 0);
    }
}

TEST (MidiMessageTests, DataConstructorWorksWithMetaEvents)
{
    const auto status = (uint8) 0x90;

    for (const auto& input : metaEvents)
    {
        int bytesUsed = 0;
        const MidiMessage msg (input.data(), (int) input.size(), bytesUsed, status);

        EXPECT_TRUE (msg.isMetaEvent());
        EXPECT_EQ (msg.getMetaEventLength(), (int) input.size() - 3);
        EXPECT_EQ (msg.getMetaEventType(), (int) input[1]);
    }
}

TEST (MidiMessageTests, DataConstructorWorksWithMalformedMetaEvents)
{
    const auto status = (uint8) 0x90;

    const auto runTest = [&] (const std::vector<uint8>& input)
    {
        int bytesUsed = 0;
        const MidiMessage msg (input.data(), (int) input.size(), bytesUsed, status);

        EXPECT_TRUE (msg.isMetaEvent());
        EXPECT_EQ (msg.getMetaEventLength(), jmax (0, (int) input.size() - 3));
        EXPECT_EQ (msg.getMetaEventType(), input.size() >= 2 ? input[1] : -1);
    };

    runTest ({ 0xff });

    for (const auto& input : metaEvents)
    {
        auto copy = input;
        copy[2] = 0x40; // Set the size of the message to more bytes than are present

        runTest (copy);
    }
}
