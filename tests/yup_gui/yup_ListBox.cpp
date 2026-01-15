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

    int numRows;
    Array<int> lastSelectedRows;
    int lastClickedRow = -1;
    int lastDoubleClickedRow = -1;
    int selectionChangedCallCount = 0;
    int clickCallCount = 0;
    int doubleClickCallCount = 0;
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
