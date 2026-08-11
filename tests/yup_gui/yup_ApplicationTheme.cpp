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
    auto c = Component ("testComponent");

    auto result = ApplicationTheme::findComponentColor (c, Identifier ("unknownColor"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindColorReturnsRegisteredColor)
{
    auto c = Component ("testComponent");

    const Color expected = Color::fromRGBA (255, 128, 0, 255);
    theme->setColor (Identifier ("testColor"), expected);

    auto result = ApplicationTheme::findComponentColor (c, Identifier ("testColor"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), expected);
}

TEST_F (ApplicationThemeTest, SetColorsRegistersMultipleColors)
{
    auto c = Component ("testComponent");

    const Identifier idA ("colorA");
    const Identifier idB ("colorB");
    const Color colorA = Color::fromRGBA (10, 20, 30, 255);
    const Color colorB = Color::fromRGBA (40, 50, 60, 128);

    theme->setColors ({ { idA, colorA }, { idB, colorB } });

    auto resultA = ApplicationTheme::findComponentColor (c, idA);
    auto resultB = ApplicationTheme::findComponentColor (c, idB);

    ASSERT_TRUE (resultA.has_value());
    EXPECT_EQ (resultA.value(), colorA);
    ASSERT_TRUE (resultB.has_value());
    EXPECT_EQ (resultB.value(), colorB);
}

TEST_F (ApplicationThemeTest, SetColorOverwritesExistingColor)
{
    auto c = Component ("testComponent");

    const Identifier id ("myColor");
    const Color first = Color::fromRGBA (1, 2, 3, 255);
    const Color second = Color::fromRGBA (4, 5, 6, 255);

    theme->setColor (id, first);
    theme->setColor (id, second);

    auto result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), second);
}

TEST_F (ApplicationThemeTest, ComponentColorOverridesRegisteredColor)
{
    auto c = Component ("testComponent");

    const Identifier id ("accentColor");
    const Color themeColor = Color::fromRGBA (1, 2, 3, 255);
    const Color componentColor = Color::fromRGBA (4, 5, 6, 255);

    theme->setColor (id, themeColor);
    c.setColor (id, componentColor);

    auto result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), componentColor);

    c.setColor (id, std::nullopt);

    result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), themeColor);
}

