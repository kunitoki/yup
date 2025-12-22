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

TEST (UMPKeyboardStateTests, ProcessesMidi1NoteOnOff)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_1_0);
    state.setGroup (0);

    const auto noteOn = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    state.processNextUMPPacket (View (noteOn.data));
    EXPECT_TRUE (state.isNoteOn (1, 60));

    const auto noteOff = makeMidi1NoteOffMessage (0, 0, 60, Velocity { uint7_t { 64 } });
    state.processNextUMPPacket (View (noteOff.data));
    EXPECT_FALSE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, ProcessesMidi2NoteOnOff)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_2_0);
    state.setGroup (0);

    const auto noteOn = makeMidi2NoteOnMessage (0, 1, 64, Velocity { uint16_t { 0x9000 } });
    state.processNextUMPPacket (View (noteOn.data));
    EXPECT_TRUE (state.isNoteOn (2, 64));

    const auto noteOff = makeMidi2NoteOffMessage (0, 1, 64, Velocity { uint16_t { 0x4000 } });
    state.processNextUMPPacket (View (noteOff.data));
    EXPECT_FALSE (state.isNoteOn (2, 64));
}

TEST (UMPKeyboardStateTests, FiltersByGroup)
{
    UMPKeyboardState state;
    state.setGroup (1);

    const auto noteOn = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    state.processNextUMPPacket (View (noteOn.data));
    EXPECT_FALSE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, AllNotesOffController)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_1_0);
    state.setGroup (0);

    const auto noteOn = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    state.processNextUMPPacket (View (noteOn.data));
    EXPECT_TRUE (state.isNoteOn (1, 60));

    const auto allNotesOff = makeMidi1ControlChangeMessage (0, 0, 123, ControllerValue { uint7_t { 0 } });
    state.processNextUMPPacket (View (allNotesOff.data));
    EXPECT_FALSE (state.isNoteOn (1, 60));
}
