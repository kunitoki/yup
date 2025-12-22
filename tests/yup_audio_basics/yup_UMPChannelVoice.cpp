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
UniversalPacket makePacket (PacketType type, Group group, Status status, uint32_t data)
{
    return UniversalPacket { (uint32_t (type) << 28u) | ((group & 0x0f) << 24u) | (uint32_t (status) << 16u), data };
}
} // namespace

TEST (UMPChannelVoiceTests, DISABLED_IsChannelVoiceMessageWithStatus)
{
    for (uint8_t match = 0x00; match <= 0xf0; match += 0x10)
    {
        for (uint8_t t = 0; t < 16; ++t)
        {
            const bool channelVoice = (t == uint8_t (PacketType::midi1ChannelVoice))
                                   || (t == uint8_t (PacketType::midi2ChannelVoice));
            for (uint8_t s = 0; s <= 0xf0; s += 0x10)
            {
                auto p = UniversalPacket { uint32_t ((t << 28) | (s << 16) | t | (s >> 4)) };
                EXPECT_EQ (channelVoice && (s == match), isChannelVoiceMessageWithStatus (p, Status (match)));
            }
        }
    }
}

TEST (UMPChannelVoiceTests, NoteOnOffDetection)
{
    EXPECT_TRUE (isNoteOnMessage (makeMidi1NoteOnMessage (4, 7, 99, Velocity { uint7_t { 100 } })));
    EXPECT_FALSE (isNoteOnMessage (makeMidi1NoteOnMessage (13, 5, 60, Velocity { uint7_t { 0 } })));
    EXPECT_TRUE (isNoteOffMessage (makeMidi1NoteOnMessage (13, 5, 60, Velocity { uint7_t { 0 } })));
    EXPECT_TRUE (isNoteOffMessage (makeMidi1NoteOffMessage (0, 2, 67)));

    EXPECT_TRUE (isNoteOnMessage (makeMidi2NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } })));
    EXPECT_TRUE (isNoteOnMessage (makeMidi2NoteOnMessage (13, 5, 60, Velocity { uint16_t { 0 } })));
    EXPECT_TRUE (isNoteOffMessage (makeMidi2NoteOffMessage (0, 2, 67, Velocity { uint16_t { 0x1234 } })));
}

TEST (UMPChannelVoiceTests, NoteNumberPitchVelocity)
{
    EXPECT_EQ (getNoteNumber (makeMidi1NoteOffMessage (0, 2, 67)), NoteNumber (67));
    EXPECT_EQ (getNoteNumber (makeMidi2NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } })), NoteNumber (99));

    EXPECT_EQ (getNotePitch (makeMidi1NoteOnMessage (13, 5, 60, Velocity { uint7_t { 0 } })),
               Pitch7_9 { NoteNumber (60) });
    EXPECT_EQ (getNotePitch (makeMidi2NoteOnMessage (9, 10, 127, Velocity { uint16_t { 0xa000 } }, Pitch7_9 { 89.45f })),
               Pitch7_9 { 89.45f });

    EXPECT_EQ (getNoteVelocity (makeMidi1NoteOffMessage (0, 2, 67)), Velocity { uint7_t { 64 } });
    EXPECT_EQ (getNoteVelocity (makeMidi2NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } })),
               Velocity { uint16_t { 0x4567 } });
}

TEST (UMPChannelVoiceTests, ControllerMessages)
{
    EXPECT_TRUE (isControlChangeMessage (makeMidi1ControlChangeMessage (5, 15, 7, ControllerValue { uint7_t { 100 } })));
    EXPECT_EQ (getControllerNumber (makeMidi1ControlChangeMessage (5, 15, 7, ControllerValue { uint7_t { 100 } })),
               ControllerNumber (7));
    EXPECT_EQ (getControllerValue (makeMidi1ControlChangeMessage (5, 15, 7, ControllerValue { uint7_t { 100 } })),
               ControllerValue { uint7_t { 100 } });

    EXPECT_TRUE (isControlChangeMessage (makeMidi2ControlChangeMessage (5, 15, 7, ControllerValue { 0x89abcdefu })));
    EXPECT_EQ (getControllerValue (makeMidi2ControlChangeMessage (5, 15, 7, ControllerValue { 0x89abcdefu })),
               ControllerValue { 0x89abcdefu });
}

