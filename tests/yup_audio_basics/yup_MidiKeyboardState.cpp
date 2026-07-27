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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
namespace
{
class TestListener : public MidiKeyboardState::Listener
{
public:
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        noteOnCalls.push_back ({ midiChannel, midiNoteNumber, velocity });
    }

    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        noteOffCalls.push_back ({ midiChannel, midiNoteNumber, velocity });
    }

    void reset()
    {
        noteOnCalls.clear();
        noteOffCalls.clear();
    }

    struct NoteEvent
    {
        int channel;
        int note;
        float velocity;
    };

    std::vector<NoteEvent> noteOnCalls;
    std::vector<NoteEvent> noteOffCalls;
};
} // namespace

//==============================================================================
class MidiKeyboardStateTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        state = std::make_unique<MidiKeyboardState>();
        listener = std::make_unique<TestListener>();
    }

    void TearDown() override
    {
        listener.reset();
        state.reset();
    }

    std::unique_ptr<MidiKeyboardState> state;
    std::unique_ptr<TestListener> listener;
};

//==============================================================================
// Constructor and Reset Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, Constructor)
{
    // Tests lines 43-46
    EXPECT_NO_THROW (MidiKeyboardState());

    // All notes should be off initially
    for (int ch = 1; ch <= 16; ++ch)
    {
        for (int note = 0; note < 128; ++note)
        {
            EXPECT_FALSE (state->isNoteOn (ch, note));
        }
    }
}

TEST_F (MidiKeyboardStateTests, Reset)
{
    // Tests lines 49-54
    state->noteOn (1, 60, 0.5f);
    state->noteOn (2, 64, 0.6f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (2, 64));

    state->reset();

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (2, 64));

    // Verify all notes are off
    for (int ch = 1; ch <= 16; ++ch)
    {
        for (int note = 0; note < 128; ++note)
        {
            EXPECT_FALSE (state->isNoteOn (ch, note));
        }
    }
}

//==============================================================================
// Note State Query Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, IsNoteOnInitially)
{
    // Tests lines 56-62
    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (16, 127));
}

TEST_F (MidiKeyboardStateTests, IsNoteOnAfterNoteOn)
{
    // Tests lines 56-62
    state->noteOn (1, 60, 0.5f);
    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (2, 60)); // Different channel
    EXPECT_FALSE (state->isNoteOn (1, 61)); // Different note
}

TEST_F (MidiKeyboardStateTests, IsNoteOnMultipleChannels)
{
    // Tests lines 56-62
    state->noteOn (1, 60, 0.5f);
    state->noteOn (5, 60, 0.6f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (5, 60));
    EXPECT_FALSE (state->isNoteOn (3, 60));
}

TEST_F (MidiKeyboardStateTests, IsNoteOnInvalidNote)
{
    // Tests lines 60-61 (bounds check)
    EXPECT_FALSE (state->isNoteOn (1, -1));
    EXPECT_FALSE (state->isNoteOn (1, 128));
    EXPECT_FALSE (state->isNoteOn (1, 200));
}

TEST_F (MidiKeyboardStateTests, IsNoteOnForChannels)
{
    // Tests lines 64-68
    state->noteOn (1, 60, 0.5f);
    state->noteOn (5, 60, 0.6f);

    EXPECT_TRUE (state->isNoteOnForChannels (0x0001, 60));  // Channel 1
    EXPECT_TRUE (state->isNoteOnForChannels (0x0010, 60));  // Channel 5
    EXPECT_TRUE (state->isNoteOnForChannels (0x0011, 60));  // Channels 1 and 5
    EXPECT_FALSE (state->isNoteOnForChannels (0x0002, 60)); // Channel 2
    EXPECT_FALSE (state->isNoteOnForChannels (0xFFFF, 61)); // All channels, wrong note
}

TEST_F (MidiKeyboardStateTests, IsNoteOnForChannelsInvalidNote)
{
    // Tests line 66 (bounds check)
    EXPECT_FALSE (state->isNoteOnForChannels (0xFFFF, -1));
    EXPECT_FALSE (state->isNoteOnForChannels (0xFFFF, 128));
}

//==============================================================================
// Note On Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, NoteOn)
{
    // Tests lines 70-85
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 1);
    EXPECT_EQ (listener->noteOnCalls[0].channel, 1);
    EXPECT_EQ (listener->noteOnCalls[0].note, 60);
    EXPECT_FLOAT_EQ (listener->noteOnCalls[0].velocity, 0.5f);
}

TEST_F (MidiKeyboardStateTests, NoteOnMultipleNotes)
{
    // Tests lines 70-85
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 64, 0.6f);
    state->noteOn (1, 67, 0.7f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (1, 64));
    EXPECT_TRUE (state->isNoteOn (1, 67));
    EXPECT_EQ (listener->noteOnCalls.size(), 3);
}

