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

class EasingTests : public ::testing::Test
{
protected:
    static void expectEvaluateFollowsCurve (const Easing& easing, std::initializer_list<float> parameters)
    {
        for (const float s : parameters)
        {
            const auto point = easing.getCurve().pointAt (s);
            EXPECT_NEAR (easing.evaluate (point.getX()), point.getY(), 1e-4f) << "s=" << s;
        }
    }
};

TEST_F (EasingTests, LinearEvaluatesToT)
{
    auto easing = Easing::linear();
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.5f), 0.5f, 0.001f);
    EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
}

TEST_F (EasingTests, LinearIsLinear)
{
    EXPECT_TRUE (Easing::linear().isLinear());
    EXPECT_FALSE (Easing::easeIn().isLinear());
    EXPECT_FALSE (Easing::hold().isLinear());
}

TEST_F (EasingTests, HoldReturnsZeroForAllT)
{
    auto easing = Easing::hold();
    EXPECT_TRUE (easing.isHold());
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.5f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (0.99f), 0.0f, 0.001f);
}

TEST_F (EasingTests, EaseInStartsSlow)
{
    auto easing = Easing::easeIn();
    const float atQuarter = easing.evaluate (0.25f);
    const float atHalf = easing.evaluate (0.5f);
    EXPECT_LT (atQuarter, 0.25f); // slower than linear at start
    EXPECT_LT (atHalf, 0.5f);     // still slower than linear at mid
}

TEST_F (EasingTests, EaseOutEndsSlow)
{
    auto easing = Easing::easeOut();
    const float atHalf = easing.evaluate (0.5f);
    const float atThreeQuarters = easing.evaluate (0.75f);
    EXPECT_GT (atHalf, 0.5f);           // faster than linear at start
    EXPECT_GT (atThreeQuarters, 0.75f); // still ahead at 75%
}

TEST_F (EasingTests, EvaluateBoundaries)
{
    for (auto easing : { Easing::linear(), Easing::easeIn(), Easing::easeOut(), Easing::easeInOut() })
    {
        EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
        EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
    }
}

TEST_F (EasingTests, FromTangentsRoundtrips)
{
    const Point<float> outTangent { 0.42f, 0.0f };
    const Point<float> inTangent { 0.58f, 1.0f };
    auto easing = Easing::fromTangents (outTangent, inTangent);

    EXPECT_FALSE (easing.isLinear());
    EXPECT_FALSE (easing.isHold());
    EXPECT_NEAR (easing.evaluate (0.0f), 0.0f, 0.001f);
    EXPECT_NEAR (easing.evaluate (1.0f), 1.0f, 0.001f);
}

TEST_F (EasingTests, MonotonicallyIncreasing)
{
    auto easing = Easing::easeInOut();
    float prev = 0.0f;
    for (int i = 1; i <= 100; ++i)
    {
        const float t = (float) i / 100.0f;
        const float v = easing.evaluate (t);
        EXPECT_GE (v, prev - 0.001f); // should be non-decreasing
        prev = v;
    }
}

TEST_F (EasingTests, EaseInOutSymmetricAtHalf)
{
    auto easing = Easing::easeInOut();
    // At t=0.5 easeInOut should be at 0.5 exactly (symmetric curve)
    EXPECT_NEAR (easing.evaluate (0.5f), 0.5f, 0.05f);
}

TEST_F (EasingTests, LinearEasingSatisfiesAdditivity)
{
    auto easing = Easing::linear();
    // For linear: evaluate(a) + evaluate(b) ≈ evaluate(a + b) only when a+b <= 1
    // More directly: f(0.3) + f(0.7) should equal f(1.0) = 1.0
    EXPECT_NEAR (easing.evaluate (0.3f) + easing.evaluate (0.7f), 1.0f, 0.001f);
}

TEST_F (EasingTests, CustomTangentsProduceValuesBetween0And1)
{
    const Point<float> outT { 0.25f, 0.1f };
    const Point<float> inT { 0.75f, 0.9f };
    auto easing = Easing::fromTangents (outT, inT);

    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.0f;
        const float v = easing.evaluate (t);
        EXPECT_GE (v, -0.01f) << "t=" << t;
        EXPECT_LE (v, 1.01f) << "t=" << t;
    }
}

TEST_F (EasingTests, AllBuiltInEasingTypesAreNotHold)
{
    EXPECT_FALSE (Easing::linear().isHold());
    EXPECT_FALSE (Easing::easeIn().isHold());
    EXPECT_FALSE (Easing::easeOut().isHold());
    EXPECT_FALSE (Easing::easeInOut().isHold());
}

