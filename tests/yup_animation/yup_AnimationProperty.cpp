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

TEST_F (AnimationPropertyTests, ExplicitEndValueControlsCurrentInterval)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 0.0f, 20.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 100.0f, AnimationEasing::linear())
                    .build();

    EXPECT_NEAR (prop.getValueAt (5.0f), 10.0f, 0.001f);
    EXPECT_NEAR (prop.getValueAt (10.0f), 100.0f, 0.001f);
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
    EXPECT_NEAR (mid.getRedFloat(), 0.5f, 0.05f);
    EXPECT_NEAR (mid.getGreenFloat(), 0.5f, 0.05f);
    EXPECT_NEAR (mid.getBlueFloat(), 0.5f, 0.05f);
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

// =============================================================================
// SizeProperty
// =============================================================================

TEST_F (AnimationPropertyTests, StaticSizePropertyReturnsConstant)
{
    auto prop = SizeProperty::staticValue ({ 120.0f, 80.0f });
    EXPECT_TRUE (prop.isStatic());
    const auto v = prop.getValueAt (0.0f);
    EXPECT_NEAR (v.getWidth(), 120.0f, 0.001f);
    EXPECT_NEAR (v.getHeight(), 80.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, AnimatedSizePropertyInterpolatesBothDimensions)
{
    auto prop = SizeProperty::Builder {}
                    .keyframe (0.0f, Size<float> (0.0f, 0.0f), AnimationEasing::linear())
                    .keyframe (10.0f, Size<float> (100.0f, 200.0f), AnimationEasing::linear())
                    .build();

    const auto mid = prop.getValueAt (5.0f);
    EXPECT_NEAR (mid.getWidth(), 50.0f, 1.0f);
    EXPECT_NEAR (mid.getHeight(), 100.0f, 1.0f);
}

// =============================================================================
// getStaticValue / getKeyframes
// =============================================================================

TEST_F (AnimationPropertyTests, GetStaticValueReturnsStoredValue)
{
    auto prop = FloatProperty::staticValue (77.0f);
    EXPECT_NEAR (prop.getStaticValue(), 77.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, GetKeyframesReturnsKeyframeList)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 0.0f, AnimationEasing::linear())
                    .keyframe (5.0f, 50.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 100.0f, AnimationEasing::linear())
                    .build();

    EXPECT_TRUE (prop.isAnimated());
    EXPECT_EQ (prop.getKeyframes().size(), 3u);
}

// =============================================================================
// Multiple keyframes — binary search path
// =============================================================================

TEST_F (AnimationPropertyTests, MultipleKeyframesInterpolatesCorrectInterval)
{
    auto prop = FloatProperty::Builder {}
                    .keyframe (0.0f, 0.0f, AnimationEasing::linear())
                    .keyframe (10.0f, 10.0f, AnimationEasing::linear())
                    .keyframe (20.0f, 30.0f, AnimationEasing::linear())
                    .keyframe (30.0f, 60.0f, AnimationEasing::linear())
                    .build();

    // Between 10 and 20 (linear: 10 + 50% * (30 - 10) = 20)
    EXPECT_NEAR (prop.getValueAt (15.0f), 20.0f, 0.5f);
    // Between 20 and 30 (linear: 30 + 50% * (60 - 30) = 45)
    EXPECT_NEAR (prop.getValueAt (25.0f), 45.0f, 0.5f);
}

// =============================================================================
// Default-constructed property
// =============================================================================

TEST_F (AnimationPropertyTests, DefaultConstructedFloatIsStaticZero)
{
    FloatProperty prop;
    EXPECT_TRUE (prop.isStatic());
    EXPECT_NEAR (prop.getValueAt (0.0f), 0.0f, 0.001f);
}

TEST_F (AnimationPropertyTests, DefaultConstructedColorPropertyIsStaticBlack)
{
    ColorProperty prop;
    EXPECT_TRUE (prop.isStatic());
    const Color c = prop.getValueAt (0.0f);
    EXPECT_EQ (c.getRed(), 0);
    EXPECT_EQ (c.getGreen(), 0);
    EXPECT_EQ (c.getBlue(), 0);
}

// =============================================================================
// AnimatedProperty empty keyframes
// =============================================================================

TEST_F (AnimationPropertyTests, EmptyKeyframeListReturnsDefaultValue)
{
    FloatProperty prop (FloatProperty::KeyframeList {});
    EXPECT_TRUE (prop.isAnimated());
    EXPECT_NEAR (prop.getValueAt (5.0f), 0.0f, 0.001f);
}
