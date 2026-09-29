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

    The ListBox only reads getNumRows() and getRowSize() when it is told the data changed:
    through ListBox::setModel(), ListBox::updateContent() or one of the granular notifications
    such as ListBox::rowsInserted(). Mutate the data first, then notify.

    @see ListBox, ListBoxItem
*/
class YUP_API ListBoxModel
{
public:
    /** Destructor. */
    virtual ~ListBoxModel() = default;

    //==============================================================================
    /** Returns the number of rows currently in the list.

        Read by the ListBox only when it is notified of a change, see the class description.

        @return The number of rows in the list
    */
    virtual int getNumRows() = 0;

    //==============================================================================
    /** Returns the size of a row along the list's scroll axis.

        That is the height of a row in a vertical list and its width in a horizontal one; the other
        dimension always fills the list. Return 0 or less to use ListBox::getRowSize().

        Read by the ListBox only when it is notified of a change; call ListBox::rowsChanged() when a
        row's size changes.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The size of the row in points, or 0 to use the list's row size
    */
    virtual float getRowSize (int rowIndex);

    //==============================================================================
    /** Creates or updates the component that displays a row.

        Called whenever a row comes into view, when its content or selection state changes, and
        when updateContent() refreshes the visible rows. Row components are recycled: the component
        passed in may have been showing a different row, or may be one this model created for
        another row before, so update everything it shows.

        Leave @a component null to have the list show a built-in ListBoxItem filled from getRowText()
        and getRowIcon(). Otherwise create or update it in place; reuseOrCreate() does the usual
        "reuse it when it is already of my type, create it otherwise" dance:

        @code
        void refreshRowComponent (int row, bool isSelected, std::unique_ptr<Component>& component) override
        {
            auto& card = reuseOrCreate<CardComponent> (component);
            card.setTitle (items[row].title);
        }
        @endcode

        A custom row that should still scroll and select when its empty areas are pressed calls
        setWantsMouseEvents (false, true) on itself, so only its interactive children take presses.

        @param rowIndex    The index of the row (0 to getNumRows()-1)
        @param isSelected  Whether the row is currently selected
        @param component   The row's current component, owned by the list; may be null
    */
    virtual void refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component);

    //==============================================================================
    /** Returns the text of a row shown by the built-in ListBoxItem.

        Only used for rows whose component refreshRowComponent() leaves null.

        @param rowIndex  The index of the row (0 to getNumRows()-1)
        @return The text to display for this row
    */
    virtual String getRowText (int rowIndex);

    /** Returns the icon of a row shown by the built-in ListBoxItem.

        Only used for rows whose component refreshRowComponent() leaves null.

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

    /** Called when a row is clicked or tapped.

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

        @param currentRow  The list's current row, or -1 if there is none
    */
    virtual void returnKeyPressed (int currentRow);

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

    /** Returns the row component as a T, creating one when it is null or of another type.

        Meant for refreshRowComponent(): recycled components are reused as they are, anything else is
        replaced by a new T built from @a args.

        @param component  The row component passed to refreshRowComponent()
        @param args       The constructor arguments used when a new T has to be created
        @return The component, now guaranteed to be a T
    */
    template <class T, class... Args>
    static T& reuseOrCreate (std::unique_ptr<Component>& component, Args&&... args)
    {
        if (auto* existing = dynamic_cast<T*> (component.get()))
            return *existing;

        auto created = std::make_unique<T> (std::forward<Args> (args)...);
        auto& result = *created;
        component = std::move (created);
        return result;
    }

private:
    YUP_DECLARE_NON_COPYABLE (ListBoxModel)
};

