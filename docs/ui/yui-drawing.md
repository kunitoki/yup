# Data-driven drawing

A *drawing* describes how a component paints itself as a tree of independent
shapes. Every property value is a `yup_expression` expression over the
component's reflected properties (`value`, `normalized`, `trackColor`, ...),
`bounds`, the component's local bounds, `defaultFont`, the theme's default
font, and the roles of the component's palette (`palette.accent`, ...). Because
the drawing reads the live component, it follows the component's state without
any paint code.

```cpp
#include <yup_gui/yup_gui.h>
```

Drawings are written in `.yui` files as `drawing` blocks. The `.yui` parser
turns each block into a `DataTree` with a `drawing` root, the same document the
editor tools and hot reload work on. `DrawingProgram` compiles that tree, and
`DrawingStyle` installs it on a component as a regular `ComponentStyle`.

## A first drawing

```yui
drawing Knob {
    let r: bounds.width / 2 - 4;
    let a0: 135deg;
    let a1: 405deg;
    let av: lerp (a0, a1, normalized);

    circle { id: well; center: bounds.center; radius: r; fill: backgroundColor; }

    layer {
        opacity: enabled ? 1 : 0.4;

        arc { id: track; center: bounds.center; radius: r - 2; from: a0; to: a1;
              stroke: trackColor; strokeWidth: 3; cap: round; }
        arc { id: value; center: bounds.center; radius: r - 2; from: a0; to: av;
              stroke: thumbColor; strokeWidth: 3; cap: round; }

        repeat i in 0 .. 11 {
            line { from: polar (bounds.center, r * 0.6, lerp (a0, a1, i / 10.0));
                   to: polar (bounds.center, r * 0.7, lerp (a0, a1, i / 10.0));
                   stroke: trackColor; strokeWidth: 1; }
        }
    }

    group {
        transform: transform.rotation (av + 90deg, bounds.center.x, bounds.center.y);
        rect { bounds: rect (bounds.center.x - 1, 4, 2, r * 0.5); fill: thumbColor; }
    }

    text { bounds: bounds.reduced (r * 0.5); text: format ("%.2f", value); fill: thumbColor; }
}
```

Elements are painted in document order, so later shapes draw on top. Each shape
has its own style: the well is filled, the arcs are stroked in different colors
and the ticks are repeated eleven times.

Angles are in radians, measured clockwise from the positive x axis (`deg`
converts degrees), the same convention for `arc` and `polar`.

## The drawing tree

Every block becomes an element of the drawing tree, and every `property: value;`
becomes a property of that element holding the expression text:

| `.yui`                          | Tree element                                          |
|---------------------------------|-------------------------------------------------------|
| `drawing Knob { ... }`          | `drawing`, with `name` = `Knob`                       |
| `circle { radius: r; ... }`     | `circle`, with `radius` = `r`                         |
| `let r: bounds.width / 2;`      | `let`, with `name` = `r` and `value` = `bounds.width / 2` |
| `repeat i in 0 .. 11 { ... }`   | `repeat`, with `name` = `i`, `from` = `0`, `to` = `11` |

The references below use the element and property names, which are the same in
the text and in the tree.

## Shapes

| Element   | Geometry                                                     | Draws                              |
|-----------|--------------------------------------------------------------|------------------------------------|
| `rect`    | `bounds`                                                     | A rectangle, optionally rounded.   |
| `ellipse` | `bounds`                                                     | An ellipse inscribed in `bounds`.  |
| `circle`  | `center`, `radius`                                           | A circle.                          |
| `arc`     | `center`, `radius`, `from`, `to`                             | An open arc between two angles.    |
| `line`    | `from`, `to`                                                 | A line between two points.         |
| `path`    | `data`                                                       | A path from SVG path data.         |
| `polygon` | `center`, `sides`, `radius`, `angle` (default `0`)           | A regular polygon.                 |
| `star`    | `center`, `points`, `innerRadius`, `outerRadius`, `angle`    | A star, `angle` defaults to `0`.   |
| `text`    | `bounds`, `text`                                             | A text label placed in `bounds`.   |

Geometry properties are required unless a default is listed. Shapes can't have
children.

## Paint properties

Every shape accepts these optional properties:

