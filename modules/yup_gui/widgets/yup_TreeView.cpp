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

namespace
{

/** A ListBox reads a row size of 0 as "use the default", so a fully hidden animated row stays just above it. */
constexpr float treeViewMinimumAnimatedRowSize = 0.01f;
constexpr double treeViewMaxFrameGapSeconds = 1.0 / 15.0;
constexpr double treeViewHoverExpandSeconds = 0.6;
constexpr float treeViewMaxAutoScrollSpeed = 800.0f;
constexpr float treeViewMaxAnimatedViewports = 2.0f;

float treeViewEaseOut (double progress)
{
    const auto remaining = 1.0 - jlimit (0.0, 1.0, progress);
    return static_cast<float> (1.0 - remaining * remaining);
}

double treeViewInverseEaseOut (float eased)
{
    return 1.0 - std::sqrt (jlimit (0.0, 1.0, 1.0 - static_cast<double> (eased)));
}

} // namespace

//==============================================================================
/** Feeds the visible items to the inner list. */
class TreeView::TreeListModel final : public ListBoxModel
{
public:
    explicit TreeListModel (TreeView& owner)
        : owner (owner)
    {
    }

    int getNumRows() override
    {
        return owner.getNumRowsInTree();
    }

    float getRowSize (int rowIndex) override
    {
        return owner.getRowSize (rowIndex);
    }

    void refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component) override
    {
        owner.refreshRowComponent (rowIndex, isSelected, component);
    }

    void selectedRowsChanged (const Array<int>& selectedRows) override
    {
        ignoreUnused (selectedRows);
        owner.handleListSelectionChanged();
    }

    void rowClicked (int rowIndex, const MouseEvent& event) override
    {
        auto* item = owner.getItemOnRow (rowIndex);

        if (item == nullptr)
            return;

        const BailOutChecker checker (&owner);
        item->itemClicked (event);

        if (! checker.shouldBailOut() && owner.getItemOnRow (rowIndex) == item && owner.onItemClicked)
            owner.onItemClicked (*item);
    }

    void rowDoubleClicked (int rowIndex, const MouseEvent& event) override
    {
        auto* item = owner.getItemOnRow (rowIndex);

        if (item == nullptr)
            return;

        const BailOutChecker checker (&owner);
        item->itemDoubleClicked (event);

        if (! checker.shouldBailOut() && owner.getItemOnRow (rowIndex) == item && owner.onItemDoubleClicked)
            owner.onItemDoubleClicked (*item);
    }

    void returnKeyPressed (int currentRow) override
    {
        if (auto* item = owner.getItemOnRow (currentRow); item != nullptr && owner.onReturnKeyPressed)
            owner.onReturnKeyPressed (*item);
    }

    void deleteKeyPressed (const Array<int>& selectedRows) override
    {
        ignoreUnused (selectedRows);

        if (owner.onDeleteKeyPressed)
            owner.onDeleteKeyPressed (owner.getSelectedItems());
    }

    var getDragSourceDescription (const Array<int>& selectedRows) override
    {
        return owner.getDragSourceDescription (selectedRows);
    }

private:
    TreeView& owner;
};

//==============================================================================
/** The inner list: key events do not propagate, so this is where the tree gets them first. */
class TreeView::TreeListBox final : public ListBox
{
public:
    explicit TreeListBox (TreeView& owner)
        : ListBox ("treeViewList")
        , owner (owner)
    {
    }

    void keyDown (const KeyPress& key, const Point<float>& position) override
    {
        owner.finishAnimation();

        if (! owner.handleKeyPress (key))
            ListBox::keyDown (key, position);
    }

    void mouseMove (const MouseEvent& event) override
    {
        ListBox::mouseMove (event);
        owner.updateHoveredItem (event, false);
    }

    void mouseEnter (const MouseEvent& event) override
    {
        ListBox::mouseEnter (event);
        owner.updateHoveredItem (event, false);
    }

    void mouseExit (const MouseEvent& event) override
    {
        ListBox::mouseExit (event);

        // The list also loses the mouse to the rows' own interactive children, still over their item.
        owner.updateHoveredItem (event, ! getLocalBounds().contains (event.getPosition()));
    }

    void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override
    {
        ListBox::mouseWheel (event, wheelData);

        // The content moved under a still pointer.
        owner.updateHoveredItem (event, false);
    }

    void mouseDown (const MouseEvent& event) override
    {
        if (! owner.animation.has_value())
        {
            ListBox::mouseDown (event);
            return;
        }

        // Finishing the animation moves the rows, so the press goes to the item that was under it.
        auto* pressedItem = owner.getItemAt (event.getPosition());
        owner.finishAnimation();

        if (pressedItem == nullptr)
        {
            ListBox::mouseDown (event);
            return;
        }

        // A row that was shrinking away is gone.
        if (pressedItem->rowIndex < 0)
            return;

        const auto rowBounds = getRowBounds (pressedItem->rowIndex);
        ListBox::mouseDown (event.withPosition ({ event.getPosition().getX(), rowBounds.getCenterY() }));
    }

private:
    TreeView& owner;
};

//==============================================================================
/** Holds back selection notifications, and sends a single one at the end if the selection changed. */
class TreeView::ScopedSelectionBatch
{
public:
    explicit ScopedSelectionBatch (TreeView& owner)
        : owner (owner)
    {
        ++owner.selectionBatchDepth;
    }

    ~ScopedSelectionBatch()
    {
        if (--owner.selectionBatchDepth == 0 && std::exchange (owner.selectionChangePending, false))
            owner.sendSelectionNotifications();
    }

private:
    TreeView& owner;
};

