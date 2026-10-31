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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class BreakpointCurveTests : public ::testing::Test
{
protected:
    using Curve = BreakpointCurve<double>;

    static Curve makeRamp (double curve)
    {
        Curve result;
        result.addPoint (0.0, 0.0);
        result.addPoint (1.0, 1.0, curve);
        return result;
    }
};

//==============================================================================
TEST_F (BreakpointCurveTests, ShapeMatchesThePythonOracle)
{
    // y (0.5) for k = 6, from the Phase 0 prototype.
    EXPECT_NEAR (0.0474, Curve::shape (1.0, 0.5), 1e-4);
    EXPECT_NEAR (0.1824, Curve::shape (0.5, 0.5), 1e-4);
    EXPECT_NEAR (0.3208, Curve::shape (0.25, 0.5), 1e-4);
    EXPECT_NEAR (1.0 - 0.0474, Curve::shape (-1.0, 0.5), 1e-4);
}

TEST_F (BreakpointCurveTests, ShapeIsMonotoneWithExactEndpoints)
{
    for (int c = -10; c <= 10; ++c)
    {
        const auto curve = c / 10.0;

        EXPECT_EQ (0.0, Curve::shape (curve, 0.0)) << curve;
        EXPECT_NEAR (1.0, Curve::shape (curve, 1.0), 1e-12) << curve;

        auto previous = 0.0;

        for (int i = 1; i <= 1000; ++i)
        {
            const auto value = Curve::shape (curve, i / 1000.0);
            EXPECT_GE (value, previous) << curve << " " << i;
            previous = value;
        }
    }
}

TEST_F (BreakpointCurveTests, ShapeIsContinuousThroughTheLinearFallback)
{
    EXPECT_EQ (0.3, Curve::shape (0.0, 0.3));
    EXPECT_NEAR (0.3, Curve::shape (1e-8, 0.3), 1e-8);
    EXPECT_NEAR (0.3, Curve::shape (-1e-8, 0.3), 1e-8);
    EXPECT_NEAR (0.3, Curve::shape (1e-6, 0.3), 1e-5);
}

TEST_F (BreakpointCurveTests, InverseShapeUndoesShape)
{
    for (const auto curve : { -1.0, -0.4, 0.0, 0.3, 1.0 })
        for (const auto t : { 0.0, 0.1, 0.5, 0.9, 1.0 })
            EXPECT_NEAR (t, Curve::inverseShape (curve, Curve::shape (curve, t)), 1e-9) << curve << " " << t;
}

TEST_F (BreakpointCurveTests, CurveForMidpointRecoversTheCurve)
{
    for (const auto curve : { -0.9, -0.3, 0.0, 0.5, 0.9 })
        EXPECT_NEAR (curve, Curve::curveForMidpoint (Curve::shape (curve, 0.5)), 1e-9) << curve;

    EXPECT_EQ (1.0, Curve::curveForMidpoint (0.0));
    EXPECT_EQ (-1.0, Curve::curveForMidpoint (1.0));
}

TEST_F (BreakpointCurveTests, AddPointKeepsTimesSorted)
{
    Curve curve;
    EXPECT_EQ (0, curve.addPoint (1.0, 0.5));
    EXPECT_EQ (0, curve.addPoint (0.0, 0.0));
    EXPECT_EQ (1, curve.addPoint (0.5, 1.0));

    ASSERT_EQ (3, curve.getNumPoints());
    EXPECT_EQ (0.0, curve.getPoint (0).time);
    EXPECT_EQ (0.5, curve.getPoint (1).time);
    EXPECT_EQ (1.0, curve.getPoint (2).time);
    EXPECT_EQ (1.0, curve.getLength());
}

TEST_F (BreakpointCurveTests, AddPointFailsWhenFull)
{
    Curve curve;

    for (int i = 0; i < Curve::maxPoints; ++i)
        EXPECT_EQ (i, curve.addPoint (static_cast<double> (i), 0.0));

    EXPECT_EQ (-1, curve.addPoint (100.0, 0.0));
    EXPECT_EQ (Curve::maxPoints, curve.getNumPoints());
}

TEST_F (BreakpointCurveTests, SetPointClampsBetweenNeighbors)
{
    Curve curve;
    curve.addPoint (0.0, 0.0);
    curve.addPoint (1.0, 1.0);
    curve.addPoint (2.0, 0.0);

    curve.setPoint (1, { 5.0, 2.0, 3.0 });

    EXPECT_EQ (2.0, curve.getPoint (1).time);
    EXPECT_EQ (1.0, curve.getPoint (1).level);
    EXPECT_EQ (1.0, curve.getPoint (1).curve);

    curve.setPoint (1, { -1.0, -1.0, -3.0 });

    EXPECT_EQ (0.0, curve.getPoint (1).time);
    EXPECT_EQ (0.0, curve.getPoint (1).level);
    EXPECT_EQ (-1.0, curve.getPoint (1).curve);
}

