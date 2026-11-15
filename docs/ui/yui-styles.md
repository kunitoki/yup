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

A [`YuiDocument`](yui-documents.md) builds one stylesheet from the `palette`,
`drawing` and `style` blocks of its text and imports, and attaches it to its
root, so nothing else is needed.

For a hand-written UI, parse a sheet and attach it to any component. It styles
that component and everything inside it, including children added later:

```cpp
auto sheet = StyleSheet::parse (text);
if (sheet.failed())
    DBG (sheet.getErrorMessage());
else
    editor.setStyleSheet (sheet.getReference());

cutoff.setComponentID ("cutoff");
cutoff.addStyleClass ("knob");
```

`setStyleSheet (nullptr)` detaches it, removing every value it computed. A
sheet is immutable and reference counted, so several trees can share it.
`getStyleContext()->getDiagnostics()` lists the declarations that failed in the
last restyle, such as an unknown palette or a value that doesn't compile, and
`onDiagnostic` is called for each as it is met. Each `YuiDiagnostic` points at
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

Parts combine as in CSS: `Row.toolbar > TextButton:hover`. Pseudo-elements,
`:not()`, sibling combinators and selector lists separated by commas are not
supported.

A rule overrides a rule with a lower specificity (IDs count most, then classes,
attributes and pseudo-classes, then types), and a later rule overrides an
earlier one with the same specificity. Components are restyled when their
classes, states, IDs, parents or the properties a selector reads change, once
per frame.

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

Values are evaluated when a component is restyled, not each time something
they read changes. A value that must follow another property continuously
belongs in a drawing, or in a `.yui` instance as a [binding](yui-reactivity.md).

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
| `Slider` | `trackWidth` (4) and `thumbSize` (16) for linear sliders, `rotaryInset` (0.1 of the width) for rotary ones |
| `TextButton`, `ToggleButton`, `TextEditor`, `ComboBox` | `cornerRadius` (4), `outlineWidth` (1), `focusedOutlineWidth` (2) |
| `SwitchButton` | `cornerRadius` (unset: a pill), `thumbInset` (4) |
| `ProgressBar` | `cornerRadius` (unset: a pill) |
| `ScrollBar` | `thumbInset` (3), `cornerRadius` (unset: a pill) |
| `Label` | `textHeightProportion` (unset: the font keeps its height) |

```yui
style {
    TextButton { cornerRadius: 12; focusedOutlineWidth: 3; }
    .compact   { Slider.trackWidth: 2; Slider.thumbSize: 10; }
}
```

Like the colors they are kept per widget type (`TextButton::Style::cornerRadiusId`),
so a `cornerRadius` given to a card doesn't round the buttons inside it. They
only change the painting: a slider keeps its own hit area for the thumb.

### Declaring properties

A rule can declare a typed property, giving the matched components a default
other rules and expressions can use:

```yui
style { .knob { property glow: number = 0; } }
```

## Palettes

A `palette` block declares a [palette](component-styling.md#palettes) from a
few source colors, with the roles derived from them:

```yui
palette Ember {
    colors: "#1b1b1e #ff8800 #f2efe9";   // any format ThemePalette::fromString accepts
    mode: dark;                           // or light, dark by default
    accent: #ff9a1f;                      // override single roles
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

The roles are `background`, `surface`, `surfaceRaised`, `outline`, `text`,
`textMuted`, `accent` and `onAccent`; `palette.dark` is true for a dark palette.

## Drawings

`drawing: Name;` paints the matched components with a [drawing](yui-drawing.md)
declared in the document, and removes it when no rule selects it anymore. The
properties a drawing declares can then be set like any other, typed by the
drawing:

```yui
style {
    .knob          { drawing: Knob; bezelSize: 0.8; }
    .knob:pressed  { bezelSize: 0.84; }
}
```

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

See [Layout components](component-layout.md#layout-components) for the
containers and their properties.

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
- Numbers, integers and colors (component-wise in sRGB) are interpolated, as
  are arrays of the same length, element by element, so points and rectangles
  (stored as arrays of their components) interpolate too. Other values switch
  halfway. Transforms are not interpolated yet.
- Animated values sit between explicit values and stylesheet values, so a
  value set from C++ still wins.

## Shadows, effects and cursors

Three declarations style how a component sits on screen rather than what it
paints:

```yui
style {
    .card {
        cornerRadius: 8;
        shadow: 0 4 12 #00000060, 0 0 2 palette.accent.withAlpha (0.4);
        cursor: hand;
    }
    .card:disabled { effect: blur; }
}
```

- `shadow:` takes comma separated layers, each `x y [blur [spread]] color`
  like CSS, or `none`. A layer is the component's rounded rectangle, offset,
  grown by the spread and feathered by the blur, painted behind the component,
  the first layer on top. The color is an expression, so palette roles work.
  The corner radius is the `cornerRadius` property found for the component (its
  own, a stylesheet one or a parent's, or the [widget geometry](#widget-geometry)
  one), 0 when there is none. A glow is a
  shadow without offset: `shadow: 0 0 8 palette.accent;`.
- `effect:` installs a `ComponentEffect` registered by name, or `none`. Each
  component gets its own instance from the factory:

  ```cpp
  ComponentEffectRegistry::getInstance().registerEffect ("blur", []
  {
      return ComponentEffect::Ptr (new MyBlurEffect());
  });
  ```

- `cursor:` sets the mouse cursor: `none`, `default`, `arrow`, `text`, `wait`,
  `waitArrow`, `hand`, `crosshair`, `crossbones`, `resizeLeftRight`,
  `resizeUpDown`, `resizeTopLeftRightBottom`, `resizeBottomLeftRightTop` and
  `resizeAll`.

The paint margin of the component grows to hold its shadows. When no rule sets
one of these declarations anymore, the component gets back the cursor, effect
or paint margin it had before, unless the application changed it in between.
Unknown names and malformed shadows are reported as diagnostics. They switch
at once, transitions don't interpolate them.

## See also

- [The `.yui` language](yui-language.md) - the syntax of every block.
- [Data-driven drawing](yui-drawing.md) - the drawings a rule selects.
- [How YUI fits together](yui-architecture.md) - where stylesheet values sit in
  the property cascade.
- [Component styling](component-styling.md) - properties, the theme and
  palettes from C++.
