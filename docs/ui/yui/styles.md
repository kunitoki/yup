# Stylesheets

A stylesheet sets component properties from rules, like CSS: each rule has a
selector, matched against the live components, and declarations, whose values
are expressions over the matched component. A stylesheet styles any component
tree, one built from a `.yui` document or one written in C++, and never touches
the global theme.

```yui
palette Ember { colors: "#1b1b1e #ff8800 #f2efe9"; accent: #ff9a1f; }

style {
    .synth         { palette: Ember; }
    Slider         { drawing: Knob; thumbColor: palette.textMuted; }
    Slider:hover   { thumbColor: palette.accent.brighter (0.2); }
    #cutoff .label { textHeightProportion: 0.6; }
}
```

## Attaching a stylesheet

A [`YuiDocument`](documents.md) builds one stylesheet from the `palette`,
`drawing` and `style` blocks of its text and imports, and attaches it to its
root, so nothing else is needed.

For a hand-written UI, parse a sheet and attach it to any component. It styles
that component and everything inside it, including children added later:

```cpp
auto sheet = YuiParser::parseStyleSheet (text);
if (sheet.failed())
    DBG (sheet.getErrorMessage());
else
    editor.setStyleSheet (sheet.getReference());

cutoff.setComponentID ("cutoff");
cutoff.addStyleClass ("knob");
```

The text is parsed by `yup_gui_markup`. `yup_gui` alone takes a sheet as the
tree that text is parsed into, so an application leaving the module out writes
the tree in code and gives it to `StyleSheet::fromTree`:

```cpp
auto sheet = StyleSheet::fromTree (DataTree ("style", {
    DataTree ("rule", { { "selector", "Slider:hover" } }, {
        DataTree ("declaration", { { "name", "opacity" }, { "value", "0.5" } }) }) }));
```

`setStyleSheet (nullptr)` detaches it, removing every value it computed. A
sheet is immutable and reference counted, so several trees can share it.
`getStyleContext()->getDiagnostics()` lists the declarations that failed in the
last restyle, such as an unknown palette or a value that doesn't compile, and
`onDiagnostic` is called for each as it is met. Each `DataTreeDiagnostic` points at
the declaration element, which a `YuiDocument` turns into a line and column.

In `.yui`, `id` and `class` set the same thing; `class` takes several names
separated by spaces or commas:

```yui
Slider { id: cutoff; class: "knob large"; }
```

## Selectors

| Selector | Matches |
|---|---|
| `Slider` | Components of the type, or of a type derived from it (composite components included) |
| `.knob` | Components with the class |
| `#cutoff` | The component with the ID |
| `*` | Any component |
| `:root` | The component the sheet is attached to |
| `:hover` (or `:hovered`), `:pressed` (or `:active`), `:focus` (or `:focused`), `:focus-within`, `:disabled`, `:enabled` | Interaction states |
| `:checked`, `:dragging`, ... | Any boolean state property of the component's type |
| `[label]` | Components having the property |
| `[sliderType=rotary]`, `[text!="Off"]` | Components whose property has, or hasn't, the value |
| `A B`, `A > B` | `B` inside `A`, or directly inside it |
| `A, B` | Components matching either `A` or `B` (each branch keeps its own specificity) |

Parts combine as in CSS: `Row.toolbar > TextButton:hover` or
`ComboBox:hover:enabled, ComboBox:open`. Pseudo-elements, `:not()` and sibling
combinators are not supported.

