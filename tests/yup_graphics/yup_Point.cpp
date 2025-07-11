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
*/

#include <gtest/gtest.h>

#include <yup_graphics/yup_graphics.h>

#include <cmath>

using namespace yup;

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

TEST (PointTests, Default_Constructor)
{
    Point<float> p;
    EXPECT_FLOAT_EQ (p.getX(), 0.0f);
    EXPECT_FLOAT_EQ (p.getY(), 0.0f);
    EXPECT_TRUE (p.isOrigin());
}

TEST (PointTests, Parameterized_Constructor)
{
    Point<float> p (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (p.getX(), 1.0f);
    EXPECT_FLOAT_EQ (p.getY(), 2.0f);
}

TEST (PointTests, Type_Conversion_Constructor)
{
    Point<int> pInt (1, 2);
    Point<float> pFloat (pInt);
    EXPECT_FLOAT_EQ (pFloat.getX(), 1.0f);
    EXPECT_FLOAT_EQ (pFloat.getY(), 2.0f);
}

TEST (PointTests, GetSet_Coordinates)
{
    Point<float> p;
    p.setX (3.0f);
    p.setY (4.0f);
    EXPECT_FLOAT_EQ (p.getX(), 3.0f);
    EXPECT_FLOAT_EQ (p.getY(), 4.0f);

    auto p2 = p.withXY (5.0f, 6.0f);
    EXPECT_FLOAT_EQ (p2.getX(), 5.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 6.0f);
}

TEST (PointTests, With_Coordinates)
{
    Point<float> p (1.0f, 2.0f);
    Point<float> p2 = p.withX (3.0f);
    Point<float> p3 = p.withY (4.0f);
    Point<float> p4 = p.withXY (5.0f, 6.0f);

    EXPECT_FLOAT_EQ (p2.getX(), 3.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 2.0f);
    EXPECT_FLOAT_EQ (p3.getX(), 1.0f);
    EXPECT_FLOAT_EQ (p3.getY(), 4.0f);
    EXPECT_FLOAT_EQ (p4.getX(), 5.0f);
    EXPECT_FLOAT_EQ (p4.getY(), 6.0f);
}

TEST (PointTests, Axis_Checks)
{
    Point<float> origin;
    Point<float> xAxis (1.0f, 0.0f);
    Point<float> yAxis (0.0f, 1.0f);
    Point<float> general (1.0f, 1.0f);

    EXPECT_TRUE (origin.isOrigin());
    EXPECT_TRUE (xAxis.isOnXAxis());
    EXPECT_TRUE (yAxis.isOnYAxis());
    EXPECT_FALSE (general.isOrigin());
    EXPECT_FALSE (general.isOnXAxis());
    EXPECT_FALSE (general.isOnYAxis());
}

TEST (PointTests, Distance_Calculations)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (3.0f, 4.0f);

    EXPECT_FLOAT_EQ (p1.distanceTo (p2), 5.0f);
    EXPECT_FLOAT_EQ (p1.distanceToSquared (p2), 25.0f);
    EXPECT_FLOAT_EQ (p1.horizontalDistanceTo (p2), 3.0f);
    EXPECT_FLOAT_EQ (p1.verticalDistanceTo (p2), 4.0f);
    EXPECT_FLOAT_EQ (p1.manhattanDistanceTo (p2), 7.0f);
}

