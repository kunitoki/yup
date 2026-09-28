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

namespace
{
class TestListBoxModel : public ListBoxModel
{
public:
    /** A custom row component that remembers the row it last showed. */
    class RowComponent : public Component
    {
    public:
        explicit RowComponent (int& createdCount)
        {
            ++createdCount;
        }

        int shownRow = -1;
        bool shownSelected = false;
    };

    explicit TestListBoxModel (int numRows)
        : numRows (numRows)
    {
    }

    int getNumRows() override
    {
        return numRows;
    }

    float getRowSize (int rowIndex) override
    {
        if (isPositiveAndBelow (rowIndex, static_cast<int> (rowSizes.size())))
            return rowSizes[static_cast<size_t> (rowIndex)];

        return 0.0f;
    }

    void refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component) override
    {
        ++refreshCount;

        if (! useCustomComponents)
            return;

        auto& row = reuseOrCreate<RowComponent> (component, createdComponents);
        row.shownRow = rowIndex;
        row.shownSelected = isSelected;
    }

    String getRowText (int rowIndex) override
    {
        return "Row " + String (rowIndex);
    }

    void selectedRowsChanged (const Array<int>& selectedRows) override
    {
        lastSelectedRows = selectedRows;
        selectionChangedCallCount++;
    }

    void rowClicked (int rowIndex, const MouseEvent& event) override
    {
        ignoreUnused (event);
        lastClickedRow = rowIndex;
        clickCallCount++;
    }

    void rowDoubleClicked (int rowIndex, const MouseEvent& event) override
    {
        ignoreUnused (event);
        lastDoubleClickedRow = rowIndex;
        doubleClickCallCount++;
    }

    void returnKeyPressed (int currentRow) override
    {
        lastReturnKeyRow = currentRow;
        returnKeyCallCount++;
    }

    void deleteKeyPressed (const Array<int>& selectedRows) override
    {
        lastDeleteKeyRows = selectedRows;
        deleteKeyCallCount++;
    }

    var getDragSourceDescription (const Array<int>& selectedRows) override
    {
        dragSourceCallCount++;
        if (shouldSupportDrag && ! selectedRows.isEmpty())
            return var ("DragData");
        return {};
    }

    int numRows;
    std::vector<float> rowSizes;
    bool useCustomComponents = false;
    int createdComponents = 0;
    int refreshCount = 0;
    Array<int> lastSelectedRows;
    int lastClickedRow = -1;
    int lastDoubleClickedRow = -1;
    int lastReturnKeyRow = -2;
    Array<int> lastDeleteKeyRows;
    int selectionChangedCallCount = 0;
    int clickCallCount = 0;
    int doubleClickCallCount = 0;
    int returnKeyCallCount = 0;
    int deleteKeyCallCount = 0;
    int dragSourceCallCount = 0;
    bool shouldSupportDrag = false;
};
} // namespace

//==============================================================================
class ListBoxTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        listBox = std::make_unique<ListBox>();
        listBox->setBounds (0.0f, 0.0f, 300.0f, 400.0f);

        model = std::make_unique<TestListBoxModel> (20);
        listBox->setModel (model.get());
    }

    /** Lays the list out along an orientation: 400pt along the scroll axis and 300pt across it. */
    void useOrientation (ListBox::Orientation orientation)
    {
        listBox->setOrientation (orientation);

        if (orientation == ListBox::Orientation::vertical)
            listBox->setBounds (0.0f, 0.0f, 300.0f, 400.0f);
        else
            listBox->setBounds (0.0f, 0.0f, 400.0f, 300.0f);
    }

    bool isVertical() const
    {
        return listBox->getOrientation() == ListBox::Orientation::vertical;
    }

    /** A point at a distance along the scroll axis, 100pt across it. */
    Point<float> pointAlong (float along) const
    {
        return isVertical() ? Point<float> (100.0f, along) : Point<float> (along, 100.0f);
    }

    float mainStartOf (Rectangle<float> area) const
    {
        return isVertical() ? area.getY() : area.getX();
    }

    float mainSizeOf (Rectangle<float> area) const
    {
        return isVertical() ? area.getHeight() : area.getWidth();
    }

    /** Switches the model to a row count and tells the list. */
    void setNumRows (int numRows)
    {
        model->numRows = numRows;
        listBox->updateContent();
    }

    void runFrames (int count)
    {
        for (int i = 0; i < count; ++i)
            listBox->refreshDisplay (1.0 / 60.0);
    }

    /** Builds a click in the middle of a row, with the modifiers to hold down. */
    MouseEvent makeRowClick (int rowIndex, KeyModifiers modifiers = {}) const
    {
        return MouseEvent (MouseEvent::leftButton, modifiers, listBox->getRowBounds (rowIndex).getCenter());
    }

    Array<int> selection() const
    {
        return listBox->getSelectedRows();
    }

    static MouseEvent touchAt (Point<float> position, int touchIndex = 0)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), position).withTouchIndex (touchIndex);
    }

    /** Drags a finger along the scroll axis in steps one frame apart, releasing straight after the
        last step unless told not to, which leaves the release velocity in place. */
    void swipe (float from, float to, bool release = true, int steps = 5)
    {
        listBox->mouseDown (touchAt (pointAlong (from)));

        for (int i = 1; i <= steps; ++i)
        {
            runFrames (1);
            listBox->mouseDrag (touchAt (pointAlong (from + (to - from) * static_cast<float> (i) / static_cast<float> (steps))));
        }

        if (release)
            listBox->mouseUp (touchAt (pointAlong (to)));
    }

    void tapRow (int rowIndex)
    {
        const auto position = listBox->getRowBounds (rowIndex).getCenter();
        listBox->mouseDown (touchAt (position));
        listBox->mouseUp (touchAt (position));
    }

    std::unique_ptr<TestListBoxModel> model;
    std::unique_ptr<ListBox> listBox;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (ListBoxTests, ConstructorInitializesCorrectly)
{
    EXPECT_EQ (ListBox::Orientation::vertical, listBox->getOrientation());
    EXPECT_EQ (ListBox::SelectionMode::single, listBox->getSelectionMode());
    EXPECT_EQ (-1, listBox->getSelectedRow());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (-1, listBox->getCurrentRow());
    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
    EXPECT_EQ (ListBox::ScrollState::idle, listBox->getScrollState());
    EXPECT_FLOAT_EQ (0.0f, listBox->getRowSpacing());
    EXPECT_EQ (nullptr, listBox->getHeaderComponent());
    EXPECT_EQ (nullptr, listBox->getFooterComponent());
}

TEST_F (ListBoxTests, OrientationCanBeChanged)
{
    listBox->setOrientation (ListBox::Orientation::horizontal);
    EXPECT_EQ (ListBox::Orientation::horizontal, listBox->getOrientation());

    listBox->setOrientation (ListBox::Orientation::vertical);
    EXPECT_EQ (ListBox::Orientation::vertical, listBox->getOrientation());
}

TEST_F (ListBoxTests, SelectionModeCanBeChanged)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    EXPECT_EQ (ListBox::SelectionMode::multiple, listBox->getSelectionMode());

    listBox->setSelectionMode (ListBox::SelectionMode::none);
    EXPECT_EQ (ListBox::SelectionMode::none, listBox->getSelectionMode());
}

TEST_F (ListBoxTests, ModelCanBeSetAndRetrieved)
{
    EXPECT_EQ (model.get(), listBox->getModel());

    listBox->setModel (nullptr);
    EXPECT_EQ (nullptr, listBox->getModel());
}

TEST_F (ListBoxTests, SettingAModelClearsTheSelectionAndTheCurrentRow)
{
    listBox->selectRow (5, false, dontSendNotification);

    TestListBoxModel otherModel (10);
    listBox->setModel (&otherModel);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (-1, listBox->getCurrentRow());

    listBox->setModel (nullptr);
}

//==============================================================================
// Selection Tests - Single Mode
//==============================================================================

TEST_F (ListBoxTests, SelectRowInSingleMode)
{
    listBox->selectRow (5, false, dontSendNotification);

    EXPECT_EQ (5, listBox->getSelectedRow());
    EXPECT_EQ (1, listBox->getNumSelectedRows());
    EXPECT_TRUE (listBox->isRowSelected (5));
    EXPECT_FALSE (listBox->isRowSelected (4));
    EXPECT_FALSE (listBox->isRowSelected (6));
}

TEST_F (ListBoxTests, SelectingNewRowDeselectsPreviousInSingleMode)
{
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    EXPECT_EQ (10, listBox->getSelectedRow());
    EXPECT_EQ (1, listBox->getNumSelectedRows());
    EXPECT_FALSE (listBox->isRowSelected (5));
    EXPECT_TRUE (listBox->isRowSelected (10));
}

