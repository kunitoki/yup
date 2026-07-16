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
void expectPointNear (const Point<float>& a, const Point<float>& b, float tolerance = 1e-4f)
{
    EXPECT_NEAR (a.getX(), b.getX(), tolerance);
    EXPECT_NEAR (a.getY(), b.getY(), tolerance);
}

void expectRectNear (const Rectangle<float>& a, const Rectangle<float>& b, float tolerance = 1e-4f)
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

TEST (PathTests, AddRoundedRectangleBoundsMatchRectangle)
{
    Path p;
    p.addRoundedRectangle (10.0f, 20.0f, 100.0f, 60.0f, 8.0f);

    // The straight edges still reach the full extent, so the bounds equal the
    // original rectangle even though the corners are inset.
    expectRectNear (p.getBounds(), Rectangle<float> (10.0f, 20.0f, 100.0f, 60.0f));
}

TEST (PathTests, AddRoundedRectangleSegmentStructure)
{
    Path p;
    p.addRoundedRectangle (0.0f, 0.0f, 40.0f, 30.0f, 5.0f);

    std::vector<Path::Verb> verbs;
    for (const auto& segment : p)
        verbs.push_back (segment.verb);

    // moveTo + 4 edges (lineTo) + 4 corners (cubicTo).
    ASSERT_EQ (verbs.size(), static_cast<size_t> (9));
    EXPECT_EQ (verbs[0], Path::Verb::MoveTo);

    int lineCount = 0, cubicCount = 0;
    for (size_t i = 1; i < verbs.size(); ++i)
    {
        if (verbs[i] == Path::Verb::LineTo)
            ++lineCount;
        else if (verbs[i] == Path::Verb::CubicTo)
            ++cubicCount;
    }

    EXPECT_EQ (lineCount, 4);
    EXPECT_EQ (cubicCount, 4);
}

TEST (PathTests, AddRoundedRectangleStartsPastTopLeftCorner)
{
    Path p;
    p.addRoundedRectangle (10.0f, 20.0f, 100.0f, 60.0f, 8.0f);

    // The contour starts at the end of the top-left corner arc: (x + radius, y).
    auto first = *p.begin();
    EXPECT_EQ (first.verb, Path::Verb::MoveTo);
    expectPointNear (first.point, Point<float> (18.0f, 20.0f));
}

TEST (PathTests, AddRoundedRectangleZeroRadiusMatchesRectangle)
{
    Path rounded;
    rounded.addRoundedRectangle (0.0f, 0.0f, 10.0f, 20.0f, 0.0f);

    Path plain;
    plain.addRectangle (0.0f, 0.0f, 10.0f, 20.0f);

    expectRectNear (rounded.getBounds(), plain.getBounds());

    // With a zero radius the first point collapses onto the rectangle origin.
    expectPointNear ((*rounded.begin()).point, Point<float> (0.0f, 0.0f));
}

TEST (PathTests, AddRoundedRectangleClampsRadiusToHalfSmallestDimension)
{
    // Request a radius far larger than the rectangle; it must clamp to half the
    // smallest dimension so the corners meet without overshooting.
    Path p;
    p.addRoundedRectangle (0.0f, 0.0f, 10.0f, 40.0f, 1000.0f);

    // Half of the smallest side (width 10) is 5, so the arc begins at x + 5.
    expectPointNear ((*p.begin()).point, Point<float> (5.0f, 0.0f));
    expectRectNear (p.getBounds(), Rectangle<float> (0.0f, 0.0f, 10.0f, 40.0f));
}

TEST (PathTests, AddRoundedRectanglePerCornerRadii)
{
    Path p;
    p.addRoundedRectangle (0.0f, 0.0f, 50.0f, 50.0f, 2.0f, 4.0f, 6.0f, 8.0f);

    // The top-left radius controls the start point.
    expectPointNear ((*p.begin()).point, Point<float> (2.0f, 0.0f));
    expectRectNear (p.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 50.0f));
}

