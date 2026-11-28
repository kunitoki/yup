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

    /** Gives the theme a font with a weight axis, which the font of a selected tab is another font of. */
    void useVariableFont()
    {
        const auto font = Font::loadFontFromFile (File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts/Linefont-VariableFont_wdth,wght.ttf"));
        ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();

        ApplicationTheme::getGlobalTheme()->setDefaultFont (font.getValue().withHeight (14.0f));
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
    EXPECT_FALSE (button.getCloseBounds().isEmpty());
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
    EXPECT_LE (button.getTextBounds().getRight(), button.getCloseBounds().getX());
    EXPECT_LE (button.getCloseBounds().getRight(), button.getWidth());
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

    button.mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), button.getCloseBounds().getCenter()));
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

TEST_F (TabButtonTests, PreferredLengthFollowsTheTextAndTheFontOfTheTheme)
{
    ASSERT_NO_FATAL_FAILURE (useVariableFont());

    auto& button = bar->addTab ("table", "Table");
    const auto length = button.getPreferredLength();

    button.setText ("A much longer table");
    EXPECT_GT (button.getPreferredLength(), length);

    button.setText ("Table");
    EXPECT_FLOAT_EQ (button.getPreferredLength(), length);

    // A larger font of the theme, which nothing tells the tab about
    auto theme = ApplicationTheme::getGlobalTheme();
    theme->setDefaultFont (theme->getDefaultFont().withHeight (28.0f));
    EXPECT_GT (button.getPreferredLength(), length);
}

TEST_F (TabButtonTests, TheHeavierFontOfASelectedTabIsMadeOnceForAFontOfTheTheme)
{
    ASSERT_NO_FATAL_FAILURE (useVariableFont());

    auto& button = bar->addTab ("table", "Table");
    const auto selectedFont = button.getFont (true);

    // Another font than the one of the theme, and the same one each time it is asked for
    EXPECT_NE (selectedFont, button.getFont (false));
    EXPECT_EQ (selectedFont, button.getFont (true));

    // Another font of the theme has a heavier font of its own
    auto theme = ApplicationTheme::getGlobalTheme();
    theme->setDefaultFont (theme->getDefaultFont().withHeight (20.0f));

    EXPECT_NE (selectedFont, button.getFont (true));
    EXPECT_NE (button.getFont (true), button.getFont (false));
    EXPECT_FLOAT_EQ (button.getFont (true).getHeight(), 20.0f);
}

TEST_F (TabButtonTests, ThePainterLaysTheTextOutAgainOnlyForAnotherTextOtherBoundsOrAnotherSelectedState)
{
    ASSERT_NO_FATAL_FAILURE (useVariableFont());

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (600, 48);
    Graphics g (*context, *renderer);

    auto& first = bar->addTab ("first", "First");
    bar->addTab ("second", "Second");
    ASSERT_TRUE (first.isSelected());

    // The tab lays nothing out: whoever paints it does
    auto& kept = first.getTextLayoutCache();
    EXPECT_TRUE (kept.styledText.isEmpty());

    first.paint (g);
    EXPECT_EQ (kept.text, "First");
    EXPECT_EQ (kept.font, first.getFont (true));
    EXPECT_FALSE (kept.styledText.isEmpty());
    EXPECT_EQ (kept.styledText.getMaxSize(), first.getTextBounds().getSize());

    // What is kept is what is painted again: emptied here, it stays empty until the painter has to lay it out
    const auto isLaidOutAgain = [&] (const std::function<void()>& change)
    {
        kept.styledText.startUpdate().clear();
        change();
        first.paint (g);
        return ! kept.styledText.isEmpty();
    };

    // The tab painted as it is, then moved behind a tab added before it
    EXPECT_FALSE (isLaidOutAgain ([] {}));
    EXPECT_FALSE (isLaidOutAgain ([this] { bar->addTab ("board", "Board", 0); }));
    EXPECT_GT (first.getX(), bar->getTabButton ("board")->getX());

    // The lighter font of a tab that is no longer the selected one
    EXPECT_TRUE (isLaidOutAgain ([this] { bar->setSelectedTab ("second"); }));
    EXPECT_EQ (kept.font, first.getFont (false));

    EXPECT_TRUE (isLaidOutAgain ([&first] { first.setText ("A longer first"); }));
    EXPECT_EQ (kept.text, "A longer first");

    // Other bounds of the text: another height of the bar
    EXPECT_TRUE (isLaidOutAgain ([this] { bar->setBounds (0.0f, 0.0f, 600.0f, 48.0f); }));
    EXPECT_EQ (kept.styledText.getMaxSize(), first.getTextBounds().getSize());
}

TEST_F (TabButtonTests, HasIconTellsAGlyphAsItTellsAnImage)
{
    auto& button = bar->addTab ("table", "Table");

    const auto hasIcon = [&button]
    {
        return static_cast<bool> (button.getProperty (Identifier ("hasIcon")));
    };

    EXPECT_FALSE (hasIcon());

    button.setIconGlyph (YUP_ICON_TABLE);
    EXPECT_TRUE (hasIcon());

    button.setIconImage (Image (16, 16, PixelFormat::RGBA));
    EXPECT_TRUE (hasIcon());

    button.setIconGlyph (String());
    EXPECT_FALSE (hasIcon());
}

//==============================================================================
class TabButtonGpuTests : public TabButtonTests
{
protected:
    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});

        if (gpuContext != nullptr && GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        TabButtonTests::SetUp();

        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static inline std::unique_ptr<GraphicsContext> gpuContext;
};

TEST_F (TabButtonGpuTests, TheIconPartPaintsAGlyphAsItPaintsAnImage)
{
    auto& button = bar->addTab ("table", {});
    button.setIconGlyph (YUP_ICON_TABLE);
    ASSERT_FALSE (button.getIconBounds().isEmpty());

    // A style painting the icon and nothing else, so that what is painted is the part
    button.setStyle (ComponentStyle::createStyle<TabButton> ([] (Graphics& g, const ApplicationTheme&, const TabButton& tab)
    {
        g.setFillColor (Colors::red);
        tab.paintPart (g, TabButton::Style::iconPartId);
    }));

    const auto image = button.snapshotToImage (*gpuContext);
    ASSERT_TRUE (image.isValid());

    int numPainted = 0;

    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            if (image.getPixelColor (x, y) != image.getPixelColor (0, 0))
                ++numPainted;

    EXPECT_GT (numPainted, 0);

    button.setStyle (nullptr);
}
