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
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
constexpr int kPopupTestId1 = 1;
constexpr int kPopupTestId2 = 2;
constexpr int kPopupTestId3 = 3;
constexpr int kCustomItemId = 100;

const String kPopupTestText1 = "Option 1";
const String kPopupTestText2 = "Option 2";
const String kPopupTestText3 = "Option 3";
const String kSubMenuText = "Sub Menu";
const String kShortcutText = "Ctrl+S";
} // namespace

class PopupMenuTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        parentComponent = std::make_unique<Component> ("testParent");
        parentComponent->setBounds (0, 0, 800, 600);

        targetComponent = std::make_unique<Component> ("testTarget");
        targetComponent->setBounds (100, 100, 200, 30);

        parentComponent->addAndMakeVisible (*targetComponent);
    }

    void TearDown() override
    {
        PopupMenu::dismissAllPopups();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
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

TEST_F (PopupMenuTest, UsesHandCursor)
{
    auto menu = PopupMenu::create();
    EXPECT_EQ (MouseCursor::Hand, menu->getMouseCursor().getType());
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

    menu->addItem (kPopupTestText1, kPopupTestId1);

    EXPECT_EQ (1, menu->getNumItems());
    EXPECT_FALSE (menu->isEmpty());
}

TEST_F (PopupMenuTest, AddMultipleItemsWithShortcuts)
{
    auto menu = PopupMenu::create();

    menu->addItem (kPopupTestText1, kPopupTestId1, true, false, kShortcutText);
    menu->addItem (kPopupTestText2, kPopupTestId2, false, true); // Disabled and ticked
    menu->addItem (kPopupTestText3, kPopupTestId3);

    EXPECT_EQ (3, menu->getNumItems());

    // Verify items can be iterated
    int itemCount = 0;
    for (const auto& item : *menu)
    {
        itemCount++;
    }
    EXPECT_EQ (3, itemCount);
}

TEST_F (PopupMenuTest, TellsItsItemsAsSeriesADrawingReadsWithOneIndex)
{
    auto subMenu = PopupMenu::create();
    subMenu->addItem ("Sub Item 1", 10);

    auto menu = PopupMenu::create();
    menu->addItem (kPopupTestText1, kPopupTestId1, true, false, kShortcutText);
    menu->addSeparator();
    menu->addItem (kPopupTestText2, kPopupTestId2, false, true);
    menu->addSubMenu (kSubMenuText, subMenu, true);

    const auto read = [&menu] (const char* name)
    {
        return menu->getProperty (Identifier (name));
    };

    // One value for each item in every series, so that one index reads all there is to tell of an item
    for (const auto* name : { "itemBounds", "itemTexts", "itemShortcuts", "itemColors", "itemEnabled", "itemTicked", "itemHovered", "itemSeparator", "itemSubMenu", "itemActive", "itemCustom" })
    {
        const auto* property = menu->getTypeInfo().findProperty (Identifier (name));

        ASSERT_NE (property, nullptr) << name;
        EXPECT_TRUE (property->isSeries()) << name;
        EXPECT_EQ (read (name).size(), 4) << name;
    }

    EXPECT_EQ (read ("itemTexts")[0].toString(), kPopupTestText1);
    EXPECT_EQ (read ("itemShortcuts")[0].toString(), kShortcutText);
    EXPECT_TRUE (read ("itemShortcuts")[2].toString().isEmpty());
    EXPECT_FALSE (static_cast<bool> (read ("itemSeparator")[0]));
    EXPECT_TRUE (static_cast<bool> (read ("itemSeparator")[1]));
    EXPECT_TRUE (static_cast<bool> (read ("itemEnabled")[0]));
    EXPECT_FALSE (static_cast<bool> (read ("itemEnabled")[2]));
    EXPECT_TRUE (static_cast<bool> (read ("itemTicked")[2]));
    EXPECT_TRUE (static_cast<bool> (read ("itemSubMenu")[3]));
    EXPECT_FALSE (static_cast<bool> (read ("itemHovered")[0]));
    EXPECT_FALSE (static_cast<bool> (read ("itemActive")[3]));
    EXPECT_FALSE (static_cast<bool> (read ("itemCustom")[0]));

    // A drawing paints the menu from them, a turn of a repeat for each item
    const auto parsed = YuiParser::parse (R"yui(
        drawing Menu {
            rect { bounds: bounds; cornerRadius: 6; fill: palette.surfaceRaised; }

            repeat i in 0 .. len (itemTexts) {
                rect { id: highlight; bounds: itemBounds[i].reduced (2, 1); cornerRadius: 3; fill: palette.accent.withAlpha (0.3);
                       visible: itemHovered[i] && itemEnabled[i] && ! itemSeparator[i]; }

                line { id: rule; from: point (itemBounds[i].x + 8, itemBounds[i].centerY); to: point (itemBounds[i].right - 8, itemBounds[i].centerY);
                       stroke: palette.outline; visible: itemSeparator[i]; }

                text { id: label; bounds: itemBounds[i].reduced (12, 2); text: itemTexts[i]; justification: centerLeft;
                       fill: itemEnabled[i] ? palette.text : palette.textMuted; visible: ! itemSeparator[i] && ! itemCustom[i]; }

                text { id: shortcut; bounds: itemBounds[i].reduced (12, 2); text: itemShortcuts[i]; justification: centerRight;
                       fill: palette.textMuted; visible: len (itemShortcuts[i]) > 0; }
            }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    const auto style = DrawingStyle::create (*menu, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    int numLabelRows = 0;

    for (const auto& element : style.getReference()->measureElements (*menu))
        if (element.id == "label")
            numLabelRows = element.numRows;

    EXPECT_EQ (numLabelRows, 4);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (200, 120);
    Graphics g (*context, *renderer);

    style.getReference()->paint (g, *theme, *menu);
}

TEST_F (PopupMenuTest, AddSeparatorIncreasesCount)
{
    auto menu = PopupMenu::create();

    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addSeparator();
    menu->addItem (kPopupTestText2, kPopupTestId2);

    EXPECT_EQ (3, menu->getNumItems());
}

TEST_F (PopupMenuTest, AddSubMenu)
{
    auto menu = PopupMenu::create();
    auto subMenu = PopupMenu::create();

    subMenu->addItem ("Sub Item 1", 10);
    subMenu->addItem ("Sub Item 2", 11);

    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addSubMenu (kSubMenuText, subMenu, true);

    EXPECT_EQ (2, menu->getNumItems());
}

TEST_F (PopupMenuTest, AddCustomItem)
{
    auto menu = PopupMenu::create();
    auto customComponent = std::make_unique<Label> ("customLabel");
    customComponent->setText ("Custom Menu Item");
    customComponent->setSize (150, 25);

    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addCustomItem (std::move (customComponent), kCustomItemId);

    EXPECT_EQ (2, menu->getNumItems());
}

TEST_F (PopupMenuTest, ClearRemovesAllItems)
{
    auto menu = PopupMenu::create();

    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addSeparator();
    menu->addItem (kPopupTestText2, kPopupTestId2);

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
    PopupMenu::Item textItem (kPopupTestText1, kPopupTestId1, true, false);

    EXPECT_EQ (kPopupTestText1, textItem.text);
    EXPECT_EQ (kPopupTestId1, textItem.itemID);
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
    menu->addItem (kPopupTestText1, kPopupTestId1);

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
    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addItem (kPopupTestText2, kPopupTestId2);

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
    menu->addItem (kPopupTestText1, kPopupTestId1);

    int selectedId = -1;
    menu->onItemSelected = [&selectedId] (int itemId)
    {
        selectedId = itemId;
    };

    EXPECT_EQ (-1, selectedId);

    // Simulate calling the callback directly
    if (menu->onItemSelected)
        menu->onItemSelected (kPopupTestId1);

    EXPECT_EQ (kPopupTestId1, selectedId);
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

TEST_F (PopupMenuTest, TakesStyleColorsFromTargetComponentTree)
{
    const auto sectionColor = Color (0xff112233);

    Component section ("section");
    section.setBounds (0, 0, 400, 300);
    section.setColor (PopupMenu::Style::menuBackground, sectionColor);
    parentComponent->addAndMakeVisible (section);

    Component target ("target");
    target.setBounds (10, 10, 100, 20);
    section.addAndMakeVisible (target);

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (&target));
    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->show();

    EXPECT_EQ (parentComponent.get(), menu->getParentComponent());
    EXPECT_EQ (sectionColor, menu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_FALSE (menu->findColor (PopupMenu::Style::menuBorder).has_value());

    PopupMenu::dismissAllPopups();
}

TEST_F (PopupMenuTest, TakesPaletteFromTargetComponentTree)
{
    const ThemePalette sectionPalette (std::vector<Color> { Color (0xff101010), Color (0xff0080ff) });

    Component section ("section");
    section.setBounds (0, 0, 400, 300);
    section.setPalette (sectionPalette);
    parentComponent->addAndMakeVisible (section);

    Component target ("target");
    target.setBounds (10, 10, 100, 20);
    section.addAndMakeVisible (target);

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (&target));
    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->show();

    ASSERT_NE (nullptr, menu->getPalette());
    EXPECT_EQ (sectionPalette, *menu->getPalette());

    PopupMenu::dismissAllPopups();
}

TEST_F (PopupMenuTest, KeepsOwnStyleColorsOverTargetComponentTree)
{
    const auto ownColor = Color (0xff445566);
    targetComponent->setColor (PopupMenu::Style::menuItemText, Color (0xff778899));

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (targetComponent.get()));
    menu->setColor (PopupMenu::Style::menuItemText, ownColor);
    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->show();

    EXPECT_EQ (ownColor, menu->findColor (PopupMenu::Style::menuItemText));

    PopupMenu::dismissAllPopups();
}

TEST_F (PopupMenuTest, SubmenuTakesStyleColorsFromMenu)
{
    const auto targetColor = Color (0xff223344);
    targetComponent->setColor (PopupMenu::Style::menuItemText, targetColor);

    auto subMenu = PopupMenu::create();
    subMenu->addItem (kPopupTestText2, kPopupTestId2);

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (targetComponent.get()));
    menu->addSubMenu (kSubMenuText, subMenu);
    menu->show();

    menu->keyDown (KeyPress (KeyPress::downKey), {});
    menu->keyDown (KeyPress (KeyPress::rightKey), {});

    ASSERT_TRUE (subMenu->isVisible());
    EXPECT_EQ (targetColor, subMenu->findColor (PopupMenu::Style::menuItemText));

    PopupMenu::dismissAllPopups();
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

    menu->addItem ("First Item", kPopupTestId1);
    menu->addItem ("Second Item", kPopupTestId1); // Same ID
    menu->addItem ("Third Item", kPopupTestId1);  // Same ID again

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

    menu->addItem (longText, kPopupTestId1);
    EXPECT_EQ (1, menu->getNumItems());
}

TEST_F (PopupMenuTest, WidthUsesTextSizeAndRespectsLimits)
{
    auto oldTheme = ApplicationTheme::getGlobalTheme();
    auto theme = createThemeVersion1();
    ApplicationTheme::setGlobalTheme (theme);
    const ScopeGuard restoreTheme { [&]
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
    } };

    PopupMenu::Options options;
    options.withParentComponent (parentComponent.get())
        .withPosition (Point<int> (10, 10))
        .withMinimumWidth (120)
        .withMaximumWidth (260);

    auto menu = PopupMenu::create (options);
    menu->addItem ("A very long popup menu item that should be measured before the menu is shown", kPopupTestId1);
    menu->show();

    EXPECT_GE (menu->getWidth(), 120);
    EXPECT_LE (menu->getWidth(), 260);
    EXPECT_EQ (260, menu->getWidth());

    menu->dismiss();
}

TEST_F (PopupMenuTest, SpecialCharactersInText)
{
    auto menu = PopupMenu::create();

    menu->addItem (L"Item with üñíçødé", kPopupTestId1);
    menu->addItem ("Item with\nNewline", kPopupTestId2);
    menu->addItem ("Item with\tTab", kPopupTestId3);
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

    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addSeparator();
    menu->addItem (kPopupTestText2, kPopupTestId2);

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

//==============================================================================
// Scrolling Tests

TEST_F (PopupMenuTest, MenuWithManyItemsInSmallSpace)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 200, 120); // Small height to force scrolling

    PopupMenu::Options options;
    options.withParentComponent (smallParent.get())
        .withPosition (Point<int> (10, 10));

    auto menu = PopupMenu::create (options);
    for (int i = 1; i <= 30; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();

    // The menu should have been created and be visible
    EXPECT_TRUE (menu->isVisible());
    EXPECT_EQ (30, menu->getNumItems());

    // The menu height should be constrained by the parent
    EXPECT_LE (menu->getHeight(), smallParent->getHeight());

    int laidOutItems = 0;
    int hiddenItems = 0;
    for (const auto& item : *menu)
    {
        if (item->area.isEmpty())
            ++hiddenItems;
        else
            ++laidOutItems;
    }

    EXPECT_GT (laidOutItems, 0);
    EXPECT_GT (hiddenItems, 0);
    EXPECT_LT (laidOutItems, menu->getNumItems());

    // With the new approach, the menu should show even with limited space
    EXPECT_TRUE (menu->isVisible());
}

TEST_F (PopupMenuTest, MouseWheelEventHandling)
{
    // Set up in a small parent
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 200, 100);

    PopupMenu::Options options;
    options.withParentComponent (smallParent.get())
        .withPosition (Point<int> (10, 10));

    auto menu = PopupMenu::create (options);
    for (int i = 1; i <= 25; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();

    // Test that mouse wheel events are handled without crashing
    MouseEvent mouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (50, 50));
    MouseWheelData wheelDown (0.0f, -1.0f); // Scroll down
    MouseWheelData wheelUp (0.0f, 1.0f);    // Scroll up

    // These should not throw or crash
    EXPECT_NO_THROW (menu->mouseWheel (mouseEvent, wheelDown));
    EXPECT_NO_THROW (menu->mouseWheel (mouseEvent, wheelUp));

    // Menu should still be responsive
    EXPECT_TRUE (menu->isVisible());

    // Test multiple scroll events
    for (int i = 0; i < 5; ++i)
        EXPECT_NO_THROW (menu->mouseWheel (mouseEvent, wheelDown));

    EXPECT_TRUE (menu->canScrollUp());

    for (int i = 0; i < 3; ++i)
        EXPECT_NO_THROW (menu->mouseWheel (mouseEvent, wheelUp));

    EXPECT_TRUE (menu->isVisible());
}

