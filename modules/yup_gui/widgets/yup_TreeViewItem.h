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

class TreeView;

//==============================================================================
/**
    A node shown by a TreeView.

    Subclass it to describe your data: override getItemText() (and optionally getItemIcon(), or
    hasItemIcon() and paintItemIcon() to draw the icon yourself) for the built-in look, or
    refreshItemComponent() for custom content. An item owns its sub-items, and a
    TreeView owns its root item.

    Every structural change made through this class - adding, removing or moving sub-items, opening
    or closing - is reported to the TreeView the item belongs to, which updates only the rows that
    are affected. There is nothing to notify by hand; call itemChanged() when what an item shows
    changes.

    Sub-items can be created lazily: return true from mightContainSubItems() and add them in
    itemOpennessChanged() the first time the item opens.

    @code
    class FolderItem : public TreeViewItem
    {
    public:
        explicit FolderItem (File folder) : folder (folder) {}

        String getItemText() const override { return folder.getFileName(); }
        bool mightContainSubItems() const override { return folder.isDirectory(); }

        void itemOpennessChanged (bool isNowOpen) override
        {
            if (isNowOpen && getNumSubItems() == 0)
                for (const auto& child : folder.findChildFiles (File::findFilesAndDirectories, false))
                    addSubItem (std::make_unique<FolderItem> (child));
        }

    private:
        File folder;
    };
    @endcode

    @see TreeView, DataTreeViewItem
*/
class YUP_API TreeViewItem
{
public:
    //==============================================================================
    /** Creates an item with no sub-items, closed and not attached to any TreeView. */
    TreeViewItem();

    /** Destructor. Deletes the sub-items. */
    virtual ~TreeViewItem();

    //==============================================================================
    /** Adds a sub-item, taking ownership of it.

        @param newItem  The item to add; it must not be null nor belong to another parent
        @param index    Where to insert it among the sub-items, or -1 to append
        @return The added item
    */
    TreeViewItem& addSubItem (std::unique_ptr<TreeViewItem> newItem, int index = -1);

    /** Removes a sub-item and hands it back.

        @param index  The index of the sub-item to remove
        @return The removed item, or nullptr when the index is out of range
    */
    std::unique_ptr<TreeViewItem> removeSubItem (int index);

    /** Moves a sub-item to a new position among its siblings.

        @param currentIndex  The index of the sub-item to move
        @param newIndex      The index the sub-item has once the move is done
    */
    void moveSubItem (int currentIndex, int newIndex);

    /** Deletes all the sub-items. */
    void clearSubItems();

    /** Returns the number of sub-items. */
    int getNumSubItems() const noexcept;

    /** Returns a sub-item, or nullptr when the index is out of range. */
    TreeViewItem* getSubItem (int index) const noexcept;

    /** Returns the item this one is a sub-item of, or nullptr for a root or a detached item. */
    TreeViewItem* getParentItem() const noexcept;

    /** Returns the index of this item among its parent's sub-items, or -1 when it has no parent. */
    int getIndexInParent() const noexcept;

    /** Returns how many parents this item has: 0 for a root item. */
    int getDepth() const noexcept;

    /** Returns the TreeView this item is shown by, or nullptr. */
    TreeView* getOwnerView() const noexcept;

    /** Returns true if this item is @a possibleAncestor or one of its descendants. */
    bool isEqualToOrDescendantOf (const TreeViewItem& possibleAncestor) const noexcept;

    //==============================================================================
    /** Opens or closes the item.

        Opening shows the sub-items and calls itemOpennessChanged(), which may create them. The
        TreeView may animate the change, see TreeView::setExpandAnimationTime(). Closing an item
        that holds the selection moves the selection to the item.

        @param shouldBeOpen  Whether the item should be open
    */
    void setOpen (bool shouldBeOpen);

    /** Returns true if the item is open. */
    bool isOpen() const noexcept;

    /** Opens or closes this item and all its descendants in one go.

        Opening only opens the items whose mightContainSubItems() returns true, and each of them gets
        its itemOpennessChanged() call before its own sub-items are visited, so lazily created
        sub-items are opened too.

        @param shouldBeOpen  Whether the items should be open
    */
    void setOpenRecursively (bool shouldBeOpen);

    //==============================================================================
    /** Returns true if the item is selected in its TreeView. */
    bool isSelected() const;

    /** Selects or deselects the item.

        Only items shown by a TreeView on a visible row can be selected.

        @param shouldBeSelected  Whether the item should be selected
        @param deselectOthers    Whether the rest of the selection should be cleared when selecting
    */
    void setSelected (bool shouldBeSelected, bool deselectOthers = true);

    /** Returns true while the mouse is over the item's row. Fingers never hover. */
    bool isHovered() const;

    //==============================================================================
    /** Tells the TreeView that what this item shows changed, including its height. */
    void itemChanged();

