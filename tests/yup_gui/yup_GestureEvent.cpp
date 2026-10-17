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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

TEST (GestureEventTests, DefaultEventIsNeutral)
{
    const GestureEvent event;

    EXPECT_EQ (event.getType(), GestureType::none);
    EXPECT_EQ (event.getPhase(), GestureEvent::Phase::ended);
    EXPECT_FLOAT_EQ (event.getScale(), 1.0f);
    EXPECT_FLOAT_EQ (event.getScaleDelta(), 1.0f);
    EXPECT_FLOAT_EQ (event.getRotation(), 0.0f);
    EXPECT_EQ (event.getTranslation(), Point<float>());
    EXPECT_EQ (event.getVelocity(), Point<float>());
    EXPECT_EQ (event.getSwipeDirection(), GestureEvent::Direction::none);
    EXPECT_EQ (event.getTouchCount(), 0);
    EXPECT_EQ (event.getTapCount(), 0);
    EXPECT_FALSE (event.isTrackpad());
    EXPECT_EQ (event.getSourceComponent(), nullptr);
}

TEST (GestureEventTests, BuildersReturnModifiedCopies)
{
    const GestureEvent original (GestureType::pinch, GestureEvent::Phase::began);

    const auto event = original.withPhase (GestureEvent::Phase::changed)
                           .withPosition ({ 10.0f, 20.0f })
                           .withTouchCount (2)
                           .withTrackpad (true)
                           .withScale (1.5f, 1.1f)
                           .withRotation (0.5f, 0.1f)
                           .withTranslation ({ 3.0f, 4.0f }, { 1.0f, 2.0f })
                           .withVelocity ({ 100.0f, -50.0f })
                           .withSwipeDirection (GestureEvent::Direction::left)
                           .withTapCount (2)
                           .withModifiers (KeyModifiers (KeyModifiers::shiftMask));

    EXPECT_EQ (original.getPhase(), GestureEvent::Phase::began);
    EXPECT_FLOAT_EQ (original.getScale(), 1.0f);

    EXPECT_EQ (event.getType(), GestureType::pinch);
    EXPECT_EQ (event.getPhase(), GestureEvent::Phase::changed);
    EXPECT_EQ (event.getPosition(), Point<float> (10.0f, 20.0f));
    EXPECT_EQ (event.getTouchCount(), 2);
    EXPECT_TRUE (event.isTrackpad());
    EXPECT_FLOAT_EQ (event.getScale(), 1.5f);
    EXPECT_FLOAT_EQ (event.getScaleDelta(), 1.1f);
    EXPECT_FLOAT_EQ (event.getRotation(), 0.5f);
    EXPECT_FLOAT_EQ (event.getRotationDelta(), 0.1f);
    EXPECT_EQ (event.getTranslation(), Point<float> (3.0f, 4.0f));
    EXPECT_EQ (event.getTranslationDelta(), Point<float> (1.0f, 2.0f));
    EXPECT_EQ (event.getVelocity(), Point<float> (100.0f, -50.0f));
    EXPECT_EQ (event.getSwipeDirection(), GestureEvent::Direction::left);
    EXPECT_EQ (event.getTapCount(), 2);
    EXPECT_TRUE (event.getModifiers().isShiftDown());
}

TEST (GestureEventTests, RelativePositionToNullKeepsTheEvent)
{
    const auto event = GestureEvent (GestureType::tap, GestureEvent::Phase::ended).withPosition ({ 5.0f, 6.0f });

    EXPECT_EQ (event.withRelativePositionTo (nullptr).getPosition(), Point<float> (5.0f, 6.0f));
    EXPECT_EQ (event.withRelativePositionTo (nullptr).getSourceComponent(), nullptr);
}

TEST (GestureEventTests, GestureTypesCombineAsFlags)
{
    const auto types = GestureType::pinch | GestureType::rotate;

    EXPECT_TRUE (hasBitValueSet (types, GestureType::pinch));
    EXPECT_TRUE (hasBitValueSet (types, GestureType::rotate));
    EXPECT_FALSE (hasBitValueSet (types, GestureType::pan));
    EXPECT_TRUE (hasBitValueSet (GestureType::all, GestureType::swipe));
    EXPECT_FALSE (hasBitValueSet (GestureType::none, GestureType::tap));
}