TEST_F (PopupMenuTest, ScrollingKeepsRowsAdjacentToScrollIndicators)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    PopupMenu::Options options;
    options.withParentComponent (smallParent.get())
        .withPosition (Point<int> (10, 10));

    auto menu = PopupMenu::create (options);
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();

    MouseEvent mouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (50, 50));
    MouseWheelData wheelDown (0.0f, -1.0f);
    for (int i = 0; i < 100; ++i)
        menu->mouseWheel (mouseEvent, wheelDown);

    EXPECT_TRUE (menu->canScrollUp());
    EXPECT_FALSE (menu->canScrollDown());

    float lastVisibleItemBottom = 0.0f;
    for (const auto& item : *menu)
    {
        if (! item->area.isEmpty())
            lastVisibleItemBottom = jmax (lastVisibleItemBottom, item->area.getBottom());
    }

    const auto downIndicatorTop = menu->getScrollDownBounds().getY();
    EXPECT_LE (lastVisibleItemBottom, downIndicatorTop);
    EXPECT_LE (downIndicatorTop - lastVisibleItemBottom, 4.0f);
}

TEST_F (PopupMenuTest, ShowScrollsTickedItemIntoView)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (smallParent.get()).withPosition (Point<int> (10, 10)));
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i, true, i == 35);

    menu->show();
    ASSERT_TRUE (menu->needsScrolling());

    const auto& ticked = **std::find_if (menu->begin(), menu->end(), [] (const auto& item)
    {
        return item->isTicked;
    });

    EXPECT_TRUE (menu->canScrollUp());
    EXPECT_FALSE (ticked.area.isEmpty());
    EXPECT_GE (ticked.area.getY(), menu->getScrollUpBounds().getBottom());
    EXPECT_LE (ticked.area.getBottom(), menu->getScrollDownBounds().getY());
}

