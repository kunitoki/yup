/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
#include <tuple>

using namespace yup;

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

TEST (LineTests, DefaultConstructor)
{
    Line<float> l;
    EXPECT_EQ (l.getStart(), Point<float> (0, 0));
    EXPECT_EQ (l.getEnd(), Point<float> (0, 0));
    EXPECT_FLOAT_EQ (l.length(), 0.0f);
    EXPECT_FLOAT_EQ (l.slope(), 0.0f);
    EXPECT_TRUE (l.contains (Point<float> (0, 0)));
}

TEST (LineTests, ParameterizedConstructor)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ (l.getStart(), Point<float> (1.0f, 2.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (3.0f, 4.0f));
    EXPECT_FLOAT_EQ (l.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (l.getStartY(), 2.0f);
    EXPECT_FLOAT_EQ (l.getEndX(), 3.0f);
    EXPECT_FLOAT_EQ (l.getEndY(), 4.0f);
}

TEST (LineTests, SetAndWithStartEnd)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    l.setStart (Point<float> (5.0f, 6.0f));
    EXPECT_EQ (l.getStart(), Point<float> (5.0f, 6.0f));
    auto l2 = l.withStart (Point<float> (7.0f, 8.0f));
    EXPECT_EQ (l2.getStart(), Point<float> (7.0f, 8.0f));
    l.setEnd (Point<float> (9.0f, 10.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (9.0f, 10.0f));
    auto l3 = l.withEnd (Point<float> (11.0f, 12.0f));
    EXPECT_EQ (l3.getEnd(), Point<float> (11.0f, 12.0f));
}

TEST (LineTests, Reverse)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    auto rev = l.reversed();
    EXPECT_EQ (rev.getStart(), Point<float> (3.0f, 4.0f));
    EXPECT_EQ (rev.getEnd(), Point<float> (1.0f, 2.0f));
    l.reverse();
    EXPECT_EQ (l.getStart(), Point<float> (3.0f, 4.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (1.0f, 2.0f));
}

TEST (LineTests, LengthAndSlope)
{
    Line<float> l (0.0f, 0.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (l.length(), 5.0f);
    EXPECT_FLOAT_EQ (l.slope(), 4.0f / 3.0f);
    Line<float> v (1.0f, 1.0f, 1.0f, 5.0f);
    EXPECT_FLOAT_EQ (v.slope(), 0.0f);
}

TEST (LineTests, Contains)
{
    Line<float> l (0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE (l.contains (Point<float> (5.0f, 5.0f)));
    EXPECT_FALSE (l.contains (Point<float> (5.0f, 6.0f)));
    EXPECT_TRUE (l.contains (Point<float> (5.001f, 5.001f), 0.01f));
}

TEST (LineTests, PointAlong)
{
    Line<float> l (0.0f, 0.0f, 10.0f, 0.0f);
    EXPECT_EQ (l.pointAlong (0.0f), Point<float> (0, 0));
    EXPECT_EQ (l.pointAlong (0.5f), Point<float> (5, 0));
    EXPECT_EQ (l.pointAlong (1.0f), Point<float> (10, 0));
    EXPECT_EQ (l.pointAlong (1.5f), Point<float> (15, 0));
}

TEST (LineTests, Translate)
{
    Line<float> l (0.0f, 0.0f, 1.0f, 1.0f);
    auto t = l.translated (2.0f, 3.0f);
    EXPECT_EQ (t.getStart(), Point<float> (2.0f, 3.0f));
    EXPECT_EQ (t.getEnd(), Point<float> (3.0f, 4.0f));
    l.translate (1.0f, 1.0f);
    EXPECT_EQ (l.getStart(), Point<float> (1.0f, 1.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (2.0f, 2.0f));
}

TEST (LineTests, ExtendBeforeAfter)
{
    Line<float> l (0.0f, 0.0f, 10.0f, 0.0f);
    auto eb = l.extendedBefore (5.0f);
    EXPECT_EQ (eb.getStart(), Point<float> (-5.0f, 0.0f));
    auto ea = l.extendedAfter (5.0f);
    EXPECT_EQ (ea.getEnd(), Point<float> (15.0f, 0.0f));
}

TEST (LineTests, KeepOnlyStartAndEnd)
{
    Line<float> l (0.0f, 0.0f, 10.0f, 0.0f);
    auto ks = l.keepOnlyStart (0.5f);
    EXPECT_EQ (ks.getEnd(), Point<float> (5.0f, 0.0f));
    auto ke = l.keepOnlyEnd (0.5f);
    EXPECT_EQ (ke.getStart(), Point<float> (5.0f, 0.0f));
}

TEST (LineTests, RotateAtPoint)
{
    Line<float> l (2.0f, 0.0f, 4.0f, 0.0f);
    auto rl = l.rotateAtPoint (Point<float> (2.0f, 0.0f), MathConstants<float>::halfPi);
    EXPECT_NEAR (rl.getStartX(), 2.0f, tol);
    EXPECT_NEAR (rl.getStartY(), 0.0f, tol);
    EXPECT_NEAR (rl.getEndX(), 2.0f, tol);
    EXPECT_NEAR (rl.getEndY(), 2.0f, tol);
}

TEST (LineTests, ToAndRoundToInt)
{
    Line<float> lf (1.2f, 2.3f, 3.4f, 4.5f);
    auto lint = lf.roundToInt();
    EXPECT_EQ (lint.getStart(), Point<int> (1, 2));
    EXPECT_EQ (lint.getEnd(), Point<int> (3, 4));
    auto toInt = lf.to<int>();
    EXPECT_EQ (toInt.getStart(), Point<int> (1, 2));
    EXPECT_EQ (toInt.getEnd(), Point<int> (3, 4));
}

TEST (LineTests, UnaryMinus)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    auto neg = -l;
    EXPECT_EQ (neg.getStart(), Point<float> (-1.0f, -2.0f));
    EXPECT_EQ (neg.getEnd(), Point<float> (-3.0f, -4.0f));
}

TEST (LineTests, Equality)
{
    Line<float> l1 (0.0f, 0.0f, 1.0f, 1.0f);
    Line<float> l2 (0.0f, 0.0f, 1.0f, 1.0f);
    Line<float> l3 (1.0f, 1.0f, 2.0f, 2.0f);
    EXPECT_TRUE (l1 == l2);
    EXPECT_FALSE (l1 != l2);
    EXPECT_FALSE (l1 == l3);
    EXPECT_TRUE (l1 != l3);
}

TEST (LineTests, StructuredBinding)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    auto [x1, y1, x2, y2] = l;
    EXPECT_EQ (x1, 1.0f);
    EXPECT_EQ (y1, 2.0f);
    EXPECT_EQ (x2, 3.0f);
    EXPECT_EQ (y2, 4.0f);
}

TEST (LineTests, Accessors)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ (l.getStart(), Point<float> (1.0f, 2.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (3.0f, 4.0f));
}

TEST (LineTests, StreamOutput)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);
    String str;
    str << l;
    EXPECT_EQ (str, "1, 2, 3, 4");
}
