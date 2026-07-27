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

TEST (UMPMidi1ChannelVoiceMessageTests, Constructors)
{
    Midi1ChannelVoiceMessage m (4, Status (Midi1ChannelVoiceStatus::noteOn) + 6, 44, 101);

    EXPECT_TRUE (isMidi1ChannelVoiceMessage (m));
    EXPECT_EQ (m.data[0], 0x24962c65u);
    EXPECT_EQ (m.getGroup(), 4u);

    auto view = Midi1ChannelVoiceMessageView { m };
    EXPECT_EQ (view.getGroup(), 4u);
    EXPECT_EQ (view.getStatus(), Status (Midi1ChannelVoiceStatus::noteOn));
    EXPECT_EQ (view.getChannel(), 6u);
    EXPECT_EQ (view.getDataByte1(), 0x2cu);
    EXPECT_EQ (view.getDataByte2(), 0x65u);
}

TEST (UMPMidi1ChannelVoiceMessageTests, ViewAndChannel)
{
    constexpr uint32_t testCases[] = {
        0x208e2345u, 0x23922345u, 0x2aa54621u, 0x26b12345u, 0x27cd2345u, 0x29ca2345u, 0x2ce74621u
    };

    for (const auto data : testCases)
    {
        Midi1ChannelVoiceMessage m { UniversalPacket { data } };
        const auto status = uint8_t ((data >> 16u) & 0xffu);
        EXPECT_EQ (m.getChannel(), Channel (status & 0x0f));
    }
}

TEST (UMPMidi1ChannelVoiceMessageTests, DataBytesAnd14Bit)
{
    auto m = makeMidi1ChannelVoiceMessage (1, Status (Midi1ChannelVoiceStatus::noteOn), 9, 0x12, 0x34);
    auto view = Midi1ChannelVoiceMessageView { m };

    EXPECT_EQ (view.getDataByte1(), 0x12u);
    EXPECT_EQ (view.getDataByte2(), 0x34u);
    EXPECT_EQ (view.get14BitValue(), 0x1a12u);
}

TEST (UMPMidi1ChannelVoiceMessageTests, MakeMessageHelpers)
{
    auto noteOn = makeMidi1NoteOnMessage (1, 2, 60, Velocity { uint7_t { 12 } });
    auto noteOff = makeMidi1NoteOffMessage (3, 4, 61, Velocity { uint7_t { 20 } });
    auto pressure = makeMidi1PolyPressureMessage (1, 5, 62, ControllerValue { uint7_t { 33 } });
    auto control = makeMidi1ControlChangeMessage (2, 6, 7, ControllerValue { uint7_t { 99 } });
    auto program = makeMidi1ProgramChangeMessage (3, 7, 42);
    auto channelPressure = makeMidi1ChannelPressureMessage (4, 8, ControllerValue { uint7_t { 77 } });
    auto pitchBend = makeMidi1PitchBendMessage (5, 9, PitchBend { uint14_t { 0x1234 } });

    EXPECT_TRUE (isMidi1ChannelVoiceMessage (noteOn));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (noteOff));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (pressure));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (control));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (program));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (channelPressure));
    EXPECT_TRUE (isMidi1ChannelVoiceMessage (pitchBend));
}

TEST (UMPMidi1ChannelVoiceMessageTests, NoteOnNoteAndVelocityStoredCorrectly)
{
    auto msg = makeMidi1NoteOnMessage (1, 2, 60, Velocity { uint7_t { 12 } });
    auto view = Midi1ChannelVoiceMessageView { msg };

    EXPECT_EQ (view.getDataByte1(), 60u); // note number
    EXPECT_EQ (view.getDataByte2(), 12u); // velocity
    EXPECT_EQ (view.getStatus(), Status (Midi1ChannelVoiceStatus::noteOn));
    EXPECT_EQ (view.getChannel(), 2u);
}

TEST (UMPMidi1ChannelVoiceMessageTests, NoteOffNoteAndVelocityStoredCorrectly)
{
    auto msg = makeMidi1NoteOffMessage (0, 3, 64, Velocity { uint7_t { 80 } });
    auto view = Midi1ChannelVoiceMessageView { msg };

    EXPECT_EQ (view.getDataByte1(), 64u);
    EXPECT_EQ (view.getDataByte2(), 80u);
    EXPECT_EQ (view.getStatus(), Status (Midi1ChannelVoiceStatus::noteOff));
    EXPECT_EQ (view.getChannel(), 3u);
}

TEST (UMPMidi1ChannelVoiceMessageTests, PitchBend14BitValueMatchesInput)
{
    auto msg = makeMidi1PitchBendMessage (2, 5, PitchBend { uint14_t { 0x2000 } });
    auto view = Midi1ChannelVoiceMessageView { msg };

    // LSB goes in byte1, MSB in byte2; get14BitValue reconstructs from both
    EXPECT_EQ (view.get14BitValue(), 0x2000u);
}
