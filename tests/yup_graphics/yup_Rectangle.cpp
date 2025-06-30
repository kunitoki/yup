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

using namespace yup;

namespace
{
static constexpr float tol = 1e-5f;
} // namespace

TEST (RectangleTests, Default_Constructor)
{
    Rectangle<float> r;
    EXPECT_FLOAT_EQ (r.getX(), 0.0f);
    EXPECT_FLOAT_EQ (r.getY(), 0.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 0.0f);
    EXPECT_TRUE (r.isEmpty());
}

TEST (RectangleTests, Parameterized_Constructors)
{
    // Constructor with x, y, width, height
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (r1.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r1.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r1.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r1.getHeight(), 4.0f);

    // Constructor with x, y, Size
    Rectangle<float> r2 (1.0f, 2.0f, Size<float> (3.0f, 4.0f));
    EXPECT_FLOAT_EQ (r2.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r2.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r2.getHeight(), 4.0f);

    // Constructor with Point, width, height
    Rectangle<float> r3 (Point<float> (1.0f, 2.0f), 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (r3.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r3.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r3.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r3.getHeight(), 4.0f);

    // Constructor with Point, Size
    Rectangle<float> r4 (Point<float> (1.0f, 2.0f), Size<float> (3.0f, 4.0f));
    EXPECT_FLOAT_EQ (r4.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r4.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r4.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r4.getHeight(), 4.0f);
}

TEST (RectangleTests, Type_Conversion_Constructor)
{
    Rectangle<int> rInt (1, 2, 3, 4);
    Rectangle<float> rFloat (rInt);
    EXPECT_FLOAT_EQ (rFloat.getX(), 1.0f);
    EXPECT_FLOAT_EQ (rFloat.getY(), 2.0f);
    EXPECT_FLOAT_EQ (rFloat.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (rFloat.getHeight(), 4.0f);
}

TEST (RectangleTests, GetSet_Coordinates)
{
    Rectangle<float> r;
    r.setX (1.0f);
    r.setY (2.0f);
    r.setWidth (3.0f);
    r.setHeight (4.0f);
    EXPECT_FLOAT_EQ (r.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);
}

TEST (RectangleTests, With_Coordinates)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r2 = r.withX (5.0f);
    Rectangle<float> r3 = r.withY (6.0f);
    Rectangle<float> r4 = r.withWidth (7.0f);
    Rectangle<float> r5 = r.withHeight (8.0f);

    EXPECT_FLOAT_EQ (r2.getX(), 5.0f);
    EXPECT_FLOAT_EQ (r3.getY(), 6.0f);
    EXPECT_FLOAT_EQ (r4.getWidth(), 7.0f);
    EXPECT_FLOAT_EQ (r5.getHeight(), 8.0f);
}

TEST (RectangleTests, Position_And_Size)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test getPosition
    Point<float> pos = r.getPosition();
    EXPECT_FLOAT_EQ (pos.getX(), 1.0f);
    EXPECT_FLOAT_EQ (pos.getY(), 2.0f);

    // Test setPosition
    r.setPosition (Point<float> (5.0f, 6.0f));
    EXPECT_FLOAT_EQ (r.getX(), 5.0f);
    EXPECT_FLOAT_EQ (r.getY(), 6.0f);

    // Test withPosition
    Rectangle<float> r2 = r.withPosition (Point<float> (7.0f, 8.0f));
    EXPECT_FLOAT_EQ (r2.getX(), 7.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 8.0f);

    // Test withZeroPosition
    Rectangle<float> r3 = r.withZeroPosition();
    EXPECT_FLOAT_EQ (r3.getX(), 0.0f);
    EXPECT_FLOAT_EQ (r3.getY(), 0.0f);

    // Test getSize
    Size<float> size = r.getSize();
    EXPECT_FLOAT_EQ (size.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (size.getHeight(), 4.0f);

    // Test setSize
    r.setSize (Size<float> (7.0f, 8.0f));
    EXPECT_FLOAT_EQ (r.getWidth(), 7.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 8.0f);

    // Test withSize
    Rectangle<float> r4 = r.withSize (Size<float> (9.0f, 10.0f));
    EXPECT_FLOAT_EQ (r4.getWidth(), 9.0f);
    EXPECT_FLOAT_EQ (r4.getHeight(), 10.0f);

    // Test withZeroSize
    Rectangle<float> r5 = r.withZeroSize();
    EXPECT_FLOAT_EQ (r5.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r5.getHeight(), 0.0f);
}

TEST (RectangleTests, Corners)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test getTopLeft
    Point<float> tl = r.getTopLeft();
    EXPECT_FLOAT_EQ (tl.getX(), 1.0f);
    EXPECT_FLOAT_EQ (tl.getY(), 2.0f);

    // Test getTopRight
    Point<float> tr = r.getTopRight();
    EXPECT_FLOAT_EQ (tr.getX(), 4.0f);
    EXPECT_FLOAT_EQ (tr.getY(), 2.0f);

    // Test getBottomLeft
    Point<float> bl = r.getBottomLeft();
    EXPECT_FLOAT_EQ (bl.getX(), 1.0f);
    EXPECT_FLOAT_EQ (bl.getY(), 6.0f);

    // Test getBottomRight
    Point<float> br = r.getBottomRight();
    EXPECT_FLOAT_EQ (br.getX(), 4.0f);
    EXPECT_FLOAT_EQ (br.getY(), 6.0f);

    // Test setTopLeft
    r.setTopLeft (Point<float> (5.0f, 6.0f));
    EXPECT_FLOAT_EQ (r.getX(), 5.0f);
    EXPECT_FLOAT_EQ (r.getY(), 6.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);

    // Test setTopRight
    r.setTopRight (Point<float> (8.0f, 6.0f));
    EXPECT_FLOAT_EQ (r.getX(), 5.0f);
    EXPECT_FLOAT_EQ (r.getY(), 6.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);

    // Test setBottomLeft
    r.setBottomLeft (Point<float> (5.0f, 9.0f));
    EXPECT_FLOAT_EQ (r.getX(), 5.0f);
    EXPECT_FLOAT_EQ (r.getY(), 5.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);

    // Test setBottomRight
    r.setBottomRight (Point<float> (8.0f, 9.0f));
    EXPECT_FLOAT_EQ (r.getX(), 4.0f);
    EXPECT_FLOAT_EQ (r.getY(), 5.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);
}

