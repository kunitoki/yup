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
constexpr int kCustomItemId = 100;

const String kTestText1 = "Option 1";
const String kTestText2 = "Option 2";
const String kTestText3 = "Option 3";
const String kSubMenuText = "Sub Menu";
const String kShortcutText = "Ctrl+S";
} // namespace

class PopupMenuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        parentComponent = std::make_unique<Component> ("testParent");
        parentComponent->setBounds (0, 0, 800, 600);

        targetComponent = std::make_unique<Component> ("testTarget");
        targetComponent->setBounds (100, 100, 200, 30);

        parentComponent->addAndMakeVisible (*targetComponent);
    }

    std::unique_ptr<Component> parentComponent;
    std::unique_ptr<Component> targetComponent;
};

//==============================================================================
// Basic Creation and Construction

TEST_F (PopupMenuTest, StaticCreateMethodWorks)
{
    auto menu = PopupMenu::create();
    EXPECT_NE (nullptr, menu.get());
    EXPECT_TRUE (menu->isEmpty());
    EXPECT_EQ (0, menu->getNumItems());
}

TEST_F (PopupMenuTest, CreateWithOptions)
{
    PopupMenu::Options options;
    options.withParentComponent (parentComponent.get())
        .withMinimumWidth (150)
        .withMaximumWidth (300);

    auto menu = PopupMenu::create (options);
    EXPECT_NE (nullptr, menu.get());

    const auto& menuOptions = menu->getOptions();
    EXPECT_EQ (parentComponent.get(), menuOptions.parentComponent);
    EXPECT_EQ (150, menuOptions.minWidth.value_or (0));
    EXPECT_EQ (300, menuOptions.maxWidth.value_or (0));
}

//==============================================================================
// Item Management

TEST_F (PopupMenuTest, AddItemIncreasesCount)
{
    auto menu = PopupMenu::create();
    EXPECT_EQ (0, menu->getNumItems());
    EXPECT_TRUE (menu->isEmpty());

    menu->addItem (kTestText1, kTestId1);

    EXPECT_EQ (1, menu->getNumItems());
    EXPECT_FALSE (menu->isEmpty());
}

TEST_F (PopupMenuTest, AddMultipleItemsWithShortcuts)
{
    auto menu = PopupMenu::create();

    menu->addItem (kTestText1, kTestId1, true, false, kShortcutText);
    menu->addItem (kTestText2, kTestId2, false, true); // Disabled and ticked
    menu->addItem (kTestText3, kTestId3);

    EXPECT_EQ (3, menu->getNumItems());

    // Verify items can be iterated
    int itemCount = 0;
    for (const auto& item : *menu)
    {
        itemCount++;
    }
    EXPECT_EQ (3, itemCount);
}

TEST_F (PopupMenuTest, AddSeparatorIncreasesCount)
{
    auto menu = PopupMenu::create();

    menu->addItem (kTestText1, kTestId1);
    menu->addSeparator();
    menu->addItem (kTestText2, kTestId2);

    EXPECT_EQ (3, menu->getNumItems());
}

TEST_F (PopupMenuTest, AddSubMenu)
{
    auto menu = PopupMenu::create();
    auto subMenu = PopupMenu::create();

    subMenu->addItem ("Sub Item 1", 10);
    subMenu->addItem ("Sub Item 2", 11);

    menu->addItem (kTestText1, kTestId1);
    menu->addSubMenu (kSubMenuText, subMenu, true);

    EXPECT_EQ (2, menu->getNumItems());
}

TEST_F (PopupMenuTest, AddCustomItem)
{
    auto menu = PopupMenu::create();
    auto customComponent = std::make_unique<Label> ("customLabel");
    customComponent->setText ("Custom Menu Item");
    customComponent->setSize (150, 25);

    menu->addItem (kTestText1, kTestId1);
    menu->addCustomItem (std::move (customComponent), kCustomItemId);

    EXPECT_EQ (2, menu->getNumItems());
}

TEST_F (PopupMenuTest, ClearRemovesAllItems)
{
    auto menu = PopupMenu::create();

    menu->addItem (kTestText1, kTestId1);
    menu->addSeparator();
    menu->addItem (kTestText2, kTestId2);

    EXPECT_EQ (3, menu->getNumItems());
    EXPECT_FALSE (menu->isEmpty());

    menu->clear();

    EXPECT_EQ (0, menu->getNumItems());
    EXPECT_TRUE (menu->isEmpty());
}

//==============================================================================
// Item Class Tests

