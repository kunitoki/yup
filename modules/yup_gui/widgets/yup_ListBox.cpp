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

//==============================================================================
int ListBoxModel::getRowHeight (int rowIndex)
{
    ignoreUnused (rowIndex);
    return 0;
}

int ListBoxModel::getRowWidth (int rowIndex)
{
    ignoreUnused (rowIndex);
    return 0;
}

Component* ListBoxModel::refreshComponentForRow (int rowIndex, Component* existingComponent)
{
    ignoreUnused (rowIndex, existingComponent);
    return nullptr;
}

void ListBoxModel::paintListBoxItem (int rowIndex, Graphics& g, Rectangle<float> area, bool isSelected)
{
    ignoreUnused (rowIndex, g, area, isSelected);
}

String ListBoxModel::getRowText (int rowIndex)
{
    ignoreUnused (rowIndex);
    return {};
}

Image ListBoxModel::getRowIcon (int rowIndex)
{
    ignoreUnused (rowIndex);
    return {};
}

void ListBoxModel::selectedRowsChanged (const Array<int>& selectedRows)
{
    ignoreUnused (selectedRows);
}

void ListBoxModel::rowClicked (int rowIndex, const MouseEvent& event)
{
    ignoreUnused (rowIndex, event);
}

void ListBoxModel::rowDoubleClicked (int rowIndex, const MouseEvent& event)
{
    ignoreUnused (rowIndex, event);
}

void ListBoxModel::returnKeyPressed (int lastSelectedRow)
{
    ignoreUnused (lastSelectedRow);
}

void ListBoxModel::deleteKeyPressed (const Array<int>& selectedRows)
{
    ignoreUnused (selectedRows);
}

var ListBoxModel::getDragSourceDescription (const Array<int>& selectedRows)
{
    ignoreUnused (selectedRows);
    return {};
}

//==============================================================================
// Internal ListBoxRow component that wraps user components
class ListBox::ListBoxRow : public Component
{
public:
    ListBoxRow (ListBox& owner, int rowIndex)
        : owner (owner)
        , rowIndex (rowIndex)
    {
        setOpaque (false);
        setWantsMouseEvents (false, true);
    }

    void updateContent (int newRowIndex)
    {
        rowIndex = newRowIndex;

        if (owner.model == nullptr)
        {
            contentComponent.reset();
            return;
        }

        // Ask model for component
        Component* newComponent = owner.model->refreshComponentForRow (rowIndex, contentComponent.get());

        if (newComponent != contentComponent.get())
        {
            // Model returned a new component (or nullptr)
            contentComponent.reset (newComponent);

            if (contentComponent != nullptr)
            {
                addAndMakeVisible (*contentComponent);
                contentComponent->setBounds (getLocalBounds());
            }
        }

        // If model returned nullptr, we need to create a default item
        if (contentComponent == nullptr && ! defaultItem)
        {
            defaultItem = std::make_unique<ListBoxItem>();
            addAndMakeVisible (*defaultItem);
            defaultItem->setBounds (getLocalBounds());
        }

        // Update default item content if it exists
        if (defaultItem != nullptr)
        {
            defaultItem->setText (owner.model->getRowText (rowIndex));
            defaultItem->setIcon (owner.model->getRowIcon (rowIndex));
            defaultItem->setSelected (selected);
            defaultItem->setHovered (hovered);
        }

        repaint();
    }

    void setSelected (bool shouldBeSelected)
    {
        if (selected != shouldBeSelected)
        {
            selected = shouldBeSelected;

            if (defaultItem != nullptr)
                defaultItem->setSelected (selected);

            repaint();
        }
    }

    void setHovered (bool shouldBeHovered)
    {
        if (hovered != shouldBeHovered)
        {
            hovered = shouldBeHovered;

            if (defaultItem != nullptr)
                defaultItem->setHovered (hovered);

            repaint();
        }
    }

    int getRowIndex() const { return rowIndex; }

    Component* getContentComponent() const { return contentComponent.get(); }

