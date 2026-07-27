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
struct ControlValue
{
    int control, value;
};

struct DataEntry
{
    int controllerBase, channel, parameter, value;
    double time;

    std::array<ControlValue, 4> getControlValues() const
    {
        return { { { controllerBase + 1, (parameter >> 7) & 0x7f },
                   { controllerBase + 0, (parameter >> 0) & 0x7f },
                   { 0x06, (value >> 7) & 0x7f },
                   { 0x26, (value >> 0) & 0x7f } } };
    }

    void addToSequence (MidiMessageSequence& s) const
    {
        for (const auto& pair : getControlValues())
            s.addEvent (MidiMessage::controllerEvent (channel, pair.control, pair.value), time);
    }

    bool matches (const MidiMessage* begin, const MidiMessage* end) const
    {
        const auto isEqual = [this] (const ControlValue& cv, const MidiMessage& msg)
        {
            return exactlyEqual (msg.getTimeStamp(), time)
                && msg.isController()
                && msg.getChannel() == channel
                && msg.getControllerNumber() == cv.control
                && msg.getControllerValue() == cv.value;
        };

        const auto pairs = getControlValues();
        return std::equal (pairs.begin(), pairs.end(), begin, end, isEqual);
    }
};

bool messagesAreEqual (const MidiMessage& a, const MidiMessage& b)
{
    return a.getDescription() == b.getDescription()
        && exactlyEqual (a.getTimeStamp(), b.getTimeStamp());
}

} // namespace

class MidiMessageSequenceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        s.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
        s.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));
        s.addEvent (MidiMessage::noteOn (1, 30, 0.5f).withTimeStamp (2.0));
        s.addEvent (MidiMessage::noteOff (1, 30, 0.5f).withTimeStamp (8.0));
    }

    void addNrpn (MidiMessageSequence& seq, int channel, int parameter, int value, double time = 0.0)
    {
        DataEntry { 0x62, channel, parameter, value, time }.addToSequence (seq);
    }

    void addRpn (MidiMessageSequence& seq, int channel, int parameter, int value, double time = 0.0)
    {
        DataEntry { 0x64, channel, parameter, value, time }.addToSequence (seq);
    }

    void checkNrpn (const MidiMessage* begin, const MidiMessage* end, int channel, int parameter, int value, double time = 0.0)
    {
        auto result = DataEntry { 0x62, channel, parameter, value, time }.matches (begin, end);
        EXPECT_TRUE (result);
    }

    void checkRpn (const MidiMessage* begin, const MidiMessage* end, int channel, int parameter, int value, double time = 0.0)
    {
        auto result = DataEntry { 0x64, channel, parameter, value, time }.matches (begin, end);
        EXPECT_TRUE (result);
    }

    MidiMessageSequence s;
};

TEST_F (MidiMessageSequenceTest, StartAndEndTime)
{
    EXPECT_EQ (s.getStartTime(), 0.0);
    EXPECT_EQ (s.getEndTime(), 8.0);
    EXPECT_EQ (s.getEventTime (1), 2.0);
}

TEST_F (MidiMessageSequenceTest, MatchingNoteOffAndOns)
{
    s.updateMatchedPairs();
    EXPECT_EQ (s.getTimeOfMatchingKeyUp (0), 4.0);
    EXPECT_EQ (s.getTimeOfMatchingKeyUp (1), 8.0);
    EXPECT_EQ (s.getIndexOfMatchingKeyUp (0), 2);
    EXPECT_EQ (s.getIndexOfMatchingKeyUp (1), 3);
}

TEST_F (MidiMessageSequenceTest, TimeAndIndices)
{
    EXPECT_EQ (s.getNextIndexAtTime (0.5), 1);
    EXPECT_EQ (s.getNextIndexAtTime (2.5), 2);
    EXPECT_EQ (s.getNextIndexAtTime (9.0), 4);
}