TEST_F (PopupMenuTest, TappingScrollIndicatorsScrollsTheMenu)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (smallParent.get()).withPosition (Point<int> (10, 10)));
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();
    ASSERT_TRUE (menu->needsScrolling());
    EXPECT_FALSE (menu->canScrollUp());

    const auto tap = [&] (Point<float> position)
    {
        const auto event = MouseEvent (MouseEvent::leftButton, KeyModifiers(), position).withTouchIndex (0).withLastMouseDownPosition (position);
        menu->mouseDown (event);
        menu->mouseUp (event);
    };

    tap (menu->getScrollDownBounds().getCenter());
    EXPECT_TRUE (menu->canScrollUp());
    EXPECT_TRUE (menu->isBeingShown());

    tap (menu->getScrollUpBounds().getCenter());
    EXPECT_FALSE (menu->canScrollUp());
    EXPECT_TRUE (menu->isBeingShown());
}

TEST_F (PopupMenuTest, TouchDragScrollsWithoutSelectingOrDismissing)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (smallParent.get()).withPosition (Point<int> (10, 10)));
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    int selectedId = -1;
    menu->show ([&] (int id)
    {
        selectedId = id;
    });

    const auto start = Point<float> (50.0f, menu->getHeight() * 0.5f);
    const auto touch = [&] (Point<float> position)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), position).withTouchIndex (0).withLastMouseDownPosition (start);
    };

    menu->mouseDown (touch (start));
    menu->mouseDrag (touch (start.translated (0.0f, -60.0f)));
    EXPECT_TRUE (menu->canScrollUp());

    // Released past the menu edge: a drag neither picks an item nor dismisses
    menu->mouseUp (touch (start.translated (0.0f, -200.0f)));
    EXPECT_TRUE (menu->isBeingShown());
    EXPECT_EQ (selectedId, -1);

    menu->mouseDown (touch (start));
    menu->mouseDrag (touch (start.translated (0.0f, 200.0f)));
    menu->mouseUp (touch (start.translated (0.0f, 200.0f)));
    EXPECT_FALSE (menu->canScrollUp());
    EXPECT_TRUE (menu->isBeingShown());
    EXPECT_EQ (selectedId, -1);

    // The callback writes to this stack frame: dismiss before leaving it, not in TearDown
    PopupMenu::dismissAllPopups();
    EXPECT_EQ (selectedId, 0);
}

