/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

//==============================================================================

class NoteExpressionMidiConverterTests : public ::testing::Test
{
protected:
    static NoteExpression makeExpression (int sampleOffset, int channel, int key, NoteExpression::Type type, float value)
    {
        NoteExpression expression;
        expression.sampleOffset = sampleOffset;
        expression.channel = static_cast<int8> (channel);
        expression.key = static_cast<int8> (key);
        expression.type = type;
        expression.value = value;
        return expression;
    }

    static std::vector<std::pair<int, MidiMessage>> collect (const MidiBuffer& midi)
    {
        std::vector<std::pair<int, MidiMessage>> result;

        for (const auto metadata : midi)
            result.emplace_back (metadata.samplePosition, metadata.getMessage());

        return result;
    }

    void SetUp() override
    {
        expressions.reserve (16);
    }

    NoteExpressionBuffer expressions;
    MidiBuffer midi;
};

//==============================================================================

TEST_F (NoteExpressionMidiConverterTests, BufferDropsEventsBeyondReservedCapacity)
{
    NoteExpressionBuffer buffer;
    buffer.reserve (1);

    EXPECT_TRUE (buffer.add (makeExpression (0, 1, 60, NoteExpression::Type::pressure, 0.5f)));

    bool added = true;
    {
        const ScopedJassertDisabler disabler;
        added = buffer.add (makeExpression (1, 1, 60, NoteExpression::Type::pressure, 0.5f));
    }

    EXPECT_FALSE (added);
    EXPECT_EQ (1, buffer.getNumEvents());
}

TEST_F (NoteExpressionMidiConverterTests, BufferSortIsStable)
{
    expressions.add (makeExpression (10, 1, 60, NoteExpression::Type::pressure, 0.1f));
    expressions.add (makeExpression (5, 1, 60, NoteExpression::Type::pressure, 0.2f));
    expressions.add (makeExpression (10, 1, 60, NoteExpression::Type::pressure, 0.3f));
    expressions.sort();

    std::vector<float> values;
    for (const auto& expression : expressions)
        values.push_back (expression.value);

    EXPECT_EQ ((std::vector<float> { 0.2f, 0.1f, 0.3f }), values);
}

TEST_F (NoteExpressionMidiConverterTests, ChannelWideModeLeavesMidiUntouchedWithoutExpressions)
{
    NoteExpressionMidiConverter converter;
    midi.addEvent (MidiMessage::noteOn (3, 60, 0.8f), 4);

    converter.process (midi, expressions);

    const auto events = collect (midi);
    ASSERT_EQ (1u, events.size());
    EXPECT_EQ (3, events[0].second.getChannel());
}

TEST_F (NoteExpressionMidiConverterTests, ChannelWideModeConvertsExpressionsInSampleOrder)
{
    NoteExpressionMidiConverter converter;
    converter.prepare (1024);

    midi.addEvent (MidiMessage::noteOn (2, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOff (2, 60), 20);
    expressions.add (makeExpression (5, 2, 60, NoteExpression::Type::tuning, 1.0f));
    expressions.add (makeExpression (6, 2, 60, NoteExpression::Type::pressure, 1.0f));
    expressions.add (makeExpression (7, 2, 60, NoteExpression::Type::brightness, 0.0f));
    expressions.add (makeExpression (8, 2, 60, NoteExpression::Type::volume, 2.0f));

    converter.process (midi, expressions);

    const auto events = collect (midi);
    ASSERT_EQ (5u, events.size());

    EXPECT_TRUE (events[0].second.isNoteOn());
    EXPECT_EQ (5, events[1].first);
    EXPECT_TRUE (events[1].second.isPitchWheel());
    EXPECT_EQ (2, events[1].second.getChannel());
    EXPECT_EQ (8192 + 4096, events[1].second.getPitchWheelValue());
    EXPECT_TRUE (events[2].second.isChannelPressure());
    EXPECT_EQ (127, events[2].second.getChannelPressureValue());
    EXPECT_TRUE (events[3].second.isControllerOfType (74));
    EXPECT_EQ (0, events[3].second.getControllerValue());
    EXPECT_TRUE (events[4].second.isNoteOff());
}

TEST_F (NoteExpressionMidiConverterTests, MpeModeSendsZoneConfigurationOnce)
{
    NoteExpressionMidiConverter converter (NoteExpressionMidiConverter::Mode::mpe);

    converter.process (midi, expressions);
    const auto first = collect (midi);
    ASSERT_EQ (3u, first.size());
    EXPECT_TRUE (first[2].second.isControllerOfType (6));
    EXPECT_EQ (15, first[2].second.getControllerValue());

    midi.clear();
    converter.process (midi, expressions);
    EXPECT_TRUE (midi.isEmpty());
}

TEST_F (NoteExpressionMidiConverterTests, MpeModeGivesEachNoteItsOwnChannel)
{
    NoteExpressionMidiConverter converter (NoteExpressionMidiConverter::Mode::mpe);
    converter.process (midi, expressions);
    midi.clear();

    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOn (1, 64, 0.8f), 0);
    midi.addEvent (MidiMessage::controllerEvent (1, 64, 127), 1);
    expressions.add (makeExpression (2, 1, 64, NoteExpression::Type::tuning, -48.0f));

    converter.process (midi, expressions);

    // Each note-on is preceded by a pitch bend, pressure and CC74 reset on its member channel
    const auto events = collect (midi);
    ASSERT_EQ (10u, events.size());

    ASSERT_TRUE (events[3].second.isNoteOn());
    ASSERT_TRUE (events[7].second.isNoteOn());
    const auto firstChannel = events[3].second.getChannel();
    const auto secondChannel = events[7].second.getChannel();
    EXPECT_GE (firstChannel, 2);
    EXPECT_GE (secondChannel, 2);
    EXPECT_NE (firstChannel, secondChannel);

    EXPECT_EQ (1, events[8].second.getChannel());

    EXPECT_TRUE (events[9].second.isPitchWheel());
    EXPECT_EQ (secondChannel, events[9].second.getChannel());
    EXPECT_EQ (0, events[9].second.getPitchWheelValue());
}

