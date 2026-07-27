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

namespace
{

LottieExpressionEvaluator::CompositionContext makeTestCtx()
{
    LottieExpressionEvaluator::CompositionContext ctx;
    ctx.size = { 500.0f, 500.0f };
    ctx.frameRate = 30.0f;
    return ctx;
}

} // namespace

class LottieExpressionEvaluatorTests : public ::testing::Test
{
protected:
    AnimationTransform dummyTransform;
};

// ── Composition context ──────────────────────────────────────────────────────

TEST_F (LottieExpressionEvaluatorTests, EmptyExpressionReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    EXPECT_EQ (eval.evaluate ({}).kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ArithmeticExpressionReturnsStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("2 + 3");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 5.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LayerPositionReferenceByName)
{
    dummyTransform.position = AnimationProperty<Point<float>>::staticValue ({ 100.0f, 200.0f });

    auto ctx = makeTestCtx();
    ctx.layers.push_back ({ "Background", 1, &dummyTransform });

    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (ctx);
    auto r = eval.evaluate (R"(thisComp.layer("Background").transform.position)");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::LayerPropertyRef);
    EXPECT_EQ (r.referencedLayerName, "Background");
    EXPECT_EQ (r.referencedProperty, "transform.position");
}

TEST_F (LottieExpressionEvaluatorTests, LayerRotationReferenceByName)
{
    dummyTransform.rotation = AnimationProperty<float>::staticValue (45.0f);

    auto ctx = makeTestCtx();
    ctx.layers.push_back ({ "Spinner", 2, &dummyTransform });

    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (ctx);
    auto r = eval.evaluate (R"(thisComp.layer("Spinner").transform.rotation)");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::LayerPropertyRef);
    EXPECT_EQ (r.referencedProperty, "transform.rotation");
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 45.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LayerReferenceByIndex)
{
    dummyTransform.opacity = AnimationProperty<float>::staticValue (80.0f);

    auto ctx = makeTestCtx();
    ctx.layers.push_back ({ "Foo", 3, &dummyTransform });

    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (ctx);
    auto r = eval.evaluate ("thisComp.layer(3).transform.opacity");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::LayerPropertyRef);
    EXPECT_EQ (r.referencedLayerId, 3);
    EXPECT_EQ (r.referencedProperty, "transform.opacity");
}

TEST_F (LottieExpressionEvaluatorTests, UnknownLayerNameReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate (R"(thisComp.layer("Nonexistent").transform.position)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisCompDimensions)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto w = eval.evaluate ("thisComp.width");
    auto h = eval.evaluate ("thisComp.height");
    EXPECT_EQ (w.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (w.value)), 500.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (h.value)), 500.0f);
}

TEST_F (LottieExpressionEvaluatorTests, MathPiExpression)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("Math.PI * 2");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_NEAR (static_cast<double> (r.value), MathConstants<double>::twoPi, 1e-6);
}

TEST_F (LottieExpressionEvaluatorTests, BmRtMultiStatementExpressionHandled)
{
    dummyTransform.position = AnimationProperty<Point<float>>::staticValue ({ 10.0f, 20.0f });

    auto ctx = makeTestCtx();
    ctx.layers.push_back ({ "Null 1", 1, &dummyTransform });

    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (ctx);

    // This is the exact pattern used in Lottie files from some AE exporters.
    auto r = eval.evaluate ("var $bm_rt;\n$bm_rt = thisComp.layer(\"Null 1\").transform.position;");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::LayerPropertyRef);
    EXPECT_EQ (r.referencedLayerName, "Null 1");
    EXPECT_EQ (r.referencedProperty, "transform.position");
}

// ── AE math helpers ──────────────────────────────────────────────────────────