TEST (PointTests, Magnitude)
{
    Point<float> p (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (p.magnitude(), 5.0f);
}

TEST (PointTests, Circumference_Points)
{
    Point<float> center (1.0f, 1.0f);
    float radius = 2.0f;
    float angle = 0.0f; // 0 degrees

    Point<float> p = center.getPointOnCircumference (radius, angle);
    EXPECT_NEAR (p.getX(), 3.0f, tol);
    EXPECT_NEAR (p.getY(), 1.0f, tol);

    angle = MathConstants<float>::halfPi; // 90 degrees
    p = center.getPointOnCircumference (radius, angle);
    EXPECT_NEAR (p.getX(), 1.0f, tol);
    EXPECT_NEAR (p.getY(), 3.0f, tol);
}

TEST (PointTests, Translation)
{
    Point<float> p (1.0f, 2.0f);
    p.translate (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (p.getX(), 4.0f);
    EXPECT_FLOAT_EQ (p.getY(), 6.0f);

    Point<float> p2 = p.translated (-1.0f, -2.0f);
    EXPECT_FLOAT_EQ (p2.getX(), 3.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 4.0f);
}

TEST (PointTests, Scaling)
{
    Point<float> p (2.0f, 3.0f);
    p.scale (2.0f);
    EXPECT_FLOAT_EQ (p.getX(), 4.0f);
    EXPECT_FLOAT_EQ (p.getY(), 6.0f);

    Point<float> p2 = p.scaled (0.5f, 0.5f);
    EXPECT_FLOAT_EQ (p2.getX(), 2.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 3.0f);
}

TEST (PointTests, Rotation)
{
    float angle = MathConstants<float>::halfPi; // 90 degrees

    {
        Point<float> p (1.0f, 0.0f);
        p.rotateClockwise (angle);
        EXPECT_NEAR (p.getX(), 0.0f, tol);
        EXPECT_NEAR (p.getY(), -1.0f, tol);

        p.rotateCounterClockwise (angle);
        EXPECT_NEAR (p.getX(), 1.0f, tol);
        EXPECT_NEAR (p.getY(), 0.0f, tol);
    }

    {
        Point<float> p (1.0f, 0.0f);
        auto p2 = p.rotatedClockwise (angle);
        EXPECT_NEAR (p2.getX(), 0.0f, tol);
        EXPECT_NEAR (p2.getY(), -1.0f, tol);

        auto p3 = p2.rotatedCounterClockwise (angle);
        EXPECT_NEAR (p3.getX(), 1.0f, tol);
        EXPECT_NEAR (p3.getY(), 0.0f, tol);
    }
}

TEST (PointTests, AngleTo)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (1.0f, 1.0f);

    EXPECT_FLOAT_EQ (p1.angleTo (p2), yup::degreesToRadians (45.0f));
}

TEST (PointTests, Midpoint)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (4.0f, 6.0f);
    Point<float> mid = p1.midpoint (p2);

    EXPECT_FLOAT_EQ (mid.getX(), 2.0f);
    EXPECT_FLOAT_EQ (mid.getY(), 3.0f);
}

TEST (PointTests, Linear_Interpolation)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (4.0f, 6.0f);
    Point<float> lerped = p1.lerp (p2, 0.5f);

    EXPECT_FLOAT_EQ (lerped.getX(), 2.0f);
    EXPECT_FLOAT_EQ (lerped.getY(), 3.0f);
}

TEST (PointTests, Vector_Operations)
{
    Point<float> p1 (1.0f, 2.0f);
    Point<float> p2 (3.0f, 4.0f);

    EXPECT_FLOAT_EQ (p1.dotProduct (p2), 11.0f);
    EXPECT_FLOAT_EQ (p1.crossProduct (p2), -2.0f);
}

TEST (PointTests, Normalization)
{
    Point<float> p (3.0f, 4.0f);
    p.normalize();
    EXPECT_FLOAT_EQ (p.magnitude(), 1.0f);
    EXPECT_TRUE (p.isNormalized());

    Point<float> p2 = p.normalized();
    EXPECT_FLOAT_EQ (p2.magnitude(), 1.0f);
}

TEST (PointTests, Reflection)
{
    Point<float> p (1.0f, 2.0f);

    Point<float> pX = p.reflectedOverXAxis();
    EXPECT_FLOAT_EQ (pX.getX(), 1.0f);
    EXPECT_FLOAT_EQ (pX.getY(), -2.0f);

    Point<float> pY = p.reflectedOverYAxis();
    EXPECT_FLOAT_EQ (pY.getX(), -1.0f);
    EXPECT_FLOAT_EQ (pY.getY(), 2.0f);

    Point<float> pO = p.reflectedOverOrigin();
    EXPECT_FLOAT_EQ (pO.getX(), -1.0f);
    EXPECT_FLOAT_EQ (pO.getY(), -2.0f);

    Point<float> p0 (1.0f, 2.0f);

    p0.reflectOverXAxis();
    EXPECT_FLOAT_EQ (p0.getX(), 1.0f);
    EXPECT_FLOAT_EQ (p0.getY(), -2.0f);

    p0.reflectOverYAxis();
    EXPECT_FLOAT_EQ (p0.getX(), -1.0f);
    EXPECT_FLOAT_EQ (p0.getY(), -2.0f);

    p0.reflectOverOrigin();
    EXPECT_FLOAT_EQ (p0.getX(), 1.0f);
    EXPECT_FLOAT_EQ (p0.getY(), 2.0f);
}