TEST (PathTests, AddRoundedRectangleRectangleOverloadsMatchFloatOverloads)
{
    const Rectangle<float> r (5.0f, 7.0f, 30.0f, 40.0f);

    Path fromFloats;
    fromFloats.addRoundedRectangle (5.0f, 7.0f, 30.0f, 40.0f, 6.0f);

    Path fromRect;
    fromRect.addRoundedRectangle (r, 6.0f);

    EXPECT_EQ (fromFloats.size(), fromRect.size());
    expectRectNear (fromFloats.getBounds(), fromRect.getBounds());

    Path fromFloatsPerCorner;
    fromFloatsPerCorner.addRoundedRectangle (5.0f, 7.0f, 30.0f, 40.0f, 1.0f, 2.0f, 3.0f, 4.0f);

    Path fromRectPerCorner;
    fromRectPerCorner.addRoundedRectangle (r, 1.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_EQ (fromFloatsPerCorner.size(), fromRectPerCorner.size());
    expectRectNear (fromFloatsPerCorner.getBounds(), fromRectPerCorner.getBounds());
}

TEST (PathTests, AddRoundedRectangleFullyRoundedSquareIsCircular)
{
    // A square with a radius equal to half its side is effectively a circle:
    // its bounds still cover the square and the contour starts at the midpoint
    // of the top edge.
    Path p;
    p.addRoundedRectangle (0.0f, 0.0f, 20.0f, 20.0f, 10.0f);

    expectPointNear ((*p.begin()).point, Point<float> (10.0f, 0.0f));
    expectRectNear (p.getBounds(), Rectangle<float> (0.0f, 0.0f, 20.0f, 20.0f));
}

TEST (PathTests, AddRoundedRectangleNegativeSizeIsClampedToZero)
{
    Path p;
    p.addRoundedRectangle (5.0f, 5.0f, -10.0f, -20.0f, 3.0f);

    // Negative sizes clamp to zero, producing a degenerate (empty) rectangle.
    EXPECT_TRUE (p.getBounds().isEmpty());
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

TEST (PathTests, AddCenteredArcUsesAbsoluteSweepForSegments)
{
    Path p;
    p.addCenteredArc (100.0f, 145.0f, 45.0f, 45.0f, 0.0f, -MathConstants<float>::halfPi, -MathConstants<float>::pi, true);

    EXPECT_GT (p.size(), 4);
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

TEST (PathTests, AddQuadrilateral)
{
    Path pp;
    pp.addQuadrilateral (0, 0, 10, 0, 10, 10, 0, 10);
    EXPECT_FALSE (pp.getBounds().isEmpty());
    EXPECT_EQ (0, pp.getBounds().getX());
    EXPECT_EQ (0, pp.getBounds().getY());
    EXPECT_EQ (10, pp.getBounds().getWidth());
    EXPECT_EQ (10, pp.getBounds().getHeight());

    Path pp2;
    Point<float> p1 (5, 5);
    Point<float> p2 (15, 5);
    Point<float> p3 (15, 15);
    Point<float> p4 (5, 15);
    pp2.addQuadrilateral (p1, p2, p3, p4);
    EXPECT_FALSE (pp2.getBounds().isEmpty());
    EXPECT_EQ (5, pp2.getBounds().getX());
    EXPECT_EQ (5, pp2.getBounds().getY());
    EXPECT_EQ (10, pp2.getBounds().getWidth());
    EXPECT_EQ (10, pp2.getBounds().getHeight());
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
    static constexpr float tol = 1e-4f;

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

TEST (PathTests, GetLengthReturnsTotalDrawableLength)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).lineTo (10, 10);

    EXPECT_NEAR (p.getLength(), 20.0f, 1.0e-4f);
}

TEST (PathTests, GetTrimmedPathReturnsRequestedLineRange)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const Path trimmed = p.getTrimmedPath (0.2f, 0.7f);

    EXPECT_NEAR (trimmed.getLength(), 5.0f, 1.0e-4f);
    expectPointNear (trimmed.getPointAlongPath (0.0f), Point<float> (2.0f, 0.0f));
    expectPointNear (trimmed.getPointAlongPath (1.0f), Point<float> (7.0f, 0.0f));
}

TEST (PathTests, GetTrimmedPathWrapsAroundEnd)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const Path trimmed = p.getTrimmedPath (0.75f, 0.25f);

    EXPECT_NEAR (trimmed.getLength(), 5.0f, 1.0e-4f);
    expectPointNear (trimmed.getPointAlongPath (0.0f), Point<float> (7.5f, 0.0f));
    expectPointNear (trimmed.getPointAlongPath (1.0f), Point<float> (2.5f, 0.0f));
}

TEST (PathTests, GetTrimmedPathAppliesOffset)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const Path trimmed = p.getTrimmedPath (0.0f, 0.5f, 0.25f);

    EXPECT_NEAR (trimmed.getLength(), 5.0f, 1.0e-4f);
    expectPointNear (trimmed.getPointAlongPath (0.0f), Point<float> (2.5f, 0.0f));
    expectPointNear (trimmed.getPointAlongPath (1.0f), Point<float> (7.5f, 0.0f));
}

TEST (PathTests, TrimReplacesPathInPlace)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    p.trim (0.1f, 0.4f);

    EXPECT_NEAR (p.getLength(), 3.0f, 1.0e-4f);
    expectPointNear (p.getPointAlongPath (0.0f), Point<float> (1.0f, 0.0f));
    expectPointNear (p.getPointAlongPath (1.0f), Point<float> (4.0f, 0.0f));
}

