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

#include <gtest/gtest.h>

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

TEST (MidiBufferTests, DefaultIsEmpty)
{
    MidiBuffer buffer;
    EXPECT_TRUE (buffer.isEmpty());
    EXPECT_EQ (0, buffer.getNumEvents());
}

TEST (MidiBufferTests, AddEventMakesNonEmpty)
{
    MidiBuffer buffer;
    buffer.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (1, buffer.getNumEvents());
}

TEST (MidiBufferTests, ClearAllRemovesAllEvents)
{
    MidiBuffer buffer;
    buffer.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    buffer.addEvent (MidiMessage::noteOff (1, 60), 10);
    buffer.clear();
    EXPECT_TRUE (buffer.isEmpty());
    EXPECT_EQ (0, buffer.getNumEvents());
}

TEST (MidiBufferTests, IterationYieldsEventsInOrder)
{
    MidiBuffer buffer;
    buffer.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    buffer.addEvent (MidiMessage::noteOn (1, 64, 0.5f), 10);
    buffer.addEvent (MidiMessage::noteOff (1, 60), 20);

    int prevPos = -1;
    int count = 0;
    for (const auto& meta : buffer)
    {
        EXPECT_GE (meta.samplePosition, prevPos);
        prevPos = meta.samplePosition;
        ++count;
    }
    EXPECT_EQ (3, count);
}

TEST (MidiBufferTests, Clear)
{
    const auto message = MidiMessage::noteOn (1, 64, 0.5f);

    const auto testBuffer = [&]
    {
        MidiBuffer buffer;
        buffer.addEvent (message, 0);
        buffer.addEvent (message, 10);
        buffer.addEvent (message, 20);
        buffer.addEvent (message, 30);
        return buffer;
    }();

    {
        auto buffer = testBuffer;
        buffer.clear (10, 0);
        EXPECT_EQ (buffer.getNumEvents(), 4);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 1);
        EXPECT_EQ (buffer.getNumEvents(), 3);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 10);
        EXPECT_EQ (buffer.getNumEvents(), 3);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 20);
        EXPECT_EQ (buffer.getNumEvents(), 2);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 30);
        EXPECT_EQ (buffer.getNumEvents(), 1);
    }

    {
        auto buffer = testBuffer;
        buffer.clear (10, 300);
        EXPECT_EQ (buffer.getNumEvents(), 1);
    }
}

TEST (MidiBufferTests, CopyConstructorPreservesEvents)
{
    MidiBuffer original;
    original.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    original.addEvent (MidiMessage::noteOff (1, 60), 10);

    MidiBuffer copy (original);
    EXPECT_EQ (copy.getNumEvents(), original.getNumEvents());
    EXPECT_FALSE (copy.isEmpty());
}

TEST (MidiBufferTests, AddEventsFromAnotherBuffer)
{
    MidiBuffer source;
    source.addEvent (MidiMessage::noteOn (1, 72, 0.8f), 5);
    source.addEvent (MidiMessage::noteOff (1, 72), 15);

    MidiBuffer destination;
    destination.addEvent (MidiMessage::noteOn (1, 60, 0.5f), 0);
    destination.addEvents (source, 0, -1, 20); // offset by 20 samples

    EXPECT_EQ (destination.getNumEvents(), 3);
}

TEST (MidiBufferTests, EventDataMatchesAddedMessage)
{
    MidiBuffer buffer;
    auto noteOn = MidiMessage::noteOn (2, 64, 0.75f);
    buffer.addEvent (noteOn, 100);

    for (const auto& meta : buffer)
    {
        EXPECT_EQ (meta.samplePosition, 100);
        EXPECT_EQ (meta.getMessage().getChannel(), 2);
        EXPECT_EQ (meta.getMessage().getNoteNumber(), 64);
    }
}