TEST_F (ListBoxTests, DeselectRowInSingleMode)
{
    listBox->selectRow (5, false, dontSendNotification);
    listBox->deselectRow (5, dontSendNotification);

    EXPECT_EQ (-1, listBox->getSelectedRow());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_FALSE (listBox->isRowSelected (5));
}

TEST_F (ListBoxTests, DeselectAllRowsInSingleMode)
{
    listBox->selectRow (5, false, dontSendNotification);
    listBox->deselectAllRows (dontSendNotification);

    EXPECT_EQ (-1, listBox->getSelectedRow());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, SelectionNotificationSentInSingleMode)
{
    listBox->selectRow (5, false, sendNotification);

    EXPECT_EQ (1, model->selectionChangedCallCount);
    EXPECT_EQ (1, model->lastSelectedRows.size());
    EXPECT_EQ (5, model->lastSelectedRows[0]);
}

TEST_F (ListBoxTests, SelectingARowMakesItTheCurrentRow)
{
    listBox->selectRow (7, false, dontSendNotification);

    EXPECT_EQ (7, listBox->getCurrentRow());
}

//==============================================================================
// Selection Tests - Multiple Mode
//==============================================================================

TEST_F (ListBoxTests, SelectMultipleRowsInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);
    listBox->selectRow (15, false, dontSendNotification);

    EXPECT_EQ (3, listBox->getNumSelectedRows());
    EXPECT_TRUE (listBox->isRowSelected (5));
    EXPECT_TRUE (listBox->isRowSelected (10));
    EXPECT_TRUE (listBox->isRowSelected (15));
}

TEST_F (ListBoxTests, SetSelectedRowsInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    Array<int> rowsToSelect { 3, 7, 11, 15 };
    listBox->setSelectedRows (rowsToSelect, dontSendNotification);

    auto selectedRows = listBox->getSelectedRows();
    EXPECT_EQ (4, selectedRows.size());
    EXPECT_TRUE (listBox->isRowSelected (3));
    EXPECT_TRUE (listBox->isRowSelected (7));
    EXPECT_TRUE (listBox->isRowSelected (11));
    EXPECT_TRUE (listBox->isRowSelected (15));
}

TEST_F (ListBoxTests, SetSelectedRowsIgnoresRowsTheModelDoesNotHave)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->setSelectedRows ({ -1, 3, 3, 50 }, dontSendNotification);

    EXPECT_EQ (Array<int> ({ 3 }), selection());
}

TEST_F (ListBoxTests, GetSelectedRowsReturnsSortedArray)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    Array<int> rowsToSelect { 15, 3, 11, 7 };
    listBox->setSelectedRows (rowsToSelect, dontSendNotification);

    auto selectedRows = listBox->getSelectedRows();
    EXPECT_EQ (4, selectedRows.size());

    for (int i = 1; i < selectedRows.size(); ++i)
    {
        EXPECT_LT (selectedRows[i - 1], selectedRows[i]);
    }
}

TEST_F (ListBoxTests, DeselectRowInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);
    listBox->deselectRow (5, dontSendNotification);

    EXPECT_EQ (1, listBox->getNumSelectedRows());
    EXPECT_FALSE (listBox->isRowSelected (5));
    EXPECT_TRUE (listBox->isRowSelected (10));
}

TEST_F (ListBoxTests, ChangingToSingleModeKeepsOnlyFirstSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);
    listBox->selectRow (15, false, dontSendNotification);

    listBox->setSelectionMode (ListBox::SelectionMode::single);

    EXPECT_EQ (1, listBox->getNumSelectedRows());
    EXPECT_TRUE (listBox->isRowSelected (5));
}

//==============================================================================
// Selection Tests - None Mode
//==============================================================================

TEST_F (ListBoxTests, NoSelectionAllowedInNoneMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::none);

    listBox->selectRow (5, false, dontSendNotification);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_FALSE (listBox->isRowSelected (5));
}

TEST_F (ListBoxTests, ChangingToNoneModeDeselectsAll)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->setSelectionMode (ListBox::SelectionMode::none);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

//==============================================================================
// Row Size Tests
//==============================================================================

TEST_F (ListBoxTests, RowSizeDefaultDependsOnTheOrientation)
{
    EXPECT_FLOAT_EQ (24.0f, listBox->getRowSize());
    EXPECT_FLOAT_EQ (24.0f, listBox->getRowBounds (0).getHeight());

    useOrientation (ListBox::Orientation::horizontal);

    EXPECT_FLOAT_EQ (96.0f, listBox->getRowSize());
    EXPECT_FLOAT_EQ (96.0f, listBox->getRowBounds (0).getWidth());
}

TEST_F (ListBoxTests, AnExplicitRowSizeSurvivesAnOrientationChange)
{
    listBox->setRowSize (40.0f);
    listBox->setOrientation (ListBox::Orientation::horizontal);

    EXPECT_FLOAT_EQ (40.0f, listBox->getRowSize());
    EXPECT_FLOAT_EQ (40.0f, listBox->getRowBounds (0).getWidth());
}

TEST_F (ListBoxTests, ModelRowSizesOverrideTheDefaultInBothOrientations)
{
    model->rowSizes = { 30.0f, 40.0f, 0.0f, 60.0f };

    for (auto orientation : { ListBox::Orientation::vertical, ListBox::Orientation::horizontal })
    {
        useOrientation (orientation);
        listBox->setRowSize (50.0f);

        EXPECT_FLOAT_EQ (30.0f, mainSizeOf (listBox->getRowBounds (0)));
        EXPECT_FLOAT_EQ (40.0f, mainSizeOf (listBox->getRowBounds (1)));
        EXPECT_FLOAT_EQ (50.0f, mainSizeOf (listBox->getRowBounds (2)));
        EXPECT_FLOAT_EQ (60.0f, mainSizeOf (listBox->getRowBounds (3)));
        EXPECT_FLOAT_EQ (120.0f, mainStartOf (listBox->getRowBounds (3)));
    }
}

//==============================================================================
// Layout Tests - both orientations
//==============================================================================

TEST_F (ListBoxTests, LayoutAndHitTestingMatchInBothOrientations)
{
    for (auto orientation : { ListBox::Orientation::vertical, ListBox::Orientation::horizontal })
    {
        useOrientation (orientation);
        listBox->setRowSize (50.0f);
        listBox->setScrollPosition (0.0f);

        const auto bounds = listBox->getRowBounds (3);
        EXPECT_FLOAT_EQ (150.0f, mainStartOf (bounds));
        EXPECT_FLOAT_EQ (50.0f, mainSizeOf (bounds));

        EXPECT_EQ (3, listBox->getRowAt (pointAlong (175.0f)));
        EXPECT_EQ (0, listBox->getVisibleRowRange().getStart());
        EXPECT_EQ (8, listBox->getVisibleRowRange().getEnd());

        listBox->scrollToRow (19);
        EXPECT_FLOAT_EQ (600.0f, listBox->getScrollPosition());
        EXPECT_TRUE (listBox->getVisibleRowRange().contains (19));
        EXPECT_EQ (19, listBox->getRowAt (pointAlong (375.0f)));
    }
}

TEST_F (ListBoxTests, VisibleRangeCalculatedForFixedHeight)
{
    listBox->setRowSize (20.0f);

    auto visibleRange = listBox->getVisibleRowRange();
    auto visibleCount = listBox->getVisibleRowsCount();

    EXPECT_GT (visibleRange.getLength(), 0);
    EXPECT_EQ (visibleRange.getLength(), visibleCount);
}

TEST_F (ListBoxTests, RowSpacingOpensGapsThatHitNoRow)
{
    listBox->setRowSize (20.0f);
    listBox->setRowSpacing (10.0f);

    EXPECT_FLOAT_EQ (10.0f, listBox->getRowSpacing());
    EXPECT_FLOAT_EQ (30.0f, listBox->getRowBounds (1).getY());
    EXPECT_EQ (0, listBox->getRowAt ({ 100.0f, 15.0f }));
    EXPECT_EQ (-1, listBox->getRowAt ({ 100.0f, 25.0f }));
    EXPECT_EQ (1, listBox->getRowAt ({ 100.0f, 35.0f }));
}