TEST_F (PopupMenuTest, PressDoesNothingUntilRelease)
{
    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (parentComponent.get()).withPosition (Point<int> (10, 10)));

    auto submenu = PopupMenu::create();
    submenu->addItem ("Sub 1", 10);
    menu->addSubMenu ("More", submenu);
    menu->addItem ("Item 1", 1);

    int selectedId = -1;
    menu->show ([&] (int id)
    {
        selectedId = id;
    });

    std::vector<Point<float>> itemCenters;
    for (const auto& item : *menu)
        itemCenters.push_back (item->area.getCenter());

    const auto touch = [] (Point<float> position)
    {
        return MouseEvent (MouseEvent::leftButton, KeyModifiers(), position).withTouchIndex (0).withLastMouseDownPosition (position);
    };

    // A finger may be starting a drag: the press neither opens the submenu nor picks the item
    menu->mouseDown (touch (itemCenters[0]));
    EXPECT_FALSE (menu->isItemShowingSubmenu (0));
    menu->mouseUp (touch (itemCenters[0]));
    EXPECT_TRUE (menu->isItemShowingSubmenu (0));

    menu->mouseDown (touch (itemCenters[1]));
    EXPECT_EQ (selectedId, -1);
    menu->mouseUp (touch (itemCenters[1]));
    EXPECT_EQ (selectedId, 1);
}