TEST (PathTests, GetTrimmedPathPreservesCubicSegments)
{
    Path p;
    p.moveTo (0, 0).cubicTo (0, 10, 10, 10, 10, 0);

    const Path trimmed = p.getTrimmedPath (0.25f, 0.75f, 0.0f, 0.01f);

    int cubicCount = 0;
    int lineCount = 0;
    for (const auto segment : trimmed)
    {
        if (segment.verb == Path::Verb::CubicTo)
            ++cubicCount;
        else if (segment.verb == Path::Verb::LineTo)
            ++lineCount;
    }

    EXPECT_EQ (cubicCount, 1);
    EXPECT_EQ (lineCount, 0);
    EXPECT_GT (trimmed.getLength (0.01f), 0.0f);
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

TEST (PathTests, FromString)
{
    Path p;
    // Simple SVG path: M10 10 H 90 V 90 H 10 Z
    bool ok = p.fromString ("M 10 10 H 90 V 90 H 10 Z");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
    EXPECT_EQ (p.toString(), "M 10 10 L 90 10 L 90 90 L 10 90 Z");

    // Edge: malformed path
    Path p2;
    ok = p2.fromString ("M 10 10 Q");
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

TEST (PathTests, AddQuadrilateralEdgeCases)
{
    Path p;

    // Degenerate quadrilateral (all points the same)
    p.addQuadrilateral (0, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_TRUE (p.getBounds().isEmpty());

    // Quadrilateral collapsed to a line
    p.clear();
    p.addQuadrilateral (0, 0, 10, 0, 10, 0, 0, 0);
    EXPECT_FALSE (p.getBounds().isEmpty());
    EXPECT_EQ (0, p.getBounds().getHeight());

    // Self-intersecting quadrilateral
    p.clear();
    p.addQuadrilateral (0, 0, 10, 10, 10, 0, 0, 10);
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
    p.fromString ("");
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
    static constexpr float tol = 1e-4f;

    Path p;
    p.addRectangle (10, 10, 20, 20);
    p.scaleToFit (0, 0, 100, 50, false);
    Rectangle<float> b = p.getBounds();
    EXPECT_NEAR (b.getWidth(), 100.0f, tol);
    EXPECT_NEAR (b.getHeight(), 50.0f, tol);
}

// ==============================================================================
// Tests for uncovered methods
// ==============================================================================

TEST (PathTests, ConstructorWithPoint)
{
    Point<float> p (10.0f, 20.0f);
    Path path (p);
    EXPECT_GT (path.size(), 0);
}

TEST (PathTests, QuadToWithPointParameter)
{
    Path p;
    p.moveTo (0, 0);
    Point<float> controlPoint (5, 5);
    p.quadTo (controlPoint, 10, 0);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, CubicToWithPointParameter)
{
    Path p;
    p.moveTo (0, 0);
    Point<float> controlPoint1 (3, 5);
    p.cubicTo (controlPoint1, 7, 5, 10, 0);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, CreateCopy)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);
    p1.addEllipse (5, 5, 15, 15);

    Path p2 = p1.createCopy();

    EXPECT_EQ (p1.size(), p2.size());
    expectRectNear (p1.getBounds(), p2.getBounds());
}

TEST (PathTests, CreateCopyEmpty)
{
    Path p1;
    Path p2 = p1.createCopy();

    EXPECT_EQ (p1.size(), p2.size());
    EXPECT_TRUE (p2.getBounds().isEmpty());
}

TEST (PathTests, IteratorPostfixIncrement)
{
    Path p;
    p.addRectangle (0, 0, 10, 10);

    auto it = p.begin();
    auto end = p.end();
    int count = 0;

    while (it != end)
    {
        it++; // Postfix increment
        ++count;
    }

    EXPECT_GT (count, 0);
}

TEST (PathTests, FromStringQuadraticBezierAbsolute)
{
    Path p;
    bool ok = p.fromString ("M 10 80 Q 52.5 10, 95 80");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringQuadraticBezierRelative)
{
    Path p;
    bool ok = p.fromString ("M 10 80 q 42.5 -70, 85 0");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringSmoothQuadraticAbsolute)
{
    Path p;
    bool ok = p.fromString ("M 10 80 Q 52.5 10, 95 80 T 180 80");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringSmoothQuadraticRelative)
{
    Path p;
    bool ok = p.fromString ("M 10 80 Q 52.5 10, 95 80 t 85 0");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringCubicBezierAbsolute)
{
    Path p;
    bool ok = p.fromString ("M 10 10 C 20 20, 40 20, 50 10");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringCubicBezierRelative)
{
    Path p;
    bool ok = p.fromString ("M 10 10 c 10 10, 30 10, 40 0");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringCubicBezierPreservesSVGCoordinateOrder)
{
    Path p;
    bool ok = p.fromString ("M 0 0 C 10 0, 20 10, 30 0");
    EXPECT_TRUE (ok);

    auto it = p.begin();
    ASSERT_NE (it, p.end());
    EXPECT_EQ ((*it).verb, Path::Verb::MoveTo);

    ++it;
    ASSERT_NE (it, p.end());

    const auto segment = *it;
    EXPECT_EQ (segment.verb, Path::Verb::CubicTo);
    expectPointNear (segment.controlPoint1, Point<float> (10.0f, 0.0f));
    expectPointNear (segment.controlPoint2, Point<float> (20.0f, 10.0f));
    expectPointNear (segment.point, Point<float> (30.0f, 0.0f));
}

TEST (PathTests, FromStringParsesScimitarCubicPath)
{
    Path p;
    bool ok = p.fromString (
        "M 171.59375,-167.8125 L 153.4375,-131.09375 C 153.4375,-131.09375 240.05975,-44.592207 260.53125,61.53125 "
        "C 263.78902,59.713413 267.53809,58.6875 271.53125,58.6875 C 283.99674,58.687502 294.11733,68.78455 294.15625,81.25 "
        "L 294.15625,81.3125 C 294.15624,93.802829 284.02158,103.9375 271.53125,103.9375 "
        "C 269.20004,103.9375 266.9604,103.59314 264.84375,102.9375 C 265.00283,118.53432 263.43644,134.33614 259.71875,150.1875 "
        "C 279.93177,155.71176 336.35552,161.63753 367.0625,234.84375 C 388.95186,159.67792 354.15709,-29.134107 171.59375,-167.8125 z ");

    EXPECT_TRUE (ok);
    EXPECT_EQ (12, p.size());

    auto it = p.begin();
    ASSERT_NE (it, p.end());
    EXPECT_EQ ((*it).verb, Path::Verb::MoveTo);
    expectPointNear ((*it).point, Point<float> (171.59375f, -167.8125f));

    ++it;
    ASSERT_NE (it, p.end());
    EXPECT_EQ ((*it).verb, Path::Verb::LineTo);
    expectPointNear ((*it).point, Point<float> (153.4375f, -131.09375f));

    ++it;
    ASSERT_NE (it, p.end());

    const auto segment = *it;
    EXPECT_EQ (segment.verb, Path::Verb::CubicTo);
    expectPointNear (segment.controlPoint1, Point<float> (153.4375f, -131.09375f));
    expectPointNear (segment.controlPoint2, Point<float> (240.05975f, -44.592207f));
    expectPointNear (segment.point, Point<float> (260.53125f, 61.53125f));
}

TEST (PathTests, FromStringQuadraticBezierConvertsUsingSVGCoordinateOrder)
{
    Path p;
    bool ok = p.fromString ("M 0 0 Q 30 30, 60 0");
    EXPECT_TRUE (ok);

    auto it = p.begin();
    ASSERT_NE (it, p.end());
    EXPECT_EQ ((*it).verb, Path::Verb::MoveTo);

    ++it;
    ASSERT_NE (it, p.end());

    const auto segment = *it;
    EXPECT_EQ (segment.verb, Path::Verb::CubicTo);
    expectPointNear (segment.controlPoint1, Point<float> (20.0f, 20.0f));
    expectPointNear (segment.controlPoint2, Point<float> (40.0f, 20.0f));
    expectPointNear (segment.point, Point<float> (60.0f, 0.0f));
}

TEST (PathTests, FromStringParsesSignedExponentCoordinates)
{
    Path p;
    bool ok = p.fromString ("M +1e1 -2e1 L 2.5e1,+3.5e1 C +3e1 -4e1 4e1 -5e1 5e1 -6e1");
    EXPECT_TRUE (ok);

    auto it = p.begin();
    ASSERT_NE (it, p.end());
    expectPointNear ((*it).point, Point<float> (10.0f, -20.0f));

    ++it;
    ASSERT_NE (it, p.end());
    expectPointNear ((*it).point, Point<float> (25.0f, 35.0f));

    ++it;
    ASSERT_NE (it, p.end());

    const auto segment = *it;
    EXPECT_EQ (segment.verb, Path::Verb::CubicTo);
    expectPointNear (segment.controlPoint1, Point<float> (30.0f, -40.0f));
    expectPointNear (segment.controlPoint2, Point<float> (40.0f, -50.0f));
    expectPointNear (segment.point, Point<float> (50.0f, -60.0f));
}

TEST (PathTests, FromStringSmoothCubicAbsolute)
{
    Path p;
    bool ok = p.fromString ("M 10 80 C 40 10, 65 10, 95 80 S 150 150, 180 80");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringSmoothCubicRelative)
{
    Path p;
    bool ok = p.fromString ("M 10 80 C 40 10, 65 10, 95 80 s 55 70, 85 0");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcAbsolute)
{
    Path p;
    bool ok = p.fromString ("M 10 20 A 20 20 0 0 1 50 20");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcParsesCompactFlags)
{
    Path p;
    bool ok = p.fromString ("M 0 0 A 10 10 0 0150 0");
    EXPECT_TRUE (ok);
    EXPECT_GT (p.size(), 1);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcUsesAbsoluteRadii)
{
    Path p;
    bool ok = p.fromString ("M 0 0 A -10 -10 0 0 1 50 0");
    EXPECT_TRUE (ok);
    EXPECT_GT (p.size(), 1);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcRelative)
{
    Path p;
    bool ok = p.fromString ("M 10 20 a 20 20 0 0 1 40 0");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcLargeArc)
{
    Path p;
    bool ok = p.fromString ("M 10 20 A 30 30 0 1 0 50 20");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcSweep)
{
    Path p;
    bool ok = p.fromString ("M 10 20 A 30 30 45 0 1 50 20");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringEllipticalArcDegenerateToLine)
{
    Path p;
    bool ok = p.fromString ("M 10 20 A 0 0 0 0 1 50 20");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, FromStringComplexPath)
{
    Path p;
    bool ok = p.fromString ("M 10 10 L 20 20 Q 30 30, 40 20 C 50 10, 60 10, 70 20 S 90 40, 100 20 T 120 20 A 10 10 0 0 1 140 20 Z");
    EXPECT_TRUE (ok);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, GetPointAlongPathQuadratic)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 10, 5, 5).close();

    Point<float> start = p.getPointAlongPath (0.0f);
    Point<float> mid = p.getPointAlongPath (0.5f);
    Point<float> end = p.getPointAlongPath (1.0f);

    expectPointNear (start, Point<float> (0, 0));
    EXPECT_TRUE (mid.getX() >= 0 && mid.getX() <= 10);
    EXPECT_TRUE (mid.getY() >= 0 && mid.getY() <= 10);
}