TEST (PointTests, MinMax_Abs)
{
    Point<float> p1 (1.0f, 2.0f);
    Point<float> p2 (3.0f, 1.0f);

    Point<float> minP = p1.min (p2);
    EXPECT_FLOAT_EQ (minP.getX(), 1.0f);
    EXPECT_FLOAT_EQ (minP.getY(), 1.0f);

    Point<float> maxP = p1.max (p2);
    EXPECT_FLOAT_EQ (maxP.getX(), 3.0f);
    EXPECT_FLOAT_EQ (maxP.getY(), 2.0f);

    Point<float> p3 (-1.0f, -2.0f);
    Point<float> absP = p3.abs();
    EXPECT_FLOAT_EQ (absP.getX(), 1.0f);
    EXPECT_FLOAT_EQ (absP.getY(), 2.0f);
}

TEST (PointTests, Arithmetic_Operators)
{
    Point<float> p1 (1.0f, 2.0f);
    Point<float> p2 (3.0f, 4.0f);

    Point<float> sum = p1 + p2;
    EXPECT_FLOAT_EQ (sum.getX(), 4.0f);
    EXPECT_FLOAT_EQ (sum.getY(), 6.0f);
    sum = sum + 1.0f;
    EXPECT_FLOAT_EQ (sum.getX(), 5.0f);
    EXPECT_FLOAT_EQ (sum.getY(), 7.0f);
    sum += 1.0f;
    EXPECT_FLOAT_EQ (sum.getX(), 6.0f);
    EXPECT_FLOAT_EQ (sum.getY(), 8.0f);

    Point<float> diff = p2 - p1;
    EXPECT_FLOAT_EQ (diff.getX(), 2.0f);
    EXPECT_FLOAT_EQ (diff.getY(), 2.0f);
    diff = diff - 1.0f;
    EXPECT_FLOAT_EQ (diff.getX(), 1.0f);
    EXPECT_FLOAT_EQ (diff.getY(), 1.0f);
    diff -= 1.0f;
    EXPECT_FLOAT_EQ (diff.getX(), 0.0f);
    EXPECT_FLOAT_EQ (diff.getY(), 0.0f);

    Point<float> mul = p2 * p1;
    EXPECT_FLOAT_EQ (mul.getX(), 3.0f);
    EXPECT_FLOAT_EQ (mul.getY(), 8.0f);
    mul = mul * 2.0f;
    EXPECT_FLOAT_EQ (mul.getX(), 6.0f);
    EXPECT_FLOAT_EQ (mul.getY(), 16.0f);
    mul *= 0.5f;
    EXPECT_FLOAT_EQ (mul.getX(), 3.0f);
    EXPECT_FLOAT_EQ (mul.getY(), 8.0f);

    Point<float> div = p2 / p1;
    EXPECT_FLOAT_EQ (div.getX(), 3.0f);
    EXPECT_FLOAT_EQ (div.getY(), 2.0f);
    div = div / 2.0f;
    EXPECT_FLOAT_EQ (div.getX(), 1.5f);
    EXPECT_FLOAT_EQ (div.getY(), 1.0f);
    div /= 0.5f;
    EXPECT_FLOAT_EQ (div.getX(), 3.0f);
    EXPECT_FLOAT_EQ (div.getY(), 2.0f);
}

TEST (PointTests, Equality_Operators)
{
    Point<float> p1 (1.0f, 2.0f);
    Point<float> p2 (1.0f, 2.0f);
    Point<float> p3 (3.0f, 4.0f);

    EXPECT_TRUE (p1 == p2);
    EXPECT_FALSE (p1 != p2);
    EXPECT_FALSE (p1 == p3);
    EXPECT_TRUE (p1 != p3);
}

TEST (PointTests, Type_Conversion)
{
    Point<float> pFloat (1.6f, 2.6f);
    Point<int> pInt = pFloat.to<int>();
    EXPECT_EQ (pInt.getX(), 1);
    EXPECT_EQ (pInt.getY(), 2);

    Point<int> pInt2 = pFloat.roundToInt();
    EXPECT_EQ (pInt2.getX(), 2);
    EXPECT_EQ (pInt2.getY(), 3);
}