//==============================================================================
TreeView::TreeView (StringRef componentID)
    : Component (componentID)
{
    // The list paints everything, this component only paints the drop indicator over it.
    setOpaque (false);

    // The rows paint the hover of the item under the mouse themselves, see TreeView::Style::itemHoveredColorId.
    setColor (ListBox::Style::hoveredRowBackgroundColorId, Colors::transparentBlack);

    model = std::make_unique<TreeListModel> (*this);

    list = std::make_unique<TreeListBox> (*this);
    list->setRowSize (defaultItemHeight);
    list->setModel (model.get());
    addAndMakeVisible (*list);
}

TreeView::~TreeView()
{
    // The rows and their custom content go first, and the items must not reach a view that is going away.
    list->setModel (nullptr);
    rows.clear();

    if (rootItem != nullptr)
        rootItem->setOwnerViewRecursively (nullptr);
}

//==============================================================================
void TreeView::setRootItem (std::unique_ptr<TreeViewItem> newRootItem)
{
    // The root cannot be a sub-item of something else.
    jassert (newRootItem == nullptr || newRootItem->getParentItem() == nullptr);

    finishAnimation();
    clearDragState();

    hoveredItem = nullptr;
    notifyingHoverItem = nullptr;

    // The previous items stay alive until the new rows are in, so their selection callbacks can run.
    auto previousRootItem = std::move (rootItem);

    if (previousRootItem != nullptr)
        previousRootItem->setOwnerViewRecursively (nullptr);

    rootItem = std::move (newRootItem);

    if (rootItem != nullptr)
        rootItem->setOwnerViewRecursively (this);

    ++bulkUpdateDepth;
    ensureHiddenRootIsOpen();
    --bulkUpdateDepth;

    rebuildRows ({}, nullptr);
}

TreeViewItem* TreeView::getRootItem() const noexcept
{
    return rootItem.get();
}

void TreeView::setRootItemVisible (bool shouldBeVisible)
{
    if (rootVisible == shouldBeVisible)
        return;

    beginBulkUpdate();
    rootVisible = shouldBeVisible;
    ensureHiddenRootIsOpen();
    endBulkUpdate();
}

bool TreeView::isRootItemVisible() const noexcept
{
    return rootVisible;
}

void TreeView::setOpenCloseButtonsVisible (bool shouldBeVisible)
{
    if (openCloseButtonsVisible == shouldBeVisible)
        return;

    openCloseButtonsVisible = shouldBeVisible;
    list->updateContent();
}

bool TreeView::areOpenCloseButtonsVisible() const noexcept
{
    return openCloseButtonsVisible;
}

//==============================================================================
void TreeView::setIndentSize (float newIndentSize)
{
    indentSize = jmax (0.0f, newIndentSize);
    list->updateContent();
}

float TreeView::getIndentSize() const noexcept
{
    return indentSize;
}

void TreeView::setDefaultItemHeight (float newHeight)
{
    defaultItemHeight = jmax (1.0f, newHeight);
    list->setRowSize (defaultItemHeight);
    list->updateContent();
}

float TreeView::getDefaultItemHeight() const noexcept
{
    return defaultItemHeight;
}

void TreeView::setIndentGuidesVisible (bool shouldBeVisible)
{
    indentGuidesVisible = shouldBeVisible;
    list->repaint();
}

bool TreeView::areIndentGuidesVisible() const noexcept
{
    return indentGuidesVisible;
}

void TreeView::setExpandAnimationTime (double seconds)
{
    expandAnimationTime = jmax (0.0, seconds);

    if (expandAnimationTime <= 0.0)
        finishAnimation();
}

double TreeView::getExpandAnimationTime() const noexcept
{
    return expandAnimationTime;
}

//==============================================================================
void TreeView::setVerticalScrollBarVisibility (ScrollBar::VisibilityMode mode)
{
    list->setVerticalScrollBarVisibility (mode);
}

void TreeView::setScrollOptions (const KineticScroller::Options& newOptions)
{
    list->setScrollOptions (newOptions);
}

const KineticScroller::Options& TreeView::getScrollOptions() const noexcept
{
    return list->getScrollOptions();
}

void TreeView::setScrollPosition (float newPosition, bool animated)
{
    list->setScrollPosition (newPosition, animated);
}

float TreeView::getScrollPosition() const noexcept
{
    return list->getScrollPosition();
}

//==============================================================================
void TreeView::setSelectionMode (ListBox::SelectionMode mode)
{
    list->setSelectionMode (mode);

    // The list drops the selection without telling its model when selection is turned off.
    handleListSelectionChanged();
}

ListBox::SelectionMode TreeView::getSelectionMode() const noexcept
{
    return list->getSelectionMode();
}

int TreeView::getNumSelectedItems() const
{
    return static_cast<int> (getSelectedItems().size());
}

std::vector<TreeViewItem*> TreeView::getSelectedItems() const
{
    if (bulkUpdateDepth > 0)
        return bulkSelection;

    std::vector<TreeViewItem*> items;

    for (const auto row : list->getSelectedRows())
    {
        if (auto* item = getItemOnRow (row))
            items.push_back (item);
    }

    return items;
}

void TreeView::clearSelectedItems()
{
    if (bulkUpdateDepth > 0)
    {
        bulkSelection.clear();
        return;
    }

    list->deselectAllRows();
}

//==============================================================================
int TreeView::getNumRowsInTree() const noexcept
{
    return static_cast<int> (rows.size());
}

TreeViewItem* TreeView::getItemOnRow (int rowIndex) const noexcept
{
    return isPositiveAndBelow (rowIndex, getNumRowsInTree()) ? rows[static_cast<size_t> (rowIndex)] : nullptr;
}

