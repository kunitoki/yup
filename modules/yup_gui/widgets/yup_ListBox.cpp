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

namespace
{

/** The default drag image: a circle carrying the number of rows being dragged. */
class DragCountComponent final : public Component
{
public:
    explicit DragCountComponent (int count)
        : text (String (count))
    {
        setSize (48, 48);
        setOpaque (false);
    }

    void paint (Graphics& g) override
    {
        const auto bounds = getLocalBounds().to<float>();

        g.setFillColor (Color (0xff9e3f6d));
        g.fillEllipse (bounds);

        g.setFillColor (Colors::white);
        g.fillFittedText (text, ApplicationTheme::getGlobalTheme()->getDefaultFont(), bounds);
    }

private:
    String text;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragCountComponent)
};

/** Reads and builds geometry along a list's scroll (main) axis, so layout is written once for both orientations. */
struct ListBoxAxis
{
    bool vertical = true;

    float main (Point<float> point) const { return vertical ? point.getY() : point.getX(); }

    float cross (Point<float> point) const { return vertical ? point.getX() : point.getY(); }

    float mainStart (Rectangle<float> area) const { return vertical ? area.getY() : area.getX(); }

    float mainSize (Rectangle<float> area) const { return vertical ? area.getHeight() : area.getWidth(); }

    Rectangle<float> makeRect (float mainPosition, float mainLength, Rectangle<float> crossArea) const
    {
        return vertical ? Rectangle<float> (crossArea.getX(), mainPosition, crossArea.getWidth(), mainLength)
                        : Rectangle<float> (mainPosition, crossArea.getY(), mainLength, crossArea.getHeight());
    }
};

ListBoxAxis axisOf (const ListBox& listBox)
{
    return { listBox.getOrientation() == ListBox::Orientation::vertical };
}

/** Moves a component that scrolls with the content, resizing it only when its size changed. */
void placeScrollingComponent (Component& component, Rectangle<float> bounds)
{
    if (component.getWidth() == bounds.getWidth() && component.getHeight() == bounds.getHeight())
        component.setPosition (bounds.getPosition());
    else
        component.setBounds (bounds);
}

constexpr float listBoxDefaultVerticalRowSize = 24.0f;
constexpr float listBoxDefaultHorizontalRowSize = 96.0f;
constexpr float listBoxDefaultRefreshIndicatorSize = 56.0f;
constexpr float listBoxTouchSlop = 10.0f;
constexpr double listBoxLongPressSeconds = 0.5;
constexpr double listBoxMaxFrameGapSeconds = 1.0 / 15.0;

} // namespace

//==============================================================================
float ListBoxModel::getRowSize (int rowIndex)
{
    ignoreUnused (rowIndex);
    return 0.0f;
}