TEST (UMPChannelVoiceTests, PressureAndPitchBend)
{
    EXPECT_TRUE (isPolyPressureMessage (makeMidi1PolyPressureMessage (14, 2, 64, ControllerValue { uint7_t { 77 } })));
    EXPECT_EQ (getPolyPressureValue (makeMidi2PolyPressureMessage (1, 4, 99, ControllerValue { 12345u })),
               ControllerValue { 12345u });

    EXPECT_TRUE (isChannelPressureMessage (makeMidi1ChannelPressureMessage (8, 8, ControllerValue { uint7_t { 77 } })));
    EXPECT_EQ (getChannelPressureValue (makeMidi2ChannelPressureMessage (9, 0, ControllerValue { 12345u })),
               ControllerValue { 12345u });

    EXPECT_TRUE (isChannelPitchBendMessage (makeMidi1PitchBendMessage (1, 13, PitchBend { uint14_t { 8177 } })));
    EXPECT_EQ (getChannelPitchBendValue (makeMidi2PitchBendMessage (2, 14, PitchBend { 0x81234567u })),
               PitchBend { 0x81234567u });
}

TEST (UMPChannelVoiceTests, Midi2SpecificPredicates)
{
    EXPECT_TRUE (isRegisteredControllerMessage (makeRegisteredControllerMessage (2, 9, 0, 4, ControllerValue { 123456u })));
    EXPECT_TRUE (isAssignableControllerMessage (makeAssignableControllerMessage (8, 0, 4, 12, ControllerValue { 987654u })));
    EXPECT_TRUE (isRegisteredPerNoteControllerMessage (makeRegisteredPerNoteControllerMessage (15, 10, 44, 2, ControllerValue { 123456u })));
    EXPECT_TRUE (isAssignablePerNoteControllerMessage (makeAssignablePerNoteControllerMessage (3, 7, 64, 99, ControllerValue { 987654u })));
    EXPECT_TRUE (isPerNotePitchBendMessage (makePerNotePitchBendMessage (11, 12, 13, PitchBend { 0x80000001u })));
}

TEST (UMPChannelVoiceTests, Midi2AttributesAndSensitivity)
{
    const auto noteOn = makeMidi2NoteOnMessage (9, 10, 127, Velocity { uint16_t { 0xa000 } }, Pitch7_9 { NoteNumber { 60 } });
    EXPECT_TRUE (isNoteOnWithPitch7_9 (noteOn));
    EXPECT_EQ (getMidi2NoteAttribute (noteOn), NoteAttribute::pitch_7_9);

    const auto reg = makeRegisteredControllerMessage (2, 9, 0, RegisteredParameterNumber::pitchBendSensitivity, ControllerValue { 0x12340000u });
    EXPECT_TRUE (isPitchBendSensitivityMessage (reg));
    EXPECT_EQ (getPitchBendSensitivityValue (reg), PitchBendSensitivity { 0x12340000u });
}

TEST (UMPChannelVoiceTests, Midi2ToMidi1Translation)
{
    const auto noteOn = Midi2ChannelVoiceMessageView { makeMidi2NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } }) };
    const auto noteOff = Midi2ChannelVoiceMessageView { makeMidi2NoteOffMessage (3, 9, 66, Velocity { uint16_t { 0x1234 } }) };

    auto m1 = asMidi1ChannelVoiceMessage (noteOn);
    ASSERT_TRUE (m1.has_value());
    EXPECT_EQ (*m1, makeMidi1NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } }));

    auto m2 = asMidi1ChannelVoiceMessage (noteOff);
    ASSERT_TRUE (m2.has_value());
    EXPECT_EQ (*m2, makeMidi1NoteOffMessage (3, 9, 66, Velocity { uint16_t { 0x1234 } }));
}
