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

#include <yup_animation/yup_animation.h>

using namespace yup;

class AnimationTransformTests : public ::testing::Test
{
};

TEST_F (AnimationTransformTests, IdentityTransformIsStatic)
{
    AnimationTransform t;
    EXPECT_TRUE (t.isStatic());
}

TEST_F (AnimationTransformTests, OpacityAtReturnsNormalised)
{
    AnimationTransform t;
    t.opacity = FloatProperty::staticValue (100.0f);
    EXPECT_NEAR (t.opacityAt (0.0f), 1.0f, 0.001f);

    t.opacity = FloatProperty::staticValue (50.0f);
    EXPECT_NEAR (t.opacityAt (0.0f), 0.5f, 0.001f);

    t.opacity = FloatProperty::staticValue (0.0f);
    EXPECT_NEAR (t.opacityAt (0.0f), 0.0f, 0.001f);
}

TEST_F (AnimationTransformTests, TranslationProducesCorrectTransform)
{
    AnimationTransform t;
    t.position = Vec2Property::staticValue ({ 100.0f, 200.0f });

    const AffineTransform xf = t.toAffineTransform (0.0f);

    // A point at origin should map to (100, 200) after translation
    Point<float> origin { 0.0f, 0.0f };
    const auto transformed = origin.transformed (xf);
    EXPECT_NEAR (transformed.getX(), 100.0f, 0.1f);
    EXPECT_NEAR (transformed.getY(), 200.0f, 0.1f);
}

TEST_F (AnimationTransformTests, ScaleProducesCorrectTransform)
{
    AnimationTransform t;
    // Lottie scale is percentage: 200% = 2x
    t.scale = SizeProperty::staticValue ({ 200.0f, 200.0f });

    const AffineTransform xf = t.toAffineTransform (0.0f);

    Point<float> p { 1.0f, 1.0f };
    const auto transformed = p.transformed (xf);
    EXPECT_NEAR (transformed.getX(), 2.0f, 0.1f);
    EXPECT_NEAR (transformed.getY(), 2.0f, 0.1f);
}

TEST_F (AnimationTransformTests, SkewAxisUsesLottieBasis)
{
    AnimationTransform t;
    t.skew = FloatProperty::staticValue (45.0f);
    t.skewAxis = FloatProperty::staticValue (0.0f);

    const AffineTransform xf = t.toAffineTransform (0.0f);

    const auto xAxis = Point<float> { 1.0f, 0.0f }.transformed (xf);
    const auto yAxis = Point<float> { 0.0f, 1.0f }.transformed (xf);

    EXPECT_NEAR (xAxis.getX(), 1.0f, 0.001f);
    EXPECT_NEAR (xAxis.getY(), 0.0f, 0.001f);
    EXPECT_NEAR (yAxis.getX(), -1.0f, 0.001f);
    EXPECT_NEAR (yAxis.getY(), 1.0f, 0.001f);
}

TEST_F (AnimationTransformTests, SkewAxisRotatesSkewBasis)
{
    AnimationTransform t;
    t.skew = FloatProperty::staticValue (45.0f);
    t.skewAxis = FloatProperty::staticValue (90.0f);

    const AffineTransform xf = t.toAffineTransform (0.0f);

    const auto xAxis = Point<float> { 1.0f, 0.0f }.transformed (xf);
    const auto yAxis = Point<float> { 0.0f, 1.0f }.transformed (xf);

    EXPECT_NEAR (xAxis.getX(), 1.0f, 0.001f);
    EXPECT_NEAR (xAxis.getY(), 1.0f, 0.001f);
    EXPECT_NEAR (yAxis.getX(), 0.0f, 0.001f);
    EXPECT_NEAR (yAxis.getY(), 1.0f, 0.001f);
}

TEST_F (AnimationTransformTests, AnimatedPositionChangesOverTime)
{
    AnimationTransform t;
    t.position = Vec2Property::Builder {}
                     .keyframe (0.0f, Point<float> (0.0f, 0.0f), AnimationEasing::linear())
                     .keyframe (10.0f, Point<float> (100.0f, 0.0f), AnimationEasing::linear())
                     .build();

    EXPECT_FALSE (t.isStatic());

    const AffineTransform xf0 = t.toAffineTransform (0.0f);
    const AffineTransform xf10 = t.toAffineTransform (10.0f);

    Point<float> origin { 0.0f, 0.0f };
    EXPECT_NEAR (origin.transformed (xf0).getX(), 0.0f, 0.1f);
    EXPECT_NEAR (origin.transformed (xf10).getX(), 100.0f, 0.1f);
}