TEST (RectangleTests, Center)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test getCenterX
    EXPECT_FLOAT_EQ (r.getCenterX(), 2.5f);

    // Test getCenterY
    EXPECT_FLOAT_EQ (r.getCenterY(), 4.0f);

    // Test getCenter
    Point<float> center = r.getCenter();
    EXPECT_FLOAT_EQ (center.getX(), 2.5f);
    EXPECT_FLOAT_EQ (center.getY(), 4.0f);

    // Test setCenter
    r.setCenter (5.0f, 6.0f);
    EXPECT_FLOAT_EQ (r.getCenterX(), 5.0f);
    EXPECT_FLOAT_EQ (r.getCenterY(), 6.0f);

    // Test withCenter
    Rectangle<float> r2 = r.withCenter (7.0f, 8.0f);
    EXPECT_FLOAT_EQ (r2.getCenterX(), 7.0f);
    EXPECT_FLOAT_EQ (r2.getCenterY(), 8.0f);
}

TEST (RectangleTests, Shape_Checks)
{
    // Test isEmpty/isPoint
    Rectangle<float> empty;
    EXPECT_TRUE (empty.isEmpty());
    EXPECT_TRUE (empty.isPoint());

    // Test isLine
    Rectangle<float> horizontalLine (1.0f, 2.0f, 3.0f, 0.0f);
    Rectangle<float> verticalLine (1.0f, 2.0f, 0.0f, 3.0f);
    EXPECT_TRUE (horizontalLine.isLine());
    EXPECT_TRUE (verticalLine.isLine());
    EXPECT_TRUE (horizontalLine.isHorizontalLine());
    EXPECT_TRUE (verticalLine.isVerticalLine());

    // Test normal rectangle
    Rectangle<float> normal (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FALSE (normal.isEmpty());
    EXPECT_FALSE (normal.isPoint());
    EXPECT_FALSE (normal.isLine());
}

TEST (RectangleTests, Sides)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test leftSide
    Line<float> left = r.leftSide();
    EXPECT_FLOAT_EQ (left.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (left.getStartY(), 2.0f);
    EXPECT_FLOAT_EQ (left.getEndX(), 1.0f);
    EXPECT_FLOAT_EQ (left.getEndY(), 6.0f);

    // Test topSide
    Line<float> top = r.topSide();
    EXPECT_FLOAT_EQ (top.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (top.getStartY(), 2.0f);
    EXPECT_FLOAT_EQ (top.getEndX(), 4.0f);
    EXPECT_FLOAT_EQ (top.getEndY(), 2.0f);

    // Test rightSide
    Line<float> right = r.rightSide();
    EXPECT_FLOAT_EQ (right.getStartX(), 4.0f);
    EXPECT_FLOAT_EQ (right.getStartY(), 2.0f);
    EXPECT_FLOAT_EQ (right.getEndX(), 4.0f);
    EXPECT_FLOAT_EQ (right.getEndY(), 6.0f);

    // Test bottomSide
    Line<float> bottom = r.bottomSide();
    EXPECT_FLOAT_EQ (bottom.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (bottom.getStartY(), 6.0f);
    EXPECT_FLOAT_EQ (bottom.getEndX(), 4.0f);
    EXPECT_FLOAT_EQ (bottom.getEndY(), 6.0f);

    // Test diagonals
    Line<float> diag1 = r.diagonalTopToBottom();
    Line<float> diag2 = r.diagonalBottomToTop();
    EXPECT_FLOAT_EQ (diag1.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (diag1.getStartY(), 2.0f);
    EXPECT_FLOAT_EQ (diag1.getEndX(), 4.0f);
    EXPECT_FLOAT_EQ (diag1.getEndY(), 6.0f);
    EXPECT_FLOAT_EQ (diag2.getStartX(), 1.0f);
    EXPECT_FLOAT_EQ (diag2.getStartY(), 6.0f);
    EXPECT_FLOAT_EQ (diag2.getEndX(), 4.0f);
    EXPECT_FLOAT_EQ (diag2.getEndY(), 2.0f);
}

TEST (RectangleTests, Translation)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test translate with deltas
    r.translate (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getY(), 4.0f);

    // Test translate with point
    r.translate (Point<float> (1.0f, 2.0f));
    EXPECT_FLOAT_EQ (r.getX(), 3.0f);
    EXPECT_FLOAT_EQ (r.getY(), 6.0f);

    // Test translated
    Rectangle<float> r2 = r.translated (1.0f, 2.0f);
    EXPECT_FLOAT_EQ (r2.getX(), 4.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 8.0f);

    Rectangle<float> r3 = r.translated (Point<float> (1.0f, 2.0f));
    EXPECT_FLOAT_EQ (r3.getX(), 4.0f);
    EXPECT_FLOAT_EQ (r3.getY(), 8.0f);
}

TEST (RectangleTests, Scaling)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test scale uniform
    r.scale (2.0f);
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getY(), 4.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 6.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 8.0f);

    // Test scale non-uniform
    r.scale (0.5f, 2.0f);
    EXPECT_FLOAT_EQ (r.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r.getY(), 8.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 16.0f);

    // Test scaled
    Rectangle<float> r2 = r.scaled (2.0f);
    EXPECT_FLOAT_EQ (r2.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 16.0f);
    EXPECT_FLOAT_EQ (r2.getWidth(), 6.0f);
    EXPECT_FLOAT_EQ (r2.getHeight(), 32.0f);

    Rectangle<float> r3 = r.scaled (0.5f, 2.0f);
    EXPECT_FLOAT_EQ (r3.getX(), 0.5f);
    EXPECT_FLOAT_EQ (r3.getY(), 16.0f);
    EXPECT_FLOAT_EQ (r3.getWidth(), 1.5f);
    EXPECT_FLOAT_EQ (r3.getHeight(), 32.0f);
}

TEST (RectangleTests, Remove_From_Sides)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test removeFromTop
    Rectangle<float> top = r.removeFromTop (1.0f);
    EXPECT_FLOAT_EQ (top.getHeight(), 1.0f);
    EXPECT_FLOAT_EQ (r.getY(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 3.0f);

    // Test removeFromLeft
    Rectangle<float> left = r.removeFromLeft (1.0f);
    EXPECT_FLOAT_EQ (left.getWidth(), 1.0f);
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 2.0f);

    // Test removeFromBottom
    Rectangle<float> bottom = r.removeFromBottom (1.0f);
    EXPECT_FLOAT_EQ (bottom.getHeight(), 1.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 2.0f);

    // Test removeFromRight
    Rectangle<float> right = r.removeFromRight (1.0f);
    EXPECT_FLOAT_EQ (right.getWidth(), 1.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 1.0f);
}

