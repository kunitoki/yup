/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace yup
{

//==============================================================================
class PopupMenu;

/**
    A popup menu that can display a list of items.

    This class supports both native system menus and custom rendered menus.
*/
class JUCE_API PopupMenu : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** Convenience typedef for a reference-counted pointer to a PopupMenu. */
    using Ptr = ReferenceCountedObjectPtr<PopupMenu>;

    //==============================================================================
    /** Options for showing the popup menu. */
    struct Options
    {
        Options()
            : useNativeMenus (false)
            , parentComponent (nullptr)
            , minWidth (0)
            , maxWidth (0)
            , standardItemHeight (22)
            , dismissOnSelection (true)
        {
        }

        /** Whether to use native system menus (when available). */
        bool useNativeMenus;

        /** The parent component to attach the menu to. */
        Component* parentComponent;

        /** The position to show the menu at (relative to parent). */
        Point<int> targetScreenPosition;

        /** The area to position the menu relative to. */
        Rectangle<int> targetArea;

        /** Minimum width for the menu. */
        int minWidth;

        /** Maximum width for the menu. */
        int maxWidth;

        /** Standard menu item height. */
        int standardItemHeight;

        /** Whether to dismiss the menu when an item is selected. */
        bool dismissOnSelection;
    };

    //==============================================================================
    /** Creates an empty popup menu. */
    PopupMenu();

    /** Destructor. */
    ~PopupMenu();

    //==============================================================================

    static Ptr create();

    //==============================================================================
    /** Adds a menu item.

        @param text         The text to display
        @param itemID       Unique ID for this item
        @param isEnabled    Whether the item is enabled
        @param isTicked     Whether to show a checkmark
        @param shortcutText Optional shortcut key text
    */
    void addItem (const String& text, int itemID, bool isEnabled = true, bool isTicked = false, const String& shortcutText = {});



    /** Adds a separator line. */
    void addSeparator();

    /** Adds a sub-menu.

        @param text     The text to display for the sub-menu
        @param subMenu  The sub-menu to show
        @param isEnabled Whether the sub-menu is enabled
    */
    void addSubMenu (const String& text, PopupMenu::Ptr subMenu, bool isEnabled = true);

    /** Adds a custom component as a menu item (non-native mode only).

        @param component    The component to add
        @param itemID       Unique ID for this item
    */
    void addCustomItem (std::unique_ptr<Component> component, int itemID);

    /** Adds all items from another menu. */
    void addItemsFromMenu (const PopupMenu& otherMenu);

    //==============================================================================
    /** Returns the number of items in the menu. */
    int getNumItems() const;

    /** Returns true if the menu is empty. */
    bool isEmpty() const { return getNumItems() == 0; }

    /** Clears all items from the menu. */
    void clear();

    //==============================================================================
    /** Shows the menu asynchronously and calls the callback when an item is selected.

        @param options      Options for showing the menu
        @param callback     Function to call when an item is selected (optional)
    */
    void show (const Options& options = Options{}, std::function<void(int)> callback = nullptr);

    /** Shows the menu at a specific screen position.

        @param screenPos    Screen position to show the menu
        @param callback     Function to call when an item is selected (optional)
    */
    void showAt (Point<int> screenPos, std::function<void(int)> callback = nullptr);

    /** Shows the menu relative to a component.

        @param targetComp   Component to show the menu relative to
        @param callback     Function to call when an item is selected (optional)
    */
    void showAt (Component* targetComp, std::function<void(int)> callback = nullptr);

    //==============================================================================
    /** Callback type for menu item selection. */
    std::function<void(int selectedItemID)> onItemSelected;

    //==============================================================================
    /** Dismisses all currently open popup menus. */
    static void dismissAllPopups();

    //==============================================================================
    class MenuWindow;

private:
    //==============================================================================
    friend class MenuWindow;

    // PopupMenuItem is now an implementation detail
    class PopupMenuItem;
    std::vector<std::unique_ptr<PopupMenuItem>> items;

    void showCustom (const Options& options, std::function<void(int)> callback);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupMenu)
};

} // namespace yup
