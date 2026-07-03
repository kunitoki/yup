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
// KeyPath
// =============================================================================

class KeyPathTests : public ::testing::Test
{
};

// ---- size -------------------------------------------------------------------

TEST_F (KeyPathTests, Size_SingleComponent)
{
    KeyPath kp ("Layer");
    EXPECT_EQ (kp.size(), 1u);
}

TEST_F (KeyPathTests, Size_TwoComponents)
{
    KeyPath kp ("Layer.Fill");
    EXPECT_EQ (kp.size(), 2u);
}

TEST_F (KeyPathTests, Size_ThreeComponents)
{
    KeyPath kp ("Layer.Group.Fill");
    EXPECT_EQ (kp.size(), 3u);
}

TEST_F (KeyPathTests, Size_SingleWildcard)
{
    KeyPath kp ("*");
    EXPECT_EQ (kp.size(), 1u);
}

TEST_F (KeyPathTests, Size_Globstar)
{
    KeyPath kp ("**");
    EXPECT_EQ (kp.size(), 1u);
}

// ---- matchesComponent -------------------------------------------------------

TEST_F (KeyPathTests, MatchesComponent_ExactMatchAtDepthZero)
{
    KeyPath kp ("Layer.Fill");
    EXPECT_TRUE (kp.matchesComponent ("Layer", 0));
    EXPECT_FALSE (kp.matchesComponent ("Other", 0));
}

TEST_F (KeyPathTests, MatchesComponent_ExactMatchAtDepthOne)
{
    KeyPath kp ("Layer.Fill");
    EXPECT_TRUE (kp.matchesComponent ("Fill", 1));
    EXPECT_FALSE (kp.matchesComponent ("Layer", 1));
}

TEST_F (KeyPathTests, MatchesComponent_SingleWildcardMatchesAnything)
{
    KeyPath kp ("*");
    EXPECT_TRUE (kp.matchesComponent ("Layer", 0));
    EXPECT_TRUE (kp.matchesComponent ("anything", 0));
    EXPECT_TRUE (kp.matchesComponent ("", 0));
}

TEST_F (KeyPathTests, MatchesComponent_GlobstarMatchesAnything)
{
    KeyPath kp ("**");
    EXPECT_TRUE (kp.matchesComponent ("Layer", 0));
    EXPECT_TRUE (kp.matchesComponent ("anything", 0));
}

// ---- fullyResolvesTo --------------------------------------------------------

TEST_F (KeyPathTests, FullyResolvesTo_LastComponentMatches)
{
    KeyPath kp ("Layer.Fill");
    EXPECT_TRUE (kp.fullyResolvesTo ("Fill", 1));
    EXPECT_FALSE (kp.fullyResolvesTo ("Fill", 0)); // Not at last depth
    EXPECT_FALSE (kp.fullyResolvesTo ("Layer", 1));
}

TEST_F (KeyPathTests, FullyResolvesTo_SingleComponentKey)
{
    KeyPath kp ("Layer");
    EXPECT_TRUE (kp.fullyResolvesTo ("Layer", 0));
    EXPECT_FALSE (kp.fullyResolvesTo ("Other", 0));
}

TEST_F (KeyPathTests, FullyResolvesTo_WildcardAtLastPosition)
{
    KeyPath kp ("Layer.*");
    EXPECT_TRUE (kp.fullyResolvesTo ("anything", 1));
    EXPECT_TRUE (kp.fullyResolvesTo ("Fill", 1));
}

// ---- propagate --------------------------------------------------------------

TEST_F (KeyPathTests, Propagate_GlobstarPropagatesAtAnyDepth)
{
    KeyPath kp ("**");
    EXPECT_TRUE (kp.propagate ("Layer", 0));
    EXPECT_TRUE (kp.propagate ("anything", 0));
}

TEST_F (KeyPathTests, Propagate_NonGlobstarDoesNotPropagate)
{
    // propagate() only returns true for "**" globstar components,
    // not for literal key matches
    KeyPath kp ("Layer.Fill");
    EXPECT_FALSE (kp.propagate ("Layer", 0));
    EXPECT_FALSE (kp.propagate ("Other", 0));
}

// ---- nextDepth ---------------------------------------------------------------

TEST_F (KeyPathTests, NextDepth_NonGlobstarIncrementsDepth)
{
    KeyPath kp ("Layer.Fill");
    EXPECT_EQ (kp.nextDepth ("Layer", 0), 1u);
    EXPECT_EQ (kp.nextDepth ("Fill", 1), 2u);
}

TEST_F (KeyPathTests, NextDepth_GlobstarStaysAtSameDepth)
{
    KeyPath kp ("**");
    // Globstar keeps depth stable so it can consume multiple levels
    EXPECT_EQ (kp.nextDepth ("Layer", 0), 0u);
}

// =============================================================================
// PropertyOverrideSet
// =============================================================================

class PropertyOverrideSetTests : public ::testing::Test
{
};

TEST_F (PropertyOverrideSetTests, HasOverride_FalseByDefault)
{
    PropertyOverrideSet set;
    EXPECT_FALSE (set.hasOverride (AnimationPropertyID::FillColor));
    EXPECT_FALSE (set.hasOverride (AnimationPropertyID::StrokeWidth));
}

TEST_F (PropertyOverrideSetTests, SetFloatOverride_HasOverrideReturnsTrue)
{
    PropertyOverrideSet set;
    set.setFloatOverride (AnimationPropertyID::StrokeWidth, [] (float) -> std::optional<float>
    {
        return 3.0f;
    });
    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::StrokeWidth));
    EXPECT_FALSE (set.hasOverride (AnimationPropertyID::FillOpacity));
}