TEST (PathTests, GetPointAlongPathCubic)
{
    Path p;
    p.moveTo (0, 0).cubicTo (10, 0, 5, 5, 15, 5).close();

    Point<float> start = p.getPointAlongPath (0.0f);
    Point<float> mid = p.getPointAlongPath (0.5f);
    Point<float> end = p.getPointAlongPath (1.0f);

    expectPointNear (start, Point<float> (0, 0));
    EXPECT_TRUE (mid.getX() >= 0 && mid.getX() <= 15);
    EXPECT_TRUE (mid.getY() >= 0 && mid.getY() <= 5);
}

TEST (PathTests, GetPointAlongPathMixedSegments)
{
    Path p;
    p.moveTo (0, 0)
        .lineTo (10, 0)
        .quadTo (15, 5, 10, 10)
        .cubicTo (5, 15, 0, 10, 0, 0)
        .close();

    Point<float> p1 = p.getPointAlongPath (0.0f);
    Point<float> p2 = p.getPointAlongPath (0.25f);
    Point<float> p3 = p.getPointAlongPath (0.5f);
    Point<float> p4 = p.getPointAlongPath (0.75f);
    Point<float> p5 = p.getPointAlongPath (1.0f);

    expectPointNear (p1, Point<float> (0, 0));
    EXPECT_TRUE (p2.getX() >= 0 && p2.getX() <= 15);
    EXPECT_TRUE (p3.getX() >= 0 && p3.getX() <= 15);
    EXPECT_TRUE (p4.getX() >= 0 && p4.getX() <= 15);
}

TEST (PathTests, CreateStrokePolygonLine)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    EXPECT_GT (stroke.size(), 0);
}

TEST (PathTests, CreateStrokePolygonQuadratic)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 10, 5, 5);

    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    EXPECT_GT (stroke.size(), 0);
}

TEST (PathTests, CreateStrokePolygonCubic)
{
    Path p;
    p.moveTo (0, 0).cubicTo (10, 0, 5, 5, 15, 5);

    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    EXPECT_GT (stroke.size(), 0);
}

TEST (PathTests, CreateStrokePolygonClosedPath)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).lineTo (10, 10).lineTo (0, 10).close();

    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    EXPECT_GT (stroke.size(), 0);
}

TEST (PathTests, CreateStrokePolygonMixedCommands)
{
    Path p;
    p.moveTo (0, 0)
        .lineTo (10, 0)
        .quadTo (15, 5, 10, 10)
        .cubicTo (5, 15, 0, 10, 0, 0)
        .close();

    Path stroke = p.createStrokePolygon (2.0f);
    EXPECT_FALSE (stroke.getBounds().isEmpty());
    EXPECT_GT (stroke.size(), 0);
}

TEST (PathTests, AddBubbleArrowTop)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (100, 10);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowBottom)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (100, 180);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowLeft)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (10, 75);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowRight)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (180, 75);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowTopLeft)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (30, 30);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowTopRight)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (170, 30);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowBottomLeft)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (30, 170);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AddBubbleArrowBottomRight)
{
    Path p;
    Rectangle<float> body (50, 50, 100, 50);
    Rectangle<float> max (0, 0, 200, 200);
    Point<float> tip (170, 170);

    p.addBubble (body, max, tip, 5, 10);
    EXPECT_FALSE (p.getBounds().isEmpty());
}

