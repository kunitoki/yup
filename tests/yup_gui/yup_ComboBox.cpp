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
constexpr int kTestId1 = 1;
constexpr int kTestId2 = 2;
constexpr int kTestId3 = 3;

const String kTestText1 = "Option 1";
const String kTestText2 = "Option 2";
const String kTestText3 = "Option 3";
const String kPlaceholderText = "Select an option";
} // namespace

class ComboBoxTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        comboBox = std::make_unique<ComboBox> ("testComboBox");
        comboBox->setBounds (0, 0, 200, 30);
    }

    std::unique_ptr<ComboBox> comboBox;
};

TEST_F (ComboBoxTest, ConstructorInitializesCorrectly)
{
    EXPECT_EQ (0, comboBox->getNumItems());
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (0, comboBox->getSelectedId());
    EXPECT_TRUE (comboBox->getText().isEmpty());
    EXPECT_FALSE (comboBox->isTextEditable());
}

TEST_F (ComboBoxTest, AddItemIncreasesCount)
{
    comboBox->addItem (kTestText1, kTestId1);

    EXPECT_EQ (1, comboBox->getNumItems());
    EXPECT_EQ (kTestText1, comboBox->getItemText (0));
    EXPECT_EQ (kTestId1, comboBox->getItemId (0));
}

TEST_F (ComboBoxTest, AddMultipleItems)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);
    comboBox->addItem (kTestText3, kTestId3);

    EXPECT_EQ (3, comboBox->getNumItems());

    EXPECT_EQ (kTestText1, comboBox->getItemText (0));
    EXPECT_EQ (kTestId1, comboBox->getItemId (0));

    EXPECT_EQ (kTestText2, comboBox->getItemText (1));
    EXPECT_EQ (kTestId2, comboBox->getItemId (1));

    EXPECT_EQ (kTestText3, comboBox->getItemText (2));
    EXPECT_EQ (kTestId3, comboBox->getItemId (2));
}

TEST_F (ComboBoxTest, AddItemListWorksCorrectly)
{
    StringArray items;
    items.add (kTestText1);
    items.add (kTestText2);
    items.add (kTestText3);

    comboBox->addItemList (items, kTestId1);

    EXPECT_EQ (3, comboBox->getNumItems());
    EXPECT_EQ (kTestText1, comboBox->getItemText (0));
    EXPECT_EQ (kTestId1, comboBox->getItemId (0));
    EXPECT_EQ (kTestText2, comboBox->getItemText (1));
    EXPECT_EQ (kTestId1 + 1, comboBox->getItemId (1));
    EXPECT_EQ (kTestText3, comboBox->getItemText (2));
    EXPECT_EQ (kTestId1 + 2, comboBox->getItemId (2));
}

TEST_F (ComboBoxTest, ClearRemovesAllItems)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    EXPECT_EQ (2, comboBox->getNumItems());

    comboBox->clear();

    EXPECT_EQ (0, comboBox->getNumItems());
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (0, comboBox->getSelectedId());
}

TEST_F (ComboBoxTest, GetItemTextHandlesInvalidIndices)
{
    comboBox->addItem (kTestText1, kTestId1);

    EXPECT_EQ (kTestText1, comboBox->getItemText (0));
    EXPECT_TRUE (comboBox->getItemText (-1).isEmpty());
    EXPECT_TRUE (comboBox->getItemText (1).isEmpty());
    EXPECT_TRUE (comboBox->getItemText (999).isEmpty());
}

TEST_F (ComboBoxTest, GetItemIdHandlesInvalidIndices)
{
    comboBox->addItem (kTestText1, kTestId1);

    EXPECT_EQ (kTestId1, comboBox->getItemId (0));
    EXPECT_EQ (0, comboBox->getItemId (-1));
    EXPECT_EQ (0, comboBox->getItemId (1));
    EXPECT_EQ (0, comboBox->getItemId (999));
}

TEST_F (ComboBoxTest, ChangeItemTextUpdatesText)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    const String newText = "Updated Option";
    comboBox->changeItemText (0, newText);

    EXPECT_EQ (newText, comboBox->getItemText (0));
    EXPECT_EQ (kTestId1, comboBox->getItemId (0));
    EXPECT_EQ (kTestText2, comboBox->getItemText (1));
}