TEST_F (MidiMessageSequenceTest, DeletingEventsWithoutMatchedPairs)
{
    s.deleteEvent (0, true);
    EXPECT_EQ (s.getNumEvents(), 3);
}

TEST_F (MidiMessageSequenceTest, DeletingEventsWithMatchedPairs)
{
    s.updateMatchedPairs();
    s.deleteEvent (0, true);
    EXPECT_EQ (s.getNumEvents(), 2);
}

TEST_F (MidiMessageSequenceTest, MergingSequences)
{
    s.updateMatchedPairs();
    s.deleteEvent (0, true);

    MidiMessageSequence s2;
    s2.addEvent (MidiMessage::noteOn (2, 25, 0.5f).withTimeStamp (0.0));
    s2.addEvent (MidiMessage::noteOn (2, 40, 0.5f).withTimeStamp (1.0));
    s2.addEvent (MidiMessage::noteOff (2, 40, 0.5f).withTimeStamp (5.0));
    s2.addEvent (MidiMessage::noteOn (2, 80, 0.5f).withTimeStamp (3.0));
    s2.addEvent (MidiMessage::noteOff (2, 80, 0.5f).withTimeStamp (7.0));
    s2.addEvent (MidiMessage::noteOff (2, 25, 0.5f).withTimeStamp (9.0));

    s.addSequence (s2, 0.0, 0.0, 8.0); // Intentionally cut off the last note off
    s.updateMatchedPairs();

    EXPECT_EQ (s.getNumEvents(), 7);
    EXPECT_EQ (s.getIndexOfMatchingKeyUp (0), -1); // Truncated note, should be no note off
    EXPECT_EQ (s.getTimeOfMatchingKeyUp (1), 5.0);
}

TEST_F (MidiMessageSequenceTest, CreateControllerUpdatesForTimeEmitsNRPNComponentsInCorrectOrder)
{
    const auto channel = 1;
    const auto number = 200;
    const auto value = 300;

    MidiMessageSequence sequence;
    addNrpn (sequence, channel, number, value);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime (channel, 1.0, m);

    checkNrpn (m.begin(), m.end(), channel, number, value);
}

TEST_F (MidiMessageSequenceTest, CreateControllerUpdatesForTimeIgnoresNRPNsAfterFinalRequestedTime)
{
    const auto channel = 2;
    const auto number = 123;
    const auto value = 456;

    MidiMessageSequence sequence;
    addRpn (sequence, channel, number, value, 0.5);
    addRpn (sequence, channel, 111, 222, 1.5);
    addRpn (sequence, channel, 333, 444, 2.5);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime (channel, 1.0, m);

    checkRpn (m.begin(), std::next (m.begin(), 4), channel, number, value, 0.5);
}

TEST_F (MidiMessageSequenceTest, CreateControllerUpdatesForTimeEmitsSeparateNRPNMessagesWhenAppropriate)
{
    const auto channel = 2;
    const auto numberA = 1111;
    const auto valueA = 9999;

    const auto numberB = 8888;
    const auto valueB = 2222;

    const auto numberC = 7777;
    const auto valueC = 3333;

    const auto numberD = 6666;
    const auto valueD = 4444;

    const auto time = 0.5;

    MidiMessageSequence sequence;
    addRpn (sequence, channel, numberA, valueA, time);
    addRpn (sequence, channel, numberB, valueB, time);
    addNrpn (sequence, channel, numberC, valueC, time);
    addNrpn (sequence, channel, numberD, valueD, time);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime (channel, time * 2, m);

    checkRpn (std::next (m.begin(), 0), std::next (m.begin(), 4), channel, numberA, valueA, time);
    checkRpn (std::next (m.begin(), 4), std::next (m.begin(), 8), channel, numberB, valueB, time);
    checkNrpn (std::next (m.begin(), 8), std::next (m.begin(), 12), channel, numberC, valueC, time);
    checkNrpn (std::next (m.begin(), 12), std::next (m.begin(), 16), channel, numberD, valueD, time);
}

