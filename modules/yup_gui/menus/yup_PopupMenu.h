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
/**
    A popup menu that can display a list of items.

    This class supports both native system menus and custom rendered menus.
*/
class YUP_API PopupMenu : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** Convenience typedef for a reference-counted pointer to a PopupMenu. */
    using Ptr = ReferenceCountedObjectPtr<PopupMenu>;

    //==============================================================================
    /** Options for showing the popup menu. */
    struct Options
    {
        Options();

        /** The parent component to attach the menu to. */
        Options& withParentComponent (Component* parentComponent);

        /** The area to position the menu relative to. */
        Options& withTargetArea (const Rectangle<int>& targetArea);

        /** How to position the menu relative to the target area. */
        Options& withJustification (Justification justification);

        /** The position to show the menu at (relative to parent). */
        Options& withTargetPosition (const Point<int>& targetPosition);

        /** Minimum width for the menu. */
        Options& withMinimumWidth (int minWidth);

        /** Maximum width for the menu. */
        Options& withMaximumWidth (int maxWidth);

        /** Whether to add the menu as a child to the topmost component. */
        Options& withAsChildToTopmost (bool addAsChildToTopmost);

        /** Whether to use native system menus (when available). */
        Options& withNativeMenus (bool useNativeMenus);

        Component* parentComponent;
        Point<int> targetPosition;
        Rectangle<int> targetArea;
        Justification justification;
        std::optional<int> minWidth;
        std::optional<int> maxWidth;
        bool dismissOnSelection;
        bool addAsChildToTopmost;
        bool useNativeMenus;
    };

    //==============================================================================
    /** Destructor. */
    ~PopupMenu();

    //==============================================================================
    /** Creates a popup menu with the given options.

        @param options The options for the popup menu.

        @return A pointer to the popup menu.
    */
    static Ptr create (const Options& options = {});

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
    void show (std::function<void (int)> callback = nullptr);

    //==============================================================================
    /** Callback type for menu item selection. */
    std::function<void (int selectedItemID)> onItemSelected;

    //==============================================================================
    /** Dismisses all currently open popup menus. */
    static void dismissAllPopups();

    //==============================================================================
    class MenuWindow;

private:
    //==============================================================================
    friend class MenuWindow;

    PopupMenu (const Options& options = {});
    void showCustom (const Options& options, std::function<void (int)> callback);

    // PopupMenuItem is now an implementation detail
    class PopupMenuItem;
    std::vector<std::unique_ptr<PopupMenuItem>> items;

    Options options;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupMenu)
};

} // namespace yup