TEST_F (PopupMenuTest, ItemConstructorText)
{
    PopupMenu::Item textItem (kTestText1, kTestId1, true, false);

    EXPECT_EQ (kTestText1, textItem.text);
    EXPECT_EQ (kTestId1, textItem.itemID);
    EXPECT_TRUE (textItem.isEnabled);
    EXPECT_FALSE (textItem.isTicked);
    EXPECT_FALSE (textItem.isHovered);
    EXPECT_FALSE (textItem.isSeparator());
    EXPECT_FALSE (textItem.isSubMenu());
    EXPECT_FALSE (textItem.isCustomComponent());
}

TEST_F (PopupMenuTest, ItemConstructorSubMenu)
{
    auto subMenu = PopupMenu::create();
    PopupMenu::Item subMenuItem (kSubMenuText, subMenu, true);

    EXPECT_EQ (kSubMenuText, subMenuItem.text);
    EXPECT_TRUE (subMenuItem.isEnabled);
    EXPECT_FALSE (subMenuItem.isSeparator());
    EXPECT_TRUE (subMenuItem.isSubMenu());
    EXPECT_FALSE (subMenuItem.isCustomComponent());
    EXPECT_EQ (subMenu.get(), subMenuItem.subMenu.get());
}

TEST_F (PopupMenuTest, ItemConstructorCustomComponent)
{
    auto customComponent = std::make_unique<Label> ("testLabel");
    auto* componentPtr = customComponent.get();

    PopupMenu::Item customItem (std::move (customComponent), kCustomItemId);

    EXPECT_EQ (kCustomItemId, customItem.itemID);
    EXPECT_FALSE (customItem.isSeparator());
    EXPECT_FALSE (customItem.isSubMenu());
    EXPECT_TRUE (customItem.isCustomComponent());
    EXPECT_EQ (componentPtr, customItem.customComponent.get());
}

TEST_F (PopupMenuTest, ItemSeparatorBehavior)
{
    PopupMenu::Item separatorItem; // Default constructor creates separator

    EXPECT_TRUE (separatorItem.text.isEmpty());
    EXPECT_EQ (0, separatorItem.itemID);
    EXPECT_TRUE (separatorItem.isSeparator());
    EXPECT_FALSE (separatorItem.isSubMenu());
    EXPECT_FALSE (separatorItem.isCustomComponent());
    EXPECT_EQ (nullptr, separatorItem.subMenu.get());
    EXPECT_EQ (nullptr, separatorItem.customComponent.get());
}

//==============================================================================
// Options Configuration

TEST_F (PopupMenuTest, OptionsDefaultValues)
{
    PopupMenu::Options options;

    EXPECT_EQ (nullptr, options.parentComponent);
    EXPECT_EQ (nullptr, options.targetComponent);
    EXPECT_EQ (Justification::topLeft, options.alignment);
    EXPECT_EQ (PopupMenu::PositioningMode::atPoint, options.positioningMode);
    EXPECT_TRUE (options.dismissOnSelection);
    EXPECT_TRUE (options.dismissAllPopups);
    EXPECT_FALSE (options.minWidth.has_value());
    EXPECT_FALSE (options.maxWidth.has_value());
}

TEST_F (PopupMenuTest, OptionsFluentInterface)
{
    PopupMenu::Options options;
    Point<int> testPosition (50, 75);
    Rectangle<int> testArea (10, 20, 100, 50);

    options.withParentComponent (parentComponent.get())
        .withPosition (testPosition, Justification::center)
        .withTargetArea (testArea, PopupMenu::Placement::above())
        .withRelativePosition (targetComponent.get(), PopupMenu::Placement::below())
        .withMinimumWidth (120)
        .withMaximumWidth (400);

    EXPECT_EQ (parentComponent.get(), options.parentComponent);
    EXPECT_EQ (testPosition, options.targetPosition);
    EXPECT_EQ (Justification::center, options.alignment);
    EXPECT_EQ (testArea, options.targetArea);
    EXPECT_EQ (targetComponent.get(), options.targetComponent);
    EXPECT_EQ (120, options.minWidth.value());
    EXPECT_EQ (400, options.maxWidth.value());
}

TEST_F (PopupMenuTest, OptionsFloatToIntConversion)
{
    PopupMenu::Options options;
    Point<float> floatPosition (50.5f, 75.7f);
    Rectangle<float> floatArea (10.2f, 20.8f, 100.1f, 50.9f);

    options.withPosition (floatPosition)
        .withTargetArea (floatArea, PopupMenu::Placement::toRight());

    EXPECT_EQ (Point<int> (50, 75), options.targetPosition);
    EXPECT_EQ (Rectangle<int> (10, 20, 100, 50), options.targetArea);
}