//==============================================================================
// MidiEventHolder tests (via public interface)
TEST (MidiEventHolderTests, EventHolderViaAddEvent)
{
    MidiMessageSequence seq;
    MidiMessage msg = MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0);

    auto* holder = seq.addEvent (msg);

    EXPECT_NE (holder, nullptr);
    EXPECT_TRUE (messagesAreEqual (holder->message, msg));
    EXPECT_EQ (holder->noteOffObject, nullptr);
}

TEST (MidiEventHolderTests, NoteOffObjectAfterUpdateMatchedPairs)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    seq.updateMatchedPairs();

    auto* noteOnHolder = seq.getEventPointer (0);
    auto* noteOffHolder = seq.getEventPointer (1);

    EXPECT_NE (noteOnHolder, nullptr);
    EXPECT_NE (noteOffHolder, nullptr);
    EXPECT_EQ (noteOnHolder->noteOffObject, noteOffHolder);
}

//==============================================================================
// Constructor and assignment tests
TEST (MidiMessageSequenceTests, DefaultConstructor)
{
    EXPECT_NO_THROW (MidiMessageSequence());

    MidiMessageSequence seq;
    EXPECT_EQ (seq.getNumEvents(), 0);
}

TEST (MidiMessageSequenceTests, CopyConstructor)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq1.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));
    seq1.updateMatchedPairs();

    MidiMessageSequence seq2 (seq1);

    EXPECT_EQ (seq2.getNumEvents(), 2);
    EXPECT_EQ (seq2.getEventTime (0), 0.0);
    EXPECT_EQ (seq2.getEventTime (1), 4.0);
    EXPECT_EQ (seq2.getTimeOfMatchingKeyUp (0), 4.0);
}

TEST (MidiMessageSequenceTests, CopyAssignment)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq1.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));
    seq1.updateMatchedPairs();

    MidiMessageSequence seq2;
    seq2 = seq1;

    EXPECT_EQ (seq2.getNumEvents(), 2);
    EXPECT_EQ (seq2.getEventTime (0), 0.0);
    EXPECT_EQ (seq2.getEventTime (1), 4.0);
    EXPECT_EQ (seq2.getTimeOfMatchingKeyUp (0), 4.0);
}

TEST (MidiMessageSequenceTests, MoveConstructor)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq1.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    MidiMessageSequence seq2 (std::move (seq1));

    EXPECT_EQ (seq2.getNumEvents(), 2);
    EXPECT_EQ (seq2.getEventTime (0), 0.0);
    EXPECT_EQ (seq2.getEventTime (1), 4.0);
}

TEST (MidiMessageSequenceTests, MoveAssignment)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq1.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    MidiMessageSequence seq2;
    seq2 = std::move (seq1);

    EXPECT_EQ (seq2.getNumEvents(), 2);
    EXPECT_EQ (seq2.getEventTime (0), 0.0);
    EXPECT_EQ (seq2.getEventTime (1), 4.0);
}

TEST (MidiMessageSequenceTests, SwapWith)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq1.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    MidiMessageSequence seq2;
    seq2.addEvent (MidiMessage::noteOn (2, 70, 0.6f).withTimeStamp (1.0));

    seq1.swapWith (seq2);

    EXPECT_EQ (seq1.getNumEvents(), 1);
    EXPECT_EQ (seq1.getEventTime (0), 1.0);
    EXPECT_EQ (seq2.getNumEvents(), 2);
    EXPECT_EQ (seq2.getEventTime (0), 0.0);
}

//==============================================================================
// Basic operations
TEST (MidiMessageSequenceTests, Clear)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    EXPECT_EQ (seq.getNumEvents(), 2);

    seq.clear();

    EXPECT_EQ (seq.getNumEvents(), 0);
}

TEST (MidiMessageSequenceTests, GetNumEvents)
{
    MidiMessageSequence seq;
    EXPECT_EQ (seq.getNumEvents(), 0);

    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    EXPECT_EQ (seq.getNumEvents(), 1);

    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));
    EXPECT_EQ (seq.getNumEvents(), 2);
}

