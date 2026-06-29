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

class AnimationPropertyTests : public ::testing::Test
{
};

TEST_F (AnimationPropertyTests, StaticFloatReturnsConstantValue)
{
    auto prop = FloatProperty::staticValue (42.0f);
    EXPECT_TRUE (prop.isStatic());
    EXPECT_FALSE (prop.isAnimated());
    EXPECT_NEAR (prop.getValueAt (0.0f), 42.0f, 0.001f);
    EXPECT_NEAR (prop.getValueAt (100.0f), 42.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, AnimatedFloatInterpolatesCorrectly)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 0.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 100.0f, AnimationEasing::linear())
                    .build();

    EXPECT_TRUE (prop.isAnimated());
    EXPECT_NEAR (prop.getValueAt (0.0f), 0.0f, 0.1f);
    EXPECT_NEAR (prop.getValueAt (5.0f), 50.0f, 1.0f);
    EXPECT_NEAR (prop.getValueAt (10.0f), 100.0f, 0.1f);
}

TEST_F (AnimationPropertyTests, ClampsBelowFirstKeyframe)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (5.0f, 10.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 20.0f, AnimationEasing::linear())
                    .build();

    EXPECT_NEAR (prop.getValueAt (0.0f), 10.0f, 0.001f);
    EXPECT_NEAR (prop.getValueAt (3.0f), 10.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, ClampsAboveLastKeyframe)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 10.0f, AnimationEasing::linear())
                    .keyframe (5.0f, 20.0f, AnimationEasing::linear())
                    .build();

    EXPECT_NEAR (prop.getValueAt (10.0f), 20.0f, 0.001f);
    EXPECT_NEAR (prop.getValueAt (100.0f), 20.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, SingleKeyframeReturnsConstant)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 7.0f, AnimationEasing::linear())
                    .build();

    EXPECT_NEAR (prop.getValueAt (0.0f), 7.0f, 0.001f);
    EXPECT_NEAR (prop.getValueAt (5.0f), 7.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, ColorPropertyInterpolatesPerChannel)
{
    auto prop = ColorProperty::Builder {}
                    .keyframe (0.0f, Color::fromRGBA (0, 0, 0, 255), AnimationEasing::linear())
                    .keyframe (10.0f, Color::fromRGBA (255, 255, 255, 255), AnimationEasing::linear())
                    .build();

    const Color mid = prop.getValueAt (5.0f);
    EXPECT_NEAR (mid.getFloatRed(), 0.5f, 0.05f);
    EXPECT_NEAR (mid.getFloatGreen(), 0.5f, 0.05f);
    EXPECT_NEAR (mid.getFloatBlue(), 0.5f, 0.05f);
}

TEST_F (AnimationPropertyTests, Vec2PropertyInterpolatesBothAxes)
{
    auto prop = Vec2Property::Builder {}
                    .keyframe (0.0f, Point<float> (0.0f, 0.0f), AnimationEasing::linear())
                    .keyframe (10.0f, Point<float> (100.0f, 200.0f), AnimationEasing::linear())
                    .build();

    const auto mid = prop.getValueAt (5.0f);
    EXPECT_NEAR (mid.getX(), 50.0f, 1.0f);
    EXPECT_NEAR (mid.getY(), 100.0f, 1.0f);
}

TEST_F (AnimationPropertyTests, HoldEasingSnapsToStartValue)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 0.0f, AnimationEasing::hold())
                    .keyframe (10.0f, 100.0f, AnimationEasing::linear())
                    .build();

    // With hold easing, value should not change until the next keyframe
    EXPECT_NEAR (prop.getValueAt (4.99f), 0.0f, 1.0f);
}

TEST_F (AnimationPropertyTests, WithStaticValueUpdatesValue)
{
    auto prop = FloatProperty::staticValue (1.0f);
    prop = prop.withStaticValue (42.0f);
    EXPECT_NEAR (prop.getValueAt (0.0f), 42.0f, 0.001f);
}