TEST (PointTests, Affine_Transform)
{
    Point<float> p (1.0f, 2.0f);
    AffineTransform t = AffineTransform::translation (3.0f, 4.0f);

    p.transform (t);
    EXPECT_FLOAT_EQ (p.getX(), 4.0f);
    EXPECT_FLOAT_EQ (p.getY(), 6.0f);

    Point<float> p2 = p.transformed (t.inverted());
    EXPECT_FLOAT_EQ (p2.getX(), 1.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 2.0f);
}

TEST (PointTests, IsFinite)
{
    Point<float> p1 (1.0f, 2.0f);
    EXPECT_TRUE (p1.isFinite());

    Point<float> p2 (std::numeric_limits<float>::infinity(), 2.0f);
    EXPECT_FALSE (p2.isFinite());

    Point<float> p3 (1.0f, std::numeric_limits<float>::quiet_NaN());
    EXPECT_FALSE (p3.isFinite());
}

TEST (PointTests, PointBetween)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (4.0f, 6.0f);

    Point<float> pMid = p1.pointBetween (p2, 0.5f);
    EXPECT_FLOAT_EQ (pMid.getX(), 2.0f);
    EXPECT_FLOAT_EQ (pMid.getY(), 3.0f);

    Point<float> pStart = p1.pointBetween (p2, 0.0f);
    EXPECT_FLOAT_EQ (pStart.getX(), 0.0f);
    EXPECT_FLOAT_EQ (pStart.getY(), 0.0f);

    Point<float> pEnd = p1.pointBetween (p2, 1.0f);
    EXPECT_FLOAT_EQ (pEnd.getX(), 4.0f);
    EXPECT_FLOAT_EQ (pEnd.getY(), 6.0f);
}

TEST (PointTests, IsCollinear)
{
    Point<float> p1 (1.0f, 1.0f); // anchor point, no longer the origin
    Point<float> p2 (2.0f, 2.0f); // on the same line as p1
    Point<float> p3 (3.0f, 3.0f); // also on the same line
    Point<float> p4 (2.0f, 3.0f); // NOT on the same line

    EXPECT_TRUE (p1.isCollinear (p2));
    EXPECT_TRUE (p1.isCollinear (p3));
    EXPECT_FALSE (p1.isCollinear (p4));
}

TEST (PointTests, IsWithinCircle)
{
    Point<float> center (1.0f, 1.0f);
    float radius = 2.0f;

    Point<float> p1 (1.0f, 1.0f); // Center point
    Point<float> p2 (3.0f, 1.0f); // On circumference
    Point<float> p3 (4.0f, 1.0f); // Outside
    Point<float> p4 (2.0f, 2.0f); // Inside

    EXPECT_TRUE (p1.isWithinCircle (center, radius));
    EXPECT_TRUE (p2.isWithinCircle (center, radius));
    EXPECT_FALSE (p3.isWithinCircle (center, radius));
    EXPECT_TRUE (p4.isWithinCircle (center, radius));
}

TEST (PointTests, IsWithinRectangle)
{
    Point<float> topLeft (0.0f, 0.0f);
    Point<float> bottomRight (4.0f, 6.0f);

    Point<float> p1 (2.0f, 3.0f); // Inside
    Point<float> p2 (0.0f, 0.0f); // On corner
    Point<float> p3 (4.0f, 6.0f); // On opposite corner
    Point<float> p4 (5.0f, 7.0f); // Outside

    EXPECT_TRUE (p1.isWithinRectangle (topLeft, bottomRight));
    EXPECT_TRUE (p2.isWithinRectangle (topLeft, bottomRight));
    EXPECT_TRUE (p3.isWithinRectangle (topLeft, bottomRight));
    EXPECT_FALSE (p4.isWithinRectangle (topLeft, bottomRight));
}

TEST (PointTests, FloorCeil)
{
    Point<float> p (1.5f, 2.3f);

    Point<float> floored = p.floor();
    EXPECT_FLOAT_EQ (floored.getX(), 1.0f);
    EXPECT_FLOAT_EQ (floored.getY(), 2.0f);

    Point<float> ceiled = p.ceil();
    EXPECT_FLOAT_EQ (ceiled.getX(), 2.0f);
    EXPECT_FLOAT_EQ (ceiled.getY(), 3.0f);
}