TEST_F (EasingTests, EaseInIsNotEaseOut)
{
    auto eIn = Easing::easeIn();
    auto eOut = Easing::easeOut();

    // At t=0.3: easeIn < linear < easeOut
    const float vIn = eIn.evaluate (0.3f);
    const float vOut = eOut.evaluate (0.3f);
    EXPECT_LT (vIn, vOut);
}

// ── CubicBezier curve ─────────────────────────────────────────────────────────

TEST_F (EasingTests, GetCurveSpansUnitSquare)
{
    const Easing easing (0.25f, 0.1f, 0.75f, 0.9f);
    const auto& curve = easing.getCurve();

    EXPECT_EQ (curve.p0(), Point<float> (0.0f, 0.0f));
    EXPECT_EQ (curve.p1(), Point<float> (0.25f, 0.1f));
    EXPECT_EQ (curve.p2(), Point<float> (0.75f, 0.9f));
    EXPECT_EQ (curve.p3(), Point<float> (1.0f, 1.0f));
}

TEST_F (EasingTests, EvaluateInvertsCurveX)
{
    for (const auto& easing : { Easing::easeIn(), Easing::easeOut(), Easing::easeInOut(), Easing (0.68f, -0.55f, 0.265f, 1.55f) })
        expectEvaluateFollowsCurve (easing, { 0.05f, 0.2f, 0.35f, 0.5f, 0.65f, 0.8f, 0.95f });
}

TEST_F (EasingTests, FlatSlopeStillSolvesCurve)
{
    // x(t) = 3t - 6t^2 + 4t^3 has a zero slope at t = 0.5, so Newton steps leave the curve
    const Easing easing (1.0f, 0.0f, 0.0f, 1.0f);

    expectEvaluateFollowsCurve (easing, { 0.1f, 0.2f, 0.3f, 0.7f, 0.8f, 0.9f });
    EXPECT_NEAR (easing.evaluate (0.5f), 0.5f, 1e-4f);
}

// ── operator== / operator!= ───────────────────────────────────────────────────

TEST_F (EasingTests, DefaultConstructedEqualsLinear)
{
    Easing def;
    EXPECT_EQ (def, Easing::linear());
}

TEST_F (EasingTests, LinearEqualsSelf)
{
    EXPECT_EQ (Easing::linear(), Easing::linear());
}

TEST_F (EasingTests, LinearNotEqualsEaseIn)
{
    EXPECT_NE (Easing::linear(), Easing::easeIn());
}

TEST_F (EasingTests, LinearNotEqualsEaseOut)
{
    EXPECT_NE (Easing::linear(), Easing::easeOut());
}

TEST_F (EasingTests, LinearNotEqualsEaseInOut)
{
    EXPECT_NE (Easing::linear(), Easing::easeInOut());
}

TEST_F (EasingTests, LinearNotEqualsHold)
{
    EXPECT_NE (Easing::linear(), Easing::hold());
}

TEST_F (EasingTests, EaseInEqualsSelf)
{
    EXPECT_EQ (Easing::easeIn(), Easing::easeIn());
}

TEST_F (EasingTests, EaseInNotEqualsEaseOut)
{
    EXPECT_NE (Easing::easeIn(), Easing::easeOut());
}

TEST_F (EasingTests, EaseInNotEqualsEaseInOut)
{
    EXPECT_NE (Easing::easeIn(), Easing::easeInOut());
}

TEST_F (EasingTests, EaseOutEqualsSelf)
{
    EXPECT_EQ (Easing::easeOut(), Easing::easeOut());
}

TEST_F (EasingTests, HoldEqualsSelf)
{
    EXPECT_EQ (Easing::hold(), Easing::hold());
}

TEST_F (EasingTests, CopyConstructedEqualsOriginal)
{
    auto original = Easing::easeInOut();
    auto copy = original;
    EXPECT_EQ (copy, original);
}

TEST_F (EasingTests, SameTangentsProduceEqualEasing)
{
    const Point<float> outT { 0.25f, 0.1f };
    const Point<float> inT { 0.75f, 0.9f };
    auto a = Easing::fromTangents (outT, inT);
    auto b = Easing::fromTangents (outT, inT);
    EXPECT_EQ (a, b);
}

TEST_F (EasingTests, DifferentTangentsProduceNotEqualEasing)
{
    auto a = Easing::fromTangents ({ 0.25f, 0.1f }, { 0.75f, 0.9f });
    auto b = Easing::fromTangents ({ 0.5f, 0.0f }, { 0.5f, 1.0f });
    EXPECT_NE (a, b);
}

TEST_F (EasingTests, HoldNotEqualsEaseIn)
{
    EXPECT_NE (Easing::hold(), Easing::easeIn());
}

TEST_F (EasingTests, HoldNotEqualsEaseOut)
{
    EXPECT_NE (Easing::hold(), Easing::easeOut());
}