TEST_F (AnimationTransformTests, SpatialPositionUsesExplicitEndValue)
{
    AnimationTransform t;
    t.spatialKeyframes.push_back ({ 0.0f,
                                    Point<float> (0.0f, 0.0f),
                                    Point<float> (20.0f, 0.0f),
                                    Point<float> {},
                                    Point<float> {},
                                    AnimationEasing::linear() });
    t.spatialKeyframes.push_back ({ 10.0f,
                                    Point<float> (100.0f, 0.0f),
                                    std::nullopt,
                                    Point<float> {},
                                    Point<float> {},
                                    AnimationEasing::linear() });

    EXPECT_NEAR (t.positionAt (5.0f).getX(), 10.0f, 0.001f);
    EXPECT_NEAR (t.positionAt (10.0f).getX(), 100.0f, 0.001f);
}

// =============================================================================
// Rotation
// =============================================================================

TEST_F (AnimationTransformTests, RotationProducesCorrectTransform)
{
    AnimationTransform t;
    // 90-degree rotation (Lottie rotation is in degrees)
    t.rotation = FloatProperty::staticValue (90.0f);

    const AffineTransform xf = t.toAffineTransform (0.0f);

    // Rotating (1, 0) by 90° should yield approximately (0, 1)
    const Point<float> px { 1.0f, 0.0f };
    const auto rotated = px.transformed (xf);
    EXPECT_NEAR (rotated.getX(), 0.0f, 0.01f);
    EXPECT_NEAR (rotated.getY(), 1.0f, 0.01f);
}

TEST_F (AnimationTransformTests, ZeroRotationIsIdentity)
{
    AnimationTransform t;
    t.rotation = FloatProperty::staticValue (0.0f);
    t.scale = SizeProperty::staticValue ({ 100.0f, 100.0f });

    const AffineTransform xf = t.toAffineTransform (0.0f);
    EXPECT_TRUE (xf.isIdentity());
}

// =============================================================================
// Anchor point
// =============================================================================

TEST_F (AnimationTransformTests, AnchorShiftsOriginBeforeScale)
{
    AnimationTransform t;
    // Anchor at (50, 50) — origin moves to -50,-50 first, then identity scale
    t.anchor = Vec2Property::staticValue ({ 50.0f, 50.0f });
    t.position = Vec2Property::staticValue ({ 50.0f, 50.0f });
    t.scale = SizeProperty::staticValue ({ 100.0f, 100.0f });

    const AffineTransform xf = t.toAffineTransform (0.0f);

    // With anchor=(50,50) and position=(50,50) the transform is: translate(-50,-50) then translate(50,50) = identity
    const Point<float> origin { 50.0f, 50.0f };
    const auto result = origin.transformed (xf);
    EXPECT_NEAR (result.getX(), 50.0f, 0.1f);
    EXPECT_NEAR (result.getY(), 50.0f, 0.1f);
}

// =============================================================================
// Separate position X/Y
// =============================================================================

TEST_F (AnimationTransformTests, SeparatePositionXYCombinesAxes)
{
    AnimationTransform t;
    t.separatePosition = true;
    t.positionX = FloatProperty::staticValue (30.0f);
    t.positionY = FloatProperty::staticValue (40.0f);
    t.scale = SizeProperty::staticValue ({ 100.0f, 100.0f });

    const auto pos = t.positionAt (0.0f);
    EXPECT_NEAR (pos.getX(), 30.0f, 0.001f);
    EXPECT_NEAR (pos.getY(), 40.0f, 0.001f);
}

// =============================================================================
// Opacity clamping
// =============================================================================

TEST_F (AnimationTransformTests, OpacityAtClampsAbove100)
{
    AnimationTransform t;
    t.opacity = FloatProperty::staticValue (150.0f);
    EXPECT_NEAR (t.opacityAt (0.0f), 1.0f, 0.001f);
}

TEST_F (AnimationTransformTests, OpacityAtClampsBelow0)
{
    AnimationTransform t;
    t.opacity = FloatProperty::staticValue (-10.0f);
    EXPECT_NEAR (t.opacityAt (0.0f), 0.0f, 0.001f);
}

// =============================================================================
// isStatic with animated sub-properties
// =============================================================================

TEST_F (AnimationTransformTests, AnimatedScaleIsNotStatic)
{
    AnimationTransform t;
    t.scale = SizeProperty::Builder {}
                  .keyframe (0.0f, Size<float> (100.0f, 100.0f), AnimationEasing::linear())
                  .keyframe (10.0f, Size<float> (200.0f, 200.0f), AnimationEasing::linear())
                  .build();
    EXPECT_FALSE (t.isStatic());
}

TEST_F (AnimationTransformTests, AnimatedOpacityIsNotStatic)
{
    AnimationTransform t;
    t.opacity = FloatProperty::Builder {}
                    .keyframe (0.0f, 100.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 0.0f, AnimationEasing::linear())
                    .build();
    EXPECT_FALSE (t.isStatic());
}
