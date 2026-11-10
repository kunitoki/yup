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

class NoteExpressionBufferTests : public ::testing::Test
{
protected:
    static NoteExpression makeNoteExpression (int sampleOffset, float value)
    {
        NoteExpression expression;
        expression.sampleOffset = sampleOffset;
        expression.noteId = 7;
        expression.channel = 1;
        expression.key = 60;
        expression.type = NoteExpression::Type::tuning;
        expression.value = value;
        return expression;
    }
};

//==============================================================================

TEST_F (NoteExpressionBufferTests, StartsEmpty)
{
    NoteExpressionBuffer buffer;

    EXPECT_TRUE (buffer.isEmpty());
    EXPECT_EQ (0, buffer.getNumEvents());
    EXPECT_EQ (buffer.begin(), buffer.end());
}

TEST_F (NoteExpressionBufferTests, AddedEventsAreIterableInOrder)
{
    NoteExpressionBuffer buffer;
    buffer.reserve (4);

    EXPECT_TRUE (buffer.add (makeNoteExpression (0, 1.0f)));
    EXPECT_TRUE (buffer.add (makeNoteExpression (8, 2.0f)));

    EXPECT_FALSE (buffer.isEmpty());
    EXPECT_EQ (2, buffer.getNumEvents());
    ASSERT_EQ (2, static_cast<int> (buffer.end() - buffer.begin()));

    EXPECT_EQ (0, buffer.begin()[0].sampleOffset);
    EXPECT_FLOAT_EQ (1.0f, buffer.begin()[0].value);
    EXPECT_EQ (8, buffer.begin()[1].sampleOffset);
    EXPECT_EQ (NoteExpression::Type::tuning, buffer.begin()[1].type);
    EXPECT_EQ (7, buffer.begin()[1].noteId);
}

TEST_F (NoteExpressionBufferTests, ClearKeepsReservedCapacity)
{
    NoteExpressionBuffer buffer;
    buffer.reserve (2);

    EXPECT_TRUE (buffer.add (makeNoteExpression (0, 0.5f)));
    EXPECT_TRUE (buffer.add (makeNoteExpression (1, 0.5f)));

    buffer.clear();
    EXPECT_TRUE (buffer.isEmpty());

    EXPECT_TRUE (buffer.add (makeNoteExpression (2, 0.5f)));
    EXPECT_TRUE (buffer.add (makeNoteExpression (3, 0.5f)));
    EXPECT_EQ (2, buffer.getNumEvents());
}

TEST_F (NoteExpressionBufferTests, NegativeReserveIsClampedToZero)
{
    NoteExpressionBuffer buffer;
    buffer.reserve (-5);

    EXPECT_TRUE (buffer.isEmpty());
}

TEST_F (NoteExpressionBufferTests, SortOrdersBySampleOffsetAndKeepsSimultaneousOrder)
{
    NoteExpressionBuffer buffer;
    buffer.reserve (3);

    EXPECT_TRUE (buffer.add (makeNoteExpression (10, 1.0f)));
    EXPECT_TRUE (buffer.add (makeNoteExpression (2, 2.0f)));
    EXPECT_TRUE (buffer.add (makeNoteExpression (10, 3.0f)));

    buffer.sort();

    const auto* events = buffer.begin();
    EXPECT_EQ (2, events[0].sampleOffset);
    EXPECT_FLOAT_EQ (1.0f, events[1].value);
    EXPECT_FLOAT_EQ (3.0f, events[2].value);
}
