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

TEST_F (LottieExpressionEvaluatorTests, LoopOutReturnsUnknown)
{
    LottieExpressionEvaluator eval;
    eval.setupCompositionContext (makeTestCtx());
    auto r = eval.evaluate ("loopOut('cycle')");
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