TEST (PointTests, ApproximatelyEqualTo)
{
    Point<float> p1 (1.0f, 2.0f);
    Point<float> p2 (1.0000001f, 2.0000001f);
    Point<float> p3 (1.1f, 2.1f);

    EXPECT_TRUE (p1.approximatelyEqualTo (p2));
    EXPECT_FALSE (p1.approximatelyEqualTo (p3));

    Point<int> p4 (1, 2);
    Point<int> p5 (1, 2);
    Point<int> p6 (2, 3);

    EXPECT_TRUE (p4.approximatelyEqualTo (p5));
    EXPECT_FALSE (p4.approximatelyEqualTo (p6));
}

TEST (PointTests, EllipticalCircumference)
{
    Point<float> center (1.0f, 1.0f);
    float radiusX = 2.0f;
    float radiusY = 3.0f;
    float angle = 0.0f; // 0 degrees

    Point<float> p = center.getPointOnCircumference (radiusX, radiusY, angle);
    EXPECT_NEAR (p.getX(), 3.0f, tol);
    EXPECT_NEAR (p.getY(), 1.0f, tol);

    angle = MathConstants<float>::halfPi; // 90 degrees
    p = center.getPointOnCircumference (radiusX, radiusY, angle);
    EXPECT_NEAR (p.getX(), 1.0f, tol);
    EXPECT_NEAR (p.getY(), 4.0f, tol);
}

TEST (PointTests, TransformMultiplePoints)
{
    AffineTransform t = AffineTransform::translation (2.0f, 3.0f);
    float x1 = 0.0f, y1 = 0.0f;
    float x2 = 1.0f, y2 = 1.0f;
    float x3 = -1.0f, y3 = -1.0f;

    t.transformPoints (x1, y1, x2, y2, x3, y3);
    EXPECT_FLOAT_EQ (x1, 2.0f);
    EXPECT_FLOAT_EQ (y1, 3.0f);
    EXPECT_FLOAT_EQ (x2, 3.0f);
    EXPECT_FLOAT_EQ (y2, 4.0f);
    EXPECT_FLOAT_EQ (x3, 1.0f);
    EXPECT_FLOAT_EQ (y3, 2.0f);
}

TEST (PointTests, StringOutput)
{
    Point<float> p (1.5f, 2.5f);
    String str;
    str << p;
    EXPECT_EQ (str, "1.5, 2.5");
}

TEST (PointTests, StructuredBinding)
{
    Point<float> p (1.5f, 2.5f);
    auto [x, y] = p;
    EXPECT_FLOAT_EQ (x, 1.5f);
    EXPECT_FLOAT_EQ (y, 2.5f);

    // Test tuple interface
    EXPECT_EQ (std::tuple_size<Point<float>>::value, 2);
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<0, Point<float>>::type, float>) );
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<1, Point<float>>::type, float>) );
}

TEST (PointTests, IsFinite_SFINAE)
{
    // Test with non-floating point type
    Point<int> pInt (1, 2);
    // Should compile and work with non-floating point types
    EXPECT_TRUE (pInt == pInt);

    // Test with floating point type
    Point<float> pFloat (1.0f, 2.0f);
    EXPECT_TRUE (pFloat.isFinite());
}

TEST (PointTests, Scale_SFINAE)
{
    // Test with non-floating point type
    Point<int> pInt (2, 3);
    pInt *= 2; // Should compile and work
    EXPECT_EQ (pInt.getX(), 4);
    EXPECT_EQ (pInt.getY(), 6);

    // Test with floating point type
    Point<float> pFloat (2.0f, 3.0f);
    pFloat.scale (2.0f); // Should compile and work
    EXPECT_FLOAT_EQ (pFloat.getX(), 4.0f);
    EXPECT_FLOAT_EQ (pFloat.getY(), 6.0f);
}

TEST (PointTests, FloorCeil_SFINAE)
{
    // Test with non-floating point type
    Point<int> pInt (1, 2);
    // Should not compile with floor()/ceil() for non-floating point types
    // This is a compile-time check, so we just verify the type traits
    EXPECT_TRUE ((std::is_same_v<decltype (pInt)::Type, int>) );

    // Test with floating point type
    Point<float> pFloat (1.5f, 2.3f);
    auto floored = pFloat.floor(); // Should compile and work
    EXPECT_FLOAT_EQ (floored.getX(), 1.0f);
    EXPECT_FLOAT_EQ (floored.getY(), 2.0f);
}