int TreeView::getRowOf (const TreeViewItem& item) const noexcept
{
    return item.ownerView == this ? item.rowIndex : -1;
}

TreeViewItem* TreeView::getItemAt (Point<float> position) const
{
    // The list fills this component, so positions need no conversion.
    return getItemOnRow (list->getRowAt (position));
}

TreeViewItem* TreeView::getHoveredItem() const noexcept
{
    return hoveredItem;
}

Rectangle<float> TreeView::getItemBounds (const TreeViewItem& item) const
{
    const auto rowIndex = getRowOf (item);

    if (rowIndex < 0)
        return {};

    return list->getRowBounds (rowIndex);
}

void TreeView::scrollToItem (const TreeViewItem& item, ListBox::ScrollAlignment alignment, bool animated)
{
    if (const auto rowIndex = getRowOf (item); rowIndex >= 0)
        list->scrollToRow (rowIndex, alignment, animated);
}

//==============================================================================
DataTree TreeView::getOpennessState (bool includeScrollPosition) const
{
    std::function<DataTree (const TreeViewItem&)> capture = [&capture] (const TreeViewItem& item) -> DataTree
    {
        std::vector<DataTree> children;

        for (const auto& subItem : item.subItems)
        {
            if (auto child = capture (*subItem); child.isValid())
                children.push_back (child);
        }

        const auto selected = item.isSelected();

        if (! item.open && ! selected && children.empty())
            return {};

        DataTree node ("Item");

        {
            auto transaction = node.beginTransaction();
            transaction.setProperty ("id", item.getUniqueName());
            transaction.setProperty ("open", item.open);

            if (selected)
                transaction.setProperty ("selected", true);

            for (const auto& child : children)
                transaction.addChild (child);
        }

        return node;
    };

    DataTree state ("TreeViewState");

    {
        auto transaction = state.beginTransaction();

        if (includeScrollPosition)
            transaction.setProperty ("scrollPosition", static_cast<double> (getScrollPosition()));

        if (rootItem != nullptr)
        {
            if (auto rootNode = capture (*rootItem); rootNode.isValid())
                transaction.addChild (rootNode);
        }
    }

    return state;
}

void TreeView::restoreOpennessState (const DataTree& state, bool restoreSelection)
{
    if (rootItem == nullptr)
        return;

    const auto rootNode = state.getChildWithName ("Item");

    if (! rootNode.isValid() || rootNode.getProperty ("id").toString() != rootItem->getUniqueName())
        return;

    std::vector<TreeViewItem*> selection;

    std::function<void (TreeViewItem&, const DataTree&)> restore = [&] (TreeViewItem& item, const DataTree& node)
    {
        item.setOpen (static_cast<bool> (node.getProperty ("open", false)));

        if (static_cast<bool> (node.getProperty ("selected", false)))
            selection.push_back (&item);

        // Opening may have created the sub-items, so they are only listed now.
        for (int index = 0; index < item.getNumSubItems(); ++index)
        {
            auto& subItem = *item.subItems[static_cast<size_t> (index)];
            const auto name = subItem.getUniqueName();
            const auto childNode = node.findChild ([&name] (const DataTree& child)
            {
                return child.getProperty ("id").toString() == name;
            });

            if (childNode.isValid())
                restore (subItem, childNode);
            else
                subItem.setOpen (false);
        }
    };

    beginBulkUpdate();
    restore (*rootItem, rootNode);

    if (restoreSelection)
    {
        bulkSelection = selection;
        bulkCurrentItem = selection.empty() ? nullptr : selection.back();
    }

    endBulkUpdate();

    if (state.hasProperty ("scrollPosition"))
        list->setScrollPosition (static_cast<float> (static_cast<double> (state.getProperty ("scrollPosition"))));
}

//==============================================================================
std::vector<TreeViewItem*> TreeView::getDraggedItems (const DragAndDropSourceDetails& details)
{
    auto* source = details.sourceComponent.get();

    if (source == nullptr)
    {
        if (auto* manager = DragAndDropManager::getInstanceWithoutCreating(); manager != nullptr && manager->isDragging())
            source = manager->getCurrentDragSourceComponent();
    }

    auto* view = source != nullptr ? dynamic_cast<TreeView*> (source->getParentComponent()) : nullptr;

    if (view == nullptr || source != view->list.get())
        return {};

    const auto selected = view->getSelectedItems();
    std::vector<TreeViewItem*> items;

    // An item travels with its parent, so it is not dragged on its own.
    for (auto* item : selected)
    {
        const auto hasDraggedParent = std::any_of (selected.begin(), selected.end(), [item] (const TreeViewItem* other)
        {
            return other != item && item->isEqualToOrDescendantOf (*other);
        });

        if (! hasDraggedParent)
            items.push_back (item);
    }

    return items;
}

//==============================================================================
void TreeView::resized()
{
    list->setBounds (getLocalBounds());
}

void TreeView::paintOverChildren (Graphics& g)
{
    if (! dropTarget.has_value())
        return;

    const auto color = ApplicationTheme::findComponentColor (*this, Style::dropIndicatorColorId)
                           .value_or (ApplicationTheme::getGlobalTheme()->getPalette().getColor (ThemePalette::Role::accent));

    if (dropTarget->into)
    {
        g.setStrokeColor (color);
        g.setStrokeWidth (2.0f);
        g.strokeRect (dropTarget->indicator.reduced (1.0f));
    }
    else
    {
        g.setFillColor (color);
        g.fillRect (dropTarget->indicator);
    }
}