TEST (RectangleTests, Remove_From_Sides_Edge_Cases)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test removeFromTop with amount larger than height
    Rectangle<float> top = r.removeFromTop (5.0f);
    EXPECT_FLOAT_EQ (top.getHeight(), 4.0f);
    EXPECT_FLOAT_EQ (r.getY(), 6.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 0.0f);

    // Test removeFromLeft with amount larger than width
    Rectangle<float> left = r.removeFromLeft (5.0f);
    EXPECT_FLOAT_EQ (left.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getX(), 4.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.0f);

    // Test removeFromBottom with amount larger than height
    Rectangle<float> bottom = r.removeFromBottom (5.0f);
    EXPECT_FLOAT_EQ (bottom.getHeight(), 0.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 0.0f);

    // Test removeFromRight with amount larger than width
    Rectangle<float> right = r.removeFromRight (5.0f);
    EXPECT_FLOAT_EQ (right.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.0f);
}

TEST (RectangleTests, Reduce_And_Enlarge)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test reduce uniform
    r.reduce (0.5f);
    EXPECT_FLOAT_EQ (r.getX(), 1.5f);
    EXPECT_FLOAT_EQ (r.getY(), 2.5f);
    EXPECT_FLOAT_EQ (r.getWidth(), 2.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 3.0f);

    // Test reduce non-uniform
    r.reduce (0.5f, 1.0f);
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getY(), 3.5f);
    EXPECT_FLOAT_EQ (r.getWidth(), 1.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 1.0f);

    // Test reduce with all sides
    r.reduce (0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ (r.getX(), 2.1f);
    EXPECT_FLOAT_EQ (r.getY(), 3.7f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.6f);
    EXPECT_FLOAT_EQ (r.getHeight(), 0.4f);

    // Test reduced
    Rectangle<float> r2 = r.reduced (0.1f);
    Rectangle<float> r3 = r.reduced (0.1f, 0.2f);
    Rectangle<float> r4 = r.reduced (0.1f, 0.2f, 0.3f, 0.4f);

    // Test enlarge uniform
    r.enlarge (0.5f);
    EXPECT_FLOAT_EQ (r.getX(), 1.6f);
    EXPECT_FLOAT_EQ (r.getY(), 3.2f);
    EXPECT_FLOAT_EQ (r.getWidth(), 1.6f);
    EXPECT_FLOAT_EQ (r.getHeight(), 1.4f);

    // Test enlarge non-uniform
    r.enlarge (0.5f, 1.0f);
    EXPECT_FLOAT_EQ (r.getX(), 1.1f);
    EXPECT_FLOAT_EQ (r.getY(), 2.2f);
    EXPECT_FLOAT_EQ (r.getWidth(), 2.6f);
    EXPECT_FLOAT_EQ (r.getHeight(), 3.4f);

    // Test enlarged
    Rectangle<float> r5 = r.enlarged (0.5f);
    Rectangle<float> r6 = r.enlarged (0.5f, 1.0f);
}

