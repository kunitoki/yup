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

//==============================================================================
class TabButtonTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());

        bar = std::make_unique<TabBar>();
        bar->setAnimationDuration (0.0);
        bar->setBounds (0.0f, 0.0f, 600.0f, 36.0f);
    }

    void TearDown() override
    {
        bar.reset();
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr oldTheme;
    std::unique_ptr<TabBar> bar;
};

//==============================================================================

TEST_F (TabButtonTests, KeepsItsIdentifierAndBar)
{
    auto& button = bar->addTab ("table", "Table");

    EXPECT_EQ (Identifier ("table"), button.getTabId());
    EXPECT_EQ (bar.get(), &button.getTabBar());
    EXPECT_EQ (String ("Table"), button.getText());
    EXPECT_FALSE (button.isOverflowButton());
    EXPECT_FALSE (button.getWantsKeyboardFocus());
}

TEST_F (TabButtonTests, PreferredLengthGrowsWithAnIconAndACloseButton)
{
    auto& button = bar->addTab ("table", "Table");
    const auto textOnly = button.getPreferredLength();

    button.setIconGlyph (YUP_ICON_TABLE);
    const auto withIcon = button.getPreferredLength();
    EXPECT_GT (withIcon, textOnly);
    EXPECT_TRUE (button.hasIcon());

    button.setClosable (true);
    EXPECT_GT (button.getPreferredLength(), withIcon);
    EXPECT_FALSE (button.getCloseButtonBounds().isEmpty());
}

TEST_F (TabButtonTests, SelectedFontKeepsTheDefaultHeight)
{
    auto& button = bar->addTab ("table", "Table");

    // The width is measured with the selected font, so a smaller one would cut the text of unselected tabs.
    EXPECT_FLOAT_EQ (button.getFont (false).getHeight(), button.getFont (true).getHeight());
}

TEST_F (TabButtonTests, IconOnlyTabsAreShorterThanTabsWithText)
{
    auto& withText = bar->addTab ("table", "Table");
    withText.setIconGlyph (YUP_ICON_TABLE);

    auto& iconOnly = bar->addTab ("board", {});
    iconOnly.setIconGlyph (YUP_ICON_TABLE);

    EXPECT_LT (iconOnly.getPreferredLength(), withText.getPreferredLength());
    EXPECT_TRUE (iconOnly.getTextBounds().isEmpty());
    EXPECT_FALSE (iconOnly.getIconBounds().isEmpty());
}

TEST_F (TabButtonTests, IconImageReplacesTheGlyph)
{
    auto& button = bar->addTab ("table", "Table");
    button.setIconGlyph (YUP_ICON_TABLE);

    button.setIconImage (Image (16, 16, PixelFormat::RGBA));

    EXPECT_TRUE (button.getIconGlyph().isEmpty());
    EXPECT_TRUE (button.getIconImage().isValid());

    button.setIconGlyph (YUP_ICON_TABLE);

    EXPECT_FALSE (button.getIconImage().isValid());
}

TEST_F (TabButtonTests, TextBoundsDoNotOverlapTheIconOrTheCloseButton)
{
    auto& button = bar->addTab ("table", "Table");
    button.setIconGlyph (YUP_ICON_TABLE);
    button.setClosable (true);

    EXPECT_LE (button.getIconBounds().getRight(), button.getTextBounds().getX());
    EXPECT_LE (button.getTextBounds().getRight(), button.getCloseButtonBounds().getX());
    EXPECT_LE (button.getCloseButtonBounds().getRight(), button.getWidth());
}

TEST_F (TabButtonTests, CustomComponentKeepsItsSizeAndIgnoresTheMouse)
{
    auto& button = bar->addTab ("custom", "Ignored");

    auto content = std::make_unique<Component>();
    content->setSize (40.0f, 20.0f);
    auto* raw = content.get();
    button.setCustomComponent (std::move (content));

    EXPECT_EQ (raw, button.getCustomComponent());
    EXPECT_EQ (&button, raw->getParentComponent());
    EXPECT_FALSE (raw->doesWantSelfMouseEvents());
    EXPECT_GE (button.getPreferredLength(), 40.0f);
    EXPECT_FLOAT_EQ (40.0f, raw->getWidth());
    EXPECT_FLOAT_EQ (20.0f, raw->getHeight());
    EXPECT_TRUE (button.getTextBounds().isEmpty());

    button.setCustomComponent (nullptr);

    EXPECT_EQ (nullptr, button.getCustomComponent());
    EXPECT_FALSE (button.getTextBounds().isEmpty());
}

TEST_F (TabButtonTests, SelectedStateFollowsTheBar)
{
    auto& first = bar->addTab ("first", "First");
    auto& second = bar->addTab ("second", "Second");

    EXPECT_TRUE (first.isSelected());
    EXPECT_FALSE (second.isSelected());

    bar->setSelectedTab ("second");

    EXPECT_FALSE (first.isSelected());
    EXPECT_TRUE (second.isSelected());
}

TEST_F (TabButtonTests, CloseButtonHoverFollowsTheMouse)
{
    auto& button = bar->addTab ("table", "Table");
    button.setClosable (true);

    button.mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), button.getCloseButtonBounds().getCenter()));
    EXPECT_TRUE (button.isCloseButtonOver());

    button.mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), { 1.0f, 1.0f }));
    EXPECT_FALSE (button.isCloseButtonOver());
}

TEST_F (TabButtonTests, VerticalTabsUseARowLength)
{
    bar->setBounds (0.0f, 0.0f, 160.0f, 400.0f);
    bar->setOrientation (TabBar::Orientation::vertical);

    auto& shortTab = bar->addTab ("a", "A");
    auto& longTab = bar->addTab ("b", "A much longer tab text");

    EXPECT_FLOAT_EQ (shortTab.getPreferredLength(), longTab.getPreferredLength());
}