void TreeView::refreshDisplay (double lastFrameTimeSeconds)
{
    const auto deltaSeconds = jlimit (0.0, treeViewMaxFrameGapSeconds, lastFrameTimeSeconds);

    if (animation.has_value())
    {
        animation->progress += deltaSeconds / expandAnimationTime;

        if (animation->progress >= 1.0)
        {
            finishAnimation();
        }
        else
        {
            list->rowsChanged (animation->firstRow, animation->numRows);
            list->repaintRow (animation->item->rowIndex);
        }
    }

    if (! activeDrag.has_value())
        return;

    if (autoScrollSpeed != 0.0f)
    {
        list->setScrollPosition (list->getScrollPosition() + autoScrollSpeed * static_cast<float> (deltaSeconds));
        updateDragState (*activeDrag);
    }

    if (hoverExpandItem != nullptr)
    {
        hoverExpandSeconds += deltaSeconds;

        if (hoverExpandSeconds >= treeViewHoverExpandSeconds)
        {
            std::exchange (hoverExpandItem, nullptr)->setOpen (true);

            if (activeDrag.has_value())
                updateDragState (*activeDrag);
        }
    }
}

void TreeView::keyDown (const KeyPress& key, const Point<float>& position)
{
    list->keyDown (key, position);
}

//==============================================================================
bool TreeView::isInterestedInDragSource (const DragAndDropSourceDetails& details)
{
    // Asked again for every event of the drag, so where the content may land is decided per item in itemDragMove().
    ignoreUnused (details);
    return rootItem != nullptr;
}

void TreeView::itemDragEnter (const DragAndDropSourceDetails& details)
{
    updateDragState (details);
}

void TreeView::itemDragMove (const DragAndDropSourceDetails& details)
{
    updateDragState (details);
}

void TreeView::itemDragExit (const DragAndDropSourceDetails& details)
{
    ignoreUnused (details);
    clearDragState();
}

bool TreeView::itemDropped (const DragAndDropSourceDetails& details)
{
    finishAnimation();

    const auto target = findDropTarget (details);
    clearDragState();

    if (! target.has_value())
        return false;

    target->parent->itemDropped (details, target->insertIndex);
    return true;
}

//==============================================================================
void TreeView::subItemAdded (TreeViewItem& parent, int index)
{
    if (bulkUpdateDepth > 0)
        return;

    finishAnimation();

    if (areSubItemsShown (parent))
    {
        std::vector<TreeViewItem*> run;
        appendRun (*parent.subItems[static_cast<size_t> (index)], run);
        insertRows (getRowForSubItem (parent, index), run);
    }

    refreshItem (parent);
    jassert (isRowListInSync());
}

void TreeView::subItemRemoving (TreeViewItem& item)
{
    forgetItems (item);

    if (bulkUpdateDepth > 0)
        return;

    finishAnimation();

    if (item.rowIndex >= 0)
        removeRows (item.rowIndex, getRunLength (item));
}

void TreeView::subItemsClearing (TreeViewItem& parent)
{
    for (const auto& subItem : parent.subItems)
        forgetItems (*subItem);

    if (bulkUpdateDepth > 0)
        return;

    finishAnimation();

    if (! areSubItemsShown (parent))
        return;

    int count = 0;

    for (const auto& subItem : parent.subItems)
        count += getRunLength (*subItem);

    removeRows (getRowForSubItem (parent, 0), count);
}

void TreeView::subItemsChanged (TreeViewItem& parent)
{
    if (bulkUpdateDepth > 0)
        return;

    refreshItem (parent);
    jassert (isRowListInSync());
}

void TreeView::subItemMoved (TreeViewItem& parent, int newIndex)
{
    if (bulkUpdateDepth > 0)
        return;

    finishAnimation();

    auto& item = *parent.subItems[static_cast<size_t> (newIndex)];

    if (item.rowIndex < 0)
        return;

    const auto oldRow = item.rowIndex;
    const auto length = getRunLength (item);

    if (length == 1)
    {
        rows.erase (rows.begin() + oldRow);
        renumberRows (oldRow);

        const auto newRow = getRowForSubItem (parent, newIndex);
        rows.insert (rows.begin() + newRow, &item);
        renumberRows (jmin (oldRow, newRow));

        list->rowMoved (oldRow, newRow);
        jassert (isRowListInSync());
        return;
    }

    // A whole branch moves as a removal and an insertion, after which the selection is put back on the same items.
    const ScopedSelectionBatch batch (*this);
    const auto selection = getSelectedItems();
    auto* currentItem = getItemOnRow (list->getCurrentRow());
    const std::vector<TreeViewItem*> run (rows.begin() + oldRow, rows.begin() + oldRow + length);

    removeRows (oldRow, length);
    insertRows (getRowForSubItem (parent, newIndex), run);

    Array<int> selectedRows;

    for (auto* selectedItem : selection)
        selectedRows.add (selectedItem->rowIndex);

    list->setSelectedRows (selectedRows);
    list->setCurrentRow (currentItem != nullptr ? currentItem->rowIndex : -1);

    jassert (isRowListInSync());
}

void TreeView::setItemOpen (TreeViewItem& item, bool shouldBeOpen)
{
    // A hidden root always shows its sub-items, so only its flag changes.
    if (bulkUpdateDepth > 0 || (&item == rootItem.get() && ! rootVisible))
    {
        item.open = shouldBeOpen;
        item.itemOpennessChanged (shouldBeOpen);
        return;
    }

    if (shouldBeOpen)
        expandItem (item);
    else
        collapseItem (item);
}

bool TreeView::isItemSelected (const TreeViewItem& item) const
{
    if (bulkUpdateDepth > 0)
        return std::find (bulkSelection.begin(), bulkSelection.end(), &item) != bulkSelection.end();

    return item.rowIndex >= 0 && list->isRowSelected (item.rowIndex);
}