TEST (RectangleTests, Reduce_And_Enlarge_Edge_Cases)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test reduce with negative values
    r.reduce (-0.5f);
    EXPECT_FLOAT_EQ (r.getX(), 0.5f);
    EXPECT_FLOAT_EQ (r.getY(), 1.5f);
    EXPECT_FLOAT_EQ (r.getWidth(), 4.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 5.0f);

    // Test reduce with values larger than half the size
    r.reduce (2.0f);
    EXPECT_FLOAT_EQ (r.getX(), 2.5f);
    EXPECT_FLOAT_EQ (r.getY(), 3.5f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 1.0f);

    // Test enlarge with negative values
    r.enlarge (-0.5f);
    EXPECT_FLOAT_EQ (r.getX(), 3.0f);
    EXPECT_FLOAT_EQ (r.getY(), 4.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 0.0f);
}

TEST (RectangleTests, Contains)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test contains with coordinates
    EXPECT_TRUE (r.contains (2.0f, 3.0f));
    EXPECT_FALSE (r.contains (0.0f, 0.0f));
    EXPECT_TRUE (r.contains (1.0f, 2.0f)); // Edge case
    EXPECT_TRUE (r.contains (4.0f, 6.0f)); // Edge case

    // Test contains with point
    EXPECT_TRUE (r.contains (Point<float> (2.0f, 3.0f)));
    EXPECT_FALSE (r.contains (Point<float> (0.0f, 0.0f)));

    // Test contains with rect
    EXPECT_TRUE (r.contains (r));
    EXPECT_TRUE (r.contains (r.reduced (0.5f)));
    EXPECT_FALSE (r.contains (Rectangle<float> (2.0f, 2.0f, 3.0f, 4.0f)));
    EXPECT_FALSE (r.contains (r.enlarged (0.5f)));
    EXPECT_FALSE (r.contains (Rectangle<float> (10.0f, 20.0f, 30.0f, 40.0f)));
}

