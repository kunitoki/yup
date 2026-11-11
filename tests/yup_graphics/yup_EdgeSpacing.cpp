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

TEST (EdgeSpacingTests, DefaultConstructor)
{
    EdgeSpacing<float> e;
    EXPECT_FLOAT_EQ (e.getLeft(), 0.0f);
    EXPECT_FLOAT_EQ (e.getTop(), 0.0f);
    EXPECT_FLOAT_EQ (e.getRight(), 0.0f);
    EXPECT_FLOAT_EQ (e.getBottom(), 0.0f);
    EXPECT_TRUE (e.isZero());
    EXPECT_TRUE (e.isUniform());
}

TEST (EdgeSpacingTests, UniformConstructor)
{
    EdgeSpacing<float> e (5.0f);
    EXPECT_FLOAT_EQ (e.getLeft(), 5.0f);
    EXPECT_FLOAT_EQ (e.getTop(), 5.0f);
    EXPECT_FLOAT_EQ (e.getRight(), 5.0f);
    EXPECT_FLOAT_EQ (e.getBottom(), 5.0f);
    EXPECT_FALSE (e.isZero());
    EXPECT_TRUE (e.isUniform());
}

TEST (EdgeSpacingTests, HorizontalVerticalConstructor)
{
    EdgeSpacing<float> e (8.0f, 4.0f);
    EXPECT_FLOAT_EQ (e.getLeft(), 8.0f);
    EXPECT_FLOAT_EQ (e.getRight(), 8.0f);
    EXPECT_FLOAT_EQ (e.getTop(), 4.0f);
    EXPECT_FLOAT_EQ (e.getBottom(), 4.0f);
    EXPECT_FALSE (e.isUniform());
}

TEST (EdgeSpacingTests, FourEdgeConstructor)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (e.getLeft(), 1.0f);
    EXPECT_FLOAT_EQ (e.getTop(), 2.0f);
    EXPECT_FLOAT_EQ (e.getRight(), 3.0f);
    EXPECT_FLOAT_EQ (e.getBottom(), 4.0f);
}

TEST (EdgeSpacingTests, Totals)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (e.getHorizontal(), 4.0f);
    EXPECT_FLOAT_EQ (e.getVertical(), 6.0f);
}

TEST (EdgeSpacingTests, Setters)
{
    EdgeSpacing<float> e;
    e.setLeft (1.0f).setTop (2.0f).setRight (3.0f).setBottom (4.0f);
    EXPECT_EQ (e, EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 4.0f));
}

TEST (EdgeSpacingTests, Withers)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ (e.withLeft (10.0f), EdgeSpacing<float> (10.0f, 2.0f, 3.0f, 4.0f));
    EXPECT_EQ (e.withTop (10.0f), EdgeSpacing<float> (1.0f, 10.0f, 3.0f, 4.0f));
    EXPECT_EQ (e.withRight (10.0f), EdgeSpacing<float> (1.0f, 2.0f, 10.0f, 4.0f));
    EXPECT_EQ (e.withBottom (10.0f), EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 10.0f));
    EXPECT_EQ (e, EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 4.0f));
}

TEST (EdgeSpacingTests, SubtractedFromRectangle)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r (0.0f, 0.0f, 100.0f, 100.0f);

    auto inner = e.subtractedFrom (r);
    EXPECT_EQ (inner, Rectangle<float> (1.0f, 2.0f, 96.0f, 94.0f));
}

TEST (EdgeSpacingTests, SubtractFromRectangleInPlace)
{
    EdgeSpacing<float> e (10.0f);
    Rectangle<float> r (0.0f, 0.0f, 100.0f, 100.0f);
    e.subtractFrom (r);
    EXPECT_EQ (r, Rectangle<float> (10.0f, 10.0f, 80.0f, 80.0f));
}

TEST (EdgeSpacingTests, SubtractedFromClampsToZero)
{
    EdgeSpacing<float> e (100.0f);
    Rectangle<float> r (0.0f, 0.0f, 50.0f, 50.0f);

    auto inner = e.subtractedFrom (r);
    EXPECT_FLOAT_EQ (inner.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (inner.getHeight(), 0.0f);
}

TEST (EdgeSpacingTests, AddedToRectangle)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r (10.0f, 10.0f, 100.0f, 100.0f);

    auto outer = e.addedTo (r);
    EXPECT_EQ (outer, Rectangle<float> (9.0f, 8.0f, 104.0f, 106.0f));
}

TEST (EdgeSpacingTests, AddToRectangleInPlace)
{
    EdgeSpacing<float> e (5.0f);
    Rectangle<float> r (10.0f, 10.0f, 100.0f, 100.0f);
    e.addTo (r);
    EXPECT_EQ (r, Rectangle<float> (5.0f, 5.0f, 110.0f, 110.0f));
}

TEST (EdgeSpacingTests, RectangleReducedOverload)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r (0.0f, 0.0f, 100.0f, 100.0f);
    EXPECT_EQ (r.reduced (e), e.subtractedFrom (r));
}

