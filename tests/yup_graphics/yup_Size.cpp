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

using namespace yup;

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

TEST (SizeTests, DefaultConstructor)
{
    Size<float> s;
    EXPECT_FLOAT_EQ (s.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (s.getHeight(), 0.0f);
    EXPECT_TRUE (s.isZero());
    EXPECT_TRUE (s.isEmpty());
    EXPECT_TRUE (s.isSquare());
}

TEST (SizeTests, ParameterizedConstructor)
{
    Size<float> s (3.5f, 4.5f);
    EXPECT_FLOAT_EQ (s.getWidth(), 3.5f);
    EXPECT_FLOAT_EQ (s.getHeight(), 4.5f);
    EXPECT_FALSE (s.isZero());
    EXPECT_FALSE (s.isEmpty());
    EXPECT_FALSE (s.isSquare());
}

TEST (SizeTests, GetSetWidthHeight)
{
    Size<int> s;
    s.setWidth (5).setHeight (6);
    EXPECT_EQ (s.getWidth(), 5);
    EXPECT_EQ (s.getHeight(), 6);
    auto s2 = s.withWidth (7);
    EXPECT_EQ (s2.getWidth(), 7);
    EXPECT_EQ (s2.getHeight(), 6);
    auto s3 = s.withHeight (8);
    EXPECT_EQ (s3.getWidth(), 5);
    EXPECT_EQ (s3.getHeight(), 8);
}

TEST (SizeTests, EmptyAndZero)
{
    Size<int> s1 (0, 5);
    EXPECT_TRUE (s1.isEmpty());
    EXPECT_FALSE (s1.isZero());
    EXPECT_TRUE (s1.isHorizontallyEmpty());
    EXPECT_FALSE (s1.isVerticallyEmpty());

    Size<int> s2 (5, 0);
    EXPECT_TRUE (s2.isEmpty());
    EXPECT_FALSE (s2.isZero());
    EXPECT_FALSE (s2.isHorizontallyEmpty());
    EXPECT_TRUE (s2.isVerticallyEmpty());
}

TEST (SizeTests, SquareCheck)
{
    Size<float> s (5.0f, 5.0f);
    EXPECT_TRUE (s.isSquare());
    s.setHeight (6.0f);
    EXPECT_FALSE (s.isSquare());
}

TEST (SizeTests, Area)
{
    Size<float> s (3.0f, 4.0f);
    EXPECT_FLOAT_EQ (s.area(), 12.0f);
}

TEST (SizeTests, Reverse)
{
    Size<float> s (2.0f, 3.0f);
    auto rev = s.reversed();
    EXPECT_FLOAT_EQ (rev.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (rev.getHeight(), 2.0f);
    s.reverse();
    EXPECT_FLOAT_EQ (s.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (s.getHeight(), 2.0f);
}

TEST (SizeTests, EnlargeReduce)
{
    Size<float> s (2.0f, 3.0f);
    auto enlarged = s.enlarged (1.0f);
    EXPECT_FLOAT_EQ (enlarged.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (enlarged.getHeight(), 4.0f);
    s.enlarge (2.0f, 1.0f);
    EXPECT_FLOAT_EQ (s.getWidth(), 4.0f);
    EXPECT_FLOAT_EQ (s.getHeight(), 4.0f);

    auto reduced = s.reduced (1.0f);
    EXPECT_FLOAT_EQ (reduced.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (reduced.getHeight(), 3.0f);
    s.reduce (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (s.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (s.getHeight(), 2.0f);
}

TEST (SizeTests, Scale)
{
    Size<float> s (3.0f, 4.0f);
    auto scaled = s.scaled (2.0f);
    EXPECT_FLOAT_EQ (scaled.getWidth(), 6.0f);
    EXPECT_FLOAT_EQ (scaled.getHeight(), 8.0f);
    s.scale (0.5f, 0.25f);
    EXPECT_FLOAT_EQ (s.getWidth(), 1.5f);
    EXPECT_FLOAT_EQ (s.getHeight(), 1.0f);
}

TEST (SizeTests, ConvertAndRound)
{
    Size<float> s (3.7f, 4.2f);
    auto toInt = s.to<int>();
    EXPECT_EQ (toInt.getWidth(), 3);
    EXPECT_EQ (toInt.getHeight(), 4);
    auto rounded = s.roundToInt();
    EXPECT_EQ (rounded.getWidth(), 4);
    EXPECT_EQ (rounded.getHeight(), 4);
}

TEST (SizeTests, PrimitiveConversions)
{
    Size<float> s (3.7f, 4.2f);

    auto toPoint = s.toPoint();
    EXPECT_FLOAT_EQ (toPoint.getX(), 3.7f);
    EXPECT_FLOAT_EQ (toPoint.getY(), 4.2f);

    auto toPointInt = s.toPoint<int>();
    EXPECT_EQ (toPointInt.getX(), 3);
    EXPECT_EQ (toPointInt.getY(), 4);

    auto toRectangle1 = s.toRectangle();
    EXPECT_FLOAT_EQ (toRectangle1.getX(), 0.0f);
    EXPECT_FLOAT_EQ (toRectangle1.getY(), 0.0f);
    EXPECT_FLOAT_EQ (toRectangle1.getWidth(), 3.7f);
    EXPECT_FLOAT_EQ (toRectangle1.getHeight(), 4.2f);

    auto toRectangle2 = s.toRectangle (1.2f, 2.9f);
    EXPECT_FLOAT_EQ (toRectangle2.getX(), 1.2f);
    EXPECT_FLOAT_EQ (toRectangle2.getY(), 2.9f);
    EXPECT_FLOAT_EQ (toRectangle2.getWidth(), 3.7f);
    EXPECT_FLOAT_EQ (toRectangle2.getHeight(), 4.2f);

    auto toRectangle3 = s.toRectangle (Point<float> { 1.2f, 2.9f });
    EXPECT_FLOAT_EQ (toRectangle3.getX(), 1.2f);
    EXPECT_FLOAT_EQ (toRectangle3.getY(), 2.9f);
    EXPECT_FLOAT_EQ (toRectangle3.getWidth(), 3.7f);
    EXPECT_FLOAT_EQ (toRectangle3.getHeight(), 4.2f);

    auto toRectangleInt = s.toRectangle<int>();
    EXPECT_EQ (toRectangleInt.getX(), 0);
    EXPECT_EQ (toRectangleInt.getY(), 0);
    EXPECT_EQ (toRectangleInt.getWidth(), 3);
    EXPECT_EQ (toRectangleInt.getHeight(), 4);
}

TEST (SizeTests, ArithmeticOperators)
{
    {
        Size<int> s (2, 4);
        auto mul = s * 2;
        EXPECT_FLOAT_EQ (mul.getWidth(), 4);
        EXPECT_FLOAT_EQ (mul.getHeight(), 8);
        s *= 2;
        EXPECT_FLOAT_EQ (s.getWidth(), 4);
        EXPECT_FLOAT_EQ (s.getHeight(), 8);
        auto div = mul / 2;
        EXPECT_FLOAT_EQ (div.getWidth(), 2);
        EXPECT_FLOAT_EQ (div.getHeight(), 4);
        mul /= 2;
        EXPECT_FLOAT_EQ (mul.getWidth(), 2);
        EXPECT_FLOAT_EQ (mul.getHeight(), 4);
    }

    {
        Size<float> s (2.0f, 3.0f);
        auto mul = s * 2.0f;
        EXPECT_FLOAT_EQ (mul.getWidth(), 4.0f);
        EXPECT_FLOAT_EQ (mul.getHeight(), 6.0f);
        s *= 0.5f;
        EXPECT_FLOAT_EQ (s.getWidth(), 1.0f);
        EXPECT_FLOAT_EQ (s.getHeight(), 1.5f);
        auto div = mul / 2.0f;
        EXPECT_FLOAT_EQ (div.getWidth(), 2.0f);
        EXPECT_FLOAT_EQ (div.getHeight(), 3.0f);
        mul /= 2.0f;
        EXPECT_FLOAT_EQ (mul.getWidth(), 2.0f);
        EXPECT_FLOAT_EQ (mul.getHeight(), 3.0f);
    }
}

TEST (SizeTests, EqualityAndApproxEqual)
{
    Size<float> s1 (2.0f, 3.0f), s2 (2.0000001f, 3.0000001f), s3 (2.1f, 3.1f);
    EXPECT_TRUE (s1 == s1);
    EXPECT_FALSE (s1 != s1);
    EXPECT_TRUE (s1.approximatelyEqualTo (s2));
    EXPECT_FALSE (s1.approximatelyEqualTo (s3));
}

TEST (SizeTests, StructuredBinding)
{
    Size<int> s (1, 2);
    auto [w, h] = s;
    EXPECT_EQ (w, 1);
    EXPECT_EQ (h, 2);
}
