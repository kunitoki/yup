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
    enum class Placement
    {
        above,          //< Menu appears above the target
        below,          //< Menu appears below the target (default)
        toLeft,         //< Menu appears to the left of the target
        toRight,        //< Menu appears to the right of the target
        centered        //< Menu is centered on the target
    };

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
        Options& withTargetArea (Rectangle<int> area, Placement placement = Placement::below);
        Options& withTargetArea (Rectangle<float> area, Placement placement = Placement::below);

        /** Position menu relative to a component (uses the component's bounds).
            The component must be a child of the parent component (if parent is set).

            @param component    The component to position relative to
            @param placement    Where to place menu relative to component (default: below)
        */
        Options& withRelativePosition (Component* component, Placement placement = Placement::below);

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
    class Item
    {
    public:
        Item() = default;
        Item (const String& itemText, int itemID, bool isEnabled = true, bool isTicked = false);
        Item (const String& itemText, PopupMenu::Ptr subMenu, bool isEnabled = true);
        Item (std::unique_ptr<Component> component, int itemID);
        ~Item();

        bool isSeparator() const;
        bool isSubMenu() const;
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

    //==============================================================================
    /** Dismiss popup if visible. */
    void dismiss();

    //==============================================================================
    /** Callback type for menu item selection. */
    std::function<void (int selectedItemID)> onItemSelected;

    //==============================================================================
    // Color identifiers for theming
    struct Colors
    {
        static inline const Identifier menuBackground { "menuBackground" };
        static inline const Identifier menuBorder { "menuBorder" };
        static inline const Identifier menuItemText { "menuItemText" };
        static inline const Identifier menuItemTextDisabled { "menuItemTextDisabled" };
        static inline const Identifier menuItemBackground { "menuItemBackground" };
        static inline const Identifier menuItemBackgroundHighlighted { "menuItemBackgroundHighlighted" };
    };

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
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusLost() override;

private:
    PopupMenu (const Options& options = {});
    void showCustom (const Options& options, std::function<void (int)> callback);

    int getHoveredItem() const;
    void setHoveredItem (int itemIndex);

    int getItemIndexAt (Point<float> position) const;

    void dismiss (int itemID);
    void setSelectedItemID (int itemID);

    void setupMenuItems();
    void positionMenu();

    // PopupMenuItem is now an implementation detail
    class PopupMenuItem;
    std::vector<std::unique_ptr<Item>> items;

    Options options;
    int selectedItemID = -1;
    bool isBeingDismissed = false;

    std::function<void (int)> menuCallback;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupMenu)
};

} // namespace yup