TEST_F (MidiKeyboardStateTests, NoteOnMultipleChannels)
{
    // Tests lines 70-85
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (5, 60, 0.6f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (5, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 2);
}

TEST_F (MidiKeyboardStateTests, DISABLED_NoteOnInvalidNote)
{
    // Tests lines 77-84 (bounds check)
    state->addListener (listener.get());

    state->noteOn (1, -1, 0.5f);
    state->noteOn (1, 128, 0.5f);

    EXPECT_EQ (listener->noteOnCalls.size(), 0);
}

TEST_F (MidiKeyboardStateTests, NoteOnInternal)
{
    // Tests lines 87-97
    state->addListener (listener.get());

    // Call internal method directly (would normally be called by processNextMidiEvent)
    state->processNextMidiEvent (MidiMessage::noteOn (1, 60, 0.5f));

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 1);
}

//==============================================================================
// Note Off Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, NoteOff)
{
    // Tests lines 99-111
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    listener->reset();

    state->noteOff (1, 60, 0.0f);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOffCalls.size(), 1);
    EXPECT_EQ (listener->noteOffCalls[0].channel, 1);
    EXPECT_EQ (listener->noteOffCalls[0].note, 60);
}

TEST_F (MidiKeyboardStateTests, NoteOffWithoutNoteOn)
{
    // Tests line 103 (note not on)
    state->addListener (listener.get());

    state->noteOff (1, 60, 0.0f);

    EXPECT_EQ (listener->noteOffCalls.size(), 0);
}

TEST_F (MidiKeyboardStateTests, NoteOffWrongChannel)
{
    // Tests line 103
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    listener->reset();

    state->noteOff (2, 60, 0.0f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOffCalls.size(), 0);
}

TEST_F (MidiKeyboardStateTests, NoteOffMultipleNotes)
{
    // Tests lines 99-111
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 64, 0.6f);
    state->noteOn (1, 67, 0.7f);
    listener->reset();

    state->noteOff (1, 64, 0.0f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (1, 64));
    EXPECT_TRUE (state->isNoteOn (1, 67));
    EXPECT_EQ (listener->noteOffCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, NoteOffInternal)
{
    // Tests lines 113-123
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    listener->reset();

    // Call internal method directly
    state->processNextMidiEvent (MidiMessage::noteOff (1, 60));

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOffCalls.size(), 1);
}

//==============================================================================
// All Notes Off Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, AllNotesOffSingleChannel)
{
    // Tests lines 125-139
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 64, 0.6f);
    state->noteOn (1, 67, 0.7f);
    state->noteOn (2, 72, 0.8f);
    listener->reset();

    state->allNotesOff (1);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (1, 64));
    EXPECT_FALSE (state->isNoteOn (1, 67));
    EXPECT_TRUE (state->isNoteOn (2, 72)); // Other channel unaffected
}

TEST_F (MidiKeyboardStateTests, AllNotesOffAllChannels)
{
    // Tests lines 129-133
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (5, 64, 0.6f);
    state->noteOn (10, 67, 0.7f);
    state->noteOn (16, 72, 0.8f);
    listener->reset();

    state->allNotesOff (0);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (5, 64));
    EXPECT_FALSE (state->isNoteOn (10, 67));
    EXPECT_FALSE (state->isNoteOn (16, 72));
}

TEST_F (MidiKeyboardStateTests, AllNotesOffEmptyState)
{
    // Tests lines 125-139
    state->addListener (listener.get());

    state->allNotesOff (1);

    EXPECT_EQ (listener->noteOffCalls.size(), 0);
}

//==============================================================================
// Process MIDI Event Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, ProcessNextMidiEventNoteOn)
{
    // Tests lines 141-156 (note on path)
    state->addListener (listener.get());

    MidiMessage msg = MidiMessage::noteOn (1, 60, 0.5f);
    state->processNextMidiEvent (msg);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiEventNoteOff)
{
    // Tests lines 147-150
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    listener->reset();

    MidiMessage msg = MidiMessage::noteOff (1, 60);
    state->processNextMidiEvent (msg);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOffCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiEventAllNotesOff)
{
    // Tests lines 151-155
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 64, 0.6f);
    state->noteOn (1, 67, 0.7f);
    listener->reset();

    MidiMessage msg = MidiMessage::allNotesOff (1);
    state->processNextMidiEvent (msg);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (1, 64));
    EXPECT_FALSE (state->isNoteOn (1, 67));
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiEventNonNoteMessage)
{
    // Tests lines 141-156 (no matching case)
    state->addListener (listener.get());

    MidiMessage msg = MidiMessage::controllerEvent (1, 7, 100);
    state->processNextMidiEvent (msg);

    EXPECT_EQ (listener->noteOnCalls.size(), 0);
    EXPECT_EQ (listener->noteOffCalls.size(), 0);
}

