# Declarative layout in `.yui`

Layout in `.yui` uses the five container components built on [`FlexBox`, `Grid`
and `StackLayout`](component-layout.md): `Row`, `Column`, `FlexBox`, `Grid` and
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
  <img class="light-only rounded" src="../_static/images/ui/yui-layout_synth_light.jpg#gh-light-mode-only" alt="A column holding a header row and a grid of four rotary sliders" width="480">
  <img class="dark-only rounded" src="../_static/images/ui/yui-layout_synth_dark.jpg#gh-dark-mode-only" alt="A column holding a header row and a grid of four rotary sliders" width="480">
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
  <img class="light-only rounded" src="../_static/images/ui/yui-documents_composite_light.jpg#gh-light-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="480">
  <img class="dark-only rounded" src="../_static/images/ui/yui-documents_composite_dark.jpg#gh-dark-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="480">
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

### `Grid`

| Property | Values | Default |
|---|---|---|
| `columns` | Integer count (`3` -> three `1fr` tracks) or CSS track list (`"180px 1fr 2fr"`, `"repeat(auto-fill, 96px)"`, `"minmax(80px, 1fr)"`) | empty |
| `rows` | Integer count or CSS track list | empty |
| `areas` | Comma-separated template rows (`"header header, nav content"`) | empty |
| `justify` | `start`, `end`, `center`, `stretch` | `stretch` |
| `align` | `start`, `end`, `center`, `stretch` | `stretch` |

### `Stack`

`Stack` places all visible children in the same content box (after `padding`),
aligned individually or via the container's `justify` and `align` (`stretch`,
`start`, `center`, `end`). Use `Stack` for overlays, badges, backdrops and
layered pages.

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

- [Component Layout](component-layout.md) - the underlying C++ `FlexBox` and
  `Grid` algorithms and deliberate differences from browser CSS.
- [Stylesheets](yui-styles.md) - styling layout properties and animating
  transforms.
- [Complete UIs in `.yui`](yui-documents.md) - tokens, composite components and
  live document updates.
