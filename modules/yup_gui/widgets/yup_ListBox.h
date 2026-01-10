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

namespace yup
{

class ListBox;

//==============================================================================
/**
    Abstract base class for providing data to a ListBox component.

    A ListBox uses a ListBoxModel to determine the number of rows to display,
    create the components for each row, and handle selection and interaction events.

    The model is not owned by the ListBox, so you must ensure it remains valid
    for the lifetime of the ListBox that uses it.

    @see ListBox, ListBoxItem
*/
class YUP_API ListBoxModel
{
public:
    /** Destructor. */
    virtual ~ListBoxModel() = default;

    //==============================================================================
    /** Returns the number of rows currently in the list.

        This is called frequently by the ListBox to determine how many rows to display.

        @return The number of rows in the list
    */
    virtual int getNumRows() = 0;

    //==============================================================================
    /** Returns the height of a specific row.

        This is only called if the ListBox has variable height mode enabled.
        Return 0 to use the fixed row height set on the ListBox.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The height of the row in pixels, or 0 to use fixed height
    */
    virtual int getRowHeight (int rowIndex);

    /** Returns the width of a specific row.

        This is only called if the ListBox has variable width mode enabled
        and is using horizontal orientation.
        Return 0 to use the fixed row width set on the ListBox.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The width of the row in pixels, or 0 to use fixed width
    */
    virtual int getRowWidth (int rowIndex);

    //==============================================================================
    /** Creates or updates the component for a row.

        This method is called when the ListBox needs to display a row. You can either:
        1. Return a new Component that will be used to display the row
        2. Update and return the existingComponent if it's reusable
        3. Return nullptr to use the default list item renderer

        If you return a Component, the ListBox takes ownership of it and will delete it
        when it's no longer needed.

        If you return nullptr, the ListBox will create a ListBoxItem and use
        the paintListBoxItem(), getRowText(), and getRowIcon() methods to render it.

        @param rowIndex          The index of the row (0 to getNumRows()-1)
        @param existingComponent An existing component that might be reusable, or nullptr
        @return A component to display the row, or nullptr to use default rendering
    */
    virtual Component* refreshComponentForRow (int rowIndex, Component* existingComponent);

    //==============================================================================
    /** Paints the content of a list item when using default rendering.

        This is only called when refreshComponentForRow() returns nullptr and the
        ListBox creates a ListBoxItem for rendering.

        You can override this to provide custom painting, or use getRowText() and
        getRowIcon() for simple text+icon rendering.

        @param rowIndex   The index of the row being painted
        @param g          The graphics context to paint with
        @param area       The area to paint within
        @param isSelected Whether this row is currently selected
    */
    virtual void paintListBoxItem (int rowIndex, Graphics& g, Rectangle<float> area, bool isSelected);

    /** Returns the text to display for a row when using default rendering.

        This is only called when refreshComponentForRow() returns nullptr.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The text to display for this row
    */
    virtual String getRowText (int rowIndex);

    /** Returns the icon to display for a row when using default rendering.

        This is only called when refreshComponentForRow() returns nullptr.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The icon image to display, or an invalid Image to display no icon
    */
    virtual Image getRowIcon (int rowIndex);

    //==============================================================================
    /** Called when the selected rows change.

        This is called after the selection has changed, either through user interaction
        or programmatically.

        @param selectedRows  An array of currently selected row indices (sorted ascending)
    */
    virtual void selectedRowsChanged (const Array<int>& selectedRows);

    /** Called when a row is clicked.

        @param rowIndex  The index of the clicked row
        @param event     The mouse event that triggered the click
    */
    virtual void rowClicked (int rowIndex, const MouseEvent& event);

    /** Called when a row is double-clicked.

        @param rowIndex  The index of the double-clicked row
        @param event     The mouse event that triggered the double-click
    */
    virtual void rowDoubleClicked (int rowIndex, const MouseEvent& event);

