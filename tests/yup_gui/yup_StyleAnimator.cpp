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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class StyleAnimatorTests : public ::testing::Test
{
protected:
    using Type = ComponentProperty::Type;

    static StyleAnimator::Transition transition (double duration, double delay = 0.0)
    {
        StyleAnimator::Transition result;
        result.property = Identifier ("glow");
        result.duration = duration;
        result.delay = delay;
        return result;
    }

    static StyleAnimator::Track track (std::vector<StyleAnimator::Frame> frames)
    {
        StyleAnimator::Track result;
        result.storageId = Identifier ("glow");
        result.type = Type::number;
        result.frames = std::move (frames);
        return result;
    }

    void startAnimation (const String& text, std::vector<StyleAnimator::Frame> frames)
    {
        auto parsed = StyleAnimator::parseAnimations (text);
        ASSERT_TRUE (parsed.wasOk()) << parsed.getErrorMessage();
        ASSERT_EQ (parsed.getReference().size(), 1u);

        std::vector<StyleAnimator::Track> tracks;
        tracks.push_back (track (std::move (frames)));
        animator.startAnimation (component, parsed.getReference().front(), std::move (tracks), 0.0);
    }

    void advance (double now)
    {
        reported.clear();
        ended.clear();

        animator.advance (now, [this] (Component&, const Identifier& storageId, const var* value)
        {
            if (value != nullptr)
                reported.set (storageId, *value);
            else
                ended.add (storageId);
        });
    }

    double glow() const
    {
        const auto* value = reported.getVarPointer (Identifier ("glow"));
        return value != nullptr ? static_cast<double> (*value) : -1.0;
    }

    Component component;
    StyleAnimator animator;
    NamedValueSet reported;
    Array<Identifier> ended;
};

TEST_F (StyleAnimatorTests, ParsesTransitionEntries)
{
    const auto parsed = StyleAnimator::parseTransitions ("opacity 200ms easeOut, thumbColor 0.15s linear 50ms");
    ASSERT_TRUE (parsed.wasOk()) << parsed.getErrorMessage();

    const auto& entries = parsed.getReference();
    ASSERT_EQ (entries.size(), 2u);

    EXPECT_EQ (entries[0].property, Identifier ("opacity"));
    EXPECT_DOUBLE_EQ (entries[0].duration, 0.2);
    EXPECT_DOUBLE_EQ (entries[0].delay, 0.0);
    EXPECT_TRUE (entries[0].easing == Easing::easeOut());

    EXPECT_EQ (entries[1].property, Identifier ("thumbColor"));
    EXPECT_DOUBLE_EQ (entries[1].duration, 0.15);
    EXPECT_DOUBLE_EQ (entries[1].delay, 0.05);
    EXPECT_TRUE (entries[1].easing.isLinear());
}

TEST_F (StyleAnimatorTests, ParsesCubicBezierEasings)
{
    for (const auto* text : { "glow 1s cubicBezier(0.4, 0, 0.2, 1)", "glow 1s cubicBezier (0.4, 0, 0.2, 1), opacity 1s" })
    {
        const auto parsed = StyleAnimator::parseTransitions (text);
        ASSERT_TRUE (parsed.wasOk()) << parsed.getErrorMessage();
        EXPECT_TRUE (parsed.getReference().front().easing == Easing (0.4f, 0.0f, 0.2f, 1.0f)) << text;
    }
}

TEST_F (StyleAnimatorTests, NoneDeclaresNothing)
{
    const auto transitions = StyleAnimator::parseTransitions ("none");
    ASSERT_TRUE (transitions.wasOk());
    EXPECT_TRUE (transitions.getReference().empty());

    const auto animations = StyleAnimator::parseAnimations (" none ");
    ASSERT_TRUE (animations.wasOk());
    EXPECT_TRUE (animations.getReference().empty());
}

TEST_F (StyleAnimatorTests, ReportsTransitionErrors)
{
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 200").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 1s 2s 3s").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow opacity 1s").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("200ms").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 1s ease-out").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 1s cubicBezier (2, 0, 0, 1)").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 1s cubicBezier (0, 0, 1)").failed());
    EXPECT_TRUE (StyleAnimator::parseTransitions ("glow 1s, ").failed());
}

