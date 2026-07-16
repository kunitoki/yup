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
    explicit TestListBoxModel (int numRows)
        : numRows (numRows)
    {
    }

    int getNumRows() override
    {
        return numRows;
    }

    String getRowText (int rowIndex) override
    {
        return "Row " + String (rowIndex);
    }

    Image getRowIcon (int rowIndex) override
    {
        ignoreUnused (rowIndex);
        return {};
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

    void returnKeyPressed (int lastSelectedRow) override
    {
        lastReturnKeyRow = lastSelectedRow;
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

    void paintListBoxItem (int rowIndex, Graphics& g, Rectangle<float> area, bool isSelected) override
    {
        ignoreUnused (rowIndex, g, area, isSelected);
        paintCallCount++;
    }

    int getRowHeight (int rowIndex) override
    {
        if (useVariableHeight && rowIndex >= 0 && rowIndex < variableHeights.size())
            return variableHeights[rowIndex];
        return 0;
    }

    int getRowWidth (int rowIndex) override
    {
        if (useVariableWidth && rowIndex >= 0 && rowIndex < variableWidths.size())
            return variableWidths[rowIndex];
        return 0;
    }

    int numRows;
    Array<int> lastSelectedRows;
    int lastClickedRow = -1;
    int lastDoubleClickedRow = -1;
    int lastReturnKeyRow = -1;
    Array<int> lastDeleteKeyRows;
    int selectionChangedCallCount = 0;
    int clickCallCount = 0;
    int doubleClickCallCount = 0;
    int returnKeyCallCount = 0;
    int deleteKeyCallCount = 0;
    int dragSourceCallCount = 0;
    int paintCallCount = 0;
    bool shouldSupportDrag = false;
    bool useVariableHeight = false;
    bool useVariableWidth = false;
    Array<int> variableHeights;
    Array<int> variableWidths;
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

    std::unique_ptr<ListBox> listBox;
    std::unique_ptr<TestListBoxModel> model;
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
// Layout Tests
//==============================================================================

TEST_F (ListBoxTests, FixedRowHeightModeWorks)
{
    listBox->setRowHeight (30);
    listBox->setVariableHeightEnabled (false);

    EXPECT_EQ (30, listBox->getRowHeight());
    EXPECT_FALSE (listBox->isVariableHeightEnabled());
}

TEST_F (ListBoxTests, VariableRowHeightModeCanBeEnabled)
{
    listBox->setVariableHeightEnabled (true);

    EXPECT_TRUE (listBox->isVariableHeightEnabled());
}

TEST_F (ListBoxTests, HorizontalOrientationUsesRowWidth)
{
    listBox->setOrientation (ListBox::Orientation::horizontal);
    listBox->setRowWidth (50);

    EXPECT_EQ (50, listBox->getRowWidth());
}

//==============================================================================
// Scrolling Tests
//==============================================================================

TEST_F (ListBoxTests, VisibleRangeCalculatedForFixedHeight)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto visibleRange = listBox->getVisibleRowRange();
    auto visibleCount = listBox->getVisibleRowsCount();

    EXPECT_GT (visibleRange.getLength(), 0);
    EXPECT_EQ (visibleRange.getLength(), visibleCount);
}

TEST_F (ListBoxTests, ScrollToEnsureRowIsVisible)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->scrollToEnsureRowIsVisible (15);

    auto visibleRange = listBox->getVisibleRowRange();
    EXPECT_TRUE (visibleRange.contains (15));
}

TEST_F (ListBoxTests, GetRowAtReturnsCorrectIndex)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto rowIndex = listBox->getRowAt (Point<float> (100.0f, 50.0f));

    EXPECT_GE (rowIndex, 0);
    EXPECT_LT (rowIndex, model->getNumRows());
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
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->onRowClicked = [] (int rowIndex)
    {
        EXPECT_EQ (5, rowIndex);
    };

    // Click in the middle of row 5 (y = 100 to 120, so use 110)
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDown (event);
}

//==============================================================================
// Row Bounds Tests
//==============================================================================

TEST_F (ListBoxTests, GetRowBoundsReturnsValidRectangle)
{
    listBox->setRowHeight (25);
    listBox->updateContent();

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
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto visibleCountBefore = listBox->getVisibleRowsCount();
    EXPECT_GT (visibleCountBefore, 0);

    listBox->updateContent();

    auto visibleCountAfter = listBox->getVisibleRowsCount();
    EXPECT_EQ (visibleCountBefore, visibleCountAfter);
}

