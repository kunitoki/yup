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
class TabBarTests : public ::testing::Test
{
protected:
    struct MoveEvent
    {
        Identifier tabId;
        int oldIndex = -1;
        int newIndex = -1;
    };

    struct RecordingListener : TabBar::Listener
    {
        void tabSelectionChanged (TabBar&, const Identifier& tabId) override { selections.push_back (tabId); }

        void tabRemoved (TabBar&, const Identifier& tabId) override { removals.push_back (tabId); }

        std::vector<Identifier> selections;
        std::vector<Identifier> removals;
    };

    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());

        bar = std::make_unique<TabBar>();
        bar->setAnimationDuration (0.0);
        bar->setBounds (0.0f, 0.0f, 600.0f, 36.0f);

        bar->onSelectionChanged = [this] (const Identifier& tabId)
        {
            selections.push_back (tabId);
        };

        bar->onTabMoved = [this] (const Identifier& tabId, int oldIndex, int newIndex)
        {
            moves.push_back ({ tabId, oldIndex, newIndex });
        };
    }

    void TearDown() override
    {
        bar.reset();
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        oldTheme = nullptr;
    }

    void addTabs (std::initializer_list<const char*> tabIds)
    {
        for (const auto* tabId : tabIds)
            bar->addTab (tabId, String (tabId).toUpperCase());
    }

    TabButton& tab (const char* tabId) const
    {
        return *bar->getTabButton (tabId);
    }

    std::vector<Identifier> order() const
    {
        std::vector<Identifier> result;

        for (int i = 0; i < bar->getNumTabs(); ++i)
            result.push_back (bar->getTabId (i));

        return result;
    }

    void runFrames (int count)
    {
        for (int i = 0; i < count; ++i)
            bar->refreshDisplay (1.0 / 60.0);
    }

    void pressKey (int keyCode)
    {
        bar->keyDown (KeyPress (keyCode), {});
    }

    /** Mouse events reach a tab relative to where the tab is at that moment. */
    static MouseEvent eventAt (const TabButton& button, Point<float> barPoint)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), barPoint - button.getBounds().getTopLeft());
    }

    static void pressAt (TabButton& button, Point<float> barPoint) { button.mouseDown (eventAt (button, barPoint)); }

    static void dragTo (TabButton& button, Point<float> barPoint) { button.mouseDrag (eventAt (button, barPoint)); }

    static void releaseAt (TabButton& button, Point<float> barPoint) { button.mouseUp (eventAt (button, barPoint)); }

    static void click (TabButton& button)
    {
        const auto center = button.getBounds().getCenter();
        pressAt (button, center);
        releaseAt (button, center);
    }

    static Point<float> closeButtonCenter (const TabButton& button)
    {
        return button.getCloseButtonBounds().getCenter() + button.getBounds().getTopLeft();
    }

    ApplicationTheme::Ptr oldTheme;
    std::unique_ptr<TabBar> bar;
    std::vector<Identifier> selections;
    std::vector<MoveEvent> moves;
};

//==============================================================================
// Tabs and selection
//==============================================================================

TEST_F (TabBarTests, StartsEmptyWithoutSelection)
{
    EXPECT_EQ (0, bar->getNumTabs());
    EXPECT_TRUE (bar->getSelectedTabId().isNull());
    EXPECT_EQ (-1, bar->getSelectedTabIndex());
    EXPECT_TRUE (bar->getIndicatorBounds().isEmpty());
}

TEST_F (TabBarTests, AddingFirstTabSelectsItAndNotifiesOnce)
{
    bar->addTab ("a", "A");

    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
    ASSERT_EQ (1u, selections.size());
    EXPECT_EQ (Identifier ("a"), selections[0]);

    bar->addTab ("b", "B");

    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
    EXPECT_EQ (1u, selections.size());
}

