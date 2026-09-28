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

using namespace yup;

TEST (Vector3Tests, DefaultIsOrigin)
{
    constexpr Vector3<float> v;
    EXPECT_EQ (v, Vector3<float> (0.0f, 0.0f, 0.0f));
}

TEST (Vector3Tests, AccessorsAndWithers)
{
    constexpr Vector3<float> v (1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ (v.getX(), 1.0f);
    EXPECT_FLOAT_EQ (v.getY(), 2.0f);
    EXPECT_FLOAT_EQ (v.getZ(), 3.0f);

    EXPECT_EQ (v.withX (5.0f), Vector3<float> (5.0f, 2.0f, 3.0f));
    EXPECT_EQ (v.withY (5.0f), Vector3<float> (1.0f, 5.0f, 3.0f));
    EXPECT_EQ (v.withZ (5.0f), Vector3<float> (1.0f, 2.0f, 5.0f));
}

TEST (Vector3Tests, ArithmeticOperators)
{
    const Vector3<float> a (1.0f, 2.0f, 3.0f);
    const Vector3<float> b (4.0f, 5.0f, 6.0f);

    EXPECT_EQ (a + b, Vector3<float> (5.0f, 7.0f, 9.0f));
    EXPECT_EQ (b - a, Vector3<float> (3.0f, 3.0f, 3.0f));
    EXPECT_EQ (a * 2.0f, Vector3<float> (2.0f, 4.0f, 6.0f));
    EXPECT_EQ (2.0f * a, Vector3<float> (2.0f, 4.0f, 6.0f));
    EXPECT_EQ (b / 2.0f, Vector3<float> (2.0f, 2.5f, 3.0f));
    EXPECT_EQ (-a, Vector3<float> (-1.0f, -2.0f, -3.0f));

    auto c = a;
    c += b;
    EXPECT_EQ (c, Vector3<float> (5.0f, 7.0f, 9.0f));
    c -= b;
    EXPECT_EQ (c, a);
    c *= 3.0f;
    EXPECT_EQ (c, Vector3<float> (3.0f, 6.0f, 9.0f));
}

TEST (Vector3Tests, DotProduct)
{
    EXPECT_FLOAT_EQ (Vector3<float> (1.0f, 2.0f, 3.0f).dotProduct ({ 4.0f, -5.0f, 6.0f }), 12.0f);
    EXPECT_FLOAT_EQ (Vector3<float> (1.0f, 0.0f, 0.0f).dotProduct ({ 0.0f, 1.0f, 0.0f }), 0.0f);
}

TEST (Vector3Tests, CrossProductFollowsRightHandRule)
{
    const Vector3<float> x (1.0f, 0.0f, 0.0f);
    const Vector3<float> y (0.0f, 1.0f, 0.0f);

    EXPECT_EQ (x.crossProduct (y), Vector3<float> (0.0f, 0.0f, 1.0f));
    EXPECT_EQ (y.crossProduct (x), Vector3<float> (0.0f, 0.0f, -1.0f));
    EXPECT_EQ (x.crossProduct (x), Vector3<float>());
}

TEST (Vector3Tests, LengthAndNormalized)
{
    const Vector3<float> v (2.0f, 3.0f, 6.0f);
    EXPECT_FLOAT_EQ (v.lengthSquared(), 49.0f);
    EXPECT_FLOAT_EQ (v.length(), 7.0f);

    const auto n = v.normalized();
    EXPECT_NEAR (n.length(), 1.0f, 1.0e-6f);
    EXPECT_TRUE (n.approximatelyEqualTo ({ 2.0f / 7.0f, 3.0f / 7.0f, 6.0f / 7.0f }));
}

TEST (Vector3Tests, NormalizingZeroVectorKeepsItZero)
{
    EXPECT_EQ (Vector3<float>().normalized(), Vector3<float>());
}

TEST (Vector3Tests, ConvertsBetweenTypes)
{
    const Vector3<float> v (1.75f, -2.25f, 3.5f);
    EXPECT_EQ (v.to<int>(), Vector3<int> (1, -2, 3));
    EXPECT_EQ (Vector3<int> (1, 2, 3).to<double>(), Vector3<double> (1.0, 2.0, 3.0));
}

TEST (Vector3Tests, EqualityAndApproximateEquality)
{
    const Vector3<float> a (1.0f, 2.0f, 3.0f);
    EXPECT_TRUE (a == Vector3<float> (1.0f, 2.0f, 3.0f));
    EXPECT_TRUE (a != Vector3<float> (1.0f, 2.0f, 4.0f));
    EXPECT_TRUE (a.approximatelyEqualTo (a + Vector3<float> (0.0f, 0.0f, 1.0e-7f)));
    EXPECT_FALSE (a.approximatelyEqualTo (a + Vector3<float> (0.0f, 0.0f, 1.0e-2f)));
    EXPECT_TRUE (Vector3<int> (1, 2, 3).approximatelyEqualTo ({ 1, 2, 3 }));
}

TEST (Vector3Tests, ToString)
{
    EXPECT_EQ (Vector3<int> (1, 2, 3).toString(), "1, 2, 3");
}