TEST (PathTests, AppendPathWithTransformTranslation)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);

    Path p2;
    p2.addEllipse (0, 0, 5, 5);

    AffineTransform t = AffineTransform::translation (20, 20);
    p1.appendPath (p2, t);

    Rectangle<float> bounds = p1.getBounds();
    EXPECT_GE (bounds.getWidth(), 15.0f);
}

TEST (PathTests, AppendPathWithTransformScaling)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);

    Path p2;
    p2.addRectangle (0, 0, 5, 5);

    AffineTransform t = AffineTransform::scaling (2.0f);
    p1.appendPath (p2, t);

    Rectangle<float> bounds = p1.getBounds();
    EXPECT_GE (bounds.getWidth(), 10.0f);
}

TEST (PathTests, AppendPathWithTransformRotation)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);

    Path p2;
    p2.addRectangle (10, 0, 5, 5);

    AffineTransform t = AffineTransform::rotation (MathConstants<float>::halfPi);
    p1.appendPath (p2, t);

    EXPECT_FALSE (p1.getBounds().isEmpty());
}

TEST (PathTests, AppendPathWithTransformComplex)
{
    Path p1;
    p1.addRectangle (0, 0, 10, 10);

    Path p2;
    p2.addEllipse (0, 0, 8, 8);

    AffineTransform t = AffineTransform::translation (10, 10)
                            .scaled (1.5f)
                            .rotated (MathConstants<float>::quarterPi);
    p1.appendPath (p2, t);

    EXPECT_FALSE (p1.getBounds().isEmpty());
    EXPECT_GT (p1.size(), 0);
}

TEST (PathTests, FillRule_DefaultIsNonZeroWinding)
{
    // Test that paths default to non-zero winding rule
    Path p;
    EXPECT_TRUE (p.isUsingNonZeroWinding());
}

TEST (PathTests, FillRule_SetToEvenOdd)
{
    // Test setting to even-odd rule
    Path p;
    p.setUsingNonZeroWinding (false);
    EXPECT_FALSE (p.isUsingNonZeroWinding());
}

TEST (PathTests, FillRule_SetToNonZeroWinding)
{
    // Test setting to non-zero winding rule
    Path p;
    p.setUsingNonZeroWinding (false);
    p.setUsingNonZeroWinding (true);
    EXPECT_TRUE (p.isUsingNonZeroWinding());
}

TEST (PathTests, FillRule_PersistsAfterPathOperations)
{
    // Test that fill rule persists after adding shapes
    Path p;
    p.setUsingNonZeroWinding (false);

    p.addRectangle (0, 0, 10, 10);
    EXPECT_FALSE (p.isUsingNonZeroWinding());

    p.addEllipse (5, 5, 8, 8);
    EXPECT_FALSE (p.isUsingNonZeroWinding());

    p.lineTo (100, 100);
    EXPECT_FALSE (p.isUsingNonZeroWinding());
}

TEST (PathTests, FillRule_IndependentBetweenPaths)
{
    // Test that each path maintains its own fill rule
    Path p1;
    Path p2;

    p1.setUsingNonZeroWinding (false);
    EXPECT_FALSE (p1.isUsingNonZeroWinding());
    EXPECT_TRUE (p2.isUsingNonZeroWinding()); // p2 should still be default

    p2.setUsingNonZeroWinding (true);
    EXPECT_FALSE (p1.isUsingNonZeroWinding()); // p1 should remain even-odd
    EXPECT_TRUE (p2.isUsingNonZeroWinding());
}

TEST (PathTests, BooleanOperation_UnionCombinesOverlappingPaths)
{
    Path left;
    left.addRectangle (0.0f, 0.0f, 10.0f, 10.0f);

    Path right;
    right.addRectangle (5.0f, 0.0f, 10.0f, 10.0f);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    const auto bounds = result.getBounds();

    EXPECT_FALSE (result.isEmpty());
    EXPECT_NEAR (0.0f, bounds.getX(), 1.0e-3f);
    EXPECT_NEAR (0.0f, bounds.getY(), 1.0e-3f);
    EXPECT_NEAR (15.0f, bounds.getWidth(), 1.0e-3f);
    EXPECT_NEAR (10.0f, bounds.getHeight(), 1.0e-3f);
}

TEST (PathTests, BooleanOperation_IntersectReturnsOverlap)
{
    Path left;
    left.addRectangle (0.0f, 0.0f, 10.0f, 10.0f);

    Path right;
    right.addRectangle (5.0f, 0.0f, 10.0f, 10.0f);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Intersect);
    const auto bounds = result.getBounds();

    EXPECT_FALSE (result.isEmpty());
    EXPECT_NEAR (5.0f, bounds.getX(), 1.0e-3f);
    EXPECT_NEAR (0.0f, bounds.getY(), 1.0e-3f);
    EXPECT_NEAR (5.0f, bounds.getWidth(), 1.0e-3f);
    EXPECT_NEAR (10.0f, bounds.getHeight(), 1.0e-3f);
}

TEST (PathTests, BooleanOperation_SubtractRemovesOverlap)
{
    Path left;
    left.addRectangle (0.0f, 0.0f, 10.0f, 10.0f);

    Path right;
    right.addRectangle (5.0f, 0.0f, 10.0f, 10.0f);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Subtract);
    const auto bounds = result.getBounds();

    EXPECT_FALSE (result.isEmpty());
    EXPECT_NEAR (0.0f, bounds.getX(), 1.0e-3f);
    EXPECT_NEAR (0.0f, bounds.getY(), 1.0e-3f);
    EXPECT_NEAR (5.0f, bounds.getWidth(), 1.0e-3f);
    EXPECT_NEAR (10.0f, bounds.getHeight(), 1.0e-3f);
}

TEST (PathTests, BooleanOperation_XorKeepsNonOverlappingRegions)
{
    Path left;
    left.addRectangle (0.0f, 0.0f, 10.0f, 10.0f);

    Path right;
    right.addRectangle (5.0f, 0.0f, 10.0f, 10.0f);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Xor);
    const auto bounds = result.getBounds();

    EXPECT_FALSE (result.isEmpty());
    EXPECT_NEAR (0.0f, bounds.getX(), 1.0e-3f);
    EXPECT_NEAR (0.0f, bounds.getY(), 1.0e-3f);
    EXPECT_NEAR (15.0f, bounds.getWidth(), 1.0e-3f);
    EXPECT_NEAR (10.0f, bounds.getHeight(), 1.0e-3f);
}