TEST_F (PopupMenuTest, MouseDragDoesNotScroll)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (smallParent.get()).withPosition (Point<int> (10, 10)));
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();

    const auto start = Point<float> (50.0f, menu->getHeight() * 0.5f);
    menu->mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), start).withLastMouseDownPosition (start));
    menu->mouseDrag (MouseEvent (MouseEvent::leftButton, KeyModifiers(), start.translated (0.0f, -60.0f)).withLastMouseDownPosition (start));

    EXPECT_FALSE (menu->canScrollUp());
}

TEST_F (PopupMenuTest, ScrollingWithCustomComponents)
{
    // Use small parent to trigger scrolling behavior
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 200, 150);

    PopupMenu::Options options;
    options.withParentComponent (smallParent.get())
        .withPosition (Point<int> (10, 10));

    auto menu = PopupMenu::create (options);
    for (int i = 1; i <= 15; ++i)
    {
        menu->addItem (String ("Item ") + String (i), i);

        // Add custom component every few items
        if (i % 3 == 0)
        {
            auto customComponent = std::make_unique<Label> ("customLabel" + String (i));
            customComponent->setText ("Custom " + String (i));
            customComponent->setSize (150, 30);
            menu->addCustomItem (std::move (customComponent), 1000 + i);
        }
    }

    menu->show();

    // Should handle mixed content properly
    EXPECT_TRUE (menu->isVisible());
    EXPECT_GT (menu->getNumItems(), 15); // Should have both regular and custom items

    // Test mouse wheel with mixed content
    MouseEvent mouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (50, 50));
    MouseWheelData wheel (0.0f, -1.0f);
    EXPECT_NO_THROW (menu->mouseWheel (mouseEvent, wheel));

    // Menu should still be functional after scrolling with custom components
    EXPECT_TRUE (menu->isVisible());
}

TEST_F (PopupMenuTest, IsBeingShownReturnsCorrectState)
{
    auto menu = PopupMenu::create();
    EXPECT_FALSE (menu->isBeingShown());

    parentComponent->setVisible (true);

    menu->addItem ("Item 1", 1);
    menu->show();
    EXPECT_TRUE (menu->isBeingShown());

    menu->dismissAllPopups();
    EXPECT_FALSE (menu->isBeingShown());
}

TEST_F (PopupMenuTest, IsItemShowingSubmenu)
{
    auto menu = PopupMenu::create();
    menu->addItem ("Item 1", 1);

    auto sub = PopupMenu::create();
    sub->addItem ("Sub 1", 10);
    menu->addSubMenu ("More", *sub, true);

    EXPECT_FALSE (menu->isItemShowingSubmenu (0));
    EXPECT_FALSE (menu->isItemShowingSubmenu (1));

    menu->show();
    EXPECT_FALSE (menu->isItemShowingSubmenu (0));
}