TEST_F (StyleAnimatorTests, ParsesAnimationEntries)
{
    const auto parsed = StyleAnimator::parseAnimations ("pulse 1s easeInOut 0.5s infinite alternate both, flash 200ms 2.5 reverse forwards");
    ASSERT_TRUE (parsed.wasOk()) << parsed.getErrorMessage();

    const auto& entries = parsed.getReference();
    ASSERT_EQ (entries.size(), 2u);

    EXPECT_EQ (entries[0].name, Identifier ("pulse"));
    EXPECT_DOUBLE_EQ (entries[0].duration, 1.0);
    EXPECT_DOUBLE_EQ (entries[0].delay, 0.5);
    EXPECT_TRUE (entries[0].easing == Easing::easeInOut());
    EXPECT_TRUE (std::isinf (entries[0].iterations));
    EXPECT_EQ (entries[0].direction, StyleAnimator::Direction::alternate);
    EXPECT_EQ (entries[0].fillMode, StyleAnimator::FillMode::both);

    EXPECT_EQ (entries[1].name, Identifier ("flash"));
    EXPECT_DOUBLE_EQ (entries[1].duration, 0.2);
    EXPECT_DOUBLE_EQ (entries[1].iterations, 2.5);
    EXPECT_EQ (entries[1].direction, StyleAnimator::Direction::reverse);
    EXPECT_EQ (entries[1].fillMode, StyleAnimator::FillMode::forwards);
}

TEST_F (StyleAnimatorTests, ReportsAnimationErrors)
{
    EXPECT_TRUE (StyleAnimator::parseAnimations ("1s").failed());
    EXPECT_TRUE (StyleAnimator::parseAnimations ("pulse flash 1s").failed());
    EXPECT_TRUE (StyleAnimator::parseAnimations ("pulse 1s 2 3").failed());
    EXPECT_TRUE (StyleAnimator::parseAnimations ("pulse 1s normal reverse").failed());
}

TEST_F (StyleAnimatorTests, InterpolatesNumbersIntegersAndColors)
{
    EXPECT_DOUBLE_EQ (static_cast<double> (StyleAnimator::interpolate (0.0, 10.0, 0.25, Type::number)), 2.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (StyleAnimator::interpolate (0.0, 10.0, 1.5, Type::number)), 15.0);
    EXPECT_EQ (static_cast<int> (StyleAnimator::interpolate (0, 10, 0.26, Type::integer)), 3);

    const auto black = VariantConverter<Color>::toVar (Color (0xff000000));
    const auto white = VariantConverter<Color>::toVar (Color (0x00ffffff));
    const auto mixed = VariantConverter<Color>::fromVar (StyleAnimator::interpolate (black, white, 0.5, Type::color));

    EXPECT_NEAR (mixed.getRed(), 128, 1);
    EXPECT_NEAR (mixed.getGreen(), 128, 1);
    EXPECT_NEAR (mixed.getBlue(), 128, 1);
    EXPECT_NEAR (mixed.getAlpha(), 128, 1);
}

TEST_F (StyleAnimatorTests, InterpolatesPointsRectanglesAndArraysElementWise)
{
    const auto point = StyleAnimator::interpolate (VariantConverter<Point<float>>::toVar ({ 0.0f, 10.0f }),
                                                   VariantConverter<Point<float>>::toVar ({ 10.0f, 20.0f }),
                                                   0.5,
                                                   Type::point);

    EXPECT_EQ (VariantConverter<Point<float>>::fromVar (point), Point<float> (5.0f, 15.0f));

    const auto area = StyleAnimator::interpolate (VariantConverter<Rectangle<float>>::toVar ({ 0.0f, 0.0f, 10.0f, 20.0f }),
                                                  VariantConverter<Rectangle<float>>::toVar ({ 2.0f, 2.0f, 20.0f, 20.0f }),
                                                  0.5,
                                                  Type::rectangle);

    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (area), Rectangle<float> (1.0f, 1.0f, 15.0f, 20.0f));

    const auto lanes = StyleAnimator::interpolate (var (Array<var> { 0.0, 4.0 }), var (Array<var> { 10.0, 4.0 }), 0.25, Type::array);
    ASSERT_NE (lanes.getArray(), nullptr);
    ASSERT_EQ (lanes.getArray()->size(), 2);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*lanes.getArray())[0]), 2.5);

    // Elements that aren't numbers, and arrays of different sizes, switch halfway
    const auto names = StyleAnimator::interpolate (var (Array<var> { "a" }), var (Array<var> { "b" }), 0.6, Type::array);
    EXPECT_EQ ((*names.getArray())[0].toString(), "b");

    const auto resized = StyleAnimator::interpolate (var (Array<var> { 0.0 }), var (Array<var> { 1.0, 2.0 }), 0.4, Type::array);
    ASSERT_EQ (resized.getArray()->size(), 1);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*resized.getArray())[0]), 0.0);
}

TEST_F (StyleAnimatorTests, SwitchesOtherValuesHalfway)
{
    EXPECT_EQ (StyleAnimator::interpolate ("a", "b", 0.4, Type::string).toString(), "a");
    EXPECT_EQ (StyleAnimator::interpolate ("a", "b", 0.6, Type::string).toString(), "b");
    EXPECT_FALSE (static_cast<bool> (StyleAnimator::interpolate (false, true, 0.4, Type::boolean)));
    EXPECT_TRUE (static_cast<bool> (StyleAnimator::interpolate (false, true, 0.6, Type::boolean)));
    EXPECT_EQ (StyleAnimator::interpolate ("a", 10.0, 0.6, Type::number), var (10.0));
}