TEST_F (TabBarTests, AddTabInsertsAtIndex)
{
    addTabs ({ "a", "b" });
    bar->addTab ("c", "C", 1);

    EXPECT_EQ ((std::vector<Identifier> { "a", "c", "b" }), order());
    EXPECT_EQ (1, bar->indexOfTab ("c"));
    EXPECT_EQ (-1, bar->indexOfTab ("missing"));
    EXPECT_EQ (nullptr, bar->getTabButton ("missing"));
    EXPECT_TRUE (bar->getTabId (5).isNull());
}

TEST_F (TabBarTests, SelectingNotifiesUnlessToldNot)
{
    addTabs ({ "a", "b", "c" });
    selections.clear();

    bar->setSelectedTab ("b");
    ASSERT_EQ (1u, selections.size());
    EXPECT_EQ (Identifier ("b"), selections[0]);

    bar->setSelectedTab ("b");
    EXPECT_EQ (1u, selections.size());

    bar->setSelectedTab ("c", dontSendNotification);
    EXPECT_EQ (1u, selections.size());
    EXPECT_EQ (2, bar->getSelectedTabIndex());
    EXPECT_TRUE (tab ("c").isSelected());
    EXPECT_FALSE (tab ("b").isSelected());
}

TEST_F (TabBarTests, ListenersAreToldEvenWithoutNotification)
{
    RecordingListener listener;
    addTabs ({ "a", "b" });
    bar->addListener (&listener);

    bar->setSelectedTab ("b", dontSendNotification);

    ASSERT_EQ (1u, listener.selections.size());
    EXPECT_EQ (Identifier ("b"), listener.selections[0]);

    bar->removeListener (&listener);
}

TEST_F (TabBarTests, RemovingSelectedMiddleTabSelectsTheNextOne)
{
    addTabs ({ "a", "b", "c" });
    bar->setSelectedTab ("b");
    selections.clear();

    bar->removeTab ("b");

    EXPECT_EQ (Identifier ("c"), bar->getSelectedTabId());
    ASSERT_EQ (1u, selections.size());
    EXPECT_EQ (Identifier ("c"), selections[0]);
}

TEST_F (TabBarTests, RemovingSelectedLastTabSelectsThePreviousOne)
{
    addTabs ({ "a", "b", "c" });
    bar->setSelectedTab ("c");

    bar->removeTab ("c");

    EXPECT_EQ (Identifier ("b"), bar->getSelectedTabId());
}

TEST_F (TabBarTests, RemovingUnselectedTabKeepsSelectionSilently)
{
    RecordingListener listener;
    addTabs ({ "a", "b" });
    bar->addListener (&listener);
    selections.clear();

    bar->removeTab ("b");

    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
    EXPECT_TRUE (selections.empty());
    ASSERT_EQ (1u, listener.removals.size());
    EXPECT_EQ (Identifier ("b"), listener.removals[0]);

    bar->removeListener (&listener);
}

TEST_F (TabBarTests, RemovingTheOnlyTabClearsSelection)
{
    addTabs ({ "a" });
    selections.clear();

    bar->removeTab ("a");

    EXPECT_EQ (0, bar->getNumTabs());
    EXPECT_TRUE (bar->getSelectedTabId().isNull());
    ASSERT_EQ (1u, selections.size());
    EXPECT_TRUE (selections[0].isNull());
    EXPECT_TRUE (bar->getIndicatorBounds().isEmpty());
}

TEST_F (TabBarTests, ClearTabsRemovesEverythingAndNotifiesOnce)
{
    RecordingListener listener;
    addTabs ({ "a", "b", "c" });
    bar->addListener (&listener);
    selections.clear();

    bar->clearTabs();

    EXPECT_EQ (0, bar->getNumTabs());
    EXPECT_TRUE (bar->getSelectedTabId().isNull());
    EXPECT_EQ (1u, selections.size());
    EXPECT_EQ (3u, listener.removals.size());

    bar->removeListener (&listener);
}

