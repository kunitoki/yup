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

TEST (UMPKeyboardStateTests, DirectNoteOnOff)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_1_0);
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);
    EXPECT_TRUE (state.isNoteOn (1, 60));

    state.noteOff (1, 60, 0.5f);
    EXPECT_FALSE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, DirectNoteOnMidi2Protocol)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_2_0);
    state.setGroup (0);

    state.noteOn (5, 72, 1.0f);
    EXPECT_TRUE (state.isNoteOn (5, 72));

    state.noteOff (5, 72, 0.0f);
    EXPECT_FALSE (state.isNoteOn (5, 72));
}

TEST (UMPKeyboardStateTests, IsNoteOnForChannels)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);
    state.noteOn (2, 60, 0.7f);

    EXPECT_TRUE (state.isNoteOnForChannels (0x0003, 60));
    EXPECT_FALSE (state.isNoteOnForChannels (0x0004, 60));
    EXPECT_TRUE (state.isNoteOnForChannels (0x0001, 60));
}

TEST (UMPKeyboardStateTests, ResetClearsAllNotes)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);
    state.noteOn (2, 64, 0.7f);
    state.noteOn (3, 67, 0.9f);

    EXPECT_TRUE (state.isNoteOn (1, 60));
    EXPECT_TRUE (state.isNoteOn (2, 64));
    EXPECT_TRUE (state.isNoteOn (3, 67));

    state.reset();

    EXPECT_FALSE (state.isNoteOn (1, 60));
    EXPECT_FALSE (state.isNoteOn (2, 64));
    EXPECT_FALSE (state.isNoteOn (3, 67));
}

TEST (UMPKeyboardStateTests, AllNotesOffMethod)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (3, 60, 0.8f);
    state.noteOn (3, 64, 0.7f);
    state.noteOn (4, 67, 0.9f);

    state.allNotesOff (3);

    EXPECT_FALSE (state.isNoteOn (3, 60));
    EXPECT_FALSE (state.isNoteOn (3, 64));
    EXPECT_TRUE (state.isNoteOn (4, 67));
}

TEST (UMPKeyboardStateTests, AllNotesOffAllChannels)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);
    state.noteOn (2, 64, 0.7f);
    state.noteOn (3, 67, 0.9f);

    state.allNotesOff (0);

    EXPECT_FALSE (state.isNoteOn (1, 60));
    EXPECT_FALSE (state.isNoteOn (2, 64));
    EXPECT_FALSE (state.isNoteOn (3, 67));
}

TEST (UMPKeyboardStateTests, DISABLED_InvalidNoteNumbers)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, -1, 0.8f);
    state.noteOn (1, 128, 0.8f);
    state.noteOn (1, 200, 0.8f);

    EXPECT_FALSE (state.isNoteOn (1, -1));
    EXPECT_FALSE (state.isNoteOn (1, 128));
    EXPECT_FALSE (state.isNoteOn (1, 200));
}

TEST (UMPKeyboardStateTests, Midi1ZeroVelocityNoteOnBecomesNoteOff)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_1_0);
    state.setGroup (0);

    const auto noteOn1 = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    state.processNextUMPPacket (View (noteOn1.data));
    EXPECT_TRUE (state.isNoteOn (1, 60));

    const auto noteOnZero = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 0 } });
    state.processNextUMPPacket (View (noteOnZero.data));
    EXPECT_FALSE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, Midi2ZeroVelocityNoteOnBecomesNoteOff)
{
    UMPKeyboardState state (ump::PacketProtocol::MIDI_2_0);
    state.setGroup (0);

    const auto noteOn1 = makeMidi2NoteOnMessage (0, 1, 64, Velocity { uint16_t { 0x8000 } });
    state.processNextUMPPacket (View (noteOn1.data));
    EXPECT_TRUE (state.isNoteOn (2, 64));

    const auto noteOnZero = makeMidi2NoteOnMessage (0, 1, 64, Velocity { uint16_t { 0 } });
    state.processNextUMPPacket (View (noteOnZero.data));
    EXPECT_FALSE (state.isNoteOn (2, 64));
}