TEST_F (PopupMenuTest, NeedsScrolling)
{
    auto menu = PopupMenu::create();
    EXPECT_FALSE (menu->needsScrolling());

    for (int i = 1; i <= 100; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();
    EXPECT_TRUE (menu->needsScrolling());

    auto empty = PopupMenu::create();
    EXPECT_FALSE (empty->needsScrolling());
}

TEST_F (PopupMenuTest, GetScrollUpBounds)
{
    auto menu = PopupMenu::create();
    for (int i = 1; i <= 100; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();
    const auto upBounds = menu->getScrollUpBounds();
    EXPECT_FALSE (upBounds.isEmpty());
}

TEST_F (PopupMenuTest, TellsWhereAnItemPutsItsTextItsTickItsShortcutAndItsArrow)
{
    auto subMenu = PopupMenu::create();
    subMenu->addItem ("Sub Item 1", 10);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (parentComponent.get()).withPosition (Point<int> (10, 10)));
    menu->addItem (kPopupTestText1, kPopupTestId1, true, false, kShortcutText);
    menu->addSeparator();
    menu->addItem (kPopupTestText2, kPopupTestId2, true, true);
    menu->addSubMenu (kSubMenuText, subMenu, true);
    menu->show();

    const auto areaOf = [&menu] (int itemIndex)
    {
        return (*(menu->begin() + itemIndex))->area;
    };

    const auto withShortcut = areaOf (0);
    const auto ticked = areaOf (2);
    const auto withSubMenu = areaOf (3);
    ASSERT_FALSE (withShortcut.isEmpty());
    ASSERT_FALSE (ticked.isEmpty());
    ASSERT_FALSE (withSubMenu.isEmpty());

    // The text is inside the padding of its item. One item is ticked, so all of them leave the room of a tick before it
    EXPECT_EQ (menu->getItemTextBounds (0), withShortcut.reduced (12.0f, 2.0f).withTrimmedLeft (8.0f).withTrimmedRight (80.0f));
    EXPECT_EQ (menu->getItemTextBounds (2), ticked.reduced (12.0f, 2.0f).withTrimmedLeft (8.0f));
    EXPECT_EQ (menu->getItemTextBounds (3), withSubMenu.reduced (12.0f, 2.0f).withTrimmedLeft (8.0f).withTrimmedRight (24.0f));

    // The tick is before the text, the shortcut and the arrow after it, all inside the item
    const auto tick = menu->getItemTickBounds (2);
    EXPECT_EQ (tick, Rectangle<float> (ticked.getX() + 4.0f, ticked.getCenterY() - 6.0f, 12.0f, 12.0f));
    EXPECT_LE (tick.getRight(), menu->getItemTextBounds (2).getX());
    EXPECT_TRUE (ticked.contains (tick));

    const auto shortcut = menu->getItemShortcutBounds (0);
    EXPECT_EQ (shortcut, Rectangle<float> (withShortcut.getRight() - 80.0f, withShortcut.getY() + 2.0f, 75.0f, withShortcut.getHeight() - 2.0f));
    EXPECT_GE (shortcut.getX(), menu->getItemTextBounds (0).getRight());
    EXPECT_TRUE (withShortcut.contains (shortcut));

    const auto arrow = menu->getItemArrowBounds (3);
    EXPECT_EQ (arrow, Rectangle<float> (withSubMenu.getRight() - 16.0f, withSubMenu.getCenterY() - 6.0f, 8.0f, 12.0f));
    EXPECT_GE (arrow.getX(), menu->getItemTextBounds (3).getRight());
    EXPECT_TRUE (withSubMenu.contains (arrow));

    // An item has only the areas of what it shows, and a separator none
    EXPECT_TRUE (menu->getItemTickBounds (0).isEmpty());
    EXPECT_TRUE (menu->getItemArrowBounds (0).isEmpty());
    EXPECT_TRUE (menu->getItemShortcutBounds (2).isEmpty());
    EXPECT_TRUE (menu->getItemShortcutBounds (3).isEmpty());
    EXPECT_FALSE (areaOf (1).isEmpty());
    EXPECT_TRUE (menu->getItemTextBounds (1).isEmpty());
    EXPECT_TRUE (menu->getItemTextBounds (-1).isEmpty());
    EXPECT_TRUE (menu->getItemTextBounds (4).isEmpty());

    menu->dismiss();
}

TEST_F (PopupMenuTest, AnItemScrolledOutOfTheMenuHasNoTextBounds)
{
    auto smallParent = std::make_unique<Component> ("smallParent");
    smallParent->setBounds (0, 0, 220, 130);

    auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (smallParent.get()).withPosition (Point<int> (10, 10)));
    for (int i = 1; i <= 40; ++i)
        menu->addItem (String ("Item ") + String (i), i);

    menu->show();
    ASSERT_TRUE (menu->needsScrolling());

    // No item is ticked, so the text starts at the padding of its item
    const auto first = (*menu->begin())->area;
    ASSERT_FALSE (first.isEmpty());
    EXPECT_EQ (menu->getItemTextBounds (0), first.reduced (12.0f, 2.0f));

    ASSERT_TRUE ((*(menu->begin() + 39))->area.isEmpty());
    EXPECT_TRUE (menu->getItemTextBounds (39).isEmpty());

    menu->dismiss();
}

TEST_F (PopupMenuTest, OptionsWithFocusComponent)
{
    auto menu = PopupMenu::create (PopupMenu::Options().withFocusComponent (parentComponent.get()));
    menu->addItem ("Item 1", 1);

    EXPECT_NO_THROW (menu->show());
    EXPECT_TRUE (menu->isVisible());
}

TEST_F (PopupMenuTest, DismissingAllPopupsFromTheSelectionCallbackIsSafe)
{
    PopupMenu::dismissAllPopups();

    // Nothing else holds the menu while the callback runs, so this drops its last reference.
    PopupMenu* rawMenu = nullptr;
    WeakReference<Component> menuReference;
    int selectedId = -1;

    {
        auto menu = PopupMenu::create (PopupMenu::Options().withParentComponent (parentComponent.get()).withPosition (Point<int> (10, 10)));
        menu->addItem (kPopupTestText1, kPopupTestId1);

        menu->show ([&selectedId] (int id)
        {
            selectedId = id;
            PopupMenu::dismissAllPopups();
        });

        rawMenu = menu.get();
        menuReference = rawMenu;
    }

    const auto& firstItem = **rawMenu->begin();
    const auto itemCenter = firstItem.area.getCenter();
    const auto click = MouseEvent (MouseEvent::leftButton, KeyModifiers(), itemCenter)
                           .withLastMouseDownPosition (itemCenter);

    rawMenu->mouseDown (click);
    rawMenu->mouseUp (click);

    EXPECT_EQ (kPopupTestId1, selectedId);
    EXPECT_EQ (nullptr, menuReference.get());
}

TEST_F (PopupMenuTest, StyleIdentifiersContainAllStyleIds)
{
    auto id = PopupMenu::Style::menuItemBackgroundActiveSubmenu;
    EXPECT_FALSE (id.isNull());
    EXPECT_NE (id, PopupMenu::Style::menuItemBackground);
    EXPECT_NE (id, PopupMenu::Style::menuItemBackgroundHighlighted);
    EXPECT_NE (id, PopupMenu::Style::menuItemText);
    EXPECT_NE (id, PopupMenu::Style::menuItemTextDisabled);
    EXPECT_FALSE (PopupMenu::Style::cornerRadiusId.isNull());
    EXPECT_FALSE (PopupMenu::Style::outlineWidthId.isNull());
    EXPECT_FALSE (PopupMenu::Style::itemCornerRadiusId.isNull());
    EXPECT_FALSE (PopupMenu::Style::itemHeightId.isNull());
    EXPECT_FALSE (PopupMenu::Style::verticalPaddingId.isNull());
}