    void paint (Graphics& g) override
    {
        // Paint background if using custom component
        if (contentComponent != nullptr)
        {
            Color backgroundColor;

            if (selected)
            {
                backgroundColor = owner.findColor (ListBox::Style::selectedRowBackgroundColorId)
                                      .value_or (Color (0xff3a7ebf));
            }
            else if (hovered)
            {
                backgroundColor = owner.findColor (ListBox::Style::hoveredRowBackgroundColorId)
                                      .value_or (Color (0x22ffffff));
            }
            else
            {
                backgroundColor = owner.findColor (ListBox::Style::rowBackgroundColorId)
                                      .value_or (Color (0x00000000));
            }

            if (backgroundColor.getAlpha() > 0)
            {
                g.setFillColor (backgroundColor);
                g.fillRect (getLocalBounds());
            }
        }
    }

    void resized() override
    {
        if (contentComponent != nullptr)
            contentComponent->setBounds (getLocalBounds());

        if (defaultItem != nullptr)
            defaultItem->setBounds (getLocalBounds());
    }

private:
    ListBox& owner;
    int rowIndex;
    std::unique_ptr<Component> contentComponent;
    std::unique_ptr<ListBoxItem> defaultItem;
    bool selected = false;
    bool hovered = false;
};

//==============================================================================
ListBox::ListBox (StringRef componentID, Orientation orientation)
    : Component (componentID)
    , orientation (orientation)
{
    setWantsKeyboardFocus (true);
    setWantsMouseEvents (true, true);
    setOpaque (true);

    // Create scrollbars
    verticalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::vertical);
    verticalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    verticalScrollBar->onScrollPositionChanged = [this] (double newPosition)
    {
        handleScrollBarMoved();
    };
    addAndMakeVisible (*verticalScrollBar);

    horizontalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::horizontal);
    horizontalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    horizontalScrollBar->onScrollPositionChanged = [this] (double newPosition)
    {
        handleScrollBarMoved();
    };
    addAndMakeVisible (*horizontalScrollBar);
}

ListBox::~ListBox()
{
    setModel (nullptr);
}

//==============================================================================
void ListBox::setModel (ListBoxModel* newModel)
{
    if (model != newModel)
    {
        model = newModel;
        updateContent();
    }
}

ListBoxModel* ListBox::getModel() const noexcept
{
    return model;
}

//==============================================================================
void ListBox::setSelectionMode (SelectionMode mode)
{
    if (selectionMode != mode)
    {
        selectionMode = mode;

        if (selectionMode == SelectionMode::none)
        {
            deselectAllRows (dontSendNotification);
        }
        else if (selectionMode == SelectionMode::single && selectedRows.size() > 1)
        {
            auto firstSelected = selectedRows.getFirst();
            selectedRows.clear();
            selectedRows.add (firstSelected);
            notifySelectionChanged();
        }
    }
}

ListBox::SelectionMode ListBox::getSelectionMode() const noexcept
{
    return selectionMode;
}

//==============================================================================
int ListBox::getSelectedRow() const
{
    return (selectedRows.size() == 1) ? selectedRows.getFirst() : -1;
}

void ListBox::selectRow (int rowIndex, bool scrollToShowRow, NotificationType notification)
{
    if (selectionMode == SelectionMode::none)
        return;

    if (model == nullptr || rowIndex < 0 || rowIndex >= model->getNumRows())
        return;

    if (selectionMode == SelectionMode::single)
    {
        if (selectedRows.size() == 1 && selectedRows.getFirst() == rowIndex)
            return;

        selectedRows.clear();
        selectedRows.add (rowIndex);
        lastSelectedRow = rowIndex;
    }
    else // Multiple selection mode
    {
        if (! selectedRows.contains (rowIndex))
        {
            selectedRows.add (rowIndex);
            std::sort (selectedRows.begin(), selectedRows.end());
            lastSelectedRow = rowIndex;
        }
    }

    if (scrollToShowRow)
        scrollToEnsureRowIsVisible (rowIndex);

    updateVisibleRows();

    if (notification != dontSendNotification)
        notifySelectionChanged();
}