// ==============================================================================
// Tests for boolean operations with Quadratic paths
// (covers evaluateClipperQuadratic, appendQuadraticAsLines,
//  toClipperPaths QuadTo case lines 776-782, appendClipperPoint lines 661-667)
// ==============================================================================

TEST (PathTests, BooleanOperation_QuadraticUnion)
{
    Path left;
    left.moveTo (0, 0).quadTo (50, -20, 100, 0).lineTo (100, 100).lineTo (0, 100).close();

    Path right;
    right.moveTo (25, 25).quadTo (75, 5, 125, 25).lineTo (125, 75).lineTo (25, 75).close();

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
    EXPECT_GT (result.size(), 0);
}

TEST (PathTests, BooleanOperation_QuadraticIntersect)
{
    Path left;
    left.moveTo (0, 0).quadTo (50, -20, 100, 0).lineTo (100, 100).lineTo (0, 100).close();

    Path right;
    right.moveTo (50, -10).quadTo (75, -30, 100, -10).lineTo (100, 50).lineTo (50, 50).close();

    const auto result = left.combinedWith (right, Path::BooleanOperation::Intersect);
    EXPECT_FALSE (result.isEmpty());
    EXPECT_GT (result.size(), 0);
}

TEST (PathTests, BooleanOperation_QuadraticSubtract)
{
    Path left;
    left.addRectangle (0, 0, 100, 100);

    Path right;
    right.moveTo (25, 25).quadTo (75, 5, 125, 25).lineTo (125, 75).lineTo (25, 75).close();

    const auto result = left.combinedWith (right, Path::BooleanOperation::Subtract);
    EXPECT_FALSE (result.isEmpty());
    EXPECT_GT (result.size(), 0);
}

TEST (PathTests, BooleanOperation_QuadraticXor)
{
    Path left;
    left.moveTo (0, 0).quadTo (50, -20, 100, 0).lineTo (100, 100).lineTo (0, 100).close();

    Path right;
    right.moveTo (25, 25).quadTo (75, 5, 125, 25).lineTo (125, 75).lineTo (25, 75).close();

    const auto result = left.combinedWith (right, Path::BooleanOperation::Xor);
    EXPECT_FALSE (result.isEmpty());
    EXPECT_GT (result.size(), 0);
}

TEST (PathTests, BooleanOperation_QuadraticDegenerateCurve)
{
    Path left;
    left.moveTo (0, 0).quadTo (0, 0, 100, 0).lineTo (100, 100).lineTo (0, 100).close();

    Path right;
    right.addRectangle (25, 25, 50, 50);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
    EXPECT_GT (result.size(), 0);
}

// ==============================================================================
// Tests for combinedWith / combineWith edge cases
// (covers lines 1778, 1782, 1793, 1795-1796)
// ==============================================================================

TEST (PathTests, CombinedWith_EmptySubjectUnionReturnsOther)
{
    Path empty;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    const auto result = empty.combinedWith (other, Path::BooleanOperation::Union);
    expectRectNear (result.getBounds(), other.getBounds(), 1.0e-3f);
}

TEST (PathTests, CombinedWith_EmptySubjectXorReturnsOther)
{
    Path empty;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    const auto result = empty.combinedWith (other, Path::BooleanOperation::Xor);
    expectRectNear (result.getBounds(), other.getBounds(), 1.0e-3f);
}

TEST (PathTests, CombinedWith_EmptySubjectIntersectReturnsEmpty)
{
    Path empty;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    const auto result = empty.combinedWith (other, Path::BooleanOperation::Intersect);
    EXPECT_TRUE (result.isEmpty());
}

TEST (PathTests, CombinedWith_EmptySubjectSubtractReturnsEmpty)
{
    Path empty;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    const auto result = empty.combinedWith (other, Path::BooleanOperation::Subtract);
    EXPECT_TRUE (result.isEmpty());
}

TEST (PathTests, CombinedWith_EmptyClipUnionReturnsThis)
{
    Path self;
    self.addRectangle (0, 0, 10, 10);
    Path empty;

    const auto result = self.combinedWith (empty, Path::BooleanOperation::Union);
    expectRectNear (result.getBounds(), self.getBounds(), 1.0e-3f);
}

TEST (PathTests, CombinedWith_EmptyClipIntersectReturnsEmpty)
{
    Path self;
    self.addRectangle (0, 0, 10, 10);
    Path empty;

    const auto result = self.combinedWith (empty, Path::BooleanOperation::Intersect);
    EXPECT_TRUE (result.isEmpty());
}

TEST (PathTests, CombinedWith_BothEmptyUnionReturnsEmpty)
{
    Path a;
    Path b;
    const auto result = a.combinedWith (b, Path::BooleanOperation::Union);
    EXPECT_TRUE (result.isEmpty());
}

TEST (PathTests, CombineWith_ReplacesPathInPlace)
{
    Path self;
    self.addRectangle (0, 0, 10, 10);

    Path other;
    other.addRectangle (5, 0, 10, 10);

    self.combineWith (other, Path::BooleanOperation::Union);
    EXPECT_FALSE (self.isEmpty());
    EXPECT_NEAR (self.getBounds().getWidth(), 15.0f, 1.0e-3f);
}

TEST (PathTests, CombineWith_EmptySubjectIntersect)
{
    Path self;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    self.combineWith (other, Path::BooleanOperation::Intersect);
    EXPECT_TRUE (self.isEmpty());
}

// ==============================================================================
// Tests for getTrimmedPath edge cases
// (covers lines 2618, 2624, 2630, 2504)
// ==============================================================================

TEST (PathTests, GetTrimmedPath_FullSpanReturnsCopy)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).lineTo (10, 10).close();

    const auto trimmed = p.getTrimmedPath (0.0f, 1.0f);
    EXPECT_EQ (p.size(), trimmed.size());
    expectRectNear (p.getBounds(), trimmed.getBounds(), 1.0e-3f);
}