void TreeView::setItemSelected (TreeViewItem& item, bool shouldBeSelected, bool deselectOthers)
{
    if (bulkUpdateDepth > 0)
    {
        bulkSelection.erase (std::remove (bulkSelection.begin(), bulkSelection.end(), &item), bulkSelection.end());

        if (shouldBeSelected)
        {
            if (deselectOthers)
                bulkSelection.clear();

            bulkSelection.push_back (&item);
        }

        return;
    }

    finishAnimation();

    if (item.rowIndex < 0)
        return;

    if (! shouldBeSelected)
        list->deselectRow (item.rowIndex);
    else if (deselectOthers)
        list->setSelectedRows ({ item.rowIndex });
    else
        list->selectRow (item.rowIndex, false);
}

void TreeView::refreshItem (const TreeViewItem& item)
{
    if (bulkUpdateDepth == 0 && getRowOf (item) >= 0)
        list->rowsChanged (item.rowIndex, 1);
}

void TreeView::repaintItem (const TreeViewItem& item)
{
    if (bulkUpdateDepth == 0 && getRowOf (item) >= 0)
        list->repaintRow (item.rowIndex);
}

void TreeView::beginBulkUpdate()
{
    if (bulkUpdateDepth == 0)
    {
        finishAnimation();

        // From here the rows are stale until the rebuild at the end, so the selection is kept by item.
        bulkSelection = getSelectedItems();
        bulkCurrentItem = getItemOnRow (list->getCurrentRow());
    }

    ++bulkUpdateDepth;
}

void TreeView::endBulkUpdate()
{
    jassert (bulkUpdateDepth > 0);

    if (--bulkUpdateDepth > 0)
        return;

    const auto selection = std::exchange (bulkSelection, {});
    auto* currentItem = std::exchange (bulkCurrentItem, nullptr);

    rebuildRows (selection, currentItem);

    if (activeDrag.has_value())
        updateDragState (*activeDrag);
}

//==============================================================================
float TreeView::getOpenFraction (const TreeViewItem& item) const
{
    if (animation.has_value() && animation->item == &item)
        return getRevealFraction();

    return item.open ? 1.0f : 0.0f;
}

float TreeView::getFullItemHeight (const TreeViewItem& item) const
{
    const auto height = item.getItemHeight();
    return height > 0.0f ? height : defaultItemHeight;
}

//==============================================================================
float TreeView::getRowSize (int rowIndex) const
{
    auto* item = getItemOnRow (rowIndex);

    if (item == nullptr)
        return defaultItemHeight;

    const auto height = getFullItemHeight (*item);

    if (animation.has_value() && rowIndex >= animation->firstRow && rowIndex < animation->firstRow + animation->numRows)
        return jmax (treeViewMinimumAnimatedRowSize, height * getRevealFraction());

    return height;
}

void TreeView::refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component)
{
    auto* item = getItemOnRow (rowIndex);

    if (item == nullptr)
        return;

    auto& row = ListBoxModel::reuseOrCreate<TreeViewRow> (component);
    row.update (*this, *item, getDisplayDepth (*item), isSelected);
}

void TreeView::handleListSelectionChanged()
{
    if (selectionBatchDepth > 0)
    {
        selectionChangePending = true;
        return;
    }

    sendSelectionNotifications();
}

var TreeView::getDragSourceDescription (const Array<int>& selectedRows) const
{
    Array<var> descriptions;

    for (const auto rowIndex : selectedRows)
    {
        auto* item = getItemOnRow (rowIndex);

        if (item == nullptr)
            return {};

        auto description = item->getDragSourceDescription();

        // One undraggable item makes the whole selection undraggable.
        if (description.isVoid())
            return {};

        descriptions.add (description);
    }

    if (descriptions.isEmpty())
        return {};

    return descriptions.size() == 1 ? descriptions.getFirst() : var (descriptions);
}

bool TreeView::handleKeyPress (const KeyPress& key)
{
    const auto keyCode = key.getKey();

    if (keyCode != KeyPress::rightKey && keyCode != KeyPress::leftKey)
        return false;

    auto* item = getItemOnRow (list->getCurrentRow());

    if (item == nullptr)
        return true;

    const bool opening = keyCode == KeyPress::rightKey;

    if (key.getModifiers().isAltDown())
    {
        item->setOpenRecursively (opening);
        return true;
    }

    if (opening)
    {
        if (! item->mightContainSubItems())
            return true;

        if (! item->isOpen())
            item->setOpen (true);
        else if (item->getNumSubItems() > 0)
            selectOnlyRow (item->rowIndex + 1);

        return true;
    }

    if (item->isOpen() && item->mightContainSubItems())
        item->setOpen (false);
    else if (auto* parent = item->getParentItem(); parent != nullptr && parent->rowIndex >= 0)
        selectOnlyRow (parent->rowIndex);

    return true;
}

//==============================================================================
bool TreeView::isDisplayedOpen (const TreeViewItem& item) const noexcept
{
    return item.open || (&item == rootItem.get() && ! rootVisible);
}

bool TreeView::areSubItemsShown (const TreeViewItem& item) const noexcept
{
    if (&item == openingItem || ! isDisplayedOpen (item))
        return false;

    return item.rowIndex >= 0 || (&item == rootItem.get() && ! rootVisible);
}

int TreeView::getDisplayDepth (const TreeViewItem& item) const noexcept
{
    return item.getDepth() - (rootVisible ? 0 : 1);
}

void TreeView::appendRun (TreeViewItem& item, std::vector<TreeViewItem*>& run) const
{
    run.push_back (&item);

    if (&item == openingItem || ! isDisplayedOpen (item))
        return;

    for (const auto& subItem : item.subItems)
        appendRun (*subItem, run);
}