TEST_F (ListBoxTests, InsetsAndHeaderAndFooterOffsetTheRows)
{
    auto header = std::make_unique<Component>();
    header->setSize (300.0f, 40.0f);
    auto* headerComponent = header.get();

    auto footer = std::make_unique<Component>();
    footer->setSize (300.0f, 30.0f);
    auto* footerComponent = footer.get();

    listBox->setRowSize (20.0f);
    listBox->setHeaderComponent (std::move (header));
    listBox->setFooterComponent (std::move (footer));
    listBox->setContentInsets (8.0f, 12.0f);

    EXPECT_EQ (headerComponent, listBox->getHeaderComponent());
    EXPECT_EQ (footerComponent, listBox->getFooterComponent());

    EXPECT_FLOAT_EQ (8.0f, headerComponent->getY());
    EXPECT_FLOAT_EQ (40.0f, headerComponent->getHeight());
    EXPECT_FLOAT_EQ (48.0f, listBox->getRowBounds (0).getY());
    EXPECT_EQ (-1, listBox->getRowAt ({ 100.0f, 20.0f }));
    EXPECT_EQ (0, listBox->getRowAt ({ 100.0f, 50.0f }));

    // 8 + 40 + 20 * 20 + 30 + 12 is 490pt of content inside 400pt.
    listBox->setScrollPosition (10000.0f);
    EXPECT_FLOAT_EQ (90.0f, listBox->getScrollPosition());
    EXPECT_FLOAT_EQ (448.0f - 90.0f, footerComponent->getY());
    EXPECT_FLOAT_EQ (30.0f, footerComponent->getHeight());

    listBox->setHeaderComponent (nullptr);
    EXPECT_EQ (nullptr, listBox->getHeaderComponent());
}

//==============================================================================
// Scrolling Tests
//==============================================================================

TEST_F (ListBoxTests, ScrollToRowShowsTheRow)
{
    listBox->setRowSize (20.0f);
    listBox->setBounds (0.0f, 0.0f, 300.0f, 200.0f);

    listBox->scrollToRow (15);

    auto visibleRange = listBox->getVisibleRowRange();
    EXPECT_TRUE (visibleRange.contains (15));
}

