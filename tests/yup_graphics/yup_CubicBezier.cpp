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

#include <yup_graphics/yup_graphics.h>

#include <cmath>

using namespace yup;

namespace
{

// A collinear cubic bezier forming a straight 100-unit horizontal line.
// With p1 = p0 + 1/3*(p3-p0) and p2 = p0 + 2/3*(p3-p0), B(t) = 100*t exactly.
CubicBezier makeHorizontalLine (float length = 100.0f)
{
    return CubicBezier ({ 0.0f, 0.0f },
                        { length / 3.0f, 0.0f },
                        { 2.0f * length / 3.0f, 0.0f },
                        { length, 0.0f });
}

// A vertical line for angle tests
CubicBezier makeVerticalLine (float length = 100.0f)
{
    return CubicBezier ({ 0.0f, 0.0f },
                        { 0.0f, length / 3.0f },
                        { 0.0f, 2.0f * length / 3.0f },
                        { 0.0f, length });
}

} // namespace

class CubicBezierTests : public ::testing::Test
{
};

// =============================================================================
// Construction
// =============================================================================

TEST_F (CubicBezierTests, DefaultConstructor_AllPointsAtOrigin)
{
    CubicBezier b;
    EXPECT_FLOAT_EQ (b.p0().getX(), 0.0f);
    EXPECT_FLOAT_EQ (b.p0().getY(), 0.0f);
    EXPECT_FLOAT_EQ (b.p3().getX(), 0.0f);
    EXPECT_FLOAT_EQ (b.p3().getY(), 0.0f);
}

TEST_F (CubicBezierTests, ExplicitConstructor_StoresPoints)
{
    CubicBezier b ({ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 });
    EXPECT_FLOAT_EQ (b.p0().getX(), 1.0f);
    EXPECT_FLOAT_EQ (b.p0().getY(), 2.0f);
    EXPECT_FLOAT_EQ (b.p1().getX(), 3.0f);
    EXPECT_FLOAT_EQ (b.p1().getY(), 4.0f);
    EXPECT_FLOAT_EQ (b.p2().getX(), 5.0f);
    EXPECT_FLOAT_EQ (b.p2().getY(), 6.0f);
    EXPECT_FLOAT_EQ (b.p3().getX(), 7.0f);
    EXPECT_FLOAT_EQ (b.p3().getY(), 8.0f);
}

TEST_F (CubicBezierTests, FromPoints_EquivalentToConstructor)
{
    const auto a = CubicBezier ({ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 });
    const auto b = CubicBezier::fromPoints ({ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 });

    EXPECT_FLOAT_EQ (a.p0().getX(), b.p0().getX());
    EXPECT_FLOAT_EQ (a.p0().getY(), b.p0().getY());
    EXPECT_FLOAT_EQ (a.p3().getX(), b.p3().getX());
    EXPECT_FLOAT_EQ (a.p3().getY(), b.p3().getY());
}

// =============================================================================
// pointAt
// =============================================================================

TEST_F (CubicBezierTests, PointAt_ZeroReturnsP0)
{
    auto b = makeHorizontalLine();
    const auto pt = b.pointAt (0.0f);
    EXPECT_NEAR (pt.getX(), 0.0f, 1e-4f);
    EXPECT_NEAR (pt.getY(), 0.0f, 1e-4f);
}

TEST_F (CubicBezierTests, PointAt_OneReturnsP3)
{
    auto b = makeHorizontalLine (100.0f);
    const auto pt = b.pointAt (1.0f);
    EXPECT_NEAR (pt.getX(), 100.0f, 1e-4f);
    EXPECT_NEAR (pt.getY(), 0.0f, 1e-4f);
}

TEST_F (CubicBezierTests, PointAt_HalfOnStraightLineIsMidpoint)
{
    auto b = makeHorizontalLine (100.0f);
    const auto pt = b.pointAt (0.5f);
    EXPECT_NEAR (pt.getX(), 50.0f, 1e-4f);
    EXPECT_NEAR (pt.getY(), 0.0f, 1e-4f);
}

TEST_F (CubicBezierTests, PointAt_QuarterOnStraightLine)
{
    auto b = makeHorizontalLine (100.0f);
    const auto pt = b.pointAt (0.25f);
    EXPECT_NEAR (pt.getX(), 25.0f, 1e-4f);
    EXPECT_NEAR (pt.getY(), 0.0f, 1e-4f);
}

