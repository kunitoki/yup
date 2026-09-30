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
TreeViewItem::TreeViewItem() = default;

TreeViewItem::~TreeViewItem() = default;

//==============================================================================
TreeViewItem& TreeViewItem::addSubItem (std::unique_ptr<TreeViewItem> newItem, int index)
{
    // An item can only be added once, and only where it does not create a cycle.
    jassert (newItem != nullptr && newItem->parentItem == nullptr && newItem->ownerView == nullptr);
    jassert (! isEqualToOrDescendantOf (*newItem));

    const auto numSubItems = getNumSubItems();
    if (! isPositiveAndNotGreaterThan (index, numSubItems))
        index = numSubItems;

    auto& added = *newItem;
    added.parentItem = this;
    subItems.insert (subItems.begin() + index, std::move (newItem));
    added.setOwnerViewRecursively (ownerView);

    if (ownerView != nullptr)
        ownerView->subItemAdded (*this, index);

    return added;
}

std::unique_ptr<TreeViewItem> TreeViewItem::removeSubItem (int index)
{
    if (! isPositiveAndBelow (index, getNumSubItems()))
        return {};

    if (ownerView != nullptr)
        ownerView->subItemRemoving (*subItems[static_cast<size_t> (index)]);

    auto removed = std::move (subItems[static_cast<size_t> (index)]);
    subItems.erase (subItems.begin() + index);

    removed->parentItem = nullptr;
    removed->setOwnerViewRecursively (nullptr);

    if (ownerView != nullptr)
        ownerView->subItemsChanged (*this);

    return removed;
}

void TreeViewItem::moveSubItem (int currentIndex, int newIndex)
{
    const auto numSubItems = getNumSubItems();

    if (currentIndex == newIndex || ! isPositiveAndBelow (currentIndex, numSubItems) || ! isPositiveAndBelow (newIndex, numSubItems))
        return;

    auto moved = std::move (subItems[static_cast<size_t> (currentIndex)]);
    subItems.erase (subItems.begin() + currentIndex);
    subItems.insert (subItems.begin() + newIndex, std::move (moved));

    if (ownerView != nullptr)
        ownerView->subItemMoved (*this, newIndex);
}

void TreeViewItem::clearSubItems()
{
    if (subItems.empty())
        return;

    if (ownerView != nullptr)
        ownerView->subItemsClearing (*this);

    // Detach first, so nothing reaches the view while the items are being deleted.
    for (auto& item : subItems)
    {
        item->parentItem = nullptr;
        item->setOwnerViewRecursively (nullptr);
    }

    subItems.clear();

    if (ownerView != nullptr)
        ownerView->subItemsChanged (*this);
}

int TreeViewItem::getNumSubItems() const noexcept
{
    return static_cast<int> (subItems.size());
}

TreeViewItem* TreeViewItem::getSubItem (int index) const noexcept
{
    return isPositiveAndBelow (index, getNumSubItems()) ? subItems[static_cast<size_t> (index)].get() : nullptr;
}

TreeViewItem* TreeViewItem::getParentItem() const noexcept
{
    return parentItem;
}

int TreeViewItem::getIndexInParent() const noexcept
{
    if (parentItem == nullptr)
        return -1;

    const auto& siblings = parentItem->subItems;
    const auto it = std::find_if (siblings.begin(), siblings.end(), [this] (const auto& sibling)
    {
        return sibling.get() == this;
    });

    return static_cast<int> (std::distance (siblings.begin(), it));
}

int TreeViewItem::getDepth() const noexcept
{
    int depth = 0;

    for (auto* parent = parentItem; parent != nullptr; parent = parent->parentItem)
        ++depth;

    return depth;
}

TreeView* TreeViewItem::getOwnerView() const noexcept
{
    return ownerView;
}

bool TreeViewItem::isEqualToOrDescendantOf (const TreeViewItem& possibleAncestor) const noexcept
{
    for (auto* item = this; item != nullptr; item = item->parentItem)
    {
        if (item == &possibleAncestor)
            return true;
    }

    return false;
}