void ListBox::deselectRow (int rowIndex, NotificationType notification)
{
    if (selectedRows.contains (rowIndex))
    {
        selectedRows.removeAllInstancesOf (rowIndex);

        if (lastSelectedRow == rowIndex)
            lastSelectedRow = selectedRows.isEmpty() ? -1 : selectedRows.getLast();

        updateVisibleRows();

        if (notification != dontSendNotification)
            notifySelectionChanged();
    }
}

void ListBox::deselectAllRows (NotificationType notification)
{
    if (! selectedRows.isEmpty())
    {
        selectedRows.clear();
        lastSelectedRow = -1;
        updateVisibleRows();

        if (notification != dontSendNotification)
            notifySelectionChanged();
    }
}

Array<int> ListBox::getSelectedRows() const
{
    return selectedRows;
}

void ListBox::setSelectedRows (const Array<int>& rows, NotificationType notification)
{
    if (selectionMode == SelectionMode::none)
        return;

    selectedRows = rows;

    if (selectionMode == SelectionMode::single && selectedRows.size() > 1)
    {
        auto firstSelected = selectedRows.getFirst();
        selectedRows.clear();
        if (firstSelected >= 0)
            selectedRows.add (firstSelected);
    }

    // Sort for efficient lookup
    std::sort (selectedRows.begin(), selectedRows.end());

    lastSelectedRow = selectedRows.isEmpty() ? -1 : selectedRows.getLast();

    updateVisibleRows();

    if (notification != dontSendNotification)
        notifySelectionChanged();
}

bool ListBox::isRowSelected (int rowIndex) const
{
    return std::binary_search (selectedRows.begin(), selectedRows.end(), rowIndex);
}

int ListBox::getNumSelectedRows() const
{
    return selectedRows.size();
}

//==============================================================================
void ListBox::updateContent()
{
    // Clear all cached components
    rowComponents.clear();
    hoveredRow = -1;

    // Update scrolling calculations
    updateScrolling();
    updateScrollBars();
    updateScrolling();

    // Layout visible rows
    layoutRows();

    repaint();
}

void ListBox::repaintRow (int rowIndex)
{
    if (visibleRowRange.contains (rowIndex))
    {
        auto it = rowComponents.find (rowIndex);
        if (it != rowComponents.end())
        {
            it->second->repaint();
        }
    }
}

void ListBox::scrollToEnsureRowIsVisible (int rowIndex)
{
    if (model == nullptr || rowIndex < 0 || rowIndex >= model->getNumRows())
        return;

    scrollToRow (rowIndex);
}

//==============================================================================
void ListBox::setOrientation (Orientation newOrientation)
{
    if (orientation != newOrientation)
    {
        orientation = newOrientation;
        updateContent();
    }
}

ListBox::Orientation ListBox::getOrientation() const noexcept
{
    return orientation;
}

//==============================================================================
void ListBox::setRowHeight (int newHeight)
{
    if (fixedRowHeight != newHeight)
    {
        fixedRowHeight = jmax (1, newHeight);
        if (! variableHeightEnabled)
            updateContent();
    }
}

void ListBox::setRowWidth (int newWidth)
{
    if (fixedRowWidth != newWidth)
    {
        fixedRowWidth = jmax (1, newWidth);
        if (! variableWidthEnabled)
            updateContent();
    }
}

int ListBox::getRowHeight() const noexcept
{
    return fixedRowHeight;
}

int ListBox::getRowWidth() const noexcept
{
    return fixedRowWidth;
}

//==============================================================================
void ListBox::setVariableHeightEnabled (bool enabled)
{
    if (variableHeightEnabled != enabled)
    {
        variableHeightEnabled = enabled;
        updateContent();
    }
}

void ListBox::setVariableWidthEnabled (bool enabled)
{
    if (variableWidthEnabled != enabled)
    {
        variableWidthEnabled = enabled;
        updateContent();
    }
}

bool ListBox::isVariableHeightEnabled() const noexcept
{
    return variableHeightEnabled;
}

bool ListBox::isVariableWidthEnabled() const noexcept
{
    return variableWidthEnabled;
}

//==============================================================================
void ListBox::setMinimumContentSize (int minSize)
{
    auto newMinimumContentSize = jmax (0, minSize);

    if (minimumContentSize != newMinimumContentSize)
    {
        minimumContentSize = newMinimumContentSize;
        updateContent();
    }
}

