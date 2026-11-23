# Declarative layout in `.yui`

Layout in `.yui` uses the five container components built on [`FlexBox`, `Grid`
and `StackLayout`](../component-layout.md): `Row`, `Column`, `FlexBox`, `Grid` and
`Stack`. Every `Component` carries the layout item properties (`flexGrow`,
`preferredWidth`, `margin`, `gridArea`, ...), so any built-in widget or
composite `component` can be placed inside any container without writing
`resized()` in C++.

```yui
tokens {
    property spacing: number = 8;
}

Column {
    padding: "12 16";
    gap: spacing;

    Row {
        id: header;
        preferredHeight: 36;
        gap: spacing;

        Label      { text: "SYNTH"; flexGrow: 1; }
        TextButton { text: "Init"; preferredWidth: 72; }
    }

    Grid {
        columns: "repeat(4, minmax(72px, 1fr))";
        rows: 1;
        gap: spacing;
        flexGrow: 1;

        Slider { id: cutoff; sliderType: rotaryVerticalDrag; }
        Slider { id: res;    sliderType: rotaryVerticalDrag; }
        Slider { id: env;    sliderType: rotaryVerticalDrag; }
        Slider { id: drive;  sliderType: rotaryVerticalDrag; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_synth_light.jpg#gh-light-mode-only" alt="A column holding a header row and a grid of four rotary sliders" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_synth_dark.jpg#gh-dark-mode-only" alt="A column holding a header row and a grid of four rotary sliders" width="560">
</p>

## Containers

| `.yui` type | C++ class | Layout model |
|---|---|---|
| `Row` | `RowComponent` | Horizontal `FlexBox` (`direction: row`) |
| `Column` | `ColumnComponent` | Vertical `FlexBox` (`direction: column`) |
| `FlexBox` | `FlexBoxComponent` | Configurable `FlexBox` |
| `Grid` | `GridComponent` | Two-dimensional track `Grid` |
| `Stack` | `StackComponent` | Overlapping `StackLayout` |

Composite components commonly inherit from a layout container so their internal
parts arrange themselves automatically:

```yui
component LabeledKnob : Column {
    property label: string = "VALUE";
    property alias value <=> dial.value;

    align: stretch;
    gap: 4;

    Slider { id: dial; sliderType: rotaryVerticalDrag; flexGrow: 1; }
    Label  { text: root.label; justification: center; preferredHeight: 18; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/documents_composite_light.jpg#gh-light-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/documents_composite_dark.jpg#gh-dark-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="560">
</p>

### Shared container properties

Every layout container inherits `padding` and `gap` from `LayoutComponent`:

- `padding`: a single number (`12`) or a string of 1, 2 or 4 edge values in CSS
  top-right-bottom-left order (`"8 16"`, `"4 8 12 8"`).
- `gap`: spacing in logical pixels between adjacent items or tracks.

### `FlexBox`, `Row` and `Column`

| Property | Values | Default |
|---|---|---|
| `direction` | `row`, `rowReverse`, `column`, `columnReverse` | `row` (`column` on `Column`) |
| `wrap` | `noWrap`, `wrap`, `wrapReverse` | `noWrap` |
| `justify` | `start`, `end`, `center`, `spaceBetween`, `spaceAround`, `spaceEvenly` | `start` |
| `align` | `start`, `end`, `center`, `stretch`, `baseline` | `stretch` |

`justify` places the items along the direction of the container, in the room
they leave:

```yui
component Strip : Row {
    property name: string = "";

    gap: 6; flexGrow: 1;

    TextButton { text: root.name; preferredWidth: 120; }
    TextButton { text: "B"; preferredWidth: 44; }
    TextButton { text: "C"; preferredWidth: 44; }
}

Column {
    gap: 8;

    Strip { name: "start";        justify: start; }
    Strip { name: "center";       justify: center; }
    Strip { name: "end";          justify: end; }
    Strip { name: "spaceBetween"; justify: spaceBetween; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_justify_light.jpg#gh-light-mode-only" alt="Four rows of three items, justified to the start, the center, the end and with the space between them" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_justify_dark.jpg#gh-dark-mode-only" alt="Four rows of three items, justified to the start, the center, the end and with the space between them" width="560">
</p>

`align` places them across it. `start`, `center` and `end` keep the height an
item asks for, and `stretch`, the default, gives the height of the row to the
items asking for none:

```yui
component Tray : Row {
    property name: string = "";
    property tall: number = -1;                 // -1 asks for no height

    gap: 4; flexGrow: 1;

    TextButton { text: root.name; flexGrow: 1; preferredHeight: root.tall < 0 ? -1 : 24; }
    TextButton { preferredWidth: 14; preferredHeight: root.tall * 0.7; }
    TextButton { preferredWidth: 14; preferredHeight: root.tall; }
}

Row {
    gap: 20;

    Tray { name: "start";   align: start;  tall: 56; }
    Tray { name: "center";  align: center; tall: 56; }
    Tray { name: "end";     align: end;    tall: 56; }
    Tray { name: "stretch"; align: stretch; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_align_light.jpg#gh-light-mode-only" alt="Four rows of three items of different heights, aligned to the start, the center and the end, and stretched" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_align_dark.jpg#gh-dark-mode-only" alt="Four rows of three items of different heights, aligned to the start, the center and the end, and stretched" width="640">
</p>

With `wrap: wrap` the items that don't fit go on a new line. Here a
[`repeat`](documents.md#repeating-components) makes them, each as wide as its
index says:

```yui
FlexBox {
    wrap: wrap;
    gap: 6;

    repeat i in 0 .. 10 {
        TextButton { text: format ("%d", i + 1); preferredWidth: 44 + (i % 3) * 28; preferredHeight: 30; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_wrap_light.jpg#gh-light-mode-only" alt="Ten items of three widths wrapping on three lines" width="480">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_wrap_dark.jpg#gh-dark-mode-only" alt="Ten items of three widths wrapping on three lines" width="480">
</p>

### `Grid`

| Property | Values | Default |
|---|---|---|
| `columns` | Integer count (`3` -> three `1fr` tracks) or CSS track list (`"180px 1fr 2fr"`, `"repeat(auto-fill, 96px)"`, `"minmax(80px, 1fr)"`) | empty |
| `rows` | Integer count or CSS track list | empty |
| `areas` | Comma-separated template rows (`"header header, nav content"`) | empty |
| `justify` | `start`, `end`, `center`, `stretch` | `stretch` |
| `align` | `start`, `end`, `center`, `stretch` | `stretch` |

`areas` names the cells of the tracks, a row of names for each row of the grid,
and an item takes the area its `gridArea` names, as a string:

```yui
Grid {
    columns: "96px 1fr";
    rows: "32px 1fr 24px";
    areas: "header header, nav content, footer footer";
    gap: 6;

    TextButton { text: "header";  gridArea: "header"; }
    TextButton { text: "nav";     gridArea: "nav"; }
    TextButton { text: "content"; gridArea: "content"; }
    TextButton { text: "footer";  gridArea: "footer"; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_grid_light.jpg#gh-light-mode-only" alt="A grid with a header and a footer across two columns, and a narrow and a wide cell between them" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_grid_dark.jpg#gh-dark-mode-only" alt="A grid with a header and a footer across two columns, and a narrow and a wide cell between them" width="560">
</p>

Without an area the items fill the cells in order. An item can span several
tracks, or be given its column and row:

```yui
Grid {
    columns: 4;
    rows: 3;
    gap: 6;

    TextButton { text: "gridColumnSpan: 2"; gridColumnSpan: 2; }
    TextButton { text: "1"; }
    TextButton { text: "gridRowSpan: 2"; gridRowSpan: 2; }

    repeat i in 2 .. 5 { TextButton { text: format ("%d", i); } }

    TextButton { text: "gridColumn: 1"; gridColumn: 1; gridRow: 2; gridColumnSpan: 3; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_grid-spans_light.jpg#gh-light-mode-only" alt="A grid of four columns where items span two columns, two rows, and three columns from the second one" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_grid-spans_dark.jpg#gh-dark-mode-only" alt="A grid of four columns where items span two columns, two rows, and three columns from the second one" width="560">
</p>

### `Stack`

`Stack` places all visible children in the same content box (after `padding`),
aligned individually or via the container's `justify` and `align` (`stretch`,
`start`, `center`, `end`). Use `Stack` for overlays, badges, backdrops and
layered pages.

```yui
style {
    .over { backgroundColor: palette.accent; textColor: palette.onAccent; }
}

Stack {
    TextButton { text: "content"; }
    TextButton { text: "3"; class: over; alignSelf: start; justifySelf: end; margin: 8; preferredWidth: 28; preferredHeight: 28; }
    TextButton { text: "saved"; class: over; alignSelf: end; justifySelf: center; margin: 10; preferredWidth: 120; preferredHeight: 28; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_stack_light.jpg#gh-light-mode-only" alt="A stack with an item filling it, a badge at its top right and a notice at its bottom center" width="480">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_stack_dark.jpg#gh-dark-mode-only" alt="A stack with an item filling it, a badge at its top right and a notice at its bottom center" width="480">
</p>

## Item properties

Item properties live on `Component`, are **never inherited** by descendants,
and notify the parent layout container automatically when changed in markup,
bindings, C++, or stylesheets:

| Property | Containers | Description |
|---|---|---|
| `preferredWidth`, `preferredHeight` | All | Requested item size (`-1` for `auto`) |
| `minWidth`, `maxWidth`, `minHeight`, `maxHeight` | `FlexBox`, `Grid` | Size constraints (`-1` for none) |
| `margin` | All | Outer spacing around the item (`8` or `"4 8 4 8"`) |
| `alignSelf`, `justifySelf` | All | Per-item alignment override (`auto`, `start`, `end`, `center`, `stretch`, `baseline`) |
| `flexGrow`, `flexShrink`, `flexBasis` | `FlexBox`, `Row`, `Column` | Flex factor and initial main size (`-1` for `auto`) |
| `order` | `FlexBox`, `Row`, `Column` | Visual/layout order within a flex container (`0` default) |
| `gridArea` | `Grid` | Named template area from `Grid.areas` |
| `gridColumn`, `gridRow` | `Grid` | 0-based track index (`-1` for auto-placement) |
| `gridColumnSpan`, `gridRowSpan` | `Grid` | Number of tracks spanned (`1` default) |

Use `preferredWidth` and `preferredHeight` inside layout containers rather than
setting `width` and `height`: `x`, `y`, `width` and `height` reflect the live
bounds written by the layout pass, whereas `preferredWidth` and
`preferredHeight` express the size the container should allocate.

```yui
Row {
    gap: 6;

    TextButton { text: "order: 1"; order: 1; preferredWidth: 80; }
    TextButton { text: "margin"; margin: "0 20"; preferredWidth: 80; }
    TextButton { text: "alignSelf: end"; alignSelf: end; preferredWidth: 120; preferredHeight: 24; }
    TextButton { text: "flexGrow: 1"; flexGrow: 1; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/layout_items_light.jpg#gh-light-mode-only" alt="A row whose first item is placed last by its order, with an item kept apart by its margin, one aligned to the end and one growing" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/layout_items_dark.jpg#gh-dark-mode-only" alt="A row whose first item is placed last by its order, with an item kept apart by its margin, one aligned to the end and one growing" width="640">
</p>

## Visibility, transforms and relayout

- **Hidden children are skipped**: setting `visible: false;` collapses an item
  out of `Row`, `Column`, `FlexBox`, `Grid` and `Stack` and triggers a parent
  relayout. To hide a component visually while keeping its slot in the layout,
  style `opacity: 0;` instead.
- **Transforms do not disturb layout**: layout containers read and write each
  child's untransformed base bounds (`getBounds()` / `setBounds()`), not
  `getBoundsInParent()`. Applying or animating `transform:` in a stylesheet (for
  example a hover scale or press offset) never shifts sibling items or triggers
  feedback loops during layout passes.
- **One layout for many changes**: a container is laid out once when a document
  is loaded and once when a stylesheet restyles its items, not at each child
  and each property. From C++, a `LayoutComponent::ScopedLayoutBatch` does the
  same for children added or removed in a loop, see [Layout
  components](../component-layout.md#layout-components).
- **Stylesheet-driven layout**: container and item properties are styleable, so
  rules can adapt spacing, track templates or flex shares per class or state:

```yui
style {
    Row.toolbar        { gap: 6; padding: "6 10"; }
    .compact Grid      { columns: 2; gap: 4; }
    .Inspector:disabled { preferredWidth: 0; }
}
```

## Root windows and fixed positioning

Outside layout containers (such as inside a plain `Component` or a root
`Window`), position children directly with `x`, `y`, `width` and `height`
expressions (`width: parent.width * 0.5;`). When `Window` is the root element of
a `.yui` document, `title`, `resizable` and `fullScreen` configure the hosting
native window or `YuiView`.

## See also

- [Component Layout](../component-layout.md) - the underlying C++ `FlexBox` and
  `Grid` algorithms and deliberate differences from browser CSS.
- [Stylesheets](styles.md) - styling layout properties and animating
  transforms.
- [Complete UIs in `.yui`](documents.md) - tokens, composite components and
  live document updates.
