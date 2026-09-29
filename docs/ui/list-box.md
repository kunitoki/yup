# List Box

`ListBox` (module `yup_gui`) shows a scrollable list of rows, vertically or
horizontally, from a `ListBoxModel` you provide. Only the visible rows have
components, and they are recycled as the list scrolls, so a list of a million
rows costs the same as a list of twenty.

```cpp
class FruitModel : public yup::ListBoxModel
{
public:
    int getNumRows() override { return fruits.size(); }
    yup::String getRowText (int row) override { return fruits[row]; }

    yup::StringArray fruits { "Apple", "Banana", "Cherry" };
};

FruitModel model;
yup::ListBox list;
list.setModel (&model);
addAndMakeVisible (list);
```

The list never owns its model: keep the model alive for as long as the list
uses it, or call `setModel (nullptr)` first.

## The model

| Override | Purpose |
| --- | --- |
| `getNumRows()` | The row count (the only required override). |
| `getRowSize (row)` | The row's size along the scroll axis; 0 uses `ListBox::getRowSize()`. |
| `getRowText (row)`, `getRowIcon (row)` | What the built-in row shows. |
| `refreshRowComponent (row, isSelected, component)` | Custom row components, see below. |
| `selectedRowsChanged`, `rowClicked`, `rowDoubleClicked` | Interaction. |
| `returnKeyPressed (currentRow)`, `deleteKeyPressed (selectedRows)` | Keyboard. |
| `getDragSourceDescription (selectedRows)` | Dragging rows out, see [drag and drop](component-drag-and-drop.md). |

The list reads `getNumRows()` and `getRowSize()` only when it is told the data
changed (see [change notifications](#change-notifications)), so change the data
first and notify afterwards.

## Custom rows

Leave the component null in `refreshRowComponent()` to get the built-in
`ListBoxItem`, which shows `getRowText()` and `getRowIcon()` and truncates long
text with an ellipsis. Otherwise create or update your own component in place.
`reuseOrCreate<T>()` reuses the component when it already is a `T` (a recycled
row) and creates one otherwise:

```cpp
void refreshRowComponent (int row, bool isSelected, std::unique_ptr<yup::Component>& component) override
{
    auto& card = reuseOrCreate<CardComponent> (component);
    card.setTitle (items[row].title);
    card.setHighlighted (isSelected);
}
```

The component passed in may have shown a different row before, so update
everything it displays. For icons as drawables, use
`reuseOrCreate<ListBoxItem>()` and `setIconDrawable()`: the list keeps the
selected and hovered state of any `ListBoxItem` up to date.

Presses on a row's interactive children (buttons, sliders) belong to those
children. Call `setWantsMouseEvents (false, true)` on the row component itself so
that pressing its empty areas still selects and scrolls the list.

## Sizes, spacing and header

| Call | Effect |
| --- | --- |
| `setRowSize (size)` | Default row size along the scroll axis: 24 points vertically and 96 horizontally until set. |
| `setRowSpacing (gap)` | Gap between rows. Pressing a gap selects nothing. |
| `setContentInsets (leading, trailing)` | Empty space before the first and after the last row. |
| `setHeaderComponent (c)`, `setFooterComponent (c)` | Components that scroll with the rows, sized by their current height (vertical) or width (horizontal). |

## Orientation and nesting

`setOrientation (ListBox::Orientation::horizontal)` lays rows out left to right,
with the same features as a vertical list: the scrollbar moves to the bottom
edge, left and right arrows navigate, and a plain vertical mouse wheel scrolls
it. Built-in rows put their icon above the text.

Lists can be nested, for example horizontal carousels as rows of a vertical
list. A swipe that starts on the inner list but moves across it is handed over
to the enclosing list that scrolls in that direction.

## Change notifications

After changing the model, tell the list what changed:

| Call | When |
| --- | --- |
| `rowsInserted (start, count)` | Rows were inserted. |
| `rowsRemoved (start, count)` | Rows were removed. |
| `rowMoved (from, to)` | A row moved. |
| `rowsChanged (start, count)` | Rows changed content or size. |
| `updateContent()` | Anything else: re-reads the whole model. |

The selection, the current row and the row components follow their rows.
Removing a selected row reports the new selection through
`selectedRowsChanged()`, while rows merely shifting index do not. When the
change is entirely above the visible rows, the scroll position moves with it, so
what is on screen stays put.

## Scrolling

`scrollToRow (row, alignment, animated)` brings a row into view, aligned to the
`nearest` edge (the default, which does nothing when the row is visible),
`start`, `center` or `end`. `setScrollPosition()` and `getScrollPosition()` work
on the raw scroll offset.

To follow the scrolling:

- `onScroll (offset)` - on every change of the scroll position.
- `onVisibleRowsChanged (range)` - when the range of visible rows changes.
- `onScrollStateChanged (state)` - `idle`, `dragging` (following a finger) or
  `settling` (a fling, a bounce or an animated scroll).
- `onEndReached()` - once when the end comes within `setEndReachedThreshold()`
  (half the visible size by default), also when the content is shorter than the
  list. It fires again after the row count changes, which is what infinite
  loading needs:

```cpp
list.onEndReached = [this]
{
    const auto start = model.getNumRows();
    model.loadMore();
    list.rowsInserted (start, model.getNumRows() - start);
};
```

## Selection and the current row

`setSelectionMode()` picks `none`, `single` or `multiple` selection. The current
row is separate from the selection, like a keyboard focus within the list:

| Key | Effect |
| --- | --- |
| Arrows along the scroll axis, Page Up/Down, Home/End | Move the current row and select it. |
| Shift + arrows (multiple) | Extend the selection from the anchor to the current row. |
| Cmd/Ctrl + arrows (multiple) | Move the current row without selecting. |
| Space (multiple) | Toggle the current row. |
| Return | `returnKeyPressed (currentRow)`. |

`setCurrentRow()`, `getCurrentRow()` and `onCurrentRowChanged` give access to
it.

## Mouse and touch

With the mouse, a press selects straight away (Shift extends, Cmd/Ctrl toggles)
and a drag starts dragging the selected rows out of the list.

With touch:

- A drag scrolls the list and never changes the selection; releasing it flings
  with momentum.
- A tap selects on release, or toggles in multiple selection mode.
- Touching a list that is still moving stops it, without selecting.
- A long press selects the row and starts dragging the selected rows out.
- Touch never hovers.

`setMouseDragScrollingEnabled (true)` makes the mouse behave like a finger, so a
desktop list scrolls with momentum and overscroll too; clicks then select on
release and a long press drags rows out.

## Overscroll and pull-to-refresh

Dragged past its ends, the list stretches with a rubber band and springs back.
`setScrollOptions()` tunes the physics with a `KineticScroller::Options`:

```cpp
list.setScrollOptions (yup::KineticScroller::Options()
                           .withOverscrollEnabled (true)
                           .withOverscrollResistance (0.4f)
                           .withDeceleration (0.998f));
```

With `setPullToRefreshEnabled (true)`, pulling the content away from its start
past the indicator size and releasing calls `onRefresh` and keeps a spinning
indicator in view until you call `setRefreshing (false)`:

```cpp
list.setPullToRefreshEnabled (true);
list.onRefresh = [this]
{
    model.reload ([this] { list.updateContent(); list.setRefreshing (false); });
};
```

`KineticScroller` is also available on its own, for components that scroll
something other than a list: it is pure physics, driven by pointer positions and
frame times.

## Appearance

The theme paints the list through `ListBox::Style` colors (see
[component styling](component-styling.md)), and the pull-to-refresh indicator
size is the `ListBox::Style::refreshIndicatorSizeId` metric.
