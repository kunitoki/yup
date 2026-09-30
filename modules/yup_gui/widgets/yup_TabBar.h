/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

//==============================================================================
/** A strip of tabs, one of which is selected.

    A TabBar works on its own as a segmented control, or as the strip of a TabComponent. Tabs are
    addressed by stable identifiers rather than by index, so they keep their identity when they are
    moved, added or removed.

    The selection indicator slides between tabs, and when reordering is enabled the neighbours of a
    dragged tab slide aside. Tabs that do not fit are handled by the Overflow policy, and the arrow
    keys, Home and End move the selection when the bar has the keyboard focus.

    @code
    tabBar.setLayout (TabBar::Layout::fill);
    tabBar.addTab ("12h", "12-hour");
    tabBar.addTab ("24h", "24-hour");
    tabBar.onSelectionChanged = [] (const Identifier& tabId) { DBG (tabId); };
    @endcode

    @see TabButton, TabComponent
*/
class YUP_API TabBar : public Component
{
public:
    //==============================================================================
    /** The direction the tabs are laid out in. */
    enum class Orientation
    {
        horizontal, /**< Tabs go from left to right. */
        vertical    /**< Tabs go from top to bottom. */
    };

    /** How the selected tab is marked. */
    enum class Variant
    {
        pill,     /**< A raised pill inside a rounded track, like a segmented control. */
        underline /**< An accent bar on the edge facing the content. */
    };

    /** How the length of the bar is shared between tabs. */
    enum class Layout
    {
        natural, /**< Each tab is as long as its content. */
        fill     /**< The tabs share the whole length of the bar equally. */
    };

    /** What happens to tabs that do not fit in the bar. */
    enum class Overflow
    {
        menu,   /**< Trailing tabs are hidden behind a button that lists them in a menu. */
        scroll, /**< The tabs scroll with the mouse wheel, and the selected tab scrolls into view. */
        shrink  /**< The tabs shrink down to a minimum length, and their text is shortened. */
    };

    //==============================================================================
    /** Receives changes to the tabs of a bar.

        Listeners are always told about changes, synchronously, whatever NotificationType was used
        to make them: the notification type only applies to the std::function callbacks.
    */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when the selected tab changes.

            @param bar    The bar.
            @param tabId  The newly selected tab, or a null Identifier when there is no selection.
        */
        virtual void tabSelectionChanged (TabBar& bar, const Identifier& tabId) { ignoreUnused (bar, tabId); }

        /** Called after a tab has been removed.

            @param bar    The bar.
            @param tabId  The removed tab.
        */
        virtual void tabRemoved (TabBar& bar, const Identifier& tabId) { ignoreUnused (bar, tabId); }