TEST (EdgeSpacingTests, RectangleEnlargedOverload)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r (10.0f, 10.0f, 100.0f, 100.0f);
    EXPECT_EQ (r.enlarged (e), e.addedTo (r));
}

TEST (EdgeSpacingTests, RectangleReduceInPlaceOverload)
{
    EdgeSpacing<float> e (10.0f);
    Rectangle<float> r (0.0f, 0.0f, 100.0f, 100.0f);
    r.reduce (e);
    EXPECT_EQ (r, Rectangle<float> (10.0f, 10.0f, 80.0f, 80.0f));
}

TEST (EdgeSpacingTests, Addition)
{
    EdgeSpacing<float> a (1.0f, 2.0f, 3.0f, 4.0f);
    EdgeSpacing<float> b (10.0f, 20.0f, 30.0f, 40.0f);
    EXPECT_EQ (a + b, EdgeSpacing<float> (11.0f, 22.0f, 33.0f, 44.0f));

    a += b;
    EXPECT_EQ (a, EdgeSpacing<float> (11.0f, 22.0f, 33.0f, 44.0f));
}

TEST (EdgeSpacingTests, Subtraction)
{
    EdgeSpacing<float> a (10.0f, 20.0f, 30.0f, 40.0f);
    EdgeSpacing<float> b (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ (a - b, EdgeSpacing<float> (9.0f, 18.0f, 27.0f, 36.0f));

    a -= b;
    EXPECT_EQ (a, EdgeSpacing<float> (9.0f, 18.0f, 27.0f, 36.0f));
}

TEST (EdgeSpacingTests, Negation)
{
    EdgeSpacing<float> e (1.0f, -2.0f, 3.0f, -4.0f);
    EXPECT_EQ (-e, EdgeSpacing<float> (-1.0f, 2.0f, -3.0f, 4.0f));
}

TEST (EdgeSpacingTests, ScalarMultiplication)
{
    EdgeSpacing<float> e (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_EQ (e * 2.0f, EdgeSpacing<float> (2.0f, 4.0f, 6.0f, 8.0f));

    e *= 2.0f;
    EXPECT_EQ (e, EdgeSpacing<float> (2.0f, 4.0f, 6.0f, 8.0f));
}

TEST (EdgeSpacingTests, ScalarDivision)
{
    EdgeSpacing<float> e (2.0f, 4.0f, 6.0f, 8.0f);
    EXPECT_EQ (e / 2.0f, EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 4.0f));

    e /= 2.0f;
    EXPECT_EQ (e, EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 4.0f));
}

TEST (EdgeSpacingTests, Conversion)
{
    EdgeSpacing<float> e (1.4f, 2.6f, 3.0f, 4.0f);
    auto i = e.to<int>();
    EXPECT_EQ (i, EdgeSpacing<int> (1, 2, 3, 4));
}

TEST (EdgeSpacingTests, ConvertingConstructor)
{
    EdgeSpacing<int> i (1, 2, 3, 4);
    EdgeSpacing<float> f (i);
    EXPECT_EQ (f, EdgeSpacing<float> (1.0f, 2.0f, 3.0f, 4.0f));
}

TEST (EdgeSpacingTests, RoundToInt)
{
    EdgeSpacing<float> e (1.4f, 2.6f, 3.5f, 4.49f);
    EXPECT_EQ (e.roundToInt(), EdgeSpacing<int> (1, 3, 4, 4));
}

TEST (EdgeSpacingTests, Equality)
{
    EdgeSpacing<float> a (1.0f, 2.0f, 3.0f, 4.0f);
    EdgeSpacing<float> b (1.0f, 2.0f, 3.0f, 4.0f);
    EdgeSpacing<float> c (1.0f, 2.0f, 3.0f, 5.0f);
    EXPECT_TRUE (a == b);
    EXPECT_FALSE (a == c);
    EXPECT_TRUE (a != c);
    EXPECT_TRUE (a.approximatelyEqualTo (b));
}

TEST (EdgeSpacingTests, ToString)
{
    EdgeSpacing<int> e (1, 2, 3, 4);
    EXPECT_EQ (e.toString(), String ("1, 2, 3, 4"));

    String s;
    s << e;
    EXPECT_EQ (s, String ("1, 2, 3, 4"));
}

TEST (EdgeSpacingTests, StructuredBindings)
{
    EdgeSpacing<int> e (1, 2, 3, 4);
    auto [l, t, r, b] = e;
    EXPECT_EQ (l, 1);
    EXPECT_EQ (t, 2);
    EXPECT_EQ (r, 3);
    EXPECT_EQ (b, 4);
}

TEST (EdgeSpacingTests, ConstexprUsage)
{
    constexpr EdgeSpacing<int> e (1, 2, 3, 4);
    static_assert (e.getLeft() == 1);
    static_assert (e.getHorizontal() == 4);
    static_assert (e.getVertical() == 6);
    static_assert (! e.isZero());
    static_assert (e.withLeft (5).getLeft() == 5);
    SUCCEED();
}