TEST_F (ListBoxTests, ScrollToRowAlignments)
{
    // 20 rows of 50pt is 1000pt of content inside a 400pt viewport.
    listBox->setRowSize (50.0f);

    listBox->scrollToRow (10, ListBox::ScrollAlignment::start);
    EXPECT_FLOAT_EQ (500.0f, listBox->getScrollPosition());

    listBox->scrollToRow (10, ListBox::ScrollAlignment::center);
    EXPECT_FLOAT_EQ (325.0f, listBox->getScrollPosition());

    listBox->scrollToRow (10, ListBox::ScrollAlignment::end);
    EXPECT_FLOAT_EQ (150.0f, listBox->getScrollPosition());

    // Row 5 is fully visible at 150, so nearest leaves the view alone.
    listBox->scrollToRow (5, ListBox::ScrollAlignment::nearest);
    EXPECT_FLOAT_EQ (150.0f, listBox->getScrollPosition());

    // Past the end the position clamps.
    listBox->scrollToRow (19, ListBox::ScrollAlignment::start);
    EXPECT_FLOAT_EQ (600.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, AnimatedScrollToRowSettlesOverFrames)
{
    listBox->setRowSize (50.0f);

    listBox->scrollToRow (10, ListBox::ScrollAlignment::start, true);

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
    EXPECT_EQ (ListBox::ScrollState::settling, listBox->getScrollState());

    runFrames (60);

    EXPECT_FLOAT_EQ (500.0f, listBox->getScrollPosition());
    EXPECT_EQ (ListBox::ScrollState::idle, listBox->getScrollState());
    EXPECT_EQ (10, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, ScrollToRowIgnoresRowsTheModelDoesNotHave)
{
    listBox->setRowSize (50.0f);

    listBox->scrollToRow (-1);
    listBox->scrollToRow (1000);

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, GetRowAtReturnsCorrectIndex)
{
    listBox->setRowSize (20.0f);

    EXPECT_EQ (2, listBox->getRowAt (Point<float> (100.0f, 50.0f)));
}

TEST_F (ListBoxTests, BringingAnOffscreenRowIntoViewScrollsTheList)
{
    // 20 rows of 50pt is 1000pt of content inside a 400pt viewport, so the list has room to scroll.
    listBox->setRowSize (50.0f);

    EXPECT_EQ (0, listBox->getVisibleRowRange().getStart());

    // Row 19 sits below the fold, so bringing it into view has to pull the offset down.
    listBox->scrollToRow (19);

    const auto afterScrolling = listBox->getVisibleRowRange().getStart();
    EXPECT_GT (afterScrolling, 0);
    EXPECT_TRUE (listBox->getVisibleRowRange().contains (19));

    // It is on screen now, so asking again must leave the view exactly where it is.
    listBox->scrollToRow (19);
    EXPECT_EQ (afterScrolling, listBox->getVisibleRowRange().getStart());

    // A row above the fold moves the offset back the other way.
    listBox->scrollToRow (0);
    EXPECT_EQ (0, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, WheelScrollingMovesTheViewAndClampsAtTheTop)
{
    // 20 rows of 50pt is 1000pt of content inside a 400pt viewport, so there is somewhere to go.
    listBox->setRowSize (50.0f);

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 10.0f));

    // Which way a notch scrolls follows the platform's wheel convention, so find the direction that
    // moves away from the top and drive that one.
    float awayFromTop = -10000.0f;
    listBox->mouseWheel (event, MouseWheelData (0.0f, awayFromTop));

    if (listBox->getVisibleRowRange().getStart() == 0)
    {
        awayFromTop = 10000.0f;
        listBox->mouseWheel (event, MouseWheelData (0.0f, awayFromTop));
    }

    EXPECT_GT (listBox->getVisibleRowRange().getStart(), 0);

    // A notch the other way comes straight back to the top rather than running off it.
    listBox->mouseWheel (event, MouseWheelData (0.0f, -awayFromTop));
    EXPECT_EQ (0, listBox->getVisibleRowRange().getStart());

    // Once it is there, another notch the same way has nothing left to move.
    listBox->mouseWheel (event, MouseWheelData (0.0f, -awayFromTop));
    EXPECT_EQ (0, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, AVerticalWheelScrollsAHorizontalList)
{
    useOrientation (ListBox::Orientation::horizontal);
    listBox->setRowSize (100.0f);

    const MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (10.0f, 10.0f));

    listBox->mouseWheel (event, MouseWheelData (0.0f, -1.0f));

    if (listBox->getScrollPosition() == 0.0f)
        listBox->mouseWheel (event, MouseWheelData (0.0f, 1.0f));

    // One notch is three rows of the list's own row size.
    EXPECT_FLOAT_EQ (300.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, OverscrollOptionsRoundTrip)
{
    listBox->setScrollOptions (KineticScroller::Options().withOverscrollEnabled (false).withBounceBackTime (0.2f));

    EXPECT_FALSE (listBox->getScrollOptions().overscrollEnabled);
    EXPECT_FLOAT_EQ (0.2f, listBox->getScrollOptions().bounceBackTime);
}

//==============================================================================
// Observation Tests
//==============================================================================

TEST_F (ListBoxTests, OnScrollAndOnScrollStateChangedFollowAnAnimatedScroll)
{
    listBox->setRowSize (50.0f);

    std::vector<float> offsets;
    std::vector<ListBox::ScrollState> states;
    listBox->onScroll = [&] (float offset) { offsets.push_back (offset); };
    listBox->onScrollStateChanged = [&] (ListBox::ScrollState state) { states.push_back (state); };

    listBox->setScrollPosition (100.0f);
    ASSERT_EQ (1u, offsets.size());
    EXPECT_FLOAT_EQ (100.0f, offsets.back());
    EXPECT_TRUE (states.empty());

    listBox->setScrollPosition (300.0f, true);
    runFrames (60);

    EXPECT_GT (offsets.size(), 2u);
    EXPECT_FLOAT_EQ (300.0f, offsets.back());

    ASSERT_EQ (2u, states.size());
    EXPECT_EQ (ListBox::ScrollState::settling, states[0]);
    EXPECT_EQ (ListBox::ScrollState::idle, states[1]);
}

TEST_F (ListBoxTests, OnVisibleRowsChangedReportsTheNewRange)
{
    listBox->setRowSize (50.0f);

    std::vector<Range<int>> ranges;
    listBox->onVisibleRowsChanged = [&] (Range<int> range) { ranges.push_back (range); };

    listBox->setScrollPosition (100.0f);

    ASSERT_EQ (1u, ranges.size());
    EXPECT_EQ (2, ranges[0].getStart());
    EXPECT_EQ (10, ranges[0].getEnd());

    // Same range, no callback.
    listBox->setScrollPosition (101.0f);
    EXPECT_EQ (1u, ranges.size());
}

TEST_F (ListBoxTests, OnEndReachedFiresOnceAndRearmsWhenScrollingBack)
{
    // 100 rows of 20pt is 2000pt; the end is reached within 200pt (half the viewport).
    setNumRows (100);
    listBox->setRowSize (20.0f);

    int calls = 0;
    listBox->onEndReached = [&] { ++calls; };

    listBox->setScrollPosition (1300.0f);
    EXPECT_EQ (0, calls);

    listBox->setScrollPosition (1400.0f);
    EXPECT_EQ (1, calls);

    listBox->setScrollPosition (1500.0f);
    EXPECT_EQ (1, calls);

    listBox->setScrollPosition (0.0f);
    listBox->setScrollPosition (1600.0f);
    EXPECT_EQ (2, calls);
}

TEST_F (ListBoxTests, OnEndReachedFiresForShortContentAndRearmsWhenRowsAreAdded)
{
    TestListBoxModel shortModel (5);

    ListBox list;
    list.setBounds (0.0f, 0.0f, 300.0f, 400.0f);

    int calls = 0;
    list.onEndReached = [&] { ++calls; };

    list.setModel (&shortModel);
    EXPECT_EQ (1, calls);

    list.updateContent();
    EXPECT_EQ (1, calls);

    shortModel.numRows += 5;
    list.rowsInserted (5, 5);
    EXPECT_EQ (2, calls);

    list.setModel (nullptr);
}

TEST_F (ListBoxTests, EndReachedThresholdRoundTrips)
{
    EXPECT_FLOAT_EQ (0.5f, listBox->getEndReachedThreshold());

    listBox->setEndReachedThreshold (2.0f);
    EXPECT_FLOAT_EQ (2.0f, listBox->getEndReachedThreshold());
}

//==============================================================================
// Change Notification Tests
//==============================================================================

TEST_F (ListBoxTests, UpdateContentRereadsTheRowCountAndDropsVanishedSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setSelectedRows ({ 2, 15 }, dontSendNotification);

    setNumRows (10);

    EXPECT_EQ (Array<int> ({ 2 }), selection());
    EXPECT_EQ (1, model->selectionChangedCallCount);
    EXPECT_EQ (-1, listBox->getCurrentRow());
}

TEST_F (ListBoxTests, InsertingRowsShiftsTheSelectionAndTheCurrentRowSilently)
{
    listBox->selectRow (5, false, sendNotification);
    const auto notifications = model->selectionChangedCallCount;

    model->numRows += 2;
    listBox->rowsInserted (3, 2);

    EXPECT_EQ (Array<int> ({ 7 }), selection());
    EXPECT_EQ (7, listBox->getCurrentRow());
    EXPECT_EQ (notifications, model->selectionChangedCallCount);
}

TEST_F (ListBoxTests, InsertingRowsAboveTheViewKeepsTheVisibleRowsInPlace)
{
    setNumRows (50);
    listBox->setRowSize (20.0f);
    listBox->setScrollPosition (200.0f);
    ASSERT_EQ (10, listBox->getVisibleRowRange().getStart());

    model->numRows += 3;
    listBox->rowsInserted (2, 3);

    EXPECT_FLOAT_EQ (260.0f, listBox->getScrollPosition());
    EXPECT_EQ (13, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, InsertingRowsBelowTheViewDoesNotScroll)
{
    setNumRows (50);
    listBox->setRowSize (20.0f);
    listBox->setScrollPosition (200.0f);

    model->numRows += 5;
    listBox->rowsInserted (50, 5);

    EXPECT_FLOAT_EQ (200.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, RemovingRowsAboveTheViewKeepsTheVisibleRowsInPlace)
{
    setNumRows (50);
    listBox->setRowSize (20.0f);
    listBox->setScrollPosition (200.0f);

    model->numRows -= 3;
    listBox->rowsRemoved (2, 3);

    EXPECT_FLOAT_EQ (140.0f, listBox->getScrollPosition());
    EXPECT_EQ (7, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, RemovingASelectedRowNotifies)
{
    listBox->selectRow (3, false, sendNotification);
    const auto notifications = model->selectionChangedCallCount;

    model->numRows -= 1;
    listBox->rowsRemoved (3, 1);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (-1, listBox->getCurrentRow());
    EXPECT_EQ (notifications + 1, model->selectionChangedCallCount);
}

TEST_F (ListBoxTests, RemovingAnUnselectedRowShiftsTheSelectionSilently)
{
    listBox->selectRow (5, false, sendNotification);
    const auto notifications = model->selectionChangedCallCount;

    model->numRows -= 1;
    listBox->rowsRemoved (1, 1);

    EXPECT_EQ (Array<int> ({ 4 }), selection());
    EXPECT_EQ (4, listBox->getCurrentRow());
    EXPECT_EQ (notifications, model->selectionChangedCallCount);
}

TEST_F (ListBoxTests, MovingARowCarriesTheSelectionWithIt)
{
    listBox->selectRow (2, false, dontSendNotification);

    listBox->rowMoved (2, 5);
    EXPECT_EQ (Array<int> ({ 5 }), selection());
    EXPECT_EQ (5, listBox->getCurrentRow());

    // A row between the two positions shifts back by one.
    listBox->selectRow (4, false, dontSendNotification);
    listBox->rowMoved (2, 5);
    EXPECT_EQ (Array<int> ({ 3 }), selection());

    // And forwards by one when the move goes the other way.
    listBox->rowMoved (6, 1);
    EXPECT_EQ (Array<int> ({ 4 }), selection());
}

TEST_F (ListBoxTests, RowsChangedRereadsTheirSizes)
{
    listBox->setRowSize (20.0f);

    model->rowSizes = { 100.0f };
    listBox->rowsChanged (0, 1);

    EXPECT_FLOAT_EQ (100.0f, listBox->getRowBounds (0).getHeight());
    EXPECT_FLOAT_EQ (100.0f, listBox->getRowBounds (1).getY());
}

TEST_F (ListBoxTests, RowsGrowingAboveTheViewKeepTheVisibleRowsInPlace)
{
    setNumRows (50);
    listBox->setRowSize (20.0f);
    listBox->setScrollPosition (200.0f);

    model->rowSizes = { 60.0f };
    listBox->rowsChanged (0, 1);

    EXPECT_FLOAT_EQ (240.0f, listBox->getScrollPosition());
    EXPECT_EQ (10, listBox->getVisibleRowRange().getStart());
}

TEST_F (ListBoxTests, RowsChangedRefreshesVisibleRows)
{
    model->useCustomComponents = true;
    listBox->updateContent();

    const auto refreshes = model->refreshCount;
    listBox->rowsChanged (0, 2);

    EXPECT_EQ (refreshes + 2, model->refreshCount);
}

//==============================================================================
// Row Component Tests
//==============================================================================

TEST_F (ListBoxTests, TheBuiltInItemShowsTheRowTextAndFollowsTheSelection)
{
    auto* item = dynamic_cast<ListBoxItem*> (listBox->getComponentForRow (3));
    ASSERT_NE (nullptr, item);

    EXPECT_EQ ("Row 3", item->getText());
    EXPECT_EQ (ListBoxItem::IconPosition::left, item->getIconPosition());
    EXPECT_FALSE (item->isSelected());

    listBox->selectRow (3, false, dontSendNotification);
    EXPECT_TRUE (item->isSelected());
}

TEST_F (ListBoxTests, AHorizontalListPutsBuiltInIconsAboveTheText)
{
    useOrientation (ListBox::Orientation::horizontal);

    auto* item = dynamic_cast<ListBoxItem*> (listBox->getComponentForRow (0));
    ASSERT_NE (nullptr, item);

    EXPECT_EQ (ListBoxItem::IconPosition::above, item->getIconPosition());
}

TEST_F (ListBoxTests, CustomRowComponentsAreRecycledWhileScrolling)
{
    model->useCustomComponents = true;
    setNumRows (200);
    listBox->setRowSize (20.0f);

    for (int i = 0; i < 100; ++i)
        listBox->setScrollPosition (static_cast<float> (i) * 37.0f);

    // 400pt of 20pt rows shows at most 21 rows at once, so nothing beyond that is ever created.
    EXPECT_LE (model->createdComponents, 21);

    const auto first = listBox->getVisibleRowRange().getStart();
    auto* row = dynamic_cast<TestListBoxModel::RowComponent*> (listBox->getComponentForRow (first));
    ASSERT_NE (nullptr, row);
    EXPECT_EQ (first, row->shownRow);
}

TEST_F (ListBoxTests, SelectionChangesRefreshTheAffectedCustomRows)
{
    model->useCustomComponents = true;
    listBox->updateContent();

    listBox->selectRow (2, false, dontSendNotification);

    auto* row = dynamic_cast<TestListBoxModel::RowComponent*> (listBox->getComponentForRow (2));
    ASSERT_NE (nullptr, row);
    EXPECT_TRUE (row->shownSelected);

    listBox->selectRow (4, false, dontSendNotification);
    EXPECT_FALSE (row->shownSelected);
}

TEST_F (ListBoxTests, UpdateContentKeepsTheRowComponents)
{
    auto* before = listBox->getComponentForRow (0);
    ASSERT_NE (nullptr, before);

    listBox->updateContent();

    EXPECT_EQ (before, listBox->getComponentForRow (0));
}

TEST_F (ListBoxTests, GetComponentForRowReturnsNullForOutOfRange)
{
    listBox->setRowSize (20.0f);

    EXPECT_EQ (nullptr, listBox->getComponentForRow (-1));
    EXPECT_EQ (nullptr, listBox->getComponentForRow (1000));
}

TEST_F (ListBoxTests, GetComponentForRowReturnsNullForInvisibleRow)
{
    setNumRows (100);
    listBox->setRowSize (20.0f);

    EXPECT_EQ (nullptr, listBox->getComponentForRow (50));
}

//==============================================================================
// Model Integration Tests
//==============================================================================

TEST_F (ListBoxTests, ModelCallbacksInvokedOnSelection)
{
    listBox->selectRow (5, false, sendNotification);

    EXPECT_EQ (1, model->selectionChangedCallCount);
    EXPECT_EQ (1, model->lastSelectedRows.size());
    EXPECT_EQ (5, model->lastSelectedRows[0]);
}

TEST_F (ListBoxTests, RowClickCallbackInvoked)
{
    listBox->setRowSize (20.0f);

    int clickedRow = -1;
    listBox->onRowClicked = [&] (int rowIndex) { clickedRow = rowIndex; };

    // Click in the middle of row 5 (y = 100 to 120, so use 110)
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDown (event);

    EXPECT_EQ (5, clickedRow);
    EXPECT_EQ (5, model->lastClickedRow);
    EXPECT_EQ (5, listBox->getCurrentRow());
}

//==============================================================================
// Row Bounds Tests
//==============================================================================

TEST_F (ListBoxTests, GetRowBoundsReturnsValidRectangle)
{
    listBox->setRowSize (25.0f);

    auto bounds = listBox->getRowBounds (0);

    EXPECT_FALSE (bounds.isEmpty());
    EXPECT_EQ (25.0f, bounds.getHeight());
}

TEST_F (ListBoxTests, GetRowBoundsReturnsEmptyForInvalidIndex)
{
    auto bounds = listBox->getRowBounds (-1);
    EXPECT_TRUE (bounds.isEmpty());

    bounds = listBox->getRowBounds (1000);
    EXPECT_TRUE (bounds.isEmpty());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (ListBoxTests, SelectOutOfRangeRowDoesNothing)
{
    listBox->selectRow (-1, false, dontSendNotification);
    EXPECT_EQ (0, listBox->getNumSelectedRows());

    listBox->selectRow (1000, false, dontSendNotification);
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, EmptyModelHandledCorrectly)
{
    auto emptyModel = std::make_unique<TestListBoxModel> (0);
    listBox->setModel (emptyModel.get());

    EXPECT_EQ (0, listBox->getVisibleRowsCount());

    listBox->selectRow (0, false, dontSendNotification);
    EXPECT_EQ (0, listBox->getNumSelectedRows());

    listBox->setModel (nullptr);
}

TEST_F (ListBoxTests, NullModelHandledCorrectly)
{
    listBox->setModel (nullptr);

    EXPECT_EQ (nullptr, listBox->getModel());
    EXPECT_EQ (0, listBox->getVisibleRowsCount());

    listBox->selectRow (0, false, dontSendNotification);
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

//==============================================================================
// Component Lifecycle Tests
//==============================================================================

TEST_F (ListBoxTests, UpdateContentRefreshesVisibleRows)
{
    listBox->setRowSize (20.0f);

    auto visibleCountBefore = listBox->getVisibleRowsCount();
    EXPECT_GT (visibleCountBefore, 0);

    const auto refreshes = model->refreshCount;
    listBox->updateContent();

    EXPECT_EQ (visibleCountBefore, listBox->getVisibleRowsCount());
    EXPECT_GE (model->refreshCount, refreshes + visibleCountBefore);
}

TEST_F (ListBoxTests, ResizeUpdatesVisibleRows)
{
    listBox->setRowSize (10.0f);

    auto visibleCountBefore = listBox->getVisibleRowsCount();

    listBox->setSize (300, 800);

    auto visibleCountAfter = listBox->getVisibleRowsCount();
    EXPECT_GT (visibleCountAfter, visibleCountBefore);
}

//==============================================================================
// Minimum Content Size Tests
//==============================================================================

TEST_F (ListBoxTests, MinimumContentSizeCanBeSet)
{
    listBox->setMinimumContentSize (500);

    EXPECT_EQ (500, listBox->getMinimumContentSize());
}

TEST_F (ListBoxTests, MinimumContentSizeDefaultsToZero)
{
    EXPECT_EQ (0, listBox->getMinimumContentSize());
}

TEST_F (ListBoxTests, MinimumContentSizeMakesShortContentScrollable)
{
    setNumRows (2);
    listBox->setMinimumContentSize (1000);

    listBox->setScrollPosition (10000.0f);
    EXPECT_FLOAT_EQ (600.0f, listBox->getScrollPosition());
}

//==============================================================================
// Scrollbar Tests
//==============================================================================

TEST_F (ListBoxTests, VerticalScrollBarExists)
{
    EXPECT_NE (nullptr, listBox->getVerticalScrollBar());
}

TEST_F (ListBoxTests, HorizontalScrollBarExists)
{
    EXPECT_NE (nullptr, listBox->getHorizontalScrollBar());
}

TEST_F (ListBoxTests, VerticalScrollBarVisibilityCanBeChanged)
{
    listBox->setVerticalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysVisible);

    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysVisible, listBox->getVerticalScrollBar()->getVisibilityMode());
}

TEST_F (ListBoxTests, HorizontalScrollBarVisibilityCanBeChanged)
{
    listBox->setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysVisible);

    EXPECT_EQ (ScrollBar::VisibilityMode::alwaysVisible, listBox->getHorizontalScrollBar()->getVisibilityMode());
}

TEST_F (ListBoxTests, OnlyTheScrollBarAlongTheScrollAxisIsUsed)
{
    listBox->setRowSize (50.0f);
    EXPECT_TRUE (listBox->getVerticalScrollBar()->isVisible());
    EXPECT_FALSE (listBox->getHorizontalScrollBar()->isVisible());

    useOrientation (ListBox::Orientation::horizontal);
    EXPECT_FALSE (listBox->getVerticalScrollBar()->isVisible());
    EXPECT_TRUE (listBox->getHorizontalScrollBar()->isVisible());
}

TEST_F (ListBoxTests, TheScrollBarFollowsTheScrollPosition)
{
    listBox->setRowSize (50.0f);
    listBox->setScrollPosition (250.0f);

    EXPECT_DOUBLE_EQ (250.0, listBox->getVerticalScrollBar()->getCurrentRangeStart());
    EXPECT_DOUBLE_EQ (1000.0, listBox->getVerticalScrollBar()->getRangeMaximum());
}

//==============================================================================
// Callback Tests
//==============================================================================

TEST_F (ListBoxTests, OnSelectionChangedCallbackInvoked)
{
    int callbackCount = 0;
    listBox->onSelectionChanged = [&callbackCount]()
    {
        callbackCount++;
    };

    listBox->selectRow (5, false, sendNotification);

    EXPECT_EQ (1, callbackCount);
}

TEST_F (ListBoxTests, OnSelectionChangedNotInvokedWithDontSendNotification)
{
    int callbackCount = 0;
    listBox->onSelectionChanged = [&callbackCount]()
    {
        callbackCount++;
    };

    listBox->selectRow (5, false, dontSendNotification);

    EXPECT_EQ (0, callbackCount);
}

TEST_F (ListBoxTests, OnRowDoubleClickedCallbackInvoked)
{
    int callbackCount = 0;
    int lastDoubleClickedRow = -1;

    listBox->onRowDoubleClicked = [&callbackCount, &lastDoubleClickedRow] (int rowIndex)
    {
        callbackCount++;
        lastDoubleClickedRow = rowIndex;
    };

    listBox->setRowSize (20.0f);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDoubleClick (event);

    EXPECT_EQ (1, callbackCount);
    EXPECT_EQ (5, lastDoubleClickedRow);
}

TEST_F (ListBoxTests, ModelDoubleClickCallbackInvoked)
{
    listBox->setRowSize (20.0f);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDoubleClick (event);

    EXPECT_EQ (1, model->doubleClickCallCount);
    EXPECT_EQ (5, model->lastDoubleClickedRow);
}

//==============================================================================
// Keyboard Navigation Tests
//==============================================================================

TEST_F (ListBoxTests, DownArrowKeySelectsNextRow)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());

    EXPECT_EQ (Array<int> ({ 6 }), selection());
    EXPECT_EQ (6, listBox->getCurrentRow());
}

TEST_F (ListBoxTests, UpArrowKeySelectsPreviousRow)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::upKey), Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (4));
}