TEST (MidiMessageSequenceTests, GetEventPointer)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    auto* event = seq.getEventPointer (0);
    EXPECT_NE (event, nullptr);
    EXPECT_TRUE (event->message.isNoteOn());
    EXPECT_EQ (event->message.getNoteNumber(), 60);
}

TEST (MidiMessageSequenceTests, GetEventPointerOutOfRange)
{
    MidiMessageSequence seq;

    auto* event = seq.getEventPointer (0);
    EXPECT_EQ (event, nullptr);

    auto* event2 = seq.getEventPointer (100);
    EXPECT_EQ (event2, nullptr);
}

TEST (MidiMessageSequenceTests, BeginEndIterators)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    int count = 0;
    for (auto* event : seq)
    {
        EXPECT_NE (event, nullptr);
        ++count;
    }

    EXPECT_EQ (count, 2);
}

TEST (MidiMessageSequenceTests, ConstBeginEndIterators)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    const auto& constSeq = seq;

    int count = 0;
    for (auto* event : constSeq)
    {
        EXPECT_NE (event, nullptr);
        ++count;
    }

    EXPECT_EQ (count, 2);
}

TEST (MidiMessageSequenceTests, GetIndexOf)
{
    MidiMessageSequence seq;
    auto* event1 = seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    auto* event2 = seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    EXPECT_EQ (seq.getIndexOf (event1), 0);
    EXPECT_EQ (seq.getIndexOf (event2), 1);
}

TEST (MidiMessageSequenceTests, GetIndexOfNonExistentEvent)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    // Test with nullptr
    EXPECT_EQ (seq.getIndexOf (nullptr), -1);
}

TEST (MidiMessageSequenceTests, GetEventTimeValidIndex)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (3.5));

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 3.5);
}

TEST (MidiMessageSequenceTests, GetEventTimeInvalidIndex)
{
    MidiMessageSequence seq;

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 0.0);
    EXPECT_DOUBLE_EQ (seq.getEventTime (100), 0.0);
}

TEST (MidiMessageSequenceTests, GetStartTimeEmptySequence)
{
    MidiMessageSequence seq;
    EXPECT_DOUBLE_EQ (seq.getStartTime(), 0.0);
}

TEST (MidiMessageSequenceTests, GetEndTimeEmptySequence)
{
    MidiMessageSequence seq;
    EXPECT_DOUBLE_EQ (seq.getEndTime(), 0.0);
}

//==============================================================================
// Add event tests
TEST (MidiMessageSequenceTests, AddEventWithConstRef)
{
    MidiMessageSequence seq;
    MidiMessage msg = MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0);

    auto* event = seq.addEvent (msg);

    EXPECT_NE (event, nullptr);
    EXPECT_EQ (seq.getNumEvents(), 1);
    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 2.0);
}

TEST (MidiMessageSequenceTests, AddEventWithMove)
{
    MidiMessageSequence seq;

    auto* event = seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0));

    EXPECT_NE (event, nullptr);
    EXPECT_EQ (seq.getNumEvents(), 1);
    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 2.0);
}

TEST (MidiMessageSequenceTests, AddEventWithTimeAdjustment)
{
    MidiMessageSequence seq;
    MidiMessage msg = MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0);

    seq.addEvent (msg, 1.5);

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 3.5);
}

TEST (MidiMessageSequenceTests, AddEventsMaintainsOrder)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0));
    seq.addEvent (MidiMessage::noteOn (1, 62, 0.5f).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOn (1, 64, 0.5f).withTimeStamp (3.0));

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 1.0);
    EXPECT_DOUBLE_EQ (seq.getEventTime (1), 2.0);
    EXPECT_DOUBLE_EQ (seq.getEventTime (2), 3.0);
}

