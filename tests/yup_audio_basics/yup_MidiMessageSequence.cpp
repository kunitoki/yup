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

#if 0

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

    void addToSequence(MidiMessageSequence& s) const
    {
        for (const auto& pair : getControlValues())
            s.addEvent(MidiMessage::controllerEvent(channel, pair.control, pair.value), time);
    }

    bool matches(const MidiMessage* begin, const MidiMessage* end) const
    {
        const auto isEqual = [this](const ControlValue& cv, const MidiMessage& msg)
        {
            return exactlyEqual(msg.getTimeStamp(), time)
                && msg.isController()
                && msg.getChannel() == channel
                && msg.getControllerNumber() == cv.control
                && msg.getControllerValue() == cv.value;
        };

        const auto pairs = getControlValues();
        return std::equal(pairs.begin(), pairs.end(), begin, end, isEqual);
    }
};

bool messagesAreEqual(const MidiMessage& a, const MidiMessage& b)
{
    return a.getDescription() == b.getDescription()
        && exactlyEqual(a.getTimeStamp(), b.getTimeStamp());
}

} // namespace

class MidiMessageSequenceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        s.addEvent(MidiMessage::noteOn(1, 60, 0.5f).withTimeStamp(0.0));
        s.addEvent(MidiMessage::noteOff(1, 60, 0.5f).withTimeStamp(4.0));
        s.addEvent(MidiMessage::noteOn(1, 30, 0.5f).withTimeStamp(2.0));
        s.addEvent(MidiMessage::noteOff(1, 30, 0.5f).withTimeStamp(8.0));
    }

    void addNrpn(MidiMessageSequence& seq, int channel, int parameter, int value, double time = 0.0)
    {
        DataEntry{ 0x62, channel, parameter, value, time }.addToSequence(seq);
    }

    void addRpn(MidiMessageSequence& seq, int channel, int parameter, int value, double time = 0.0)
    {
        DataEntry{ 0x64, channel, parameter, value, time }.addToSequence(seq);
    }

    void checkNrpn(const MidiMessage* begin, const MidiMessage* end, int channel, int parameter, int value, double time = 0.0)
    {
        auto result = DataEntry{ 0x62, channel, parameter, value, time }.matches(begin, end);
        EXPECT_TRUE(result);
    }

    void checkRpn(const MidiMessage* begin, const MidiMessage* end, int channel, int parameter, int value, double time = 0.0)
    {
        auto result = DataEntry{ 0x64, channel, parameter, value, time }.matches(begin, end);
        EXPECT_TRUE(result);
    }

    MidiMessageSequence s;
};

TEST_F(MidiMessageSequenceTest, StartAndEndTime)
{
    EXPECT_EQ(s.getStartTime(), 0.0);
    EXPECT_EQ(s.getEndTime(), 8.0);
    EXPECT_EQ(s.getEventTime(1), 2.0);
}

TEST_F(MidiMessageSequenceTest, MatchingNoteOffAndOns)
{
    s.updateMatchedPairs();
    EXPECT_EQ(s.getTimeOfMatchingKeyUp(0), 4.0);
    EXPECT_EQ(s.getTimeOfMatchingKeyUp(1), 8.0);
    EXPECT_EQ(s.getIndexOfMatchingKeyUp(0), 2);
    EXPECT_EQ(s.getIndexOfMatchingKeyUp(1), 3);
}

TEST_F(MidiMessageSequenceTest, TimeAndIndices)
{
    EXPECT_EQ(s.getNextIndexAtTime(0.5), 1);
    EXPECT_EQ(s.getNextIndexAtTime(2.5), 2);
    EXPECT_EQ(s.getNextIndexAtTime(9.0), 4);
}

TEST_F(MidiMessageSequenceTest, DeletingEvents)
{
    s.deleteEvent(0, true);
    EXPECT_EQ(s.getNumEvents(), 2);
}

TEST_F(MidiMessageSequenceTest, MergingSequences)
{
    MidiMessageSequence s2;
    s2.addEvent(MidiMessage::noteOn(2, 25, 0.5f).withTimeStamp(0.0));
    s2.addEvent(MidiMessage::noteOn(2, 40, 0.5f).withTimeStamp(1.0));
    s2.addEvent(MidiMessage::noteOff(2, 40, 0.5f).withTimeStamp(5.0));
    s2.addEvent(MidiMessage::noteOn(2, 80, 0.5f).withTimeStamp(3.0));
    s2.addEvent(MidiMessage::noteOff(2, 80, 0.5f).withTimeStamp(7.0));
    s2.addEvent(MidiMessage::noteOff(2, 25, 0.5f).withTimeStamp(9.0));

    s.addSequence(s2, 0.0, 0.0, 8.0); // Intentionally cut off the last note off
    s.updateMatchedPairs();

    EXPECT_EQ(s.getNumEvents(), 7);
    EXPECT_EQ(s.getIndexOfMatchingKeyUp(0), -1); // Truncated note, should be no note off
    EXPECT_EQ(s.getTimeOfMatchingKeyUp(1), 5.0);
}

TEST_F(MidiMessageSequenceTest, CreateControllerUpdatesForTimeEmitsNRPNComponentsInCorrectOrder)
{
    const auto channel = 1;
    const auto number = 200;
    const auto value = 300;

    MidiMessageSequence sequence;
    addNrpn(sequence, channel, number, value);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime(channel, 1.0, m);

    checkNrpn(m.begin(), m.end(), channel, number, value);
}

TEST_F(MidiMessageSequenceTest, CreateControllerUpdatesForTimeIgnoresNRPNsAfterFinalRequestedTime)
{
    const auto channel = 2;
    const auto number = 123;
    const auto value = 456;

    MidiMessageSequence sequence;
    addRpn(sequence, channel, number, value, 0.5);
    addRpn(sequence, channel, 111, 222, 1.5);
    addRpn(sequence, channel, 333, 444, 2.5);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime(channel, 1.0, m);

    checkRpn(m.begin(), std::next(m.begin(), 4), channel, number, value, 0.5);
}

TEST_F(MidiMessageSequenceTest, CreateControllerUpdatesForTimeEmitsSeparateNRPNMessagesWhenAppropriate)
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
    addRpn(sequence, channel, numberA, valueA, time);
    addRpn(sequence, channel, numberB, valueB, time);
    addNrpn(sequence, channel, numberC, valueC, time);
    addNrpn(sequence, channel, numberD, valueD, time);

    Array<MidiMessage> m;
    sequence.createControllerUpdatesForTime(channel, time * 2, m);

    checkRpn(std::next(m.begin(), 0), std::next(m.begin(), 4), channel, numberA, valueA, time);
    checkRpn(std::next(m.begin(), 4), std::next(m.begin(), 8), channel, numberB, valueB, time);
    checkNrpn(std::next(m.begin(), 8), std::next(m.begin(), 12), channel, numberC, valueC, time);
    checkNrpn(std::next(m.begin(), 12), std::next(m.begin(), 16), channel, numberD, valueD, time);
}

#endif
