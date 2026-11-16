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

class StyleStateTests : public ::testing::Test
{
protected:
};

TEST_F (StyleStateTests, AnimatedValuesMaskComputedValuesUntilCleared)
{
    StyleState state;
    const Identifier key ("cornerRadius");

    EXPECT_EQ (state.getStyleValue (key), nullptr);

    EXPECT_TRUE (state.setComputedValue (key, 8.0));
    EXPECT_FALSE (state.setComputedValue (key, 8.0));
    ASSERT_NE (state.getStyleValue (key), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (*state.getStyleValue (key)), 8.0);

    EXPECT_TRUE (state.setAnimatedValue (key, 4.0));
    ASSERT_NE (state.getStyleValue (key), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (*state.getStyleValue (key)), 4.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (*state.getComputedValue (key)), 8.0);

    EXPECT_TRUE (state.removeAnimatedValue (key));
    ASSERT_NE (state.getStyleValue (key), nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (*state.getStyleValue (key)), 8.0);

    EXPECT_TRUE (state.clearComputedValues());
    EXPECT_EQ (state.getStyleValue (key), nullptr);
}

TEST_F (StyleStateTests, StoresPaletteTransformFilterShadowsAndGlowsWithChangeDetection)
{
    StyleState state;

    ThemePalette palette;
    palette.setColor (ThemePalette::Role::accent, Colors::cyan);
    EXPECT_TRUE (state.setStylePalette (palette));
    EXPECT_FALSE (state.setStylePalette (palette));
    ASSERT_NE (state.getStylePalette(), nullptr);
    EXPECT_EQ (state.getStylePalette()->getColor (ThemePalette::Role::accent), Colors::cyan);
    EXPECT_TRUE (state.setStylePalette (std::nullopt));
    EXPECT_EQ (state.getStylePalette(), nullptr);

    EXPECT_FALSE (state.hasStyleTransform());
    const auto shifted = AffineTransform::translation (12.0f, -4.0f);
    EXPECT_TRUE (state.setStyleTransform (shifted));
    EXPECT_FALSE (state.setStyleTransform (shifted));
    EXPECT_TRUE (state.hasStyleTransform());
    EXPECT_EQ (state.getStyleTransform(), shifted);

    const std::vector<FilterEffect::Operation> filterOps {
        { .type = FilterEffect::Type::blur, .amount = 4.0f }
    };
    EXPECT_TRUE (state.setFilterOperations (filterOps));
    const auto firstInstance = state.getFilterEffect();
    ASSERT_NE (firstInstance, nullptr);
    EXPECT_FALSE (state.setFilterOperations (filterOps));
    EXPECT_EQ (state.getFilterEffect(), firstInstance);
    EXPECT_TRUE (state.setFilterOperations ({}));
    EXPECT_EQ (state.getFilterEffect(), nullptr);

    const std::vector<StyleState::Shadow> shadows {
        { .offset = { 0.0f, 4.0f }, .blur = 8.0f, .spread = 0.0f, .color = Color (0x80000000) }
    };
    const std::vector<StyleState::Shadow> glows {
        { .offset = { 0.0f, 0.0f }, .blur = 12.0f, .spread = 2.0f, .color = Color (0x6000ffff) }
    };
    EXPECT_TRUE (state.setShadows (shadows));
    EXPECT_FALSE (state.setShadows (shadows));
    EXPECT_EQ (state.getShadows().size(), 1u);

    EXPECT_TRUE (state.setGlows (glows));
    EXPECT_FALSE (state.setGlows (glows));
    EXPECT_EQ (state.getGlows().size(), 1u);
}