TEST (UMPKeyboardStateTests, ProcessBuffer)
{
    UMPKeyboardState state;
    state.setGroup (0);

    UMPPacketBuffer buffer;
    const auto noteOn = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
    buffer.addEvent (noteOn.data, 1, 0);

    state.processNextUMPBuffer (buffer, 0, 64, false);
    EXPECT_TRUE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, ProcessBufferWithIndirectEvents)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);

    UMPPacketBuffer buffer;
    state.processNextUMPBuffer (buffer, 0, 64, true);

    EXPECT_FALSE (buffer.isEmpty());
}

class KeyboardStateListener : public UMPKeyboardState::Listener
{
public:
    void handleNoteOn (UMPKeyboardState*, int channel, int note, float velocity) override
    {
        noteOnCalls.push_back ({ channel, note, velocity });
    }

    void handleNoteOff (UMPKeyboardState*, int channel, int note, float velocity) override
    {
        noteOffCalls.push_back ({ channel, note, velocity });
    }

    struct Event
    {
        int channel;
        int note;
        float velocity;
    };

    std::vector<Event> noteOnCalls;
    std::vector<Event> noteOffCalls;
};

TEST (UMPKeyboardStateTests, ListenerReceivesNoteOnOff)
{
    UMPKeyboardState state;
    state.setGroup (0);
    KeyboardStateListener listener;

    state.addListener (&listener);

    state.noteOn (1, 60, 0.8f);
    state.noteOff (1, 60, 0.5f);

    ASSERT_EQ (listener.noteOnCalls.size(), 1u);
    EXPECT_EQ (listener.noteOnCalls[0].channel, 1);
    EXPECT_EQ (listener.noteOnCalls[0].note, 60);
    EXPECT_FLOAT_EQ (listener.noteOnCalls[0].velocity, 0.8f);

    ASSERT_EQ (listener.noteOffCalls.size(), 1u);
    EXPECT_EQ (listener.noteOffCalls[0].channel, 1);
    EXPECT_EQ (listener.noteOffCalls[0].note, 60);
    EXPECT_FLOAT_EQ (listener.noteOffCalls[0].velocity, 0.5f);

    state.removeListener (&listener);
}

TEST (UMPKeyboardStateTests, RemovedListenerDoesNotReceiveEvents)
{
    UMPKeyboardState state;
    state.setGroup (0);
    KeyboardStateListener listener;

    state.addListener (&listener);
    state.removeListener (&listener);

    state.noteOn (1, 60, 0.8f);

    EXPECT_TRUE (listener.noteOnCalls.empty());
}

TEST (UMPKeyboardStateTests, NoteOffOnNonActiveNote)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOff (1, 60, 0.5f);
    EXPECT_FALSE (state.isNoteOn (1, 60));
}

TEST (UMPKeyboardStateTests, MultipleChannelsSameNote)
{
    UMPKeyboardState state;
    state.setGroup (0);

    state.noteOn (1, 60, 0.8f);
    state.noteOn (2, 60, 0.7f);
    state.noteOn (3, 60, 0.6f);

    EXPECT_TRUE (state.isNoteOn (1, 60));
    EXPECT_TRUE (state.isNoteOn (2, 60));
    EXPECT_TRUE (state.isNoteOn (3, 60));

    state.noteOff (2, 60, 0.5f);

    EXPECT_TRUE (state.isNoteOn (1, 60));
    EXPECT_FALSE (state.isNoteOn (2, 60));
    EXPECT_TRUE (state.isNoteOn (3, 60));
}

TEST (UMPKeyboardStateTests, IgnoresNonNotePackets)
{
    UMPKeyboardState state;
    state.setGroup (0);

    const auto cc = makeMidi1ControlChangeMessage (0, 0, 7, ControllerValue { uint7_t { 100 } });
    state.processNextUMPPacket (View (cc.data));

    EXPECT_FALSE (state.isNoteOn (1, 60));
}