TEST_F (CubicBezierTests, PointAt_IsOnCurveForArbitraryCubic)
{
    CubicBezier b ({ 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 });
    const auto p0 = b.pointAt (0.0f);
    const auto p1 = b.pointAt (1.0f);
    EXPECT_NEAR (p0.getX(), 0.0f, 1e-4f);
    EXPECT_NEAR (p0.getY(), 0.0f, 1e-4f);
    EXPECT_NEAR (p1.getX(), 100.0f, 1e-4f);
    EXPECT_NEAR (p1.getY(), 0.0f, 1e-4f);
}

// =============================================================================
// length
// =============================================================================

TEST_F (CubicBezierTests, Length_StraightHorizontalLineApproximatesEuclidean)
{
    auto b = makeHorizontalLine (100.0f);
    EXPECT_NEAR (b.length(), 100.0f, 0.5f);
}

TEST_F (CubicBezierTests, Length_DegenerateCurveAllSamePointIsZero)
{
    CubicBezier b ({ 5, 5 }, { 5, 5 }, { 5, 5 }, { 5, 5 });
    EXPECT_NEAR (b.length(), 0.0f, 1e-3f);
}

TEST_F (CubicBezierTests, Length_IsPositiveForNonDegenerate)
{
    CubicBezier b ({ 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 });
    EXPECT_GT (b.length(), 0.0f);
}

// =============================================================================
// tAtLength
// =============================================================================

TEST_F (CubicBezierTests, TAtLength_ZeroLengthReturnsZero)
{
    auto b = makeHorizontalLine (100.0f);
    EXPECT_NEAR (b.tAtLength (0.0f), 0.0f, 1e-3f);
}

TEST_F (CubicBezierTests, TAtLength_HalfLengthReturnsApproximatelyHalf)
{
    auto b = makeHorizontalLine (100.0f);
    EXPECT_NEAR (b.tAtLength (50.0f, 100.0f), 0.5f, 5e-3f);
}

TEST_F (CubicBezierTests, TAtLength_FullLengthReturnsOne)
{
    auto b = makeHorizontalLine (100.0f);
    const float len = b.length();
    EXPECT_NEAR (b.tAtLength (len, len), 1.0f, 5e-3f);
}

TEST_F (CubicBezierTests, TAtLength_SingleArgComputesTotalInternally)
{
    auto b = makeHorizontalLine (100.0f);
    EXPECT_NEAR (b.tAtLength (50.0f), 0.5f, 5e-3f);
}

// =============================================================================
// split
// =============================================================================

TEST_F (CubicBezierTests, Split_TwoHalvesCoverFullCurve)
{
    auto b = makeHorizontalLine (100.0f);
    CubicBezier first, second;
    b.split (first, second);

    EXPECT_NEAR (first.p0().getX(), 0.0f, 1e-3f);
    EXPECT_NEAR (second.p3().getX(), 100.0f, 1e-3f);
    // Junction point should match
    EXPECT_NEAR (first.p3().getX(), second.p0().getX(), 1e-3f);
    EXPECT_NEAR (first.p3().getY(), second.p0().getY(), 1e-3f);
}

TEST_F (CubicBezierTests, Split_EachHalfApproximatelyHalfLength)
{
    auto b = makeHorizontalLine (100.0f);
    CubicBezier first, second;
    b.split (first, second);

    EXPECT_NEAR (first.length(), 50.0f, 1.0f);
    EXPECT_NEAR (second.length(), 50.0f, 1.0f);
}

// =============================================================================
// parameterSplitLeft
// =============================================================================

TEST_F (CubicBezierTests, ParameterSplitLeft_LeftPortionStartsAtP0)
{
    auto b = makeHorizontalLine (100.0f);
    CubicBezier left;
    b.parameterSplitLeft (0.5f, left);

    EXPECT_NEAR (left.p0().getX(), 0.0f, 1e-3f);
    EXPECT_NEAR (left.p0().getY(), 0.0f, 1e-3f);
}

TEST_F (CubicBezierTests, ParameterSplitLeft_RightPortionEndsAtP3)
{
    auto b = makeHorizontalLine (100.0f);
    CubicBezier left;
    b.parameterSplitLeft (0.5f, left); // b is now [0.5, 1]

    EXPECT_NEAR (b.p3().getX(), 100.0f, 1e-3f);
    EXPECT_NEAR (b.p3().getY(), 0.0f, 1e-3f);
}

// =============================================================================
// splitAtLength
// =============================================================================

TEST_F (CubicBezierTests, SplitAtLength_LeftAndRightTotalLengthIsApproxOriginal)
{
    CubicBezier b ({ 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 });
    const float totalLen = b.length();
    const float splitLen = totalLen * 0.3f;

    CubicBezier left, right;
    b.splitAtLength (splitLen, left, right);

    EXPECT_NEAR (left.length() + right.length(), totalLen, 1.0f);
}

// =============================================================================
// onInterval
// =============================================================================

