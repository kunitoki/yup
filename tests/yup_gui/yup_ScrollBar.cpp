/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
class ScrollBarTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        scrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::vertical);
        scrollBar->setBounds (0.0f, 0.0f, 20.0f, 400.0f);
    }

    std::unique_ptr<ScrollBar> scrollBar;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (ScrollBarTests, ConstructorInitializesCorrectly)
{
    EXPECT_EQ (ScrollBar::Orientation::vertical, scrollBar->getOrientation());
    EXPECT_EQ (ScrollBar::VisibilityMode::autoHide, scrollBar->getVisibilityMode());
    EXPECT_EQ (0.0, scrollBar->getCurrentRangeStart());
}

TEST_F (ScrollBarTests, OrientationCanBeChanged)
{
    scrollBar->setOrientation (ScrollBar::Orientation::horizontal);
    EXPECT_EQ (ScrollBar::Orientation::horizontal, scrollBar->getOrientation());
}

TEST_F (ScrollBarTests, VisibilityModeCanBeChanged)
{
    scrollBar->setVisibilityMode (ScrollBar::VisibilityMode::alwaysVisible);
    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysVisible, scrollBar->getVisibilityMode());

    scrollBar->setVisibilityMode (ScrollBar::VisibilityMode::alwaysHidden);
    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysHidden, scrollBar->getVisibilityMode());
}

//==============================================================================
// Range Tests
//==============================================================================

TEST_F (ScrollBarTests, RangeLimitsCanBeSet)
{
    scrollBar->setRangeLimits (0.0, 1000.0);

    EXPECT_EQ (0.0, scrollBar->getRangeMinimum());
    EXPECT_EQ (1000.0, scrollBar->getRangeMaximum());
}

TEST_F (ScrollBarTests, CurrentRangeCanBeSet)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (100.0, 200.0);

    EXPECT_EQ (100.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (200.0, scrollBar->getCurrentRangeEnd());
    EXPECT_EQ (100.0, scrollBar->getCurrentRangeSize());
}

TEST_F (ScrollBarTests, CurrentRangeIsClamped)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (-100.0, 1500.0);

    EXPECT_EQ (0.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (1000.0, scrollBar->getCurrentRangeEnd());
}

//==============================================================================
// Scrolling Tests
//==============================================================================

TEST_F (ScrollBarTests, SetCurrentRangeStartMaintainsSize)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    scrollBar->setCurrentRangeStart (200.0, dontSendNotification);

    EXPECT_EQ (200.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (300.0, scrollBar->getCurrentRangeEnd());
    EXPECT_EQ (100.0, scrollBar->getCurrentRangeSize());
}

TEST_F (ScrollBarTests, ScrollByChangePosition)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    scrollBar->scrollBy (50.0, dontSendNotification);

    EXPECT_EQ (50.0, scrollBar->getCurrentRangeStart());
}

TEST_F (ScrollBarTests, ScrollingIsClamped)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    scrollBar->setCurrentRangeStart (950.0, dontSendNotification);

    EXPECT_EQ (900.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (1000.0, scrollBar->getCurrentRangeEnd());
}

TEST_F (ScrollBarTests, ScrollPositionChangedCallbackInvoked)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    bool callbackInvoked = false;
    double receivedPosition = -1.0;

    scrollBar->onScrollPositionChanged = [&callbackInvoked, &receivedPosition] (double newPosition)
    {
        callbackInvoked = true;
        receivedPosition = newPosition;
    };

    scrollBar->setCurrentRangeStart (200.0, sendNotification);

    EXPECT_TRUE (callbackInvoked);
    EXPECT_EQ (200.0, receivedPosition);
}

//==============================================================================
// Visibility Tests
//==============================================================================

TEST_F (ScrollBarTests, AutoHideModeWorks)
{
    scrollBar->setAutoHide (true);
    EXPECT_TRUE (scrollBar->isAutoHide());

    scrollBar->setAutoHide (false);
    EXPECT_FALSE (scrollBar->isAutoHide());
}

TEST_F (ScrollBarTests, IsScrollingNeededWhenContentExceedsViewport)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    EXPECT_TRUE (scrollBar->isScrollingNeeded());

    scrollBar->setCurrentRange (0.0, 1000.0);

    EXPECT_FALSE (scrollBar->isScrollingNeeded());
}

//==============================================================================
// Sizing Tests
//==============================================================================