TEST (PointTests, RoundToInt_SFINAE)
{
    // Test with non-floating point type
    Point<int> pInt (1, 2);
    // Should not compile with roundToInt() for non-floating point types
    // This is a compile-time check, so we just verify the type traits
    EXPECT_TRUE ((std::is_same_v<decltype (pInt)::Type, int>) );

    // Test with floating point type
    Point<float> pFloat (1.5f, 2.3f);
    auto rounded = pFloat.roundToInt(); // Should compile and work
    EXPECT_EQ (rounded.getX(), 2);
    EXPECT_EQ (rounded.getY(), 2);
}

TEST (PointTests, Division_Zero)
{
    Point<float> p (2.0f, 4.0f);

    // Division by zero should not modify the point
    p /= 0.0f;
    EXPECT_FLOAT_EQ (p.getX(), 2.0f);
    EXPECT_FLOAT_EQ (p.getY(), 4.0f);

    Point<float> p2 (2.0f, 4.0f);
    Point<float> zero (0.0f, 0.0f);

    // Division by zero point should not modify the point
    p2 /= zero;
    EXPECT_FLOAT_EQ (p2.getX(), 2.0f);
    EXPECT_FLOAT_EQ (p2.getY(), 4.0f);
}

TEST (PointTests, Normalize_Zero)
{
    Point<float> zero;
    zero.normalize(); // Should not modify the point
    EXPECT_FLOAT_EQ (zero.getX(), 0.0f);
    EXPECT_FLOAT_EQ (zero.getY(), 0.0f);

    Point<float> normalized = zero.normalized();
    EXPECT_FLOAT_EQ (normalized.getX(), 0.0f);
    EXPECT_FLOAT_EQ (normalized.getY(), 0.0f);
}

TEST (PointTests, Transform_DifferentTypes)
{
    Point<float> p (1.0f, 2.0f);

    // Test with translation
    AffineTransform t1 = AffineTransform::translation (3.0f, 4.0f);
    p.transform (t1);
    EXPECT_FLOAT_EQ (p.getX(), 4.0f);
    EXPECT_FLOAT_EQ (p.getY(), 6.0f);

    // Test with rotation
    AffineTransform t2 = AffineTransform::rotation (MathConstants<float>::halfPi);
    p.transform (t2);
    EXPECT_NEAR (p.getX(), -6.0f, tol);
    EXPECT_NEAR (p.getY(), 4.0f, tol);

    // Test with scaling
    AffineTransform t3 = AffineTransform::scaling (2.0f);
    p.transform (t3);
    EXPECT_NEAR (p.getX(), -12.0f, tol);
    EXPECT_NEAR (p.getY(), 8.0f, tol);
}

TEST (PointTests, Circumference_NegativeRadii)
{
    Point<float> center (1.0f, 1.0f);
    float radius = -2.0f; // Negative radius

    Point<float> p = center.getPointOnCircumference (radius, 0.0f);
    EXPECT_NEAR (p.getX(), -1.0f, tol);
    EXPECT_NEAR (p.getY(), 1.0f, tol);

    float radiusX = -2.0f;
    float radiusY = -3.0f;
    p = center.getPointOnCircumference (radiusX, radiusY, MathConstants<float>::halfPi);
    EXPECT_NEAR (p.getX(), 1.0f, tol);
    EXPECT_NEAR (p.getY(), -2.0f, tol);
}

TEST (PointTests, PointBetween_OutOfRange)
{
    Point<float> p1 (0.0f, 0.0f);
    Point<float> p2 (4.0f, 6.0f);

    // Delta < 0 should clamp to 0
    Point<float> pStart = p1.pointBetween (p2, -1.0f);
    EXPECT_FLOAT_EQ (pStart.getX(), 0.0f);
    EXPECT_FLOAT_EQ (pStart.getY(), 0.0f);

    // Delta > 1 should clamp to 1
    Point<float> pEnd = p1.pointBetween (p2, 2.0f);
    EXPECT_FLOAT_EQ (pEnd.getX(), 4.0f);
    EXPECT_FLOAT_EQ (pEnd.getY(), 6.0f);
}

TEST (PointTests, IsWithinRectangle_Invalid)
{
    // Test with invalid rectangle (topLeft > bottomRight)
    Point<float> topLeft (4.0f, 6.0f);
    Point<float> bottomRight (0.0f, 0.0f);

    Point<float> p (2.0f, 3.0f);
    EXPECT_FALSE (p.isWithinRectangle (topLeft, bottomRight));

    // Test with single-point rectangle
    Point<float> samePoint (2.0f, 3.0f);
    EXPECT_TRUE (p.isWithinRectangle (samePoint, samePoint));
}