//==============================================================================
// Delete event tests
TEST (MidiMessageSequenceTests, DeleteEventValidIndex)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    EXPECT_EQ (seq.getNumEvents(), 2);

    seq.deleteEvent (0, false);

    EXPECT_EQ (seq.getNumEvents(), 1);
    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 4.0);
}

TEST (MidiMessageSequenceTests, DeleteEventInvalidIndex)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    EXPECT_NO_THROW (seq.deleteEvent (100, false));
    EXPECT_EQ (seq.getNumEvents(), 1);
}

//==============================================================================
// Update matched pairs tests
TEST (MidiMessageSequenceTests, UpdateMatchedPairsConsecutiveNoteOns)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    seq.updateMatchedPairs();

    // First note-on should get a synthetic note-off at time 2.0
    EXPECT_EQ (seq.getNumEvents(), 4);
    EXPECT_DOUBLE_EQ (seq.getTimeOfMatchingKeyUp (0), 2.0);
    EXPECT_EQ (seq.getIndexOfMatchingKeyUp (0), 1);
}

TEST (MidiMessageSequenceTests, UpdateMatchedPairsDifferentChannels)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (2, 60, 0.5f).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (2.0));
    seq.addEvent (MidiMessage::noteOff (2, 60, 0.5f).withTimeStamp (3.0));

    seq.updateMatchedPairs();

    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (0), 2.0);
    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (1), 3.0);
}

TEST (MidiMessageSequenceTests, UpdateMatchedPairsDifferentNotes)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (1, 62, 0.5f).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (2.0));
    seq.addEvent (MidiMessage::noteOff (1, 62, 0.5f).withTimeStamp (3.0));

    seq.updateMatchedPairs();

    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (0), 2.0);
    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (1), 3.0);
}

TEST (MidiMessageSequenceTests, UpdateMatchedPairsUnmatchedNoteOn)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (1, 62, 0.5f).withTimeStamp (1.0));

    seq.updateMatchedPairs();

    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (0), 0.0);
    EXPECT_EQ (seq.getTimeOfMatchingKeyUp (1), 0.0);
}

TEST (MidiMessageSequenceTests, GetIndexOfMatchingKeyUpInvalidIndex)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    EXPECT_EQ (seq.getIndexOfMatchingKeyUp (100), -1);
}

//==============================================================================
// Time manipulation tests
TEST (MidiMessageSequenceTests, AddTimeToMessages)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (4.0));

    seq.addTimeToMessages (2.5);

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 2.5);
    EXPECT_DOUBLE_EQ (seq.getEventTime (1), 6.5);
}

TEST (MidiMessageSequenceTests, AddTimeToMessagesNegative)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (5.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (9.0));

    seq.addTimeToMessages (-2.0);

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 3.0);
    EXPECT_DOUBLE_EQ (seq.getEventTime (1), 7.0);
}

TEST (MidiMessageSequenceTests, AddTimeToMessagesZero)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (5.0));
    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (9.0));

    seq.addTimeToMessages (0.0);

    EXPECT_DOUBLE_EQ (seq.getEventTime (0), 5.0);
    EXPECT_DOUBLE_EQ (seq.getEventTime (1), 9.0);
}

//==============================================================================
// Add sequence tests
TEST (MidiMessageSequenceTests, AddSequenceSimple)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    MidiMessageSequence seq2;
    seq2.addEvent (MidiMessage::noteOn (2, 70, 0.6f).withTimeStamp (1.0));
    seq2.addEvent (MidiMessage::noteOff (2, 70, 0.6f).withTimeStamp (5.0));

    seq1.addSequence (seq2, 0.0);

    EXPECT_EQ (seq1.getNumEvents(), 3);
}

TEST (MidiMessageSequenceTests, AddSequenceWithTimeAdjustment)
{
    MidiMessageSequence seq1;
    seq1.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    MidiMessageSequence seq2;
    seq2.addEvent (MidiMessage::noteOn (2, 70, 0.6f).withTimeStamp (1.0));

    seq1.addSequence (seq2, 2.5);

    EXPECT_EQ (seq1.getNumEvents(), 2);
    EXPECT_DOUBLE_EQ (seq1.getEventTime (1), 3.5);
}