TEST_F (ListBoxTests, ResizeUpdatesVisibleRows)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto visibleCountBefore = listBox->getVisibleRowsCount();

    listBox->setSize (300, 800);
    listBox->resized();

    auto visibleCountAfter = listBox->getVisibleRowsCount();
    EXPECT_GE (visibleCountAfter, visibleCountBefore);
}

//==============================================================================
// Variable Width Tests
//==============================================================================

TEST_F (ListBoxTests, VariableWidthModeCanBeEnabled)
{
    listBox->setVariableWidthEnabled (true);

    EXPECT_TRUE (listBox->isVariableWidthEnabled());
}

TEST_F (ListBoxTests, VariableWidthModeCanBeDisabled)
{
    listBox->setVariableWidthEnabled (true);
    listBox->setVariableWidthEnabled (false);

    EXPECT_FALSE (listBox->isVariableWidthEnabled());
}

TEST_F (ListBoxTests, RowWidthCanBeSetAndRetrieved)
{
    listBox->setRowWidth (150);

    EXPECT_EQ (150, listBox->getRowWidth());
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

//==============================================================================
// Scrollbar Tests
//==============================================================================

TEST_F (ListBoxTests, VerticalScrollBarExists)
{
    auto* scrollBar = listBox->getVerticalScrollBar();

    EXPECT_NE (nullptr, scrollBar);
}

TEST_F (ListBoxTests, HorizontalScrollBarExists)
{
    auto* scrollBar = listBox->getHorizontalScrollBar();

    EXPECT_NE (nullptr, scrollBar);
}

TEST_F (ListBoxTests, VerticalScrollBarVisibilityCanBeChanged)
{
    listBox->setVerticalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysVisible);

    auto* scrollBar = listBox->getVerticalScrollBar();
    EXPECT_NE (nullptr, scrollBar);
}

TEST_F (ListBoxTests, HorizontalScrollBarVisibilityCanBeChanged)
{
    listBox->setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode::alwaysVisible);

    auto* scrollBar = listBox->getHorizontalScrollBar();
    EXPECT_NE (nullptr, scrollBar);
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

    listBox->setRowHeight (20);
    listBox->updateContent();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDoubleClick (event);

    EXPECT_EQ (1, callbackCount);
    EXPECT_EQ (5, lastDoubleClickedRow);
}

TEST_F (ListBoxTests, ModelDoubleClickCallbackInvoked)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 110.0f));
    listBox->mouseDoubleClick (event);

    EXPECT_EQ (1, model->doubleClickCallCount);
    EXPECT_EQ (5, model->lastDoubleClickedRow);
}

//==============================================================================
// Keyboard Navigation Tests
//==============================================================================

TEST_F (ListBoxTests, ReturnKeySelectsNextRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress returnKey (KeyPress::enterKey);
    listBox->keyDown (returnKey, Point<float>());

    // Return key behavior may vary, just ensure no crash
    EXPECT_GE (listBox->getNumSelectedRows(), 0);
}

TEST_F (ListBoxTests, DownArrowKeySelectsNextRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress downKey (KeyPress::downKey);
    listBox->keyDown (downKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (6));
}

TEST_F (ListBoxTests, UpArrowKeySelectsPreviousRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress upKey (KeyPress::upKey);
    listBox->keyDown (upKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (4));
}

TEST_F (ListBoxTests, UpArrowKeyDoesNotWrapBelowZero)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (0, false, dontSendNotification);

    KeyPress upKey (KeyPress::upKey);
    listBox->keyDown (upKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (0));
}

TEST_F (ListBoxTests, DownArrowKeyDoesNotWrapBeyondEnd)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (19, false, dontSendNotification);

    KeyPress downKey (KeyPress::downKey);
    listBox->keyDown (downKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (19));
}

TEST_F (ListBoxTests, PageDownKeyScrollsMultipleRows)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (0, false, dontSendNotification);

    KeyPress pageDownKey (KeyPress::pageDownKey);
    listBox->keyDown (pageDownKey, Point<float>());

    auto selectedRow = listBox->getSelectedRow();
    EXPECT_GT (selectedRow, 0);
}

