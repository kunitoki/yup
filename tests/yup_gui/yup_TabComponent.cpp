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
class TabComponentTests : public ::testing::Test
{
protected:
    /** A page that reports when it is deleted. */
    class TrackedPage : public Component
    {
    public:
        explicit TrackedPage (bool& deleted)
            : deleted (deleted)
        {
        }

        ~TrackedPage() override { deleted = true; }

    private:
        bool& deleted;
    };

    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());

        tabs = std::make_unique<TabComponent>();
        tabs->getTabBar().setAnimationDuration (0.0);
        tabs->setBounds (0.0f, 0.0f, 400.0f, 300.0f);
    }

    void TearDown() override
    {
        tabs.reset();
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr oldTheme;
    std::unique_ptr<TabComponent> tabs;
    Component firstPage;
    Component secondPage;
};

//==============================================================================

TEST_F (TabComponentTests, OnlyTheSelectedPageIsVisible)
{
    tabs->addTab ("first", "First", firstPage);
    tabs->addTab ("second", "Second", secondPage);

    EXPECT_TRUE (firstPage.isVisible());
    EXPECT_FALSE (secondPage.isVisible());
    EXPECT_EQ (tabs.get(), secondPage.getParentComponent());

    tabs->getTabBar().setSelectedTab ("second");

    EXPECT_FALSE (firstPage.isVisible());
    EXPECT_TRUE (secondPage.isVisible());
}

TEST_F (TabComponentTests, PagesFollowSelectionWithoutNotification)
{
    tabs->addTab ("first", "First", firstPage);
    tabs->addTab ("second", "Second", secondPage);

    tabs->getTabBar().setSelectedTab ("second", dontSendNotification);

    EXPECT_FALSE (firstPage.isVisible());
    EXPECT_TRUE (secondPage.isVisible());
}

TEST_F (TabComponentTests, GetTabContentFindsThePage)
{
    tabs->addTab ("first", "First", firstPage);

    EXPECT_EQ (&firstPage, tabs->getTabContent ("first"));
    EXPECT_EQ (nullptr, tabs->getTabContent ("missing"));
}

TEST_F (TabComponentTests, OwnedPageIsDeletedWithItsTab)
{
    bool deleted = false;
    tabs->addTab ("owned", "Owned", std::make_unique<TrackedPage> (deleted));

    tabs->removeTab ("owned");

    EXPECT_TRUE (deleted);
    EXPECT_EQ (nullptr, tabs->getTabContent ("owned"));
    EXPECT_EQ (0, tabs->getTabBar().getNumTabs());
}

TEST_F (TabComponentTests, UnownedPageSurvivesItsTab)
{
    tabs->addTab ("first", "First", firstPage);

    tabs->removeTab ("first");

    EXPECT_EQ (nullptr, firstPage.getParentComponent());
    EXPECT_EQ (nullptr, tabs->getTabContent ("first"));
}

TEST_F (TabComponentTests, RemovingThroughTheBarDropsThePage)
{
    bool deleted = false;
    tabs->addTab ("first", "First", firstPage);
    tabs->addTab ("owned", "Owned", std::make_unique<TrackedPage> (deleted));

    tabs->getTabBar().removeTab ("owned");

    EXPECT_TRUE (deleted);
    EXPECT_EQ (nullptr, tabs->getTabContent ("owned"));
}

TEST_F (TabComponentTests, RemovingTheSelectedTabShowsTheNextPage)
{
    tabs->addTab ("first", "First", firstPage);
    tabs->addTab ("second", "Second", secondPage);

    tabs->removeTab ("first");

    EXPECT_TRUE (secondPage.isVisible());
}

TEST_F (TabComponentTests, PlacementSetsTheBarBoundsAndOrientation)
{
    tabs->addTab ("first", "First", firstPage);
    tabs->setTabBarThickness (40.0f);

    auto& bar = tabs->getTabBar();

    tabs->setTabBarPlacement (TabComponent::Placement::top);
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 400.0f, 40.0f), bar.getBounds());
    EXPECT_EQ (Rectangle<float> (0.0f, 40.0f, 400.0f, 260.0f), firstPage.getBounds());
    EXPECT_EQ (TabBar::Orientation::horizontal, bar.getOrientation());
    EXPECT_FALSE (bar.isFlipped());

    tabs->setTabBarPlacement (TabComponent::Placement::bottom);
    EXPECT_EQ (Rectangle<float> (0.0f, 260.0f, 400.0f, 40.0f), bar.getBounds());
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 400.0f, 260.0f), firstPage.getBounds());
    EXPECT_TRUE (bar.isFlipped());

    tabs->setTabBarPlacement (TabComponent::Placement::left);
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 40.0f, 300.0f), bar.getBounds());
    EXPECT_EQ (Rectangle<float> (40.0f, 0.0f, 360.0f, 300.0f), firstPage.getBounds());
    EXPECT_EQ (TabBar::Orientation::vertical, bar.getOrientation());
    EXPECT_FALSE (bar.isFlipped());

    tabs->setTabBarPlacement (TabComponent::Placement::right);
    EXPECT_EQ (Rectangle<float> (360.0f, 0.0f, 40.0f, 300.0f), bar.getBounds());
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 360.0f, 300.0f), firstPage.getBounds());
    EXPECT_TRUE (bar.isFlipped());
}

TEST_F (TabComponentTests, PagesAddedLaterGetTheContentBounds)
{
    tabs->setTabBarPlacement (TabComponent::Placement::left);
    tabs->addTab ("first", "First", firstPage);

    EXPECT_EQ (Rectangle<float> (36.0f, 0.0f, 364.0f, 300.0f), firstPage.getBounds());
}