//==============================================================================
// Placement Configuration

TEST_F (PopupMenuTest, PlacementDefaultValues)
{
    PopupMenu::Placement placement;

    EXPECT_EQ (PopupMenu::Side::below, placement.side);
    EXPECT_EQ (Justification::topLeft, placement.alignment);
}

TEST_F (PopupMenuTest, PlacementConstructor)
{
    PopupMenu::Placement placement (PopupMenu::Side::above, Justification::center);

    EXPECT_EQ (PopupMenu::Side::above, placement.side);
    EXPECT_EQ (Justification::center, placement.alignment);
}

TEST_F (PopupMenuTest, PlacementStaticMethods)
{
    auto belowPlacement = PopupMenu::Placement::below (Justification::bottomRight);
    EXPECT_EQ (PopupMenu::Side::below, belowPlacement.side);
    EXPECT_EQ (Justification::bottomRight, belowPlacement.alignment);

    auto abovePlacement = PopupMenu::Placement::above (Justification::topRight);
    EXPECT_EQ (PopupMenu::Side::above, abovePlacement.side);
    EXPECT_EQ (Justification::topRight, abovePlacement.alignment);

    auto rightPlacement = PopupMenu::Placement::toRight();
    EXPECT_EQ (PopupMenu::Side::toRight, rightPlacement.side);
    EXPECT_EQ (Justification::topLeft, rightPlacement.alignment);

    auto leftPlacement = PopupMenu::Placement::toLeft();
    EXPECT_EQ (PopupMenu::Side::toLeft, leftPlacement.side);
    EXPECT_EQ (Justification::topLeft, leftPlacement.alignment);

    auto centeredPlacement = PopupMenu::Placement::centered();
    EXPECT_EQ (PopupMenu::Side::centered, centeredPlacement.side);
    EXPECT_EQ (Justification::center, centeredPlacement.alignment);
}

//==============================================================================
// Show/Hide Functionality

TEST_F (PopupMenuTest, ShowAndDismissBasic)
{
    PopupMenu::dismissAllPopups();

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (targetComponent.get()));
    menu->addItem (kTestText1, kTestId1);

    EXPECT_FALSE (menu->isVisible());

    // Note: We can't easily test the actual showing without a proper display context
    // But we can test that the show method doesn't crash
    menu->show();
    menu->dismiss();

    // Test dismiss without showing doesn't crash
    menu->dismiss();

    PopupMenu::dismissAllPopups();
}

TEST_F (PopupMenuTest, ShowWithCallback)
{
    PopupMenu::dismissAllPopups();

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (targetComponent.get()));
    menu->addItem (kTestText1, kTestId1);
    menu->addItem (kTestText2, kTestId2);

    int selectedItemId = -1;
    bool callbackCalled = false;

    menu->show ([&selectedItemId, &callbackCalled] (int itemId)
    {
        selectedItemId = itemId;
        callbackCalled = true;
    });

    EXPECT_FALSE (callbackCalled);
    EXPECT_EQ (selectedItemId, -1);

    PopupMenu::dismissAllPopups();

    EXPECT_TRUE (callbackCalled);
    EXPECT_EQ (selectedItemId, 0);
}

TEST_F (PopupMenuTest, DismissAllPopupsStatic)
{
    PopupMenu::dismissAllPopups();

    auto menu1 = PopupMenu::create (PopupMenu::Options().withParentComponent (targetComponent.get()));
    auto menu2 = PopupMenu::create (PopupMenu::Options().withParentComponent (targetComponent.get()));

    menu1->addItem ("Menu 1 Item", 1);
    menu2->addItem ("Menu 2 Item", 2);

    bool dismissedMenu1 = false;
    bool dismissedMenu2 = false;

    menu1->show ([&dismissedMenu1] (int itemId)
    {
        dismissedMenu1 = true;
    });
    menu2->show ([&dismissedMenu2] (int itemId)
    {
        dismissedMenu2 = true;
    });

    PopupMenu::dismissAllPopups();

    EXPECT_TRUE (dismissedMenu1);
    EXPECT_TRUE (dismissedMenu2);
}

//==============================================================================
// Callback and Event Handling

TEST_F (PopupMenuTest, OnItemSelectedCallback)
{
    auto menu = PopupMenu::create();
    menu->addItem (kTestText1, kTestId1);

    int selectedId = -1;
    menu->onItemSelected = [&selectedId] (int itemId)
    {
        selectedId = itemId;
    };

    EXPECT_EQ (-1, selectedId);

    // Simulate calling the callback directly
    if (menu->onItemSelected)
        menu->onItemSelected (kTestId1);

    EXPECT_EQ (kTestId1, selectedId);
}

