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

namespace yup
{

//==============================================================================
DataTreeViewItem::DataTreeViewItem (DataTree nodeToShow, UndoManager::Ptr undoManagerToUse)
    : node (std::move (nodeToShow))
    , undoManager (std::move (undoManagerToUse))
{
    node.addListener (this);
}

DataTreeViewItem::~DataTreeViewItem()
{
    node.removeListener (this);
}

//==============================================================================
const DataTree& DataTreeViewItem::getDataTree() const noexcept
{
    return node;
}

UndoManager::Ptr DataTreeViewItem::getUndoManager() const noexcept
{
    return undoManager;
}

//==============================================================================
std::unique_ptr<TreeViewItem> DataTreeViewItem::createSubItem (const DataTree& child)
{
    return std::make_unique<DataTreeViewItem> (child, undoManager);
}

//==============================================================================
bool DataTreeViewItem::mightContainSubItems() const
{
    return node.getNumChildren() > 0;
}

String DataTreeViewItem::getItemText() const
{
    return node.getType().toString();
}

void DataTreeViewItem::itemOpennessChanged (bool isNowOpen)
{
    if (isNowOpen)
        createSubItems();
}

var DataTreeViewItem::getDragSourceDescription() const
{
    return getItemText();
}

bool DataTreeViewItem::isInterestedInDragSource (const DragAndDropSourceDetails& details) const
{
    const auto draggedNodes = getDraggedNodes (details);

    if (draggedNodes.empty())
        return false;

    return std::none_of (draggedNodes.begin(), draggedNodes.end(), [this] (const DataTree& dragged)
    {
        return node == dragged || node.isAChildOf (dragged);
    });
}

void DataTreeViewItem::itemDropped (const DragAndDropSourceDetails& details, int insertIndex)
{
    // Taken before anything moves: moving the nodes deletes the items that were dragged.
    const auto draggedNodes = getDraggedNodes (details);

    if (draggedNodes.empty())
        return;

    {
        std::optional<UndoManager::ScopedTransaction> undoTransaction;

        if (undoManager != nullptr)
            undoTransaction.emplace (*undoManager);

        // Without sub-items yet, the TreeView could only offer "into", which appends.
        auto targetIndex = subItemsCreated ? insertIndex : node.getNumChildren();

        for (const auto& dragged : draggedNodes)
        {
            auto oldParent = dragged.getParent();

            if (! oldParent.isValid())
                continue;

            if (oldParent == node)
            {
                // The target index counts the node itself when it sits before it.
                const auto currentIndex = node.indexOf (dragged);
                const auto newIndex = jmin (node.getNumChildren() - 1, targetIndex > currentIndex ? targetIndex - 1 : targetIndex);

                if (newIndex != currentIndex)
                {
                    auto transaction = node.beginTransaction (undoManager.get());
                    transaction.moveChild (currentIndex, newIndex);
                }

                targetIndex = newIndex + 1;
            }
            else
            {
                {
                    auto transaction = oldParent.beginTransaction (undoManager.get());
                    transaction.removeChild (dragged);
                }

                {
                    auto transaction = node.beginTransaction (undoManager.get());
                    transaction.addChild (dragged, targetIndex);
                }

                ++targetIndex;
            }
        }
    }

    bool deselectOthers = true;

    for (const auto& dragged : draggedNodes)
    {
        for (int index = 0; index < getNumSubItems(); ++index)
        {
            auto* item = dynamic_cast<DataTreeViewItem*> (getSubItem (index));

            if (item != nullptr && item->node == dragged)
            {
                item->setSelected (true, std::exchange (deselectOthers, false));
                break;
            }
        }
    }
}

//==============================================================================
void DataTreeViewItem::propertyChanged (DataTree& tree, const Identifier& property)
{
    ignoreUnused (tree, property);
    itemChanged();
}

void DataTreeViewItem::childAdded (DataTree& parent, DataTree& child)
{
    ignoreUnused (parent);

    // Before the first open there are no sub-items to update, but the disclosure button may appear.
    if (! subItemsCreated)
    {
        itemChanged();
        return;
    }

    addSubItem (createSubItem (child), node.indexOf (child));
}

void DataTreeViewItem::childRemoved (DataTree& parent, DataTree& child, int formerIndex)
{
    ignoreUnused (parent);

    if (! subItemsCreated)
    {
        itemChanged();
        return;
    }

    const auto showsChild = [this, &child] (int index)
    {
        auto* item = dynamic_cast<DataTreeViewItem*> (getSubItem (index));
        return item != nullptr && item->node == child;
    };

    auto index = formerIndex;

    // Sub-items that are not DataTreeViewItems cannot be checked, and then the index is trusted.
    if (! showsChild (index))
    {
        for (int candidate = 0; candidate < getNumSubItems(); ++candidate)
        {
            if (showsChild (candidate))
            {
                index = candidate;
                break;
            }
        }
    }

    removeSubItem (index);
}

void DataTreeViewItem::childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex)
{
    ignoreUnused (parent, child);

    if (subItemsCreated)
        moveSubItem (oldIndex, newIndex);
}

//==============================================================================
void DataTreeViewItem::createSubItems()
{
    if (subItemsCreated)
        return;

    subItemsCreated = true;

    for (int index = 0; index < node.getNumChildren(); ++index)
        addSubItem (createSubItem (node.getChild (index)));
}

std::vector<DataTree> DataTreeViewItem::getDraggedNodes (const DragAndDropSourceDetails& details)
{
    std::vector<DataTree> nodes;

    for (auto* item : TreeView::getDraggedItems (details))
    {
        auto* dataItem = dynamic_cast<DataTreeViewItem*> (item);

        if (dataItem == nullptr)
            return {};

        nodes.push_back (dataItem->node);
    }

    return nodes;
}

} // namespace yup