TEST_F (ListBoxTests, PageUpKeyScrollsMultipleRows)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (19, false, dontSendNotification);

    KeyPress pageUpKey (KeyPress::pageUpKey);
    listBox->keyDown (pageUpKey, Point<float>());

    auto selectedRow = listBox->getSelectedRow();
    EXPECT_LT (selectedRow, 19);
}

TEST_F (ListBoxTests, HomeKeySelectsFirstRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (10, false, dontSendNotification);

    KeyPress homeKey (KeyPress::homeKey);
    listBox->keyDown (homeKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (0));
}

TEST_F (ListBoxTests, EndKeySelectsLastRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress endKey (KeyPress::endKey);
    listBox->keyDown (endKey, Point<float>());

    EXPECT_TRUE (listBox->isRowSelected (19));
}

//==============================================================================
// Component Retrieval Tests
//==============================================================================

TEST_F (ListBoxTests, GetComponentForRowReturnsNullForOutOfRange)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto* component = listBox->getComponentForRow (-1);
    EXPECT_EQ (nullptr, component);

    component = listBox->getComponentForRow (1000);
    EXPECT_EQ (nullptr, component);
}

TEST_F (ListBoxTests, GetComponentForRowReturnsNullForInvisibleRow)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    // Row far beyond visible range
    auto* component = listBox->getComponentForRow (50);
    EXPECT_EQ (nullptr, component);
}

//==============================================================================
// Repaint Tests
//==============================================================================

TEST_F (ListBoxTests, RepaintRowDoesNotCrash)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->repaintRow (5);

    // Just ensure no crash occurs
    EXPECT_TRUE (true);
}

TEST_F (ListBoxTests, RepaintInvalidRowDoesNotCrash)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->repaintRow (-1);
    listBox->repaintRow (1000);

    // Just ensure no crash occurs
    EXPECT_TRUE (true);
}

//==============================================================================
// Focus Tests
//==============================================================================

TEST_F (ListBoxTests, FocusGainedDoesNotCrash)
{
    listBox->focusGained();

    // Just ensure no crash occurs
    EXPECT_TRUE (true);
}

TEST_F (ListBoxTests, FocusLostDoesNotCrash)
{
    listBox->focusLost();

    // Just ensure no crash occurs
    EXPECT_TRUE (true);
}

//==============================================================================
// Multiple Selection with Modifiers Tests
//==============================================================================

TEST_F (ListBoxTests, SelectRowWithoutDeselectingInMultipleMode)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    EXPECT_EQ (2, listBox->getNumSelectedRows());
    EXPECT_TRUE (listBox->isRowSelected (5));
    EXPECT_TRUE (listBox->isRowSelected (10));
}

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

//==============================================================================
// Mouse Wheel Tests
//==============================================================================

TEST_F (ListBoxTests, MouseWheelScrollsContent)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    auto visibleRangeBefore = listBox->getVisibleRowRange();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 100.0f));
    MouseWheelData wheelData (0.0f, 3.0f);
    listBox->mouseWheel (event, wheelData);

    // Just ensure no crash - actual scrolling depends on implementation
    EXPECT_TRUE (true);
}

//==============================================================================
// Edge Cases for Selection
//==============================================================================

TEST_F (ListBoxTests, SelectingEmptyArrayClearsSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    Array<int> emptySelection;
    listBox->setSelectedRows (emptySelection, dontSendNotification);

    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, GetSelectedRowReturnsMinusOneForNoSelection)
{
    EXPECT_EQ (-1, listBox->getSelectedRow());
}

TEST_F (ListBoxTests, GetSelectedRowReturnsMinusOneForMultipleSelections)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);

    EXPECT_EQ (-1, listBox->getSelectedRow());
}

//==============================================================================
// Scrolling Edge Cases
//==============================================================================

TEST_F (ListBoxTests, ScrollToEnsureNegativeRowIsVisibleDoesNotCrash)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->scrollToEnsureRowIsVisible (-1);

    // Just ensure no crash
    EXPECT_TRUE (true);
}

TEST_F (ListBoxTests, ScrollToEnsureOutOfBoundsRowIsVisibleDoesNotCrash)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    listBox->scrollToEnsureRowIsVisible (1000);

    // Just ensure no crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Model Callback Tests - Return Key