TEST_F (TabBarTests, MoveTabReordersAndNotifies)
{
    addTabs ({ "a", "b", "c" });

    bar->moveTab ("a", 2);

    EXPECT_EQ ((std::vector<Identifier> { "b", "c", "a" }), order());
    ASSERT_EQ (1u, moves.size());
    EXPECT_EQ (Identifier ("a"), moves[0].tabId);
    EXPECT_EQ (0, moves[0].oldIndex);
    EXPECT_EQ (2, moves[0].newIndex);
    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());

    bar->moveTab ("a", 99, dontSendNotification);
    EXPECT_EQ (1u, moves.size());
}

//==============================================================================
// Layout
//==============================================================================

TEST_F (TabBarTests, NaturalLayoutUsesPreferredLengthsInOrder)
{
    addTabs ({ "a", "bb", "ccc" });

    auto x = bar->getTabArea().getX();

    for (const auto* tabId : { "a", "bb", "ccc" })
    {
        const auto bounds = tab (tabId).getBounds();
        EXPECT_FLOAT_EQ (x, bounds.getX());
        EXPECT_FLOAT_EQ (tab (tabId).getPreferredLength(), bounds.getWidth());
        EXPECT_FLOAT_EQ (bar->getTabArea().getHeight(), bounds.getHeight());
        x += bounds.getWidth();
    }
}

TEST_F (TabBarTests, SelectionDoesNotChangeTabWidths)
{
    addTabs ({ "first", "second", "third" });

    std::vector<float> widths;
    for (const auto* tabId : { "first", "second", "third" })
        widths.push_back (tab (tabId).getWidth());

    bar->setSelectedTab ("third");

    EXPECT_FLOAT_EQ (widths[0], tab ("first").getWidth());
    EXPECT_FLOAT_EQ (widths[1], tab ("second").getWidth());
    EXPECT_FLOAT_EQ (widths[2], tab ("third").getWidth());
}

TEST_F (TabBarTests, FillLayoutSharesTheLengthEqually)
{
    bar->setLayout (TabBar::Layout::fill);
    addTabs ({ "a", "bbbbbbbbbb", "c" });

    const auto expected = bar->getTabArea().getWidth() / 3.0f;

    EXPECT_FLOAT_EQ (expected, tab ("a").getWidth());
    EXPECT_FLOAT_EQ (expected, tab ("bbbbbbbbbb").getWidth());
    EXPECT_FLOAT_EQ (expected, tab ("c").getWidth());
    EXPECT_FLOAT_EQ (bar->getTabArea().getRight(), tab ("c").getBounds().getRight());
}

TEST_F (TabBarTests, VerticalOrientationLaysTabsAlongY)
{
    bar->setBounds (0.0f, 0.0f, 120.0f, 400.0f);
    bar->setOrientation (TabBar::Orientation::vertical);
    addTabs ({ "a", "b" });

    const auto area = bar->getTabArea();
    const auto first = tab ("a").getBounds();
    const auto second = tab ("b").getBounds();

    EXPECT_FLOAT_EQ (area.getY(), first.getY());
    EXPECT_FLOAT_EQ (first.getBottom(), second.getY());
    EXPECT_FLOAT_EQ (area.getWidth(), first.getWidth());
    EXPECT_FLOAT_EQ (tab ("a").getPreferredLength(), first.getHeight());
}

TEST_F (TabBarTests, UnderlineVariantUsesTheFullBoundsAndAnEdgeIndicator)
{
    bar->setVariant (TabBar::Variant::underline);
    addTabs ({ "a", "b" });

    EXPECT_EQ (bar->getLocalBounds(), bar->getTabArea());

    const auto indicator = bar->getIndicatorBounds();
    EXPECT_FLOAT_EQ (tab ("a").getX(), indicator.getX());
    EXPECT_FLOAT_EQ (tab ("a").getWidth(), indicator.getWidth());
    EXPECT_FLOAT_EQ (bar->getHeight(), indicator.getBottom());

    bar->setFlipped (true);
    EXPECT_FLOAT_EQ (0.0f, bar->getIndicatorBounds().getY());
}