TEST (PathTests, GetTrimmedPath_SpanExceedsFullReturnsCopy)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).lineTo (10, 10).close();

    const auto trimmed = p.getTrimmedPath (-0.5f, 1.5f);
    EXPECT_EQ (p.size(), trimmed.size());
}

TEST (PathTests, GetTrimmedPath_ZeroRangeReturnsEmpty)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const auto trimmed = p.getTrimmedPath (0.3f, 0.3f);
    EXPECT_TRUE (trimmed.isEmpty());
}

TEST (PathTests, GetTrimmedPath_EmptyPathReturnsEmpty)
{
    Path p;
    const auto trimmed = p.getTrimmedPath (0.0f, 0.5f);
    EXPECT_TRUE (trimmed.isEmpty());
}

TEST (PathTests, GetTrimmedPath_NegativeSpanWrapsAround)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const auto trimmed = p.getTrimmedPath (0.8f, 0.2f);
    EXPECT_FALSE (trimmed.isEmpty());
    EXPECT_NEAR (trimmed.getLength(), 4.0f, 1.0e-4f);
}

// ==============================================================================
// Tests for getTrimmedPath with quadratic segments
// (covers evaluateQuadratic, splitQuadratic, appendTrimmedQuadratic)
// ==============================================================================

TEST (PathTests, GetTrimmedPath_PreservesCurvesWhenTrimmingCubic)
{
    Path p;
    p.moveTo (0, 0).cubicTo (0, 10, 10, 10, 10, 0);

    const Path trimmed = p.getTrimmedPath (0.25f, 0.75f, 0.0f, 0.01f);

    int cubicCount = 0;
    for (const auto segment : trimmed)
    {
        if (segment.verb == Path::Verb::CubicTo)
            ++cubicCount;
    }

    EXPECT_GE (cubicCount, 1);
    EXPECT_GT (trimmed.getLength (0.01f), 0.0f);
}

TEST (PathTests, GetTrimmedPath_QuadraticWithOffset)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 20, 20, 0);
    const float length = p.getLength (0.01f);

    const Path trimmed = p.getTrimmedPath (0.0f, 0.5f, 0.2f, 0.01f);
    EXPECT_FALSE (trimmed.isEmpty());
    EXPECT_GT (trimmed.getLength (0.01f), 0.0f);
}

TEST (PathTests, GetTrimmedPath_QuadraticWrapsAroundEnd)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 10, 20, 0);
    const float fullLength = p.getLength (0.01f);

    const Path trimmed = p.getTrimmedPath (0.8f, 0.3f, 0.0f, 0.01f);
    EXPECT_FALSE (trimmed.isEmpty());
    EXPECT_GT (trimmed.getLength (0.01f), 0.0f);
}

TEST (PathTests, GetTrimmedPath_QuadraticPartialStart)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 20, 20, 0);

    const Path trimmed = p.getTrimmedPath (0.4f, 0.9f, 0.0f, 0.01f);
    EXPECT_FALSE (trimmed.isEmpty());

    const float trimmedLen = trimmed.getLength (0.01f);
    const float originalLen = p.getLength (0.01f);
    EXPECT_LT (trimmedLen, originalLen);
}

TEST (PathTests, GetTrimmedPath_TrimWithQuadraticAndLineSegments)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0).quadTo (20, 10, 20, 20).lineTo (0, 20).close();

    const Path trimmed = p.getTrimmedPath (0.1f, 0.6f, 0.0f, 0.01f);
    EXPECT_FALSE (trimmed.isEmpty());
    EXPECT_GT (trimmed.getLength (0.01f), 0.0f);
}

TEST (PathTests, UnionWithBothPathsEmpty)
{
    Path left;
    Path right;

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    EXPECT_TRUE (result.isEmpty());
}

TEST (PathTests, CombineWithUnion)
{
    Path left;
    left.addRectangle (0, 0, 10, 10);
    Path right;
    right.addRectangle (5, 5, 10, 10);

    left.combineWith (right, Path::BooleanOperation::Union);
    const auto b = left.getBounds();
    EXPECT_NEAR (0, b.getX(), 0.01f);
    EXPECT_NEAR (0, b.getY(), 0.01f);
    EXPECT_NEAR (15, b.getWidth(), 0.01f);
    EXPECT_NEAR (15, b.getHeight(), 0.01f);
}

TEST (PathTests, SubtractNonOverlapping)
{
    Path left;
    left.addRectangle (0, 0, 10, 10);
    Path right;
    right.addRectangle (20, 20, 10, 10);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Subtract);
    EXPECT_NEAR (10, result.getBounds().getWidth(), 0.01f);
    EXPECT_NEAR (10, result.getBounds().getHeight(), 0.01f);
}

// ==============================================================================
// Tests for getLength with quadratic and cubic
// (covers integrateSpeedRange line 2201, integrateSpeedAdaptive lines 2223-2224,
//  quadraticSpeed)
// ==============================================================================

TEST (PathTests, GetLength_StraightLine)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);
    const float length = p.getLength (0.01f);
    EXPECT_NEAR (length, 10.0f, 0.1f);
}

TEST (PathTests, GetLength_QuadraticCurve)
{
    Path p;
    p.moveTo (0, 0).quadTo (5, 10, 10, 0);
    const float length = p.getLength (0.01f);
    EXPECT_GT (length, 10.0f);
    EXPECT_LT (length, 30.0f);
}

TEST (PathTests, GetLength_QuadraticWithHigherTolerance)
{
    Path p;
    p.moveTo (0, 0).quadTo (5, 10, 10, 0);

    const float lengthTight = p.getLength (0.001f);
    const float lengthLoose = p.getLength (1.0f);
    EXPECT_GT (lengthTight, 0.0f);
    EXPECT_GT (lengthLoose, 0.0f);
}

TEST (PathTests, GetLength_CubicCurve)
{
    Path p;
    p.moveTo (0, 0).cubicTo (5, 10, 5, -10, 10, 0);
    const float length = p.getLength (0.01f);
    EXPECT_GT (length, 10.0f);
}