TEST_F (StyleAnimatorTests, AdvancesATransitionAndReportsItsEnd)
{
    animator.startTransition (component, Identifier ("glow"), Type::number, 0.0, 10.0, transition (1.0), 0.0);
    EXPECT_TRUE (animator.isAnimating());

    advance (0.5);
    EXPECT_DOUBLE_EQ (glow(), 5.0);

    advance (1.0);
    EXPECT_TRUE (reported.isEmpty());
    EXPECT_TRUE (ended.contains (Identifier ("glow")));
    EXPECT_FALSE (animator.isAnimating());
}

TEST_F (StyleAnimatorTests, ShowsTheStartValueDuringTheTransitionDelay)
{
    animator.startTransition (component, Identifier ("glow"), Type::number, 0.0, 10.0, transition (1.0, 0.5), 0.0);

    advance (0.25);
    EXPECT_DOUBLE_EQ (glow(), 0.0);

    advance (1.0);
    EXPECT_DOUBLE_EQ (glow(), 5.0);
}

TEST_F (StyleAnimatorTests, StartingATransitionReplacesTheRunningOne)
{
    const Identifier glowId ("glow");

    animator.startTransition (component, glowId, Type::number, 0.0, 10.0, transition (1.0), 0.0);
    animator.startTransition (component, glowId, Type::number, 5.0, 0.0, transition (1.0), 0.5);

    ASSERT_NE (animator.findTransitionTarget (component, glowId), nullptr);
    EXPECT_EQ (*animator.findTransitionTarget (component, glowId), var (0.0));
    EXPECT_EQ (animator.getTransitions (component).size(), 1);

    advance (1.0);
    EXPECT_DOUBLE_EQ (glow(), 2.5);

    EXPECT_TRUE (animator.cancelTransition (component, glowId));
    EXPECT_EQ (animator.findTransitionTarget (component, glowId), nullptr);
    EXPECT_FALSE (animator.isAnimating());
}

TEST_F (StyleAnimatorTests, PlaysIterationsInTheirDirection)
{
    startAnimation ("pulse 1s 2 alternate", { { 0.0, 0.0 }, { 1.0, 10.0 } });

    advance (0.25);
    EXPECT_DOUBLE_EQ (glow(), 2.5);

    advance (1.25);
    EXPECT_DOUBLE_EQ (glow(), 7.5);

    advance (2.5);
    EXPECT_TRUE (reported.isEmpty());
    EXPECT_TRUE (ended.contains (Identifier ("glow")));
    EXPECT_FALSE (animator.isAnimating());
}

TEST_F (StyleAnimatorTests, FillsBeforeAndAfterTheActiveTime)
{
    startAnimation ("pulse 1s 1s reverse both", { { 0.0, 0.0 }, { 1.0, 10.0 } });

    advance (0.5);
    EXPECT_DOUBLE_EQ (glow(), 10.0);

    advance (1.75);
    EXPECT_DOUBLE_EQ (glow(), 2.5);

    advance (3.0);
    EXPECT_DOUBLE_EQ (glow(), 0.0);
    EXPECT_FALSE (animator.isAnimating());

    EXPECT_TRUE (animator.stopAnimations (component).contains (Identifier ("glow")));
}

TEST_F (StyleAnimatorTests, ShowsNothingDuringTheDelayWithoutBackwardsFill)
{
    startAnimation ("pulse 1s 1s", { { 0.0, 0.0 }, { 1.0, 10.0 } });

    advance (0.5);
    EXPECT_TRUE (reported.isEmpty());
    EXPECT_TRUE (animator.isAnimating());
}

TEST_F (StyleAnimatorTests, EasesEachKeyframeSegment)
{
    startAnimation ("pulse 1s easeIn", { { 0.0, 0.0 }, { 0.5, 10.0 }, { 1.0, 0.0 } });

    advance (0.25);
    EXPECT_NEAR (glow(), Easing::easeIn().evaluate (0.5f) * 10.0, 1.0e-5);

    advance (0.75);
    EXPECT_NEAR (glow(), 10.0 - Easing::easeIn().evaluate (0.5f) * 10.0, 1.0e-5);
}

TEST_F (StyleAnimatorTests, DropsDeletedComponents)
{
    auto transient = std::make_unique<Component>();
    animator.startTransition (*transient, Identifier ("glow"), Type::number, 0.0, 10.0, transition (1.0), 0.0);
    transient.reset();

    advance (0.5);
    EXPECT_TRUE (reported.isEmpty());
    EXPECT_TRUE (ended.isEmpty());
    EXPECT_FALSE (animator.isAnimating());
}
