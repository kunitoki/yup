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

//==============================================================================
class KineticScrollerTests : public ::testing::Test
{
protected:
    static constexpr float viewport = 100.0f;
    static constexpr double frame = 1.0 / 60.0;

    /** The distance a fling covers per unit of release velocity with the default deceleration. */
    static float flingDistancePerVelocity()
    {
        return 1.0f / (-1000.0f * std::log (KineticScroller::Options().deceleration));
    }

    /** Drags the pointer 100pt towards the start over 50ms and releases straight away: a release
        velocity of 2000pt/s, starting from offset 100. */
    static void flingForward (KineticScroller& scroller)
    {
        scroller.beginDrag (500.0f, 0.0);
        scroller.dragTo (475.0f, 0.0125);
        scroller.dragTo (450.0f, 0.025);
        scroller.dragTo (425.0f, 0.0375);
        scroller.dragTo (400.0f, 0.05);
        scroller.endDrag (0.05);
    }

    /** Runs frames until the motion ends, returning the largest offset seen on the way. */
    static float settle (KineticScroller& scroller)
    {
        auto largest = scroller.getOffset();

        for (int i = 0; i < 10000 && scroller.update (frame); ++i)
            largest = jmax (largest, scroller.getOffset());

        return largest;
    }
};

//==============================================================================
TEST_F (KineticScrollerTests, OptionsBuildersSetEveryField)
{
    const auto options = KineticScroller::Options()
                             .withOverscrollEnabled (false)
                             .withOverscrollResistance (0.3f)
                             .withMaxOverscrollFraction (0.25f)
                             .withBounceBackTime (0.5f)
                             .withDeceleration (0.99f)
                             .withMaxFlingVelocity (1000.0f)
                             .withScrollAnimationTime (0.2f);

    EXPECT_FALSE (options.overscrollEnabled);
    EXPECT_FLOAT_EQ (0.3f, options.overscrollResistance);
    EXPECT_FLOAT_EQ (0.25f, options.maxOverscrollFraction);
    EXPECT_FLOAT_EQ (0.5f, options.bounceBackTime);
    EXPECT_FLOAT_EQ (0.99f, options.deceleration);
    EXPECT_FLOAT_EQ (1000.0f, options.maxFlingVelocity);
    EXPECT_FLOAT_EQ (0.2f, options.scrollAnimationTime);
}

TEST_F (KineticScrollerTests, ConstructorsTakeDefaultOrGivenOptions)
{
    const KineticScroller defaulted;
    EXPECT_TRUE (defaulted.getOptions().overscrollEnabled);
    EXPECT_FLOAT_EQ (KineticScroller::Options().deceleration, defaulted.getOptions().deceleration);
    EXPECT_FLOAT_EQ (0.0f, defaulted.getOffset());

    const KineticScroller configured (KineticScroller::Options().withOverscrollEnabled (false).withDeceleration (0.99f));
    EXPECT_FALSE (configured.getOptions().overscrollEnabled);
    EXPECT_FLOAT_EQ (0.99f, configured.getOptions().deceleration);
    EXPECT_FLOAT_EQ (0.0f, configured.getOffset());
}

TEST_F (KineticScrollerTests, DragFollowsThePointerOneToOne)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (500.0f, 0.0);
    scroller.dragTo (450.0f, 0.01);
    EXPECT_FLOAT_EQ (50.0f, scroller.getOffset());

    scroller.dragTo (380.0f, 0.02);
    EXPECT_FLOAT_EQ (120.0f, scroller.getOffset());
    EXPECT_TRUE (scroller.isDragging());
    EXPECT_FALSE (scroller.isAnimating());
}

TEST_F (KineticScrollerTests, DraggingPastAnEdgeIsResistedAndCapped)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (0.0f, 0.0);
    scroller.dragTo (100.0f, 0.1);

    // 100pt past the edge maps to d * (1 - 1 / (e * c / d + 1)).
    const auto expected = viewport * (1.0f - 1.0f / (100.0f * 0.55f / viewport + 1.0f));
    EXPECT_NEAR (-expected, scroller.getOffset(), 0.001f);
    EXPECT_NEAR (-expected, scroller.getOverscroll(), 0.001f);

    scroller.dragTo (100000.0f, 0.2);
    EXPECT_FLOAT_EQ (-0.5f * viewport, scroller.getOverscroll());
}

TEST_F (KineticScrollerTests, DraggingPastAnEdgeClampsWhenOverscrollIsDisabled)
{
    KineticScroller scroller (KineticScroller::Options().withOverscrollEnabled (false));
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (0.0f, 0.0);
    scroller.dragTo (100.0f, 0.1);

    EXPECT_FLOAT_EQ (0.0f, scroller.getOffset());
    EXPECT_FLOAT_EQ (0.0f, scroller.getOverscroll());
}