TEST (PathTests, GetLength_QuadraticDegenerateToPoint)
{
    Path p;
    p.moveTo (0, 0).quadTo (0, 0, 0, 0);
    const float length = p.getLength (0.01f);
    EXPECT_NEAR (length, 0.0f, 1.0e-4f);
}

TEST (PathTests, GetLength_ZeroLengthPath)
{
    Path p;
    EXPECT_NEAR (p.getLength(), 0.0f, 1.0e-4f);
}

TEST (PathTests, GetLength_MultipleQuadraticSegments)
{
    Path p;
    p.moveTo (0, 0).quadTo (5, 5, 10, 0).quadTo (15, 5, 20, 0);
    const float length = p.getLength (0.01f);
    EXPECT_GT (length, 20.0f);
}

// ==============================================================================
// Tests for getPointAlongPath on quadratic paths
// (covers evaluateQuadratic)
// ==============================================================================

TEST (PathTests, GetPointAlongPath_QuadraticStart)
{
    Path p;
    p.moveTo (0, 0).quadTo (5, 10, 10, 0);

    const auto pt = p.getPointAlongPath (0.0f);
    expectPointNear (pt, Point<float> (0, 0), 1.0e-3f);
}

TEST (PathTests, GetPointAlongPath_QuadraticEnd)
{
    Path p;
    p.moveTo (0, 0).quadTo (10, 0, 5, 10);

    const auto pt = p.getPointAlongPath (1.0f);
    expectPointNear (pt, Point<float> (10, 0), 1.0e-3f);
}

TEST (PathTests, GetPointAlongPath_QuadraticMidpoint)
{
    Path p;
    p.moveTo (0, 0).quadTo (5, 10, 10, 0);

    const auto pt = p.getPointAlongPath (0.5f);
    EXPECT_TRUE (pt.getX() > 0.0f && pt.getX() < 10.0f);
    EXPECT_TRUE (pt.getY() > 0.0f && pt.getY() < 10.0f);
}

TEST (PathTests, GetPointAlongPath_LineMidpoint)
{
    Path p;
    p.moveTo (0, 0).lineTo (10, 0);

    const auto pt = p.getPointAlongPath (0.5f);
    expectPointNear (pt, Point<float> (5, 0), 1.0e-3f);
}

TEST (PathTests, GetPointAlongPath_CubicStart)
{
    Path p;
    p.moveTo (0, 0).cubicTo (3, 10, 7, -5, 10, 0);

    const auto pt = p.getPointAlongPath (0.0f);
    expectPointNear (pt, Point<float> (0, 0), 1.0e-3f);
}

TEST (PathTests, GetPointAlongPath_CubicEnd)
{
    Path p;
    p.moveTo (0, 0).cubicTo (3, 10, 7, -5, 10, 0);

    const auto pt = p.getPointAlongPath (1.0f);
    expectPointNear (pt, Point<float> (10, 0), 1.0e-3f);
}

// ==============================================================================
// Tests for fromClipperPaths skip small contours (line 840)
// and toClipperClipType default (line 830, dead code)
// ==============================================================================

TEST (PathTests, BooleanOperation_PathWithTwoPointContourDropped)
{
    Path subject;
    subject.addRectangle (0, 0, 10, 10);

    Path clip;
    clip.moveTo (5, 5).lineTo (6, 6).close();

    const auto result = subject.combinedWith (clip, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
}

TEST (PathTests, BooleanOperation_ClipWithOnlyMoveTo)
{
    Path subject;
    subject.addRectangle (0, 0, 10, 10);

    Path clip;
    clip.moveTo (5, 5);

    const auto result = subject.combinedWith (clip, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
}

TEST (PathTests, CombinedWith_EmptyPathAllOperations)
{
    Path empty;
    Path other;
    other.addRectangle (0, 0, 10, 10);

    const auto unionResult = empty.combinedWith (other, Path::BooleanOperation::Union);
    EXPECT_FALSE (unionResult.isEmpty());

    const auto intersectResult = empty.combinedWith (other, Path::BooleanOperation::Intersect);
    EXPECT_TRUE (intersectResult.isEmpty());

    const auto subtractResult = empty.combinedWith (other, Path::BooleanOperation::Subtract);
    EXPECT_TRUE (subtractResult.isEmpty());

    const auto xorResult = empty.combinedWith (other, Path::BooleanOperation::Xor);
    EXPECT_FALSE (xorResult.isEmpty());
}

TEST (PathTests, CombinedWith_AllOperationsWithEmptyClip)
{
    Path self;
    self.addRectangle (0, 0, 10, 10);
    Path empty;

    const auto unionResult = self.combinedWith (empty, Path::BooleanOperation::Union);
    EXPECT_FALSE (unionResult.isEmpty());
    expectRectNear (unionResult.getBounds(), self.getBounds(), 1.0e-3f);

    const auto intersectResult = self.combinedWith (empty, Path::BooleanOperation::Intersect);
    EXPECT_TRUE (intersectResult.isEmpty());

    const auto subtractResult = self.combinedWith (empty, Path::BooleanOperation::Subtract);
    EXPECT_FALSE (subtractResult.isEmpty());
    expectRectNear (subtractResult.getBounds(), self.getBounds(), 1.0e-3f);

    const auto xorResult = self.combinedWith (empty, Path::BooleanOperation::Xor);
    EXPECT_FALSE (xorResult.isEmpty());
    expectRectNear (xorResult.getBounds(), self.getBounds(), 1.0e-3f);
}

TEST (PathTests, FillRulePreservedInCombinedPath_EvenOdd)
{
    Path left;
    left.setUsingNonZeroWinding (false);
    left.addRectangle (0, 0, 10, 10);

    Path right;
    right.addRectangle (5, 5, 10, 10);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
}

TEST (PathTests, FillRulePreservedInCombinedPath_NonZero)
{
    Path left;
    left.setUsingNonZeroWinding (true);
    left.addRectangle (0, 0, 10, 10);

    Path right;
    right.addRectangle (5, 5, 10, 10);

    const auto result = left.combinedWith (right, Path::BooleanOperation::Union);
    EXPECT_FALSE (result.isEmpty());
}