TEST (MidiMessageSequenceTests, AddSequenceWithTimeRangeInclusive)
{
    MidiMessageSequence seq1;

    MidiMessageSequence seq2;
    seq2.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq2.addEvent (MidiMessage::noteOn (1, 62, 0.5f).withTimeStamp (2.0));
    seq2.addEvent (MidiMessage::noteOn (1, 64, 0.5f).withTimeStamp (4.0));
    seq2.addEvent (MidiMessage::noteOn (1, 65, 0.5f).withTimeStamp (6.0));

    seq1.addSequence (seq2, 0.0, 2.0, 6.0);

    EXPECT_EQ (seq1.getNumEvents(), 2); // Only events at 2.0 and 4.0
    EXPECT_DOUBLE_EQ (seq1.getEventTime (0), 2.0);
    EXPECT_DOUBLE_EQ (seq1.getEventTime (1), 4.0);
}

TEST (MidiMessageSequenceTests, AddSequenceWithTimeRangeAndAdjustment)
{
    MidiMessageSequence seq1;

    MidiMessageSequence seq2;
    seq2.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq2.addEvent (MidiMessage::noteOn (1, 62, 0.5f).withTimeStamp (2.0));
    seq2.addEvent (MidiMessage::noteOn (1, 64, 0.5f).withTimeStamp (4.0));

    // addSequence (other, timeAdjustment, firstAllowableTime, endOfAllowableDestTimes)
    // For each event: t = event.time + timeAdjustment
    // Include if: t >= firstAllowableTime && t < endOfAllowableDestTimes
    seq1.addSequence (seq2, 1.0, 1.0, 4.0);

    // Event at 0.0 + 1.0 = 1.0 (included: 1.0 >= 1.0 && 1.0 < 4.0)
    // Event at 2.0 + 1.0 = 3.0 (included: 3.0 >= 1.0 && 3.0 < 4.0)
    // Event at 4.0 + 1.0 = 5.0 (excluded: 5.0 >= 4.0)
    EXPECT_EQ (seq1.getNumEvents(), 2);
    EXPECT_DOUBLE_EQ (seq1.getEventTime (0), 1.0); // 0.0 + 1.0
    EXPECT_DOUBLE_EQ (seq1.getEventTime (1), 3.0); // 2.0 + 1.0
}

//==============================================================================
// Sort tests
TEST (MidiMessageSequenceTests, SortMaintainsStability)
{
    MidiMessageSequence seq;
    auto* event1 = seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (2.0));
    auto* event2 = seq.addEvent (MidiMessage::noteOn (1, 62, 0.6f).withTimeStamp (2.0));
    auto* event3 = seq.addEvent (MidiMessage::noteOn (1, 64, 0.7f).withTimeStamp (2.0));

    // All at same time, should maintain insertion order
    EXPECT_EQ (seq.getEventPointer (0), event1);
    EXPECT_EQ (seq.getEventPointer (1), event2);
    EXPECT_EQ (seq.getEventPointer (2), event3);
}

//==============================================================================
// Extract/delete channel messages tests
TEST (MidiMessageSequenceTests, ExtractMidiChannelMessages)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (2, 62, 0.5f).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOn (3, 64, 0.5f).withTimeStamp (2.0));

    MidiMessageSequence extracted;
    seq.extractMidiChannelMessages (2, extracted, false);

    EXPECT_EQ (extracted.getNumEvents(), 1);
    EXPECT_EQ (extracted.getEventPointer (0)->message.getChannel(), 2);
}