//==============================================================================
// Process MIDI Buffer Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferBasic)
{
    // Tests lines 158-181
    state->addListener (listener.get());

    MidiBuffer buffer;
    buffer.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    buffer.addEvent (MidiMessage::noteOn (1, 64, 0.6f), 10);
    buffer.addEvent (MidiMessage::noteOff (1, 60), 20);

    state->processNextMidiBuffer (buffer, 0, 100, false);

    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (1, 64));
    EXPECT_EQ (listener->noteOnCalls.size(), 2);
    EXPECT_EQ (listener->noteOffCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferWithInjectEvents)
{
    // Tests lines 168-178 (inject indirect events)
    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 64, 0.6f);

    MidiBuffer buffer;
    state->processNextMidiBuffer (buffer, 0, 100, true);

    // Should inject the noteOn events
    EXPECT_GT (buffer.getNumEvents(), 0);

    int noteCount = 0;
    for (const auto metadata : buffer)
    {
        if (metadata.getMessage().isNoteOn())
            ++noteCount;
    }

    EXPECT_EQ (noteCount, 2);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferWithoutInjectEvents)
{
    // Tests lines 158-181 (without inject)
    state->noteOn (1, 60, 0.5f);

    MidiBuffer buffer;
    state->processNextMidiBuffer (buffer, 0, 100, false);

    // Should NOT inject events
    EXPECT_EQ (buffer.getNumEvents(), 0);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferMultipleCalls)
{
    // Tests lines 158-181
    state->addListener (listener.get());

    MidiBuffer buffer1;
    buffer1.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    state->processNextMidiBuffer (buffer1, 0, 100, false);

    MidiBuffer buffer2;
    buffer2.addEvent (MidiMessage::noteOn (1, 64, 0.6f), 0);
    state->processNextMidiBuffer (buffer2, 0, 100, false);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (1, 64));
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferEmptyBuffer)
{
    // Tests lines 158-181
    state->addListener (listener.get());

    MidiBuffer buffer;
    state->processNextMidiBuffer (buffer, 0, 100, false);

    EXPECT_EQ (listener->noteOnCalls.size(), 0);
    EXPECT_EQ (listener->noteOffCalls.size(), 0);
}

TEST_F (MidiKeyboardStateTests, ProcessNextMidiBufferClearsEventsToAdd)
{
    // Tests line 180
    state->noteOn (1, 60, 0.5f);

    MidiBuffer buffer;
    state->processNextMidiBuffer (buffer, 0, 100, true);

    // Process again - should not inject same events
    MidiBuffer buffer2;
    state->processNextMidiBuffer (buffer2, 0, 100, true);

    EXPECT_EQ (buffer2.getNumEvents(), 0);
}

//==============================================================================
// Listener Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, AddListener)
{
    // Tests lines 184-188
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);

    EXPECT_EQ (listener->noteOnCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, RemoveListener)
{
    // Tests lines 190-194
    state->addListener (listener.get());
    state->removeListener (listener.get());

    state->noteOn (1, 60, 0.5f);

    EXPECT_EQ (listener->noteOnCalls.size(), 0);
}