TEST_F (CubicBezierTests, OnInterval_FullIntervalReproducesOriginalEndpoints)
{
    auto b = makeHorizontalLine (100.0f);
    const auto sub = b.onInterval (0.0f, 1.0f);

    EXPECT_NEAR (sub.p0().getX(), b.p0().getX(), 1e-3f);
    EXPECT_NEAR (sub.p3().getX(), b.p3().getX(), 1e-3f);
}

TEST_F (CubicBezierTests, OnInterval_HalfIntervalHasApproximatelyHalfLength)
{
    auto b = makeHorizontalLine (100.0f);
    const auto sub = b.onInterval (0.0f, 0.5f);

    EXPECT_NEAR (sub.length(), 50.0f, 1.0f);
}

TEST_F (CubicBezierTests, OnInterval_SubcurveStartMatchesPointAt)
{
    CubicBezier b ({ 0, 0 }, { 0, 100 }, { 100, 100 }, { 100, 0 });
    const float t0 = 0.25f, t1 = 0.75f;
    const auto sub = b.onInterval (t0, t1);

    const auto expectedStart = b.pointAt (t0);
    const auto expectedEnd = b.pointAt (t1);

    EXPECT_NEAR (sub.p0().getX(), expectedStart.getX(), 1e-3f);
    EXPECT_NEAR (sub.p0().getY(), expectedStart.getY(), 1e-3f);
    EXPECT_NEAR (sub.p3().getX(), expectedEnd.getX(), 1e-3f);
    EXPECT_NEAR (sub.p3().getY(), expectedEnd.getY(), 1e-3f);
}

// =============================================================================
// angleAt
// =============================================================================

TEST_F (CubicBezierTests, AngleAt_HorizontalLineIsZeroRadians)
{
    auto b = makeHorizontalLine (100.0f);
    EXPECT_NEAR (b.angleAt (0.5f), 0.0f, 1e-3f);
}

TEST_F (CubicBezierTests, AngleAt_VerticalLineIsHalfPiRadians)
{
    auto b = makeVerticalLine (100.0f);
    EXPECT_NEAR (b.angleAt (0.5f), yup::MathConstants<float>::pi / 2.0f, 1e-3f);
}

TEST_F (CubicBezierTests, AngleAt_DiagonalLineIsQuarterPiRadians)
{
    // Diagonal line (1,0)→(1,1) as a cubic: both interior control points on line
    CubicBezier b ({ 0.0f, 0.0f }, { 50.0f, 50.0f }, { 50.0f, 50.0f }, { 100.0f, 100.0f });
    EXPECT_NEAR (b.angleAt (0.5f), yup::MathConstants<float>::pi / 4.0f, 1e-2f);
}

// =============================================================================
// derivative
// =============================================================================

TEST_F (CubicBezierTests, Derivative_HorizontalLineAtMidIsPositiveX)
{
    // Horizontal line from (0,0) to (100,0)
    auto b = makeHorizontalLine (100.0f);
    const auto d = b.derivative (0.5f);
    EXPECT_GT (d.getX(), 0.0f);          // moving right
    EXPECT_NEAR (d.getY(), 0.0f, 1e-3f); // no vertical component
}

TEST_F (CubicBezierTests, Derivative_VerticalLineAtMidIsPositiveY)
{
    auto b = makeVerticalLine (100.0f);
    const auto d = b.derivative (0.5f);
    EXPECT_NEAR (d.getX(), 0.0f, 1e-3f);
    EXPECT_GT (d.getY(), 0.0f);
}

TEST_F (CubicBezierTests, Derivative_AtZeroMatchesFirstSegmentDirection)
{
    // Start tangent = 3 * (P1 - P0)
    CubicBezier b ({ 0.0f, 0.0f }, { 10.0f, 0.0f }, { 90.0f, 0.0f }, { 100.0f, 0.0f });
    const auto d = b.derivative (0.0f);
    // 3 * (10 - 0) = 30 in X
    EXPECT_NEAR (d.getX(), 30.0f, 1.0f);
    EXPECT_NEAR (d.getY(), 0.0f, 1e-3f);
}

TEST_F (CubicBezierTests, Derivative_AtOneMatchesLastSegmentDirection)
{
    // End tangent = 3 * (P3 - P2)
    CubicBezier b ({ 0.0f, 0.0f }, { 10.0f, 0.0f }, { 90.0f, 0.0f }, { 100.0f, 0.0f });
    const auto d = b.derivative (1.0f);
    // 3 * (100 - 90) = 30 in X
    EXPECT_NEAR (d.getX(), 30.0f, 1.0f);
    EXPECT_NEAR (d.getY(), 0.0f, 1e-3f);
}