TEST_F (PopupMenuTest, TypeInfoExposesStyleablePropertiesAndSignal)
{
    const auto& info = PopupMenu::getClassTypeInfo();

    EXPECT_EQ (Identifier ("PopupMenu"), info.getTypeName());
    EXPECT_FALSE (info.hasFactory());

    for (const auto* propName : { "backgroundColor",
                                  "borderColor",
                                  "itemTextColor",
                                  "itemTextDisabledColor",
                                  "itemBackgroundColor",
                                  "itemHighlightedBackgroundColor",
                                  "itemActiveSubmenuBackgroundColor",
                                  "cornerRadius",
                                  "outlineWidth",
                                  "itemCornerRadius",
                                  "itemHeight",
                                  "verticalPadding" })
    {
        const auto* prop = info.findProperty (Identifier (propName));
        ASSERT_NE (nullptr, prop) << propName;
        EXPECT_TRUE (prop->isStyleable) << propName;
    }

    EXPECT_EQ (ComponentProperty::LayoutEffect::own, info.findProperty ("itemHeight")->layoutEffect);
    EXPECT_EQ (ComponentProperty::LayoutEffect::own, info.findProperty ("verticalPadding")->layoutEffect);
    EXPECT_NE (nullptr, info.findSignal ("onItemSelected"));
}

