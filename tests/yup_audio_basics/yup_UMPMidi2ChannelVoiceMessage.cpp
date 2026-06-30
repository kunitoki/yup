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

TEST (UMPMidi2ChannelVoiceMessageTests, Constructors)
{
    Midi2ChannelVoiceMessage m (4, Status (ChannelVoiceStatus::noteOn), 6, 44, 101, 0x12345678u);

    EXPECT_TRUE (isMidi2ChannelVoiceMessage (m));
    EXPECT_EQ (m.getGroup(), 4u);

    auto view = Midi2ChannelVoiceMessageView { m };
    EXPECT_EQ (view.getGroup(), 4u);
    EXPECT_EQ (view.getStatus(), Status (ChannelVoiceStatus::noteOn));
    EXPECT_EQ (view.getChannel(), 6u);
    EXPECT_EQ (view.getByte3(), 44u);
    EXPECT_EQ (view.getByte4(), 101u);
    EXPECT_EQ (view.getData(), 0x12345678u);
}

TEST (UMPMidi2ChannelVoiceMessageTests, MakeMessageHelpers)
{
    auto noteOn = makeMidi2NoteOnMessage (1, 2, 60, Velocity { uint16_t { 0x1234 } });
    auto noteOff = makeMidi2NoteOffMessage (3, 4, 61, Velocity { uint16_t { 0x5678 } });
    auto poly = makeMidi2PolyPressureMessage (1, 5, 62, ControllerValue { uint32_t { 0x12345678 } });
    auto control = makeMidi2ControlChangeMessage (2, 6, 7, ControllerValue { uint32_t { 0xabcdef01 } });
    auto program = makeMidi2ProgramChangeMessage (3, 7, 42);
    auto channelPressure = makeMidi2ChannelPressureMessage (4, 8, ControllerValue { uint32_t { 0xdeadbeef } });
    auto pitchBend = makeMidi2PitchBendMessage (5, 9, PitchBend { uint32_t { 0x81234567 } });

    EXPECT_TRUE (isMidi2ChannelVoiceMessage (noteOn));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (noteOff));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (poly));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (control));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (program));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (channelPressure));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (pitchBend));
}

TEST (UMPMidi2ChannelVoiceMessageTests, PerNoteMessages)
{
    auto reg = makeRegisteredPerNoteControllerMessage (15, 10, 44, 2, ControllerValue { 123456u });
    auto assign = makeAssignablePerNoteControllerMessage (3, 7, 64, 99, ControllerValue { 987654u });
    auto perNote = makePerNotePitchBendMessage (11, 12, 13, PitchBend { uint32_t { 0x80000001u } });

    EXPECT_TRUE (isMidi2ChannelVoiceMessage (reg));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (assign));
    EXPECT_TRUE (isMidi2ChannelVoiceMessage (perNote));
}

TEST (UMPMidi2ChannelVoiceMessageTests, ConstructorDataFieldIsStoredCorrectly)
{
    Midi2ChannelVoiceMessage m (4, Status (ChannelVoiceStatus::noteOn), 6, 44, 101, 0x12345678u);
    auto view = Midi2ChannelVoiceMessageView { m };
    EXPECT_EQ (view.getData(), 0x12345678u);
}

TEST (UMPMidi2ChannelVoiceMessageTests, NoteOnByte3IsNoteNumber)
{
    auto msg = makeMidi2NoteOnMessage (1, 2, 60, Velocity { uint16_t { 0x8000 } });
    auto view = Midi2ChannelVoiceMessageView { msg };
    EXPECT_EQ (view.getByte3(), 60u);
    EXPECT_EQ (view.getStatus(), Status (ChannelVoiceStatus::noteOn));
    EXPECT_EQ (view.getChannel(), 2u);
}

TEST (UMPMidi2ChannelVoiceMessageTests, PitchBendDataIsFullRange)
{
    auto msg = makeMidi2PitchBendMessage (0, 1, PitchBend { uint32_t { 0x80000000u } });
    auto view = Midi2ChannelVoiceMessageView { msg };
    EXPECT_EQ (view.getData(), 0x80000000u);
}