void ListBoxModel::refreshRowComponent (int rowIndex, bool isSelected, std::unique_ptr<Component>& component)
{
    ignoreUnused (rowIndex, isSelected, component);
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

void ListBoxModel::returnKeyPressed (int currentRow)
{
    ignoreUnused (currentRow);
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
/** Hosts one visible row: the model's component, or the built-in ListBoxItem when the model leaves it null. */
class ListBox::ListBoxRow final : public Component
{
public:
    explicit ListBoxRow (ListBox& owner)
        : owner (owner)
    {
        setOpaque (false);
        setWantsMouseEvents (false, true);
    }

    void refresh (int newRowIndex, bool shouldBeSelected)
    {
        rowIndex = newRowIndex;
        selected = shouldBeSelected;

        if (owner.model == nullptr)
            return;

        owner.model->refreshRowComponent (rowIndex, selected, content);

        if (content != nullptr)
        {
            if (builtInItem != nullptr)
                builtInItem->setVisible (false);

            if (content->getParentComponent() != this)
                addAndMakeVisible (*content);
        }
        else
        {
            if (builtInItem == nullptr)
            {
                builtInItem = std::make_unique<ListBoxItem>();
                addChildComponent (*builtInItem);
            }

            builtInItem->setIconPosition (owner.orientation == Orientation::horizontal
                                              ? ListBoxItem::IconPosition::above
                                              : ListBoxItem::IconPosition::left);
            builtInItem->setText (owner.model->getRowText (rowIndex));
            builtInItem->setIcon (owner.model->getRowIcon (rowIndex));
            builtInItem->setVisible (true);
        }

        resized();
        pushStateToItem();
        repaint();
    }

    void setHovered (bool shouldBeHovered)
    {
        if (hovered == shouldBeHovered)
            return;

        hovered = shouldBeHovered;
        pushStateToItem();
        repaint();
    }

    int getRowIndex() const noexcept { return rowIndex; }

    bool isRowSelected() const noexcept { return selected; }

    Component* getDisplayedComponent() const
    {
        return content != nullptr ? content.get() : builtInItem.get();
    }

    void paint (Graphics& g) override
    {
        // A ListBoxItem paints its own states, anything else gets them painted underneath it.
        if (dynamic_cast<ListBoxItem*> (getDisplayedComponent()) != nullptr)
            return;

        Color backgroundColor;

        if (selected)
            backgroundColor = owner.findColor (ListBox::Style::selectedRowBackgroundColorId).value_or (Color (0xff3a7ebf));
        else if (hovered)
            backgroundColor = owner.findColor (ListBox::Style::hoveredRowBackgroundColorId).value_or (Color (0x22ffffff));
        else
            backgroundColor = owner.findColor (ListBox::Style::rowBackgroundColorId).value_or (Color (0x00000000));

        if (backgroundColor.getAlpha() > 0)
        {
            g.setFillColor (backgroundColor);
            g.fillRect (getLocalBounds());
        }
    }

    void resized() override
    {
        if (auto* displayed = getDisplayedComponent())
            displayed->setBounds (getLocalBounds());
    }

private:
    void pushStateToItem()
    {
        if (auto* item = dynamic_cast<ListBoxItem*> (getDisplayedComponent()))
        {
            item->setSelected (selected);
            item->setHovered (hovered);
        }
    }

    ListBox& owner;
    int rowIndex = -1;
    std::unique_ptr<Component> content;
    std::unique_ptr<ListBoxItem> builtInItem;
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

    verticalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::vertical);
    verticalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    verticalScrollBar->onScrollPositionChanged = [this] (double)
    {
        handleScrollBarMoved (*verticalScrollBar);
    };
    addAndMakeVisible (*verticalScrollBar);

    horizontalScrollBar = std::make_unique<ScrollBar> (ScrollBar::Orientation::horizontal);
    horizontalScrollBar->setVisibilityMode (ScrollBar::VisibilityMode::autoHide);
    horizontalScrollBar->onScrollPositionChanged = [this] (double)
    {
        handleScrollBarMoved (*horizontalScrollBar);
    };
    addAndMakeVisible (*horizontalScrollBar);
}

ListBox::~ListBox()
{
    // A list carrying a gesture on behalf of this one will never see its release now.
    if (gesture.handOffTarget.get() != nullptr)
    {
        MessageManager::callAsync ([target = gesture.handOffTarget]
        {
            if (auto* list = dynamic_cast<ListBox*> (target.get()))
                list->releaseLostGesture();
        });
    }

    // The model may already be gone, so nothing here may call into it.
    model = nullptr;
    destroyAllRows();
}

//==============================================================================
void ListBox::setModel (ListBoxModel* newModel)
{
    if (model == newModel)
        return;

    destroyAllRows();

    model = newModel;
    selectedRows.clear();
    currentRow = -1;
    selectionAnchor = -1;
    hoveredRow = -1;
    rowSelectedOnMouseUp = -1;
    endReachedArmed = true;
    gesture = {};

    numRows = model != nullptr ? jmax (0, model->getNumRows()) : 0;
    readRowSizes();

    updateLayout();
    dispatchPendingNotifications();
}

ListBoxModel* ListBox::getModel() const noexcept
{
    return model;
}

//==============================================================================
void ListBox::setSelectionMode (SelectionMode mode)
{
    if (selectionMode == mode)
        return;

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
        updateRowStates();
        notifySelectionChanged();
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
    jassert (isRowCountInSync());

    if (selectionMode == SelectionMode::none || rowIndex < 0 || rowIndex >= numRows)
        return;

    bool changed = false;

    if (selectionMode == SelectionMode::single)
    {
        if (selectedRows.size() != 1 || selectedRows.getFirst() != rowIndex)
        {
            selectedRows.clear();
            selectedRows.add (rowIndex);
            changed = true;
        }
    }
    else if (! selectedRows.contains (rowIndex))
    {
        selectedRows.add (rowIndex);
        std::sort (selectedRows.begin(), selectedRows.end());
        changed = true;
    }

    selectionAnchor = rowIndex;
    setCurrentRow (rowIndex, notification);

    if (scrollToShowRow)
        scrollToRow (rowIndex);

    if (! changed)
        return;

    updateRowStates();

    if (notification != dontSendNotification)
        notifySelectionChanged();
}

void ListBox::deselectRow (int rowIndex, NotificationType notification)
{
    if (! selectedRows.contains (rowIndex))
        return;

    selectedRows.removeAllInstancesOf (rowIndex);

    if (selectionAnchor == rowIndex)
        selectionAnchor = selectedRows.isEmpty() ? -1 : selectedRows.getLast();

    updateRowStates();

    if (notification != dontSendNotification)
        notifySelectionChanged();
}

void ListBox::deselectAllRows (NotificationType notification)
{
    if (selectedRows.isEmpty())
        return;

    selectedRows.clear();
    selectionAnchor = -1;
    updateRowStates();

    if (notification != dontSendNotification)
        notifySelectionChanged();
}

Array<int> ListBox::getSelectedRows() const
{
    return selectedRows;
}

void ListBox::setSelectedRows (const Array<int>& rows, NotificationType notification)
{
    jassert (isRowCountInSync());

    if (selectionMode == SelectionMode::none)
        return;

    Array<int> newRows;

    for (const auto row : rows)
    {
        if (row >= 0 && row < numRows && ! newRows.contains (row))
            newRows.add (row);

        if (selectionMode == SelectionMode::single && ! newRows.isEmpty())
            break;
    }

    std::sort (newRows.begin(), newRows.end());
    selectedRows = std::move (newRows);

    selectionAnchor = selectedRows.isEmpty() ? -1 : selectedRows.getLast();

    if (! selectedRows.isEmpty())
        setCurrentRow (selectedRows.getLast(), notification);

    updateRowStates();

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
void ListBox::setCurrentRow (int rowIndex, NotificationType notification)
{
    if (rowIndex < -1 || rowIndex >= numRows || rowIndex == currentRow)
        return;

    currentRow = rowIndex;

    if (notification != dontSendNotification && onCurrentRowChanged)
        onCurrentRowChanged (currentRow);
}

int ListBox::getCurrentRow() const noexcept
{
    return currentRow;
}

//==============================================================================
void ListBox::setDragSourceEnabled (bool shouldBeEnabled)
{
    dragSourceEnabled = shouldBeEnabled;
}

bool ListBox::isDragSourceEnabled() const noexcept
{
    return dragSourceEnabled;
}

std::unique_ptr<Component> ListBox::createDragSourceComponent (const Array<int>& rows)
{
    return std::make_unique<DragCountComponent> (rows.size());
}

void ListBox::dragOperationEnded (const DragAndDropData&, DragAndDropAction)
{
    dragSourceComponent.reset();
}

//==============================================================================
void ListBox::updateContent()
{
    const auto previousNumRows = numRows;

    numRows = model != nullptr ? jmax (0, model->getNumRows()) : 0;
    readRowSizes();

    const auto previousSelectionSize = selectedRows.size();
    selectedRows.removeIf ([this] (int row) { return row >= numRows; });

    for (auto* index : { &currentRow, &selectionAnchor, &hoveredRow, &rowSelectedOnMouseUp })
    {
        if (*index >= numRows)
            *index = -1;
    }

    if (numRows != previousNumRows)
        endReachedArmed = true;

    updateLayout();
    refreshVisibleRows();

    if (selectedRows.size() != previousSelectionSize)
        notifySelectionChanged();

    dispatchPendingNotifications();
}

void ListBox::rowsInserted (int startRow, int count)
{
    if (model == nullptr || count <= 0)
        return;

    startRow = jlimit (0, numRows, startRow);

    const auto anchorRow = visibleRowRange.isEmpty() ? -1 : visibleRowRange.getStart();
    const auto anchorStart = anchorRow >= 0 ? rowStarts[static_cast<size_t> (anchorRow)] : 0.0f;
    const auto mapping = [startRow, count] (int row)
    {
        return row >= startRow ? row + count : row;
    };

    remapRowIndices (mapping);

    numRows += count;
    rowSizes.insert (rowSizes.begin() + startRow, static_cast<size_t> (count), 0.0f);

    for (int row = startRow; row < startRow + count; ++row)
        rowSizes[static_cast<size_t> (row)] = readRowSize (row);

    rebuildRowStarts (startRow);
    jassert (isRowCountInSync());

    endReachedArmed = true;

    if (anchorRow >= 0 && startRow < anchorRow)
        anchorScrollPosition (anchorRow, anchorStart, mapping);

    updateLayout();
    updateRowStates();
    dispatchPendingNotifications();
}

void ListBox::rowsRemoved (int startRow, int count)
{
    if (model == nullptr)
        return;

    startRow = jlimit (0, numRows, startRow);
    count = jmin (count, numRows - startRow);

    if (count <= 0)
        return;

    const auto endRow = startRow + count;
    const auto anchorRow = visibleRowRange.isEmpty() ? -1 : visibleRowRange.getStart();
    const auto anchorStart = anchorRow >= 0 ? rowStarts[static_cast<size_t> (anchorRow)] : 0.0f;
    const auto removedSelectedRows = std::any_of (selectedRows.begin(), selectedRows.end(), [&] (int row)
    {
        return row >= startRow && row < endRow;
    });

    const auto mapping = [startRow, endRow, count] (int row)
    {
        if (row < startRow)
            return row;

        return row < endRow ? -1 : row - count;
    };

    remapRowIndices (mapping);

    numRows -= count;
    rowSizes.erase (rowSizes.begin() + startRow, rowSizes.begin() + endRow);
    rebuildRowStarts (startRow);
    jassert (isRowCountInSync());

    endReachedArmed = true;

    if (anchorRow >= 0 && endRow <= anchorRow)
        anchorScrollPosition (anchorRow, anchorStart, mapping);

    updateLayout();
    updateRowStates();

    if (removedSelectedRows)
        notifySelectionChanged();

    dispatchPendingNotifications();
}

void ListBox::rowMoved (int fromRow, int toRow)
{
    jassert (isRowCountInSync());

    if (model == nullptr || fromRow == toRow || ! isPositiveAndBelow (fromRow, numRows) || ! isPositiveAndBelow (toRow, numRows))
        return;

    const auto mapping = [fromRow, toRow] (int row)
    {
        if (row == fromRow)
            return toRow;

        if (fromRow < toRow && row > fromRow && row <= toRow)
            return row - 1;

        if (toRow < fromRow && row >= toRow && row < fromRow)
            return row + 1;

        return row;
    };

    remapRowIndices (mapping);

    const auto movedSize = rowSizes[static_cast<size_t> (fromRow)];
    rowSizes.erase (rowSizes.begin() + fromRow);
    rowSizes.insert (rowSizes.begin() + toRow, movedSize);
    rebuildRowStarts (jmin (fromRow, toRow));

    updateLayout();
    updateRowStates();
    dispatchPendingNotifications();
}

void ListBox::rowsChanged (int startRow, int count)
{
    jassert (isRowCountInSync());

    if (model == nullptr)
        return;

    startRow = jlimit (0, numRows, startRow);
    const auto endRow = jmin (numRows, startRow + jmax (0, count));

    if (endRow <= startRow)
        return;

    const auto anchorRow = visibleRowRange.isEmpty() ? -1 : visibleRowRange.getStart();
    const auto anchorStart = anchorRow >= 0 ? rowStarts[static_cast<size_t> (anchorRow)] : 0.0f;

    for (int row = startRow; row < endRow; ++row)
        rowSizes[static_cast<size_t> (row)] = readRowSize (row);

    rebuildRowStarts (startRow);

    if (anchorRow >= 0 && endRow <= anchorRow)
        anchorScrollPosition (anchorRow, anchorStart, [] (int row) { return row; });

    updateLayout();

    for (auto& [row, rowComponent] : visibleRows)
    {
        if (row >= startRow && row < endRow)
            rowComponent->refresh (row, isRowSelected (row));
    }

    updateRowStates();
    dispatchPendingNotifications();
}

void ListBox::repaintRow (int rowIndex)
{
    if (auto it = visibleRows.find (rowIndex); it != visibleRows.end())
        it->second->repaint();
}

//==============================================================================
void ListBox::scrollToRow (int rowIndex, ScrollAlignment alignment, bool animated)
{
    jassert (isRowCountInSync());

    if (rowIndex < 0 || rowIndex >= numRows)
        return;

    const auto start = getRowsOrigin() + rowStarts[static_cast<size_t> (rowIndex)];
    const auto size = rowSizes[static_cast<size_t> (rowIndex)];
    const auto position = scroller.getOffset();

    float target = start;

    switch (alignment)
    {
        case ScrollAlignment::start:
            break;

        case ScrollAlignment::center:
            target = start + (size - viewportSize) * 0.5f;
            break;

        case ScrollAlignment::end:
            target = start + size - viewportSize;
            break;

        case ScrollAlignment::nearest:
            if (start >= position && start + size <= position + viewportSize)
                return;

            if (start >= position && size <= viewportSize)
                target = start + size - viewportSize;

            break;
    }

    setScrollPosition (target, animated);
}

void ListBox::setScrollPosition (float newOffset, bool animated)
{
    if (animated)
        scroller.animateTo (newOffset);
    else
        scroller.setOffset (newOffset);

    updateLayout();
    dispatchPendingNotifications();
}

float ListBox::getScrollPosition() const noexcept
{
    return scroller.getOffset();
}

void ListBox::setScrollOptions (const KineticScroller::Options& newOptions)
{
    scroller.setOptions (newOptions);
}

const KineticScroller::Options& ListBox::getScrollOptions() const noexcept
{
    return scroller.getOptions();
}

ListBox::ScrollState ListBox::getScrollState() const noexcept
{
    if (gesture.scrolling)
        return ScrollState::dragging;

    return scroller.isAnimating() ? ScrollState::settling : ScrollState::idle;
}

void ListBox::setMouseDragScrollingEnabled (bool shouldBeEnabled)
{
    mouseDragScrollingEnabled = shouldBeEnabled;
}

bool ListBox::isMouseDragScrollingEnabled() const noexcept
{
    return mouseDragScrollingEnabled;
}

//==============================================================================
void ListBox::setPullToRefreshEnabled (bool shouldBeEnabled)
{
    // Pulling to refresh is an overscroll gesture, it cannot work with overscroll disabled.
    jassert (! shouldBeEnabled || scroller.getOptions().overscrollEnabled);

    pullToRefreshEnabled = shouldBeEnabled;

    if (! pullToRefreshEnabled)
        setRefreshing (false);
}

bool ListBox::isPullToRefreshEnabled() const noexcept
{
    return pullToRefreshEnabled;
}

void ListBox::setRefreshing (bool shouldBeRefreshing)
{
    if (refreshing == shouldBeRefreshing)
        return;

    refreshing = shouldBeRefreshing;

    // Spring back while the limits still include the indicator, then drop them to the content start.
    if (! refreshing && scroller.getOffset() < 0.0f && ! scroller.isDragging())
        scroller.animateTo (0.0f);

    updateLayout();
    dispatchPendingNotifications();
}

bool ListBox::isRefreshing() const noexcept
{
    return refreshing;
}

float ListBox::getPullToRefreshProgress() const noexcept
{
    if (! pullToRefreshEnabled)
        return 0.0f;

    if (refreshing)
        return 1.0f;

    return jlimit (0.0f, 1.0f, -scroller.getOffset() / getRefreshIndicatorSize());
}

float ListBox::getRefreshIndicatorSize() const
{
    if (auto theme = ApplicationTheme::getGlobalTheme())
    {
        if (auto size = theme->findMetric (*this, Style::refreshIndicatorSizeId))
            return jmax (1.0f, *size);
    }

    return listBoxDefaultRefreshIndicatorSize;
}

void ListBox::setEndReachedThreshold (float viewportFraction)
{
    endReachedThreshold = jmax (0.0f, viewportFraction);
}

float ListBox::getEndReachedThreshold() const noexcept
{
    return endReachedThreshold;
}

//==============================================================================
void ListBox::setOrientation (Orientation newOrientation)
{
    if (orientation == newOrientation)
        return;

    orientation = newOrientation;
    updateContent();
}

ListBox::Orientation ListBox::getOrientation() const noexcept
{
    return orientation;
}

//==============================================================================
void ListBox::setRowSize (float newSize)
{
    rowSize = jmax (1.0f, newSize);
    rowSizeIsExplicit = true;

    readRowSizes();
    updateLayout();
    dispatchPendingNotifications();
}

float ListBox::getRowSize() const noexcept
{
    if (rowSizeIsExplicit)
        return rowSize;

    return orientation == Orientation::vertical ? listBoxDefaultVerticalRowSize : listBoxDefaultHorizontalRowSize;
}

void ListBox::setRowSpacing (float newSpacing)
{
    rowSpacing = jmax (0.0f, newSpacing);

    rebuildRowStarts (0);
    updateLayout();
    dispatchPendingNotifications();
}

float ListBox::getRowSpacing() const noexcept
{
    return rowSpacing;
}

void ListBox::setContentInsets (float leading, float trailing)
{
    leadingInset = jmax (0.0f, leading);
    trailingInset = jmax (0.0f, trailing);

    updateLayout();
    dispatchPendingNotifications();
}

//==============================================================================
void ListBox::setHeaderComponent (std::unique_ptr<Component> newHeader)
{
    headerComponent = std::move (newHeader);

    if (headerComponent != nullptr)
        addAndMakeVisible (*headerComponent);

    updateLayout();
    dispatchPendingNotifications();
}

Component* ListBox::getHeaderComponent() const noexcept
{
    return headerComponent.get();
}

void ListBox::setFooterComponent (std::unique_ptr<Component> newFooter)
{
    footerComponent = std::move (newFooter);

    if (footerComponent != nullptr)
        addAndMakeVisible (*footerComponent);

    updateLayout();
    dispatchPendingNotifications();
}

Component* ListBox::getFooterComponent() const noexcept
{
    return footerComponent.get();
}

//==============================================================================
void ListBox::setMinimumContentSize (int minSize)
{
    const auto newMinimumContentSize = jmax (0, minSize);

    if (minimumContentSize == newMinimumContentSize)
        return;

    minimumContentSize = newMinimumContentSize;
    updateLayout();
    dispatchPendingNotifications();
}

int ListBox::getMinimumContentSize() const noexcept
{
    return minimumContentSize;
}

//==============================================================================
void ListBox::setVerticalScrollBarVisibility (ScrollBar::VisibilityMode mode)
{
    verticalScrollBar->setVisibilityMode (mode);
    resized();
}

void ListBox::setHorizontalScrollBarVisibility (ScrollBar::VisibilityMode mode)
{
    horizontalScrollBar->setVisibilityMode (mode);
    resized();
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
    auto it = visibleRows.find (rowIndex);
    return it != visibleRows.end() ? it->second->getDisplayedComponent() : nullptr;
}

Rectangle<float> ListBox::getRowBounds (int rowIndex) const
{
    if (rowIndex < 0 || rowIndex >= numRows)
        return {};

    const auto axis = axisOf (*this);
    const auto area = getContentArea();
    const auto position = axis.mainStart (area) + getRowsOrigin() + rowStarts[static_cast<size_t> (rowIndex)] - scroller.getOffset();

    return axis.makeRect (position, rowSizes[static_cast<size_t> (rowIndex)], area);
}

//==============================================================================
void ListBox::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void ListBox::resized()
{
    auto bounds = getLocalBounds();

    verticalScrollBar->setBounds (bounds.removeFromRight (verticalScrollBar->getScrollBarWidth()));
    horizontalScrollBar->setBounds (bounds.removeFromBottom (horizontalScrollBar->getScrollBarWidth()));

    updateLayout();
    dispatchPendingNotifications();
}

void ListBox::refreshDisplay (double lastFrameTimeSeconds)
{
    const auto deltaSeconds = jlimit (0.0, listBoxMaxFrameGapSeconds, lastFrameTimeSeconds);
    gestureClock += deltaSeconds;

    if (gesture.active
        && ! gesture.scrolling
        && ! gesture.tapCancelled
        && ! gesture.longPressed
        && gesture.pressedRow >= 0
        && gestureClock - gesture.downTime >= listBoxLongPressSeconds)
    {
        triggerLongPress();
    }

    if (scroller.update (deltaSeconds))
        updateLayout();

    // The indicator spins while refreshing.
    if (refreshing)
        repaint();

    dispatchPendingNotifications();
}

//==============================================================================
void ListBox::mouseDown (const MouseEvent& event)
{
    jassert (isRowCountInSync());

    if (isTouchLike (event))
    {
        gestureDown (event);
        return;
    }

    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    rowSelectedOnMouseUp = -1;

    const auto rowIndex = getRowIndexAt (event.getPosition());

    if (rowIndex < 0)
        return;

    const auto modifiers = event.getModifiers();
    const bool isShiftDown = modifiers.isShiftDown();
    const bool isCommandDown = modifiers.isCommandDown() || modifiers.isControlDown();

    if (! isShiftDown && ! isCommandDown
        && selectionMode == SelectionMode::multiple
        && isRowSelected (rowIndex))
    {
        rowSelectedOnMouseUp = rowIndex;
    }
    else
    {
        handleRowSelection (rowIndex, isCommandDown, isShiftDown);
    }

    setCurrentRow (rowIndex);
    handleRowClick (rowIndex, event);
}

void ListBox::mouseUp (const MouseEvent& event)
{
    if (isTouchLike (event))
    {
        gestureUp (event);
        return;
    }

    if (rowSelectedOnMouseUp < 0)
        return;

    const auto rowIndex = rowSelectedOnMouseUp;
    rowSelectedOnMouseUp = -1;

    if (! isCurrentlyDragging())
        handleRowSelection (rowIndex, false, false);
}

void ListBox::mouseDrag (const MouseEvent& event)
{
    if (isTouchLike (event))
    {
        gestureDrag (event);
        return;
    }

    const auto delta = event.getPosition() - event.getLastMouseDownPosition();

    if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f)
        return;

    startDraggingSelectedRows (-1, {});
}

void ListBox::mouseMove (const MouseEvent& event)
{
    if (event.isTouch())
        return;

    updateHoveredRow (event.getPosition());
}

void ListBox::mouseExit (const MouseEvent& event)
{
    ignoreUnused (event);

    if (hoveredRow < 0)
        return;

    hoveredRow = -1;
    updateRowStates();
}

void ListBox::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    if (getMaxScrollOffset() <= 0.0f)
        return;

    const bool vertical = orientation == Orientation::vertical;

    // A plain vertical wheel still scrolls a horizontal list.
    auto delta = vertical ? wheelData.getDeltaY() : wheelData.getDeltaX();
    if (delta == 0.0f)
        delta = vertical ? wheelData.getDeltaX() : wheelData.getDeltaY();

    scroller.setOffset (scroller.getOffset() - delta * getRowSize() * 3.0f);

    updateLayout();
    updateHoveredRow (event.getPosition());
    dispatchPendingNotifications();
}