    //==============================================================================
    /** Called when the return key is pressed.

        @param lastSelectedRow  The last row that was selected, or -1 if none
    */
    virtual void returnKeyPressed (int lastSelectedRow);

    /** Called when the delete or backspace key is pressed.

        @param selectedRows  An array of currently selected row indices
    */
    virtual void deleteKeyPressed (const Array<int>& selectedRows);

    //==============================================================================
    /** Returns a description for drag-and-drop operations.

        Override this to support drag-and-drop. Return a var containing information
        about the dragged rows.

        @param selectedRows  An array of the selected row indices being dragged
        @return A var describing the drag source, or an empty var for no drag support
    */
    virtual var getDragSourceDescription (const Array<int>& selectedRows);

protected:
    /** Constructor. */
    ListBoxModel() = default;

private:
    YUP_DECLARE_NON_COPYABLE (ListBoxModel)
};

//==============================================================================
/**
    A component that displays a scrollable list of items.

    The ListBox component displays a list of items that can be laid out vertically
    or horizontally. It supports variable item sizes, single and multiple selection,
    scrolling, and keyboard navigation.

    The ListBox uses a ListBoxModel to provide the data and create components for
    each item. The model is not owned by the ListBox, so you must ensure it remains
    valid for the lifetime of the ListBox.

    @code
    class MyListBoxModel : public ListBoxModel
    {
    public:
        int getNumRows() override { return 100; }

        String getRowText (int rowIndex) override
        {
            return "Item " + String (rowIndex);
        }
    };

    MyListBoxModel model;
    ListBox listBox;
    listBox.setModel (&model);
    listBox.setBounds (0, 0, 300, 400);
    @endcode

    @see ListBoxModel, ListBoxItem
*/
class YUP_API ListBox : public Component
{
public:
    //==============================================================================
    /** Defines the layout orientation of the list. */
    enum class Orientation
    {
        vertical,  /**< Items are laid out vertically (top to bottom). */
        horizontal /**< Items are laid out horizontally (left to right). */
    };

    /** Defines the selection behavior of the list. */
    enum class SelectionMode
    {
        none,    /**< No selection is allowed. */
        single,  /**< Only one item can be selected at a time. */
        multiple /**< Multiple items can be selected. */
    };

    //==============================================================================
    /** Creates a ListBox.

        @param componentID    Optional component identifier
        @param orientation    The layout orientation (default is vertical)
    */
    ListBox (StringRef componentID = {}, Orientation orientation = Orientation::vertical);

    /** Destructor. */
    ~ListBox() override;

    //==============================================================================
    /** Sets the model that provides the list data.

        The model is not owned by the ListBox. You must ensure it remains valid
        for the lifetime of the ListBox.

        Setting a new model will rebuild the list content.

        @param newModel  The model to use, or nullptr to clear
    */
    void setModel (ListBoxModel* newModel);

    /** Returns the current model.

        @return The current model, or nullptr if none is set
    */
    ListBoxModel* getModel() const noexcept;

    //==============================================================================
    /** Sets the selection mode.

        @param mode  The selection mode to use
    */
    void setSelectionMode (SelectionMode mode);

    /** Returns the current selection mode.

        @return The selection mode
    */
    SelectionMode getSelectionMode() const noexcept;

    //==============================================================================
    /** Returns the index of the currently selected row.

        If multiple rows are selected or no rows are selected, this returns -1.

        @return The selected row index, or -1
    */
    int getSelectedRow() const;

    /** Selects a single row.

        In single selection mode, this will deselect any other rows.
        In multiple selection mode, this will add to the current selection
        unless replace is implied by the notification type.

        @param rowIndex           The index of the row to select
        @param scrollToShowRow    Whether to scroll to make the row visible
        @param notification       Whether to send change notifications
    */
    void selectRow (int rowIndex, bool scrollToShowRow = true, NotificationType notification = sendNotification);

    /** Deselects a specific row.

        @param rowIndex      The index of the row to deselect
        @param notification  Whether to send change notifications
    */
    void deselectRow (int rowIndex, NotificationType notification = sendNotification);