| Property                            | Type          | Effect                                              |
|-------------------------------------|---------------|-----------------------------------------------------|
| `fill`, `fillGradient`              | color, gradient | Fills the shape.                                  |
| `stroke`, `strokeGradient`          | color, gradient | Strokes the shape.                                |
| `strokeWidth`, `strokeMiterLimit`   | float         | Stroke geometry.                                    |
| `strokePosition`                    | enum          | `center`, `inside` or `outside`.                    |
| `cap`, `join`                       | enum          | Stroke cap (`butt`, `round`, `square`) and join.    |
| `feather`, `opacity`, `tint`        | float, float, color | Softens, fades or tints the shape.            |
| `blend`                             | enum          | Blend mode.                                         |
| `transform`                         | transform     | Transforms the shape.                               |
| `clip`, `clipPath`                  | rect, path    | Clips the shape.                                    |
| `visible`                           | bool          | Skips the shape when false.                         |

A shape with neither a fill nor a stroke is filled with the current fill color.
`rect` adds `cornerRadius` and the per-corner `topLeftRadius`, `topRightRadius`,
`bottomLeftRadius` and `bottomRightRadius`. `text` adds `font`, `justification`
(default `center`) and a `stroke` that outlines the glyphs; without `font` it
uses the theme's default font. Text is fitted in `bounds` at the font height, so
size it from the geometry with `font: defaultFont.withHeight (bounds.height * 0.3)`.
Bare enumeration values like `round` resolve to the property they are assigned to.

## Palette colors