void ListBox::mouseDoubleClick (const MouseEvent& event)
{
    if (isTouchLike (event) && gesture.active)
    {
        if (gesture.scrolling)
            return;

        // The platform reports the double-tap before the release that completes it, and that
        // release must not toggle the row the first tap just selected.
        gesture.doubleTapped = true;
    }

    const auto rowIndex = getRowIndexAt (event.getPosition());

    if (rowIndex < 0)
        return;

    if (onRowDoubleClicked)
        onRowDoubleClicked (rowIndex);

    if (model != nullptr)
        model->rowDoubleClicked (rowIndex, event);
}

//==============================================================================
void ListBox::keyDown (const KeyPress& key, const Point<float>& position)
{
    ignoreUnused (position);
    jassert (isRowCountInSync());

    if (model == nullptr || numRows == 0)
        return;

    const auto keyCode = key.getKey();
    const auto modifiers = key.getModifiers();
    const bool vertical = orientation == Orientation::vertical;
    const auto previousKey = vertical ? KeyPress::upKey : KeyPress::leftKey;
    const auto nextKey = vertical ? KeyPress::downKey : KeyPress::rightKey;
    const auto page = jmax (1, getVisibleRowsCount());

    if (keyCode == previousKey)
        navigateTo (jmax (0, currentRow - 1), modifiers);
    else if (keyCode == nextKey)
        navigateTo (jmin (numRows - 1, currentRow + 1), modifiers);
    else if (keyCode == KeyPress::pageUpKey)
        navigateTo (jmax (0, currentRow - page), modifiers);
    else if (keyCode == KeyPress::pageDownKey)
        navigateTo (jmin (numRows - 1, currentRow + page), modifiers);
    else if (keyCode == KeyPress::homeKey)
        navigateTo (0, modifiers);
    else if (keyCode == KeyPress::endKey)
        navigateTo (numRows - 1, modifiers);
    else if (keyCode == KeyPress::spaceKey)
    {
        if (selectionMode == SelectionMode::multiple && currentRow >= 0)
        {
            if (isRowSelected (currentRow))
                deselectRow (currentRow, sendNotification);
            else
                selectRow (currentRow, false, sendNotification);
        }
    }
    else if (keyCode == KeyPress::enterKey || keyCode == KeyPress::kpEnterKey)
        model->returnKeyPressed (currentRow);
    else if (keyCode == KeyPress::deleteKey || keyCode == KeyPress::backspaceKey)
        model->deleteKeyPressed (selectedRows);
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
bool ListBox::isRowCountInSync() const
{
    return model == nullptr || numRows == model->getNumRows();
}

void ListBox::readRowSizes()
{
    rowSizes.resize (static_cast<size_t> (numRows));

    for (int row = 0; row < numRows; ++row)
        rowSizes[static_cast<size_t> (row)] = readRowSize (row);

    rebuildRowStarts (0);
}

float ListBox::readRowSize (int rowIndex) const
{
    const auto size = model != nullptr ? model->getRowSize (rowIndex) : 0.0f;
    return size > 0.0f ? size : getRowSize();
}

void ListBox::rebuildRowStarts (int fromRow)
{
    rowStarts.resize (static_cast<size_t> (numRows) + 1);
    rowStarts[0] = 0.0f;

    for (int row = jmax (0, fromRow); row < numRows; ++row)
    {
        const auto index = static_cast<size_t> (row);
        rowStarts[index + 1] = rowStarts[index] + rowSizes[index] + rowSpacing;
    }
}

float ListBox::getMainSizeOf (const Component* component) const
{
    if (component == nullptr)
        return 0.0f;

    return orientation == Orientation::vertical ? component->getHeight() : component->getWidth();
}

float ListBox::getRowsOrigin() const
{
    return leadingInset + getMainSizeOf (headerComponent.get());
}

float ListBox::getRowsExtent() const
{
    return numRows > 0 ? rowStarts[static_cast<size_t> (numRows)] - rowSpacing : 0.0f;
}

float ListBox::getTotalContentSize() const
{
    const auto total = getRowsOrigin() + getRowsExtent() + getMainSizeOf (footerComponent.get()) + trailingInset;
    return jmax (total, static_cast<float> (minimumContentSize));
}

float ListBox::getMinScrollOffset() const
{
    // While refreshing, the content rests below its start so the indicator stays in view.
    return refreshing ? -getRefreshIndicatorSize() : 0.0f;
}

float ListBox::getMaxScrollOffset() const
{
    return jmax (0.0f, getTotalContentSize() - viewportSize);
}

Rectangle<float> ListBox::getContentArea() const
{
    auto bounds = getLocalBounds();

    if (verticalScrollBar->isVisible())
        bounds = bounds.withTrimmedRight (verticalScrollBar->getScrollBarWidth());

    if (horizontalScrollBar->isVisible())
        bounds = bounds.withTrimmedBottom (horizontalScrollBar->getScrollBarWidth());

    return bounds;
}

Range<int> ListBox::computeVisibleRowRange() const
{
    if (numRows == 0 || viewportSize <= 0.0f)
        return {};

    const auto windowStart = scroller.getOffset() - getRowsOrigin();
    const auto windowEnd = windowStart + viewportSize;
    const auto begin = rowStarts.begin();
    const auto end = begin + numRows;

    auto first = jmax (0, static_cast<int> (std::upper_bound (begin, end, windowStart) - begin) - 1);

    // Starting in the gap after a row: that row is already out of view.
    if (rowStarts[static_cast<size_t> (first)] + rowSizes[static_cast<size_t> (first)] <= windowStart)
        ++first;

    const auto last = static_cast<int> (std::lower_bound (begin, end, windowEnd) - begin);

    return { jmin (first, last), last };
}

int ListBox::getRowIndexAt (Point<float> position) const
{
    if (numRows == 0)
        return -1;

    const auto area = getContentArea();

    if (! area.contains (position))
        return -1;

    const auto axis = axisOf (*this);
    const auto along = axis.main (position) - axis.mainStart (area) + scroller.getOffset() - getRowsOrigin();

    if (along < 0.0f)
        return -1;

    const auto begin = rowStarts.begin();
    const auto row = static_cast<int> (std::upper_bound (begin, begin + numRows, along) - begin) - 1;

    // Past the row's own size is the spacing after it.
    if (row < 0 || along >= rowStarts[static_cast<size_t> (row)] + rowSizes[static_cast<size_t> (row)])
        return -1;

    return row;
}

//==============================================================================
void ListBox::updateLayout()
{
    // Only the scrollbar along the scroll axis is used, and the other one must not eat into the viewport.
    (orientation == Orientation::vertical ? horizontalScrollBar : verticalScrollBar)->setVisible (false);

    viewportSize = axisOf (*this).mainSize (getContentArea());
    scroller.setLimits (getMinScrollOffset(), getMaxScrollOffset(), viewportSize);

    updateScrollBars();

    layoutHeaderAndFooter (getContentArea());
    layoutRows();

    repaint();
}

void ListBox::layoutRows()
{
    visibleRowRange = computeVisibleRowRange();

    for (auto it = visibleRows.begin(); it != visibleRows.end();)
    {
        if (visibleRowRange.contains (it->first))
        {
            ++it;
            continue;
        }

        it->second->setVisible (false);
        recycledRows.push_back (std::move (it->second));
        it = visibleRows.erase (it);
    }

    for (int rowIndex = visibleRowRange.getStart(); rowIndex < visibleRowRange.getEnd(); ++rowIndex)
    {
        auto& row = visibleRows[rowIndex];
        const bool isNewlyVisible = row == nullptr;

        if (isNewlyVisible)
        {
            if (! recycledRows.empty())
            {
                row = std::move (recycledRows.back());
                recycledRows.pop_back();
            }
            else
            {
                row = std::make_unique<ListBoxRow> (*this);
                addChildComponent (*row);
            }
        }

        placeScrollingComponent (*row, getRowBounds (rowIndex).toNearestInt());

        if (isNewlyVisible)
        {
            row->refresh (rowIndex, isRowSelected (rowIndex));
            row->setHovered (rowIndex == hoveredRow);
        }

        row->setVisible (true);
    }

    verticalScrollBar->toFront (false);
    horizontalScrollBar->toFront (false);
}

void ListBox::layoutHeaderAndFooter (Rectangle<float> contentArea)
{
    const auto axis = axisOf (*this);
    const auto origin = axis.mainStart (contentArea) + leadingInset - scroller.getOffset();
    const auto headerSize = getMainSizeOf (headerComponent.get());

    if (headerComponent != nullptr)
        placeScrollingComponent (*headerComponent, axis.makeRect (origin, headerSize, contentArea));

    if (footerComponent != nullptr)
        placeScrollingComponent (*footerComponent, axis.makeRect (origin + headerSize + getRowsExtent(), getMainSizeOf (footerComponent.get()), contentArea));
}

void ListBox::updateScrollBars()
{
    const bool vertical = orientation == Orientation::vertical;
    auto& scrollBar = vertical ? *verticalScrollBar : *horizontalScrollBar;

    if (model == nullptr)
    {
        scrollBar.setVisible (false);
        return;
    }

    // Overscroll stretches the content, not the scrollbar: it shows the clamped position.
    const auto position = jlimit (0.0f, getMaxScrollOffset(), scroller.getOffset());

    scrollBar.setRangeLimits (0.0, getTotalContentSize());
    scrollBar.setCurrentRange (position, position + viewportSize);

    // The bar only updates its own visibility when its ranges change, which misses it being hidden above.
    const auto mode = scrollBar.getVisibilityMode();
    scrollBar.setVisible (mode == ScrollBar::VisibilityMode::alwaysVisible
                          || (mode == ScrollBar::VisibilityMode::autoHide && scrollBar.isScrollingNeeded()));
}

void ListBox::handleScrollBarMoved (ScrollBar& scrollBar)
{
    scroller.setOffset (static_cast<float> (scrollBar.getCurrentRangeStart()));

    updateLayout();
    dispatchPendingNotifications();
}

void ListBox::refreshVisibleRows()
{
    for (auto& [rowIndex, row] : visibleRows)
    {
        row->refresh (rowIndex, isRowSelected (rowIndex));
        row->setHovered (rowIndex == hoveredRow);
    }
}

void ListBox::updateRowStates()
{
    for (auto& [rowIndex, row] : visibleRows)
    {
        const auto selected = isRowSelected (rowIndex);

        // A row whose index shifted shows another row's content, so it needs refreshing as well.
        if (row->getRowIndex() != rowIndex || row->isRowSelected() != selected)
            row->refresh (rowIndex, selected);

        row->setHovered (rowIndex == hoveredRow);
    }
}

void ListBox::destroyAllRows()
{
    visibleRows.clear();
    recycledRows.clear();
    visibleRowRange = {};
}

//==============================================================================
void ListBox::remapRowIndices (const std::function<int (int)>& mapping)
{
    const auto remap = [&mapping] (int row)
    {
        return row >= 0 ? mapping (row) : row;
    };

    Array<int> remappedSelection;

    for (const auto row : selectedRows)
    {
        if (const auto newRow = remap (row); newRow >= 0)
            remappedSelection.add (newRow);
    }

    std::sort (remappedSelection.begin(), remappedSelection.end());
    selectedRows = std::move (remappedSelection);

    currentRow = remap (currentRow);
    selectionAnchor = remap (selectionAnchor);
    hoveredRow = remap (hoveredRow);
    rowSelectedOnMouseUp = remap (rowSelectedOnMouseUp);

    std::unordered_map<int, std::unique_ptr<ListBoxRow>> remappedRows;

    for (auto& [rowIndex, row] : visibleRows)
    {
        if (const auto newRow = remap (rowIndex); newRow >= 0)
        {
            remappedRows.emplace (newRow, std::move (row));
        }
        else
        {
            row->setVisible (false);
            recycledRows.push_back (std::move (row));
        }
    }

    visibleRows = std::move (remappedRows);
}

void ListBox::anchorScrollPosition (int anchorRow, float anchorStart, const std::function<int (int)>& mapping)
{
    const auto newAnchorRow = mapping (anchorRow);

    if (newAnchorRow < 0)
        return;

    // The content already changed size, so the anchored position must be checked against the new limits.
    scroller.setLimits (getMinScrollOffset(), getMaxScrollOffset(), viewportSize);

    const auto delta = rowStarts[static_cast<size_t> (newAnchorRow)] - anchorStart;

    if (delta != 0.0f)
        scroller.translate (delta);
}

//==============================================================================
void ListBox::handleRowClick (int rowIndex, const MouseEvent& event)
{
    const BailOutChecker checker (this);

    if (onRowClicked)
        onRowClicked (rowIndex);

    if (checker.shouldBailOut())
        return;

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

    if (shouldExtend && selectionAnchor >= 0)
    {
        selectRange (selectionAnchor, rowIndex);
    }
    else if (shouldToggle)
    {
        if (isRowSelected (rowIndex))
            deselectRow (rowIndex, sendNotification);
        else
            selectRow (rowIndex, false, sendNotification);
    }
    else
    {
        deselectAllRows (dontSendNotification);
        selectRow (rowIndex, false, sendNotification);
    }
}

void ListBox::selectRange (int fromRow, int toRow)
{
    selectedRows.clear();

    for (int row = jmin (fromRow, toRow); row <= jmax (fromRow, toRow); ++row)
        selectedRows.add (row);

    updateRowStates();
    notifySelectionChanged();
}

void ListBox::navigateTo (int rowIndex, const KeyModifiers& modifiers)
{
    if (rowIndex < 0 || rowIndex >= numRows)
        return;

    const bool isMultiple = selectionMode == SelectionMode::multiple;
    const bool isShiftDown = modifiers.isShiftDown();
    const bool isCommandDown = modifiers.isCommandDown() || modifiers.isControlDown();

    if (isMultiple && isShiftDown)
    {
        if (selectionAnchor < 0)
            selectionAnchor = currentRow >= 0 ? currentRow : rowIndex;

        setCurrentRow (rowIndex);
        selectRange (selectionAnchor, rowIndex);
    }
    else if (isMultiple && isCommandDown)
    {
        setCurrentRow (rowIndex);
    }
    else
    {
        setCurrentRow (rowIndex);
        selectionAnchor = rowIndex;

        if (selectionMode == SelectionMode::single)
            selectRow (rowIndex, false, sendNotification);
        else if (isMultiple && (selectedRows.size() != 1 || selectedRows.getFirst() != rowIndex))
            setSelectedRows ({ rowIndex }, sendNotification);
    }

    scrollToRow (rowIndex);
}

void ListBox::notifySelectionChanged()
{
    if (onSelectionChanged)
        onSelectionChanged();

    if (model != nullptr)
        model->selectedRowsChanged (selectedRows);
}

void ListBox::updateHoveredRow (Point<float> position)
{
    const auto newHoveredRow = getRowIndexAt (position);

    if (newHoveredRow == hoveredRow)
        return;

    hoveredRow = newHoveredRow;
    updateRowStates();
}

//==============================================================================
bool ListBox::isTouchLike (const MouseEvent& event) const
{
    return event.isTouch() || mouseDragScrollingEnabled;
}

void ListBox::gestureDown (const MouseEvent& event)
{
    if (gesture.active)
    {
        // Only the first finger drives a gesture.
        if (event.getTouchIndex() != gesture.touchIndex)
            return;

        // The same pointer pressing again means its release was lost.
        releaseLostGesture();
    }

    if (! event.isTouch() && getWantsKeyboardFocus())
        takeKeyboardFocus();

    const auto position = event.getPosition();
    const bool wasMoving = scroller.isAnimating();

    gesture = {};
    gesture.active = true;
    gesture.touchIndex = event.getTouchIndex();
    gesture.downPosition = position;
    gesture.lastPosition = position;
    gesture.downTime = gestureClock;

    // A press that stops moving content is only there to catch it, it is not a tap.
    gesture.tapCancelled = wasMoving;
    gesture.pressedRow = wasMoving ? -1 : getRowIndexAt (position);

    hoveredRow = -1;
    updateRowStates();

    scroller.beginDrag (axisOf (*this).main (position), gestureClock);
    dispatchPendingNotifications();
}

void ListBox::gestureDrag (const MouseEvent& event)
{
    if (! gesture.active || event.getTouchIndex() != gesture.touchIndex)
        return;

    const auto position = event.getPosition();
    gesture.lastPosition = position;

    if (auto* target = dynamic_cast<ListBox*> (gesture.handOffTarget.get()))
    {
        target->mouseDrag (translateEventFor (event, position, *target));
        return;
    }

    if (gesture.longPressed || gesture.crossAxis)
        return;

    const auto axis = axisOf (*this);

    if (! gesture.scrolling)
    {
        const auto delta = position - gesture.downPosition;
        const auto along = std::abs (axis.main (delta));
        const auto across = std::abs (axis.cross (delta));

        if (jmax (along, across) < listBoxTouchSlop)
            return;

        gesture.tapCancelled = true;

        if (across > along)
        {
            // Moving across this list: an enclosing list scrolling that way takes the rest of the gesture.
            gesture.crossAxis = true;

            if (auto* target = findHandOffTarget (event))
            {
                gesture.handOffTarget = target;

                if (scroller.getOverscroll() != 0.0f)
                    scroller.endDrag (gestureClock);
                else
                    scroller.stop();

                target->mouseDown (translateEventFor (event, gesture.downPosition, *target));
                target->mouseDrag (translateEventFor (event, position, *target));
            }

            return;
        }

        // Scrolling starts from the edge of the slop, so the content does not jump by the slop distance.
        gesture.scrolling = true;

        const auto direction = axis.main (delta) < 0.0f ? -1.0f : 1.0f;
        scroller.beginDrag (axis.main (gesture.downPosition) + direction * listBoxTouchSlop, gestureClock);
    }

    scroller.dragTo (axis.main (position), gestureClock);

    updateLayout();
    dispatchPendingNotifications();
}

void ListBox::gestureUp (const MouseEvent& event)
{
    if (! gesture.active || event.getTouchIndex() != gesture.touchIndex)
        return;

    if (auto* target = dynamic_cast<ListBox*> (gesture.handOffTarget.get()))
    {
        const auto forwarded = translateEventFor (event, event.getPosition(), *target);
        gesture = {};
        target->mouseUp (forwarded);
        return;
    }

    bool shouldRefresh = false;

    if (gesture.scrolling)
    {
        if (pullToRefreshEnabled && ! refreshing && scroller.getOverscroll() <= -getRefreshIndicatorSize())
        {
            refreshing = true;
            shouldRefresh = true;
            updateLayout();
        }

        scroller.endDrag (gestureClock);
    }
    else
    {
        // Not a scroll: release the content without a fling, bouncing back if it was caught overscrolled.
        if (scroller.getOverscroll() != 0.0f)
            scroller.endDrag (gestureClock);
        else
            scroller.stop();
    }

    const auto finished = gesture;
    gesture = {};

    updateLayout();

    const auto rowIndex = getRowIndexAt (event.getPosition());
    const BailOutChecker checker (this);

    if (! finished.scrolling
        && ! finished.tapCancelled
        && ! finished.longPressed
        && ! finished.doubleTapped
        && finished.pressedRow >= 0
        && rowIndex == finished.pressedRow)
    {
        tapRow (rowIndex, event);

        // A tap may close whatever hosts the list.
        if (checker.shouldBailOut())
            return;
    }

    dispatchPendingNotifications();

    if (shouldRefresh && ! checker.shouldBailOut() && onRefresh)
        onRefresh();
}

void ListBox::releaseLostGesture()
{
    if (! gesture.active)
        return;

    if (gesture.scrolling || scroller.getOverscroll() != 0.0f)
        scroller.endDrag (gestureClock);
    else
        scroller.stop();

    gesture = {};

    updateLayout();
    dispatchPendingNotifications();
}

ListBox* ListBox::findHandOffTarget (const MouseEvent& event)
{
    const auto motionOrientation = orientation == Orientation::vertical ? Orientation::horizontal : Orientation::vertical;

    for (auto* parent = getParentComponentWithType<ListBox>(); parent != nullptr; parent = parent->getParentComponentWithType<ListBox>())
    {
        if (parent->getOrientation() == motionOrientation && parent->isTouchLike (event))
            return parent;
    }

    return nullptr;
}

MouseEvent ListBox::translateEventFor (const MouseEvent& event, Point<float> localPosition, Component& target) const
{
    return event.withPosition (target.screenToLocal (localToScreen (localPosition)))
        .withLastMouseDownPosition (target.screenToLocal (localToScreen (gesture.downPosition)))
        .withSourceComponent (&target);
}

void ListBox::triggerLongPress()
{
    gesture.longPressed = true;

    const auto rowIndex = gesture.pressedRow;

    if (! isRowSelected (rowIndex))
        selectRow (rowIndex, false, sendNotification);

    setCurrentRow (rowIndex);

    // Touch sessions follow the finger; a mouse press with drag scrolling enabled drags like any mouse drag.
    const auto touchIndex = gesture.touchIndex;
    startDraggingSelectedRows (touchIndex, touchIndex >= 0 ? localToScreen (gesture.lastPosition) : Point<float>());
}

void ListBox::tapRow (int rowIndex, const MouseEvent& event)
{
    if (event.isTouch())
    {
        // Fingers have no modifiers: a tap toggles in multiple selection mode.
        if (selectionMode == SelectionMode::multiple && isRowSelected (rowIndex))
            deselectRow (rowIndex, sendNotification);
        else
            selectRow (rowIndex, false, sendNotification);
    }
    else
    {
        const auto modifiers = event.getModifiers();
        handleRowSelection (rowIndex, modifiers.isCommandDown() || modifiers.isControlDown(), modifiers.isShiftDown());
    }

    setCurrentRow (rowIndex);
    handleRowClick (rowIndex, event);
}

bool ListBox::startDraggingSelectedRows (int touchIndex, Point<float> screenPosition)
{
    if (! dragSourceEnabled || model == nullptr || isCurrentlyDragging())
        return false;

    const auto rows = getSelectedRows();
    const auto description = model->getDragSourceDescription (rows);

    if (description.isVoid())
        return false;

    auto data = DragAndDropData {}.withNativeObject (description);

    if (description.isString())
        data = data.withText (description.toString());

    dragSourceComponent = createDragSourceComponent (rows);

    auto options = DragAndDropSource::DragOptions {}.withData (data);

    if (dragSourceComponent != nullptr)
    {
        const Point<float> hotspot { dragSourceComponent->getWidth() * 0.5f, dragSourceComponent->getHeight() * 0.5f };

        options = options.withDragImageComponent (dragSourceComponent.get(), hotspot);
    }

    if (touchIndex >= 0)
        options = options.withTouchPointer (touchIndex, screenPosition);

    rowSelectedOnMouseUp = -1;

    return startDragging (options);
}

//==============================================================================
void ListBox::dispatchPendingNotifications()
{
    const BailOutChecker checker (this);

    if (const auto position = scroller.getOffset(); position != lastNotifiedScrollPosition)
    {
        lastNotifiedScrollPosition = position;

        if (onScroll)
        {
            onScroll (position);

            if (checker.shouldBailOut())
                return;
        }
    }

    if (visibleRowRange != lastNotifiedVisibleRows)
    {
        lastNotifiedVisibleRows = visibleRowRange;

        if (onVisibleRowsChanged)
        {
            onVisibleRowsChanged (visibleRowRange);

            if (checker.shouldBailOut())
                return;
        }
    }

    if (const auto state = getScrollState(); state != lastNotifiedScrollState)
    {
        lastNotifiedScrollState = state;

        if (onScrollStateChanged)
        {
            onScrollStateChanged (state);

            if (checker.shouldBailOut())
                return;
        }
    }

    if (model == nullptr || viewportSize <= 0.0f)
        return;

    const auto visibleEnd = jlimit (0.0f, getMaxScrollOffset(), scroller.getOffset()) + viewportSize;
    const auto distanceToEnd = getTotalContentSize() - visibleEnd;

    if (distanceToEnd > endReachedThreshold * viewportSize)
    {
        endReachedArmed = true;
    }
    else if (endReachedArmed)
    {
        endReachedArmed = false;

        if (onEndReached)
            onEndReached();
    }
}

} // namespace yup