TEST_F (PopupMenuTest, StyleSheetStylesPopupOpenedOutsideStyledRoot)
{
    Component styledPanel ("panel");
    styledPanel.setBounds (20, 20, 400, 300);
    parentComponent->addAndMakeVisible (styledPanel);

    ComboBox combo ("filterCombo");
    combo.setBounds (10, 10, 160, 28);
    styledPanel.addAndMakeVisible (combo);

    auto sheet = YuiParser::parseStyleSheet (R"(
        palette Warm {
            colors: "#14161a #1e222a #ff8844 #f0f4f8";
            surfaceRaised: #1e222a;
            accent: #ff8844;
            outline: #334455;
            text: #f0f4f8;
        }
        style {
            :root {
                palette: Warm;
            }
            PopupMenu {
                backgroundColor: palette.surfaceRaised;
                itemTextColor: palette.text;
                cornerRadius: 10;
                itemCornerRadius: 6;
                itemHeight: 30;
                verticalPadding: 8;
            }
            ComboBox > PopupMenu {
                borderColor: palette.accent;
            }
        }
    )");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    styledPanel.setStyleSheet (sheet.getReference());
    styledPanel.getStyleContext()->restyleNow();

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (&combo));
    menu->addItem (kPopupTestText1, kPopupTestId1);
    menu->addItem (kPopupTestText2, kPopupTestId2);
    menu->show();

    EXPECT_EQ (parentComponent.get(), menu->getParentComponent());
    EXPECT_EQ (Color (0xff1e222a), menu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_EQ (Color (0xfff0f4f8), menu->findColor (PopupMenu::Style::menuItemText));
    EXPECT_EQ (Color (0xffff8844), menu->findColor (PopupMenu::Style::menuBorder));
    EXPECT_FLOAT_EQ (10.0f, menu->findPropertyAs<float> ("cornerRadius"));
    EXPECT_FLOAT_EQ (6.0f, menu->findPropertyAs<float> ("itemCornerRadius"));
    EXPECT_FLOAT_EQ (30.0f, menu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (8.0f, menu->findPropertyAs<float> ("verticalPadding"));
    EXPECT_FLOAT_EQ (76.0f, menu->getHeight());

    const auto& firstItem = **menu->begin();
    EXPECT_FLOAT_EQ (8.0f, firstItem.area.getY());
    EXPECT_FLOAT_EQ (30.0f, firstItem.area.getHeight());

    PopupMenu::dismissAllPopups();
}

TEST_F (PopupMenuTest, SubmenuAndNestedSubmenuInheritStyleFromParentPopupMenu)
{
    auto sheet = YuiParser::parseStyleSheet (R"(
        style {
            PopupMenu {
                backgroundColor: #aa0000;
                itemHeight: 18;
                cornerRadius: 3;
            }
            #contextMenu {
                backgroundColor: #123456;
                borderColor: #654321;
                itemTextColor: #ddeeff;
                cornerRadius: 12;
                itemCornerRadius: 5;
                itemHeight: 28;
                verticalPadding: 6;
                shadow: 0 4 12 #00000080;
                filter: brightness(1.1);
            }
        }
    )");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    parentComponent->setStyleSheet (sheet.getReference());
    parentComponent->getStyleContext()->restyleNow();

    auto nestedSubMenu = PopupMenu::create();
    nestedSubMenu->setColor (PopupMenu::Style::menuItemText, Color (0xff00ff88));
    nestedSubMenu->addItem ("Deep Item", 30);

    auto subMenu = PopupMenu::create();
    subMenu->addSubMenu ("Deep Menu", nestedSubMenu);
    subMenu->addItem (kPopupTestText2, kPopupTestId2);

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (targetComponent.get()));
    menu->setComponentID ("contextMenu");
    menu->addSubMenu (kSubMenuText, subMenu);
    menu->show();

    menu->keyDown (KeyPress (KeyPress::downKey), {});
    menu->keyDown (KeyPress (KeyPress::rightKey), {});

    ASSERT_TRUE (subMenu->isVisible());
    EXPECT_TRUE (subMenu->isSubmenu());
    EXPECT_EQ (menu.get(), subMenu->getParentMenu());
    EXPECT_EQ (Color (0xff123456), subMenu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_EQ (Color (0xff654321), subMenu->findColor (PopupMenu::Style::menuBorder));
    EXPECT_EQ (Color (0xffddeeff), subMenu->findColor (PopupMenu::Style::menuItemText));
    EXPECT_FLOAT_EQ (12.0f, subMenu->findPropertyAs<float> ("cornerRadius"));
    EXPECT_FLOAT_EQ (5.0f, subMenu->findPropertyAs<float> ("itemCornerRadius"));
    EXPECT_FLOAT_EQ (28.0f, subMenu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (6.0f, subMenu->findPropertyAs<float> ("verticalPadding"));
    EXPECT_FLOAT_EQ (68.0f, subMenu->getHeight());

    ASSERT_NE (nullptr, subMenu->getStyleState());
    EXPECT_EQ (1, static_cast<int> (subMenu->getStyleState()->getShadows().size()));
    EXPECT_NE (nullptr, subMenu->getStyleState()->getFilterEffect());

    // Open the nested submenu and check multi-level inheritance plus explicit override preservation
    subMenu->keyDown (KeyPress (KeyPress::rightKey), {});

    ASSERT_TRUE (nestedSubMenu->isVisible());
    EXPECT_TRUE (nestedSubMenu->isSubmenu());
    EXPECT_EQ (subMenu.get(), nestedSubMenu->getParentMenu());
    EXPECT_EQ (Color (0xff123456), nestedSubMenu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_EQ (Color (0xff00ff88), nestedSubMenu->findColor (PopupMenu::Style::menuItemText));
    EXPECT_FLOAT_EQ (12.0f, nestedSubMenu->findPropertyAs<float> ("cornerRadius"));
    EXPECT_FLOAT_EQ (28.0f, nestedSubMenu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (40.0f, nestedSubMenu->getHeight());

    PopupMenu::dismissAllPopups();

    EXPECT_FALSE (subMenu->isSubmenu());
    EXPECT_EQ (nullptr, subMenu->getParentMenu());
    EXPECT_FALSE (nestedSubMenu->isSubmenu());
    EXPECT_EQ (nullptr, nestedSubMenu->getParentMenu());
}

TEST_F (PopupMenuTest, QualifiedDeclarationsAndLiveRestylePropagateToPopupAndSubmenu)
{
    Component toolbar ("toolbar");
    toolbar.addStyleClass ("toolbar");
    toolbar.setBounds (0, 0, 400, 60);
    parentComponent->addAndMakeVisible (toolbar);

    Component trigger ("trigger");
    trigger.setBounds (10, 10, 80, 24);
    toolbar.addAndMakeVisible (trigger);

    auto sheet = YuiParser::parseStyleSheet (R"(
        style {
            .toolbar {
                PopupMenu.backgroundColor: #224466;
                PopupMenu.itemHeight: 26;
            }
            .toolbar.compact {
                PopupMenu.backgroundColor: #335577;
                PopupMenu.itemHeight: 20;
            }
        }
    )");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    parentComponent->setStyleSheet (sheet.getReference());
    parentComponent->getStyleContext()->restyleNow();

    auto subMenu = PopupMenu::create();
    subMenu->addItem (kPopupTestText2, kPopupTestId2);

    auto menu = PopupMenu::create (PopupMenu::Options()
                                       .withParentComponent (parentComponent.get())
                                       .withRelativePosition (&trigger));
    menu->addSubMenu (kSubMenuText, subMenu);
    menu->show();

    menu->keyDown (KeyPress (KeyPress::downKey), {});
    menu->keyDown (KeyPress (KeyPress::rightKey), {});
    ASSERT_TRUE (subMenu->isVisible());

    EXPECT_EQ (Color (0xff224466), menu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_EQ (Color (0xff224466), subMenu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_FLOAT_EQ (26.0f, menu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (26.0f, subMenu->findPropertyAs<float> ("itemHeight"));

    toolbar.addStyleClass ("compact");
    parentComponent->getStyleContext()->restyleNow();

    EXPECT_EQ (Color (0xff335577), menu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_EQ (Color (0xff335577), subMenu->findColor (PopupMenu::Style::menuBackground));
    EXPECT_FLOAT_EQ (20.0f, menu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (20.0f, subMenu->findPropertyAs<float> ("itemHeight"));
    EXPECT_FLOAT_EQ (28.0f, subMenu->getHeight());

    PopupMenu::dismissAllPopups();
}
