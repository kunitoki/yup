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
class YUP_API PopupMenu
    : public Component
    , public ReferenceCountedObject
{
public:
    //==============================================================================
    /** Convenience typedef for a reference-counted pointer to a PopupMenu. */
    using Ptr = ReferenceCountedObjectPtr<PopupMenu>;

    //==============================================================================
    /** Menu positioning relative to rectangles/components */
    enum class Side
    {
        above,   //< Menu appears above the target
        below,   //< Menu appears below the target (default)
        toLeft,  //< Menu appears to the left of the target
        toRight, //< Menu appears to the right of the target
        centered //< Menu is centered on the target
    };

    /** Placement of the menu relative to the target. */
    struct Placement
    {
        Side side = Side::below;
        Justification alignment = Justification::topLeft;

        /** Constructor. */
        Placement() = default;

        /** Constructor. */
        Placement (Side s, Justification align = Justification::topLeft)
            : side (s)
            , alignment (align)
        {
        }

        /** Returns a placement below the target. */
        static Placement below (Justification align = Justification::topLeft) { return { Side::below, align }; }

        /** Returns a placement above the target. */
        static Placement above (Justification align = Justification::topLeft) { return { Side::above, align }; }

        /** Returns a placement to the right of the target. */
        static Placement toRight (Justification align = Justification::topLeft) { return { Side::toRight, align }; }

        /** Returns a placement to the left of the target. */
        static Placement toLeft (Justification align = Justification::topLeft) { return { Side::toLeft, align }; }

        /** Returns a centered placement. */
        static Placement centered() { return { Side::centered, Justification::center }; }
    };

    /** Positioning mode for the menu. */
    enum class PositioningMode
    {
        atPoint,
        relativeToArea,
        relativeToComponent
    };

    //==============================================================================
    /** Options for showing the popup menu. */
    struct Options
    {
        Options();

        /** Sets the parent component. When set, menu appears as child using local coordinates.
            When not set, menu appears as desktop window using screen coordinates. */
        Options& withParentComponent (Component* parentComponent);

        /** Position menu at a specific point.
            - With parent: point is relative to parent component
            - Without parent: point is in screen coordinates

            @param position     The point to show the menu at
            @param alignment    How to align the menu relative to the point (default: top-left of menu at point)
        */
        Options& withPosition (Point<int> position, Justification alignment = Justification::topLeft);
        Options& withPosition (Point<float> position, Justification alignment = Justification::topLeft);

        /** Position menu relative to a rectangle (like a button).
            - With parent: rectangle is relative to parent component
            - Without parent: rectangle is in screen coordinates

            @param area         The rectangle to position relative to
            @param placement    Where to place menu relative to rectangle (default: below the rectangle)
        */
        Options& withTargetArea (Rectangle<int> area, Placement placement = Placement::below());
        Options& withTargetArea (Rectangle<float> area, Placement placement = Placement::below());

        /** Position menu relative to a component (uses the component's bounds).
            The component must be a child of the parent component (if parent is set).

            @param component    The component to position relative to
            @param placement    Where to place menu relative to component (default: below)
        */
        Options& withRelativePosition (Component* component, Placement placement = Placement::below());

        /** Minimum width for the menu. */
        Options& withMinimumWidth (int minWidth);

        /** Maximum width for the menu. */
        Options& withMaximumWidth (int maxWidth);

        Component* parentComponent;
        Component* targetComponent;
        Point<int> targetPosition;
        Rectangle<int> targetArea;
        Justification alignment;
        Placement placement;
        PositioningMode positioningMode;
        std::optional<int> minWidth;
        std::optional<int> maxWidth;
        bool dismissOnSelection;
        bool dismissAllPopups;
    };

    //==============================================================================
    /** Destructor. */
    ~PopupMenu();

    //==============================================================================
    /** Creates a popup menu with the given options.

        @param options The options for the popup menu.

        @return A pointer to the popup menu.
    */
    [[nodiscard]] static Ptr create (const Options& options = {});

    //==============================================================================
    /** Returns the options for the popup menu. */
    [[nodiscard]] const Options& getOptions() const { return options; }

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

    //==============================================================================
    /** Returns the number of items in the menu. */
    [[nodiscard]] int getNumItems() const;

    /** Returns true if the menu is empty. */
    [[nodiscard]] bool isEmpty() const { return getNumItems() == 0; }

    /** Clears all items from the menu. */
    void clear();

    //==============================================================================
    /** Represents a menu item. */
    class Item
    {
    public:
        /** Constructors. */
        Item() = default;
        Item (const String& itemText, int itemID, bool isEnabled = true, bool isTicked = false);
        Item (const String& itemText, PopupMenu::Ptr subMenu, bool isEnabled = true);
        Item (std::unique_ptr<Component> component, int itemID);

        /** Destructor. */
        ~Item();

        /** Returns true if the item is a separator. */
        bool isSeparator() const;

        /** Returns true if the item is a sub-menu. */
        bool isSubMenu() const;

        /** Returns true if the item is a custom component. */
        bool isCustomComponent() const;

        String text;
        int itemID = 0;
        bool isEnabled = true;
        bool isTicked = false;
        bool isHovered = false;
        PopupMenu::Ptr subMenu;
        std::unique_ptr<Component> customComponent;
        String shortcutKeyText;
        std::optional<Color> textColor;
        Rectangle<float> area;

    private:
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Item)
    };

    /** Returns an iterator to the first item in the menu. */
    [[nodiscard]] auto begin() const { return items.begin(); }

    /** Returns an iterator to the end of the menu. */
    [[nodiscard]] auto end() const { return items.end(); }

    //==============================================================================
    /** Shows the menu asynchronously and calls the callback when an item is selected.

        @param options      Options for showing the menu
        @param callback     Function to call when an item is selected (optional)
    */
    void show (std::function<void (int)> callback = nullptr);

    /** Dismiss popup if visible. */
    void dismiss();

    /** Returns true if the menu is currently being shown. */
    bool isBeingShown() const;

    /** Returns true if a submenu is currently visible. */
    bool hasVisibleSubmenu() const;

    //==============================================================================
    /** Callback type for menu item selection. */
    std::function<void (int selectedItemID)> onItemSelected;

    //==============================================================================
    // Color identifiers for theming
    struct Style
    {
        static inline const Identifier menuBackground { "menuBackground" };
        static inline const Identifier menuBorder { "menuBorder" };
        static inline const Identifier menuItemText { "menuItemText" };
        static inline const Identifier menuItemTextDisabled { "menuItemTextDisabled" };
        static inline const Identifier menuItemBackground { "menuItemBackground" };
        static inline const Identifier menuItemBackgroundHighlighted { "menuItemBackgroundHighlighted" };
        static inline const Identifier menuItemBackgroundActiveSubmenu { "menuItemBackgroundActiveSubmenu" };
    };

    //==============================================================================
    /** Returns true if the submenu contains the given position.

        @param position The position to check

        @return True if the submenu contains the position
    */
    bool submenuContains (const Point<float>& position) const;

    /** Returns true if the item at the given index is showing a submenu.

        @param itemIndex The index of the item to check

        @return True if the item is showing a submenu
    */
    bool isItemShowingSubmenu (int itemIndex) const;

    //==============================================================================
    /** Returns true if the menu needs scrolling. */
    bool needsScrolling() const;

    /** Returns true if the menu can scroll up. */
    bool canScrollUp() const;

    /** Returns true if the menu can scroll down. */
    bool canScrollDown() const;

    /** Returns the bounds of the scroll up indicator. */
    Rectangle<float> getScrollUpIndicatorBounds() const;

    /** Returns the bounds of the scroll down indicator. */
    Rectangle<float> getScrollDownIndicatorBounds() const;

    //==============================================================================
    /** Dismisses all currently open popup menus. */
    static void dismissAllPopups();

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheel) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;