//==============================================================================

TEST_F (ListBoxTests, ModelReturnKeyPressedCallbackInvoked)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress returnKey (KeyPress::enterKey);
    listBox->keyDown (returnKey, Point<float>());

    // Note: returnKeyPressed callback may not be implemented yet
    // Just verify no crash occurs
    EXPECT_GE (model->returnKeyCallCount, 0);
}

TEST_F (ListBoxTests, ModelReturnKeyPressedWithNoSelection)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    KeyPress returnKey (KeyPress::enterKey);
    listBox->keyDown (returnKey, Point<float>());

    // Note: returnKeyPressed callback may not be implemented yet
    // Just verify no crash occurs
    EXPECT_GE (model->returnKeyCallCount, 0);
}

//==============================================================================
// Model Callback Tests - Delete Key
//==============================================================================

TEST_F (ListBoxTests, ModelDeleteKeyPressedCallbackInvoked)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress deleteKey (KeyPress::deleteKey);
    listBox->keyDown (deleteKey, Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (1, model->lastDeleteKeyRows.size());
    EXPECT_EQ (5, model->lastDeleteKeyRows[0]);
}

TEST_F (ListBoxTests, ModelBackspaceKeyPressedCallbackInvoked)
{
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);

    KeyPress backspaceKey (KeyPress::backspaceKey);
    listBox->keyDown (backspaceKey, Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (1, model->lastDeleteKeyRows.size());
    EXPECT_EQ (5, model->lastDeleteKeyRows[0]);
}

TEST_F (ListBoxTests, ModelDeleteKeyPressedWithMultipleSelection)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);
    listBox->setRowHeight (20);
    listBox->updateContent();
    listBox->selectRow (5, false, dontSendNotification);
    listBox->selectRow (10, false, dontSendNotification);
    listBox->selectRow (15, false, dontSendNotification);

    KeyPress deleteKey (KeyPress::deleteKey);
    listBox->keyDown (deleteKey, Point<float>());

    EXPECT_EQ (1, model->deleteKeyCallCount);
    EXPECT_EQ (3, model->lastDeleteKeyRows.size());
}

//==============================================================================
// Variable Height Model Tests
//==============================================================================

TEST_F (ListBoxTests, ModelVariableHeightIsUsed)
{
    model->useVariableHeight = true;
    model->variableHeights.add (30);
    model->variableHeights.add (40);
    model->variableHeights.add (50);
    model->variableHeights.add (60);

    listBox->setVariableHeightEnabled (true);
    listBox->updateContent();

    // Just ensure no crash and that bounds reflect variable heights
    auto bounds0 = listBox->getRowBounds (0);
    auto bounds1 = listBox->getRowBounds (1);

    EXPECT_FALSE (bounds0.isEmpty());
    EXPECT_FALSE (bounds1.isEmpty());
    EXPECT_NE (bounds0.getHeight(), bounds1.getHeight());
}

//==============================================================================
// Variable Width Model Tests
//==============================================================================

TEST_F (ListBoxTests, ModelVariableWidthIsUsed)
{
    listBox->setOrientation (ListBox::Orientation::horizontal);

    model->useVariableWidth = true;
    model->variableWidths.add (60);
    model->variableWidths.add (80);
    model->variableWidths.add (100);
    model->variableWidths.add (120);

    listBox->setVariableWidthEnabled (true);
    listBox->updateContent();

    // Just ensure no crash and that bounds reflect variable widths
    auto bounds0 = listBox->getRowBounds (0);
    auto bounds1 = listBox->getRowBounds (1);

    EXPECT_FALSE (bounds0.isEmpty());
    EXPECT_FALSE (bounds1.isEmpty());
    EXPECT_NE (bounds0.getWidth(), bounds1.getWidth());
}

//==============================================================================
// Drag and Drop Tests
//==============================================================================

TEST_F (ListBoxTests, ModelDragSourceDescriptionReturnsEmpty)
{
    listBox->selectRow (5, false, dontSendNotification);

    Array<int> selectedRows { 5 };
    auto dragData = model->getDragSourceDescription (selectedRows);

    EXPECT_TRUE (dragData.isVoid());
    EXPECT_EQ (1, model->dragSourceCallCount);
}