TEST_F (ScrollBarTests, ScrollBarWidthCanBeSet)
{
    scrollBar->setScrollBarWidth (15.0f);

    EXPECT_EQ (15.0f, scrollBar->getScrollBarWidth());
}

TEST_F (ScrollBarTests, MinimumScrollBarWidthEnforced)
{
    scrollBar->setScrollBarWidth (0.5f);

    EXPECT_GE (scrollBar->getScrollBarWidth(), 1.0f);
}

//==============================================================================
// ListBox Integration Tests
//==============================================================================

TEST (ListBoxScrollBarTests, ListBoxHasScrollBars)
{
    ListBox listBox;

    EXPECT_NE (nullptr, listBox.getVerticalScrollBar());
    EXPECT_NE (nullptr, listBox.getHorizontalScrollBar());
}

TEST (ListBoxScrollBarTests, ScrollBarVisibilityCanBeConfigured)
{
    ListBox listBox;

    listBox.setVerticalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysVisible);

    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysVisible,
               listBox.getVerticalScrollBar()->getVisibilityMode());
}

TEST (ListBoxScrollBarTests, HorizontalScrollBarCanBeConfigured)
{
    ListBox listBox;

    listBox.setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysHidden);

    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysHidden,
               listBox.getHorizontalScrollBar()->getVisibilityMode());
}

//==============================================================================
// Mouse Interaction Tests
//==============================================================================

TEST_F (ScrollBarTests, MouseDownOnThumbStartsDragging)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();
    auto center = thumbBounds.getCenter();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseDown (event);

    EXPECT_TRUE (scrollBar->isDragging());
}

TEST_F (ScrollBarTests, MouseUpStopsDragging)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();
    auto center = thumbBounds.getCenter();

    MouseEvent downEvent (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseDown (downEvent);

    MouseEvent upEvent (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseUp (upEvent);

    EXPECT_FALSE (scrollBar->isDragging());
}

TEST_F (ScrollBarTests, MouseDragMovesThumb)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();
    auto center = thumbBounds.getCenter();

    MouseEvent downEvent (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseDown (downEvent);

    double initialPosition = scrollBar->getCurrentRangeStart();

    MouseEvent dragEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (center.getX(), center.getY() + 50.0f));
    scrollBar->mouseDrag (dragEvent);

    EXPECT_GT (scrollBar->getCurrentRangeStart(), initialPosition);
}

TEST_F (ScrollBarTests, MouseEnterSetsHoverState)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();
    auto center = thumbBounds.getCenter();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseEnter (event);

    EXPECT_TRUE (scrollBar->isThumbHovered());
}

TEST_F (ScrollBarTests, MouseExitClearsHoverState)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();
    auto center = thumbBounds.getCenter();

    MouseEvent enterEvent (MouseEvent::leftButton, KeyModifiers(), center);
    scrollBar->mouseEnter (enterEvent);

    MouseEvent exitEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (0.0f, 0.0f));
    scrollBar->mouseExit (exitEvent);

    EXPECT_FALSE (scrollBar->isThumbHovered());
}

TEST_F (ScrollBarTests, MouseWheelScrollsContent)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (100.0, 200.0);

    double initialPosition = scrollBar->getCurrentRangeStart();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (10.0f, 100.0f));
    MouseWheelData wheelData (0.0f, 3.0f);
    scrollBar->mouseWheel (event, wheelData);

    EXPECT_NE (scrollBar->getCurrentRangeStart(), initialPosition);
}

//==============================================================================
// Rendering Bounds Tests
//==============================================================================

TEST_F (ScrollBarTests, ThumbBoundsAreValid)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();

    EXPECT_FALSE (thumbBounds.isEmpty());
    EXPECT_GT (thumbBounds.getWidth(), 0.0f);
    EXPECT_GT (thumbBounds.getHeight(), 0.0f);
}

TEST_F (ScrollBarTests, TrackBoundsAreValid)
{
    scrollBar->resized();

    auto trackBounds = scrollBar->getTrackBoundsForRendering();

    EXPECT_FALSE (trackBounds.isEmpty());
}

TEST_F (ScrollBarTests, ThumbBoundsChangeWithPosition)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto bounds1 = scrollBar->getThumbBoundsForRendering();

    scrollBar->setCurrentRangeStart (500.0, dontSendNotification);
    scrollBar->resized();

    auto bounds2 = scrollBar->getThumbBoundsForRendering();

    EXPECT_NE (bounds1.getY(), bounds2.getY());
}

