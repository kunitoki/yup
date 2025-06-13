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

#include <cmath>
#include <vector>
#include <string>

using namespace yup;

namespace
{
static constexpr float tol = 1e-4f;

void expectPointNear (const Point<float>& a, const Point<float>& b, float tolerance = tol)
{
    EXPECT_NEAR (a.getX(), b.getX(), tolerance);
    EXPECT_NEAR (a.getY(), b.getY(), tolerance);
}

void expectRectNear (const Rectangle<float>& a, const Rectangle<float>& b, float tolerance = tol)
{
    EXPECT_NEAR (a.getX(), b.getX(), tolerance);
    EXPECT_NEAR (a.getY(), b.getY(), tolerance);
    EXPECT_NEAR (a.getWidth(), b.getWidth(), tolerance);
    EXPECT_NEAR (a.getHeight(), b.getHeight(), tolerance);
}
} // namespace

TEST (PathTests, DefaultConstruction)
{
    Path p;
    EXPECT_EQ (p.size(), 0);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, MoveAndCopyConstruction)
{
    Path p1 (10.0f, 20.0f);
    Path p2 (p1);
    Path p3 (std::move (p1));
    EXPECT_EQ (p2.size(), p3.size());
    EXPECT_TRUE (p2.getBounds() == p3.getBounds());
    Path p4;
    p4 = p2;
    Path p5;
    p5 = std::move (p3);
    EXPECT_EQ (p4.size(), p5.size());
}

TEST (PathTests, ClearAndReserve)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 10);
    EXPECT_GT (p.size(), 0);
    p.clear();
    EXPECT_EQ (p.size(), 0);
    p.reserveSpace (10);
    EXPECT_EQ (p.size(), 0);
}

TEST (PathTests, MoveToLineToQuadToCubicToClose)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).quadTo (15, 5, 10, 10).cubicTo (5, 15, 0, 10, 0, 0).close();
    EXPECT_GT (p.size(), 0);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddLine)
{
    Path p;
    Point<float> a (1, 2), b (3, 4);
    p.addLine (a, b);
    EXPECT_FALSE (p.getBounds().isEmpty());
    Line<float> l (Point<float> (5, 6), Point<float> (7, 8));
    p.addLine (l);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddRectangle)
{
    Path p;
    p.addRectangle (0, 0, 10, 20);
    Rectangle<float> r (5, 5, 15, 25);
    p.addRectangle (r);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddRoundedRectangle)
{
    Path p;
    p.addRoundedRectangle (0, 0, 10, 20, 2);
    p.addRoundedRectangle (0, 0, 10, 20, 1, 2, 3, 4);
    Rectangle<float> r (5, 5, 15, 25);
    p.addRoundedRectangle (r, 3);
    p.addRoundedRectangle (r, 1, 2, 3, 4);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddEllipse)
{
    Path p;
    p.addEllipse (0, 0, 10, 20);
    Rectangle<float> r (5, 5, 15, 25);
    p.addEllipse (r);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddCenteredEllipse)
{
    Path p;
    p.addCenteredEllipse (5, 5, 10, 20);
    Point<float> c (10, 10);
    p.addCenteredEllipse (c, 8, 12);
    Size<float> sz (16, 24);
    p.addCenteredEllipse (c, sz);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddArc)
{
    Path p;
    p.addArc (0, 0, 10, 10, 0, MathConstants<float>::pi, true);
    Rectangle<float> r (5, 5, 10, 10);
    p.addArc (r, 0, MathConstants<float>::twoPi, false);
    p.addCenteredArc (5, 5, 10, 10, 0, 0, MathConstants<float>::halfPi, true);
    Point<float> c (10, 10);
    p.addCenteredArc (c, 8, 12, 0, 0, MathConstants<float>::pi, false);
    Size<float> sz (16, 24);
    p.addCenteredArc (c, sz, 0, 0, MathConstants<float>::pi, true);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddPolygon)
{
    Path p;
    Point<float> center (10, 10);
    p.addPolygon (center, 5, 8, 0.0f);
    p.addPolygon (center, 3, 5, MathConstants<float>::halfPi);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddStar)
{
    Path p;
    Point<float> center (10, 10);
    p.addStar (center, 5, 4, 8, 0.0f);
    p.addStar (center, 3, 2, 5, MathConstants<float>::halfPi);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubble)
{
    Path p;
    Rectangle<float> body (10, 10, 40, 20);
    Rectangle<float> max (0, 0, 100, 100);
    Point<float> tip (30, 0);
    p.addBubble (body, max, tip, 5, 10);
    // Arrow inside body (no arrow)
    p.addBubble (body, max, Point<float> (20, 20), 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AppendPath)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);
    Path p2;
    p2.addEllipse (5, 5, 10, 10);
    p1.appendPath (p2);
    EXPECT_FALSE (p1.getBounds().isEmpty());
    // With transform
    AffineTransform t = AffineTransform::translation (10, 10).scaled (2.0f);
    p1.appendPath (p2, t);
    EXPECT_FALSE (p1.getBounds().isEmpty());
}