TEST_F (TabBarTests, ChangingTabContentRelaysTheBarOut)
{
    addTabs ({ "a", "b" });
    const auto before = tab ("b").getX();

    tab ("a").setText ("A much longer text than before");

    EXPECT_GT (tab ("b").getX(), before);
}

//==============================================================================
// Overflow
//==============================================================================

TEST_F (TabBarTests, OverflowButtonOnlyAppearsWhenTabsDoNotFit)
{
    addTabs ({ "alpha", "beta", "gamma" });

    EXPECT_FALSE (bar->getOverflowButton().isVisible());

    // Room for the first tab and the overflow button only.
    const auto width = bar->getWidth() - bar->getTabArea().getWidth()
                     + tab ("alpha").getPreferredLength() + bar->getOverflowButton().getPreferredLength() + 4.0f;
    bar->setBounds (0.0f, 0.0f, width, 36.0f);

    EXPECT_TRUE (bar->getOverflowButton().isVisible());
    EXPECT_TRUE (tab ("alpha").isVisible());
    EXPECT_FALSE (tab ("beta").isVisible());
    EXPECT_FALSE (tab ("gamma").isVisible());
    EXPECT_FALSE (bar->getOverflowButton().isSelected());
    EXPECT_EQ (bar->getOverflowText(), bar->getOverflowButton().getText());

    bar->setBounds (0.0f, 0.0f, 600.0f, 36.0f);

    EXPECT_FALSE (bar->getOverflowButton().isVisible());
    EXPECT_TRUE (tab ("gamma").isVisible());
}

TEST_F (TabBarTests, OverflowButtonStandsInForAHiddenSelectedTab)
{
    addTabs ({ "alpha", "beta", "gamma" });
    tab ("gamma").setIconGlyph (YUP_ICON_TABLE);

    bar->setBounds (0.0f, 0.0f, 150.0f, 36.0f);
    ASSERT_TRUE (bar->getOverflowButton().isVisible());
    ASSERT_FALSE (tab ("gamma").isVisible());

    bar->setSelectedTab ("gamma");

    auto& overflowButton = bar->getOverflowButton();
    EXPECT_TRUE (overflowButton.isSelected());
    EXPECT_EQ (String ("GAMMA"), overflowButton.getText());
    EXPECT_EQ (String::fromUTF8 (YUP_ICON_TABLE), overflowButton.getIconGlyph());
    EXPECT_FALSE (tab ("gamma").isVisible());
    EXPECT_EQ (overflowButton.getBounds(), bar->getIndicatorBounds());

    bar->setSelectedTab ("alpha");

    EXPECT_FALSE (overflowButton.isSelected());
    EXPECT_EQ (bar->getOverflowText(), overflowButton.getText());
    EXPECT_TRUE (overflowButton.getIconGlyph().isEmpty());
}

TEST_F (TabBarTests, ShrinkOverflowClampsTabsToTheMinimumLength)
{
    bar->setOverflow (TabBar::Overflow::shrink);
    addTabs ({ "a first long tab", "a second long tab", "a third long tab", "a fourth long tab" });
    bar->setBounds (0.0f, 0.0f, 150.0f, 36.0f);

    EXPECT_FALSE (bar->getOverflowButton().isVisible());

    for (const auto* tabId : { "a first long tab", "a second long tab", "a third long tab", "a fourth long tab" })
    {
        EXPECT_TRUE (tab (tabId).isVisible());
        EXPECT_LT (tab (tabId).getWidth(), tab (tabId).getPreferredLength());
        EXPECT_GE (tab (tabId).getWidth(), TabBar::minimumTabLength - 0.01f);
    }
}