TEST_F (PopupMenuTest, MouseEnterExitCallbacks)
{
    auto menu = PopupMenu::create();
    menu->addItem (kTestText1, kTestId1);

    bool mouseEnterCalled = false;
    bool mouseExitCalled = false;

    menu->onMouseEnter = [&mouseEnterCalled]()
    {
        mouseEnterCalled = true;
    };

    menu->onMouseExit = [&mouseExitCalled]()
    {
        mouseExitCalled = true;
    };

    EXPECT_FALSE (mouseEnterCalled);
    EXPECT_FALSE (mouseExitCalled);

    // Simulate calling the callbacks directly
    if (menu->onMouseEnter)
        menu->onMouseEnter();

    if (menu->onMouseExit)
        menu->onMouseExit();

    EXPECT_TRUE (mouseEnterCalled);
    EXPECT_TRUE (mouseExitCalled);
}

//==============================================================================
// Style Identifiers

TEST_F (PopupMenuTest, StyleIdentifiers)
{
    // Test that style identifiers are properly defined
    EXPECT_FALSE (PopupMenu::Style::menuBackground.toString().isEmpty());
    EXPECT_FALSE (PopupMenu::Style::menuBorder.toString().isEmpty());
    EXPECT_FALSE (PopupMenu::Style::menuItemText.toString().isEmpty());
    EXPECT_FALSE (PopupMenu::Style::menuItemTextDisabled.toString().isEmpty());
    EXPECT_FALSE (PopupMenu::Style::menuItemBackground.toString().isEmpty());
    EXPECT_FALSE (PopupMenu::Style::menuItemBackgroundHighlighted.toString().isEmpty());

    // Test that identifiers are unique
    EXPECT_NE (PopupMenu::Style::menuBackground, PopupMenu::Style::menuBorder);
    EXPECT_NE (PopupMenu::Style::menuItemText, PopupMenu::Style::menuItemTextDisabled);
    EXPECT_NE (PopupMenu::Style::menuItemBackground, PopupMenu::Style::menuItemBackgroundHighlighted);
}

//==============================================================================
// Complex Menu Scenarios

TEST_F (PopupMenuTest, NestedSubMenus)
{
    auto mainMenu = PopupMenu::create();
    auto subMenu1 = PopupMenu::create();
    auto subSubMenu = PopupMenu::create();

    // Create nested structure
    subSubMenu->addItem ("Deep Item 1", 301);
    subSubMenu->addItem ("Deep Item 2", 302);

    subMenu1->addItem ("Sub Item 1", 201);
    subMenu1->addSubMenu ("Sub Sub Menu", subSubMenu);
    subMenu1->addItem ("Sub Item 2", 202);

    mainMenu->addItem ("Main Item 1", 101);
    mainMenu->addSubMenu ("Sub Menu 1", subMenu1);
    mainMenu->addItem ("Main Item 2", 102);

    EXPECT_EQ (3, mainMenu->getNumItems());
    EXPECT_EQ (3, subMenu1->getNumItems());
    EXPECT_EQ (2, subSubMenu->getNumItems());
}

TEST_F (PopupMenuTest, MixedContentMenu)
{
    auto menu = PopupMenu::create();
    auto subMenu = PopupMenu::create();
    auto customComponent = std::make_unique<TextButton> ("customButton");

    subMenu->addItem ("Sub Option", 201);

    customComponent->setButtonText ("Custom Button");
    customComponent->setSize (120, 30);

    menu->addItem ("Regular Item", 101, true, false, "Ctrl+R");
    menu->addItem ("Disabled Item", 102, false);
    menu->addItem ("Ticked Item", 103, true, true);
    menu->addSeparator();
    menu->addSubMenu ("Sub Menu", subMenu);
    menu->addSeparator();
    menu->addCustomItem (std::move (customComponent), 104);

    EXPECT_EQ (7, menu->getNumItems());
    EXPECT_FALSE (menu->isEmpty());
}

//==============================================================================
// Edge Cases and Error Handling

TEST_F (PopupMenuTest, EmptyMenuBehavior)
{
    auto menu = PopupMenu::create();

    EXPECT_TRUE (menu->isEmpty());
    EXPECT_EQ (0, menu->getNumItems());

    // Show empty menu should handle gracefully
    menu->show();
    menu->dismiss();
}