TEST (RectangleTests, Area)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ (r.area(), 12.0f);

    Rectangle<float> empty;
    EXPECT_FLOAT_EQ (empty.area(), 0.0f);
}

TEST (RectangleTests, Intersection)
{
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r2 (2.0f, 3.0f, 3.0f, 4.0f);
    Rectangle<float> r3 (5.0f, 6.0f, 3.0f, 4.0f);

    // Test intersects
    EXPECT_TRUE (r1.intersects (r2));
    EXPECT_FALSE (r1.intersects (r3));

    // Test intersection
    Rectangle<float> intersection = r1.intersection (r2);
    EXPECT_FLOAT_EQ (intersection.getX(), 2.0f);
    EXPECT_FLOAT_EQ (intersection.getY(), 3.0f);
    EXPECT_FLOAT_EQ (intersection.getWidth(), 2.0f);
    EXPECT_FLOAT_EQ (intersection.getHeight(), 3.0f);

    // Test no intersection
    Rectangle<float> noIntersection = r1.intersection (r3);
    EXPECT_TRUE (noIntersection.isEmpty());
}

TEST (RectangleTests, Intersection_Edge_Cases)
{
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> empty;

    // Test intersection with empty rectangle
    Rectangle<float> intersection1 = r1.intersection (empty);
    EXPECT_TRUE (intersection1.isEmpty());

    // Test intersection with zero-size rectangle
    Rectangle<float> zeroSize (2.0f, 3.0f, 0.0f, 0.0f);
    Rectangle<float> intersection2 = r1.intersection (zeroSize);
    EXPECT_TRUE (intersection2.isEmpty());

    // Test intersection with negative size rectangle
    Rectangle<float> negativeSize (2.0f, 3.0f, -1.0f, -1.0f);
    Rectangle<float> intersection3 = r1.intersection (negativeSize);
    EXPECT_TRUE (intersection3.isEmpty());
}