```yui
style {
    TextButton          { cornerRadius: 6; }
    .primary            { backgroundColor: palette.accent; textColor: palette.onAccent; }
    #reset              { textColor: #ff5544; outlineColor: #ff5544; }
    [text="[text]"]     { cornerRadius: 20; }
    .bar > ToggleButton { cornerRadius: 0; }
    :checked            { backgroundToggledColor: #2e9e5b; }
    :disabled           { opacity: 0.35; }
}

Row {
    class: bar; gap: 8;

    TextButton   { text: "Type"; flexGrow: 1; }
    TextButton   { text: ".class"; class: primary; flexGrow: 1; }
    TextButton   { id: reset; text: "#id"; flexGrow: 1; }
    TextButton   { text: "[text]"; flexGrow: 1; }
    ToggleButton { text: ":checked"; checked: true; flexGrow: 1; }
    TextButton   { text: ":disabled"; enabled: false; flexGrow: 1; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_selectors_light.jpg#gh-light-mode-only" alt="Six buttons, each styled by a rule matching its type, its class, its id, one of its properties or its state" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_selectors_dark.jpg#gh-dark-mode-only" alt="Six buttons, each styled by a rule matching its type, its class, its id, one of its properties or its state" width="640">
</p>

A rule overrides a rule with a lower specificity (IDs count most, then classes,
attributes and pseudo-classes, then types), and a later rule overrides an
earlier one with the same specificity. Components are restyled when their
classes, states, IDs, parents or the properties a selector reads change, once
per frame.

`Component::setHovered` and `Component::setPressed` put a component in the
`:hover` and `:pressed` states with no pointer, which is how a tool or a test
shows what those rules do.

## Values and precedence

Declaration values are expressions, evaluated for the component the rule
matches. They can read its properties (`value`, `enabled`, `self.` or
`parent.` prefixed), its palette roles (`palette.accent`), and the document
tokens. The declaration name is a property of the component, typed by its
descriptor, so `trackColor: #ff8800;` is a color and `cornerRadius: 4;` a
number. A rule can also set a property the component doesn't declare, which
its children read.

When reading a property, each component on the way up the parent chain is
asked first for a value set in C++, then for the value of a running
transition or animation, then for the stylesheet value; the theme and the
default come last. So:

- A value set with `setProperty` always wins over the stylesheet, which never
  overwrites it.