TEST_F (NoteExpressionMidiConverterTests, MpeModeRoutesNoteOffToNoteChannelAndReusesIt)
{
    NoteExpressionMidiConverter converter (NoteExpressionMidiConverter::Mode::mpe);
    converter.process (midi, expressions);
    midi.clear();

    midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midi.addEvent (MidiMessage::noteOff (1, 60), 10);
    converter.process (midi, expressions);

    const auto events = collect (midi);
    ASSERT_EQ (5u, events.size());
    ASSERT_TRUE (events[3].second.isNoteOn());
    ASSERT_TRUE (events[4].second.isNoteOff());
    EXPECT_EQ (events[3].second.getChannel(), events[4].second.getChannel());

    midi.clear();
    midi.addEvent (MidiMessage::noteOn (1, 62, 0.8f), 0);
    converter.process (midi, expressions);

    const auto next = collect (midi);
    ASSERT_EQ (4u, next.size());
    ASSERT_TRUE (next[3].second.isNoteOn());
    EXPECT_GE (next[3].second.getChannel(), 2);
}

TEST_F (NoteExpressionMidiConverterTests, MpeModeNewNoteDoesNotInheritPreviousExpression)
{
    NoteExpressionMidiConverter converter (NoteExpressionMidiConverter::Mode::mpe);
    converter.process (midi, expressions);

    // Cycle through every member channel with a bent note, so the next note reuses a bent channel
    for (int note = 0; note < 15; ++note)
    {
        midi.clear();
        expressions.clear();
        midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
        midi.addEvent (MidiMessage::noteOff (1, 60), 2);
        expressions.add (makeExpression (1, 1, 60, NoteExpression::Type::tuning, 12.0f));
        converter.process (midi, expressions);
    }

    midi.clear();
    expressions.clear();
    midi.addEvent (MidiMessage::noteOn (1, 62, 0.8f), 0);
    expressions.add (makeExpression (0, 1, 62, NoteExpression::Type::tuning, -48.0f));
    converter.process (midi, expressions);

    const auto events = collect (midi);
    const auto noteOn = std::find_if (events.begin(), events.end(), [] (const auto& e) { return e.second.isNoteOn(); });
    ASSERT_NE (events.end(), noteOn);

    // The last bend before the note-on is the note's own tuning, never the previous note's
    const auto lastBend = std::find_if (std::make_reverse_iterator (noteOn), events.rend(), [] (const auto& e) { return e.second.isPitchWheel(); });
    ASSERT_NE (events.rend(), lastBend);
    EXPECT_EQ (noteOn->second.getChannel(), lastBend->second.getChannel());
    EXPECT_EQ (0, lastBend->second.getPitchWheelValue());
}