    /** Deselects all rows.

        @param notification  Whether to send change notifications
    */
    void deselectAllRows (NotificationType notification = sendNotification);

    /** Returns an array of all currently selected row indices.

        The array is sorted in ascending order.

        @return Array of selected row indices
    */
    Array<int> getSelectedRows() const;

    /** Sets the selected rows.

        This replaces the current selection with the specified rows.
        The array will be sorted internally.

        @param rows          The row indices to select
        @param notification  Whether to send change notifications
    */
    void setSelectedRows (const Array<int>& rows, NotificationType notification = sendNotification);

    /** Returns whether a specific row is selected.

        @param rowIndex  The index of the row to check
        @return True if the row is selected
    */
    bool isRowSelected (int rowIndex) const;

    /** Returns the number of currently selected rows.

        @return The number of selected rows
    */
    int getNumSelectedRows() const;

    //==============================================================================
    /** Rebuilds the list content from the model.

        This clears the component cache and recreates all visible items.
        Call this when the model's data has changed significantly.
    */
    void updateContent();

    /** Repaints a specific row if it's currently visible.

        @param rowIndex  The index of the row to repaint
    */
    void repaintRow (int rowIndex);

    /** Scrolls to ensure a specific row is visible.

        @param rowIndex  The index of the row to show
    */
    void scrollToEnsureRowIsVisible (int rowIndex);

    //==============================================================================
    /** Sets the layout orientation.

        @param newOrientation  The orientation to use
    */
    void setOrientation (Orientation newOrientation);

    /** Returns the current layout orientation.

        @return The orientation
    */
    Orientation getOrientation() const noexcept;

    //==============================================================================
    /** Sets the fixed height for all rows (used in vertical orientation).

        This is only used when variable height mode is disabled.

        @param newHeight  The height in pixels
    */
    void setRowHeight (int newHeight);

    /** Sets the fixed width for all rows (used in horizontal orientation).

        This is only used when variable width mode is disabled.

        @param newWidth  The width in pixels
    */
    void setRowWidth (int newWidth);

    /** Returns the fixed row height.

        @return The row height in pixels
    */
    int getRowHeight() const noexcept;

    /** Returns the fixed row width.

        @return The row width in pixels
    */
    int getRowWidth() const noexcept;

    //==============================================================================
    /** Enables or disables variable row heights.

        When enabled, the model's getRowHeight() method will be called for each row.
        When disabled, all rows use the fixed row height.

        @param enabled  True to enable variable heights
    */
    void setVariableHeightEnabled (bool enabled);

    /** Enables or disables variable row widths.

        When enabled, the model's getRowWidth() method will be called for each row.
        When disabled, all rows use the fixed row width.

        @param enabled  True to enable variable widths
    */
    void setVariableWidthEnabled (bool enabled);

    /** Returns whether variable height mode is enabled.

        @return True if variable heights are enabled
    */
    bool isVariableHeightEnabled() const noexcept;

    /** Returns whether variable width mode is enabled.

        @return True if variable widths are enabled
    */
    bool isVariableWidthEnabled() const noexcept;

    //==============================================================================
    /** Sets the minimum content size to keep visible.

        @param minSize  The minimum size in pixels
    */
    void setMinimumContentSize (int minSize);

    /** Returns the minimum content size.

        @return The minimum size in pixels
    */
    int getMinimumContentSize() const noexcept;

    //==============================================================================
    /** Sets the scrollbar visibility mode for the vertical scrollbar.

        @param mode  The visibility mode (alwaysVisible, autoHide, alwaysHidden)
    */
    void setVerticalScrollBarVisibility (ScrollBar::VisibilityMode mode);

    /** Sets the scrollbar visibility mode for the horizontal scrollbar.

        @param mode  The visibility mode (alwaysVisible, autoHide, alwaysHidden)
    */
    void setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode mode);