TEST_F (ListBoxTests, UpArrowKeyDoesNotWrapBelowZero)
{
    listBox->selectRow (0, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::upKey), Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (0));
}

TEST_F (ListBoxTests, DownArrowKeyDoesNotWrapBeyondEnd)
{
    listBox->selectRow (19, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (19));
}

TEST_F (ListBoxTests, TheFirstArrowPressStartsAtTheFirstRow)
{
    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());

    EXPECT_EQ (0, listBox->getCurrentRow());
    EXPECT_TRUE (listBox->isRowSelected (0));
}

TEST_F (ListBoxTests, AHorizontalListNavigatesWithLeftAndRight)
{
    useOrientation (ListBox::Orientation::horizontal);
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::rightKey), Point<float>());
    EXPECT_EQ (6, listBox->getCurrentRow());

    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());
    EXPECT_EQ (6, listBox->getCurrentRow());

    listBox->keyDown (KeyPress (KeyPress::leftKey), Point<float>());
    EXPECT_EQ (5, listBox->getCurrentRow());
    EXPECT_TRUE (listBox->isRowSelected (5));
}

TEST_F (ListBoxTests, ArrowsScrollTheCurrentRowIntoView)
{
    listBox->setRowSize (50.0f);
    listBox->selectRow (7, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());

    EXPECT_FLOAT_EQ (50.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, PageDownKeyScrollsMultipleRows)
{
    listBox->setRowSize (20.0f);
    listBox->selectRow (0, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::pageDownKey), Point<float>());

    EXPECT_GT (listBox->getSelectedRow(), 0);
}