TEST_F (BreakpointCurveTests, RemovePointShiftsMarkers)
{
    Curve curve;

    for (int i = 0; i < 5; ++i)
        curve.addPoint (static_cast<double> (i), 0.5);

    curve.setSustainIndex (3);
    curve.setLoop (2, 3);

    curve.removePoint (1);

    EXPECT_EQ (4, curve.getNumPoints());
    EXPECT_EQ (2, curve.getSustainIndex());
    EXPECT_EQ (1, curve.getLoopStart());
    EXPECT_EQ (2, curve.getLoopEnd());

    curve.removePoint (2);

    EXPECT_EQ (-1, curve.getSustainIndex());
    EXPECT_FALSE (curve.hasLoop());
}

TEST_F (BreakpointCurveTests, InsertionShiftsMarkers)
{
    Curve curve;
    curve.addPoint (0.0, 0.0);
    curve.addPoint (1.0, 1.0);
    curve.addPoint (2.0, 0.5);
    curve.setSustainIndex (2);
    curve.setLoop (1, 2);

    curve.addPoint (0.5, 0.2);

    EXPECT_EQ (3, curve.getSustainIndex());
    EXPECT_EQ (2, curve.getLoopStart());
    EXPECT_EQ (3, curve.getLoopEnd());
}

TEST_F (BreakpointCurveTests, InvalidMarkersAreCleared)
{
    Curve curve;
    curve.addPoint (0.0, 0.0);
    curve.addPoint (1.0, 1.0);

    curve.setSustainIndex (5);
    EXPECT_EQ (-1, curve.getSustainIndex());

    curve.setLoop (1, 1);
    EXPECT_FALSE (curve.hasLoop());

    curve.setLoop (0, 1);
    EXPECT_TRUE (curve.hasLoop());

    curve.clearLoop();
    EXPECT_FALSE (curve.hasLoop());
}

TEST_F (BreakpointCurveTests, EvaluateInterpolatesSegmentsAndClampsOutside)
{
    Curve curve;
    curve.addPoint (1.0, 0.2);
    curve.addPoint (2.0, 1.0);
    curve.addPoint (4.0, 0.0, 0.5);

    EXPECT_EQ (0.2, curve.evaluate (0.0));
    EXPECT_EQ (0.2, curve.evaluate (1.0));
    EXPECT_NEAR (0.6, curve.evaluate (1.5), 1e-12);
    EXPECT_NEAR (Curve::segmentValue (1.0, 0.0, 0.5, 0.25), curve.evaluate (2.5), 1e-12);
    EXPECT_EQ (0.0, curve.evaluate (10.0));
}

TEST_F (BreakpointCurveTests, EvaluateHandlesZeroLengthSegments)
{
    Curve curve;
    curve.addPoint (0.0, 0.0);
    curve.addPoint (0.5, 1.0);
    curve.addPoint (0.5, 0.2);
    curve.addPoint (1.0, 0.2);

    EXPECT_NEAR (0.5, curve.evaluate (0.25), 1e-12);
    EXPECT_NEAR (0.2, curve.evaluate (0.5), 1e-12);
    EXPECT_NEAR (0.2, curve.evaluate (0.75), 1e-12);
}

TEST_F (BreakpointCurveTests, GetSegmentAtReturnsTheEndPoint)
{
    Curve curve;
    EXPECT_EQ (-1, curve.getSegmentAt (0.5));

    curve.addPoint (0.0, 0.0);
    curve.addPoint (1.0, 1.0);
    curve.addPoint (3.0, 0.0);

    EXPECT_EQ (1, curve.getSegmentAt (-1.0));
    EXPECT_EQ (1, curve.getSegmentAt (0.5));
    EXPECT_EQ (2, curve.getSegmentAt (1.0));
    EXPECT_EQ (2, curve.getSegmentAt (2.5));
    EXPECT_EQ (2, curve.getSegmentAt (9.0));
}

TEST_F (BreakpointCurveTests, EmptyAndSinglePointCurves)
{
    Curve curve;
    EXPECT_EQ (0.0, curve.evaluate (0.5));
    EXPECT_EQ (0.0, curve.getLength());

    curve.addPoint (0.25, 0.7);
    EXPECT_EQ (0.7, curve.evaluate (0.0));
    EXPECT_EQ (0.7, curve.evaluate (1.0));
}

TEST_F (BreakpointCurveTests, CurvedRampMatchesShape)
{
    const auto curve = makeRamp (-0.6);

    for (int i = 0; i <= 10; ++i)
        EXPECT_NEAR (Curve::shape (-0.6, i / 10.0), curve.evaluate (i / 10.0), 1e-12);
}

TEST_F (BreakpointCurveTests, EqualityComparesPointsAndMarkers)
{
    auto a = makeRamp (0.0);
    auto b = makeRamp (0.0);
    EXPECT_TRUE (a == b);

    b.setSustainIndex (1);
    EXPECT_FALSE (a == b);

    b.setSustainIndex (-1);
    b.addPoint (2.0, 0.0);
    b.removePoint (2);
    EXPECT_TRUE (a == b);
}
