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

TEST (ParameterChangeBufferTests, DefaultIsEmpty)
{
    ParameterChangeBuffer changes;
    EXPECT_TRUE (changes.isEmpty());
    EXPECT_EQ (0, changes.getNumChanges());
}

TEST (ParameterChangeBufferTests, ClearOnDefaultDoesNotCrash)
{
    ParameterChangeBuffer changes;
    EXPECT_NO_THROW ({ changes.clear(); });
    EXPECT_TRUE (changes.isEmpty());
}

TEST (ParameterChangeBufferTests, FindNextSamplePositionOnEmptyReturnsEnd)
{
    ParameterChangeBuffer changes;
    EXPECT_EQ (changes.end(), changes.findNextSamplePosition (0));
}

TEST (ParameterChangeBufferTests, AddsAndClearsReservedChanges)
{
    ParameterChangeBuffer changes;

    EXPECT_TRUE (changes.isEmpty());
    EXPECT_EQ (0, changes.getNumChanges());

    changes.reserve (2);

    EXPECT_TRUE (changes.addChange (3, 0.25f, 12));
    EXPECT_TRUE (changes.addChange (4, 0.5f, 24));

    EXPECT_FALSE (changes.isEmpty());
    EXPECT_EQ (2, changes.getNumChanges());

    ASSERT_NE (changes.begin(), changes.end());
    EXPECT_EQ (3, changes.begin()->parameterIndex);
    EXPECT_FLOAT_EQ (0.25f, changes.begin()->normalizedValue);
    EXPECT_EQ (12, changes.begin()->sampleOffset);

    changes.clear();

    EXPECT_TRUE (changes.isEmpty());
    EXPECT_EQ (0, changes.getNumChanges());
}

TEST (ParameterChangeBufferTests, SortOrdersChangesBySampleOffset)
{
    ParameterChangeBuffer changes;
    changes.reserve (4);

    EXPECT_TRUE (changes.addChange (1, 0.3f, 24));
    EXPECT_TRUE (changes.addChange (2, 0.4f, 4));
    EXPECT_TRUE (changes.addChange (1, 0.2f, 12));

    changes.sort();

    ASSERT_EQ (3, changes.getNumChanges());

    const auto* change = changes.begin();
    EXPECT_EQ (4, change[0].sampleOffset);
    EXPECT_EQ (12, change[1].sampleOffset);
    EXPECT_EQ (24, change[2].sampleOffset);
}

TEST (ParameterChangeBufferTests, FindsNextChangeAtOrAfterSamplePosition)
{
    ParameterChangeBuffer changes;
    changes.reserve (3);

    EXPECT_TRUE (changes.addChange (1, 0.1f, 3));
    EXPECT_TRUE (changes.addChange (1, 0.2f, 9));
    EXPECT_TRUE (changes.addChange (1, 0.3f, 14));
    changes.sort();

    EXPECT_EQ (3, changes.findNextSamplePosition (0)->sampleOffset);
    EXPECT_EQ (3, changes.findNextSamplePosition (3)->sampleOffset);
    EXPECT_EQ (9, changes.findNextSamplePosition (4)->sampleOffset);
    EXPECT_EQ (14, changes.findNextSamplePosition (14)->sampleOffset);
    EXPECT_EQ (changes.end(), changes.findNextSamplePosition (15));
}

TEST (AudioParameterHandleTests, AdvanceToSampleAppliesOnlyMatchingParameterChanges)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("gain")
                         .withName ("Gain")
                         .withRange (0.0f, 100.0f)
                         .withDefault (10.0f)
                         .build();

    parameter->setIndexInContainer (2);

    AudioParameterHandle handle (*parameter, 48000.0);

    ParameterChangeBuffer changes;
    changes.reserve (4);
    EXPECT_TRUE (changes.addChange (1, 0.5f, 0));
    EXPECT_TRUE (changes.addChange (2, 0.25f, 4));
    EXPECT_TRUE (changes.addChange (2, 0.75f, 8));
    changes.sort();

    handle.prepareBlock (changes, parameter->getIndexInContainer());

    EXPECT_FALSE (handle.advanceToSample (3));
    EXPECT_FLOAT_EQ (10.0f, parameter->getValue());

    EXPECT_TRUE (handle.advanceToSample (4));
    EXPECT_FLOAT_EQ (25.0f, parameter->getValue());
    EXPECT_FLOAT_EQ (25.0f, handle.getCurrentValue());

    EXPECT_FALSE (handle.advanceToSample (7));
    EXPECT_FLOAT_EQ (25.0f, parameter->getValue());

    EXPECT_TRUE (handle.advanceToSample (8));
    EXPECT_FLOAT_EQ (75.0f, parameter->getValue());
    EXPECT_FLOAT_EQ (75.0f, handle.getCurrentValue());

    EXPECT_FALSE (handle.advanceToSample (8));
}

TEST (AudioParameterHandleTests, PrepareBlockRestartsAutomationIteration)
{
    auto parameter = AudioParameterBuilder()
                         .withID ("mix")
                         .withName ("Mix")
                         .withRange (0.0f, 1.0f)
                         .withDefault (0.0f)
                         .build();

    parameter->setIndexInContainer (0);

    AudioParameterHandle handle (*parameter, 48000.0);

    ParameterChangeBuffer changes;
    changes.reserve (1);
    EXPECT_TRUE (changes.addChange (0, 1.0f, 2));

    handle.prepareBlock (changes, parameter->getIndexInContainer());
    EXPECT_TRUE (handle.advanceToSample (2));
    EXPECT_FLOAT_EQ (1.0f, parameter->getValue());

    parameter->setValue (0.0f);

    handle.prepareBlock (changes, parameter->getIndexInContainer());
    EXPECT_TRUE (handle.advanceToSample (2));
    EXPECT_FLOAT_EQ (1.0f, parameter->getValue());
}
