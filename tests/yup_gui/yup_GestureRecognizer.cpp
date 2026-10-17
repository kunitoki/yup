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

class GestureRecognizerTests : public ::testing::Test
{
protected:
    std::vector<GestureEvent> events;
    GestureRecognizer recognizer { [this] (const GestureEvent& event)
    {
        events.push_back (event);
    } };

    std::vector<GestureEvent> eventsOfType (GestureType type) const
    {
        std::vector<GestureEvent> result;

        for (const auto& event : events)
        {
            if (event.getType() == type)
                result.push_back (event);
        }

        return result;
    }

    void tap (int64 id, Point<float> position, double downTime, double upTime)
    {
        recognizer.touchDown (id, position, downTime);
        recognizer.touchUp (id, position, upTime);
    }

    /** Moves every finger by the same step, one finger at a time like a touch screen reports them. */
    void moveTogether (std::vector<std::pair<int64, Point<float>>>& fingers, Point<float> step, int steps, double startTime, double stepTime)
    {
        for (int i = 1; i <= steps; ++i)
        {
            for (auto& [id, position] : fingers)
            {
                position += step;
                recognizer.touchMove (id, position, startTime + i * stepTime);
            }
        }
    }
};

//==============================================================================

TEST_F (GestureRecognizerTests, SingleTapReportsOneTap)
{
    tap (1, { 50.0f, 60.0f }, 0.0, 80.0);

    ASSERT_EQ (events.size(), 1u);
    EXPECT_EQ (events[0].getType(), GestureType::tap);
    EXPECT_EQ (events[0].getPhase(), GestureEvent::Phase::ended);
    EXPECT_EQ (events[0].getTapCount(), 1);
    EXPECT_EQ (events[0].getTouchCount(), 1);
    EXPECT_EQ (events[0].getPosition(), Point<float> (50.0f, 60.0f));
}

TEST_F (GestureRecognizerTests, TwoQuickTapsInPlaceReportADoubleTap)
{
    tap (1, { 50.0f, 50.0f }, 0.0, 60.0);
    tap (2, { 55.0f, 52.0f }, 200.0, 260.0);

    const auto taps = eventsOfType (GestureType::tap);
    ASSERT_EQ (taps.size(), 2u);
    EXPECT_EQ (taps[0].getTapCount(), 1);
    EXPECT_EQ (taps[1].getTapCount(), 2);
}

TEST_F (GestureRecognizerTests, SlowOrDistantSecondTapStartsAgain)
{
    tap (1, { 50.0f, 50.0f }, 0.0, 60.0);
    tap (2, { 50.0f, 50.0f }, 1000.0, 1060.0);
    tap (3, { 300.0f, 50.0f }, 1100.0, 1160.0);

    const auto taps = eventsOfType (GestureType::tap);
    ASSERT_EQ (taps.size(), 3u);
    EXPECT_EQ (taps[1].getTapCount(), 1);
    EXPECT_EQ (taps[2].getTapCount(), 1);
}

TEST_F (GestureRecognizerTests, MovingOrHoldingTooLongIsNotATap)
{
    recognizer.touchDown (1, { 50.0f, 50.0f }, 0.0);
    recognizer.touchMove (1, { 80.0f, 50.0f }, 50.0);
    recognizer.touchUp (1, { 80.0f, 50.0f }, 100.0);

    tap (2, { 50.0f, 50.0f }, 1000.0, 1400.0);

    EXPECT_TRUE (eventsOfType (GestureType::tap).empty());
}

TEST_F (GestureRecognizerTests, TwoFingerTapReportsTwoTouches)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 140.0f, 100.0f }, 20.0);
    recognizer.touchUp (1, { 100.0f, 100.0f }, 90.0);
    recognizer.touchUp (2, { 140.0f, 100.0f }, 100.0);

    const auto taps = eventsOfType (GestureType::tap);
    ASSERT_EQ (taps.size(), 1u);
    EXPECT_EQ (taps[0].getTouchCount(), 2);
    EXPECT_EQ (taps[0].getPosition(), Point<float> (120.0f, 100.0f));
}

//==============================================================================