TEST (RectangleTests, Largest_Fitting_Square)
{
    Rectangle<float> r1 (1.0f, 2.0f, 4.0f, 4.0f); // Already square
    Rectangle<float> r2 (1.0f, 2.0f, 4.0f, 3.0f); // Wider than tall
    Rectangle<float> r3 (1.0f, 2.0f, 3.0f, 4.0f); // Taller than wide

    // Test already square
    Rectangle<float> square1 = r1.largestFittingSquare();
    EXPECT_FLOAT_EQ (square1.getWidth(), 4.0f);
    EXPECT_FLOAT_EQ (square1.getHeight(), 4.0f);
    EXPECT_FLOAT_EQ (square1.getX(), 1.0f);
    EXPECT_FLOAT_EQ (square1.getY(), 2.0f);

    // Test wider than tall
    Rectangle<float> square2 = r2.largestFittingSquare();
    EXPECT_FLOAT_EQ (square2.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (square2.getHeight(), 3.0f);
    EXPECT_FLOAT_EQ (square2.getX(), 1.5f);
    EXPECT_FLOAT_EQ (square2.getY(), 2.0f);

    // Test taller than wide
    Rectangle<float> square3 = r3.largestFittingSquare();
    EXPECT_FLOAT_EQ (square3.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (square3.getHeight(), 3.0f);
    EXPECT_FLOAT_EQ (square3.getX(), 1.0f);
    EXPECT_FLOAT_EQ (square3.getY(), 2.5f);
}

TEST (RectangleTests, Union_With)
{
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r2 (2.0f, 3.0f, 3.0f, 4.0f);

    Rectangle<float> containing = r1.unionWith (r2);
    EXPECT_FLOAT_EQ (containing.getX(), 1.0f);
    EXPECT_FLOAT_EQ (containing.getY(), 2.0f);
    EXPECT_FLOAT_EQ (containing.getWidth(), 4.0f);
    EXPECT_FLOAT_EQ (containing.getHeight(), 5.0f);
}

TEST (RectangleTests, Centered_Rectangle_With_Size)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);
    Size<float> size (1.0f, 2.0f);

    Rectangle<float> centered = r.centeredRectangleWithSize (size);
    EXPECT_FLOAT_EQ (centered.getX(), 2.0f);
    EXPECT_FLOAT_EQ (centered.getY(), 3.0f);
    EXPECT_FLOAT_EQ (centered.getWidth(), 1.0f);
    EXPECT_FLOAT_EQ (centered.getHeight(), 2.0f);
}

TEST (RectangleTests, Centered_Rectangle_With_Size_Edge_Cases)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test with size larger than original rectangle
    Size<float> largerSize (5.0f, 6.0f);
    Rectangle<float> centered = r.centeredRectangleWithSize (largerSize);
    EXPECT_FLOAT_EQ (centered.getX(), 0.0f);
    EXPECT_FLOAT_EQ (centered.getY(), 1.0f);
    EXPECT_FLOAT_EQ (centered.getWidth(), 5.0f);
    EXPECT_FLOAT_EQ (centered.getHeight(), 6.0f);

    // Test with zero size
    Size<float> zeroSize;
    Rectangle<float> centeredZero = r.centeredRectangleWithSize (zeroSize);
    EXPECT_FLOAT_EQ (centeredZero.getX(), 2.5f);
    EXPECT_FLOAT_EQ (centeredZero.getY(), 4.0f);
    EXPECT_FLOAT_EQ (centeredZero.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (centeredZero.getHeight(), 0.0f);
}