        /** Called after a tab has moved.

            @param bar       The bar.
            @param tabId     The moved tab.
            @param oldIndex  Where the tab was.
            @param newIndex  Where the tab is now.
        */
        virtual void tabMoved (TabBar& bar, const Identifier& tabId, int oldIndex, int newIndex) { ignoreUnused (bar, tabId, oldIndex, newIndex); }
    };

    //==============================================================================
    /** Creates an empty bar. */
    TabBar (StringRef componentID = {});

    /** Destructor. */
    ~TabBar() override;

    //==============================================================================
    /** Adds a tab.

        The first tab added to an empty bar becomes the selected tab.

        @param tabId        A unique identifier for the tab. Adding an identifier that is already
                            used asserts and returns the existing tab.
        @param text         The text shown by the tab.
        @param insertIndex  Where to insert the tab, or -1 to add it at the end.

        @returns The new tab, to configure its content.
    */
    TabButton& addTab (const Identifier& tabId, const String& text, int insertIndex = -1);

    /** Removes a tab.

        When the selected tab is removed, the tab that takes its place becomes selected, or the
        previous one if it was the last tab.

        @param tabId  The tab to remove.
    */
    void removeTab (const Identifier& tabId);

    /** Removes all tabs, leaving the bar without a selection. */
    void clearTabs();

    /** Moves a tab to a new position.

        @param tabId         The tab to move.
        @param newIndex      The new position, clamped to the valid range.
        @param notification  Whether to call onTabMoved. Listeners are always told.
    */
    void moveTab (const Identifier& tabId, int newIndex, NotificationType notification = sendNotification);

    /** Returns the number of tabs. */
    int getNumTabs() const noexcept;

    /** Returns the identifier of the tab at an index, or a null Identifier if the index is out of range. */
    Identifier getTabId (int index) const;

    /** Returns the index of a tab, or -1 if there is no such tab. */
    int indexOfTab (const Identifier& tabId) const;

    /** Returns a tab, or nullptr if there is no such tab. */
    TabButton* getTabButton (const Identifier& tabId) const;

    //==============================================================================
    /** Selects a tab.

        @param tabId         The tab to select, or a null Identifier to clear the selection.
        @param notification  Whether to call onSelectionChanged. Listeners are always told.
    */
    void setSelectedTab (const Identifier& tabId, NotificationType notification = sendNotification);

    /** Returns the selected tab, or a null Identifier when there is none. */
    Identifier getSelectedTabId() const;

    /** Returns the index of the selected tab, or -1 when there is none. */
    int getSelectedTabIndex() const;

    //==============================================================================
    /** Sets the direction of the tabs. The default is horizontal. */
    void setOrientation (Orientation newOrientation);

    /** Returns the direction of the tabs. */
    Orientation getOrientation() const noexcept { return orientation; }

    /** Sets how the selected tab is marked. The default is pill. */
    void setVariant (Variant newVariant);

    /** Returns how the selected tab is marked. */
    Variant getVariant() const noexcept { return variant; }

    /** Sets how the length of the bar is shared between tabs. The default is natural. */
    void setLayout (Layout newLayout);

    /** Returns how the length of the bar is shared between tabs. */
    Layout getLayout() const noexcept { return layout; }

    /** Sets what happens to tabs that do not fit. The default is menu. */
    void setOverflow (Overflow newOverflow);

    /** Returns what happens to tabs that do not fit. */
    Overflow getOverflow() const noexcept { return overflow; }

    /** Puts the underline on the other edge of the bar.

        The underline normally sits on the bottom edge of a horizontal bar and on the right edge
        of a vertical one. A TabComponent flips it when the bar sits below or right of the content.

        @param shouldBeFlipped  True to use the top or left edge.
    */
    void setFlipped (bool shouldBeFlipped);

    /** Returns true if the underline sits on the top or left edge. */
    bool isFlipped() const noexcept { return flipped; }

    /** Lets the user drag tabs to reorder them. Off by default.

        @param shouldBeReorderable  True to allow dragging.
    */
    void setReorderable (bool shouldBeReorderable);

    /** Returns true if the user can drag tabs to reorder them. */
    bool isReorderable() const noexcept { return reorderable; }

    /** Sets the text of the overflow button. The default is "More".

        @param newText  The text shown when the selected tab is not hidden behind the button.
    */
    void setOverflowText (const String& newText);

    /** Returns the text of the overflow button. */
    const String& getOverflowText() const noexcept { return overflowText; }

    /** Sets how long tabs and the indicator take to slide to a new position.

        @param newDurationSeconds  The duration in seconds, or 0 to move them at once. The default is 0.18.
    */
    void setAnimationDuration (double newDurationSeconds);

    /** Returns how long tabs and the indicator take to slide to a new position, in seconds. */
    double getAnimationDuration() const noexcept { return animationDuration; }

    //==============================================================================
    /** Returns the button standing for the tabs hidden by the menu overflow.

        It is only visible while some tabs do not fit. When the selected tab is hidden, the button
        shows its icon and text and reports itself as selected.
    */
    TabButton& getOverflowButton() const noexcept { return *overflowTabButton; }

    /** Returns how far the tabs are scrolled along the bar, with the scroll overflow. */
    float getScrollOffset() const noexcept { return scrollOffset; }

    /** Returns the current bounds of the selection indicator, or an empty rectangle when there is no selection.

        For the pill variant this is the pill, for the underline variant the accent bar.
    */
    Rectangle<float> getIndicatorBounds() const;

    /** Returns the area the tabs are laid out in, inside the track. */
    Rectangle<float> getTabArea() const;

    /** Returns true if the bar has the keyboard focus and was last used from the keyboard. */
    bool isFocusIndicatorVisible() const;

    /** The shortest a tab gets with the fill layout or the shrink overflow, unless its content is shorter. */
    static constexpr float minimumTabLength = 48.0f;

    //==============================================================================
    /** Adds a listener. */
    void addListener (Listener* listener);

    /** Removes a listener. */
    void removeListener (Listener* listener);

    //==============================================================================
    /** Called with the newly selected tab, or a null Identifier when the selection is cleared. */
    std::function<void (const Identifier&)> onSelectionChanged;

    /** Called when a tab has moved, after the user dropped a dragged tab or after moveTab(). */
    std::function<void (const Identifier&, int oldIndex, int newIndex)> onTabMoved;

    /** Called when the close button of a tab is clicked. When not set, the tab is removed. */
    std::function<void (const Identifier&)> onTabCloseRequested;

    //==============================================================================
    /** Color identifiers used by the bar. */
    struct Style
    {
        static inline const Identifier trackColorId { "tabBarTrack" };
        static inline const Identifier indicatorColorId { "tabBarIndicator" };
        static inline const Identifier underlineColorId { "tabBarUnderline" };
        static inline const Identifier focusOutlineColorId { "tabBarFocusOutline" };
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void focusLost() override;

private:
    friend class TabButton;

    struct Motion
    {
        Rectangle<float> from;
        Rectangle<float> to;
        double progress = 1.0;

        Rectangle<float> getCurrent() const;
        void retarget (Rectangle<float> target, bool animate);
    };

    struct Tab
    {
        std::unique_ptr<TabButton> button;
        Rectangle<float> target;
        Motion motion;
    };

    struct Press
    {
        TabButton* button = nullptr;
        Point<float> downPosition;
        float grabOffset = 0.0f;
        int originalIndex = -1;
        bool onCloseButton = false;
        bool dragging = false;
    };

    void buttonMouseDown (TabButton& button, const MouseEvent& event);
    void buttonMouseDrag (TabButton& button, const MouseEvent& event);
    void buttonMouseUp (TabButton& button, const MouseEvent& event);

    void updateLayout (bool animate, bool revealSelection);
    void updateOverflowButtonContent (bool selectedIsHidden);
    void moveButton (TabButton& button, Motion& motion, Rectangle<float> target, bool animate);
    void updateDraggedTab (float pointer);
    bool isDragging (const TabButton& button) const noexcept;
    void notifySelectionChanged (NotificationType notification);
    void notifyTabMoved (Identifier tabId, int oldIndex, int newIndex, NotificationType notification);
    void requestClose (Identifier tabId);
    void showOverflowMenu();
    int findEnabledTab (int fromIndex, int step) const;
    bool isVertical() const noexcept { return orientation == Orientation::vertical; }

    std::vector<Tab> tabs;
    std::unique_ptr<TabButton> overflowTabButton;
    Motion overflowMotion;
    Motion indicatorMotion;
    bool hasIndicator = false;
    Identifier selectedTabId;
    std::optional<Press> press;

    Orientation orientation = Orientation::horizontal;
    Variant variant = Variant::pill;
    Layout layout = Layout::natural;
    Overflow overflow = Overflow::menu;
    bool flipped = false;
    bool reorderable = false;
    String overflowText { "More" };
    double animationDuration = 0.18;
    float scrollOffset = 0.0f;
    bool keyboardNavigated = false;

    PopupMenu::Ptr overflowMenu;
    bool dismissingMenuFromMouseDown = false;
    bool ignoreMouseDownAfterMenuDismissal = false;

    ListenerList<Listener> listeners;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabBar)
};

} // namespace yup
