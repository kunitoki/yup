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

class AnimationEasingTests : public ::testing::Test
{
};

TEST_F (AnimationEasingTests, LinearEvaluatesToT)
{
    auto easing = AnimationEasing::linear();
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.5f), 0.5f, 0.001f);
    EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
}

TEST_F (AnimationEasingTests, LinearIsLinear)
{
    EXPECT_TRUE (AnimationEasing::linear().isLinear());
    EXPECT_FALSE (AnimationEasing::easeIn().isLinear());
    EXPECT_FALSE (AnimationEasing::hold().isLinear());
}

TEST_F (AnimationEasingTests, HoldReturnsZeroForAllT)
{
    auto easing = AnimationEasing::hold();
    EXPECT_TRUE (easing.isHold());
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.5f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.99f), 0.0f, 0.001f);
}

TEST_F (AnimationEasingTests, EaseInStartsSlow)
{
    auto easing = AnimationEasing::easeIn();
    const float atQuarter = easing.evaluate (0.25f);
    const float atHalf = easing.evaluate (0.5f);
    EXPECT_LT (atQuarter, 0.25f); // slower than linear at start
    EXPECT_LT (atHalf, 0.5f);     // still slower than linear at mid
}

TEST_F (AnimationEasingTests, EaseOutEndsSlow)
{
    auto easing = AnimationEasing::easeOut();
    const float atHalf = easing.evaluate (0.5f);
    const float atThreeQuarters = easing.evaluate (0.75f);
    EXPECT_GT (atHalf, 0.5f);           // faster than linear at start
    EXPECT_GT (atThreeQuarters, 0.75f); // still ahead at 75%
}

TEST_F (AnimationEasingTests, EvaluateBoundaries)
{
    for (auto easing : { AnimationEasing::linear(), AnimationEasing::easeIn(), AnimationEasing::easeOut(), AnimationEasing::easeInOut() })
    {
        EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
        EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
    }
}

TEST_F (AnimationEasingTests, FromLottieTangentsRoundtrips)
{
    const Point<float> outTangent { 0.42f, 0.0f };
    const Point<float> inTangent { 0.58f, 1.0f };
    auto easing = AnimationEasing::fromLottieTangents (outTangent, inTangent);

    EXPECT_FALSE (easing.isLinear());
    EXPECT_FALSE (easing.isHold());
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
}

TEST_F (AnimationEasingTests, MonotonicallyIncreasing)
{
    auto easing = AnimationEasing::easeInOut();
    float prev = 0.0f;
    for (int i = 1; i <= 100; ++i)
    {
        const float t = (float) i / 100.0f;
        const float v = easing.evaluate (t);
        EXPECT_GE (v, prev - 0.001f); // should be non-decreasing
        prev = v;
    }
}

TEST_F (AnimationEasingTests, EaseInOutSymmetricAtHalf)
{
    auto easing = AnimationEasing::easeInOut();
    // At t=0.5 easeInOut should be at 0.5 exactly (symmetric curve)
    EXPECT_NEAR (easing.evaluate (0.5f), 0.5f, 0.05f);
}

TEST_F (AnimationEasingTests, LinearEasingSatisfiesAdditivity)
{
    auto easing = AnimationEasing::linear();
    // For linear: evaluate(a) + evaluate(b) ≈ evaluate(a + b) only when a+b <= 1
    // More directly: f(0.3) + f(0.7) should equal f(1.0) = 1.0
    EXPECT_NEAR (easing.evaluate (0.3f) + easing.evaluate (0.7f), 1.0f, 0.001f);
}

TEST_F (AnimationEasingTests, CustomTangentsProduceValuesBetween0And1)
{
    const Point<float> outT { 0.25f, 0.1f };
    const Point<float> inT { 0.75f, 0.9f };
    auto easing = AnimationEasing::fromLottieTangents (outT, inT);

    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.0f;
        const float v = easing.evaluate (t);
        EXPECT_GE (v, -0.01f) << "t=" << t;
        EXPECT_LE (v, 1.01f) << "t=" << t;
    }
}

TEST_F (AnimationEasingTests, AllBuiltInEasingTypesAreNotHold)
{
    EXPECT_FALSE (AnimationEasing::linear().isHold());
    EXPECT_FALSE (AnimationEasing::easeIn().isHold());
    EXPECT_FALSE (AnimationEasing::easeOut().isHold());
    EXPECT_FALSE (AnimationEasing::easeInOut().isHold());
}

TEST_F (AnimationEasingTests, EaseInIsNotEaseOut)
{
    auto eIn = AnimationEasing::easeIn();
    auto eOut = AnimationEasing::easeOut();

    // At t=0.3: easeIn < linear < easeOut
    const float vIn = eIn.evaluate (0.3f);
    const float vOut = eOut.evaluate (0.3f);
    EXPECT_LT (vIn, vOut);
}