int TreeView::getRunLength (const TreeViewItem& item) const
{
    int length = 1;

    if (&item == openingItem || ! isDisplayedOpen (item))
        return length;

    for (const auto& subItem : item.subItems)
        length += getRunLength (*subItem);

    return length;
}

int TreeView::getRowForSubItem (const TreeViewItem& parent, int index) const
{
    // A hidden root has no row, so its first sub-item lands on row 0.
    if (index == 0)
        return parent.rowIndex + 1;

    const auto& previous = *parent.subItems[static_cast<size_t> (index - 1)];
    return previous.rowIndex + getRunLength (previous);
}

void TreeView::renumberRows (int fromRow)
{
    for (auto row = static_cast<size_t> (jmax (0, fromRow)); row < rows.size(); ++row)
        rows[row]->rowIndex = static_cast<int> (row);
}

void TreeView::insertRows (int firstRow, const std::vector<TreeViewItem*>& run)
{
    if (run.empty())
        return;

    rows.insert (rows.begin() + firstRow, run.begin(), run.end());
    renumberRows (firstRow);

    list->rowsInserted (firstRow, static_cast<int> (run.size()));
}

void TreeView::removeRows (int firstRow, int count)
{
    if (count <= 0)
        return;

    for (int row = firstRow; row < firstRow + count; ++row)
        rows[static_cast<size_t> (row)]->rowIndex = -1;

    rows.erase (rows.begin() + firstRow, rows.begin() + firstRow + count);
    renumberRows (firstRow);

    list->rowsRemoved (firstRow, count);
}

void TreeView::rebuildRows (const std::vector<TreeViewItem*>& itemsToSelect, TreeViewItem* itemToMakeCurrent)
{
    // The previous rows may point to deleted items, so only the current tree is visited.
    std::function<void (TreeViewItem&)> forgetRows = [&forgetRows] (TreeViewItem& item)
    {
        item.rowIndex = -1;

        for (auto& subItem : item.subItems)
            forgetRows (*subItem);
    };

    rows.clear();

    if (rootItem != nullptr)
    {
        forgetRows (*rootItem);

        if (rootVisible)
        {
            appendRun (*rootItem, rows);
        }
        else
        {
            for (auto& subItem : rootItem->subItems)
                appendRun (*subItem, rows);
        }
    }

    renumberRows (0);

    const ScopedSelectionBatch batch (*this);

    list->updateContent();

    Array<int> selectedRows;

    for (auto* item : itemsToSelect)
    {
        if (item->rowIndex >= 0)
            selectedRows.add (item->rowIndex);
    }

    list->setSelectedRows (selectedRows);
    list->setCurrentRow (itemToMakeCurrent != nullptr ? itemToMakeCurrent->rowIndex : -1);

    // Rows may now show other items even where the list saw no change, so the selection is compared by item.
    selectionChangePending = true;

    jassert (isRowListInSync());
}

void TreeView::ensureHiddenRootIsOpen()
{
    if (rootItem == nullptr || rootVisible || rootItem->open)
        return;

    rootItem->open = true;
    rootItem->itemOpennessChanged (true);
}

void TreeView::forgetItems (const TreeViewItem& subtree)
{
    const auto isInside = [&subtree] (const TreeViewItem* item)
    {
        return item != nullptr && item->isEqualToOrDescendantOf (subtree);
    };

    if (bulkUpdateDepth > 0)
    {
        // The rows are only rebuilt at the end, and these items may be deleted before then.
        std::function<void (const TreeViewItem&)> clearRows = [this, &clearRows] (const TreeViewItem& item)
        {
            if (isPositiveAndBelow (item.rowIndex, getNumRowsInTree()))
                rows[static_cast<size_t> (item.rowIndex)] = nullptr;

            for (const auto& subItem : item.subItems)
                clearRows (*subItem);
        };

        clearRows (subtree);

        notifiedSelection.erase (std::remove_if (notifiedSelection.begin(), notifiedSelection.end(), isInside), notifiedSelection.end());
        bulkSelection.erase (std::remove_if (bulkSelection.begin(), bulkSelection.end(), isInside), bulkSelection.end());

        if (isInside (bulkCurrentItem))
            bulkCurrentItem = nullptr;
    }

    if (isInside (hoverExpandItem))
        hoverExpandItem = nullptr;

    if (isInside (hoveredItem))
        hoveredItem = nullptr;

    if (isInside (notifyingHoverItem))
        notifyingHoverItem = nullptr;

    if (dropTarget.has_value() && isInside (dropTarget->parent))
    {
        dropTarget.reset();
        repaint();
    }
}

bool TreeView::isRowListInSync() const
{
    int expected = 0;

    if (rootItem != nullptr)
    {
        if (rootVisible)
        {
            expected = getRunLength (*rootItem);
        }
        else
        {
            for (const auto& subItem : rootItem->subItems)
                expected += getRunLength (*subItem);
        }
    }

    // Rows shrinking away are still there while their item is already closed.
    if (animation.has_value() && ! animation->expanding)
        expected += animation->numRows;

    if (expected != getNumRowsInTree())
        return false;

    for (size_t row = 0; row < rows.size(); ++row)
    {
        if (rows[row] == nullptr || rows[row]->rowIndex != static_cast<int> (row) || rows[row]->ownerView != this)
            return false;
    }

    return true;
}

