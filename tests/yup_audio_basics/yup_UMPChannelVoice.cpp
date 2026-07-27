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
    const auto noteOnMessage = makeMidi2NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } });
    const auto noteOffMessage = makeMidi2NoteOffMessage (3, 9, 66, Velocity { uint16_t { 0x1234 } });
    const auto noteOn = Midi2ChannelVoiceMessageView { noteOnMessage };
    const auto noteOff = Midi2ChannelVoiceMessageView { noteOffMessage };

    auto m1 = asMidi1ChannelVoiceMessage (noteOn);
    ASSERT_TRUE (m1.has_value());
    EXPECT_EQ (*m1, makeMidi1NoteOnMessage (4, 7, 99, Velocity { uint16_t { 0x4567 } }));

    auto m2 = asMidi1ChannelVoiceMessage (noteOff);
    ASSERT_TRUE (m2.has_value());
    EXPECT_EQ (*m2, makeMidi1NoteOffMessage (3, 9, 66, Velocity { uint16_t { 0x1234 } }));
}

TEST (UMPChannelVoiceTests, ProgramChangeMessages)
{
    EXPECT_TRUE (isProgramChangeMessage (makeMidi1ProgramChangeMessage (0, 5, 42)));
    EXPECT_EQ (getProgramValue (makeMidi1ProgramChangeMessage (0, 5, 42)), 42);

    EXPECT_TRUE (isProgramChangeMessage (makeMidi2ProgramChangeMessage (1, 7, 99)));
    EXPECT_EQ (getProgramValue (makeMidi2ProgramChangeMessage (1, 7, 99)), 99);
}

TEST (UMPChannelVoiceTests, Midi1ToMidi2Translation)
{
    auto midi1NoteOn = makeMidi1NoteOnMessage (2, 3, 60, Velocity { uint7_t { 100 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1NoteOn });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isNoteOnMessage (*result));

    auto midi1NoteOff = makeMidi1NoteOffMessage (1, 4, 72, Velocity { uint7_t { 64 } });
    result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1NoteOff });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isNoteOffMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1ZeroVelocityNoteOnToMidi2)
{
    auto midi1NoteOn = makeMidi1NoteOnMessage (2, 3, 60, Velocity { uint7_t { 0 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1NoteOn });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isNoteOffMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1ControlChangeToMidi2)
{
    auto midi1CC = makeMidi1ControlChangeMessage (3, 7, 10, ControllerValue { uint7_t { 64 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1CC });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isControlChangeMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1ProgramChangeToMidi2)
{
    auto midi1PC = makeMidi1ProgramChangeMessage (4, 8, 42);
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1PC });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isProgramChangeMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1PolyPressureToMidi2)
{
    auto midi1PP = makeMidi1PolyPressureMessage (5, 9, 72, ControllerValue { uint7_t { 80 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1PP });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isPolyPressureMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1ChannelPressureToMidi2)
{
    auto midi1CP = makeMidi1ChannelPressureMessage (6, 10, ControllerValue { uint7_t { 70 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1CP });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isChannelPressureMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi1PitchBendToMidi2)
{
    auto midi1PB = makeMidi1PitchBendMessage (7, 11, PitchBend { uint14_t { 8192 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1PB });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isChannelPitchBendMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2PolyPressureToMidi1)
{
    auto midi2PP = makeMidi2PolyPressureMessage (2, 5, 64, ControllerValue { 0x80000000u });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2PP });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isPolyPressureMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2ControlChangeToMidi1)
{
    auto midi2CC = makeMidi2ControlChangeMessage (3, 6, 7, ControllerValue { 0x70000000u });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2CC });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isControlChangeMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2ProgramChangeToMidi1)
{
    auto midi2PC = makeMidi2ProgramChangeMessage (4, 7, 55);
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2PC });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isProgramChangeMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2ChannelPressureToMidi1)
{
    auto midi2CP = makeMidi2ChannelPressureMessage (5, 8, ControllerValue { 0x60000000u });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2CP });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isChannelPressureMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2PitchBendToMidi1)
{
    auto midi2PB = makeMidi2PitchBendMessage (6, 9, PitchBend { 0x80000000u });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2PB });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isChannelPitchBendMessage (*result));
}

TEST (UMPChannelVoiceTests, Midi2NoteOnZeroVelocityToMidi1)
{
    auto midi2NoteOn = makeMidi2NoteOnMessage (2, 3, 60, Velocity { uint16_t { 0 } });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2NoteOn });
    ASSERT_TRUE (result.has_value());
    EXPECT_TRUE (isNoteOnMessage (*result));
    EXPECT_EQ (getNoteVelocity (*result).asUInt7(), 1);
}

TEST (UMPChannelVoiceTests, Midi2WithAttributeNotTranslatable)
{
    auto midi2NoteOn = makeMidi2NoteOnMessage (2, 3, 60, Velocity { uint16_t { 0x8000 } }, Pitch7_9 { 61.5f });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2NoteOn });
    EXPECT_FALSE (result.has_value());
}