TEST_F (KineticScrollerTests, AFlingDecaysToItsNaturalRestPoint)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    flingForward (scroller);
    EXPECT_FALSE (scroller.isDragging());
    EXPECT_TRUE (scroller.isAnimating());

    settle (scroller);

    EXPECT_FALSE (scroller.isAnimating());
    EXPECT_NEAR (100.0f + 2000.0f * flingDistancePerVelocity(), scroller.getOffset(), 0.01f);
}

TEST_F (KineticScrollerTests, AFlingIntoAnEdgeOverscrollsThenSettlesExactlyOnIt)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 500.0f, viewport);

    flingForward (scroller);
    const auto largest = settle (scroller);

    EXPECT_GT (largest, 500.0f);
    EXPECT_LE (largest, 500.0f + 0.5f * viewport);
    EXPECT_FLOAT_EQ (500.0f, scroller.getOffset());
    EXPECT_FALSE (scroller.isAnimating());
}

TEST_F (KineticScrollerTests, AFlingStopsDeadAtAnEdgeWhenOverscrollIsDisabled)
{
    KineticScroller scroller;
    scroller.setOptions (KineticScroller::Options().withOverscrollEnabled (false));
    scroller.setLimits (0.0f, 500.0f, viewport);

    flingForward (scroller);
    const auto largest = settle (scroller);

    EXPECT_FLOAT_EQ (500.0f, largest);
    EXPECT_FLOAT_EQ (500.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ReleasingAfterRestingDoesNotFling)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (500.0f, 0.0);
    scroller.dragTo (400.0f, 0.05);
    scroller.endDrag (0.2);

    EXPECT_FALSE (scroller.isAnimating());
    EXPECT_FALSE (scroller.update (frame));
    EXPECT_FLOAT_EQ (100.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ReleasingWhileOverscrolledSpringsBackToTheEdge)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (0.0f, 0.0);
    scroller.dragTo (100.0f, 0.1);
    scroller.endDrag (0.3);

    EXPECT_TRUE (scroller.isAnimating());
    settle (scroller);

    EXPECT_FLOAT_EQ (0.0f, scroller.getOffset());
    EXPECT_FLOAT_EQ (0.0f, scroller.getOverscroll());
}

TEST_F (KineticScrollerTests, TheSnapFunctionTargetIsReachedExactly)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    float seenRest = 0.0f;
    scroller.setSnapFunction ([&] (float rest, float)
    {
        seenRest = rest;
        return std::round (rest / 50.0f) * 50.0f;
    });

    flingForward (scroller);
    settle (scroller);

    EXPECT_NEAR (100.0f + 2000.0f * flingDistancePerVelocity(), seenRest, 0.01f);
    EXPECT_FLOAT_EQ (std::round (seenRest / 50.0f) * 50.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ASnapTargetShortOfTheFlingKeepsTheReleaseSpeedAndBrakesHarder)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    // The fling would rest near 1100, the snap stops it 50pt from the release point at 100.
    scroller.setSnapFunction ([] (float, float) { return 150.0f; });

    flingForward (scroller);

    // The first frame still moves at close to the 2000pt/s release speed, instead of crawling at
    // the 100pt/s the default deceleration would need to stop 50pt away.
    scroller.update (frame);
    EXPECT_GT (scroller.getOffset(), 120.0f);

    // And it lands well within a quarter of a second, exactly on target.
    for (int i = 0; i < 15; ++i)
        scroller.update (frame);

    EXPECT_FALSE (scroller.isAnimating());
    EXPECT_FLOAT_EQ (150.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ASlowReleaseStillReachesAFarSnapTarget)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);
    scroller.setSnapFunction ([] (float, float) { return 400.0f; });

    // Resting before release: no velocity at all, yet the snap still carries it the whole way.
    scroller.beginDrag (500.0f, 0.0);
    scroller.dragTo (400.0f, 0.05);
    scroller.endDrag (0.2);

    settle (scroller);

    EXPECT_FLOAT_EQ (400.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ASnapTargetOutsideTheRangeIsClamped)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);
    scroller.setSnapFunction ([] (float, float) { return 99999.0f; });

    flingForward (scroller);
    settle (scroller);

    EXPECT_FLOAT_EQ (1000.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, AnimateToReachesItsTargetInTheAnimationTime)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.animateTo (300.0f);
    EXPECT_TRUE (scroller.isAnimating());

    // Half the default 0.3s at 60fps: an ease-out curve is already past the halfway point.
    for (int i = 0; i < 9; ++i)
        EXPECT_TRUE (scroller.update (frame));

    EXPECT_GT (scroller.getOffset(), 150.0f);
    EXPECT_LT (scroller.getOffset(), 300.0f);

    int remainingFrames = 0;

    while (scroller.update (frame))
        ++remainingFrames;

    EXPECT_NEAR (9, remainingFrames, 1);
    EXPECT_FLOAT_EQ (300.0f, scroller.getOffset());
    EXPECT_FALSE (scroller.isAnimating());
}