//==============================================================================
void TreeView::expandItem (TreeViewItem& item)
{
    if (animation.has_value() && animation->item == &item && ! animation->expanding)
    {
        item.open = true;
        reverseAnimation();
        refreshItem (item);
        item.itemOpennessChanged (true);
        return;
    }

    finishAnimation();
    item.open = true;

    {
        // Sub-items created by the callback are not in the rows yet, they are inserted below with the rest.
        // An item opened by an outer callback is inside that item's pending run, which stays the one guarded.
        const auto insideOpeningItem = openingItem != nullptr && item.isEqualToOrDescendantOf (*openingItem);
        const ScopedValueSetter<TreeViewItem*> opening (openingItem, insideOpeningItem ? openingItem : &item);
        item.itemOpennessChanged (true);
    }

    if (item.open && areSubItemsShown (item))
    {
        std::vector<TreeViewItem*> run;

        for (const auto& subItem : item.subItems)
            appendRun (*subItem, run);

        const auto numRows = static_cast<int> (run.size());

        if (shouldAnimate (item, numRows))
            animation = ExpandAnimation { &item, item.rowIndex + 1, numRows, true, 0.0 };

        insertRows (item.rowIndex + 1, run);
    }

    refreshItem (item);
    jassert (isRowListInSync());
}

void TreeView::collapseItem (TreeViewItem& item)
{
    if (animation.has_value() && animation->item == &item && animation->expanding)
    {
        item.open = false;
        reverseAnimation();

        {
            const ScopedSelectionBatch batch (*this);
            moveSelectionToItem (item, animation->firstRow, animation->numRows);
            refreshItem (item);
        }

        item.itemOpennessChanged (false);
        return;
    }

    finishAnimation();

    if (areSubItemsShown (item))
    {
        const auto firstRow = item.rowIndex + 1;
        int count = 0;

        for (const auto& subItem : item.subItems)
            count += getRunLength (*subItem);

        item.open = false;

        const ScopedSelectionBatch batch (*this);

        if (count > 0)
        {
            moveSelectionToItem (item, firstRow, count);

            if (shouldAnimate (item, count))
                animation = ExpandAnimation { &item, firstRow, count, false, 0.0 };
            else
                removeRows (firstRow, count);
        }

        refreshItem (item);
    }
    else
    {
        item.open = false;
        refreshItem (item);
    }

    jassert (isRowListInSync());
    item.itemOpennessChanged (false);
}

void TreeView::moveSelectionToItem (const TreeViewItem& item, int firstRow, int count)
{
    const auto isInRun = [firstRow, count] (int row)
    {
        return row >= firstRow && row < firstRow + count;
    };

    Array<int> selectedRows;
    bool selectionInRun = false;

    for (const auto row : list->getSelectedRows())
    {
        if (isInRun (row))
            selectionInRun = true;
        else
            selectedRows.add (row);
    }

    if (selectionInRun)
    {
        selectedRows.add (item.rowIndex);
        list->setSelectedRows (selectedRows);
        list->setCurrentRow (item.rowIndex);
    }
    else if (isInRun (list->getCurrentRow()))
    {
        list->setCurrentRow (item.rowIndex);
    }
}

void TreeView::selectOnlyRow (int rowIndex)
{
    if (list->getSelectionMode() != ListBox::SelectionMode::none)
        list->setSelectedRows ({ rowIndex });

    list->setCurrentRow (rowIndex);
    list->scrollToRow (rowIndex);
}

//==============================================================================
bool TreeView::shouldAnimate (const TreeViewItem& item, int numRows) const
{
    if (expandAnimationTime <= 0.0 || item.rowIndex < 0 || ! list->getVisibleRowRange().contains (item.rowIndex))
        return false;

    const auto maxRows = jmax (1, roundToInt (treeViewMaxAnimatedViewports * getHeight() / defaultItemHeight));
    return numRows <= maxRows;
}

float TreeView::getRevealFraction() const
{
    if (! animation.has_value())
        return 1.0f;

    const auto eased = treeViewEaseOut (animation->progress);
    return animation->expanding ? eased : 1.0f - eased;
}

void TreeView::reverseAnimation()
{
    const auto reveal = getRevealFraction();

    animation->expanding = ! animation->expanding;
    animation->progress = treeViewInverseEaseOut (animation->expanding ? reveal : 1.0f - reveal);
}

void TreeView::finishAnimation()
{
    if (! animation.has_value())
        return;

    const auto finished = *animation;
    animation.reset();

    if (finished.expanding)
        list->rowsChanged (finished.firstRow, finished.numRows);
    else
        removeRows (finished.firstRow, finished.numRows);

    if (finished.item->rowIndex >= 0)
        list->repaintRow (finished.item->rowIndex);

    jassert (isRowListInSync());
}

//==============================================================================
void TreeView::sendSelectionNotifications()
{
    // Copies: the callbacks may change the tree.
    const auto previous = std::exchange (notifiedSelection, getSelectedItems());
    const auto current = notifiedSelection;

    const auto contains = [] (const std::vector<TreeViewItem*>& items, const TreeViewItem* item)
    {
        return std::find (items.begin(), items.end(), item) != items.end();
    };

    const BailOutChecker checker (this);
    bool changed = false;

    for (auto* item : previous)
    {
        if (contains (current, item))
            continue;

        changed = true;
        item->itemSelectionChanged (false);

        if (checker.shouldBailOut())
            return;
    }

    for (auto* item : current)
    {
        if (contains (previous, item))
            continue;

        changed = true;
        item->itemSelectionChanged (true);

        if (checker.shouldBailOut())
            return;
    }

    if (changed && onSelectionChanged)
        onSelectionChanged();
}

