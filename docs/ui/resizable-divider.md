# Resizable divider

`ResizableDivider` is a draggable bar (a sash) that splits an area into a
leading and a trailing pane, side by side or stacked. The divider only tracks
its own position: the parent stays in charge of the layout, usually calling
`layoutComponents` in its `resized()`.

```cpp
class SplitView : public yup::Component
{
public:
    SplitView()
    {
        divider.setMinPosition (yup::ResizableDivider::Length::pixels (120.0f));
        divider.setMinTrailingSize (yup::ResizableDivider::Length::percent (20.0f));
        divider.onPositionChanged = [this] (float) { resized(); };

        addAndMakeVisible (sidebar);
        addAndMakeVisible (content);
        addAndMakeVisible (divider);
    }

    void resized() override
    {
        divider.layoutComponents (getLocalBounds(), &sidebar, &content);
    }

private:
    yup::Component sidebar, content;
    yup::ResizableDivider divider;
};
```

`layoutComponents (area, leading, trailing)` splits `area` along the divider
axis: the leading pane, the bar, then the trailing pane. Either pane can be
`nullptr`. It sets the available extent to the size of `area`, so positions are
measured from its leading edge. To lay the panes out differently, read
`getEffectivePosition()` and place the three components yourself.

## Orientation and position

| Orientation | Bar | Panes | Cursor | Keys |
| --- | --- | --- | --- | --- |
| `Orientation::vertical` (default) | Vertical | Left and right | `ResizeLeftRight` | Left / Right |
| `Orientation::horizontal` | Horizontal | Top and bottom | `ResizeUpDown` | Up / Down |

The position is the distance from the leading edge to the bar, in the parent
space. It moves inside the *usable extent*: the available extent (the parent
width or height, or the value given to `setAvailableExtent`) minus the bar
thickness.

Positions, limits and snap points are `Length` values, either
`Length::pixels (value)` or `Length::percent (value)`. Percentages resolve
against the usable extent, so `Length::percent (50.0f)` gives two panes of the
same size. A new divider starts at 50%.

| Method | Purpose |
| --- | --- |
| `setPosition (pixels)`, `setPosition (Length)` | Move the divider. A position set before the extent is known is resolved once it is. |
| `setNormalizedPosition`, `getNormalizedPosition` | The position as a ratio of the usable extent. |
| `getPosition` | The clamped position, ignoring collapsing. |
| `getEffectivePosition` | The position to lay out, taking collapsing into account. |
| `setMinPosition`, `setMaxPosition` | Limit the position. The minimum wins when both conflict. |
| `setMinTrailingSize` | Keep the trailing pane at least this large. |

## Following the parent size

`setResizePolicy` decides what happens to the position when the available
extent changes:

- `ResizePolicy::proportional` (default) keeps the ratio between the panes.
- `ResizePolicy::preserveLeading` keeps the size of the leading pane, for a
  sidebar on the left or top.
- `ResizePolicy::preserveTrailing` keeps the size of the trailing pane, for an
  inspector on the right or bottom.

The divider remembers the position you set or dragged to, so shrinking the
parent until the limits clamp it and growing it back returns the divider to
where it was.

## Snapping, collapsing and resetting

- `addSnapPosition` / `setSnapPositions` make a drag snap to positions closer
  than `setSnapThreshold` (6 points by default).
- `setCanCollapseLeading` / `setCanCollapseTrailing` let a drag that goes past
  the minimum or maximum by `setCollapseThreshold` (32 points by default)
  collapse that pane. Dragging back expands it again. `setCollapsedSide`
  collapses or expands from code; expanding restores the position from before
  the collapse.
- `setDefaultPosition` sets the position restored by a double click.

## Keyboard

The divider takes keyboard focus. The arrow keys along its axis move it by
`setKeyboardStep` (10 points by default, five times that with Shift), and Home
and End move it to the minimum and maximum position.

## Callbacks

| Callback | Called |
| --- | --- |
| `onPositionChanged (float)` | With the effective position, whenever it changes. |
| `onDragDelta (float)` | During a drag, with the change of the effective position. |
| `onDragStarted`, `onDragEnded` | When a drag starts and ends. |
| `onCollapsedChanged (CollapseSide)` | When a pane collapses or expands. |
| `onDoubleClick` | After a double click, once the default position is restored. |

## Appearance

The theme draws a thin line that thickens and takes the accent color while the
bar is hovered, focused or dragged, and an optional grip in the middle.

| Method | Default |
| --- | --- |
| `setThickness` | 6 points, the interactive area. |
| `setLineThickness` | 1 point, the idle line. |
| `setActiveLineThickness` | 3 points, the hovered, focused or dragged line. |
| `setGripStyle` | `GripStyle::pill`; also `GripStyle::dots` or `GripStyle::none`. |

The colors are the `backgroundColor`, `lineColor`, `hoverColor`,
`draggingColor` and `gripColor` properties (`ResizableDivider::Style`). In
`.yui` documents and stylesheets the component is `ResizableDivider`, with
`orientation`, `gripStyle`, `resizePolicy`, `thickness`, `position` and
`normalized` properties and the `:dragging` and `:collapsed` states.