- Inherited properties flow down to the descendants reading the same name:
  `.panel { glow: 4; }` reaches everything in the panel with no value of its
  own. Widget colors and geometry are kept per widget type (a slider stores
  `thumbColor` as `Slider::Style::thumbColorId`), so a container sets them with
  a [qualified name](#qualified-names), `.panel { Slider.thumbColor: ...; }`,
  or a rule matching the widgets, `.panel Slider { thumbColor: ...; }`.
- Layout item properties (`flexGrow`, `margin`, ...) and the properties of
  composite components aren't inherited, they only apply to the matched
  component.

Some state properties, such as a slider's `sliderType` or a label's
`justification`, can be styled too: the stylesheet calls the component's
setter, and gives the original value back when no rule sets it anymore. What
the user edits or the component reports (`value`, `text`, `checked`,
`dragging`, the bounds, ...) can't be styled, and a rule setting it is
reported as a diagnostic.

Values are compiled once per component and track the properties, ancestors,
named siblings, models, and palette roles they read: when any of those
dependencies changes, the component is marked dirty and re-evaluated on the
next restyle pass.

### Qualified names

A declaration can target the components of one type inside the matched one:

```yui
style {
    .panel { Slider.thumbColor: palette.accent; TextButton.cornerRadius: 2; }
}
```

The value applies to every `Slider` inside `.panel`, at any depth, and is
evaluated for each of them, with their own palette and properties. A rule
matching a slider directly wins over a qualified declaration from an ancestor,
the closest ancestor wins over the farther ones.

### Widget geometry

The default theme paints a few geometry values from properties, so a
stylesheet can reshape the built-in widgets without a drawing:

| Widget | Properties (default) |
|---|---|
| `Slider` | `trackWidth` (4) and `thumbSize` (16) for linear sliders; for rotary ones, whose dial is a circle as large as the smaller side allows, as proportions of that side, `rotaryInset` (0.1), `rotaryArcWidth` (0.075), `rotaryDialInset` (0.105), `rotaryDialOutlineWidth` (0.0175), `rotaryPointerInset` (0.175) and `rotaryPointerWidth` (0.03) |
| `TextButton`, `ToggleButton`, `TextEditor`, `ComboBox` | `cornerRadius` (4), `outlineWidth` (1), `focusedOutlineWidth` (2) |
| `PopupMenu` | `cornerRadius` (4), `outlineWidth` (1), `itemCornerRadius` (2), `itemHeight` (22), `verticalPadding` (4) |
| `SwitchButton` | `cornerRadius` (unset: a pill), `thumbInset` (4) |
| `ProgressBar` | `cornerRadius` (unset: a pill) |
| `ScrollBar` | `thumbInset` (3), `cornerRadius` (unset: a pill) |
| `Label` | `textHeightProportion` (unset: the font keeps its height) |
| `Image` | `source`, `fitting`, `justification`, `imageOpacity` |

Audio widgets in `yup_audio_gui` (`KMeterComponent`, `MidiKeyboardComponent`,
`AudioGraphComponent`, `SpectrumAnalyzerComponent`, `CartesianPlane`, `XYPad`)
also register their themeable colors, properties and signals for `.yui` markup
and stylesheets.

```yui
style {
    TextButton { cornerRadius: 12; focusedOutlineWidth: 3; }
    .compact   { Slider.trackWidth: 2; Slider.thumbSize: 10; }
    PopupMenu  { cornerRadius: 8; itemCornerRadius: 5; itemHeight: 26; }
}
```

A transient `PopupMenu` opened relative to a widget (such as a `ComboBox`
dropdown or a context menu using `PopupMenu::Options::withRelativePosition()`)
is styled by that widget's stylesheet context, so `PopupMenu`, combinators like
`ComboBox > PopupMenu`, and qualified declarations like
`.toolbar { PopupMenu.backgroundColor: ...; }` match even when the popup is
mounted on a top-level popup container or as a desktop window. Any submenus
opened from that menu inherit their originating parent `PopupMenu`'s resolved
style automatically.

Like the colors they are kept per widget type (`TextButton::Style::cornerRadiusId`),
so a `cornerRadius` given to a card doesn't round the buttons inside it. They
only change the painting: a slider keeps its own hit area for the thumb.

Three widgets as the theme draws them, and with their geometry set by rules:

```yui
style {
    .styled TextButton   { cornerRadius: 16; outlineWidth: 2; }
    .styled Slider       { trackWidth: 10; thumbSize: 24; }
    .styled SwitchButton { cornerRadius: 4; thumbInset: 7; }
}

Row {
    gap: 32;

    Column {
        gap: 10; flexGrow: 1;

        TextButton   { text: "Default"; flexGrow: 1; }
        Slider       { value: 0.6; flexGrow: 1; }
        SwitchButton { checked: true; flexGrow: 1; }
    }

    Column {
        class: styled; gap: 10; flexGrow: 1;

        TextButton   { text: "Styled"; flexGrow: 1; }
        Slider       { value: 0.6; flexGrow: 1; }
        SwitchButton { checked: true; flexGrow: 1; }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_geometry_light.jpg#gh-light-mode-only" alt="A button, a slider and a switch as the theme draws them, and with another corner radius, track and thumb" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_geometry_dark.jpg#gh-dark-mode-only" alt="A button, a slider and a switch as the theme draws them, and with another corner radius, track and thumb" width="560">
</p>

### Declaring properties

A rule (or a `style`, `drawing`, `shader`, `tokens` or `component` block) can
declare a typed property, giving matched components a known type and optional default that
other rules and expressions can use without emitting an undeclared-property
warning (`DataTreeDiagnostic::Severity::warning`):

```yui
style { .knob { property glowAmount: number = 0; } }
```

## Palettes

A `palette` block declares a [palette](../component-styling.md#palettes) from a
few source colors, with the roles derived from them:

```yui
palette Ember {
    colors: "#1b1b1e #ff8800 #f2efe9";   // any format ThemePalette::fromString accepts
    mode: dark;                           // or light, dark by default
    accent: #ff9a1f;                      // override single roles
    onAccent: colors.white;               // or name one of the Colors constants
}
```

`palette: Ember;` scopes it to the matched components and their contents, the
same as `Component::setPalette` but undone when the rule stops matching. A
palette set in C++ wins over it. Values reading `palette.<role>` use the
palette of the component they are evaluated for, and are evaluated again when
that palette changes, including theme palette switches:

```yui
style {
    :root   { palette: Ember; }
    .light  { palette: Porcelain; }
    .knob   { trackColor: palette.accent; }   // Ember or Porcelain, by where the knob is
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_stylesheet_light.jpg#gh-light-mode-only" alt="Widgets under the theme palette, and in a panel a rule gives another palette to, with a class coloring the primary buttons" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_stylesheet_dark.jpg#gh-dark-mode-only" alt="Widgets under the theme palette, and in a panel a rule gives another palette to, with a class coloring the primary buttons" width="560">
</p>

The roles are `background`, `surface`, `surfaceRaised`, `outline`, `text`,
`textMuted`, `accent` and `onAccent`; `palette.dark` is true for a dark palette.

Any other name declares a custom color of the palette, for the colors a skin
needs beyond the roles. Names are made of letters, digits and `_`, and don't
start with a digit. Read them as `palette.<name>`:

```yui
palette Walnut {
    colors: "#1b1b1b #3086c6 #d9dbd4";
    wood: #1e0e06;
    needle: #f4f4f0;
}

palette Maple {
    colors: "#ece9e1 #3086c6 #23241f";
    mode: light;
    wood: #a8784a;
    needle: #b3261e;
}

style {
    #panel                { palette: Walnut; }
    #panel[scheme=light]  { palette: Maple; }
}
```

Give every palette a skin switches between the same custom colors: a name the
palette lacks reads as a transparent color. In C++ they are set with
`ThemePalette::setCustomColor`.

A palette set by a rule replaces the palette of the theme where it applies,
whatever the theme is. For it to follow the theme, declare it twice under one
name, once for each `mode`: the two blocks are the dark and the light version
of one palette, and `palette: Name;` gives the one for the mode of the
application theme, again when that changes.

```yui
palette Skin {
    colors: "#1b1b1b #3086c6 #d9dbd4";      // mode: dark, the default
    wood: #1e0e06;
}

palette Skin {
    colors: "#ece9e1 #3086c6 #23241f";
    mode: light;
    wood: #a8784a;
}

style {
    :root { palette: Skin; }                 // the dark Skin under a dark theme, the light one under a light theme
}
```

A palette declared once is used under both themes, and a name can't be declared
twice for the same mode. To choose between palettes any other way, give them
their own names and select them with rules, as `#panel[scheme=light]` does
above. `theme.dark`, true when the palette of the application theme is dark,
can take part in the choice: `scheme: theme.dark == invert.checked ? "light" : "dark";`
follows the theme until a toggle inverts it.

## Drawings

`drawing: Name;` paints the matched components with a [drawing](drawing.md)
declared in the document, and removes it when no rule selects it anymore. The
properties a drawing declares can then be set like any other, typed by the
drawing:

```yui
style {
    .knob          { drawing: Knob; bezelSize: 0.8; }
    .knob:pressed  { bezelSize: 0.84; }
}
```

So one drawing gives as many looks as there are rules setting its properties:

```yui
drawing Meter {
    property level: number = 0.5;
    property tone: color = palette.accent;
    property segments: integer = 12;

    repeat i in 0 .. segments {
        rect { bounds: rect (i * bounds.width / segments + 1, 0, bounds.width / segments - 2, bounds.height);
               cornerRadius: 2; fill: i < level * segments ? tone : palette.outline.withAlpha (0.4); }
    }
}

style {
    .meter { drawing: Meter; }
    .hot   { tone: #ff5544; level: 0.9; }
    #fine  { segments: 32; level: 0.4; }
}

Column {
    gap: 10;

    Component { class: meter; flexGrow: 1; }
    Component { class: "meter hot"; flexGrow: 1; }
    Component { id: fine; class: meter; flexGrow: 1; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_drawing_light.jpg#gh-light-mode-only" alt="Three meters painted by one drawing, with the level, the color and the number of segments rules give them" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_drawing_dark.jpg#gh-dark-mode-only" alt="Three meters painted by one drawing, with the level, the color and the number of segments rules give them" width="560">
</p>

## Layout

Layout container properties (`gap`, `padding`, `justify`, ...) and item
properties (`flexGrow`, `preferredWidth`, `margin`, ...) can be styled, and a
change relays the container out, so a stylesheet can rearrange an interface:

```yui
style {
    Row.toolbar         { gap: 4; padding: "4 8"; }
    .toolbar > .spacer  { flexGrow: 1; }
    .compact Grid       { columns: 2; }
}
```

The same toolbar twice, the second one with one more class:

```yui
style {
    Row.toolbar        { gap: 6; }
    .toolbar > .spacer { flexGrow: 1; }
    Row.compact        { gap: 2; justify: center; }
    .compact > .spacer { flexGrow: 0; preferredWidth: 24; }
}

Column {
    gap: 12;

    Row { class: toolbar; flexGrow: 1;
          TextButton { text: "Open"; preferredWidth: 72; } TextButton { text: "Save"; preferredWidth: 72; }
          Component { class: spacer; } TextButton { text: "Help"; preferredWidth: 72; } }

    Row { class: "toolbar compact"; flexGrow: 1;
          TextButton { text: "Open"; preferredWidth: 72; } TextButton { text: "Save"; preferredWidth: 72; }
          Component { class: spacer; } TextButton { text: "Help"; preferredWidth: 72; } }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_layout_light.jpg#gh-light-mode-only" alt="A toolbar with its last button pushed to the end, and the same toolbar gathered at the center by another class" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_layout_dark.jpg#gh-dark-mode-only" alt="A toolbar with its last button pushed to the end, and the same toolbar gathered at the center by another class" width="560">
</p>

See [Declarative layout in `.yui`](layout.md) and
[Layout components](../component-layout.md#layout-components) for the containers
and their properties.

## Transitions and animations

A `transition:` declaration animates a property when a restyle changes its
value, from the value shown before to the new one:

```yui
style {
    .knob {
        bezelSize: 0.8;
        trackColor: palette.accent;
        transition: bezelSize 150ms easeOut, trackColor 200ms;
    }
    .knob:hover { trackColor: palette.accent.darker (0.08); }
    .knob:pressed { bezelSize: 0.84; }
}
```

Each entry is a property name, a duration and optionally an easing and a delay
(the second time). Times take `ms` or `s`. Easings are `linear` (the default),
`easeIn`, `easeOut`, `easeInOut` and `cubicBezier (x1, y1, x2, y2)`.

An `animation:` declaration plays `@keyframes`:

```yui
style {
    .alert { animation: pulse 1s easeInOut infinite alternate; }

    @keyframes pulse {
        from { glow: 0; }
        to { glow: 8; }
    }
}
```

Besides the name, the duration, the easing (applied between each pair of
keyframes) and the delay, an entry takes an iteration count (a number or
`infinite`), a direction (`normal`, `reverse`, `alternate`, `alternateReverse`)
and a fill mode (`none`, `forwards`, `backwards`, `both`).

- A component styled for the first time (the sheet attached, the document
  built, a child added) shows its values at once. Only later restyles
  transition.
- The transitions of the new style apply. A transition it doesn't declare, or
  `transition: none`, jumps to the end. A transition whose target changes
  restarts from the value shown at that moment.
- A transition needs a value on both sides. Declare the resting value of a
  drawing property in the base rule, since its default lives in the drawing.
- An animation restarts when the `animation:` text changes and stops when no
  rule declares it. A missing `from` or `to` keyframe uses the value the style
  gives the property.
- Numbers, integers, `transform` values, `filter` chains and colors
  (component-wise in linear RGB) are interpolated, as are arrays of the same
  length, element by element, so points and rectangles (stored as arrays of
  their components) interpolate too. Matching `filter` primitives interpolate
  step by step, padding a shorter or `none` chain with identity operations
  (`0deg`, `0px`, `1`, transparent `drop-shadow`), while mismatched primitive
  types switch halfway like other discrete values.
- Stylesheet `transform:` values live in their own style layer and compose with
  any explicit C++ `Component::setTransform()` without overwriting it or
  affecting parent layout bounds.
- Animated values sit between explicit values and stylesheet values, so a
  value set from C++ still wins.
- The inputs of a [shader](shaders.md#animating-a-shader) and the properties
  of the [3D elements](scenes.md#styling-and-animating) are properties like the
  others: a transition or an animation blends the uniform of an effect, the
  position of a `Node3D`, the color of a `Light3D` or a factor of a
  `Material3D`.

## Shadows, glows, filters, effects and cursors

Five declarations style how a component is composited or interacted with rather
than what it paints internally:

```yui
style {
    .card {
        cornerRadius: 8;
        shadow: 0 4 12 #00000060;
        glow: 10 2 palette.accent.withAlpha (0.4);
        cursor: hand;
    }
    .card:hover    { filter: brightness(108%) drop-shadow(0 6 10 #00000050); }
    .card:disabled { filter: grayscale(100%) opacity(60%); effect: customHalftone; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_shadows_light.jpg#gh-light-mode-only" alt="Buttons given a shadow, a glow and a filter by a stylesheet" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_shadows_dark.jpg#gh-dark-mode-only" alt="Buttons given a shadow, a glow and a filter by a stylesheet" width="560">
</p>

- `shadow:` takes comma-separated layers, each `x y [blur [spread]] color`
  like CSS, or `none`. Each layer offsets, spreads and feathers the component's
  rounded rectangle behind the component, the first layer on top. The color is
  an expression, so palette roles work. The corner radius comes from the
  component's `cornerRadius` property (or `0` when unset).
- `glow:` takes comma-separated centered glow layers, each `blur [spread] color`,
  or `none`. Glows compose cleanly alongside `shadow:` without overwriting shadow
  layers from other matching rules.
- `filter:` applies a space-separated chain of standard CSS `<filter-function>`
  operations to the component's rendered output, or `none`:
  - `blur([radius])` (non-negative length, `0px` default)
  - `brightness([amount])`, `contrast([amount])`, `saturate([amount])`
    (number or `%`, `>= 0`, `1` / `100%` default)
  - `grayscale([amount])`, `invert([amount])`, `opacity([amount])`,
    `sepia([amount])` (number or `%`, clamped to `[0, 1]`, `1` default)
  - `hue-rotate([angle])` (`deg`, `rad`, `grad`, `turn` or `0`)
  - `drop-shadow(offsetX offsetY [blur] [color])` (follows the rendered alpha
    silhouette of the component; color accepts `.yui` expressions such as
    `palette.accent.withAlpha (0.5)`)
  Consecutive color-matrix filters collapse into a single GPU pass, and `filter:`
  runs in its own style layer so it never overwrites a C++ `setComponentEffect()`.
  The filter covers what the component paints in its paint margin and its
  `shadow:` and `glow:` layers too, as in CSS.
- `effect:` draws the component through a shader, or `none`. It takes the name
  of a [`shader` block](shaders.md#as-the-effect-of-a-component) of the
  document, whose inputs are then properties the rules set and transition:

  ```yui
  shader Crt { fragment: "fx/crt.frag"; property intensity: number = 0.5; }

  style {
      .display       { effect: Crt; transition: intensity 200ms; }
      .display:hover { intensity: 0.9; }
  }
  ```

  or the name of a `ComponentEffect` factory registered by the application in
  `ComponentEffectRegistry`. A shader of the document wins over a registered
  effect of the same name:

  ```cpp
  ComponentEffectRegistry::getInstance().registerEffect ("customHalftone", []
  {
      return ComponentEffect::Ptr (new CustomHalftoneEffect());
  });
  ```

- `cursor:` sets the mouse cursor: `none`, `default`, `arrow`, `text`, `wait`,
  `waitArrow`, `hand`, `crosshair`, `crossbones`, `resizeLeftRight`,
  `resizeUpDown`, `resizeTopLeftRightBottom`, `resizeBottomLeftRightTop` and
  `resizeAll`.

The functions of `filter:` on the same button:

```yui
style {
    TextButton { backgroundColor: palette.accent; textColor: palette.onAccent; cornerRadius: 8; }

    .blur   { filter: blur(2px); }
    .shadow { filter: drop-shadow(0 6 8 palette.accent.withAlpha (0.7)); }
    .bright { filter: brightness(150%); }
    .gray   { filter: grayscale(100%); }
    .sepia  { filter: sepia(100%); }
    .hue    { filter: hue-rotate(120deg); }
    .invert { filter: invert(100%); }
}

Grid {
    columns: 4; rows: 2; gap: 16;

    TextButton { text: "none"; }
    TextButton { text: "blur"; class: blur; }
    TextButton { text: "drop-shadow"; class: shadow; }
    TextButton { text: "brightness"; class: bright; }
    TextButton { text: "grayscale"; class: gray; }
    TextButton { text: "sepia"; class: sepia; }
    TextButton { text: "hue-rotate"; class: hue; }
    TextButton { text: "invert"; class: invert; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_filters_light.jpg#gh-light-mode-only" alt="Eight buttons: one as it is, the others blurred, with a drop shadow, brighter, gray, sepia, with its hue turned and inverted" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_filters_dark.jpg#gh-dark-mode-only" alt="Eight buttons: one as it is, the others blurred, with a drop shadow, brighter, gray, sepia, with its hue turned and inverted" width="640">
</p>

A component that paints outside its bounds itself, as a drawing with a glow
around a lamp does, asks for the room with `paintMargin`, on the component or
in a rule: a number, or a string of 1, 2 or 4 values in the order `margin`
takes them.

```yui
style {
    .lamp { drawing: Lamp; paintMargin: 12; }
    .meter { drawing: Meter; paintMargin: "0 24"; }
}
```

Only what is painted changes: the bounds, the layout and the hit testing of
the component stay the same, and what it paints is still clipped to its
parent.

A lamp whose glow is wider than its component, cut to its bounds and then
given the room:

```yui
drawing Lamp {
    circle { center: bounds.center; radius: bounds.width * 0.85; fill: palette.accent.withAlpha (0.4); feather: 16; }
    circle { center: bounds.center; radius: bounds.width / 2 - 2; fill: palette.accent; }
}

style {
    .lamp { drawing: Lamp; }
    .room { paintMargin: 28; }
}

Row {
    gap: 96; justify: center; align: center;

    Component { class: lamp; preferredWidth: 40; preferredHeight: 40; }
    Component { class: "lamp room"; preferredWidth: 40; preferredHeight: 40; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/styles_paint-margin_light.jpg#gh-light-mode-only" alt="A lamp whose glow is cut to the square of its component, and the same lamp glowing all around once it has a paint margin" width="480">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/styles_paint-margin_dark.jpg#gh-dark-mode-only" alt="A lamp whose glow is cut to the square of its component, and the same lamp glowing all around once it has a paint margin" width="480">
</p>

The paint margin of the component grows automatically to hold its `shadow:`
and `glow:` layers, when they reach further than the one `paintMargin` asks
for. A `filter:` then adds what it spreads all of that over: three times the
radius of a `blur(...)`, and of a `drop-shadow(...)` with its offset. The parent
still cuts all of it where it ends: when a component sits closer to an edge of
its parent than its shadow reaches, give the parent a `paintMargin` too. When no
rule sets one of these declarations anymore, the component gets back the cursor,
effect or paint margin it had before, unless the application changed it in
between. Unknown names and malformed declarations are reported as diagnostics.

## See also

- [The `.yui` language](language.md) - the syntax of every block.
- [Data-driven drawing](drawing.md) - the drawings a rule selects.
- [Shaders](shaders.md) - the shaders `effect:` runs, and their inputs.
- [3D scenes](scenes.md) - styling and animating the elements of a `Scene3D`.
- [How YUI fits together](architecture.md) - where stylesheet values sit in
  the property cascade.
- [Component styling](../component-styling.md) - properties, the theme and
  palettes from C++.