TEST (MidiMessageSequenceTests, ExtractMidiChannelMessagesWithMetaEvents)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::tempoMetaEvent (120).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOn (2, 62, 0.5f).withTimeStamp (2.0));

    MidiMessageSequence extracted;
    seq.extractMidiChannelMessages (1, extracted, true);

    EXPECT_EQ (extracted.getNumEvents(), 2); // Note on channel 1 + tempo meta event
}

TEST (MidiMessageSequenceTests, ExtractMidiChannelMessagesNoMetaEvents)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::tempoMetaEvent (120).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOn (2, 62, 0.5f).withTimeStamp (2.0));

    MidiMessageSequence extracted;
    seq.extractMidiChannelMessages (1, extracted, false);

    EXPECT_EQ (extracted.getNumEvents(), 1); // Only note on channel 1
}

TEST (MidiMessageSequenceTests, ExtractSysExMessages)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    uint8 sysexData[] = { 0xf0, 0x43, 0x12, 0x00, 0xf7 };
    seq.addEvent (MidiMessage::createSysExMessage (sysexData, sizeof (sysexData)).withTimeStamp (1.0));

    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (2.0));

    MidiMessageSequence extracted;
    seq.extractSysExMessages (extracted);

    EXPECT_EQ (extracted.getNumEvents(), 1);
    EXPECT_TRUE (extracted.getEventPointer (0)->message.isSysEx());
}

TEST (MidiMessageSequenceTests, DeleteMidiChannelMessages)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));
    seq.addEvent (MidiMessage::noteOn (2, 62, 0.5f).withTimeStamp (1.0));
    seq.addEvent (MidiMessage::noteOn (1, 64, 0.5f).withTimeStamp (2.0));
    seq.addEvent (MidiMessage::noteOn (3, 65, 0.5f).withTimeStamp (3.0));

    EXPECT_EQ (seq.getNumEvents(), 4);

    seq.deleteMidiChannelMessages (1);

    EXPECT_EQ (seq.getNumEvents(), 2);
    EXPECT_EQ (seq.getEventPointer (0)->message.getChannel(), 2);
    EXPECT_EQ (seq.getEventPointer (1)->message.getChannel(), 3);
}

TEST (MidiMessageSequenceTests, DeleteSysExMessages)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::noteOn (1, 60, 0.5f).withTimeStamp (0.0));

    uint8 sysexData1[] = { 0xf0, 0x43, 0x12, 0x00, 0xf7 };
    seq.addEvent (MidiMessage::createSysExMessage (sysexData1, sizeof (sysexData1)).withTimeStamp (1.0));

    seq.addEvent (MidiMessage::noteOff (1, 60, 0.5f).withTimeStamp (2.0));

    uint8 sysexData2[] = { 0xf0, 0x7e, 0x00, 0x09, 0x01, 0xf7 };
    seq.addEvent (MidiMessage::createSysExMessage (sysexData2, sizeof (sysexData2)).withTimeStamp (3.0));

    EXPECT_EQ (seq.getNumEvents(), 4);

    seq.deleteSysExMessages();

    EXPECT_EQ (seq.getNumEvents(), 2);
    EXPECT_TRUE (seq.getEventPointer (0)->message.isNoteOn());
    EXPECT_TRUE (seq.getEventPointer (1)->message.isNoteOff());
}

//==============================================================================
// CreateControllerUpdatesForTime additional tests
TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimePitchWheel)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::pitchWheel (1, 4096).withTimeStamp (0.5));
    seq.addEvent (MidiMessage::pitchWheel (1, 12000).withTimeStamp (1.0));

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 2.0, messages);

    // Should have the latest pitch wheel value
    EXPECT_GE (messages.size(), 1);

    bool foundPitchWheel = false;
    for (const auto& msg : messages)
    {
        if (msg.isPitchWheel())
        {
            EXPECT_EQ (msg.getPitchWheelValue(), 12000);
            foundPitchWheel = true;
        }
    }

    EXPECT_TRUE (foundPitchWheel);
}

TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimeProgramChange)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::programChange (1, 10).withTimeStamp (0.5));
    seq.addEvent (MidiMessage::programChange (1, 42).withTimeStamp (1.0));

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 2.0, messages);

    // Should have the latest program change
    bool foundProgramChange = false;
    for (const auto& msg : messages)
    {
        if (msg.isProgramChange())
        {
            EXPECT_EQ (msg.getProgramChangeNumber(), 42);
            foundProgramChange = true;
        }
    }

    EXPECT_TRUE (foundProgramChange);
}

TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimeProgramChangeWithBank)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::controllerEvent (1, 0x00, 5).withTimeStamp (0.5));  // Bank MSB
    seq.addEvent (MidiMessage::controllerEvent (1, 0x20, 10).withTimeStamp (0.5)); // Bank LSB
    seq.addEvent (MidiMessage::programChange (1, 42).withTimeStamp (1.0));

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 2.0, messages);

    // Should have bank MSB, bank LSB, and program change
    int bankMSBCount = 0;
    int bankLSBCount = 0;
    int programChangeCount = 0;

    for (const auto& msg : messages)
    {
        if (msg.isController())
        {
            if (msg.getControllerNumber() == 0x00)
            {
                EXPECT_EQ (msg.getControllerValue(), 5);
                ++bankMSBCount;
            }
            else if (msg.getControllerNumber() == 0x20)
            {
                EXPECT_EQ (msg.getControllerValue(), 10);
                ++bankLSBCount;
            }
        }
        else if (msg.isProgramChange())
        {
            EXPECT_EQ (msg.getProgramChangeNumber(), 42);
            ++programChangeCount;
        }
    }

    EXPECT_EQ (bankMSBCount, 1);
    EXPECT_EQ (bankLSBCount, 1);
    EXPECT_EQ (programChangeCount, 1);
}

TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimeRegularControllers)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::controllerEvent (1, 7, 100).withTimeStamp (0.5)); // Volume
    seq.addEvent (MidiMessage::controllerEvent (1, 10, 64).withTimeStamp (1.0)); // Pan
    seq.addEvent (MidiMessage::controllerEvent (1, 7, 127).withTimeStamp (1.5)); // Volume again

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 2.0, messages);

    // Should have the latest values for controllers 7 and 10
    int volumeCount = 0;
    int panCount = 0;

    for (const auto& msg : messages)
    {
        if (msg.isController())
        {
            if (msg.getControllerNumber() == 7)
            {
                EXPECT_EQ (msg.getControllerValue(), 127); // Latest volume
                ++volumeCount;
            }
            else if (msg.getControllerNumber() == 10)
            {
                EXPECT_EQ (msg.getControllerValue(), 64);
                ++panCount;
            }
        }
    }

    EXPECT_EQ (volumeCount, 1);
    EXPECT_EQ (panCount, 1);
}

TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimeIgnoresFutureEvents)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::controllerEvent (1, 7, 100).withTimeStamp (0.5));
    seq.addEvent (MidiMessage::controllerEvent (1, 7, 50).withTimeStamp (2.5));

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 1.0, messages);

    // Should only see the first controller value
    for (const auto& msg : messages)
    {
        if (msg.isController() && msg.getControllerNumber() == 7)
        {
            EXPECT_EQ (msg.getControllerValue(), 100);
        }
    }
}

TEST (MidiMessageSequenceTests, CreateControllerUpdatesForTimeDifferentChannel)
{
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::controllerEvent (1, 7, 100).withTimeStamp (0.5));
    seq.addEvent (MidiMessage::controllerEvent (2, 7, 50).withTimeStamp (1.0));

    Array<MidiMessage> messages;
    seq.createControllerUpdatesForTime (1, 2.0, messages);

    // Should only see controller from channel 1
    for (const auto& msg : messages)
    {
        EXPECT_EQ (msg.getChannel(), 1);
        if (msg.isController() && msg.getControllerNumber() == 7)
        {
            EXPECT_EQ (msg.getControllerValue(), 100);
        }
    }
}
