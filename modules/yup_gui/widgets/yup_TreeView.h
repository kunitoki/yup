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
/**
    A component that shows a hierarchy of TreeViewItem objects as an indented, scrollable list.

    The TreeView owns a root item, and each item owns its sub-items. The rows are the items that
    are currently visible: an item is visible when all its parents are open. Opening an item inserts
    the rows of its sub-items below it, closing it removes them, and any change made to the items
    through TreeViewItem updates only the affected rows, keeping the selection and the scroll
    position in place.

    The rows are shown by an internal ListBox, so scrolling, row recycling, keyboard navigation and
    selection behave like in a ListBox, and the ListBox::Style colors apply to the rows. On top of
    that, Right and Left open and close items or move to the first sub-item and to the parent, and
    Alt+Right / Alt+Left open or close a whole branch.

    The TreeView is also a drop target: items opt in to receive dragged content through
    TreeViewItem::isInterestedInDragSource(), and getDraggedItems() tells what is being dragged
    when the drag started from a TreeView, which makes reordering a few lines of code.

    @code
    auto root = std::make_unique<MyItem> ("Root");
    root->addSubItem (std::make_unique<MyItem> ("Child"));

    TreeView treeView;
    treeView.setRootItem (std::move (root));
    treeView.setRootItemVisible (false);
    @endcode

    @see TreeViewItem, DataTreeViewItem, TreeViewRow
*/
class YUP_API TreeView : public Component
    , public DragAndDropTarget
{
public:
    //==============================================================================
    /** Creates an empty TreeView.

        @param componentID  Optional component identifier
    */
    explicit TreeView (StringRef componentID = {});

    /** Destructor. Deletes the root item. */
    ~TreeView() override;

    //==============================================================================
    /** Sets the root item, deleting the previous one.

        @param newRootItem  The new root, which must not have a parent, or nullptr for an empty tree
    */
    void setRootItem (std::unique_ptr<TreeViewItem> newRootItem);

    /** Returns the root item, or nullptr. */
    TreeViewItem* getRootItem() const noexcept;

    /** Shows or hides the root item's row.

        When hidden, the root is kept open and its sub-items become the top level rows. Visible by default.

        @param shouldBeVisible  Whether the root item has a row
    */
    void setRootItemVisible (bool shouldBeVisible);

    /** Returns true if the root item has a row. */
    bool isRootItemVisible() const noexcept;

    /** Shows or hides the disclosure buttons that open and close items. Visible by default.

        @param shouldBeVisible  Whether items that may contain sub-items show a disclosure button
    */
    void setOpenCloseButtonsVisible (bool shouldBeVisible);

    /** Returns true if the disclosure buttons are shown. */
    bool areOpenCloseButtonsVisible() const noexcept;

    //==============================================================================
    /** Sets how far each level is indented, which is also the width of the disclosure button. Default 20.

        @param newIndentSize  The indentation in points
    */
    void setIndentSize (float newIndentSize);

    /** Returns how far each level is indented. */
    float getIndentSize() const noexcept;

    /** Sets the height of the rows whose item returns 0 from TreeViewItem::getItemHeight(). Default 24.

        @param newHeight  The height in points
    */
    void setDefaultItemHeight (float newHeight);

    /** Returns the default row height. */
    float getDefaultItemHeight() const noexcept;

    /** Shows or hides the vertical lines that connect the rows of each level. Visible by default.

        @param shouldBeVisible  Whether the indent guides are painted
    */
    void setIndentGuidesVisible (bool shouldBeVisible);

    /** Returns true if the indent guides are painted. */
    bool areIndentGuidesVisible() const noexcept;

    /** Sets how long opening and closing an item animates. Default 0.15 seconds.

        The rows of the sub-items grow from nothing, or shrink to nothing before being removed. The
        change is instant when the item's row is out of view, when more than two screens of rows
        would move, or when @a seconds is 0.

        @param seconds  The duration of the animation, or 0 to disable it
    */
    void setExpandAnimationTime (double seconds);

    /** Returns how long opening and closing an item animates. */
    double getExpandAnimationTime() const noexcept;

    //==============================================================================
    /** Sets the visibility mode of the vertical scrollbar. */
    void setVerticalScrollBarVisibility (ScrollBar::VisibilityMode mode);

    /** Sets the scroll physics, see ListBox::setScrollOptions(). */
    void setScrollOptions (const KineticScroller::Options& newOptions);

    /** Returns the scroll physics. */
    const KineticScroller::Options& getScrollOptions() const noexcept;

    /** Sets the scroll position, the distance in points the rows are scrolled.

        @param newPosition  The new position, clamped to the valid range
        @param animated     Whether to animate there instead of jumping
    */
    void setScrollPosition (float newPosition, bool animated = false);

    /** Returns the scroll position. */
    float getScrollPosition() const noexcept;

    //==============================================================================
    /** Sets the selection mode. Single selection by default. */
    void setSelectionMode (ListBox::SelectionMode mode);

    /** Returns the selection mode. */
    ListBox::SelectionMode getSelectionMode() const noexcept;

    /** Returns the number of selected items. Only items on a visible row can be selected. */
    int getNumSelectedItems() const;

    /** Returns the selected items, in row order. */
    std::vector<TreeViewItem*> getSelectedItems() const;

    /** Deselects all the items. */
    void clearSelectedItems();

    //==============================================================================
    /** Returns the number of rows, that is the number of visible items. */
    int getNumRowsInTree() const noexcept;

    /** Returns the item shown on a row, or nullptr when the row does not exist. */
    TreeViewItem* getItemOnRow (int rowIndex) const noexcept;

    /** Returns the row of an item, or -1 when the item is not visible or not in this tree. */
    int getRowOf (const TreeViewItem& item) const noexcept;

    /** Returns the item at a position in this component, or nullptr. */
    TreeViewItem* getItemAt (Point<float> position) const;

    /** Returns the item under the mouse, or nullptr. Fingers never hover. */
    TreeViewItem* getHoveredItem() const noexcept;

    /** Returns the bounds of an item's row in this component, or an empty rectangle when it is not visible. */
    Rectangle<float> getItemBounds (const TreeViewItem& item) const;

    /** Scrolls an item's row into view. Only visible items can be scrolled to.

        @param item       The item to show
        @param alignment  Where to place the row, see ListBox::ScrollAlignment
        @param animated   Whether to animate there instead of jumping
    */
    void scrollToItem (const TreeViewItem& item, ListBox::ScrollAlignment alignment = ListBox::ScrollAlignment::nearest, bool animated = false);

    //==============================================================================
    /** Captures which items are open and selected, identified by TreeViewItem::getUniqueName().

        @param includeScrollPosition  Whether to store the scroll position as well
        @return A DataTree to pass to restoreOpennessState()
    */
    DataTree getOpennessState (bool includeScrollPosition) const;

    /** Opens and closes the items as recorded by getOpennessState().

        Items that are not recorded are closed. Lazily created sub-items are created as their
        parents open, so a whole saved hierarchy is restored.

        @param state             A state returned by getOpennessState()
        @param restoreSelection  Whether to select the items that were selected
    */
    void restoreOpennessState (const DataTree& state, bool restoreSelection);

    //==============================================================================
    /** Returns the items being dragged when the drag started from a TreeView, or an empty vector.

        The dragged items are the selection of the source tree, without the items whose parent is
        dragged along with them. Call it before changing the trees, and do not keep the result
        beyond the drop.

        @param details  The drag, as handed to a drop target callback
        @return The dragged items, in row order
    */
    static std::vector<TreeViewItem*> getDraggedItems (const DragAndDropSourceDetails& details);

    //==============================================================================
    /** Called when the selection changes. */
    std::function<void()> onSelectionChanged;

    /** Called when an item's row is clicked. */
    std::function<void (TreeViewItem& item)> onItemClicked;

    /** Called when an item's row is double-clicked. */
    std::function<void (TreeViewItem& item)> onItemDoubleClicked;

    /** Called when the mouse moves onto an item's row, after TreeViewItem::itemEntered(). */
    std::function<void (TreeViewItem& item)> onItemEntered;

    /** Called when the mouse leaves an item's row, after TreeViewItem::itemExited(). Not called for an
        item removed from the tree. */
    std::function<void (TreeViewItem& item)> onItemExited;

    /** Called when Return is pressed, with the item on the current row. */
    std::function<void (TreeViewItem& item)> onReturnKeyPressed;

    /** Called when Delete or Backspace is pressed, with the selected items. */
    std::function<void (std::vector<TreeViewItem*> selectedItems)> onDeleteKeyPressed;

    //==============================================================================
    /** Style identifiers for theming.

        The rows also use the ListBox::Style colors for the background and the selection, while the
        hovered row is painted with itemHoveredColorId. Colors set on the TreeView reach its rows.
    */
    struct Style
    {
        static inline const Identifier indentGuideColorId { "treeViewIndentGuide" };
        static inline const Identifier disclosureColorId { "treeViewDisclosure" };
        static inline const Identifier itemTextColorId { "treeViewItemText" };
        static inline const Identifier itemTextSelectedColorId { "treeViewItemTextSelected" };
        static inline const Identifier dropIndicatorColorId { "treeViewDropIndicator" };
        static inline const Identifier itemHoveredColorId { "treeViewItemHovered" };
    };

    //==============================================================================
    /** @internal */
    void resized() override;
    /** @internal */
    void paintOverChildren (Graphics& g) override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;
    /** @internal */
    void keyDown (const KeyPress& key, const Point<float>& position) override;
    /** @internal */
    bool isInterestedInDragSource (const DragAndDropSourceDetails& details) override;
    /** @internal */
    void itemDragEnter (const DragAndDropSourceDetails& details) override;
    /** @internal */
    void itemDragMove (const DragAndDropSourceDetails& details) override;
    /** @internal */
    void itemDragExit (const DragAndDropSourceDetails& details) override;
    /** @internal */
    bool itemDropped (const DragAndDropSourceDetails& details) override;

private:
    //==============================================================================
    friend class TreeViewItem;
    friend class TreeViewRow;

    class TreeListBox;
    class TreeListModel;
    class ScopedSelectionBatch;

    /** An open or close in progress: the sub-item rows of an item growing from or shrinking to nothing. */
    struct ExpandAnimation
    {
        TreeViewItem* item = nullptr;
        int firstRow = 0;
        int numRows = 0;
        bool expanding = true;
        double progress = 0.0;
    };

    /** Where dragged content would land. */
    struct DropTarget
    {
        TreeViewItem* parent = nullptr;
        int insertIndex = 0;
        bool into = false;
        Rectangle<float> indicator;
    };

    //==============================================================================
    // Called by TreeViewItem.
    void subItemAdded (TreeViewItem& parent, int index);
    void subItemRemoving (TreeViewItem& item);
    void subItemsClearing (TreeViewItem& parent);
    void subItemsChanged (TreeViewItem& parent);
    void subItemMoved (TreeViewItem& parent, int newIndex);
    void setItemOpen (TreeViewItem& item, bool shouldBeOpen);
    bool isItemSelected (const TreeViewItem& item) const;
    void setItemSelected (TreeViewItem& item, bool shouldBeSelected, bool deselectOthers);
    void refreshItem (const TreeViewItem& item);
    void repaintItem (const TreeViewItem& item);
    void beginBulkUpdate();
    void endBulkUpdate();

    // Called by TreeViewRow.
    float getOpenFraction (const TreeViewItem& item) const;
    float getFullItemHeight (const TreeViewItem& item) const;

    // Called by the list model.
    float getRowSize (int rowIndex) const;
    void refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component);
    void handleListSelectionChanged();
    var getDragSourceDescription (const Array<int>& selectedRows) const;
    bool handleKeyPress (const KeyPress& key);

    //==============================================================================
    bool isDisplayedOpen (const TreeViewItem& item) const noexcept;
    bool areSubItemsShown (const TreeViewItem& item) const noexcept;
    int getDisplayDepth (const TreeViewItem& item) const noexcept;
    void appendRun (TreeViewItem& item, std::vector<TreeViewItem*>& run) const;
    int getRunLength (const TreeViewItem& item) const;
    int getRowForSubItem (const TreeViewItem& parent, int index) const;
    void renumberRows (int fromRow);
    void insertRows (int firstRow, const std::vector<TreeViewItem*>& run);
    void removeRows (int firstRow, int count);
    void rebuildRows (const std::vector<TreeViewItem*>& itemsToSelect, TreeViewItem* itemToMakeCurrent);
    void ensureHiddenRootIsOpen();
    void forgetItems (const TreeViewItem& subtree);
    void updateHoveredItem (const MouseEvent& event, bool pointerLeftList);
    void setHoveredItem (TreeViewItem* newItem);
    bool isRowListInSync() const;

    void expandItem (TreeViewItem& item);
    void collapseItem (TreeViewItem& item);
    void moveSelectionToItem (const TreeViewItem& item, int firstRow, int count);
    void selectOnlyRow (int rowIndex);

    bool shouldAnimate (const TreeViewItem& item, int numRows) const;
    float getRevealFraction() const;
    void reverseAnimation();
    void finishAnimation();

    void sendSelectionNotifications();

    std::optional<DropTarget> findDropTarget (const DragAndDropSourceDetails& details) const;
    void updateDragState (const DragAndDropSourceDetails& details);
    void clearDragState();

    //==============================================================================
    std::unique_ptr<TreeListModel> model;
    std::unique_ptr<TreeListBox> list;
    std::unique_ptr<TreeViewItem> rootItem;

    /** The visible items, in row order; each caches its index in TreeViewItem::rowIndex. */
    std::vector<TreeViewItem*> rows;

    bool rootVisible = true;
    bool openCloseButtonsVisible = true;
    bool indentGuidesVisible = true;
    float indentSize = 20.0f;
    float defaultItemHeight = 24.0f;
    double expandAnimationTime = 0.15;

    /** The item whose TreeViewItem::itemOpennessChanged() runs while it opens: its sub-item rows are
        not inserted yet, so changes to its sub-items must not insert rows either. */
    TreeViewItem* openingItem = nullptr;

    /** While positive, changes only update the items and the rows are rebuilt at the end. */
    int bulkUpdateDepth = 0;
    std::vector<TreeViewItem*> bulkSelection;
    TreeViewItem* bulkCurrentItem = nullptr;

    int selectionBatchDepth = 0;
    bool selectionChangePending = false;
    std::vector<TreeViewItem*> notifiedSelection;

    std::optional<ExpandAnimation> animation;

    std::optional<DropTarget> dropTarget;
    std::optional<DragAndDropSourceDetails> activeDrag;
    TreeViewItem* hoverExpandItem = nullptr;

    /** The item under the mouse, and the item whose enter or exit notification is running: cleared when
        it is removed, so the notification does not reach a deleted item. */
    TreeViewItem* hoveredItem = nullptr;
    TreeViewItem* notifyingHoverItem = nullptr;
    double hoverExpandSeconds = 0.0;
    float autoScrollSpeed = 0.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TreeView)
};

} // namespace yup
