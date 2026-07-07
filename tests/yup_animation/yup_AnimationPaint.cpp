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

// =============================================================================
// FillPaint
// =============================================================================

class FillPaintTests : public ::testing::Test
{
};

// ---- colorAt ----------------------------------------------------------------

TEST_F (FillPaintTests, ColorAt_DefaultIsBlack)
{
    FillPaint fill;
    EXPECT_EQ (fill.colorAt (0.0f), Color (0xFF000000));
}

TEST_F (FillPaintTests, ColorAt_ReturnsSetColor)
{
    FillPaint fill;
    fill.color = ColorProperty::staticValue (Color (0xFFFF0000));
    EXPECT_EQ (fill.colorAt (0.0f), Color (0xFFFF0000));
}

TEST_F (FillPaintTests, ColorAt_IndependentOfFrameNumberForStaticColor)
{
    FillPaint fill;
    fill.color = ColorProperty::staticValue (Color (0xFF0000FF));
    EXPECT_EQ (fill.colorAt (0.0f), Color (0xFF0000FF));
    EXPECT_EQ (fill.colorAt (30.0f), Color (0xFF0000FF));
    EXPECT_EQ (fill.colorAt (99.0f), Color (0xFF0000FF));
}

// ---- opacityAt --------------------------------------------------------------

TEST_F (FillPaintTests, OpacityAt_DefaultIsFull)
{
    FillPaint fill;
    EXPECT_FLOAT_EQ (fill.opacityAt (0.0f), 1.0f);
}

TEST_F (FillPaintTests, OpacityAt_HalfOpacity)
{
    FillPaint fill;
    fill.opacity = FloatProperty::staticValue (50.0f);
    EXPECT_FLOAT_EQ (fill.opacityAt (0.0f), 0.5f);
}

TEST_F (FillPaintTests, OpacityAt_ZeroOpacity)
{
    FillPaint fill;
    fill.opacity = FloatProperty::staticValue (0.0f);
    EXPECT_FLOAT_EQ (fill.opacityAt (0.0f), 0.0f);
}

TEST_F (FillPaintTests, OpacityAt_FullOpacity_IsOne)
{
    FillPaint fill;
    fill.opacity = FloatProperty::staticValue (100.0f);
    EXPECT_FLOAT_EQ (fill.opacityAt (0.0f), 1.0f);
}

// ---- defaults ---------------------------------------------------------------

TEST_F (FillPaintTests, DefaultsAreCorrect)
{
    FillPaint fill;
    EXPECT_FALSE (fill.hidden);
    EXPECT_TRUE (fill.enabled);
    EXPECT_EQ (fill.fillRule, FillPaint::FillRule::NonZero);
}

TEST_F (FillPaintTests, FillRuleEnum_ValuesAreDistinct)
{
    EXPECT_NE (FillPaint::FillRule::NonZero, FillPaint::FillRule::EvenOdd);
}

// =============================================================================
// StrokePaint
// =============================================================================

class StrokePaintTests : public ::testing::Test
{
};

// ---- colorAt ----------------------------------------------------------------

TEST_F (StrokePaintTests, ColorAt_DefaultIsBlack)
{
    StrokePaint stroke;
    EXPECT_EQ (stroke.colorAt (0.0f), Color (0xFF000000));
}

TEST_F (StrokePaintTests, ColorAt_ReturnsSetColor)
{
    StrokePaint stroke;
    stroke.color = ColorProperty::staticValue (Color (0xFF00FF00));
    EXPECT_EQ (stroke.colorAt (0.0f), Color (0xFF00FF00));
}

// ---- opacityAt --------------------------------------------------------------

TEST_F (StrokePaintTests, OpacityAt_DefaultIsFull)
{
    StrokePaint stroke;
    EXPECT_FLOAT_EQ (stroke.opacityAt (0.0f), 1.0f);
}

TEST_F (StrokePaintTests, OpacityAt_HalfOpacity)
{
    StrokePaint stroke;
    stroke.opacity = FloatProperty::staticValue (50.0f);
    EXPECT_FLOAT_EQ (stroke.opacityAt (0.0f), 0.5f);
}

TEST_F (StrokePaintTests, OpacityAt_ZeroOpacity)
{
    StrokePaint stroke;
    stroke.opacity = FloatProperty::staticValue (0.0f);
    EXPECT_FLOAT_EQ (stroke.opacityAt (0.0f), 0.0f);
}

// ---- widthAt ----------------------------------------------------------------

TEST_F (StrokePaintTests, WidthAt_DefaultIsTwo)
{
    StrokePaint stroke;
    EXPECT_FLOAT_EQ (stroke.widthAt (0.0f), 2.0f);
}

TEST_F (StrokePaintTests, WidthAt_ReturnsSetWidth)
{
    StrokePaint stroke;
    stroke.width = FloatProperty::staticValue (5.0f);
    EXPECT_FLOAT_EQ (stroke.widthAt (0.0f), 5.0f);
}

TEST_F (StrokePaintTests, WidthAt_IndependentOfFrameForStatic)
{
    StrokePaint stroke;
    stroke.width = FloatProperty::staticValue (3.0f);
    EXPECT_FLOAT_EQ (stroke.widthAt (0.0f), 3.0f);
    EXPECT_FLOAT_EQ (stroke.widthAt (30.0f), 3.0f);
}