TEST_F (PropertyOverrideSetTests, SetColorOverride_HasOverrideReturnsTrue)
{
    PropertyOverrideSet set;
    set.setColorOverride (AnimationPropertyID::FillColor, [] (float) -> std::optional<Color>
    {
        return Color (0xffff0000);
    });
    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::FillColor));
}

TEST_F (PropertyOverrideSetTests, EvaluateFloat_ReturnsCallbackValue)
{
    PropertyOverrideSet set;
    set.setFloatOverride (AnimationPropertyID::StrokeWidth, [] (float) -> std::optional<float>
    {
        return 7.5f;
    });
    EXPECT_FLOAT_EQ (set.evaluateFloat (AnimationPropertyID::StrokeWidth, 0.0f, 1.0f), 7.5f);
}

TEST_F (PropertyOverrideSetTests, EvaluateFloat_ReturnsFallbackWhenNoOverride)
{
    PropertyOverrideSet set;
    EXPECT_FLOAT_EQ (set.evaluateFloat (AnimationPropertyID::StrokeWidth, 0.0f, 2.5f), 2.5f);
}

TEST_F (PropertyOverrideSetTests, EvaluateFloat_ReturnsFallbackWhenCallbackReturnsNullopt)
{
    PropertyOverrideSet set;
    set.setFloatOverride (AnimationPropertyID::StrokeWidth, [] (float) -> std::optional<float>
    {
        return std::nullopt;
    });
    EXPECT_FLOAT_EQ (set.evaluateFloat (AnimationPropertyID::StrokeWidth, 0.0f, 4.0f), 4.0f);
}

TEST_F (PropertyOverrideSetTests, EvaluateColor_ReturnsCallbackValue)
{
    const Color expected (0xffaabbcc);
    PropertyOverrideSet set;
    set.setColorOverride (AnimationPropertyID::FillColor, [expected] (float) -> std::optional<Color>
    {
        return expected;
    });
    EXPECT_EQ (set.evaluateColor (AnimationPropertyID::FillColor, 0.0f, Color()), expected);
}

TEST_F (PropertyOverrideSetTests, EvaluateColor_ReturnsFallbackWhenNoOverride)
{
    const Color fallback (0xff00ff00);
    PropertyOverrideSet set;
    EXPECT_EQ (set.evaluateColor (AnimationPropertyID::FillColor, 0.0f, fallback), fallback);
}

TEST_F (PropertyOverrideSetTests, EvaluateFloat_PassesFrameNoToCallback)
{
    PropertyOverrideSet set;
    float capturedFrame = -1.0f;
    set.setFloatOverride (AnimationPropertyID::FillOpacity, [&capturedFrame] (float fn) -> std::optional<float>
    {
        capturedFrame = fn;
        return 1.0f;
    });
    EXPECT_TRUE (set.evaluateFloat (AnimationPropertyID::FillOpacity, 42.0f, 0.0f));
    EXPECT_FLOAT_EQ (capturedFrame, 42.0f);
}

TEST_F (PropertyOverrideSetTests, SetPointOverride_HasOverrideReturnsTrue)
{
    PropertyOverrideSet set;
    set.setPointOverride (AnimationPropertyID::TrPosition, [] (float) -> std::optional<Point<float>>
    {
        return Point<float> (10.0f, 20.0f);
    });
    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::TrPosition));
}

TEST_F (PropertyOverrideSetTests, EvaluatePoint_ReturnsCallbackValue)
{
    const Point<float> expected (5.0f, 15.0f);
    PropertyOverrideSet set;
    set.setPointOverride (AnimationPropertyID::TrPosition, [expected] (float) -> std::optional<Point<float>>
    {
        return expected;
    });
    const auto result = set.evaluatePoint (AnimationPropertyID::TrPosition, 0.0f, {});
    EXPECT_FLOAT_EQ (result.getX(), expected.getX());
    EXPECT_FLOAT_EQ (result.getY(), expected.getY());
}

TEST_F (PropertyOverrideSetTests, SetSizeOverride_HasOverrideReturnsTrue)
{
    PropertyOverrideSet set;
    set.setSizeOverride (AnimationPropertyID::TrScale, [] (float) -> std::optional<Size<float>>
    {
        return Size<float> (2.0f, 2.0f);
    });
    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::TrScale));
}

TEST_F (PropertyOverrideSetTests, EvaluateSize_ReturnsCallbackValue)
{
    const Size<float> expected (3.0f, 4.0f);
    PropertyOverrideSet set;
    set.setSizeOverride (AnimationPropertyID::TrScale, [expected] (float) -> std::optional<Size<float>>
    {
        return expected;
    });
    const auto result = set.evaluateSize (AnimationPropertyID::TrScale, 0.0f, {});
    EXPECT_FLOAT_EQ (result.getWidth(), expected.getWidth());
    EXPECT_FLOAT_EQ (result.getHeight(), expected.getHeight());
}

TEST_F (PropertyOverrideSetTests, MultipleOverrides_EachTrackedIndependently)
{
    PropertyOverrideSet set;
    set.setFloatOverride (AnimationPropertyID::StrokeWidth, [] (float) -> std::optional<float>
    {
        return 5.0f;
    });
    set.setColorOverride (AnimationPropertyID::FillColor, [] (float) -> std::optional<Color>
    {
        return Color (0xff000000);
    });

    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::StrokeWidth));
    EXPECT_TRUE (set.hasOverride (AnimationPropertyID::FillColor));
    EXPECT_FALSE (set.hasOverride (AnimationPropertyID::StrokeColor));
}