int ListBox::getMinimumContentSize() const noexcept
{
    return minimumContentSize;
}

//==============================================================================
int ListBox::getVisibleRowsCount() const
{
    return visibleRowRange.getLength();
}

Range<int> ListBox::getVisibleRowRange() const
{
    return visibleRowRange;
}

//==============================================================================
int ListBox::getRowAt (Point<float> position) const
{
    return getRowIndexAt (position);
}

Component* ListBox::getComponentForRow (int rowIndex) const
{
    auto it = rowComponents.find (rowIndex);
    return (it != rowComponents.end()) ? it->second->getContentComponent() : nullptr;
}

Rectangle<float> ListBox::getRowBounds (int rowIndex) const
{
    if (model == nullptr || rowIndex < 0 || rowIndex >= model->getNumRows())
        return {};

    auto position = getRowPosition (rowIndex);
    auto size = getRowSize (rowIndex);
    auto contentArea = getContentArea();

    if (orientation == Orientation::vertical)
        return Rectangle<float> (contentArea.getX(),
                                 contentArea.getY() + position - scrollOffset,
                                 contentArea.getWidth(),
                                 size);
    else
        return Rectangle<float> (contentArea.getX() + position - scrollOffset,
                                 contentArea.getY(),
                                 size,
                                 contentArea.getHeight());
}

//==============================================================================
void ListBox::paint (Graphics& g)
{
    // Fill background
    auto backgroundColor = findColor (Style::backgroundColorId).value_or (Color (0xffffffff));
    g.setFillColor (backgroundColor);
    g.fillRect (getLocalBounds());

    // Draw outline
    auto outlineColor = findColor (Style::outlineColorId).value_or (Color (0xffcccccc));
    g.setStrokeColor (outlineColor);
    g.setStrokeWidth (1.0f);
    g.strokeRect (getLocalBounds());
}

void ListBox::resized()
{
    // Position scrollbars
    auto bounds = getLocalBounds();

    if (verticalScrollBar != nullptr)
    {
        auto scrollBarWidth = verticalScrollBar->getScrollBarWidth();
        verticalScrollBar->setBounds (bounds.removeFromRight (scrollBarWidth));
    }

    if (horizontalScrollBar != nullptr)
    {
        auto scrollBarHeight = horizontalScrollBar->getScrollBarWidth();
        horizontalScrollBar->setBounds (bounds.removeFromBottom (scrollBarHeight));
    }

    updateScrolling();
    updateScrollBars();
    layoutRows();
}

//==============================================================================
void ListBox::mouseDown (const MouseEvent& event)
{
    takeKeyboardFocus();

    auto rowIndex = getRowIndexAt (event.getPosition());

    if (rowIndex >= 0)
    {
        auto modifiers = event.getModifiers();
        bool isShiftDown = modifiers.isShiftDown();
        bool isCommandDown = modifiers.isCommandDown() || modifiers.isControlDown();

        handleRowSelection (rowIndex, isCommandDown, isShiftDown);
        handleRowClick (rowIndex, event);
    }
}

void ListBox::mouseUp (const MouseEvent& event)
{
    ignoreUnused (event);
}

void ListBox::mouseMove (const MouseEvent& event)
{
    updateHoveredRow (event.getPosition());
}

void ListBox::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (needsScrolling())
    {
        auto delta = (orientation == Orientation::vertical)
                       ? wheelData.getDeltaY()
                       : wheelData.getDeltaX();

        // Scroll by approximately 3 rows worth of content
        auto scrollAmount = delta * fixedRowHeight * 3.0f;
        scrollBy (-scrollAmount);
        updateHoveredRow (event.getPosition());
    }
}

void ListBox::mouseDoubleClick (const MouseEvent& event)
{
    auto rowIndex = getRowIndexAt (event.getPosition());

    if (rowIndex >= 0)
    {
        if (onRowDoubleClicked)
            onRowDoubleClicked (rowIndex);

        if (model != nullptr)
            model->rowDoubleClicked (rowIndex, event);
    }
}

