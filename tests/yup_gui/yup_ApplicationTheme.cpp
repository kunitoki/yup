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

// =============================================================================

class ApplicationThemeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();

        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

// =============================================================================

TEST_F (ApplicationThemeTest, FindColorReturnsNulloptWhenColorNotRegistered)
{
    auto result = ApplicationTheme::findColor (Identifier ("unknownColor"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindColorReturnsRegisteredColor)
{
    const Color expected = Color::fromRGBA (255, 128, 0, 255);
    theme->setColor (Identifier ("testColor"), expected);

    auto result = ApplicationTheme::findColor (Identifier ("testColor"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), expected);
}

TEST_F (ApplicationThemeTest, SetColorsRegistersMultipleColors)
{
    const Identifier idA ("colorA");
    const Identifier idB ("colorB");
    const Color colorA = Color::fromRGBA (10, 20, 30, 255);
    const Color colorB = Color::fromRGBA (40, 50, 60, 128);

    theme->setColors ({ { idA, colorA }, { idB, colorB } });

    auto resultA = ApplicationTheme::findColor (idA);
    auto resultB = ApplicationTheme::findColor (idB);

    ASSERT_TRUE (resultA.has_value());
    EXPECT_EQ (resultA.value(), colorA);
    ASSERT_TRUE (resultB.has_value());
    EXPECT_EQ (resultB.value(), colorB);
}

TEST_F (ApplicationThemeTest, SetColorOverwritesExistingColor)
{
    const Identifier id ("myColor");
    const Color first = Color::fromRGBA (1, 2, 3, 255);
    const Color second = Color::fromRGBA (4, 5, 6, 255);

    theme->setColor (id, first);
    theme->setColor (id, second);

    auto result = ApplicationTheme::findColor (id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), second);
}

TEST_F (ApplicationThemeTest, FindColorUnregisteredIdDoesNotAffectOtherIds)
{
    const Identifier registered ("registered");
    const Identifier unregistered ("unregistered");
    const Color color = Color::fromRGBA (0, 0, 255, 255);

    theme->setColor (registered, color);

    EXPECT_TRUE (ApplicationTheme::findColor (registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findColor (unregistered).has_value());
}

// =============================================================================

TEST_F (ApplicationThemeTest, FindMetricReturnsNulloptWhenMetricNotRegistered)
{
    auto result = ApplicationTheme::findMetric (Identifier ("unknownMetric"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindMetricReturnsRegisteredValue)
{
    theme->setMetric (Identifier ("cornerRadius"), 8.0f);

    auto result = ApplicationTheme::findMetric (Identifier ("cornerRadius"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 8.0f);
}

TEST_F (ApplicationThemeTest, SetMetricOverwritesExistingValue)
{
    const Identifier id ("borderWidth");

    theme->setMetric (id, 1.0f);
    theme->setMetric (id, 3.5f);

    auto result = ApplicationTheme::findMetric (id);
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 3.5f);
}

TEST_F (ApplicationThemeTest, FindMetricUnregisteredIdDoesNotAffectOtherIds)
{
    const Identifier registered ("spacing");
    const Identifier unregistered ("padding");

    theme->setMetric (registered, 4.0f);

    EXPECT_TRUE (ApplicationTheme::findMetric (registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findMetric (unregistered).has_value());
}

TEST_F (ApplicationThemeTest, SetMetricAcceptsZeroAndNegativeValues)
{
    theme->setMetric (Identifier ("zeroMetric"), 0.0f);
    theme->setMetric (Identifier ("negativeMetric"), -2.5f);

    auto zero = ApplicationTheme::findMetric (Identifier ("zeroMetric"));
    auto negative = ApplicationTheme::findMetric (Identifier ("negativeMetric"));

    ASSERT_TRUE (zero.has_value());
    EXPECT_FLOAT_EQ (zero.value(), 0.0f);

    ASSERT_TRUE (negative.has_value());
    EXPECT_FLOAT_EQ (negative.value(), -2.5f);
}