TEST (UMPChannelVoiceTests, Midi1SpecialCCNotTranslated)
{
    auto midi1BankSelect = makeMidi1ControlChangeMessage (3, 7, ControlChange::bankSelectMsb, ControllerValue { uint7_t { 1 } });
    auto result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1BankSelect });
    EXPECT_FALSE (result.has_value());

    auto midi1RpnLsb = makeMidi1ControlChangeMessage (3, 7, ControlChange::rpnLsb, ControllerValue { uint7_t { 1 } });
    result = asMidi2ChannelVoiceMessage (Midi1ChannelVoiceMessageView { midi1RpnLsb });
    EXPECT_FALSE (result.has_value());
}

TEST (UMPChannelVoiceTests, Midi2SpecialCCNotTranslated)
{
    auto midi2BankSelect = makeMidi2ControlChangeMessage (3, 7, ControlChange::bankSelectMsb, ControllerValue { 0x80000000u });
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2BankSelect });
    EXPECT_FALSE (result.has_value());

    auto midi2RpnMsb = makeMidi2ControlChangeMessage (3, 7, ControlChange::rpnMsb, ControllerValue { 0x80000000u });
    result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { midi2RpnMsb });
    EXPECT_FALSE (result.has_value());
}

TEST (UMPChannelVoiceTests, Midi2ProgramChangeWithBankSelectNotTranslated)
{
    auto packet = makeMidi2ProgramChangeMessage (4, 7, 55);
    packet.setByte (3, packet.getByte (3) | 0x01);
    auto result = asMidi1ChannelVoiceMessage (Midi2ChannelVoiceMessageView { packet });
    EXPECT_FALSE (result.has_value());
}

TEST (UMPChannelVoiceTests, GetProgramValueInvalid)
{
    UniversalPacket invalidPacket { 0x10000000u };
    EXPECT_EQ (getProgramValue (invalidPacket), 0xff);
}

TEST (UMPChannelVoiceTests, GetNotePitchOnMidi1)
{
    auto midi1NoteOn = makeMidi1NoteOnMessage (2, 3, 60, Velocity { uint7_t { 100 } });
    EXPECT_EQ (getNotePitch (midi1NoteOn), Pitch7_9 { NoteNumber (60) });
}

TEST (UMPChannelVoiceTests, GetNoteVelocityOnMidi1NoteOn)
{
    auto midi1NoteOn = makeMidi1NoteOnMessage (2, 3, 60, Velocity { uint7_t { 100 } });
    EXPECT_EQ (getNoteVelocity (midi1NoteOn), Velocity { uint7_t { 100 } });
}

TEST (UMPChannelVoiceTests, PerNoteMessages)
{
    auto perNotePB = makePerNotePitchBendMessage (5, 6, 72, PitchBend { 0x80001000u });
    EXPECT_TRUE (isPerNotePitchBendMessage (perNotePB));

    auto regPerNoteCC = makeRegisteredPerNoteControllerMessage (3, 4, 60, 1, ControllerValue { 0x50000000u });
    EXPECT_TRUE (isRegisteredPerNoteControllerMessage (regPerNoteCC));
    EXPECT_EQ (getPerNoteControllerIndex (regPerNoteCC), 1);

    auto assPerNoteCC = makeAssignablePerNoteControllerMessage (7, 8, 64, 5, ControllerValue { 0x60000000u });
    EXPECT_TRUE (isAssignablePerNoteControllerMessage (assPerNoteCC));
    EXPECT_EQ (getPerNoteControllerIndex (assPerNoteCC), 5);
}

TEST (UMPChannelVoiceTests, RegisteredAndAssignableControllers)
{
    auto regCC = makeRegisteredControllerMessage (2, 3, 0, 5, ControllerValue { 0x12345678u });
    EXPECT_TRUE (isRegisteredControllerMessage (regCC));

    auto assCC = makeAssignableControllerMessage (4, 5, 1, 7, ControllerValue { 0x87654321u });
    EXPECT_TRUE (isAssignableControllerMessage (assCC));
}

TEST (UMPChannelVoiceTests, PitchBendSensitivity)
{
    auto pbSens = makeRegisteredControllerMessage (1, 2, 0, RegisteredParameterNumber::pitchBendSensitivity, ControllerValue { 0x02000000u });
    EXPECT_TRUE (isPitchBendSensitivityMessage (pbSens));
    EXPECT_EQ (getPitchBendSensitivityValue (pbSens), PitchBendSensitivity { 0x02000000u });
}

TEST (UMPChannelVoiceTests, GetMidi2NoteAttribute)
{
    auto noteOn = makeMidi2NoteOnMessage (2, 3, 60, Velocity { uint16_t { 0x8000 } }, Pitch7_9 { 61.5f });
    EXPECT_EQ (getMidi2NoteAttribute (noteOn), NoteAttribute::pitch_7_9);

    auto noteOnNoAttr = makeMidi2NoteOnMessage (2, 3, 60, Velocity { uint16_t { 0x8000 } });
    EXPECT_EQ (getMidi2NoteAttribute (noteOnNoAttr), NoteAttribute::none);
}