TEST_F (GestureRecognizerTests, LongPressBeginsAfterTheDelayAndFollowsTheFinger)
{
    recognizer.touchDown (1, { 50.0f, 50.0f }, 0.0);

    recognizer.advanceTo (400.0);
    EXPECT_TRUE (events.empty());

    recognizer.advanceTo (500.0);
    recognizer.touchMove (1, { 120.0f, 50.0f }, 600.0);
    recognizer.touchUp (1, { 120.0f, 50.0f }, 700.0);

    const auto presses = eventsOfType (GestureType::longPress);
    ASSERT_EQ (presses.size(), 3u);
    EXPECT_EQ (presses[0].getPhase(), GestureEvent::Phase::began);
    EXPECT_EQ (presses[0].getPosition(), Point<float> (50.0f, 50.0f));
    EXPECT_EQ (presses[1].getPhase(), GestureEvent::Phase::changed);
    EXPECT_EQ (presses[1].getPosition(), Point<float> (120.0f, 50.0f));
    EXPECT_EQ (presses[2].getPhase(), GestureEvent::Phase::ended);
    EXPECT_TRUE (eventsOfType (GestureType::tap).empty());
    EXPECT_TRUE (eventsOfType (GestureType::swipe).empty());
}

TEST_F (GestureRecognizerTests, MovingBeforeTheDelayPreventsALongPress)
{
    recognizer.touchDown (1, { 50.0f, 50.0f }, 0.0);
    recognizer.touchMove (1, { 80.0f, 50.0f }, 100.0);
    recognizer.advanceTo (800.0);

    EXPECT_TRUE (eventsOfType (GestureType::longPress).empty());
}

TEST_F (GestureRecognizerTests, ASecondFingerCancelsALongPress)
{
    recognizer.touchDown (1, { 50.0f, 50.0f }, 0.0);
    recognizer.advanceTo (600.0);
    recognizer.touchDown (2, { 150.0f, 50.0f }, 700.0);

    const auto presses = eventsOfType (GestureType::longPress);
    ASSERT_EQ (presses.size(), 2u);
    EXPECT_EQ (presses[1].getPhase(), GestureEvent::Phase::canceled);
}

//==============================================================================

TEST_F (GestureRecognizerTests, SpreadingFingersPinchesOut)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);

    for (int i = 1; i <= 5; ++i)
    {
        recognizer.touchMove (1, { 100.0f - 10.0f * i, 100.0f }, 10.0 * i);
        recognizer.touchMove (2, { 200.0f + 10.0f * i, 100.0f }, 10.0 * i);
    }

    recognizer.touchUp (1, { 50.0f, 100.0f }, 60.0);
    recognizer.touchUp (2, { 250.0f, 100.0f }, 60.0);

    const auto pinches = eventsOfType (GestureType::pinch);
    ASSERT_GE (pinches.size(), 3u);
    EXPECT_EQ (pinches.front().getPhase(), GestureEvent::Phase::began);
    EXPECT_EQ (pinches.front().getTouchCount(), 2);
    EXPECT_EQ (pinches.back().getPhase(), GestureEvent::Phase::ended);
    EXPECT_NEAR (pinches.back().getScale(), 2.0f, 1.0e-4f);

    auto product = 1.0f;
    for (const auto& pinch : pinches)
        product *= pinch.getScaleDelta();

    EXPECT_NEAR (product, pinches.back().getScale(), 1.0e-4f);
    EXPECT_TRUE (eventsOfType (GestureType::pan).empty());
    EXPECT_TRUE (eventsOfType (GestureType::tap).empty());
}

TEST_F (GestureRecognizerTests, ClosingFingersPinchesIn)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);
    recognizer.touchMove (1, { 125.0f, 100.0f }, 10.0);
    recognizer.touchMove (2, { 175.0f, 100.0f }, 10.0);

    const auto pinches = eventsOfType (GestureType::pinch);
    ASSERT_FALSE (pinches.empty());
    EXPECT_NEAR (pinches.back().getScale(), 0.5f, 1.0e-4f);
}

TEST_F (GestureRecognizerTests, ASmallSpanChangeIsNotAPinch)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);
    recognizer.touchMove (1, { 97.0f, 100.0f }, 10.0);
    recognizer.touchMove (2, { 203.0f, 100.0f }, 10.0);

    EXPECT_TRUE (eventsOfType (GestureType::pinch).empty());
}

TEST_F (GestureRecognizerTests, TwistingFingersRotatesClockwise)
{
    const Point<float> center (150.0f, 100.0f);
    const auto angle = 0.4f;
    const Point<float> offset (50.0f * std::cos (angle), 50.0f * std::sin (angle));

    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);
    recognizer.touchMove (2, center + offset, 10.0);
    recognizer.touchMove (1, center - offset, 10.0);

    const auto rotations = eventsOfType (GestureType::rotate);
    ASSERT_FALSE (rotations.empty());
    EXPECT_EQ (rotations.front().getPhase(), GestureEvent::Phase::began);
    EXPECT_NEAR (rotations.back().getRotation(), angle, 1.0e-3f);
    EXPECT_TRUE (eventsOfType (GestureType::pinch).empty());
}