//==============================================================================
void ListBox::keyDown (const KeyPress& key, const Point<float>& position)
{
    ignoreUnused (position);

    if (model == nullptr)
        return;

    auto numRows = model->getNumRows();
    if (numRows == 0)
        return;

    auto modifiers = key.getModifiers();
    bool isShiftDown = modifiers.isShiftDown();
    bool isCommandDown = modifiers.isCommandDown() || modifiers.isControlDown();

    if (key.getKey() == KeyPress::upKey || key.getKey() == KeyPress::downKey)
    {
        int newRow = lastSelectedRow;

        if (key.getKey() == KeyPress::upKey)
            newRow = jmax (0, lastSelectedRow - 1);
        else
            newRow = jmin (numRows - 1, lastSelectedRow + 1);

        if (newRow != lastSelectedRow)
        {
            if (selectionMode == SelectionMode::multiple && (isCommandDown || isShiftDown))
            {
                if (isShiftDown)
                    handleRowSelection (newRow, false, true);
                else
                    lastSelectedRow = newRow;
            }
            else
            {
                selectRow (newRow, true, sendNotification);
            }
        }
    }
    else if (key.getKey() == KeyPress::leftKey || key.getKey() == KeyPress::rightKey)
    {
        if (orientation == Orientation::horizontal)
        {
            int newRow = lastSelectedRow;

            if (key.getKey() == KeyPress::leftKey)
                newRow = jmax (0, lastSelectedRow - 1);
            else
                newRow = jmin (numRows - 1, lastSelectedRow + 1);

            if (newRow != lastSelectedRow)
            {
                if (selectionMode == SelectionMode::multiple && (isCommandDown || isShiftDown))
                {
                    if (isShiftDown)
                        handleRowSelection (newRow, false, true);
                    else
                        lastSelectedRow = newRow;
                }
                else
                {
                    selectRow (newRow, true, sendNotification);
                }
            }
        }
    }
    else if (key.getKey() == KeyPress::pageUpKey)
    {
        auto visibleCount = getVisibleRowsCount();
        auto newRow = jmax (0, lastSelectedRow - visibleCount);
        selectRow (newRow, true, sendNotification);
    }
    else if (key.getKey() == KeyPress::pageDownKey)
    {
        auto visibleCount = getVisibleRowsCount();
        auto newRow = jmin (numRows - 1, lastSelectedRow + visibleCount);
        selectRow (newRow, true, sendNotification);
    }
    else if (key.getKey() == KeyPress::homeKey)
    {
        selectRow (0, true, sendNotification);
    }
    else if (key.getKey() == KeyPress::endKey)
    {
        selectRow (numRows - 1, true, sendNotification);
    }
    else if (key.getKey() == KeyPress::spaceKey)
    {
        if (selectionMode == SelectionMode::multiple && lastSelectedRow >= 0)
        {
            if (isRowSelected (lastSelectedRow))
                deselectRow (lastSelectedRow, sendNotification);
            else
                selectRow (lastSelectedRow, false, sendNotification);
        }
    }
    //else if (key.getKey() == KeyPress::returnKey)
    //{
    //    if (model != nullptr)
    //        model->returnKeyPressed (lastSelectedRow);
    //}
    else if (key.getKey() == KeyPress::deleteKey || key.getKey() == KeyPress::backspaceKey)
    {
        if (model != nullptr)
            model->deleteKeyPressed (selectedRows);
    }
}

void ListBox::focusGained()
{
    repaint();
}

void ListBox::focusLost()
{
    repaint();
}

//==============================================================================
void ListBox::updateVisibleRows()
{
    // Update selection state for visible rows
    for (auto& pair : rowComponents)
    {
        auto rowIndex = pair.first;
        auto& row = pair.second;

        row->setSelected (isRowSelected (rowIndex));
        row->setHovered (hoveredRow == rowIndex);
    }
}