TEST_F (TabBarTests, ShrinkOverflowSharesTheLengthWhenThereIsRoom)
{
    bar->setOverflow (TabBar::Overflow::shrink);
    addTabs ({ "a first long tab", "a second long tab" });

    const auto available = (tab ("a first long tab").getPreferredLength() + tab ("a second long tab").getPreferredLength()) * 0.8f;
    bar->setBounds (0.0f, 0.0f, available + bar->getWidth() - bar->getTabArea().getWidth(), 36.0f);

    const auto total = tab ("a first long tab").getWidth() + tab ("a second long tab").getWidth();
    EXPECT_NEAR (bar->getTabArea().getWidth(), total, 0.01f);
}

TEST_F (TabBarTests, ScrollOverflowRevealsTheSelectedTab)
{
    bar->setOverflow (TabBar::Overflow::scroll);
    addTabs ({ "one", "two", "three", "four", "five", "six" });
    bar->setBounds (0.0f, 0.0f, 150.0f, 36.0f);

    EXPECT_FLOAT_EQ (0.0f, bar->getScrollOffset());
    EXPECT_FALSE (bar->getOverflowButton().isVisible());

    bar->setSelectedTab ("six");

    const auto area = bar->getTabArea();
    const auto bounds = tab ("six").getBounds();
    EXPECT_GT (bar->getScrollOffset(), 0.0f);
    EXPECT_NEAR (area.getRight(), bounds.getRight(), 0.01f);

    bar->setSelectedTab ("one");

    EXPECT_FLOAT_EQ (0.0f, bar->getScrollOffset());
    EXPECT_FLOAT_EQ (area.getX(), tab ("one").getX());
}

TEST_F (TabBarTests, ScrollOverflowFollowsTheMouseWheel)
{
    bar->setOverflow (TabBar::Overflow::scroll);
    addTabs ({ "one", "two", "three", "four", "five", "six" });
    bar->setBounds (0.0f, 0.0f, 150.0f, 36.0f);

    const auto wheel = [this] (float deltaY)
    {
        bar->mouseWheel (MouseEvent (MouseEvent::noButtons, KeyModifiers(), { 10.0f, 10.0f }), MouseWheelData (0.0f, deltaY));
    };

    wheel (-1.0f);
    EXPECT_GT (bar->getScrollOffset(), 0.0f);

    wheel (100.0f);
    EXPECT_FLOAT_EQ (0.0f, bar->getScrollOffset());
}

//==============================================================================
// Mouse
//==============================================================================

TEST_F (TabBarTests, PressingATabSelectsIt)
{
    addTabs ({ "a", "b" });
    selections.clear();

    pressAt (tab ("b"), tab ("b").getBounds().getCenter());

    EXPECT_EQ (Identifier ("b"), bar->getSelectedTabId());
    EXPECT_EQ (1u, selections.size());
}

TEST_F (TabBarTests, PressingADisabledTabDoesNothing)
{
    addTabs ({ "a", "b" });
    tab ("b").setEnabled (false);

    click (tab ("b"));

    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
}

TEST_F (TabBarTests, ClickingTheCloseButtonRequestsCloseWithoutSelecting)
{
    addTabs ({ "a", "b" });
    tab ("b").setClosable (true);

    std::vector<Identifier> closeRequests;
    bar->onTabCloseRequested = [&closeRequests] (const Identifier& tabId)
    {
        closeRequests.push_back (tabId);
    };

    auto& button = tab ("b");
    pressAt (button, closeButtonCenter (button));
    releaseAt (button, closeButtonCenter (button));

    ASSERT_EQ (1u, closeRequests.size());
    EXPECT_EQ (Identifier ("b"), closeRequests[0]);
    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
    EXPECT_EQ (2, bar->getNumTabs());
}

TEST_F (TabBarTests, ClickingTheCloseButtonRemovesTheTabByDefault)
{
    addTabs ({ "a", "b" });
    tab ("b").setClosable (true);

    auto& button = tab ("b");
    const auto closeCenter = closeButtonCenter (button);
    pressAt (button, closeCenter);
    releaseAt (button, closeCenter);

    EXPECT_EQ (1, bar->getNumTabs());
    EXPECT_EQ (nullptr, bar->getTabButton ("b"));
}