//==============================================================================
void TreeViewItem::setOpen (bool shouldBeOpen)
{
    if (open == shouldBeOpen)
        return;

    if (ownerView != nullptr)
    {
        ownerView->setItemOpen (*this, shouldBeOpen);
        return;
    }

    open = shouldBeOpen;
    itemOpennessChanged (open);
}

bool TreeViewItem::isOpen() const noexcept
{
    return open;
}

void TreeViewItem::setOpenRecursively (bool shouldBeOpen)
{
    auto* view = ownerView;

    if (view != nullptr)
        view->beginBulkUpdate();

    std::function<void (TreeViewItem&)> apply = [&] (TreeViewItem& item)
    {
        if (! shouldBeOpen || item.mightContainSubItems())
            item.setOpen (shouldBeOpen);

        // Opening may have created the sub-items, so they are only listed now.
        for (int index = 0; index < item.getNumSubItems(); ++index)
            apply (*item.subItems[static_cast<size_t> (index)]);
    };

    apply (*this);

    if (view != nullptr)
        view->endBulkUpdate();
}

//==============================================================================
bool TreeViewItem::isSelected() const
{
    return ownerView != nullptr && ownerView->isItemSelected (*this);
}

bool TreeViewItem::isHovered() const
{
    return ownerView != nullptr && ownerView->getHoveredItem() == this;
}

void TreeViewItem::setSelected (bool shouldBeSelected, bool deselectOthers)
{
    if (ownerView != nullptr)
        ownerView->setItemSelected (*this, shouldBeSelected, deselectOthers);
}

//==============================================================================
void TreeViewItem::itemChanged()
{
    if (ownerView != nullptr)
        ownerView->refreshItem (*this);
}

void TreeViewItem::repaintItem()
{
    if (ownerView != nullptr)
        ownerView->repaintItem (*this);
}

//==============================================================================
bool TreeViewItem::mightContainSubItems() const
{
    return getNumSubItems() > 0;
}

String TreeViewItem::getItemText() const
{
    return {};
}

Image TreeViewItem::getItemIcon() const
{
    return {};
}

bool TreeViewItem::hasItemIcon() const
{
    return getItemIcon().isValid();
}

void TreeViewItem::paintItemIcon (Graphics& g, Rectangle<float> area, bool isSelected) const
{
    ignoreUnused (isSelected);

    if (const auto icon = getItemIcon(); icon.isValid())
        g.drawImage (icon, area);
}

String TreeViewItem::getUniqueName() const
{
    return getItemText();
}

float TreeViewItem::getItemHeight() const
{
    return 0.0f;
}

void TreeViewItem::refreshItemComponent (std::unique_ptr<Component>& component)
{
    // A recycled row may still hold another item's content.
    component.reset();
}

void TreeViewItem::itemOpennessChanged (bool isNowOpen)
{
    ignoreUnused (isNowOpen);
}

void TreeViewItem::itemClicked (const MouseEvent& event)
{
    ignoreUnused (event);
}

void TreeViewItem::itemDoubleClicked (const MouseEvent& event)
{
    ignoreUnused (event);

    if (mightContainSubItems())
        setOpen (! isOpen());
}

void TreeViewItem::itemSelectionChanged (bool isNowSelected)
{
    ignoreUnused (isNowSelected);
}

void TreeViewItem::itemEntered() {}

void TreeViewItem::itemExited() {}

//==============================================================================
var TreeViewItem::getDragSourceDescription() const
{
    return {};
}

bool TreeViewItem::isInterestedInDragSource (const DragAndDropSourceDetails& details) const
{
    ignoreUnused (details);
    return false;
}

void TreeViewItem::itemDropped (const DragAndDropSourceDetails& details, int insertIndex)
{
    ignoreUnused (details, insertIndex);
}

//==============================================================================
void TreeViewItem::setOwnerViewRecursively (TreeView* newOwnerView) noexcept
{
    ownerView = newOwnerView;
    rowIndex = -1;

    for (auto& item : subItems)
        item->setOwnerViewRecursively (newOwnerView);
}

} // namespace yup