TEST_F (ListBoxTests, PageUpKeyScrollsMultipleRows)
{
    listBox->setRowSize (20.0f);
    listBox->selectRow (19, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::pageUpKey), Point<float>());

    EXPECT_LT (listBox->getSelectedRow(), 19);
}

TEST_F (ListBoxTests, HomeKeySelectsFirstRow)
{
    listBox->selectRow (10, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::homeKey), Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (0));
}

TEST_F (ListBoxTests, EndKeySelectsLastRow)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::endKey), Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (19));
}

TEST_F (ListBoxTests, ArrowsReplaceAMultipleSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setSelectedRows ({ 2, 5 }, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::downKey), Point<float>());

    EXPECT_EQ (Array<int> ({ 6 }), selection());
}

TEST_F (ListBoxTests, CommandArrowMovesTheCurrentRowWithoutSelecting)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::downKey, KeyModifiers (KeyModifiers::commandMask)), Point<float>());

    EXPECT_EQ (6, listBox->getCurrentRow());
    EXPECT_EQ (Array<int> ({ 5 }), selection());
}

TEST_F (ListBoxTests, SpaceTogglesTheCurrentRow)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->keyDown (KeyPress (KeyPress::downKey, KeyModifiers (KeyModifiers::commandMask)), Point<float>());

    listBox->keyDown (KeyPress (KeyPress::spaceKey), Point<float>());
    EXPECT_EQ (Array<int> ({ 5, 6 }), selection());

    listBox->keyDown (KeyPress (KeyPress::spaceKey), Point<float>());
    EXPECT_EQ (Array<int> ({ 5 }), selection());
}

TEST_F (ListBoxTests, ShiftArrowExtendsTheSelectionFromTheAnchor)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);

    const auto shift = KeyModifiers (KeyModifiers::shiftMask);
    listBox->keyDown (KeyPress (KeyPress::downKey, shift), Point<float>());
    listBox->keyDown (KeyPress (KeyPress::downKey, shift), Point<float>());

    EXPECT_EQ (Array<int> ({ 5, 6, 7 }), selection());
    EXPECT_EQ (7, listBox->getCurrentRow());

    listBox->keyDown (KeyPress (KeyPress::upKey, shift), Point<float>());
    listBox->keyDown (KeyPress (KeyPress::upKey, shift), Point<float>());
    listBox->keyDown (KeyPress (KeyPress::upKey, shift), Point<float>());

    EXPECT_EQ (Array<int> ({ 4, 5 }), selection());
}

TEST_F (ListBoxTests, ReturnKeyPassesTheCurrentRow)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::enterKey), Point<float>());
    EXPECT_EQ (1, model->returnKeyCallCount);
    EXPECT_EQ (5, model->lastReturnKeyRow);

    listBox->setCurrentRow (8);
    listBox->keyDown (KeyPress (KeyPress::enterKey), Point<float>());
    EXPECT_EQ (8, model->lastReturnKeyRow);

    // Return reports, it does not select.
    EXPECT_EQ (Array<int> ({ 5 }), selection());
}

TEST_F (ListBoxTests, ReturnKeyWithNoCurrentRowPassesMinusOne)
{
    listBox->keyDown (KeyPress (KeyPress::enterKey), Point<float>());

    EXPECT_EQ (1, model->returnKeyCallCount);
    EXPECT_EQ (-1, model->lastReturnKeyRow);
}

TEST_F (ListBoxTests, ModelDeleteKeyPressedCallbackInvoked)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::deleteKey), Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (1, model->lastDeleteKeyRows.size());
    EXPECT_EQ (5, model->lastDeleteKeyRows[0]);
}

TEST_F (ListBoxTests, ModelBackspaceKeyPressedCallbackInvoked)
{
    listBox->selectRow (5, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::backspaceKey), Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (1, model->lastDeleteKeyRows.size());
    EXPECT_EQ (5, model->lastDeleteKeyRows[0]);
}

TEST_F (ListBoxTests, ModelDeleteKeyPressedWithMultipleSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);
    listBox->selectRow (15, false, dontSendNotification);

    listBox->keyDown (KeyPress (KeyPress::deleteKey), Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (3, model->lastDeleteKeyRows.size());
}

//==============================================================================
// Current Row Tests
//==============================================================================

TEST_F (ListBoxTests, SetCurrentRowNotifiesAndIgnoresRowsTheModelDoesNotHave)
{
    std::vector<int> notified;
    listBox->onCurrentRowChanged = [&] (int row) { notified.push_back (row); };

    listBox->setCurrentRow (3);
    listBox->setCurrentRow (3);
    listBox->setCurrentRow (50);
    listBox->setCurrentRow (4, dontSendNotification);
    listBox->setCurrentRow (-1);

    EXPECT_EQ (std::vector<int> ({ 3, -1 }), notified);
    EXPECT_EQ (-1, listBox->getCurrentRow());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

//==============================================================================
// Repaint and Focus Tests
//==============================================================================

TEST_F (ListBoxTests, RepaintRowDoesNotCrash)
{
    listBox->repaintRow (5);
    listBox->repaintRow (-1);
    listBox->repaintRow (1000);

    SUCCEED();
}

TEST_F (ListBoxTests, FocusChangesDoNotCrash)
{
    listBox->focusGained();
    listBox->focusLost();

    SUCCEED();
}

//==============================================================================
// Hover Tests
//==============================================================================

TEST_F (ListBoxTests, TheMouseHoversRowsButATouchDoesNot)
{
    auto* item = dynamic_cast<ListBoxItem*> (listBox->getComponentForRow (2));
    ASSERT_NE (nullptr, item);

    const auto overRow = listBox->getRowBounds (2).getCenter();

    listBox->mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), overRow).withTouchIndex (0));
    EXPECT_FALSE (item->isHovered());

    listBox->mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), overRow));
    EXPECT_TRUE (item->isHovered());
}

TEST_F (ListBoxTests, TheHoverGoesWhenTheMouseLeavesTheList)
{
    auto* item = dynamic_cast<ListBoxItem*> (listBox->getComponentForRow (2));
    ASSERT_NE (nullptr, item);

    listBox->mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), listBox->getRowBounds (2).getCenter()));
    ASSERT_TRUE (item->isHovered());

    listBox->mouseExit (MouseEvent (MouseEvent::noButtons, KeyModifiers(), Point<float> (-10.0f, -10.0f)));
    EXPECT_FALSE (item->isHovered());
}

//==============================================================================
// Touch Tests
//==============================================================================

TEST_F (ListBoxTests, ATouchDragScrollsWithoutSelecting)
{
    listBox->setRowSize (50.0f);

    swipe (300.0f, 100.0f, false);

    // The first 10pt are the slop, the rest moves the content with the finger.
    EXPECT_FLOAT_EQ (190.0f, listBox->getScrollPosition());
    EXPECT_EQ (ListBox::ScrollState::dragging, listBox->getScrollState());

    listBox->mouseUp (touchAt (pointAlong (100.0f)));

    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (0, model->clickCallCount);
    EXPECT_EQ (ListBox::ScrollState::settling, listBox->getScrollState());
}

TEST_F (ListBoxTests, TouchScrollingWorksInBothOrientations)
{
    for (auto orientation : { ListBox::Orientation::vertical, ListBox::Orientation::horizontal })
    {
        useOrientation (orientation);
        listBox->setRowSize (50.0f);
        listBox->setScrollPosition (0.0f);

        swipe (300.0f, 100.0f, false);
        EXPECT_FLOAT_EQ (190.0f, listBox->getScrollPosition());

        listBox->mouseUp (touchAt (pointAlong (100.0f)));
        runFrames (600);
    }
}

