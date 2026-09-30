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
    A TreeViewItem that shows a DataTree node and follows its changes.

    The sub-items mirror the node's children. They are created the first time the item opens,
    through createSubItem(), and from then on children added, removed or moved in the DataTree are
    added, removed or moved in the TreeView, and property changes refresh the row. Undo and redo
    are reflected the same way.

    Items can be dragged and dropped to move their nodes: within a parent the move is a single
    DataTree move, across parents a removal and an addition that undo as one step when an
    UndoManager is given. A node cannot be dropped into itself or its descendants. The moved items
    are selected afterwards.

    The unique name used by TreeView::getOpennessState() is the item text, the node type by default,
    so override getItemText() or getUniqueName() when siblings share a type.

    @code
    class NodeItem : public DataTreeViewItem
    {
    public:
        using DataTreeViewItem::DataTreeViewItem;

        String getItemText() const override { return getDataTree().getProperty ("name").toString(); }

        std::unique_ptr<TreeViewItem> createSubItem (const DataTree& child) override
        {
            return std::make_unique<NodeItem> (child, getUndoManager());
        }
    };

    treeView.setRootItem (std::make_unique<NodeItem> (document, undoManager));
    @endcode

    @see TreeView, TreeViewItem, DataTree
*/
class YUP_API DataTreeViewItem : public TreeViewItem
    , private DataTreeListener
{
public:
    //==============================================================================
    /** Creates an item showing a node.

        @param node         The node to show
        @param undoManager  The UndoManager that drag and drop moves are recorded in, or nullptr
    */
    DataTreeViewItem (DataTree node, UndoManager::Ptr undoManager = nullptr);

    /** Destructor. */
    ~DataTreeViewItem() override;

    //==============================================================================
    /** Returns the node this item shows. */
    const DataTree& getDataTree() const noexcept;

    /** Returns the UndoManager moves are recorded in, or nullptr. */
    UndoManager::Ptr getUndoManager() const noexcept;

    //==============================================================================
    /** Creates the item for one of the node's children. The default creates a DataTreeViewItem.

        Override it to show your own item type throughout the tree.

        @param child  The child node
        @return The item showing it
    */
    virtual std::unique_ptr<TreeViewItem> createSubItem (const DataTree& child);

    //==============================================================================
    /** Returns true when the node has children. */
    bool mightContainSubItems() const override;

    /** Returns the node's type. */
    String getItemText() const override;

    /** Creates the sub-items the first time the item opens. */
    void itemOpennessChanged (bool isNowOpen) override;

    /** Returns the item text, which makes every DataTreeViewItem draggable. */
    var getDragSourceDescription() const override;

    /** Accepts DataTreeViewItems dragged from a TreeView, unless this item is one of them or inside one. */
    bool isInterestedInDragSource (const DragAndDropSourceDetails& details) const override;

    /** Moves the dragged nodes among this node's children. */
    void itemDropped (const DragAndDropSourceDetails& details, int insertIndex) override;

private:
    //==============================================================================
    void propertyChanged (DataTree& tree, const Identifier& property) override;
    void childAdded (DataTree& parent, DataTree& child) override;
    void childRemoved (DataTree& parent, DataTree& child, int formerIndex) override;
    void childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex) override;

    void createSubItems();
    static std::vector<DataTree> getDraggedNodes (const DragAndDropSourceDetails& details);

    DataTree node;
    UndoManager::Ptr undoManager;
    bool subItemsCreated = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataTreeViewItem)
};

} // namespace yup