TEST_F (LottieExpressionEvaluatorTests, DivHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("div(100, 4)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 25.0f);
}

TEST_F (LottieExpressionEvaluatorTests, SubHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("sub(100, 40)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 60.0f);
}

TEST_F (LottieExpressionEvaluatorTests, AddHelperWithScalarsProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("add(2, 3)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 5.0f);
}

TEST_F (LottieExpressionEvaluatorTests, AddHelperWithArraysProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("add([1,2,3], [4,5,6])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    const auto* arr = r.value.getArray();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size(), 3);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[0])), 5.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[1])), 7.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[2])), 9.0f);
}

TEST_F (LottieExpressionEvaluatorTests, MulHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("mul(5, 2)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 10.0f);
}

TEST_F (LottieExpressionEvaluatorTests, MulHelperWithArrayProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("mul([1,2,3], 2)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    const auto* arr = r.value.getArray();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size(), 3);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[0])), 2.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[1])), 4.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[2])), 6.0f);
}

TEST_F (LottieExpressionEvaluatorTests, ClampHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("clamp(50, 0, 100)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 50.0f);
}

TEST_F (LottieExpressionEvaluatorTests, ClampHelperBelowMinClampsToMin)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("clamp(-10, 0, 100)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 0.0f);
}

TEST_F (LottieExpressionEvaluatorTests, ClampHelperAboveMaxClampsToMax)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("clamp(200, 0, 100)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 100.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LinearHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("linear(0.5, 0, 100)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 50.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LengthHelperWithScalarProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("length(-5)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 5.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LengthHelperWithArrayProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("length([3, 4])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 5.0f);
}

TEST_F (LottieExpressionEvaluatorTests, LengthHelperWithTwoArraysProducesDistance)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("length([1, 2], [4, 6])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 5.0f);
}

TEST_F (LottieExpressionEvaluatorTests, NormalizeHelperProducesUnitVector)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("normalize([3, 4])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    const auto* arr = r.value.getArray();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[0])), 0.6f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[1])), 0.8f);
}

TEST_F (LottieExpressionEvaluatorTests, NormalizeHelperWithZeroLengthReturnsSame)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("normalize([0, 0])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    const auto* arr = r.value.getArray();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[0])), 0.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[1])), 0.0f);
}

TEST_F (LottieExpressionEvaluatorTests, DotHelperWithArraysProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("dot([1, 2], [3, 4])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 11.0f);
}

TEST_F (LottieExpressionEvaluatorTests, DotHelperWithScalarsProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("dot(2, 3)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 6.0f);
}

TEST_F (LottieExpressionEvaluatorTests, DegreesToRadiansHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("degreesToRadians(180)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_NEAR (static_cast<double> (r.value), MathConstants<double>::pi, 1e-6);
}

TEST_F (LottieExpressionEvaluatorTests, RadiansToDegreesHelperProducesStaticValue)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("radiansToDegrees(Math.PI)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_NEAR (static_cast<double> (r.value), 180.0, 1e-6);
}

TEST_F (LottieExpressionEvaluatorTests, RandomHelperReturnsZero)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("random(0, 100)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 0.0f);
}

TEST_F (LottieExpressionEvaluatorTests, WiggleHelperReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("wiggle(5, 20)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, LoopOutReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("loopOut('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, LoopInReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("loopIn('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, LoopOutDurationReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("loopOutDuration('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, LoopInDurationReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("loopInDuration('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisPropertyNumKeysGuardDoesNotThrow)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    // Expression pattern: if (thisProperty.numKeys > 1) { $bm_rt = thisProperty.loopOut('cycle'); } else { $bm_rt = value; }
    auto r = eval.evaluate ("var $bm_rt; if (thisProperty.numKeys > 1) { $bm_rt = thisProperty.loopOut('cycle'); } else { $bm_rt = 42; } $bm_rt");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 42.0f);
}

TEST_F (LottieExpressionEvaluatorTests, ThisLayerEffectChainDoesNotThrow)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisLayer.effect('Trace Path')('Progress')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisLayerToCompPassesThrough)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisLayer.toComp([100, 200])");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    const auto* arr = r.value.getArray();
    ASSERT_NE (arr, nullptr);
    ASSERT_EQ (arr->size(), 2);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[0])), 100.0f);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> ((*arr)[1])), 200.0f);
}

TEST_F (LottieExpressionEvaluatorTests, ThisLayerToCompNoArgsReturnsEmpty)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisLayer.toComp()");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisPropertyLoopOutDirectCallReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisProperty.loopOut('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisPropertyLoopInDirectCallReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisProperty.loopIn('cycle')");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ThisCompFrameRate)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("thisComp.frameRate");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 30.0f);
}

// ── Evaluate failure scenarios ────────────────────────────────────────────────

TEST_F (LottieExpressionEvaluatorTests, InvalidSyntaxReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("2 +");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, ReferenceToUndefinedVariableReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("undefinedVar");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::Unknown);
}

TEST_F (LottieExpressionEvaluatorTests, DivHelperZeroDivisorReturnsZero)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("div(100, 0)");
    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::StaticValue);
    EXPECT_FLOAT_EQ (static_cast<float> (static_cast<double> (r.value)), 0.0f);
}

// ── Shape content context ────────────────────────────────────────────────────

TEST_F (LottieExpressionEvaluatorTests, ShapePathReference)
{
    ShapeLayer dummyLayer;
    LottieExpressionEvaluator eval;
    eval.setupShapeContext (dummyLayer);
    auto r = eval.evaluate (R"(content("Group 1").content("Path 1").path)");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef);
    EXPECT_EQ (r.contentGroupName, "Group 1");
    EXPECT_EQ (r.contentItemName, "Path 1");
    EXPECT_EQ (r.contentProperty, "path");
}

TEST_F (LottieExpressionEvaluatorTests, ShapeRotationReference)
{
    ShapeLayer dummyLayer;
    LottieExpressionEvaluator eval;
    eval.setupShapeContext (dummyLayer);
    auto r = eval.evaluate (R"(content("Group 2").transform.rotation)");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef);
    EXPECT_EQ (r.contentGroupName, "Group 2");
    EXPECT_EQ (r.contentProperty, "transform.rotation");
}

TEST_F (LottieExpressionEvaluatorTests, BmRtShapePathReference)
{
    ShapeLayer dummyLayer;
    LottieExpressionEvaluator eval;
    eval.setupShapeContext (dummyLayer);
    auto r = eval.evaluate ("var $bm_rt;\n$bm_rt = content('Shape 1').content('Path 1').path;");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef);
    EXPECT_EQ (r.contentGroupName, "Shape 1");
    EXPECT_EQ (r.contentItemName, "Path 1");
    EXPECT_EQ (r.contentProperty, "path");
}

TEST_F (LottieExpressionEvaluatorTests, BmRtShapeRotationReference)
{
    ShapeLayer dummyLayer;
    LottieExpressionEvaluator eval;
    eval.setupShapeContext (dummyLayer);
    auto r = eval.evaluate ("var $bm_rt;\n$bm_rt = content('Shape 1').transform.rotation;");

    EXPECT_EQ (r.kind, LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef);
    EXPECT_EQ (r.contentGroupName, "Shape 1");
    EXPECT_EQ (r.contentProperty, "transform.rotation");
}