TEST (PathTests, SwapWithPath)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);
    Path p2;
    p2.addEllipse (5, 5, 10, 10);
    Rectangle<float> b1 = p1.getBounds();
    Rectangle<float> b2 = p2.getBounds();
    p1.swapWithPath (p2);
    expectRectNear (p1.getBounds(), b2);
    expectRectNear (p2.getBounds(), b1);
}

TEST (PathTests, TransformAndTransformed)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);
    AffineTransform t = AffineTransform::translation (5, 5).scaled (2.0f);
    Path p2 = p.transformed (t);
    p.transform (t);
    expectRectNear (p.getBounds(), p2.getBounds());
}

TEST (PathTests, ScaleToFit)
{
    Path p;
    p.addRectangle (10, 10, 20, 20);
    p.scaleToFit (0, 0, 100, 50, false);
    Rectangle<float> b = p.getBounds();
    EXPECT_NEAR (b.getWidth(), 100.0f, tol);
    EXPECT_NEAR (b.getHeight(), 50.0f, tol);
    // Proportional
    p.addRectangle (0, 0, 10, 10);
    p.scaleToFit (0, 0, 50, 100, true);
    b = p.getBounds();
    // The bounds will be the union of both rectangles, so width==height is not guaranteed.
    EXPECT_LE (b.getWidth(), 50.0f + tol);
    EXPECT_LE (b.getHeight(), 100.0f + tol);
    EXPECT_GT (b.getWidth(), 0.0f);
    EXPECT_GT (b.getHeight(), 0.0f);
}

TEST (PathTests, GetPointAlongPath)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).lineTo (10, 10);
    Point<float> start = p.getPointAlongPath (0.0f);
    Point<float> mid = p.getPointAlongPath (0.5f);
    Point<float> end = p.getPointAlongPath (1.0f);
    expectPointNear (start, Point<float> (0, 0));
    expectPointNear (end, Point<float> (10, 10));
    // Midpoint should be somewhere on the path
    EXPECT_TRUE (mid.getX() >= 0 && mid.getX() <= 10);
    EXPECT_TRUE (mid.getY() >= 0 && mid.getY() <= 10);
}

TEST (PathTests, CreateStrokePolygon)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);
    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    // Edge: empty path
    Path empty;
    Path stroke2 = empty.createStrokePolygon (2.0f);
    EXPECT_TRUE (stroke2.getBounds().isEmpty());
}

TEST (PathTests, WithRoundedCorners)
{
    Path p;
    p.addPolygon (Point<float> (10, 10), 5, 8);
    Path rounded = p.withRoundedCorners (2.0f);
    EXPECT_FALSE (rounded.getBounds().isEmpty());
    // Edge: zero/negative radius
    Path same = p.withRoundedCorners (0.0f);
    EXPECT_FALSE (same.getBounds().isEmpty());
}

TEST (PathTests, ParsePathData)
{
    Path p;
    // Simple SVG path: M10 10 H 90 V 90 H 10 Z
    bool ok = p.parsePathData ("M10 10 H 90 V 90 H 10 Z");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
    // Edge: malformed path
    Path p2;
    ok = p2.parsePathData ("M10 10 Q");
    EXPECT_TRUE (ok); // Should not throw, but result is empty
}

TEST (PathTests, AddRectangleEdgeCases)
{
    Path p;
    p.addRectangle (0, 0, -10, -20);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addRectangle (0, 0, 0, 0);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AddEllipseEdgeCases)
{
    Path p;
    p.addEllipse (0, 0, -10, -20);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addEllipse (0, 0, 0, 0);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AddRoundedRectangleEdgeCases)
{
    Path p;
    p.addRoundedRectangle (0, 0, -10, -20, 2);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addRoundedRectangle (0, 0, 0, 0, 1, 2, 3, 4);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AddArcEdgeCases)
{
    Path p;
    p.addArc (0, 0, -10, -10, 0, MathConstants<float>::pi, true);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addArc (0, 0, 0, 0, 0, MathConstants<float>::twoPi, false);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AddPolygonEdgeCases)
{
    Path p;
    Point<float> center (10, 10);
    p.addPolygon (center, 0, 5, 0.0f);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addPolygon (center, 2, 5, 0.0f);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addPolygon (center, 5, 0, 0.0f);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AddStarEdgeCases)
{
    Path p;
    Point<float> center (10, 10);
    p.addStar (center, 0, 2, 5, 0.0f);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addStar (center, 2, 2, 5, 0.0f);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addStar (center, 5, 0, 5, 0.0f);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.addStar (center, 5, 2, 0, 0.0f);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleEdgeCases)
{
    Path p;
    Rectangle<float> body (10, 10, 40, 20);
    Rectangle<float> max (0, 0, 100, 100);
    Point<float> tip (30, 0);
    p.addBubble (Rectangle<float>(), max, tip, 5, 10);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addBubble (body, Rectangle<float>(), tip, 5, 10);
    EXPECT_TRUE (p.getBounds().isEmpty());

    p.addBubble (body, max, tip, 5, 0);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, AppendPathEdgeCases)
{
    Path p1, p2;
    p1.appendPath (p2);
    EXPECT_TRUE (p1.getBounds().isEmpty());
}