// ---- strokeTypeAt -----------------------------------------------------------

TEST_F (StrokePaintTests, StrokeTypeAt_DefaultWidthMatchesWidthAt)
{
    StrokePaint stroke;
    stroke.width = FloatProperty::staticValue (4.0f);
    const auto st = stroke.strokeTypeAt (0.0f);
    EXPECT_FLOAT_EQ (st.getWidth(), 4.0f);
}

TEST_F (StrokePaintTests, StrokeTypeAt_CapMatchesMemberCap)
{
    StrokePaint stroke;
    stroke.cap = StrokeCap::Round;
    const auto st = stroke.strokeTypeAt (0.0f);
    EXPECT_EQ (st.getCap(), StrokeCap::Round);
}

TEST_F (StrokePaintTests, StrokeTypeAt_JoinMatchesMemberJoin)
{
    StrokePaint stroke;
    stroke.join = StrokeJoin::Bevel;
    const auto st = stroke.strokeTypeAt (0.0f);
    EXPECT_EQ (st.getJoin(), StrokeJoin::Bevel);
}

// ---- defaults ---------------------------------------------------------------

TEST_F (StrokePaintTests, DefaultsAreCorrect)
{
    StrokePaint stroke;
    EXPECT_FALSE (stroke.hidden);
    EXPECT_TRUE (stroke.enabled);
    EXPECT_EQ (stroke.cap, StrokeCap::Butt);
    EXPECT_EQ (stroke.join, StrokeJoin::Miter);
    EXPECT_FLOAT_EQ (stroke.miterLimit, 4.0f);
}

// =============================================================================
// AnimationGradient
// =============================================================================

class AnimationGradientTests : public ::testing::Test
{
};

TEST_F (AnimationGradientTests, DefaultConstruction_EmptyColorStops)
{
    AnimationGradient grad;
    EXPECT_TRUE (grad.colorStops.empty());
}

TEST_F (AnimationGradientTests, DefaultType_IsLinear)
{
    AnimationGradient grad;
    EXPECT_EQ (grad.gradientType, AnimationGradient::GradientType::Linear);
}

TEST_F (AnimationGradientTests, AddColorStop_AddsOneStop)
{
    AnimationGradient grad;
    grad.addColorStop (0.0f, Color (0xFF000000));
    EXPECT_EQ (grad.colorStops.size(), 1u);
}

TEST_F (AnimationGradientTests, AddColorStop_StoresPositionAndColor)
{
    AnimationGradient grad;
    grad.addColorStop (0.25f, Color (0xFFFF0000));

    ASSERT_EQ (grad.colorStops.size(), 1u);
    EXPECT_FLOAT_EQ (grad.colorStops[0].position.getValueAt (0.0f), 0.25f);
    EXPECT_EQ (grad.colorStops[0].color.getValueAt (0.0f), Color (0xFFFF0000));
}

TEST_F (AnimationGradientTests, AddColorStop_MultipleStops)
{
    AnimationGradient grad;
    grad.addColorStop (0.0f, Color (0xFF000000));
    grad.addColorStop (0.5f, Color (0xFF808080));
    grad.addColorStop (1.0f, Color (0xFFFFFFFF));

    EXPECT_EQ (grad.colorStops.size(), 3u);
}

TEST_F (AnimationGradientTests, ToColorGradient_EmptyStopsReturnsGradient)
{
    AnimationGradient grad;
    // Must not crash with no stops
    const auto cg = grad.toColorGradient (0.0f);
    (void) cg;
}

TEST_F (AnimationGradientTests, ToColorGradient_WithStopsReturnsNonEmptyGradient)
{
    AnimationGradient grad;
    grad.startPoint = Vec2Property::staticValue ({ 0.0f, 0.0f });
    grad.endPoint = Vec2Property::staticValue ({ 100.0f, 0.0f });
    grad.addColorStop (0.0f, Color (0xFF000000));
    grad.addColorStop (1.0f, Color (0xFFFFFFFF));
    grad.numColorPoints = 2;

    const auto cg = grad.toColorGradient (0.0f);
    EXPECT_NE (cg.getNumStops(), 0);
}

TEST_F (AnimationGradientTests, ToColorGradient_LinearAndRadialProduceGradient)
{
    AnimationGradient linear, radial;
    linear.gradientType = AnimationGradient::GradientType::Linear;
    radial.gradientType = AnimationGradient::GradientType::Radial;

    linear.addColorStop (0.0f, Color (0xFF000000));
    linear.addColorStop (1.0f, Color (0xFFFFFFFF));
    linear.numColorPoints = 2;
    radial.addColorStop (0.0f, Color (0xFF000000));
    radial.addColorStop (1.0f, Color (0xFFFFFFFF));
    radial.numColorPoints = 2;

    // Both must not crash
    const auto lg = linear.toColorGradient (0.0f);
    const auto rg = radial.toColorGradient (0.0f);
    (void) lg;
    (void) rg;
}

TEST_F (AnimationGradientTests, GradientTypeEnum_ValuesAreDistinct)
{
    EXPECT_NE (AnimationGradient::GradientType::Linear, AnimationGradient::GradientType::Radial);
}