    /** Returns the vertical scrollbar.

        @return Pointer to the vertical scrollbar
    */
    ScrollBar* getVerticalScrollBar() const noexcept;

    /** Returns the horizontal scrollbar.

        @return Pointer to the horizontal scrollbar
    */
    ScrollBar* getHorizontalScrollBar() const noexcept;

    //==============================================================================
    /** Returns the number of rows that are currently visible.

        @return The number of visible rows
    */
    int getVisibleRowsCount() const;

    /** Returns the range of row indices that are currently visible.

        @return The visible row range
    */
    Range<int> getVisibleRowRange() const;

    //==============================================================================
    /** Returns the row index at a specific position.

        @param position  The position to check (in local coordinates)
        @return The row index, or -1 if no row at that position
    */
    int getRowAt (Point<float> position) const;

    /** Returns the component being used to display a specific row.

        @param rowIndex  The row index
        @return The component, or nullptr if the row is not currently visible
    */
    Component* getComponentForRow (int rowIndex) const;

    /** Returns the bounds of a specific row.

        @param rowIndex  The row index
        @return The row bounds in local coordinates
    */
    Rectangle<float> getRowBounds (int rowIndex) const;

    //==============================================================================
    /** Callback called when a row is clicked.

        @param rowIndex  The index of the clicked row
    */
    std::function<void (int rowIndex)> onRowClicked;

    /** Callback called when a row is double-clicked.

        @param rowIndex  The index of the double-clicked row
    */
    std::function<void (int rowIndex)> onRowDoubleClicked;

    /** Callback called when the selection changes. */
    std::function<void()> onSelectionChanged;

    //==============================================================================
    /** Style identifiers for theming. */
    struct Style
    {
        static inline const Identifier backgroundColorId { "listBoxBackground" };
        static inline const Identifier outlineColorId { "listBoxOutline" };
        static inline const Identifier rowBackgroundColorId { "rowBackground" };
        static inline const Identifier selectedRowBackgroundColorId { "selectedRowBackground" };
        static inline const Identifier hoveredRowBackgroundColorId { "hoveredRowBackground" };
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;
    /** @internal */
    void mouseDoubleClick (const MouseEvent& event) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;

private:
    //==============================================================================
    class ListBoxRow;

    //==============================================================================
    void updateVisibleRows();
    void layoutRows();
    void createOrUpdateRow (int rowIndex);
    void removeUnusedRows();

    void scrollBy (float delta);
    void scrollToRow (int rowIndex);
    void updateScrolling();
    float getRowPosition (int rowIndex) const;
    float getRowSize (int rowIndex) const;

    void handleRowClick (int rowIndex, const MouseEvent& event);
    void handleRowSelection (int rowIndex, bool shouldToggle, bool shouldExtend);
    void notifySelectionChanged();

    int getRowIndexAt (Point<float> position) const;
    void updateHoveredRow (Point<float> position);

    bool needsScrolling() const;
    float getTotalContentSize() const;
    float getViewportSize() const;

    void updateScrollBars();
    void handleScrollBarMoved();
    Rectangle<float> getContentArea() const;

    //==============================================================================
    ListBoxModel* model = nullptr;
    Orientation orientation = Orientation::vertical;
    SelectionMode selectionMode = SelectionMode::single;

    Array<int> selectedRows;
    int lastSelectedRow = -1;
    int hoveredRow = -1;

    int fixedRowHeight = 24;
    int fixedRowWidth = 100;
    bool variableHeightEnabled = false;
    bool variableWidthEnabled = false;
    int minimumContentSize = 0;

    Range<int> visibleRowRange { 0, 0 };
    float scrollOffset = 0.0f;
    float totalContentSize = 0.0f;
    float viewportSize = 0.0f;

    std::unordered_map<int, std::unique_ptr<ListBoxRow>> rowComponents;
    int maxCachedRows = 50;

    std::unique_ptr<ScrollBar> verticalScrollBar;
    std::unique_ptr<ScrollBar> horizontalScrollBar;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListBox)
};

} // namespace yup