`palette.<role>` reads a role of the palette the component is drawn with: the
closest one set with `Component::setPalette()` on the component or a parent,
or else the theme palette (see [Palettes](component-styling.md#palettes)).
The roles are `background`, `surface`, `surfaceRaised`, `outline`, `text`,
`textMuted`, `accent` and `onAccent`, and `palette.dark` is true for a dark
palette.

```yui
let muted: palette.textMuted;
circle { center: bounds.center; radius: 4; fill: enabled ? palette.accent : muted; }
rect { bounds: bounds; stroke: palette.outline.withAlpha (palette.dark ? 0.6 : 0.3); }
```

Palette colors follow palette switches and component palettes on the next
paint. Prefer the component's own color properties (`thumbColor`, ...) where
one exists: they come from the same palette through the theme, but can still be
overridden per component.

## Scopes

| Element  | Properties                         | Children | Effect                                                   |
|----------|------------------------------------|----------|----------------------------------------------------------|
| `let`    | `name`, `value`                    | no       | Names a value for the elements that follow it.           |
| `layer`  | `opacity`, `visible` (optional)    | yes      | Composites its children together at a shared opacity.    |
| `group`  | `transform`, `visible` (optional)  | yes      | Applies one transform to all its children.               |
| `repeat` | `name`, `from`, `to`               | shapes   | Draws its shapes once per integer in `from .. to - 1`.   |

- A `let` is visible to the elements after it in the same scope and in nested
  scopes, never before it or outside its scope. It can shadow a component
  property or a `let` of an outer scope, but not another `let` of its own scope.
  Names start with a letter and contain letters, digits and `_`; names starting
  with `_` are reserved.
- `layer` opacity fades its children as one image, so overlapping children don't
  show through each other. Children without their own paint inherit the solid
  fill and stroke of the caller, gradients are not inherited.
- `repeat` exposes the current index as an integer `let` named by `name`. Integer
  division truncates, so write `i / 10.0` for a fraction. A `repeat` holds shapes
  and `let`s only, it can't contain `layer`, `group` or another `repeat`, and its
  iteration count is capped at `4096` (constant ranges above `4096` fail to
  compile).
- In `.yui` documents, drawings can also read registered `BindingProvider` models
  directly (such as `params.cutoff`) in addition to component properties and
  `palette.<role>`.
- Every element accepts an optional `id`, used in error messages.

## Properties

A drawing can declare properties: inputs its users set to tune it without
editing it. They go at the top of the drawing and are visible to every element,
like a `let` of the root scope:

```yui
drawing Knob {
    property bezelSize: number = 0.8;
    property ring: color = palette.accent;
    property ticks: integer = 11;
    property showTicks: boolean = true;

    let r: min (bounds.width, bounds.height) * 0.5;
    circle { center: bounds.center; radius: r * bezelSize; fill: ring; }
}
```

- The type is `number`, `integer`, `boolean`, `string`, `color`, `point`,
  `rectangle` or an array of any of them (`number[]`).
- The default is optional and is an expression, so it can read the component,
  the palette, `bounds` and the properties declared before it. It must have the
  property's type; a `number` accepts any number. Without a default a property
  is `0`, `false`, empty or transparent. An array property can't have a default,
  it is empty until the component provides one.
- Every paint reads the property by name from the component the drawing is
  installed on, like `Component::findProperty`: a value set on the component, a
  value a stylesheet computed for it, or one of these on a parent. The default
  is used when none is found.

An array property is read with `lanes[i]` and `len (lanes)`, so a drawing can
paint a list whose length the component decides:

```yui
drawing Lanes {
    property lanes: number[];

    repeat i in 0 .. len (lanes) {
        line { from: point (0, lanes[i] * bounds.height); to: point (bounds.width, lanes[i] * bounds.height); }
    }
}
```

So a stylesheet sets them like any other property, typed by the drawing, and
can vary them per class, id or state:

```yui
style {
    .knob { drawing: Knob; }
    .knob:pressed { bezelSize: 0.84; }
    #drive .knob { ring: palette.text; }
}
```

From C++, set them on the component with `setProperty ("bezelSize", 0.7)`, and
list them with `DrawingProgram::getProperties()` or
`DrawingStyle::getProperties()`.

## Errors

`DrawingProgram::create` and `DrawingStyle::create` fail with a message that
starts with the path of the offending element and property:

```text
Knob/layer[0]/arc#value.to: Unknown symbol 'avv'
drawing/circle[0]: missing the required property 'radius'
drawing/repeat[0]/layer[0]: a 'layer' can't be nested inside a repeat
```

Elements are numbered by type within their parent, or named by their `id`. The
root is named by its `name` property, or `drawing`.

## Using a drawing from C++

`YuiParser::parse` turns `.yui` text into the drawing document, and both
`DrawingStyle` and `DrawingProgram` take the `drawing` tree inside it:

```cpp
const auto result = YuiParser::parse (text);
if (! result.wasOk())
    DBG (result.diagnostics.front().toString());

auto drawing = result.document.getChildWithName ("drawing");

auto style = DrawingStyle::create (slider, drawing);
if (style.wasOk())
    slider.setStyle (style.getReference());
else
    DBG (style.getErrorMessage());
```

`DrawingProgram` compiles the same tree when a drawing is painted by hand, for
example from a component's `paint`:

```cpp
auto program = DrawingProgram::create (slider, drawing);
if (program.wasOk())
    program.getReference()->draw (g, theme, slider);
```

A drawing is bound to the component it was created for, so install it on that
component only. Creating it for many components is cheap: components that give
the names the drawing reads the same types (several sliders drawn as knobs, for
example) share one compilation, and only keep their own evaluation state.

## Drawings in a stylesheet

A `.yui` document usually holds its drawings next to `palette` and `style`
blocks, and a stylesheet rule picks a drawing with `drawing: Name;`. The rule
also sets the properties the drawing reads, so the same drawing follows any
palette, state or class:

```yui
import "knob.yui";      // drawing VolumeKnob { ... }, reading trackColor and thumbColor

style {
    .knob { drawing: VolumeKnob; trackColor: palette.accent; thumbColor: palette.text; }
    .knob:hover { trackColor: palette.accent.darker (0.08); }
}

Component {
    Slider { id: volume; class: knob; sliderType: rotaryVerticalDrag; width: parent.width; height: parent.height; }
}
```

See [Stylesheets](yui-styles.md) for selectors, palettes, transitions and
shadows, and [Complete UIs in `.yui`](yui-documents.md) for loading such a
document with `YuiDocument`.

## Limitations

- A semi-transparent `layer` is rendered offscreen at the logical size of the
  component, so it can look soft on high-density displays.
- `repeat` takes a fixed range; collections of dynamic length (such as per-voice
  modulation arcs) and nested repeats are planned. This is why the synth plugin
  knob, ported to `synthknob.yui` in the `YuiDrawing` example, draws a single
  modulation lane.
- Image and SVG shapes, layer blending and masking are planned.
- Compile errors are reported against the drawing tree, not mapped back to
  source lines.

## See also

- [The `.yui` language](yui-language.md) - the syntax of every block.
- [Stylesheets](yui-styles.md) - selecting drawings and setting their
  properties from rules.
- [How YUI fits together](yui-architecture.md) - how drawings share their
  compilation and where their values come from.
- [Reactive bindings](yui-reactivity.md) - the same expressions keeping
  properties up to date.