TEST_F (ScrollBarTests, ThumbSizeScalesWithContentRatio)
{
    scrollBar->setRangeLimits (0.0, 1000.0);

    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();
    auto smallRangeBounds = scrollBar->getThumbBoundsForRendering();

    scrollBar->setCurrentRange (0.0, 500.0);
    scrollBar->resized();
    auto largeRangeBounds = scrollBar->getThumbBoundsForRendering();

    EXPECT_LT (smallRangeBounds.getHeight(), largeRangeBounds.getHeight());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (ScrollBarTests, ZeroRangeHandling)
{
    scrollBar->setRangeLimits (0.0, 0.0);
    scrollBar->setCurrentRange (0.0, 0.0);

    EXPECT_FALSE (scrollBar->isScrollingNeeded());
}

TEST_F (ScrollBarTests, NegativeRangeHandling)
{
    scrollBar->setRangeLimits (-100.0, 100.0);
    scrollBar->setCurrentRange (-50.0, 50.0);

    EXPECT_EQ (-50.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (50.0, scrollBar->getCurrentRangeEnd());
}

TEST_F (ScrollBarTests, InvertedRangeClamping)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (500.0, 100.0); // Inverted

    // Should be clamped appropriately
    EXPECT_LE (scrollBar->getCurrentRangeStart(), scrollBar->getCurrentRangeEnd());
}

TEST_F (ScrollBarTests, VerySmallRangeSize)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (500.0, 500.1);

    EXPECT_TRUE (scrollBar->isScrollingNeeded());
}

TEST_F (ScrollBarTests, ScrollBeyondStartClamped)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (100.0, 200.0);

    scrollBar->scrollBy (-200.0, dontSendNotification);

    EXPECT_EQ (0.0, scrollBar->getCurrentRangeStart());
}

TEST_F (ScrollBarTests, ScrollBeyondEndClamped)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (800.0, 900.0);

    scrollBar->scrollBy (200.0, dontSendNotification);

    EXPECT_EQ (900.0, scrollBar->getCurrentRangeStart());
    EXPECT_EQ (1000.0, scrollBar->getCurrentRangeEnd());
}

//==============================================================================
// Horizontal Orientation Tests
//==============================================================================

TEST_F (ScrollBarTests, HorizontalOrientationWorks)
{
    scrollBar->setOrientation (ScrollBar::Orientation::horizontal);
    scrollBar->setBounds (0.0f, 0.0f, 400.0f, 20.0f);
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto thumbBounds = scrollBar->getThumbBoundsForRendering();

    EXPECT_FALSE (thumbBounds.isEmpty());
    EXPECT_GT (thumbBounds.getWidth(), 0.0f);
}

TEST_F (ScrollBarTests, HorizontalThumbMoves)
{
    scrollBar->setOrientation (ScrollBar::Orientation::horizontal);
    scrollBar->setBounds (0.0f, 0.0f, 400.0f, 20.0f);
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);
    scrollBar->resized();

    auto bounds1 = scrollBar->getThumbBoundsForRendering();

    scrollBar->setCurrentRangeStart (500.0, dontSendNotification);
    scrollBar->resized();

    auto bounds2 = scrollBar->getThumbBoundsForRendering();

    EXPECT_NE (bounds1.getX(), bounds2.getX());
}

//==============================================================================
// Callback Notification Tests
//==============================================================================

TEST_F (ScrollBarTests, CallbackNotInvokedWithDontSendNotification)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    int callbackCount = 0;
    scrollBar->onScrollPositionChanged = [&callbackCount] (double)
    {
        callbackCount++;
    };

    scrollBar->setCurrentRangeStart (200.0, dontSendNotification);

    EXPECT_EQ (0, callbackCount);
}

TEST_F (ScrollBarTests, CallbackInvokedForScrollBy)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    int callbackCount = 0;
    scrollBar->onScrollPositionChanged = [&callbackCount] (double)
    {
        callbackCount++;
    };

    scrollBar->scrollBy (50.0, sendNotification);

    EXPECT_EQ (1, callbackCount);
}

//==============================================================================
// Resized and Paint Tests
//==============================================================================

TEST_F (ScrollBarTests, ResizedDoesNotCrash)
{
    scrollBar->setRangeLimits (0.0, 1000.0);
    scrollBar->setCurrentRange (0.0, 100.0);

    scrollBar->resized();

    EXPECT_TRUE (true);
}

TEST_F (ScrollBarTests, ResizedWithZeroSize)
{
    scrollBar->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    scrollBar->resized();

    EXPECT_TRUE (true);
}