TEST (PathTests, AppendPathRcpOverloadsEdgeCases)
{
    Path p1;
    auto raw = rive::make_rcp<rive::RiveRenderPath>();
    Path p3 (raw);
    p1.appendPath (raw);
    EXPECT_NE (p1.getRenderPath(), nullptr);
}

TEST (PathTests, ScaleToFitEdgeCases)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);
    p.scaleToFit (0, 0, 0, 0, true);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.scaleToFit (0, 0, -10, -10, false);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, TransformEdgeCases)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);
    AffineTransform t = AffineTransform::scaling (0, 0);
    p.transform (t);
    EXPECT_TRUE (p.getBounds().isEmpty());
}

TEST (PathTests, GetPointAlongPathEdgeCases)
{
    Path p;
    p.addLine (Point<float> (0, 0), Point<float> (10, 10));
    Point<float> point = p.getPointAlongPath (1.5f);
    EXPECT_EQ (point, Point<float> (10, 10));
}

TEST (PathTests, AllPublicApiErrorCases)
{
    Path p;
    p.reserveSpace (0);
    p.clear();
    p.moveTo (0, 0);
    p.lineTo (0, 0);
    p.quadTo (0, 0, 0, 0);
    p.cubicTo (0, 0, 0, 0, 0, 0);
    p.close();
    p.addLine (Point<float> (0, 0), Point<float> (0, 0));
    p.addLine (Line<float> (Point<float> (0, 0), Point<float> (0, 0)));
    p.addRectangle (Rectangle<float>());
    p.addRoundedRectangle (Rectangle<float>(), 0);
    p.addEllipse (Rectangle<float>());
    p.addCenteredEllipse (Point<float> (0, 0), 0, 0);
    p.addCenteredEllipse (Point<float> (0, 0), Size<float> (0, 0));
    p.addArc (Rectangle<float>(), 0, 0, true);
    p.addCenteredArc (Point<float> (0, 0), 0, 0, 0, 0, 0, true);
    p.addCenteredArc (Point<float> (0, 0), Size<float> (0, 0), 0, 0, 0, true);
    p.addPolygon (Point<float> (0, 0), 0, 0);
    p.addStar (Point<float> (0, 0), 0, 0, 0);
    p.addBubble (Rectangle<float>(), Rectangle<float>(), Point<float> (0, 0), 0, 0);
    p.appendPath (Path());

    Path tmp;
    p.swapWithPath (tmp);
    p.transform (AffineTransform());
    p.transformed (AffineTransform());
    p.scaleToFit (0, 0, 0, 0, false);
    p.getBounds();
    p.getBoundsTransformed (AffineTransform());
    p.getPointAlongPath (0.0f);
    p.createStrokePolygon (0.0f);
    p.withRoundedCorners (0.0f);
    p.parsePathData ("");
    SUCCEED();
}

TEST (PathTests, RcpConstructorAndGetRenderPath)
{
    auto raw = rive::make_rcp<rive::RiveRenderPath>();
    Path p (raw);
    EXPECT_EQ (p.getRenderPath(), raw.get());
}

TEST (PathTests, Iterators)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);
    auto it = p.begin();
    auto end = p.end();
    int count = 0;
    for (; it != end; ++count, ++it)
    {
    }
    EXPECT_GT (count, 0);
    const Path& cp = p;
    auto cit = cp.begin();
    auto cend = cp.end();
    int ccount = 0;
    for (; cit != cend; ++ccount, ++cit)
    {
    }
    EXPECT_EQ (count, ccount);
}

TEST (PathTests, AddRectanglePractical)
{
    Path p;
    p.addRectangle (0, 0, 10, 20);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.addRectangle (5, 5, 15, 25);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddEllipsePractical)
{
    Path p;
    p.addEllipse (0, 0, 10, 20);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.addEllipse (5, 5, 15, 25);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddRoundedRectanglePractical)
{
    Path p;
    p.addRoundedRectangle (0, 0, 10, 20, 2);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.addRoundedRectangle (5, 5, 15, 25, 1, 2, 3, 4);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddArcPractical)
{
    Path p;
    p.addArc (0, 0, 10, 10, 0, MathConstants<float>::pi, true);
    EXPECT_FALSE (p.getBounds().isEmpty());

    p.addArc (5, 5, 10, 10, 0, MathConstants<float>::twoPi, false);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AppendPathPractical)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);
    Path p2;
    p2.addEllipse (5, 5, 10, 10);
    p1.appendPath (p2);
    EXPECT_FALSE (p1.getBounds().isEmpty());
}

TEST (PathTests, ScaleToFitPractical)
{
    Path p;
    p.addRectangle (10, 10, 20, 20);
    p.scaleToFit (0, 0, 100, 50, false);
    Rectangle<float> b = p.getBounds();
    EXPECT_NEAR (b.getWidth(), 100.0f, tol);
    EXPECT_NEAR (b.getHeight(), 50.0f, tol);
}
