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
    EXPECT_NEAR (yAxis.getX(), 1.0f, 0.001f);
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