TEST (RectangleTests, Transform)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test translation
    AffineTransform t1 = AffineTransform::translation (1.0f, 2.0f);
    r.transform (t1);
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getY(), 4.0f);

    // Test rotation
    AffineTransform t2 = AffineTransform::rotation (-MathConstants<float>::halfPi, 1.0f, 2.0f);
    r.transform (t2);
    EXPECT_NEAR (r.getX(), 3.0f, tol);
    EXPECT_NEAR (r.getY(), -2.0f, tol);
    EXPECT_NEAR (r.getWidth(), 4.0f, tol);
    EXPECT_NEAR (r.getHeight(), 3.0f, tol);

    // Test scaling
    AffineTransform t3 = AffineTransform::scaling (2.0f);
    r.transform (t3);
    EXPECT_NEAR (r.getX(), 6.0f, tol);
    EXPECT_NEAR (r.getY(), -4.0f, tol);
    EXPECT_NEAR (r.getWidth(), 8.0f, tol);
    EXPECT_NEAR (r.getHeight(), 6.0f, tol);

    // Test transformed
    Rectangle<float> r2 = r.transformed (t1);
    EXPECT_NEAR (r2.getX(), 7.0f, tol);
    EXPECT_NEAR (r2.getY(), -2.0f, tol);
    EXPECT_NEAR (r2.getWidth(), 8.0f, tol);
    EXPECT_NEAR (r2.getHeight(), 6.0f, tol);
}

TEST (RectangleTests, Transform_Edge_Cases)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test with zero transformation
    AffineTransform zeroTransform;
    r.transform (zeroTransform);
    EXPECT_FLOAT_EQ (r.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);

    // Test with NaN values in transformation
    AffineTransform nanTransform (
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN());
    r.transform (nanTransform);
    EXPECT_TRUE (std::isnan (r.getX()));
    EXPECT_TRUE (std::isnan (r.getY()));
    EXPECT_TRUE (std::isnan (r.getWidth()));
    EXPECT_TRUE (std::isnan (r.getHeight()));
}

TEST (RectangleTests, Type_Conversion)
{
    Rectangle<float> rFloat (1.4f, 2.6f, 3.4f, 4.6f);

    // Test to<int>
    Rectangle<int> rInt = rFloat.to<int>();
    EXPECT_EQ (rInt.getX(), 1);
    EXPECT_EQ (rInt.getY(), 2);
    EXPECT_EQ (rInt.getWidth(), 3);
    EXPECT_EQ (rInt.getHeight(), 4);

    // Test roundToInt
    Rectangle<int> rRounded = rFloat.roundToInt();
    EXPECT_EQ (rRounded.getX(), 1);
    EXPECT_EQ (rRounded.getY(), 3);
    EXPECT_EQ (rRounded.getWidth(), 3);
    EXPECT_EQ (rRounded.getHeight(), 5);
}

TEST (RectangleTests, Arithmetic_Operators)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test multiplication
    Rectangle<float> r2 = r * 2.0f;
    EXPECT_FLOAT_EQ (r2.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 4.0f);
    EXPECT_FLOAT_EQ (r2.getWidth(), 6.0f);
    EXPECT_FLOAT_EQ (r2.getHeight(), 8.0f);

    r *= 2.0f;
    EXPECT_FLOAT_EQ (r.getX(), 2.0f);
    EXPECT_FLOAT_EQ (r.getY(), 4.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 6.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 8.0f);

    // Test division
    Rectangle<float> r3 = r / 2.0f;
    EXPECT_FLOAT_EQ (r3.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r3.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r3.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r3.getHeight(), 4.0f);

    r /= 2.0f;
    EXPECT_FLOAT_EQ (r.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r.getHeight(), 4.0f);
}

