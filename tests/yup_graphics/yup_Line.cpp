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

    // Test the specific case mentioned by the user
    EXPECT_TRUE (l.contains (Point<float> (5.0f, 5.1f), 0.2f));

    // Test edge cases with corrected distance calculation
    Line<float> l2 (0.0f, 0.0f, 10.0f, 0.0f); // Horizontal line
    EXPECT_TRUE (l2.contains (Point<float> (5.0f, 0.1f), 0.2f));
    EXPECT_FALSE (l2.contains (Point<float> (5.0f, 0.3f), 0.2f));

    // Test zero-length line edge case
    Line<float> zeroLine (5.0f, 5.0f, 5.0f, 5.0f);
    EXPECT_TRUE (zeroLine.contains (Point<float> (5.0f, 5.0f), 0.1f));
    EXPECT_TRUE (zeroLine.contains (Point<float> (5.05f, 5.05f), 0.1f));
    EXPECT_FALSE (zeroLine.contains (Point<float> (5.2f, 5.2f), 0.1f));
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
    static constexpr float tol = 1e-5f;

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

// Additional missing tests
TEST (LineTests, ExtendMethods)
{
    Line<float> l (5.0f, 0.0f, 15.0f, 0.0f);

    // Test extend method (non-const version)
    l.extend (5.0f);
    EXPECT_EQ (l.getStart(), Point<float> (0.0f, 0.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (20.0f, 0.0f));

    // Test extendBefore method (non-const version)
    l.extendBefore (5.0f);
    EXPECT_EQ (l.getStart(), Point<float> (-5.0f, 0.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (20.0f, 0.0f));

    // Test extendAfter method (non-const version)
    l.extendAfter (5.0f);
    EXPECT_EQ (l.getStart(), Point<float> (-5.0f, 0.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (25.0f, 0.0f));
}

TEST (LineTests, TransformMethods)
{
    Line<float> l (1.0f, 2.0f, 3.0f, 4.0f);

    // Test transform method
    AffineTransform t = AffineTransform::translation (5.0f, 6.0f);
    l.transform (t);
    EXPECT_EQ (l.getStart(), Point<float> (6.0f, 8.0f));
    EXPECT_EQ (l.getEnd(), Point<float> (8.0f, 10.0f));

    // Test transformed method
    Line<float> l2 (1.0f, 2.0f, 3.0f, 4.0f);
    AffineTransform t2 = AffineTransform::scaling (2.0f);
    Line<float> transformed = l2.transformed (t2);
    EXPECT_EQ (transformed.getStart(), Point<float> (2.0f, 4.0f));
    EXPECT_EQ (transformed.getEnd(), Point<float> (6.0f, 8.0f));

    // Original line should be unchanged
    EXPECT_EQ (l2.getStart(), Point<float> (1.0f, 2.0f));
    EXPECT_EQ (l2.getEnd(), Point<float> (3.0f, 4.0f));
}

TEST (LineTests, EdgeCases)
{
    // Test with zero-length line
    Line<float> zeroLine (5.0f, 5.0f, 5.0f, 5.0f);
    EXPECT_FLOAT_EQ (zeroLine.length(), 0.0f);
    EXPECT_TRUE (zeroLine.contains (Point<float> (5.0f, 5.0f)));

    // Test slope with vertical line
    Line<float> verticalLine (1.0f, 1.0f, 1.0f, 5.0f);
    EXPECT_FLOAT_EQ (verticalLine.slope(), 0.0f); // Vertical line has slope 0 in this implementation

    // Test pointAlong with negative and > 1.0 values
    Line<float> l (0.0f, 0.0f, 10.0f, 0.0f);
    EXPECT_EQ (l.pointAlong (-0.5f), Point<float> (-5.0f, 0.0f));
    EXPECT_EQ (l.pointAlong (2.0f), Point<float> (20.0f, 0.0f));

    // Test extend with negative values
    Line<float> l2 (10.0f, 10.0f, 20.0f, 10.0f);
    l2.extend (-5.0f);
    EXPECT_EQ (l2.getStart(), Point<float> (15.0f, 10.0f));
    EXPECT_EQ (l2.getEnd(), Point<float> (15.0f, 10.0f));

    // Test contains with tolerance on edge cases
    Line<float> l3 (0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE (l3.contains (Point<float> (0.0f, 0.0f)));
    EXPECT_TRUE (l3.contains (Point<float> (10.0f, 10.0f)));
    EXPECT_FALSE (l3.contains (Point<float> (5.0f, 6.0f)));
    EXPECT_TRUE (l3.contains (Point<float> (5.0f, 5.1f), 0.2f)); // Corrected: should be true with sufficient tolerance
}

TEST (LineTests, TypeConversionEdgeCases)
{
    // Test with extreme values
    Line<float> l (1.9f, 2.1f, 3.9f, 4.1f);
    auto rounded = l.roundToInt();
    EXPECT_EQ (rounded.getStart(), Point<int> (2, 2));
    EXPECT_EQ (rounded.getEnd(), Point<int> (4, 4));

    // Test with negative values
    Line<float> lNeg (-1.7f, -2.3f, -3.1f, -4.9f);
    auto roundedNeg = lNeg.roundToInt();
    EXPECT_EQ (roundedNeg.getStart(), Point<int> (-2, -2));
    EXPECT_EQ (roundedNeg.getEnd(), Point<int> (-3, -5));
}

TEST (LineTests, RotationEdgeCases)
{
    static constexpr float tol = 1e-5f;

    // Test rotation with different angles
    Line<float> l (0.0f, 0.0f, 2.0f, 0.0f);

    // 180 degree rotation
    auto rotated180 = l.rotateAtPoint (Point<float> (1.0f, 0.0f), MathConstants<float>::pi);
    EXPECT_NEAR (rotated180.getStartX(), 2.0f, tol);
    EXPECT_NEAR (rotated180.getStartY(), 0.0f, tol);
    EXPECT_NEAR (rotated180.getEndX(), 0.0f, tol);
    EXPECT_NEAR (rotated180.getEndY(), 0.0f, tol);

    // 270 degree rotation
    auto rotated270 = l.rotateAtPoint (Point<float> (0.0f, 0.0f), -MathConstants<float>::halfPi);
    EXPECT_NEAR (rotated270.getStartX(), 0.0f, tol);
    EXPECT_NEAR (rotated270.getStartY(), 0.0f, tol);
    EXPECT_NEAR (rotated270.getEndX(), 0.0f, tol);
    EXPECT_NEAR (rotated270.getEndY(), -2.0f, tol);
}

TEST (LineTests, ExtendWithDifferentDirections)
{
    // Test extend with diagonal line
    Line<float> diagLine (0.0f, 0.0f, 3.0f, 4.0f); // Length = 5
    auto extended = diagLine.extended (5.0f);
    EXPECT_FLOAT_EQ (extended.length(), 15.0f);

    // Test extend before/after with diagonal line
    auto extendedBefore = diagLine.extendedBefore (5.0f);
    auto extendedAfter = diagLine.extendedAfter (5.0f);
    EXPECT_FLOAT_EQ (extendedBefore.length(), 10.0f);
    EXPECT_FLOAT_EQ (extendedAfter.length(), 10.0f);
}

TEST (LineTests, ComplexTransformations)
{
    Line<float> l (1.0f, 1.0f, 2.0f, 2.0f);

    // Test combination of transformations
    AffineTransform complex = AffineTransform::translation (5.0f, 5.0f)
                                  .scaled (2.0f)
                                  .rotated (MathConstants<float>::pi / 4.0f);

    Line<float> transformed = l.transformed (complex);
    EXPECT_FALSE (transformed.getStart() == l.getStart());
    EXPECT_FALSE (transformed.getEnd() == l.getEnd());
}

TEST (LineTests, ApproximatelyEqual)
{
    Line<float> l1 (1.0f, 2.0f, 3.0f, 4.0f);
    Line<float> l2 (1.0000001f, 2.0000001f, 3.0000001f, 4.0000001f);
    Line<float> l3 (1.1f, 2.1f, 3.1f, 4.1f);

    // Test floating point precision
    EXPECT_TRUE (l1.getStart().approximatelyEqualTo (l2.getStart()));
    EXPECT_TRUE (l1.getEnd().approximatelyEqualTo (l2.getEnd()));
    EXPECT_FALSE (l1.getStart().approximatelyEqualTo (l3.getStart()));
    EXPECT_FALSE (l1.getEnd().approximatelyEqualTo (l3.getEnd()));
}