TEST_F (MidiKeyboardStateTests, MultipleListeners)
{
    // Tests lines 184-194
    TestListener listener2;

    state->addListener (listener.get());
    state->addListener (&listener2);

    state->noteOn (1, 60, 0.5f);

    EXPECT_EQ (listener->noteOnCalls.size(), 1);
    EXPECT_EQ (listener2.noteOnCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, RemoveOneOfMultipleListeners)
{
    // Tests lines 190-194
    TestListener listener2;

    state->addListener (listener.get());
    state->addListener (&listener2);
    state->removeListener (listener.get());

    state->noteOn (1, 60, 0.5f);

    EXPECT_EQ (listener->noteOnCalls.size(), 0);
    EXPECT_EQ (listener2.noteOnCalls.size(), 1);
}

TEST_F (MidiKeyboardStateTests, ListenerNoteOnCallback)
{
    // Tests lines 92-95
    state->addListener (listener.get());

    state->noteOn (5, 72, 0.8f);

    EXPECT_EQ (listener->noteOnCalls.size(), 1);
    EXPECT_EQ (listener->noteOnCalls[0].channel, 5);
    EXPECT_EQ (listener->noteOnCalls[0].note, 72);
    EXPECT_FLOAT_EQ (listener->noteOnCalls[0].velocity, 0.8f);
}

TEST_F (MidiKeyboardStateTests, ListenerNoteOffCallback)
{
    // Tests lines 118-121
    state->addListener (listener.get());

    state->noteOn (3, 48, 0.7f);
    listener->reset();

    state->noteOff (3, 48, 0.2f);

    EXPECT_EQ (listener->noteOffCalls.size(), 1);
    EXPECT_EQ (listener->noteOffCalls[0].channel, 3);
    EXPECT_EQ (listener->noteOffCalls[0].note, 48);
    EXPECT_FLOAT_EQ (listener->noteOffCalls[0].velocity, 0.2f);
}

//==============================================================================
// Edge Case Tests
//==============================================================================
TEST_F (MidiKeyboardStateTests, AllChannelsAllNotes)
{
    // Test all 16 channels, all 128 notes
    for (int ch = 1; ch <= 16; ++ch)
    {
        for (int note = 0; note < 128; ++note)
        {
            EXPECT_FALSE (state->isNoteOn (ch, note));
            state->noteOn (ch, note, 0.5f);
            EXPECT_TRUE (state->isNoteOn (ch, note));
        }
    }

    // Turn all off
    state->allNotesOff (0);

    for (int ch = 1; ch <= 16; ++ch)
    {
        for (int note = 0; note < 128; ++note)
        {
            EXPECT_FALSE (state->isNoteOn (ch, note));
        }
    }
}

TEST_F (MidiKeyboardStateTests, SameNoteMultipleChannelsIndependent)
{
    state->noteOn (1, 60, 0.5f);
    state->noteOn (5, 60, 0.6f);
    state->noteOn (10, 60, 0.7f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (5, 60));
    EXPECT_TRUE (state->isNoteOn (10, 60));

    state->noteOff (5, 60, 0.0f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (5, 60));
    EXPECT_TRUE (state->isNoteOn (10, 60));
}

TEST_F (MidiKeyboardStateTests, RepeatedNoteOnSameNote)
{
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.5f);
    state->noteOn (1, 60, 0.6f); // Same note again
    state->noteOn (1, 60, 0.7f); // And again

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 3); // All should trigger callbacks
}

TEST_F (MidiKeyboardStateTests, ZeroVelocity)
{
    state->addListener (listener.get());

    state->noteOn (1, 60, 0.0f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 1);
    EXPECT_FLOAT_EQ (listener->noteOnCalls[0].velocity, 0.0f);
}

TEST_F (MidiKeyboardStateTests, MaxVelocity)
{
    state->addListener (listener.get());

    state->noteOn (1, 60, 1.0f);

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_EQ (listener->noteOnCalls.size(), 1);
    EXPECT_FLOAT_EQ (listener->noteOnCalls[0].velocity, 1.0f);
}

TEST_F (MidiKeyboardStateTests, ChannelBoundaries)
{
    state->noteOn (1, 60, 0.5f);  // Min channel
    state->noteOn (16, 60, 0.5f); // Max channel

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (16, 60));
}

TEST_F (MidiKeyboardStateTests, NoteBoundaries)
{
    state->noteOn (1, 0, 0.5f);   // Min note
    state->noteOn (1, 127, 0.5f); // Max note

    EXPECT_TRUE (state->isNoteOn (1, 0));
    EXPECT_TRUE (state->isNoteOn (1, 127));
}

TEST_F (MidiKeyboardStateTests, ThreadSafety)
{
    // Basic thread safety test with concurrent access
    state->addListener (listener.get());

    std::thread t1 ([this]()
    {
        for (int i = 0; i < 100; ++i)
        {
            state->noteOn (1, 60, 0.5f);
            state->noteOff (1, 60, 0.0f);
        }
    });

    std::thread t2 ([this]()
    {
        for (int i = 0; i < 100; ++i)
        {
            state->noteOn (2, 64, 0.5f);
            state->noteOff (2, 64, 0.0f);
        }
    });

    t1.join();
    t2.join();

    // Should not crash and state should be consistent
    EXPECT_FALSE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (2, 64));
}

TEST_F (MidiKeyboardStateTests, ComplexSequence)
{
    state->addListener (listener.get());

    // Simulate a musical sequence
    state->noteOn (1, 60, 0.8f); // C
    state->noteOn (1, 64, 0.7f); // E
    state->noteOn (1, 67, 0.6f); // G

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (1, 64));
    EXPECT_TRUE (state->isNoteOn (1, 67));

    state->noteOff (1, 64, 0.0f); // Release E

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_FALSE (state->isNoteOn (1, 64));
    EXPECT_TRUE (state->isNoteOn (1, 67));

    state->noteOn (1, 65, 0.7f); // F

    EXPECT_TRUE (state->isNoteOn (1, 60));
    EXPECT_TRUE (state->isNoteOn (1, 65));
    EXPECT_TRUE (state->isNoteOn (1, 67));

    state->allNotesOff (1);

    for (int note = 0; note < 128; ++note)
    {
        EXPECT_FALSE (state->isNoteOn (1, note));
    }
}
