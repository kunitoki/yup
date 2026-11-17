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

#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class PathMarkersTests : public ::testing::Test
{
protected:
    static constexpr float tolerance = 1.0e-4f;

    static void expectPlacement (const PathMarkers::Placement& placement, Point<float> position, float angle)
    {
        EXPECT_NEAR (placement.position.getX(), position.getX(), tolerance);
        EXPECT_NEAR (placement.position.getY(), position.getY(), tolerance);
        EXPECT_NEAR (placement.angle, angle, tolerance);
    }
};

//==============================================================================
TEST_F (PathMarkersTests, FindsTheStartTheMidsAndTheEndOfAnOpenPath)
{
    Path path;
    path.moveTo (0.0f, 0.0f).lineTo (10.0f, 0.0f).lineTo (10.0f, 10.0f);

    const PathMarkers markers (path);

    ASSERT_EQ (markers.getStarts().size(), 1u);
    ASSERT_EQ (markers.getMids().size(), 1u);
    ASSERT_EQ (markers.getEnds().size(), 1u);

    expectPlacement (markers.getStarts()[0], { 0.0f, 0.0f }, 0.0f);
    expectPlacement (markers.getMids()[0], { 10.0f, 0.0f }, 0.0f);
    expectPlacement (markers.getEnds()[0], { 10.0f, 10.0f }, MathConstants<float>::halfPi);
}

TEST_F (PathMarkersTests, FollowsTheControlPointsOfCurves)
{
    Path path;
    path.moveTo (0.0f, 0.0f).cubicTo (0.0f, 10.0f, 10.0f, 10.0f, 20.0f, 10.0f);

    const PathMarkers markers (path);

    ASSERT_EQ (markers.getStarts().size(), 1u);
    ASSERT_EQ (markers.getEnds().size(), 1u);

    expectPlacement (markers.getStarts()[0], { 0.0f, 0.0f }, MathConstants<float>::halfPi);
    expectPlacement (markers.getEnds()[0], { 20.0f, 10.0f }, 0.0f);
}

TEST_F (PathMarkersTests, HasNoEndWhenThePathEndsByClosing)
{
    Path path;
    path.moveTo (0.0f, 0.0f).lineTo (10.0f, 0.0f).lineTo (10.0f, 10.0f).close();

    const PathMarkers markers (path);

    EXPECT_EQ (markers.getStarts().size(), 1u);
    EXPECT_EQ (markers.getMids().size(), 2u);
    EXPECT_TRUE (markers.getEnds().empty());
    EXPECT_TRUE (PathMarkers (Path()).getStarts().empty());
}

TEST_F (PathMarkersTests, TransformPlacesTheReferencePointOnTheVertex)
{
    const PathMarkers::Placement placement { { 10.0f, 10.0f }, MathConstants<float>::halfPi };

    PathMarkers::Shape shape;
    shape.reference = { 1.0f, 1.0f };

    // Scaled by the stroke width and turned to the direction of the path
    const auto transform = PathMarkers::getTransform (placement, shape, 2.0f);
    const auto reference = Point<float> (1.0f, 1.0f).transformed (transform);
    const auto ahead = Point<float> (2.0f, 1.0f).transformed (transform);

    EXPECT_NEAR (reference.getX(), 10.0f, tolerance);
    EXPECT_NEAR (reference.getY(), 10.0f, tolerance);
    EXPECT_NEAR (ahead.getX(), 10.0f, tolerance);
    EXPECT_NEAR (ahead.getY(), 12.0f, tolerance);
}

TEST_F (PathMarkersTests, TransformScalesTheViewBoxAndKeepsAFixedAngle)
{
    const PathMarkers::Placement placement { { 5.0f, 5.0f }, MathConstants<float>::halfPi };

    PathMarkers::Shape shape;
    shape.size = { 6.0f, 6.0f };
    shape.viewBox = Rectangle<float> (0.0f, 0.0f, 12.0f, 24.0f);
    shape.angle = 0.0f;
    shape.scalesWithStrokeWidth = false;

    const auto ahead = Point<float> (4.0f, 0.0f).transformed (PathMarkers::getTransform (placement, shape, 10.0f));

    EXPECT_NEAR (ahead.getX(), 6.0f, tolerance);
    EXPECT_NEAR (ahead.getY(), 5.0f, tolerance);
}