private:
    PopupMenu (const Options& options = {});

    void showCustom (const Options& options, bool isSubmenu, std::function<void (int)> callback);
    int getItemIndexAt (Point<float> position) const;
    void dismiss (int itemID);
    void setSelectedItemIndex (int index, bool fromMouse);
    void setupMenuItems();
    void positionMenu();
    void resetInternalState();

    // Submenu functionality
    void showSubmenu (int itemIndex);
    void hideSubmenus();
    void updateSubmenuVisibility (int hoveredItemIndex);
    void cleanupSubmenu (PopupMenu::Ptr submenu);

    // Submenu positioning helpers
    Options prepareSubmenuOptions (PopupMenu::Ptr submenu);
    Placement calculateSubmenuPlacement (Rectangle<float> itemBounds, const Options& submenuOptions);
    void applySubmenuPlacement (Options& submenuOptions, Rectangle<float> itemBounds, Placement placement);

    // Submenu display helpers
    bool canShowSubmenu (int itemIndex) const;
    bool isAlreadyShowingSubmenu (int itemIndex, const Item& item) const;
    void positionSubmenu (Options& submenuOptions);

    // Scrolling functionality
    void updateScrolling();
    void calculateAvailableHeight();
    void layoutVisibleItems (float width);
    Rectangle<float> getMenuContentBounds() const;
    void updateVisibleItemRange();
    void scrollUp();
    void scrollDown();
    int getVisibleItemCount() const;

    // Keyboard navigation
    void navigateUp();
    void navigateDown();
    void navigateLeft();
    void navigateRight();
    void selectCurrentItem();
    int getSelectedItemIndex() const;
    int getFirstSelectableItemIndex() const;
    int getLastSelectableItemIndex() const;
    int getNextSelectableItemIndex (int currentIndex, bool forward) const;
    int getNextSelectableItemIndex (int currentIndex) const;
    int getPreviousSelectableItemIndex (int currentIndex) const;
    bool isItemSelectable (int index) const;
    void enterSubmenuViaKeyboard (int itemIndex);

    // PopupMenuItem is now an implementation detail
    class PopupMenuItem;
    std::vector<std::unique_ptr<Item>> items;

    Options options;
    int selectedItemIndex = -1; // For keyboard navigation
    bool isBeingDismissed = false;

    std::function<void (int)> menuCallback;

    // Submenu support
    WeakReference<Component> parentMenu;
    PopupMenu::Ptr currentSubmenu;
    int submenuItemIndex = -1;
    bool isShowingSubmenu = false;

    // Scrolling support
    Range<int> visibleItemRange;
    float availableContentHeight = 0.0f;
    float totalContentHeight = 0.0f;
    bool showScrollIndicators = false;

    static constexpr float scrollIndicatorHeight = 12.0f;
    static constexpr int scrollSpeed = 1; // Number of items to scroll per wheel event

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupMenu)
};

} // namespace yup