TEST_F (KineticScrollerTests, AnimateToClampsItsTarget)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.animateTo (5000.0f);
    settle (scroller);

    EXPECT_FLOAT_EQ (1000.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, ANegativeMinimumOffsetHolds)
{
    KineticScroller scroller;
    scroller.setLimits (-56.0f, 1000.0f, viewport);

    scroller.setOffset (-56.0f);

    EXPECT_FLOAT_EQ (-56.0f, scroller.getOffset());
    EXPECT_FLOAT_EQ (0.0f, scroller.getOverscroll());
    EXPECT_FALSE (scroller.update (frame));
    EXPECT_FLOAT_EQ (-56.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, SetOffsetClampsAndHaltsMotion)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    flingForward (scroller);
    scroller.update (frame);

    scroller.setOffset (-20.0f);

    EXPECT_FALSE (scroller.isAnimating());
    EXPECT_FALSE (scroller.update (frame));
    EXPECT_FLOAT_EQ (0.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, StopLeavesTheOffsetWhereItIs)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    flingForward (scroller);
    scroller.update (frame);

    const auto stoppedAt = scroller.getOffset();
    scroller.stop();

    EXPECT_FALSE (scroller.update (frame));
    EXPECT_FLOAT_EQ (stoppedAt, scroller.getOffset());
}

TEST_F (KineticScrollerTests, BeginningADragCatchesAFling)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 10000.0f, viewport);

    flingForward (scroller);
    scroller.update (0.1);

    const auto caughtAt = scroller.getOffset();
    scroller.beginDrag (300.0f, 1.0);

    EXPECT_TRUE (scroller.isDragging());
    EXPECT_FALSE (scroller.isAnimating());
    EXPECT_FLOAT_EQ (caughtAt, scroller.getOffset());

    scroller.dragTo (290.0f, 1.01);
    EXPECT_FLOAT_EQ (caughtAt + 10.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, TranslateShiftsADragWithoutInterruptingIt)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);

    scroller.beginDrag (500.0f, 0.0);
    scroller.dragTo (450.0f, 0.01);
    scroller.translate (30.0f);

    EXPECT_TRUE (scroller.isDragging());
    EXPECT_FLOAT_EQ (80.0f, scroller.getOffset());

    scroller.dragTo (440.0f, 0.02);
    EXPECT_FLOAT_EQ (90.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, TranslateShiftsAFlingAndItsRestPoint)
{
    KineticScroller shifted;
    KineticScroller regular;

    for (auto* scroller : { &shifted, &regular })
    {
        scroller->setLimits (0.0f, 10000.0f, viewport);
        flingForward (*scroller);
    }

    shifted.translate (40.0f);
    EXPECT_TRUE (shifted.isAnimating());

    settle (shifted);
    settle (regular);

    EXPECT_NEAR (regular.getOffset() + 40.0f, shifted.getOffset(), 0.01f);
}

TEST_F (KineticScrollerTests, TranslateClampsAnIdleOffset)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 100.0f, viewport);
    scroller.setOffset (90.0f);

    scroller.translate (50.0f);

    EXPECT_FLOAT_EQ (100.0f, scroller.getOffset());
}

TEST_F (KineticScrollerTests, LongFrameGapsAreClamped)
{
    KineticScroller stalled;
    KineticScroller regular;

    for (auto* scroller : { &stalled, &regular })
    {
        scroller->setLimits (0.0f, 10000.0f, viewport);
        flingForward (*scroller);
    }

    stalled.update (10.0);
    regular.update (1.0 / 15.0);

    EXPECT_FLOAT_EQ (regular.getOffset(), stalled.getOffset());
}

TEST_F (KineticScrollerTests, ShrinkingTheLimitsClampsAnIdleOffset)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, 1000.0f, viewport);
    scroller.setOffset (900.0f);

    scroller.setLimits (0.0f, 500.0f, viewport);

    EXPECT_FLOAT_EQ (500.0f, scroller.getOffset());
    EXPECT_FLOAT_EQ (0.0f, scroller.getMinOffset());
    EXPECT_FLOAT_EQ (500.0f, scroller.getMaxOffset());
}

TEST_F (KineticScrollerTests, AnInvertedRangeCollapsesToItsMinimum)
{
    KineticScroller scroller;
    scroller.setLimits (0.0f, -50.0f, viewport);

    EXPECT_FLOAT_EQ (0.0f, scroller.getMaxOffset());

    scroller.setOffset (40.0f);
    EXPECT_FLOAT_EQ (0.0f, scroller.getOffset());
}