//==============================================================================
std::optional<TreeView::DropTarget> TreeView::findDropTarget (const DragAndDropSourceDetails& details) const
{
    if (rootItem == nullptr)
        return {};

    const auto position = details.localPosition;
    const auto width = getWidth();
    auto* rowItem = getItemAt (position);

    DropTarget target;

    if (rowItem == nullptr)
    {
        // Past the last row: append to the root.
        const auto lastRowBottom = rows.empty() ? 0.0f : getItemBounds (*rows.back()).getBottom();
        const auto x = static_cast<float> (getDisplayDepth (*rootItem) + 1) * indentSize;

        target.parent = rootItem.get();
        target.insertIndex = rootItem->getNumSubItems();
        target.indicator = { x, lastRowBottom - 1.0f, jmax (0.0f, width - x), 2.0f };
    }
    else
    {
        enum class Zone
        {
            before,
            after,
            into,
            firstSubItem
        };

        const auto bounds = getItemBounds (*rowItem);
        const auto fraction = (position.getY() - bounds.getY()) / jmax (1.0f, bounds.getHeight());
        auto* parent = rowItem->getParentItem();

        auto zone = Zone::into;

        if (parent == nullptr)
            zone = Zone::into;
        else if (fraction < 0.25f)
            zone = Zone::before;
        else if (fraction > 0.75f)
            zone = Zone::after;
        else if (rowItem->mightContainSubItems())
            zone = Zone::into;
        else
            zone = fraction < 0.5f ? Zone::before : Zone::after;

        // Below an open item, the next row is its first sub-item.
        if (zone == Zone::after && areSubItemsShown (*rowItem) && rowItem->getNumSubItems() > 0)
            zone = Zone::firstSubItem;

        const auto lineAt = [&] (float y, int depth)
        {
            const auto x = static_cast<float> (depth) * indentSize;
            return Rectangle<float> { x, y - 1.0f, jmax (0.0f, width - x), 2.0f };
        };

        const auto depth = getDisplayDepth (*rowItem);

        switch (zone)
        {
            case Zone::into:
                target.parent = rowItem;
                target.insertIndex = rowItem->getNumSubItems();
                target.into = true;
                target.indicator = bounds;
                break;

            case Zone::before:
                target.parent = parent;
                target.insertIndex = rowItem->getIndexInParent();
                target.indicator = lineAt (bounds.getY(), depth);
                break;

            case Zone::after:
                target.parent = parent;
                target.insertIndex = rowItem->getIndexInParent() + 1;
                target.indicator = lineAt (bounds.getBottom(), depth);
                break;

            case Zone::firstSubItem:
                target.parent = rowItem;
                target.insertIndex = 0;
                target.indicator = lineAt (bounds.getBottom(), depth + 1);
                break;
        }
    }

    if (! target.parent->isInterestedInDragSource (details))
        return {};

    // Content dragged from a tree cannot be dropped into itself.
    for (const auto* dragged : getDraggedItems (details))
    {
        if (target.parent->isEqualToOrDescendantOf (*dragged))
            return {};
    }

    return target;
}

void TreeView::updateDragState (const DragAndDropSourceDetails& details)
{
    // Called again with the stored drag while auto-scrolling, which must not be copied onto itself.
    if (! activeDrag.has_value() || &*activeDrag != &details)
        activeDrag = details;

    const auto position = details.localPosition;
    const auto edgeSize = jmax (1.0f, defaultItemHeight);
    const auto height = getHeight();

    if (position.getY() < edgeSize)
        autoScrollSpeed = -treeViewMaxAutoScrollSpeed * jlimit (0.0f, 1.0f, 1.0f - position.getY() / edgeSize);
    else if (position.getY() > height - edgeSize)
        autoScrollSpeed = treeViewMaxAutoScrollSpeed * jlimit (0.0f, 1.0f, 1.0f - (height - position.getY()) / edgeSize);
    else
        autoScrollSpeed = 0.0f;

    auto* item = getItemAt (position);

    if (item != nullptr && (item->isOpen() || ! item->mightContainSubItems()))
        item = nullptr;

    if (item != hoverExpandItem)
    {
        hoverExpandItem = item;
        hoverExpandSeconds = 0.0;
    }

    dropTarget = findDropTarget (details);
    repaint();
}

void TreeView::updateHoveredItem (const MouseEvent& event, bool pointerLeftList)
{
    if (event.isTouch())
        return;

    setHoveredItem (pointerLeftList ? nullptr : getItemAt (event.getPosition()));
}

void TreeView::setHoveredItem (TreeViewItem* newItem)
{
    if (newItem == hoveredItem)
        return;

    auto* previousItem = std::exchange (hoveredItem, newItem);

    if (previousItem != nullptr)
        repaintItem (*previousItem);

    if (newItem != nullptr)
        repaintItem (*newItem);

    const BailOutChecker checker (this);

    // Each notification can change the tree: an item removed meanwhile is not reported any further.
    if (previousItem != nullptr)
    {
        notifyingHoverItem = previousItem;
        previousItem->itemExited();

        if (checker.shouldBailOut())
            return;

        if (std::exchange (notifyingHoverItem, nullptr) != nullptr && onItemExited)
            onItemExited (*previousItem);

        if (checker.shouldBailOut())
            return;
    }

    if (newItem == nullptr || hoveredItem != newItem)
        return;

    notifyingHoverItem = newItem;
    newItem->itemEntered();

    if (checker.shouldBailOut())
        return;

    if (std::exchange (notifyingHoverItem, nullptr) != nullptr && onItemEntered)
        onItemEntered (*newItem);
}

void TreeView::clearDragState()
{
    const auto hadDropTarget = dropTarget.has_value();

    activeDrag.reset();
    dropTarget.reset();
    hoverExpandItem = nullptr;
    hoverExpandSeconds = 0.0;
    autoScrollSpeed = 0.0f;

    if (hadDropTarget)
        repaint();
}

} // namespace yup