//==============================================================================
/**
    A component that displays a scrollable list of rows.

    The rows are laid out along a scroll axis, vertically or horizontally, with sizes that may vary
    per row. The list supports single and multiple selection, a keyboard-driven current row, mouse
    wheel, scrollbar and keyboard scrolling, dragging rows out, a header and a footer, and granular
    change notifications that keep the selection and the scroll position stable while rows are
    inserted, removed or moved.

    Only the visible rows have components, and they are recycled as rows scroll in and out of view.

    The ListBox uses a ListBoxModel to provide the data and the row components. The model is not
    owned by the ListBox, so you must ensure it remains valid for the lifetime of the ListBox.

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
    , public DragAndDropSource
{
public:
    //==============================================================================
    /** Defines the layout orientation of the list. */
    enum class Orientation
    {
        vertical,  /**< Rows are laid out vertically (top to bottom). */
        horizontal /**< Rows are laid out horizontally (left to right). */
    };

    /** Defines the selection behavior of the list. */
    enum class SelectionMode
    {
        none,    /**< No selection is allowed. */
        single,  /**< Only one row can be selected at a time. */
        multiple /**< Multiple rows can be selected. */
    };

    /** Where scrollToRow() places a row within the visible area. */
    enum class ScrollAlignment
    {
        nearest, /**< Scroll as little as possible to show the whole row, or not at all when it is visible. */
        start,   /**< Align the row with the start of the visible area. */
        center,  /**< Center the row in the visible area. */
        end      /**< Align the row with the end of the visible area. */
    };

    /** What the scroll position is doing. */
    enum class ScrollState
    {
        idle,     /**< Not moving. */
        dragging, /**< Following a finger. */
        settling  /**< Moving on its own: a fling, a bounce back or an animated scroll. */
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

        Setting a new model clears the selection and the current row, discards every row component
        and rebuilds the list from the new model.

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

    /** Selects a single row and makes it the current row.

        In single selection mode, this will deselect any other rows.
        In multiple selection mode, this adds the row to the current selection.

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

        This replaces the current selection with the specified rows, and makes the last of them the
        current row. The array will be sorted internally.

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
    /** Sets the current row, the keyboard focus within the list.

        The current row is separate from the selection: arrow keys move it, and in multiple selection
        mode Cmd/Ctrl + arrows move it without touching the selection, Space toggles it and Shift +
        arrows extend the selection to it. Return reports it to ListBoxModel::returnKeyPressed().
        Clicks, taps and selectRow() move it too.

        Setting it does not scroll; call scrollToRow() for that.

        @param rowIndex      The new current row, or -1 for none. Other out of range values are ignored.
        @param notification  Whether to call onCurrentRowChanged
    */
    void setCurrentRow (int rowIndex, NotificationType notification = sendNotification);

    /** Returns the current row, or -1 if there is none. */
    int getCurrentRow() const noexcept;

    /** Called when the current row changes, with the new current row. */
    std::function<void (int rowIndex)> onCurrentRowChanged;

    //==============================================================================
    /** Creates the component shown under the cursor while rows are being dragged.

        The default returns a circle with the number of dragged rows inside it. Override this to
        show something else - typically something that says what is being dragged.

        The returned component must have the size it wants to be shown at, because it becomes the
        whole of the drag image window. The list box owns it for the duration of the drag and
        deletes it when the drag ends, so an override can simply return a freshly created component.
        Returning nullptr means no drag image.

        @param selectedRows  The rows being dragged, sorted ascending
        @return The component to show, or nullptr for none
    */
    virtual std::unique_ptr<Component> createDragSourceComponent (const Array<int>& selectedRows);

    /** @internal Called when a drag this list started has ended, to release the drag image. */
    void dragOperationEnded (const DragAndDropData& data, DragAndDropAction performed) override;

    //==============================================================================
    /** Enables or disables dragging rows out of this list.

        Enabled by default. When disabled, a drag never starts and getDragSourceDescription() is not
        consulted at all, so this is how a particular list is made undraggable independently of what
        its model would otherwise allow.

        @param shouldBeEnabled  Whether rows can be dragged out of the list
    */
    void setDragSourceEnabled (bool shouldBeEnabled);

    /** Returns whether rows can be dragged out of this list.

        @return True if dragging is enabled
    */
    bool isDragSourceEnabled() const noexcept;

    //==============================================================================
    /** Re-reads the whole model and refreshes the visible rows in place.

        Reads the row count and every row size again, drops selected rows that no longer exist and
        calls ListBoxModel::refreshRowComponent() for each visible row with its existing component.
        Prefer the granular notifications below when you know what changed: they keep the selection
        and the scroll position attached to the rows that moved.
    */
    void updateContent();

    /** Tells the list that rows were inserted into the model.

        Call it after inserting. Selected rows, the current row and visible components after the
        insertion point shift with their rows; when the insertion is entirely before the first
        visible row, the scroll position moves by the inserted size so the visible rows stay put.

        @param startRow  The index of the first inserted row
        @param count     The number of inserted rows
    */
    void rowsInserted (int startRow, int count);

    /** Tells the list that rows were removed from the model.

        Call it after removing. Removed rows leave the selection, which is reported through
        ListBoxModel::selectedRowsChanged() only if a selected row was removed; rows after them shift
        silently. When the removal is entirely before the first visible row, the scroll position
        moves by the removed size so the visible rows stay put.

        @param startRow  The index of the first removed row
        @param count     The number of removed rows
    */
    void rowsRemoved (int startRow, int count);

    /** Tells the list that a row moved within the model.

        Call it after moving. The row that was at @a fromRow is now at @a toRow; the selection and
        the current row follow it and the rows in between.

        @param fromRow  The row's index before the move
        @param toRow    The row's index after the move
    */
    void rowMoved (int fromRow, int toRow);

    /** Tells the list that rows changed their content and/or their size.

        Re-reads the sizes of those rows and refreshes the ones that are visible.

        @param startRow  The index of the first changed row
        @param count     The number of changed rows
    */
    void rowsChanged (int startRow, int count);

    /** Repaints a specific row if it's currently visible.

        @param rowIndex  The index of the row to repaint
    */
    void repaintRow (int rowIndex);

    //==============================================================================
    /** Scrolls a row into view.

        @param rowIndex   The index of the row to show
        @param alignment  Where to place the row, see ScrollAlignment
        @param animated   Whether to animate there instead of jumping
    */
    void scrollToRow (int rowIndex, ScrollAlignment alignment = ScrollAlignment::nearest, bool animated = false);

    /** Sets the scroll position, the distance in points the content is scrolled along the scroll axis.

        @param newOffset  The new position, clamped to the valid range
        @param animated   Whether to animate there instead of jumping
    */
    void setScrollPosition (float newOffset, bool animated = false);

    /** Returns the scroll position. It lies outside the valid range while the list is overscrolled. */
    float getScrollPosition() const noexcept;

    /** Sets the scroll physics used by touch scrolling and animated scrolls. */
    void setScrollOptions (const KineticScroller::Options& newOptions);

    /** Returns the scroll physics. */
    const KineticScroller::Options& getScrollOptions() const noexcept;

    /** Returns what the scroll position is doing. */
    ScrollState getScrollState() const noexcept;

    /** Makes the mouse scroll the list the way a finger does.

        Off by default, where a mouse press selects straight away and a mouse drag starts dragging
        the selected rows out of the list. When enabled, a mouse drag scrolls with momentum and
        overscroll exactly like touch, a click selects when the button is released, and
        holding the button still starts dragging rows out. Useful for touch-first interfaces that
        also run on the desktop.

        @param shouldBeEnabled  Whether mouse drags scroll the list
    */
    void setMouseDragScrollingEnabled (bool shouldBeEnabled);

    /** Returns whether mouse drags scroll the list. */
    bool isMouseDragScrollingEnabled() const noexcept;

    //==============================================================================
    /** Enables pulling the content away from its start to request a refresh.

        When the leading overscroll reaches the refresh indicator size (the theme metric
        Style::refreshIndicatorSizeId, 56 points by default) and the finger is released, the list
        calls onRefresh and holds the indicator in view until setRefreshing (false). Needs overscroll,
        see setScrollOptions().

        @param shouldBeEnabled  Whether pull-to-refresh is enabled
    */
    void setPullToRefreshEnabled (bool shouldBeEnabled);

    /** Returns whether pull-to-refresh is enabled. */
    bool isPullToRefreshEnabled() const noexcept;

    /** Starts or ends the refreshing state.

        While refreshing, the content rests below its start with the refresh indicator spinning in
        the revealed space. Ending it springs the content back.

        @param shouldBeRefreshing  Whether a refresh is in progress
    */
    void setRefreshing (bool shouldBeRefreshing);

    /** Returns whether a refresh is in progress. */
    bool isRefreshing() const noexcept;

    /** Returns how far a pull has gone towards triggering a refresh, from 0 to 1; 1 while refreshing. */
    float getPullToRefreshProgress() const noexcept;

    /** Returns the size of the pull-to-refresh indicator along the scroll axis. */
    float getRefreshIndicatorSize() const;

    /** Called when a pull triggers a refresh. Call setRefreshing (false) once the new data is in. */
    std::function<void()> onRefresh;

    //==============================================================================
    /** Called whenever the scroll position changes, with the new position. */
    std::function<void (float offset)> onScroll;

    /** Called whenever the range of visible rows changes, with the new range. */
    std::function<void (Range<int> visibleRows)> onVisibleRowsChanged;

    /** Called whenever the scroll state changes, with the new state. */
    std::function<void (ScrollState state)> onScrollStateChanged;

    /** Called once when the end of the content comes within the end-reached threshold.

        Also called when the content is shorter than the list. It is called again after the row count
        changes or after the end moves back beyond the threshold, which is what infinite loading
        needs: load more rows here and report them with rowsInserted().
    */
    std::function<void()> onEndReached;

    /** Sets how close to the end, as a fraction of the visible size, onEndReached fires. Default 0.5. */
    void setEndReachedThreshold (float viewportFraction);

    /** Returns the end-reached threshold, as a fraction of the visible size. */
    float getEndReachedThreshold() const noexcept;

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
    /** Sets the default row size along the scroll axis.

        That is the row height in a vertical list and the row width in a horizontal one. Rows whose
        ListBoxModel::getRowSize() returns 0 use it. Until this is called, the default depends on the
        orientation: 24 points for a vertical list and 96 for a horizontal one.

        @param newSize  The size in points
    */
    void setRowSize (float newSize);

    /** Returns the default row size along the scroll axis. */
    float getRowSize() const noexcept;

    /** Sets the gap between consecutive rows. Pressing a gap selects nothing.

        @param newSpacing  The gap in points
    */
    void setRowSpacing (float newSpacing);

    /** Returns the gap between consecutive rows. */
    float getRowSpacing() const noexcept;

    /** Sets the empty space before the first row (and the header) and after the last row (and the footer).

        @param leading   The space at the start of the scroll axis, in points
        @param trailing  The space at the end of the scroll axis, in points
    */
    void setContentInsets (float leading, float trailing);

    //==============================================================================
    /** Sets a component that scrolls with the rows, before the first one.

        Its size along the scroll axis is its current height in a vertical list, or its width in a
        horizontal one, read again at every layout; the other dimension fills the list.

        @param newHeader  The header, or nullptr to remove it
    */
    void setHeaderComponent (std::unique_ptr<Component> newHeader);

    /** Returns the header component, or nullptr. */
    Component* getHeaderComponent() const noexcept;

    /** Sets a component that scrolls with the rows, after the last one. Sized like the header.

        @param newFooter  The footer, or nullptr to remove it
    */
    void setFooterComponent (std::unique_ptr<Component> newFooter);

    /** Returns the footer component, or nullptr. */
    Component* getFooterComponent() const noexcept;

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

    /** Returns the vertical scrollbar, used by a vertical list.

        @return Pointer to the vertical scrollbar
    */
    ScrollBar* getVerticalScrollBar() const noexcept;

    /** Returns the horizontal scrollbar, used by a horizontal list.

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
        @return The row index, or -1 if no row is at that position (including the gaps between rows)
    */
    int getRowAt (Point<float> position) const;

    /** Returns the component being used to display a specific row.

        For a row the model leaves to the built-in renderer, that is its ListBoxItem.

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
        static inline const Identifier refreshIndicatorColorId { "listBoxRefreshIndicator" };

        /** Metric: the size of the pull-to-refresh indicator along the scroll axis. */
        static inline const Identifier refreshIndicatorSizeId { "listBoxRefreshIndicatorSize" };
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
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

    /** A press that may turn into a tap, a scroll or a long-press: every touch, and mouse presses
        when mouse drag scrolling is enabled. */
    struct PointerGesture
    {
        bool active = false;
        int touchIndex = -1;
        Point<float> downPosition;
        Point<float> lastPosition;
        double downTime = 0.0;
        int pressedRow = -1;
        bool scrolling = false;
        bool crossAxis = false;
        bool tapCancelled = false;
        bool longPressed = false;
        bool doubleTapped = false;
        WeakReference<Component> handOffTarget;
    };

    //==============================================================================
    bool isRowCountInSync() const;
    void readRowSizes();
    float readRowSize (int rowIndex) const;
    void rebuildRowStarts (int fromRow);
    float getMainSizeOf (const Component* component) const;
    float getRowsOrigin() const;
    float getRowsExtent() const;
    float getTotalContentSize() const;
    float getMaxScrollOffset() const;
    Rectangle<float> getContentArea() const;
    Range<int> computeVisibleRowRange() const;
    int getRowIndexAt (Point<float> position) const;

    void updateLayout();
    void layoutRows();
    void layoutHeaderAndFooter (Rectangle<float> contentArea);
    void updateScrollBars();
    void handleScrollBarMoved (ScrollBar& scrollBar);
    void refreshVisibleRows();
    void updateRowStates();
    void destroyAllRows();

    void remapRowIndices (const std::function<int (int)>& mapping);
    void anchorScrollPosition (int anchorRow, float anchorStart, const std::function<int (int)>& mapping);

    void handleRowClick (int rowIndex, const MouseEvent& event);
    void handleRowSelection (int rowIndex, bool shouldToggle, bool shouldExtend);
    void selectRange (int fromRow, int toRow);
    void navigateTo (int rowIndex, const KeyModifiers& modifiers);
    void notifySelectionChanged();
    void updateHoveredRow (Point<float> position);

    void dispatchPendingNotifications();

    bool isTouchLike (const MouseEvent& event) const;
    void gestureDown (const MouseEvent& event);
    void gestureDrag (const MouseEvent& event);
    void gestureUp (const MouseEvent& event);
    void releaseLostGesture();
    ListBox* findHandOffTarget (const MouseEvent& event);
    MouseEvent translateEventFor (const MouseEvent& event, Point<float> localPosition, Component& target) const;
    void triggerLongPress();
    void tapRow (int rowIndex, const MouseEvent& event);
    bool startDraggingSelectedRows (int touchIndex, Point<float> screenPosition);
    float getMinScrollOffset() const;

    //==============================================================================
    ListBoxModel* model = nullptr;
    Orientation orientation = Orientation::vertical;
    SelectionMode selectionMode = SelectionMode::single;

    Array<int> selectedRows;
    int currentRow = -1;

    /** The row a shift-click extends the selection from: the last row clicked without shift, or -1.

        Shift-clicks deliberately leave it alone, so repeated shift-clicks grow or shrink one range
        anchored at that row instead of moving the anchor each time.
    */
    int selectionAnchor = -1;

    int hoveredRow = -1;

    /** The row whose selection was deferred from mouse down to mouse up, or -1.

        Pressing a row that is already part of a multiple selection must not collapse that selection
        straight away, because the press may be the start of a drag that carries all of it. The
        collapse is applied on release instead, and skipped when a drag actually begins.
    */
    int rowSelectedOnMouseUp = -1;

    /** Whether rows can be dragged out of this list. See setDragSourceEnabled(). */
    bool dragSourceEnabled = true;

    /** The drag image handed to the drag in progress, or nullptr. Owned here because the manager
        only borrows it for the duration of the drag. */
    std::unique_ptr<Component> dragSourceComponent;

    /** The row count and sizes, read from the model only when it is notified of a change. */
    int numRows = 0;
    std::vector<float> rowSizes;

    /** Prefix sums: where each row starts, relative to the first row, spacing included. One entry
        more than there are rows. */
    std::vector<float> rowStarts;

    float rowSize = 24.0f;
    bool rowSizeIsExplicit = false;
    float rowSpacing = 0.0f;
    float leadingInset = 0.0f;
    float trailingInset = 0.0f;
    int minimumContentSize = 0;

    std::unique_ptr<Component> headerComponent;
    std::unique_ptr<Component> footerComponent;

    KineticScroller scroller;
    Range<int> visibleRowRange { 0, 0 };
    float viewportSize = 0.0f;

    std::unordered_map<int, std::unique_ptr<ListBoxRow>> visibleRows;
    std::vector<std::unique_ptr<ListBoxRow>> recycledRows;

    std::unique_ptr<ScrollBar> verticalScrollBar;
    std::unique_ptr<ScrollBar> horizontalScrollBar;

    float lastNotifiedScrollPosition = 0.0f;
    Range<int> lastNotifiedVisibleRows { 0, 0 };
    ScrollState lastNotifiedScrollState = ScrollState::idle;
    float endReachedThreshold = 0.5f;
    bool endReachedArmed = true;

    PointerGesture gesture;
    double gestureClock = 0.0;
    bool mouseDragScrollingEnabled = false;
    bool pullToRefreshEnabled = false;
    bool refreshing = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ListBox)
};

} // namespace yup