TEST_F (ComboBoxTest, ChangeItemTextHandlesInvalidIndices)
{
    comboBox->addItem (kTestText1, kTestId1);

    // These should not crash
    comboBox->changeItemText (-1, "Invalid");
    comboBox->changeItemText (1, "Invalid");

    // Original item should be unchanged
    EXPECT_EQ (kTestText1, comboBox->getItemText (0));
}

TEST_F (ComboBoxTest, SelectionByIndex)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    comboBox->setSelectedItemIndex (1, dontSendNotification);

    EXPECT_EQ (1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (kTestId2, comboBox->getSelectedId());
    EXPECT_EQ (kTestText2, comboBox->getText());
}

TEST_F (ComboBoxTest, SelectionById)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    comboBox->setSelectedId (kTestId1, dontSendNotification);

    EXPECT_EQ (0, comboBox->getSelectedItemIndex());
    EXPECT_EQ (kTestId1, comboBox->getSelectedId());
    EXPECT_EQ (kTestText1, comboBox->getText());
}

TEST_F (ComboBoxTest, SelectionWithInvalidIndex)
{
    comboBox->addItem (kTestText1, kTestId1);

    // Select invalid index
    comboBox->setSelectedItemIndex (999, dontSendNotification);

    // Should deselect
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (0, comboBox->getSelectedId());
}

TEST_F (ComboBoxTest, SelectionWithInvalidId)
{
    comboBox->addItem (kTestText1, kTestId1);

    // Select invalid ID
    comboBox->setSelectedId (999, dontSendNotification);

    // Should deselect
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (0, comboBox->getSelectedId());
}

TEST_F (ComboBoxTest, PlaceholderText)
{
    comboBox->setTextWhenNothingSelected (kPlaceholderText);

    EXPECT_EQ (kPlaceholderText, comboBox->getTextWhenNothingSelected());

    // With no selection, should show placeholder in getText()
    EXPECT_EQ (kPlaceholderText, comboBox->getText());

    // After selecting an item, should show item text
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->setSelectedItemIndex (0, dontSendNotification);
    EXPECT_EQ (kTestText1, comboBox->getText());

    // After deselecting, should show placeholder again
    comboBox->setSelectedItemIndex (-1, dontSendNotification);
    EXPECT_EQ (kPlaceholderText, comboBox->getText());
}

TEST_F (ComboBoxTest, EditableText)
{
    EXPECT_FALSE (comboBox->isTextEditable());

    comboBox->setEditableText (true);
    EXPECT_TRUE (comboBox->isTextEditable());

    comboBox->setEditableText (false);
    EXPECT_FALSE (comboBox->isTextEditable());
}

TEST_F (ComboBoxTest, EmptyComboBoxBehavior)
{
    // Test behavior when no items are added
    EXPECT_EQ (0, comboBox->getNumItems());
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
    EXPECT_EQ (0, comboBox->getSelectedId());
    EXPECT_TRUE (comboBox->getText().isEmpty());

    // Trying to select should do nothing
    comboBox->setSelectedItemIndex (0, dontSendNotification);
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());

    comboBox->setSelectedId (kTestId1, dontSendNotification);
    EXPECT_EQ (-1, comboBox->getSelectedItemIndex());
}

TEST_F (ComboBoxTest, DuplicateIds)
{
    // Test behavior with duplicate IDs
    comboBox->addItem ("First", kTestId1);
    comboBox->addItem ("Second", kTestId1); // Same ID

    // Selecting by ID should select the first matching item
    comboBox->setSelectedId (kTestId1, dontSendNotification);

    EXPECT_EQ (0, comboBox->getSelectedItemIndex());
    EXPECT_EQ ("First", comboBox->getText());
}

TEST_F (ComboBoxTest, ZeroAndNegativeIds)
{
    comboBox->addItem ("Zero ID", 0);
    comboBox->addItem ("Negative ID", -1);
    comboBox->addItem ("Positive ID", kTestId1);

    comboBox->setSelectedId (0, dontSendNotification);
    EXPECT_EQ (0, comboBox->getSelectedItemIndex());
    EXPECT_EQ ("Zero ID", comboBox->getText());

    comboBox->setSelectedId (-1, dontSendNotification);
    EXPECT_EQ (1, comboBox->getSelectedItemIndex());
    EXPECT_EQ ("Negative ID", comboBox->getText());

    comboBox->setSelectedId (kTestId1, dontSendNotification);
    EXPECT_EQ (2, comboBox->getSelectedItemIndex());
    EXPECT_EQ ("Positive ID", comboBox->getText());
}