TEST_F (TabBarTests, ReleasingOutsideTheCloseButtonDoesNotClose)
{
    addTabs ({ "a", "b" });
    tab ("b").setClosable (true);

    auto& button = tab ("b");
    pressAt (button, closeButtonCenter (button));
    releaseAt (button, button.getBounds().getTopLeft() + Point<float> (2.0f, 2.0f));

    EXPECT_EQ (2, bar->getNumTabs());
}

TEST_F (TabBarTests, ShortDragDoesNotReorder)
{
    bar->setReorderable (true);
    addTabs ({ "a", "b", "c" });

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    pressAt (button, start);
    dragTo (button, start + Point<float> (5.0f, 0.0f));
    releaseAt (button, start + Point<float> (5.0f, 0.0f));

    EXPECT_EQ ((std::vector<Identifier> { "a", "b", "c" }), order());
    EXPECT_TRUE (moves.empty());
}

TEST_F (TabBarTests, DraggingPastANeighbourReordersAndNotifiesOnce)
{
    bar->setReorderable (true);
    addTabs ({ "a", "b", "c" });

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    const auto pastB = tab ("b").getBounds().getCenter() + Point<float> (2.0f, 0.0f);

    pressAt (button, start);
    dragTo (button, start + Point<float> (10.0f, 0.0f));
    dragTo (button, pastB);

    EXPECT_EQ ((std::vector<Identifier> { "b", "a", "c" }), order());
    EXPECT_TRUE (moves.empty());

    releaseAt (button, pastB);

    ASSERT_EQ (1u, moves.size());
    EXPECT_EQ (Identifier ("a"), moves[0].tabId);
    EXPECT_EQ (0, moves[0].oldIndex);
    EXPECT_EQ (1, moves[0].newIndex);

    // Dropped, the tab settles into its slot.
    EXPECT_FLOAT_EQ (tab ("b").getBounds().getRight(), button.getX());
}

TEST_F (TabBarTests, DraggingBackToTheStartDoesNotNotify)
{
    bar->setReorderable (true);
    addTabs ({ "a", "b", "c" });

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    const auto pastB = tab ("b").getBounds().getCenter() + Point<float> (2.0f, 0.0f);

    pressAt (button, start);
    dragTo (button, pastB);
    dragTo (button, start);
    releaseAt (button, start);

    EXPECT_EQ ((std::vector<Identifier> { "a", "b", "c" }), order());
    EXPECT_TRUE (moves.empty());
}

TEST_F (TabBarTests, DraggingDoesNothingWhenNotReorderable)
{
    addTabs ({ "a", "b", "c" });

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    const auto pastC = tab ("c").getBounds().getCenter() + Point<float> (2.0f, 0.0f);

    pressAt (button, start);
    dragTo (button, pastC);
    releaseAt (button, pastC);

    EXPECT_EQ ((std::vector<Identifier> { "a", "b", "c" }), order());
    EXPECT_TRUE (moves.empty());
}

TEST_F (TabBarTests, ClickIsSuppressedAfterADrag)
{
    bar->setReorderable (true);
    addTabs ({ "a", "b" });

    int clicks = 0;
    tab ("a").onClick = [&clicks]
    {
        ++clicks;
    };

    click (tab ("a"));
    EXPECT_EQ (1, clicks);

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    pressAt (button, start);
    dragTo (button, start + Point<float> (12.0f, 0.0f));
    releaseAt (button, start + Point<float> (12.0f, 0.0f));

    EXPECT_EQ (1, clicks);
}

TEST_F (TabBarTests, IndicatorFollowsTheDraggedTab)
{
    bar->setReorderable (true);
    addTabs ({ "a", "b", "c" });

    auto& button = tab ("a");
    const auto start = button.getBounds().getCenter();
    pressAt (button, start);
    dragTo (button, start + Point<float> (20.0f, 0.0f));

    EXPECT_EQ (button.getBounds(), bar->getIndicatorBounds());

    releaseAt (button, start + Point<float> (20.0f, 0.0f));
}