//==============================================================================

TEST_F (GestureRecognizerTests, TwoFingerPanReportsTranslationAndFlingVelocity)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 100.0f, 100.0f } }, { 2, { 160.0f, 100.0f } } };
    recognizer.touchDown (1, fingers[0].second, 0.0);
    recognizer.touchDown (2, fingers[1].second, 0.0);

    moveTogether (fingers, { 10.0f, 0.0f }, 10, 0.0, 10.0);

    recognizer.touchUp (1, fingers[0].second, 100.0);
    recognizer.touchUp (2, fingers[1].second, 100.0);

    const auto pans = eventsOfType (GestureType::pan);
    ASSERT_GE (pans.size(), 3u);
    EXPECT_EQ (pans.front().getPhase(), GestureEvent::Phase::began);
    EXPECT_EQ (pans.back().getPhase(), GestureEvent::Phase::ended);
    EXPECT_EQ (pans.back().getTouchCount(), 2);
    EXPECT_NEAR (pans.back().getTranslation().getX(), 100.0f, 1.0e-3f);
    EXPECT_NEAR (pans.back().getVelocity().getX(), 1000.0f, 1.0f);

    auto sum = Point<float>();
    for (const auto& pan : pans)
        sum += pan.getTranslationDelta();

    EXPECT_NEAR (sum.getX(), pans.back().getTranslation().getX(), 1.0e-3f);
}

TEST_F (GestureRecognizerTests, ThreeFingerSwipeReportsDirectionAndTouchCount)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 100.0f, 300.0f } }, { 2, { 150.0f, 300.0f } }, { 3, { 200.0f, 300.0f } } };
    for (const auto& [id, position] : fingers)
        recognizer.touchDown (id, position, 0.0);

    moveTogether (fingers, { 0.0f, -20.0f }, 5, 0.0, 10.0);

    for (const auto& [id, position] : fingers)
        recognizer.touchUp (id, position, 50.0);

    const auto swipes = eventsOfType (GestureType::swipe);
    ASSERT_EQ (swipes.size(), 1u);
    EXPECT_EQ (swipes[0].getSwipeDirection(), GestureEvent::Direction::up);
    EXPECT_EQ (swipes[0].getTouchCount(), 3);
    EXPECT_LT (swipes[0].getVelocity().getY(), -300.0f);
    EXPECT_NEAR (swipes[0].getTranslation().getY(), -100.0f, 1.0e-3f);
}

TEST_F (GestureRecognizerTests, OneFingerFlickIsASwipe)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 200.0f, 100.0f } } };
    recognizer.touchDown (1, fingers[0].second, 0.0);
    moveTogether (fingers, { -25.0f, 0.0f }, 4, 0.0, 10.0);
    recognizer.touchUp (1, fingers[0].second, 40.0);

    const auto swipes = eventsOfType (GestureType::swipe);
    ASSERT_EQ (swipes.size(), 1u);
    EXPECT_EQ (swipes[0].getSwipeDirection(), GestureEvent::Direction::left);
    EXPECT_EQ (swipes[0].getTouchCount(), 1);
}

TEST_F (GestureRecognizerTests, SlowOrStoppedMovementIsNotASwipe)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 100.0f, 100.0f } }, { 2, { 160.0f, 100.0f } } };
    recognizer.touchDown (1, fingers[0].second, 0.0);
    recognizer.touchDown (2, fingers[1].second, 0.0);

    // 100 pixels in a second is too slow
    moveTogether (fingers, { 10.0f, 0.0f }, 10, 0.0, 100.0);
    recognizer.touchUp (1, fingers[0].second, 1000.0);
    recognizer.touchUp (2, fingers[1].second, 1000.0);

    // A fast move followed by a rest has no release speed left
    fingers = { { 3, { 100.0f, 100.0f } } };
    recognizer.touchDown (3, fingers[0].second, 2000.0);
    moveTogether (fingers, { 50.0f, 0.0f }, 2, 2000.0, 10.0);
    recognizer.touchMove (3, fingers[0].second, 2300.0);
    recognizer.touchUp (3, fingers[0].second, 2400.0);

    EXPECT_TRUE (eventsOfType (GestureType::swipe).empty());
}