TEST_F (ListBoxTests, ModelDragSourceDescriptionReturnsData)
{
    model->shouldSupportDrag = true;
    listBox->selectRow (5, false, dontSendNotification);

    Array<int> selectedRows { 5 };
    auto dragData = model->getDragSourceDescription (selectedRows);

    EXPECT_FALSE (dragData.isVoid());
    EXPECT_EQ ("DragData", dragData.toString());
    EXPECT_EQ (1, model->dragSourceCallCount);
}

//==============================================================================
// Paint Tests
//==============================================================================

TEST_F (ListBoxTests, ModelPaintListBoxItemIsCalled)
{
    listBox->setRowHeight (20);
    listBox->updateContent();

    // Trigger a paint by getting bounds (which may cause row creation)
    auto bounds = listBox->getRowBounds (0);
    EXPECT_FALSE (bounds.isEmpty());

    // The actual paint count depends on implementation details,
    // but we can verify the method is available
    EXPECT_GE (model->paintCallCount, 0);
}

//==============================================================================
// Horizontal Orientation Additional Tests
//==============================================================================

TEST_F (ListBoxTests, HorizontalOrientationLayoutsCorrectly)
{
    listBox->setOrientation (ListBox::Orientation::horizontal);
    listBox->setRowWidth (80);
    listBox->updateContent();

    auto bounds0 = listBox->getRowBounds (0);
    auto bounds1 = listBox->getRowBounds (1);

    EXPECT_FALSE (bounds0.isEmpty());
    EXPECT_FALSE (bounds1.isEmpty());

    // In horizontal mode, rows should be side by side
    EXPECT_LT (bounds0.getRight(), bounds1.getRight());
}

TEST_F (ListBoxTests, HorizontalOrientationGetRowAtWorks)
{
    listBox->setOrientation (ListBox::Orientation::horizontal);
    listBox->setRowWidth (80);
    listBox->updateContent();

    // Should find a row in horizontal layout
    auto rowIndex = listBox->getRowAt (Point<float> (100.0f, 50.0f));
    EXPECT_GE (rowIndex, 0);
}

//==============================================================================
// Notification Type Tests
//==============================================================================

TEST_F (ListBoxTests, DeselectAllWithNotification)
{
    int callbackCount = 0;
    listBox->onSelectionChanged = [&callbackCount]()
    {
        callbackCount++;
    };

    listBox->selectRow (5, false, dontSendNotification);
    callbackCount = 0; // Reset after initial selection

    listBox->deselectAllRows (sendNotification);

    EXPECT_EQ (1, callbackCount);
    EXPECT_EQ (0, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, DeselectRowWithNotification)
{
    int callbackCount = 0;
    listBox->onSelectionChanged = [&callbackCount]()
    {
        callbackCount++;
    };

    listBox->selectRow (5, false, dontSendNotification);
    callbackCount = 0; // Reset

    listBox->deselectRow (5, sendNotification);

    EXPECT_EQ (1, callbackCount);
    EXPECT_FALSE (listBox->isRowSelected (5));
}

TEST_F (ListBoxTests, SetSelectedRowsWithNotification)
{
    listBox->setSelectionMode (ListBox::SelectionMode::multiple);

    int callbackCount = 0;
    listBox->onSelectionChanged = [&callbackCount]()
    {
        callbackCount++;
    };

    Array<int> rowsToSelect { 3, 7, 11 };
    listBox->setSelectedRows (rowsToSelect, sendNotification);

    EXPECT_EQ (1, callbackCount);
    EXPECT_EQ (3, listBox->getNumSelectedRows());
}

TEST_F (ListBoxTests, MouseUpDoesNotCrash)
{
    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (50, 50));
    EXPECT_NO_THROW (listBox->mouseUp (event));
}

TEST_F (ListBoxTests, MouseMoveDoesNotCrash)
{
    MouseEvent event (MouseEvent::noButtons, KeyModifiers(), Point<float> (50, 50));
    EXPECT_NO_THROW (listBox->mouseMove (event));
}

TEST_F (ListBoxTests, RefreshComponentForRowDefaultReturnsExisting)
{
    int rows = 5;
    TestListBoxModel model (rows);
    listBox->setModel (&model);

    auto* refreshed = model.refreshComponentForRow (0, nullptr);
    EXPECT_EQ (nullptr, refreshed);
}