//==============================================================================
// Keyboard
//==============================================================================

TEST_F (TabBarTests, ArrowKeysSelectNeighboursSkippingDisabledTabs)
{
    addTabs ({ "a", "b", "c", "d" });
    tab ("c").setEnabled (false);

    pressKey (KeyPress::rightKey);
    EXPECT_EQ (Identifier ("b"), bar->getSelectedTabId());

    pressKey (KeyPress::rightKey);
    EXPECT_EQ (Identifier ("d"), bar->getSelectedTabId());

    pressKey (KeyPress::rightKey);
    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());

    pressKey (KeyPress::leftKey);
    EXPECT_EQ (Identifier ("d"), bar->getSelectedTabId());

    pressKey (KeyPress::downKey);
    EXPECT_EQ (Identifier ("d"), bar->getSelectedTabId());
}

TEST_F (TabBarTests, HomeAndEndSelectTheFirstAndLastEnabledTabs)
{
    addTabs ({ "a", "b", "c", "d" });
    tab ("a").setEnabled (false);
    tab ("d").setEnabled (false);

    pressKey (KeyPress::endKey);
    EXPECT_EQ (Identifier ("c"), bar->getSelectedTabId());

    pressKey (KeyPress::homeKey);
    EXPECT_EQ (Identifier ("b"), bar->getSelectedTabId());
}

TEST_F (TabBarTests, VerticalBarsUseUpAndDown)
{
    bar->setBounds (0.0f, 0.0f, 120.0f, 400.0f);
    bar->setOrientation (TabBar::Orientation::vertical);
    addTabs ({ "a", "b" });

    pressKey (KeyPress::rightKey);
    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());

    pressKey (KeyPress::downKey);
    EXPECT_EQ (Identifier ("b"), bar->getSelectedTabId());

    pressKey (KeyPress::upKey);
    EXPECT_EQ (Identifier ("a"), bar->getSelectedTabId());
}

//==============================================================================
// Animation
//==============================================================================

TEST_F (TabBarTests, WithoutAnimationTheIndicatorMovesAtOnce)
{
    addTabs ({ "a", "b", "c" });

    bar->setSelectedTab ("c");

    EXPECT_EQ (tab ("c").getBounds(), bar->getIndicatorBounds());
}

TEST_F (TabBarTests, IndicatorSlidesToTheSelectedTab)
{
    addTabs ({ "a", "b", "c" });
    bar->setAnimationDuration (0.2);

    const auto start = bar->getIndicatorBounds();
    bar->setSelectedTab ("c");

    EXPECT_EQ (start, bar->getIndicatorBounds());

    runFrames (2);
    const auto midway = bar->getIndicatorBounds();
    EXPECT_GT (midway.getX(), start.getX());
    EXPECT_LT (midway.getX(), tab ("c").getX());

    runFrames (12);
    EXPECT_EQ (tab ("c").getBounds(), bar->getIndicatorBounds());
}

TEST_F (TabBarTests, NeighboursSlideWhenATabIsRemoved)
{
    addTabs ({ "a", "b", "c" });
    bar->setAnimationDuration (0.2);

    const auto slotOfB = tab ("b").getX();
    bar->removeTab ("b");

    EXPECT_GT (tab ("c").getX(), slotOfB);

    runFrames (15);
    EXPECT_FLOAT_EQ (slotOfB, tab ("c").getX());
}

TEST_F (TabBarTests, ResizingSnapsWithoutAnimating)
{
    bar->setLayout (TabBar::Layout::fill);
    addTabs ({ "a", "b" });
    bar->setAnimationDuration (0.2);

    bar->setBounds (0.0f, 0.0f, 300.0f, 36.0f);

    EXPECT_FLOAT_EQ (bar->getTabArea().getWidth() * 0.5f, tab ("a").getWidth());
    EXPECT_EQ (tab ("a").getBounds(), bar->getIndicatorBounds());
}