TEST_F (GestureRecognizerTests, APinchIsNeverASwipe)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);
    recognizer.touchMove (2, { 400.0f, 100.0f }, 20.0);
    recognizer.touchUp (2, { 400.0f, 100.0f }, 30.0);
    recognizer.touchUp (1, { 100.0f, 100.0f }, 30.0);

    EXPECT_FALSE (eventsOfType (GestureType::pinch).empty());
    EXPECT_TRUE (eventsOfType (GestureType::swipe).empty());
}

//==============================================================================

TEST_F (GestureRecognizerTests, AnotherFingerEndsTheRunningGestures)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 100.0f, 100.0f } }, { 2, { 160.0f, 100.0f } } };
    recognizer.touchDown (1, fingers[0].second, 0.0);
    recognizer.touchDown (2, fingers[1].second, 0.0);
    moveTogether (fingers, { 10.0f, 0.0f }, 3, 0.0, 10.0);

    ASSERT_FALSE (eventsOfType (GestureType::pan).empty());

    recognizer.touchDown (3, { 300.0f, 100.0f }, 40.0);

    EXPECT_EQ (eventsOfType (GestureType::pan).back().getPhase(), GestureEvent::Phase::ended);

    fingers.push_back ({ 3, { 300.0f, 100.0f } });
    moveTogether (fingers, { 0.0f, 10.0f }, 3, 40.0, 10.0);

    const auto pans = eventsOfType (GestureType::pan);
    EXPECT_EQ (pans.back().getPhase(), GestureEvent::Phase::changed);
    EXPECT_EQ (pans.back().getTouchCount(), 3);
    EXPECT_NEAR (pans.back().getTranslation().getY(), 30.0f, 1.0e-3f);
}

TEST_F (GestureRecognizerTests, CancelReportsCanceledAndSuppressesTapAndSwipe)
{
    std::vector<std::pair<int64, Point<float>>> fingers { { 1, { 100.0f, 100.0f } }, { 2, { 160.0f, 100.0f } } };
    recognizer.touchDown (1, fingers[0].second, 0.0);
    recognizer.touchDown (2, fingers[1].second, 0.0);
    moveTogether (fingers, { 20.0f, 0.0f }, 3, 0.0, 10.0);

    recognizer.touchCancel (1);
    recognizer.touchMove (2, fingers[1].second + Point<float> (50.0f, 0.0f), 40.0);
    recognizer.touchUp (2, fingers[1].second + Point<float> (50.0f, 0.0f), 50.0);

    EXPECT_EQ (eventsOfType (GestureType::pan).back().getPhase(), GestureEvent::Phase::canceled);
    EXPECT_TRUE (eventsOfType (GestureType::swipe).empty());
    EXPECT_TRUE (eventsOfType (GestureType::tap).empty());
    EXPECT_EQ (recognizer.getNumTouches(), 0);
}

TEST_F (GestureRecognizerTests, CancelAllCancelsEverythingAndForgetsTheFingers)
{
    recognizer.touchDown (1, { 100.0f, 100.0f }, 0.0);
    recognizer.touchDown (2, { 200.0f, 100.0f }, 0.0);
    recognizer.touchMove (2, { 300.0f, 100.0f }, 10.0);

    recognizer.cancelAll();

    EXPECT_EQ (eventsOfType (GestureType::pinch).back().getPhase(), GestureEvent::Phase::canceled);
    EXPECT_EQ (recognizer.getNumTouches(), 0);
}

TEST_F (GestureRecognizerTests, UnknownOrRepeatedTouchesAreIgnored)
{
    recognizer.touchMove (7, { 10.0f, 10.0f }, 0.0);
    recognizer.touchUp (7, { 10.0f, 10.0f }, 0.0);
    recognizer.touchCancel (7);

    recognizer.touchDown (1, { 10.0f, 10.0f }, 0.0);
    recognizer.touchDown (1, { 90.0f, 10.0f }, 10.0);

    EXPECT_EQ (recognizer.getNumTouches(), 1);
    EXPECT_TRUE (events.empty());
}

TEST_F (GestureRecognizerTests, OptionsChangeTheThresholds)
{
    GestureRecognizer::Options options;
    options.longPressDelay = 100.0;

    std::vector<GestureEvent> received;
    GestureRecognizer custom ([&] (const GestureEvent& event) { received.push_back (event); }, options);

    custom.touchDown (1, { 10.0f, 10.0f }, 0.0);
    custom.advanceTo (100.0);

    ASSERT_EQ (received.size(), 1u);
    EXPECT_EQ (received[0].getType(), GestureType::longPress);
    EXPECT_DOUBLE_EQ (custom.getOptions().longPressDelay, 100.0);
}