TEST_F (ListBoxTests, AFlingKeepsScrollingAfterReleaseAndSettles)
{
    listBox->setRowSize (50.0f);

    std::vector<ListBox::ScrollState> states;
    listBox->onScrollStateChanged = [&] (ListBox::ScrollState state) { states.push_back (state); };

    swipe (300.0f, 150.0f);
    const auto released = listBox->getScrollPosition();

    runFrames (5);
    EXPECT_GT (listBox->getScrollPosition(), released);

    runFrames (600);

    EXPECT_EQ (ListBox::ScrollState::idle, listBox->getScrollState());
    ASSERT_EQ (3u, states.size());
    EXPECT_EQ (ListBox::ScrollState::dragging, states[0]);
    EXPECT_EQ (ListBox::ScrollState::settling, states[1]);
    EXPECT_EQ (ListBox::ScrollState::idle, states[2]);
}

TEST_F (ListBoxTests, ATapSelectsOnRelease)
{
    const auto position = listBox->getRowBounds (2).getCenter();

    listBox->mouseDown (touchAt (position));
    EXPECT_EQ (0, listBox->getNumSelectedRows());

    listBox->mouseUp (touchAt (position));
    EXPECT_EQ (Array<int> ({ 2 }), selection());
    EXPECT_EQ (2, listBox->getCurrentRow());
    EXPECT_EQ (1, model->clickCallCount);
}

TEST_F (ListBoxTests, ATapTogglesInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    tapRow (2);
    tapRow (4);
    EXPECT_EQ (Array<int> ({ 2, 4 }), selection());

    tapRow (2);
    EXPECT_EQ (Array<int> ({ 4 }), selection());
}

TEST_F (ListBoxTests, APressDuringAFlingStopsItWithoutSelecting)
{
    listBox->setRowSize (50.0f);

    swipe (300.0f, 150.0f);
    runFrames (2);
    ASSERT_EQ (ListBox::ScrollState::settling, listBox->getScrollState());

    const auto position = pointAlong (200.0f);
    listBox->mouseDown (touchAt (position));
    const auto caughtAt = listBox->getScrollPosition();

    runFrames (5);
    EXPECT_FLOAT_EQ (caughtAt, listBox->getScrollPosition());

    listBox->mouseUp (touchAt (position));
    runFrames (5);

    EXPECT_FLOAT_EQ (caughtAt, listBox->getScrollPosition());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, OnlyTheFirstFingerCounts)
{
    listBox->setRowSize (50.0f);

    listBox->mouseDown (touchAt (pointAlong (300.0f)));
    listBox->mouseDown (touchAt (pointAlong (200.0f), 1));
    runFrames (1);
    listBox->mouseDrag (touchAt (pointAlong (100.0f), 1));

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());

    listBox->mouseUp (touchAt (pointAlong (100.0f), 1));
    listBox->mouseUp (touchAt (pointAlong (300.0f)));
}

TEST_F (ListBoxTests, MovingAcrossTheListNeitherScrollsNorTaps)
{
    listBox->setRowSize (50.0f);

    listBox->mouseDown (touchAt ({ 100.0f, 125.0f }));
    listBox->mouseDrag (touchAt ({ 160.0f, 130.0f }));
    listBox->mouseDrag (touchAt ({ 160.0f, 200.0f }));
    listBox->mouseUp (touchAt ({ 160.0f, 125.0f }));

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, MovingAcrossANestedListHandsTheGestureToTheParentList)
{
    listBox->setRowSize (50.0f);

    TestListBoxModel childModel (10);
    ListBox child ({}, ListBox::Orientation::horizontal);
    child.setBounds (0.0f, 0.0f, 280.0f, 100.0f);
    child.setModel (&childModel);
    listBox->addAndMakeVisible (child);

    child.mouseDown (touchAt ({ 150.0f, 80.0f }));
    runFrames (1);
    child.mouseDrag (touchAt ({ 150.0f, 50.0f }));
    child.mouseDrag (touchAt ({ 150.0f, 30.0f }));

    // 50pt up, less the 10pt slop, now moves the parent; the child stays put.
    EXPECT_FLOAT_EQ (40.0f, listBox->getScrollPosition());
    EXPECT_FLOAT_EQ (0.0f, child.getScrollPosition());
    EXPECT_EQ (ListBox::ScrollState::dragging, listBox->getScrollState());

    child.mouseUp (touchAt ({ 150.0f, 30.0f }));
    EXPECT_NE (ListBox::ScrollState::dragging, listBox->getScrollState());
    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (0, child.getNumSelectedRows());

    child.setModel (nullptr);
}

TEST_F (ListBoxTests, ALongPressSelectsAndOwnsTheRestOfTheGesture)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setRowSize (50.0f);

    const auto position = listBox->getRowBounds (3).getCenter();
    listBox->mouseDown (touchAt (position));
    runFrames (40);

    EXPECT_EQ (Array<int> ({ 3 }), selection());
    EXPECT_EQ (3, listBox->getCurrentRow());

    // It no longer scrolls, and its release is not a tap that would toggle the row back off.
    listBox->mouseDrag (touchAt (position.translated (0.0f, -100.0f)));
    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());

    listBox->mouseUp (touchAt (position));
    EXPECT_EQ (Array<int> ({ 3 }), selection());
    EXPECT_EQ (0, model->clickCallCount);
}

TEST_F (ListBoxTests, TheReleaseCompletingADoubleTapDoesNotToggle)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    tapRow (3);
    ASSERT_EQ (Array<int> ({ 3 }), selection());

    const auto position = listBox->getRowBounds (3).getCenter();
    listBox->mouseDown (touchAt (position));
    listBox->mouseDoubleClick (touchAt (position));
    listBox->mouseUp (touchAt (position));

    EXPECT_EQ (Array<int> ({ 3 }), selection());
    EXPECT_EQ (1, model->doubleClickCallCount);
}

TEST_F (ListBoxTests, DraggingPastTheStartOverscrollsAndBouncesBack)
{
    listBox->setRowSize (50.0f);

    swipe (100.0f, 300.0f, false);
    EXPECT_LT (listBox->getScrollPosition(), 0.0f);

    runFrames (10);
    listBox->mouseUp (touchAt (pointAlong (300.0f)));
    runFrames (120);

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, WithOverscrollDisabledATouchDragClamps)
{
    listBox->setRowSize (50.0f);
    listBox->setScrollOptions (KineticScroller::Options().withOverscrollEnabled (false));

    swipe (100.0f, 300.0f, false);

    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());

    listBox->mouseUp (touchAt (pointAlong (300.0f)));
}

TEST_F (ListBoxTests, TapsDoNotMoveTheList)
{
    listBox->setRowSize (50.0f);
    listBox->setScrollPosition (27.0f);

    tapRow (3);
    runFrames (60);

    EXPECT_FLOAT_EQ (27.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, PullToRefreshFiresHoldsAndReleases)
{
    int refreshes = 0;
    listBox->onRefresh = [&] { ++refreshes; };
    listBox->setPullToRefreshEnabled (true);
    EXPECT_TRUE (listBox->isPullToRefreshEnabled());

    swipe (50.0f, 350.0f, false);
    EXPECT_FLOAT_EQ (1.0f, listBox->getPullToRefreshProgress());

    runFrames (10);
    listBox->mouseUp (touchAt (pointAlong (350.0f)));

    EXPECT_EQ (1, refreshes);
    EXPECT_TRUE (listBox->isRefreshing());

    runFrames (120);
    EXPECT_FLOAT_EQ (-listBox->getRefreshIndicatorSize(), listBox->getScrollPosition());

    listBox->setRefreshing (false);
    runFrames (120);

    EXPECT_FALSE (listBox->isRefreshing());
    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
    EXPECT_FLOAT_EQ (0.0f, listBox->getPullToRefreshProgress());
}

TEST_F (ListBoxTests, AShortPullDoesNotRefresh)
{
    int refreshes = 0;
    listBox->onRefresh = [&] { ++refreshes; };
    listBox->setPullToRefreshEnabled (true);

    swipe (50.0f, 80.0f, false);
    EXPECT_GT (listBox->getPullToRefreshProgress(), 0.0f);
    EXPECT_LT (listBox->getPullToRefreshProgress(), 1.0f);

    runFrames (10);
    listBox->mouseUp (touchAt (pointAlong (80.0f)));
    runFrames (120);

    EXPECT_EQ (0, refreshes);
    EXPECT_FALSE (listBox->isRefreshing());
    EXPECT_FLOAT_EQ (0.0f, listBox->getScrollPosition());
}

TEST_F (ListBoxTests, MouseDragScrollingMakesTheMouseActLikeAFinger)
{
    listBox->setRowSize (50.0f);
    listBox->setMouseDragScrollingEnabled (true);
    EXPECT_TRUE (listBox->isMouseDragScrollingEnabled());

    const auto mouseAt = [] (Point<float> position)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), position);
    };

    listBox->mouseDown (mouseAt (pointAlong (300.0f)));
    runFrames (1);
    listBox->mouseDrag (mouseAt (pointAlong (100.0f)));

    EXPECT_FLOAT_EQ (190.0f, listBox->getScrollPosition());
    EXPECT_EQ (0, listBox->getNumSelectedRows());

    runFrames (10);
    listBox->mouseUp (mouseAt (pointAlong (100.0f)));
    runFrames (120);

    // A click still selects, on release.
    const auto overRow = listBox->getRowBounds (5).getCenter();
    listBox->mouseDown (mouseAt (overRow));
    EXPECT_EQ (0, listBox->getNumSelectedRows());

    listBox->mouseUp (mouseAt (overRow));
    EXPECT_EQ (Array<int> ({ 5 }), selection());
}