TEST (RectangleTests, Arithmetic_Operators_Edge_Cases)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);

    // Test multiplication by zero
    Rectangle<float> r2 = r * 0.0f;
    EXPECT_FLOAT_EQ (r2.getX(), 0.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 0.0f);
    EXPECT_FLOAT_EQ (r2.getWidth(), 0.0f);
    EXPECT_FLOAT_EQ (r2.getHeight(), 0.0f);

    // Test multiplication by negative value
    Rectangle<float> r3 = r * -1.0f;
    EXPECT_FLOAT_EQ (r3.getX(), -1.0f);
    EXPECT_FLOAT_EQ (r3.getY(), -2.0f);
    EXPECT_FLOAT_EQ (r3.getWidth(), -3.0f);
    EXPECT_FLOAT_EQ (r3.getHeight(), -4.0f);

    // Test division by zero
    Rectangle<float> r4 = r;
    r4 /= 0.0f;
    EXPECT_FLOAT_EQ (r4.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r4.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r4.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r4.getHeight(), 4.0f);
}

TEST (RectangleTests, Equality_Operators)
{
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r2 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r3 (2.0f, 3.0f, 4.0f, 5.0f);

    EXPECT_TRUE (r1 == r2);
    EXPECT_FALSE (r1 != r2);
    EXPECT_FALSE (r1 == r3);
    EXPECT_TRUE (r1 != r3);
}

TEST (RectangleTests, Approximately_Equal)
{
    Rectangle<float> r1 (1.0f, 2.0f, 3.0f, 4.0f);
    Rectangle<float> r2 (1.0000001f, 2.0000001f, 3.0000001f, 4.0000001f);
    Rectangle<float> r3 (1.1f, 2.1f, 3.1f, 4.1f);

    EXPECT_TRUE (r1.approximatelyEqualTo (r2));
    EXPECT_FALSE (r1.approximatelyEqualTo (r3));

    Rectangle<int> r4 (1, 2, 3, 4);
    Rectangle<int> r5 (1, 2, 3, 4);
    Rectangle<int> r6 (2, 3, 4, 5);

    EXPECT_TRUE (r4.approximatelyEqualTo (r5));
    EXPECT_FALSE (r4.approximatelyEqualTo (r6));
}

TEST (RectangleTests, String_Output)
{
    Rectangle<float> r (1.5f, 2.5f, 3.5f, 4.5f);
    String str;
    str << r;
    EXPECT_EQ (str, "1.5, 2.5, 3.5, 4.5");
}

TEST (RectangleTests, Structured_Binding)
{
    Rectangle<float> r (1.5f, 2.5f, 3.5f, 4.5f);
    auto [x, y, w, h] = r;
    EXPECT_FLOAT_EQ (x, 1.5f);
    EXPECT_FLOAT_EQ (y, 2.5f);
    EXPECT_FLOAT_EQ (w, 3.5f);
    EXPECT_FLOAT_EQ (h, 4.5f);

    // Test tuple interface
    EXPECT_EQ (std::tuple_size<Rectangle<float>>::value, 4);
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<0, Rectangle<float>>::type, float>) );
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<1, Rectangle<float>>::type, float>) );
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<2, Rectangle<float>>::type, float>) );
    EXPECT_TRUE ((std::is_same_v<std::tuple_element<3, Rectangle<float>>::type, float>) );
}

TEST (RectangleTests, Rive_Conversion)
{
    Rectangle<float> r (1.0f, 2.0f, 3.0f, 4.0f);
    rive::AABB aabb = r.toAABB();

    EXPECT_FLOAT_EQ (aabb.left(), 1.0f);
    EXPECT_FLOAT_EQ (aabb.top(), 2.0f);
    EXPECT_FLOAT_EQ (aabb.right(), 4.0f);
    EXPECT_FLOAT_EQ (aabb.bottom(), 6.0f);

    Rectangle<float> r2 (aabb);
    EXPECT_FLOAT_EQ (r2.getX(), 1.0f);
    EXPECT_FLOAT_EQ (r2.getY(), 2.0f);
    EXPECT_FLOAT_EQ (r2.getWidth(), 3.0f);
    EXPECT_FLOAT_EQ (r2.getHeight(), 4.0f);
}