void ListBox::layoutRows()
{
    if (model == nullptr)
    {
        visibleRowRange = Range<int> (0, 0);
        return;
    }

    auto numRows = model->getNumRows();
    if (numRows == 0)
    {
        visibleRowRange = Range<int> (0, 0);
        return;
    }

    // Calculate which rows are visible
    float position = 0.0f;
    int firstVisible = -1;
    int lastVisible = -1;

    for (int i = 0; i < numRows; ++i)
    {
        auto rowSize = getRowSize (i);

        if (position + rowSize > scrollOffset && firstVisible < 0)
            firstVisible = i;

        if (position < scrollOffset + viewportSize)
            lastVisible = i;

        if (position >= scrollOffset + viewportSize)
            break;

        position += rowSize;
    }

    visibleRowRange = Range<int> (firstVisible >= 0 ? firstVisible : 0,
                                  lastVisible >= 0 ? lastVisible + 1 : 0);

    for (auto& pair : rowComponents)
    {
        if (! visibleRowRange.contains (pair.first))
            pair.second->setVisible (false);
    }

    // Create or update visible rows
    for (int i = visibleRowRange.getStart(); i < visibleRowRange.getEnd(); ++i)
    {
        createOrUpdateRow (i);
    }

    // Remove rows outside visible range (if cache too large)
    removeUnusedRows();

    // Position visible row components
    for (int i = visibleRowRange.getStart(); i < visibleRowRange.getEnd(); ++i)
    {
        auto it = rowComponents.find (i);
        if (it != rowComponents.end())
        {
            auto& row = it->second;
            auto rowBounds = getRowBounds (i);

            row->setBounds (rowBounds.toNearestInt());

            if (row->getParentComponent() != this)
                addChildComponent (*row);

            row->setVisible (true);
        }
    }

    if (verticalScrollBar != nullptr)
        verticalScrollBar->toFront (false);

    if (horizontalScrollBar != nullptr)
        horizontalScrollBar->toFront (false);
}

void ListBox::createOrUpdateRow (int rowIndex)
{
    auto it = rowComponents.find (rowIndex);

    if (it != rowComponents.end())
    {
        // Update existing row
        it->second->updateContent (rowIndex);
        it->second->setSelected (isRowSelected (rowIndex));
        it->second->setHovered (hoveredRow == rowIndex);
    }
    else
    {
        // Create new row
        auto row = std::make_unique<ListBoxRow> (*this, rowIndex);
        row->updateContent (rowIndex);
        row->setSelected (isRowSelected (rowIndex));
        row->setHovered (hoveredRow == rowIndex);

        rowComponents[rowIndex] = std::move (row);
    }
}

void ListBox::removeUnusedRows()
{
    if (static_cast<int> (rowComponents.size()) <= maxCachedRows)
        return;

    // Remove rows outside visible range
    std::vector<int> rowsToRemove;

    for (const auto& pair : rowComponents)
    {
        if (! visibleRowRange.contains (pair.first))
            rowsToRemove.push_back (pair.first);
    }

    // Keep only maxCachedRows rows outside visible range
    if (rowsToRemove.size() > static_cast<size_t> (maxCachedRows / 2))
    {
        // Remove oldest rows (keep half the cache size)
        auto numToRemove = rowsToRemove.size() - (maxCachedRows / 2);

        for (size_t i = 0; i < numToRemove; ++i)
        {
            rowComponents.erase (rowsToRemove[i]);
        }
    }
}

//==============================================================================
void ListBox::scrollBy (float delta)
{
    auto oldScrollOffset = scrollOffset;

    scrollOffset += delta;

    // Clamp scroll offset
    auto maxScroll = jmax (0.0f, totalContentSize - viewportSize);
    scrollOffset = jlimit (0.0f, maxScroll, scrollOffset);

    if (oldScrollOffset == scrollOffset)
        return;

    layoutRows();
    updateScrollBars();
    repaint();
}

void ListBox::scrollToRow (int rowIndex)
{
    if (model == nullptr || rowIndex < 0 || rowIndex >= model->getNumRows())
        return;

    auto rowPosition = getRowPosition (rowIndex);
    auto rowSize = getRowSize (rowIndex);

    // Check if row is already fully visible
    if (rowPosition >= scrollOffset && rowPosition + rowSize <= scrollOffset + viewportSize)
        return;

    // Scroll to show the row
    if (rowPosition < scrollOffset)
    {
        scrollOffset = rowPosition;
    }
    else if (rowPosition + rowSize > scrollOffset + viewportSize)
    {
        scrollOffset = rowPosition + rowSize - viewportSize;
    }

    // Clamp scroll offset
    auto maxScroll = jmax (0.0f, totalContentSize - viewportSize);
    scrollOffset = jlimit (0.0f, maxScroll, scrollOffset);

    layoutRows();
    updateScrollBars();
    repaint();
}