    /** Repaints the item's row, if it is visible. */
    void repaintItem();

    //==============================================================================
    /** Returns true if this item has or may have sub-items, which gives it a disclosure button.

        The default returns getNumSubItems() > 0. Override it to return true for items whose
        sub-items are created lazily in itemOpennessChanged().
    */
    virtual bool mightContainSubItems() const;

    /** Returns the text the built-in row shows. */
    virtual String getItemText() const;

    /** Returns the icon the built-in row shows before the text, or an invalid Image for none. */
    virtual Image getItemIcon() const;

    /** Returns true if the built-in row shows an icon before the text.

        The default returns true when getItemIcon() is valid. Override it together with
        paintItemIcon() to draw the icon instead of providing an image.
    */
    virtual bool hasItemIcon() const;

    /** Draws the icon of the built-in row. Only called when hasItemIcon() returns true.

        The default draws getItemIcon() into @a area. Override it to draw a vector shape, a glyph of
        the theme's icon font or a Drawable, and to tint it for the selection:

        @code
        void paintItemIcon (Graphics& g, Rectangle<float> area, bool isSelected) const override
        {
            Path folder;
            folder.addRoundedRectangle (area.reduced (1.0f), 2.0f);

            g.setFillColor (isSelected ? Colors::white : Color (0xffe8b04a));
            g.fillPath (folder);
        }
        @endcode

        @param g           The graphics context, in the row's coordinates
        @param area        The square to draw the icon in
        @param isSelected  Whether the item is selected
    */
    virtual void paintItemIcon (Graphics& g, Rectangle<float> area, bool isSelected) const;

    /** Returns a name that identifies this item among its siblings.

        TreeView::getOpennessState() and TreeView::restoreOpennessState() use it to recognize items.
        The default returns getItemText().
    */
    virtual String getUniqueName() const;

    /** Returns the height of the item's row, or 0 or less to use TreeView::getDefaultItemHeight(). */
    virtual float getItemHeight() const;

    /** Creates or updates custom content for the item's row.

        The default resets @a component, which shows the built-in icon and text. Otherwise create or
        update it in place, typically with reuseOrCreate(). Row components are recycled, so the
        component passed in may have been showing another item, possibly of another type. The content is laid out after the indentation and
        the disclosure button, at the full height of the row.

        Content that should still let a press select the row calls setWantsMouseEvents (false, true)
        on itself, so that only its interactive children take presses.

        @param component  The row's current content, owned by the row; may be null
    */
    virtual void refreshItemComponent (std::unique_ptr<Component>& component);

    /** Called when the item opens or closes. This is where lazily created sub-items are added.

        @param isNowOpen  Whether the item is now open
    */
    virtual void itemOpennessChanged (bool isNowOpen);

    /** Called when the item's row is clicked. */
    virtual void itemClicked (const MouseEvent& event);

    /** Called when the item's row is double-clicked. The default toggles the item open or closed. */
    virtual void itemDoubleClicked (const MouseEvent& event);

    /** Called when the item gets selected or deselected. */
    virtual void itemSelectionChanged (bool isNowSelected);

    /** Called when the mouse moves onto the item's row, including onto the row's own child components. */
    virtual void itemEntered();

    /** Called when the mouse leaves the item's row.

        Not called when the item is removed from the tree while hovered.
    */
    virtual void itemExited();

    //==============================================================================
    /** Returns a description of the item for dragging it out of the TreeView.

        An empty var, the default, makes the item not draggable. When several items are dragged,
        the drag carries an array of their descriptions.
    */
    virtual var getDragSourceDescription() const;

    /** Returns true if this item accepts the dragged content as sub-items. Defaults to false.

        TreeView::getDraggedItems() tells what is being dragged when it comes from a TreeView.
    */
    virtual bool isInterestedInDragSource (const DragAndDropSourceDetails& details) const;

    /** Called when content is dropped among this item's sub-items.

        @param details      The drop
        @param insertIndex  The index among the sub-items where the content should be inserted
    */
    virtual void itemDropped (const DragAndDropSourceDetails& details, int insertIndex);

protected:
    /** Returns the content component as a T, creating one when it is null or of another type.

        Meant for refreshItemComponent(), see ListBoxModel::reuseOrCreate().
    */
    template <class T, class... Args>
    static T& reuseOrCreate (std::unique_ptr<Component>& component, Args&&... args)
    {
        return ListBoxModel::reuseOrCreate<T> (component, std::forward<Args> (args)...);
    }

private:
    friend class TreeView;

    void setOwnerViewRecursively (TreeView* newOwnerView) noexcept;

    TreeViewItem* parentItem = nullptr;
    TreeView* ownerView = nullptr;
    std::vector<std::unique_ptr<TreeViewItem>> subItems;
    int rowIndex = -1;
    bool open = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TreeViewItem)
};

} // namespace yup