TEST_F (ComboBoxTest, ComponentIdIsSet)
{
    auto newComboBox = std::make_unique<ComboBox> ("uniqueComboBoxId");
    EXPECT_EQ (String ("uniqueComboBoxId"), newComboBox->getComponentID());
}

TEST_F (ComboBoxTest, BoundsAndSizeWork)
{
    Rectangle<int> bounds (10, 20, 150, 25);
    comboBox->setBounds (bounds);

    EXPECT_EQ (bounds.to<float>(), comboBox->getBounds());
    EXPECT_EQ (150, comboBox->getWidth());
    EXPECT_EQ (25, comboBox->getHeight());
}

TEST_F (ComboBoxTest, FunctionalCallbackIsInvoked)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);
    comboBox->addItem (kTestText3, kTestId3);

    bool callbackInvoked = false;
    int callbackCount = 0;

    // Set the functional callback
    comboBox->onSelectedItemChanged = [&callbackInvoked, &callbackCount]()
    {
        callbackInvoked = true;
        ++callbackCount;
    };

    // Initially should not be invoked
    EXPECT_FALSE (callbackInvoked);
    EXPECT_EQ (0, callbackCount);

    // Select first item
    comboBox->setSelectedItemIndex (0);
    EXPECT_TRUE (callbackInvoked);
    EXPECT_EQ (1, callbackCount);

    // Reset for next test
    callbackInvoked = false;

    // Select second item
    comboBox->setSelectedItemIndex (1);
    EXPECT_TRUE (callbackInvoked);
    EXPECT_EQ (2, callbackCount);

    // Select same item again (may or may not trigger callback depending on implementation)
    callbackInvoked = false;
    comboBox->setSelectedItemIndex (1);
    // The callback behavior when selecting the same item is implementation-dependent
    // Just verify the count didn't decrease
    EXPECT_GE (callbackCount, 2);
}

TEST_F (ComboBoxTest, FunctionalCallbackCanBeCleared)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    bool callbackInvoked = false;

    // Set the functional callback
    comboBox->onSelectedItemChanged = [&callbackInvoked]()
    {
        callbackInvoked = true;
    };

    // Select item to verify callback works
    comboBox->setSelectedItemIndex (0);
    EXPECT_TRUE (callbackInvoked);

    // Clear the callback
    callbackInvoked = false;
    comboBox->onSelectedItemChanged = nullptr;

    // Select different item - callback should not be invoked
    comboBox->setSelectedItemIndex (1);
    EXPECT_FALSE (callbackInvoked);
}

TEST_F (ComboBoxTest, FunctionalCallbackWithMultipleAssignments)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);

    int callback1Count = 0;
    int callback2Count = 0;

    // Set first callback
    comboBox->onSelectedItemChanged = [&callback1Count]()
    {
        ++callback1Count;
    };

    comboBox->setSelectedItemIndex (0);
    EXPECT_EQ (1, callback1Count);
    EXPECT_EQ (0, callback2Count);

    // Replace with second callback
    comboBox->onSelectedItemChanged = [&callback2Count]()
    {
        ++callback2Count;
    };

    comboBox->setSelectedItemIndex (1);
    EXPECT_EQ (1, callback1Count); // Should not increment
    EXPECT_EQ (1, callback2Count); // Should increment
}

TEST_F (ComboBoxTest, FunctionalCallbackWithIdSelection)
{
    comboBox->addItem (kTestText1, kTestId1);
    comboBox->addItem (kTestText2, kTestId2);
    comboBox->addItem (kTestText3, kTestId3);

    int selectedId = 0;
    int selectedIndex = -1;

    comboBox->onSelectedItemChanged = [&]()
    {
        selectedId = comboBox->getSelectedId();
        selectedIndex = comboBox->getSelectedItemIndex();
    };

    // Select by ID
    comboBox->setSelectedId (kTestId2);
    EXPECT_EQ (kTestId2, selectedId);
    EXPECT_EQ (1, selectedIndex); // Should be index 1

    // Select by different ID
    comboBox->setSelectedId (kTestId3);
    EXPECT_EQ (kTestId3, selectedId);
    EXPECT_EQ (2, selectedIndex); // Should be index 2
}