TEST_F (ApplicationThemeTest, FindColorUnregisteredIdDoesNotAffectOtherIds)
{
    auto c = Component ("testComponent");

    const Identifier registered ("registered");
    const Identifier unregistered ("unregistered");
    const Color color = Color::fromRGBA (0, 0, 255, 255);

    theme->setColor (registered, color);

    EXPECT_TRUE (ApplicationTheme::findComponentColor (c, registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findComponentColor (c, unregistered).has_value());
}

// =============================================================================

TEST_F (ApplicationThemeTest, FindMetricReturnsNulloptWhenMetricNotRegistered)
{
    auto c = Component ("testComponent");

    auto result = ApplicationTheme::findComponentMetric (c, Identifier ("unknownMetric"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindMetricReturnsRegisteredValue)
{
    auto c = Component ("testComponent");

    theme->setMetric (Identifier ("cornerRadius"), 8.0f);

    auto result = ApplicationTheme::findComponentMetric (c, Identifier ("cornerRadius"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 8.0f);
}

TEST_F (ApplicationThemeTest, SetMetricOverwritesExistingValue)
{
    auto c = Component ("testComponent");

    const Identifier id ("borderWidth");

    theme->setMetric (id, 1.0f);
    theme->setMetric (id, 3.5f);

    auto result = ApplicationTheme::findComponentMetric (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 3.5f);
}

TEST_F (ApplicationThemeTest, SetMetricsRegistersMultipleValues)
{
    auto c = Component ("testComponent");

    const Identifier idA ("smallSpacing");
    const Identifier idB ("largeSpacing");

    theme->setMetrics ({ { idA, 4.0f }, { idB, 12.0f } });

    auto resultA = ApplicationTheme::findComponentMetric (c, idA);
    auto resultB = ApplicationTheme::findComponentMetric (c, idB);

    ASSERT_TRUE (resultA.has_value());
    EXPECT_FLOAT_EQ (resultA.value(), 4.0f);
    ASSERT_TRUE (resultB.has_value());
    EXPECT_FLOAT_EQ (resultB.value(), 12.0f);
}

TEST_F (ApplicationThemeTest, ComponentMetricOverridesRegisteredMetric)
{
    auto c = Component ("testComponent");

    const Identifier id ("cornerRadius");

    theme->setMetric (id, 8.0f);
    c.setMetric (id, 12.0f);

    auto result = ApplicationTheme::findComponentMetric (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 12.0f);

    c.setMetric (id, std::nullopt);

    result = ApplicationTheme::findComponentMetric (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (result.value(), 8.0f);
}

TEST_F (ApplicationThemeTest, FindMetricUnregisteredIdDoesNotAffectOtherIds)
{
    auto c = Component ("testComponent");

    const Identifier registered ("spacing");
    const Identifier unregistered ("padding");

    theme->setMetric (registered, 4.0f);

    EXPECT_TRUE (ApplicationTheme::findComponentMetric (c, registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findComponentMetric (c, unregistered).has_value());
}

TEST_F (ApplicationThemeTest, SetMetricAcceptsZeroAndNegativeValues)
{
    auto c = Component ("testComponent");

    theme->setMetric (Identifier ("zeroMetric"), 0.0f);
    theme->setMetric (Identifier ("negativeMetric"), -2.5f);

    auto zero = ApplicationTheme::findComponentMetric (c, Identifier ("zeroMetric"));
    auto negative = ApplicationTheme::findComponentMetric (c, Identifier ("negativeMetric"));

    ASSERT_TRUE (zero.has_value());
    EXPECT_FLOAT_EQ (zero.value(), 0.0f);

    ASSERT_TRUE (negative.has_value());
    EXPECT_FLOAT_EQ (negative.value(), -2.5f);
}

// =============================================================================

TEST_F (ApplicationThemeTest, SetGlobalThemeRoundTrip)
{
    auto themePtr = ApplicationTheme::getGlobalTheme();
    ASSERT_NE (nullptr, themePtr.get());

    auto newTheme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (newTheme);

    auto retrieved = ApplicationTheme::getGlobalTheme();
    EXPECT_EQ (newTheme, retrieved.get());

    ApplicationTheme::setGlobalTheme (themePtr.get());
}

TEST_F (ApplicationThemeTest, GetGlobalThemeReturnsNonNull)
{
    auto global = ApplicationTheme::getGlobalTheme();
    EXPECT_NE (nullptr, global.get());
}

TEST_F (ApplicationThemeTest, SetDefaultFont)
{
    auto originalFont = theme->getDefaultFont();

    Font newFont;
    theme->setDefaultFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultFont());
}

TEST_F (ApplicationThemeTest, GetDefaultFontReturnsValid)
{
    auto font = theme->getDefaultFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}

TEST_F (ApplicationThemeTest, SetDefaultIconFont)
{
    auto originalFont = theme->getDefaultIconFont();

    Font newFont;
    theme->setDefaultIconFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultIconFont());
}

TEST_F (ApplicationThemeTest, GetDefaultIconFontReturnsValid)
{
    auto font = theme->getDefaultIconFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}

TEST_F (ApplicationThemeTest, FindMetricInstanceMethodReturnsRegisteredValue)
{
    Component c ("testComponent");

    theme->setMetric (Identifier ("myMetric"), 15.0f);

    auto result = theme->findMetric (c, Identifier ("myMetric"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (15.0f, result.value());
}

TEST_F (ApplicationThemeTest, FindMetricInstanceMethodReturnsNulloptForUnknown)
{
    Component c ("testComponent");

    auto result = theme->findMetric (c, Identifier ("unknownMetric"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindMetricInstanceMethodRespectsComponentOverride)
{
    Component c ("testComponent");

    theme->setMetric (Identifier ("radius"), 10.0f);
    c.setMetric (Identifier ("radius"), 25.0f);

    auto result = theme->findMetric (c, Identifier ("radius"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (25.0f, result.value());
}

TEST_F (ApplicationThemeTest, SetComponentStyleRegistersStyle)
{
    ComponentStyle::Ptr style = new MockComponentStyle();
    theme->setComponentStyle<ProgressBar> (style);

    ProgressBar bar;
    auto found = ApplicationTheme::findComponentStyle (bar);
    EXPECT_EQ (style.get(), found.get());
}

#if ! YUP_DEBUG
TEST_F (ApplicationThemeTest, FindComponentStyleReturnsNullWhenNotRegistered)
{
    Slider slider (Slider::LinearHorizontal);
    auto found = ApplicationTheme::findComponentStyle (slider);
    EXPECT_EQ (nullptr, found.get());
}
#endif

TEST_F (ApplicationThemeTest, FindComponentStylePrefersComponentInstanceStyle)
{
    ComponentStyle::Ptr themeStyle = new MockComponentStyle();
    theme->setComponentStyle<Label> (themeStyle);

    Label label ("test");

    auto found = ApplicationTheme::findComponentStyle (label);
    EXPECT_EQ (themeStyle.get(), found.get());

    ComponentStyle::Ptr instanceStyle = new MockComponentStyle();
    label.setStyle (instanceStyle);

    found = ApplicationTheme::findComponentStyle (label);
    EXPECT_EQ (instanceStyle.get(), found.get());
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethod)
{
    Component c ("testComponent");

    const Color expected = Color::fromRGBA (10, 20, 30, 255);
    theme->setColor (Identifier ("bg"), expected);

    auto result = theme->findColor (c, Identifier ("bg"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), expected);
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethodReturnsNulloptForUnknown)
{
    Component c ("testComponent");

    auto result = theme->findColor (c, Identifier ("unknown"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethodRespectsComponentOverride)
{
    Component c ("testComponent");

    const Color themeColor = Color::fromRGBA (1, 2, 3, 255);
    const Color compColor = Color::fromRGBA (4, 5, 6, 255);
    theme->setColor (Identifier ("fg"), themeColor);
    c.setColor (Identifier ("fg"), compColor);

    auto result = theme->findColor (c, Identifier ("fg"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), compColor);
}

TEST_F (ApplicationThemeTest, SetDefaultMonospaceFont)
{
    auto originalFont = theme->getDefaultMonospaceFont();

    Font newFont;
    theme->setDefaultMonospaceFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultMonospaceFont());
}

TEST_F (ApplicationThemeTest, GetDefaultMonospaceFontReturnsValid)
{
    auto font = theme->getDefaultMonospaceFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}