void ListBox::updateScrolling()
{
    viewportSize = getViewportSize();
    totalContentSize = getTotalContentSize();

    // Clamp scroll offset to valid range
    auto maxScroll = jmax (0.0f, totalContentSize - viewportSize);
    scrollOffset = jlimit (0.0f, maxScroll, scrollOffset);
}

float ListBox::getRowPosition (int rowIndex) const
{
    if (model == nullptr || rowIndex < 0)
        return 0.0f;

    float position = 0.0f;

    for (int i = 0; i < rowIndex; ++i)
    {
        position += getRowSize (i);
    }

    return position;
}

float ListBox::getRowSize (int rowIndex) const
{
    if (model == nullptr || rowIndex < 0 || rowIndex >= model->getNumRows())
        return 0.0f;

    if (orientation == Orientation::vertical)
    {
        if (variableHeightEnabled)
        {
            auto height = model->getRowHeight (rowIndex);
            return height > 0 ? static_cast<float> (height) : static_cast<float> (fixedRowHeight);
        }
        return static_cast<float> (fixedRowHeight);
    }
    else
    {
        if (variableWidthEnabled)
        {
            auto width = model->getRowWidth (rowIndex);
            return width > 0 ? static_cast<float> (width) : static_cast<float> (fixedRowWidth);
        }
        return static_cast<float> (fixedRowWidth);
    }
}

//==============================================================================
void ListBox::handleRowClick (int rowIndex, const MouseEvent& event)
{
    if (onRowClicked)
        onRowClicked (rowIndex);

    if (model != nullptr)
        model->rowClicked (rowIndex, event);
}

void ListBox::handleRowSelection (int rowIndex, bool shouldToggle, bool shouldExtend)
{
    if (selectionMode == SelectionMode::none)
        return;

    if (selectionMode == SelectionMode::single)
    {
        selectRow (rowIndex, false, sendNotification);
        return;
    }

    // Multiple selection mode
    if (shouldExtend && lastSelectedRow >= 0)
    {
        // Range selection
        auto start = jmin (lastSelectedRow, rowIndex);
        auto end = jmax (lastSelectedRow, rowIndex);

        selectedRows.clear();

        for (int i = start; i <= end; ++i)
        {
            selectedRows.add (i);
        }

        lastSelectedRow = rowIndex;
        notifySelectionChanged();
    }
    else if (shouldToggle)
    {
        // Toggle selection
        if (isRowSelected (rowIndex))
            deselectRow (rowIndex, sendNotification);
        else
            selectRow (rowIndex, false, sendNotification);
    }
    else
    {
        // Replace selection
        selectRow (rowIndex, false, sendNotification);
    }
}

void ListBox::notifySelectionChanged()
{
    if (onSelectionChanged)
        onSelectionChanged();

    if (model != nullptr)
        model->selectedRowsChanged (selectedRows);
}

//==============================================================================
int ListBox::getRowIndexAt (Point<float> position) const
{
    if (model == nullptr)
        return -1;

    auto contentArea = getContentArea();
    if (! contentArea.contains (position))
        return -1;

    auto numRows = model->getNumRows();
    if (numRows == 0)
        return -1;

    float searchPosition = (orientation == Orientation::vertical)
                             ? position.getY() - contentArea.getY() + scrollOffset
                             : position.getX() - contentArea.getX() + scrollOffset;

    float currentPosition = 0.0f;

    for (int i = 0; i < numRows; ++i)
    {
        auto rowSize = getRowSize (i);

        if (searchPosition >= currentPosition && searchPosition < currentPosition + rowSize)
            return i;

        currentPosition += rowSize;
    }

    return -1;
}