//==============================================================================
// Multiple Selection with Modifiers Tests
//==============================================================================

TEST_F (ListBoxTests, SetSelectedRowsReplacesExistingSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    Array<int> newSelection { 1, 2, 3 };
    listBox->setSelectedRows (newSelection, dontSendNotification);

    EXPECT_EQ (3, listBox->getNumSelectedRows());
    EXPECT_FALSE (listBox->isRowSelected (5));
    EXPECT_FALSE (listBox->isRowSelected (10));
    EXPECT_TRUE (listBox->isRowSelected (1));
    EXPECT_TRUE (listBox->isRowSelected (2));
    EXPECT_TRUE (listBox->isRowSelected (3));
}

TEST_F (ListBoxTests, SelectingEmptyArrayClearsSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    listBox->setSelectedRows ({}, dontSendNotification);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, GetSelectedRowReturnsMinusOneForMultipleSelections)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    EXPECT_EQ (-1, listBox->getSelectedRow());
}

//==============================================================================
// Drag and Drop Tests
//==============================================================================

TEST_F (ListBoxTests, ModelDragSourceDescriptionReturnsEmpty)
{
    auto dragData = model->getDragSourceDescription ({ 5 });

    EXPECT_TRUE (dragData.isVoid());
    EXPECT_EQ (1, model->dragSourceCallCount);
}

TEST_F (ListBoxTests, ModelDragSourceDescriptionReturnsData)
{
    model->shouldSupportDrag = true;

    auto dragData = model->getDragSourceDescription ({ 5 });

    EXPECT_EQ ("DragData", dragData.toString());
    EXPECT_EQ (1, model->dragSourceCallCount);
}

TEST_F (ListBoxTests, DragSourceCanBeDisabled)
{
    EXPECT_TRUE (listBox->isDragSourceEnabled());

    listBox->setDragSourceEnabled (false);
    EXPECT_FALSE (listBox->isDragSourceEnabled());

    listBox->setDragSourceEnabled (true);
    EXPECT_TRUE (listBox->isDragSourceEnabled());
}

TEST_F (ListBoxTests, DefaultDragSourceComponentCarriesTheSelectionCount)
{
    auto dragImage = listBox->createDragSourceComponent ({ 2, 5, 7 });

    ASSERT_NE (nullptr, dragImage);

    // The drag image becomes the whole of the ghost window, so it has to be sized by whoever
    // creates it.
    EXPECT_GT (dragImage->getWidth(), 0.0f);
    EXPECT_GT (dragImage->getHeight(), 0.0f);
}

//==============================================================================
// Mouse Selection Interaction Tests
//==============================================================================

TEST_F (ListBoxTests, PlainClickReplacesTheSelectionInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setSelectedRows ({ 5, 10 }, dontSendNotification);

    listBox->mouseDown (makeRowClick (15));

    // selectRow() only ever adds in multiple mode, so without clearing first this would have
    // accumulated into { 5, 10, 15 }.
    EXPECT_EQ (Array<int> ({ 15 }), selection());
}

TEST_F (ListBoxTests, ShiftClickExtendsARangeFromTheLastPlainClick)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->mouseDown (makeRowClick (2));
    EXPECT_EQ (Array<int> ({ 2 }), selection());

    listBox->mouseDown (makeRowClick (5, KeyModifiers (KeyModifiers::shiftMask)));
    EXPECT_EQ (Array<int> ({ 2, 3, 4, 5 }), selection());
    EXPECT_EQ (5, listBox->getCurrentRow());
}

TEST_F (ListBoxTests, RepeatedShiftClicksKeepTheOriginalAnchor)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->mouseDown (makeRowClick (2));
    listBox->mouseDown (makeRowClick (5, KeyModifiers (KeyModifiers::shiftMask)));

    // A shift-click must not move the anchor, so this shrinks the range back towards row 2 rather
    // than extending from row 5.
    listBox->mouseDown (makeRowClick (3, KeyModifiers (KeyModifiers::shiftMask)));

    EXPECT_EQ (Array<int> ({ 2, 3 }), selection());
}

TEST_F (ListBoxTests, CommandClickTogglesARow)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setSelectedRows ({ 2 }, dontSendNotification);

    listBox->mouseDown (makeRowClick (5, KeyModifiers (KeyModifiers::commandMask)));
    EXPECT_EQ (Array<int> ({ 2, 5 }), selection());

    listBox->mouseDown (makeRowClick (5, KeyModifiers (KeyModifiers::commandMask)));
    EXPECT_EQ (Array<int> ({ 2 }), selection());
}

TEST_F (ListBoxTests, ClickingASelectedRowDefersTheCollapseToMouseUp)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setSelectedRows ({ 2, 5 }, dontSendNotification);

    // The press may be the start of a drag carrying the whole selection, so nothing collapses yet.
    listBox->mouseDown (makeRowClick (5));
    EXPECT_EQ (Array<int> ({ 2, 5 }), selection());

    listBox->mouseUp (makeRowClick (5));
    EXPECT_EQ (Array<int> ({ 5 }), selection());
}

TEST_F (ListBoxTests, ClickingASpacingGapSelectsNothing)
{
    listBox->setRowSize (20.0f);
    listBox->setRowSpacing (10.0f);

    listBox->mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 25.0f)));

    EXPECT_EQ (0, listBox->getNumSelectedRows());
    EXPECT_EQ (0, model->clickCallCount);
}

TEST_F (ListBoxTests, MouseUpAndMoveOutsideRowsDoNotCrash)
{
    EXPECT_NO_THROW (listBox->mouseUp (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (50, 50))));
    EXPECT_NO_THROW (listBox->mouseMove (MouseEvent (MouseEvent::noButtons, KeyModifiers(), Point<float> (50, 1000))));
}

//==============================================================================
// ListBoxModel - the hooks a text-only model does not override
//==============================================================================

namespace
{

/** The barest model that compiles: only getNumRows() is pure virtual, so everything else here is
    the inherited default. */
class BareListBoxModel : public ListBoxModel
{
public:
    int getNumRows() override { return 0; }

    using ListBoxModel::reuseOrCreate;
};

} // namespace

TEST (ListBoxModelTests, TheInheritedDefaultsAreInert)
{
    BareListBoxModel model;

    EXPECT_FLOAT_EQ (0.0f, model.getRowSize (0));
    EXPECT_EQ (String(), model.getRowText (0));
    EXPECT_FALSE (model.getRowIcon (0).isValid());

    // Leaving the component null is what asks for the built-in row.
    std::unique_ptr<Component> component;
    model.refreshRowComponent (0, false, component);
    EXPECT_EQ (nullptr, component);

    Array<int> selection;
    selection.add (0);

    // A model that offers nothing to drag gets an empty var.
    EXPECT_TRUE (model.getDragSourceDescription (selection).isVoid());

    // Nothing to notify - and notifying anyway is safe.
    const MouseEvent event;
    model.selectedRowsChanged (selection);
    model.rowClicked (0, event);
    model.rowDoubleClicked (0, event);
    model.returnKeyPressed (0);
    model.deleteKeyPressed (selection);
}

TEST (ListBoxModelTests, ReuseOrCreateReusesAMatchingComponentAndReplacesAnyOther)
{
    std::unique_ptr<Component> component;

    auto& item = BareListBoxModel::reuseOrCreate<ListBoxItem> (component);
    EXPECT_EQ (&item, component.get());

    auto& again = BareListBoxModel::reuseOrCreate<ListBoxItem> (component);
    EXPECT_EQ (&item, &again);

    auto& label = BareListBoxModel::reuseOrCreate<Label> (component);
    EXPECT_EQ (&label, component.get());
    EXPECT_EQ (nullptr, dynamic_cast<ListBoxItem*> (component.get()));
}