TEST_F (PopupMenuTest, NullSubMenuHandling)
{
    auto menu = PopupMenu::create();

    // Adding null submenu should be handled gracefully
    PopupMenu::Ptr nullSubMenu;
    menu->addSubMenu ("Null Sub Menu", nullSubMenu, true);

    EXPECT_EQ (1, menu->getNumItems());
}

TEST_F (PopupMenuTest, DuplicateItemIds)
{
    auto menu = PopupMenu::create();

    menu->addItem ("First Item", kTestId1);
    menu->addItem ("Second Item", kTestId1); // Same ID
    menu->addItem ("Third Item", kTestId1);  // Same ID again

    EXPECT_EQ (3, menu->getNumItems());
    // All items should be added regardless of duplicate IDs
}

TEST_F (PopupMenuTest, ZeroAndNegativeItemIds)
{
    auto menu = PopupMenu::create();

    menu->addItem ("Zero ID", 0);
    menu->addItem ("Negative ID", -1);
    menu->addItem ("Large Positive ID", 999999);
    menu->addItem ("Large Negative ID", -999999);

    EXPECT_EQ (4, menu->getNumItems());
}

TEST_F (PopupMenuTest, VeryLongItemText)
{
    auto menu = PopupMenu::create();

    String longText;
    for (int i = 0; i < 100; ++i)
        longText += "Very long menu item text ";

    menu->addItem (longText, kTestId1);
    EXPECT_EQ (1, menu->getNumItems());
}

TEST_F (PopupMenuTest, SpecialCharactersInText)
{
    auto menu = PopupMenu::create();

    menu->addItem (L"Item with üñíçødé", kTestId1);
    menu->addItem ("Item with\nNewline", kTestId2);
    menu->addItem ("Item with\tTab", kTestId3);
    menu->addItem ("", 0); // Empty text item

    EXPECT_EQ (4, menu->getNumItems());
}

//==============================================================================
// Component Inheritance Tests

TEST_F (PopupMenuTest, ComponentBehavior)
{
    auto menu = PopupMenu::create();

    // Test basic Component methods work
    menu->setSize (200, 300);
    EXPECT_EQ (200, menu->getWidth());
    EXPECT_EQ (300, menu->getHeight());

    menu->setTopLeft (Point<float> (50, 75));
    EXPECT_EQ (Point<float> (50, 75), menu->getBounds().getTopLeft());

    // Test visibility
    EXPECT_FALSE (menu->isVisible()); // Should start invisible

    menu->setVisible (true);
    EXPECT_TRUE (menu->isVisible());

    menu->setVisible (false);
    EXPECT_FALSE (menu->isVisible());
}

TEST_F (PopupMenuTest, ReferenceCountedBehavior)
{
    // Test that PopupMenu properly manages reference counting
    PopupMenu::Ptr menu1 = PopupMenu::create();
    EXPECT_EQ (1, menu1->getReferenceCount());

    PopupMenu::Ptr menu2 = menu1;
    EXPECT_EQ (2, menu1->getReferenceCount());
    EXPECT_EQ (2, menu2->getReferenceCount());

    menu2 = nullptr;
    EXPECT_EQ (1, menu1->getReferenceCount());

    auto subMenu = PopupMenu::create();
    menu1->addSubMenu ("Sub", subMenu);
    // subMenu should be held by the menu item now
}

//==============================================================================
// Internal State Management

TEST_F (PopupMenuTest, SubmenuVisibilityMethods)
{
    auto menu = PopupMenu::create();
    auto subMenu = PopupMenu::create();

    subMenu->addItem ("Sub Item", 201);
    menu->addSubMenu ("Sub Menu", subMenu);

    EXPECT_FALSE (menu->hasVisibleSubmenu());

    Point<float> testPoint (100.0f, 100.0f);
    EXPECT_FALSE (menu->submenuContains (testPoint));
}

TEST_F (PopupMenuTest, IteratorSupport)
{
    auto menu = PopupMenu::create();

    menu->addItem (kTestText1, kTestId1);
    menu->addSeparator();
    menu->addItem (kTestText2, kTestId2);

    // Test range-based for loop
    int itemCount = 0;
    for (const auto& item : *menu)
    {
        EXPECT_NE (nullptr, item.get());
        itemCount++;
    }
    EXPECT_EQ (3, itemCount);

    // Test iterator equality
    EXPECT_NE (menu->begin(), menu->end());

    auto it = menu->begin();
    ++it;
    ++it;
    ++it;
    EXPECT_EQ (it, menu->end());
}