void ListBox::updateHoveredRow (Point<float> position)
{
    auto newHoveredRow = getRowIndexAt (position);

    if (newHoveredRow != hoveredRow)
    {
        auto oldHoveredRow = hoveredRow;
        hoveredRow = newHoveredRow;

        // Update hover state for affected rows
        if (oldHoveredRow >= 0)
        {
            auto it = rowComponents.find (oldHoveredRow);
            if (it != rowComponents.end())
                it->second->setHovered (false);
        }

        if (hoveredRow >= 0)
        {
            auto it = rowComponents.find (hoveredRow);
            if (it != rowComponents.end())
                it->second->setHovered (true);
        }
    }
}

//==============================================================================
bool ListBox::needsScrolling() const
{
    return totalContentSize > viewportSize;
}

float ListBox::getTotalContentSize() const
{
    if (model == nullptr)
        return 0.0f;

    auto numRows = model->getNumRows();
    float totalSize = 0.0f;

    for (int i = 0; i < numRows; ++i)
    {
        totalSize += getRowSize (i);
    }

    return jmax (totalSize, static_cast<float> (minimumContentSize));
}

float ListBox::getViewportSize() const
{
    auto contentArea = getContentArea();
    return (orientation == Orientation::vertical) ? contentArea.getHeight() : contentArea.getWidth();
}

//==============================================================================
void ListBox::setVerticalScrollBarVisibility (ScrollBar::VisibilityMode mode)
{
    if (verticalScrollBar != nullptr)
    {
        verticalScrollBar->setVisibilityMode (mode);
        resized();
    }
}

void ListBox::setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode mode)
{
    if (horizontalScrollBar != nullptr)
    {
        horizontalScrollBar->setVisibilityMode (mode);
        resized();
    }
}

ScrollBar* ListBox::getVerticalScrollBar() const noexcept
{
    return verticalScrollBar.get();
}

ScrollBar* ListBox::getHorizontalScrollBar() const noexcept
{
    return horizontalScrollBar.get();
}

//==============================================================================
void ListBox::updateScrollBars()
{
    if (model == nullptr)
    {
        if (verticalScrollBar != nullptr)
            verticalScrollBar->setVisible (false);

        if (horizontalScrollBar != nullptr)
            horizontalScrollBar->setVisible (false);

        return;
    }

    if (orientation == Orientation::vertical)
    {
        if (horizontalScrollBar != nullptr)
            horizontalScrollBar->setVisible (false);
    }
    else
    {
        if (verticalScrollBar != nullptr)
            verticalScrollBar->setVisible (false);
    }

    auto contentArea = getContentArea();
    auto totalContent = getTotalContentSize();

    if (orientation == Orientation::vertical)
    {
        // Update vertical scrollbar
        if (verticalScrollBar != nullptr)
        {
            verticalScrollBar->setRangeLimits (0.0, totalContent);
            verticalScrollBar->setCurrentRange (scrollOffset, scrollOffset + contentArea.getHeight());
        }
    }
    else
    {
        // Update horizontal scrollbar
        if (horizontalScrollBar != nullptr)
        {
            horizontalScrollBar->setRangeLimits (0.0, totalContent);
            horizontalScrollBar->setCurrentRange (scrollOffset, scrollOffset + contentArea.getWidth());
        }
    }
}

void ListBox::handleScrollBarMoved()
{
    if (orientation == Orientation::vertical && verticalScrollBar != nullptr)
    {
        scrollOffset = static_cast<float> (verticalScrollBar->getCurrentRangeStart());
    }
    else if (orientation == Orientation::horizontal && horizontalScrollBar != nullptr)
    {
        scrollOffset = static_cast<float> (horizontalScrollBar->getCurrentRangeStart());
    }

    layoutRows();
    repaint();
}

Rectangle<float> ListBox::getContentArea() const
{
    auto bounds = getLocalBounds();

    // Reduce bounds by scrollbar size if visible
    if (verticalScrollBar != nullptr && verticalScrollBar->isVisible())
    {
        bounds = bounds.withTrimmedRight (verticalScrollBar->getScrollBarWidth());
    }

    if (horizontalScrollBar != nullptr && horizontalScrollBar->isVisible())
    {
        bounds = bounds.withTrimmedBottom (horizontalScrollBar->getScrollBarWidth());
    }

    return bounds;
}

} // namespace yup
