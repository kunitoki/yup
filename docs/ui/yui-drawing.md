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
editor tools and hot reload work on. `DrawingStyle` compiles that tree and
installs it on a component as a regular `ComponentStyle`.

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
| `pattern Dots { ... }`          | `pattern`, with `name` = `Dots` (`marker` and a nested `drawing` alike) |
| `use Leaf { ... }`              | `use`, with `drawing` = `Leaf`                        |

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
| `path`    | `data`                                                       | A path from SVG path data or a `path()` value. |
| `polygon` | `center`, `sides`, `radius`, `angle` (default `0`)           | A regular polygon.                 |
| `star`    | `center`, `points`, `innerRadius`, `outerRadius`, `angle`    | A star, `angle` defaults to `0`.   |
| `text`    | `bounds` or `anchor`, `text`                                 | A text label fitted in `bounds`, or starting from the `anchor` point on its baseline. |
| `image`   | `source`, `bounds` (default `bounds`)                        | A raster image from a file path or `data:image/...;base64,...` URI. |
| `svg`     | `source` or `data`, `bounds` (default `bounds`)              | An SVG from a file path (`source`) or inline SVG XML (`data` or `source`). |

Geometry properties are required unless a default is listed. Shapes can't have
children. `image` and `svg` also accept `fitting` (`scaleToFit` by default, or
`none`, `fitWidth`, `fitHeight`, `scaleToFill`, `fill`, `centerCrop`,
`centerInside`, `stretchWidth`, `stretchHeight`, `tile`) and `justification`
(`center` by default).

## Paint properties

Every shape accepts these optional properties:

| Property                            | Type          | Effect                                              |
|-------------------------------------|---------------|-----------------------------------------------------|
| `fill`, `fillGradient`, `fillPattern` | color, gradient, pattern name | Fills the shape.                    |
| `stroke`, `strokeGradient`, `strokePattern` | color, gradient, pattern name | Strokes the shape.            |
| `fillOpacity`, `strokeOpacity`      | float         | Fades the fill or the stroke only.                  |
| `strokeWidth`, `strokeMiterLimit`   | float         | Stroke geometry.                                    |
| `strokePosition`                    | enum          | `center`, `inside` or `outside`.                    |
| `dash`, `dashOffset`                | dash, float   | Dashes the stroke (`dash (6, 3)`, `"6, 3"`, `4` or a `number[]` property) and shifts the pattern start. |
| `cap`, `join`                       | enum          | Stroke cap (`butt`, `round`, `square`) and join.    |
| `feather`, `opacity`, `tint`        | float, float, color | Softens, fades or tints the shape.            |
| `blend`                             | enum          | Blend mode.                                         |
| `transform`                         | transform     | Transforms the shape.                               |
| `clip`, `clipPath`                  | rect, path    | Clips the shape.                                    |
| `color`                             | color         | The color `currentColor` stands for in the other properties. |
| `markerStart`, `markerMid`, `markerEnd` | marker name | Draws a [marker](#patterns-and-markers) at the vertices of a `path` or a `line`. |
| `visible`                           | bool          | Skips the shape when false.                         |

A shape with neither a fill nor a stroke is filled with the current fill color.
`fill: none` and `stroke: none` remove a paint instead, including one
[set by a group](#paint-set-by-groups). Bare enumeration values like `round`
resolve to the property they are assigned to.

`rect` adds `cornerRadius`, the per-corner `topLeftRadius`, `topRightRadius`,
`bottomLeftRadius` and `bottomRightRadius`, and `cornerSize` for elliptical
corners with a horizontal and a vertical radius (`cornerSize: size (8, 4)`).

`text` adds `font`, `letterSpacing` and a `stroke` that outlines the glyphs;
without `font` it uses the theme's default font. It is placed in one of two ways:

- fitted in `bounds` at the font height, aligned by `justification` (default
  `center`), so size it from the geometry with
  `font: defaultFont.withHeight (bounds.height * 0.3)`;
- starting from `anchor`, a point on its baseline, the way SVG places text.
  `textAnchor` (`start` by default, `middle` or `end`) tells which part of the
  text is on the point.

```yui
text { anchor: point (12, 20); textAnchor: middle; letterSpacing: 0.5; text: "yup"; fill: palette.text; }
```

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
| `layer`  | `opacity`, `blend`, `clip`, `clipPath`, `mask`, `maskMode`, `visible` (optional) | yes (+ `mask`, `filter`) | Composites its children into a transparency layer with optional blend mode, clip and masks, or draws them through a [filter](#filters). |
| `mask`   | `mode` (`alpha`, `invertedAlpha`, `luminance`, `invertedLuminance`), `visible` (optional) | yes | Draws a mask applied to the enclosing `layer`. |
| `clip`   | `bounds`/`rect`, `path`/`clipPath`, `visible` (optional) | yes | Clips its children to a rectangle and/or path. |
| `group`  | `transform`, `opacity`, `blend`, `color`, `visible` and [paint](#paint-set-by-groups) (optional) | yes | Applies a transform, an opacity, a blend mode and a paint to all its children. |
| `viewport` | `bounds`, `viewBox`, `fitting`, `justification`, `visible` (optional) | yes | Draws its children in a [view box](#view-boxes-and-viewports) mapped into `bounds`, clipped to them. |
| `drawing`, `use` | see [Reusing drawings](#reusing-drawings) | yes, no | Defines a drawing inside another, and paints a drawing by name. |
| `pattern`, `marker` | see [Patterns and markers](#patterns-and-markers) | yes | Define a tile to paint shapes with, and a shape drawn at the vertices of others. |
| `clipPath` | none                             | shapes   | Inside a `clip` or a `layer`: the shapes to [clip to](#clipping-to-shapes). |
| `repeat` | `name`, `from`, `to`               | shapes   | Draws its shapes once per integer in `from .. to - 1`.   |

- A `let` is visible to the elements after it in the same scope and in nested
  scopes, never before it or outside its scope. It can shadow a component
  property or a `let` of an outer scope, but not another `let` of its own scope.
  Names start with a letter and contain letters, digits and `_`; names starting
  with `_` are reserved.
- `layer` composites its children as one image so overlapping children don't
  show through each other, supports `blend` modes (`multiply`, `screen`, ...),
  and accepts a `mask` path/rect property and/or child `mask { ... }` blocks.
  Children without their own paint inherit the solid fill and stroke of the
  caller, gradients are not inherited.
- `group` fades and blends its children one by one: overlapping children show
  through each other at an `opacity` below 1. Use a `layer` to fade them as one image.
- `repeat` exposes the current index as an integer `let` named by `name`. Integer
  division truncates, so write `i / 10.0` for a fraction. A `repeat` holds shapes
  and `let`s only, it can't contain a `layer`, a `group`, a `clip`, a `viewport`,
  a `use`, a definition or another `repeat`, and its iteration count is capped at
  `4096` (constant ranges above `4096` fail to compile).
- In `.yui` documents, drawings can also read registered `BindingProvider` models
  directly (such as `params.cutoff`) in addition to component properties and
  `palette.<role>`.
- Every element accepts an optional `id`, used in error messages.

## View boxes and viewports

A drawing normally works in the pixels of its component. With a `viewBox` it
works in coordinates of its own instead, mapped into the bounds of the
component, like the `viewBox` and `preserveAspectRatio` of an SVG:

```yui
drawing Icon {
    viewBox: rect (0, 0, 24, 24);           // inside the drawing, `bounds` is this rectangle
    fitting: scaleToFit;                    // the default, see the fittings of `image`
    justification: center;                  // the default

    circle { center: bounds.center; radius: 10; fill: palette.accent; }

    viewport {
        bounds: rect (14, 14, 8, 8);
        viewBox: rect (0, 0, 100, 100);
        fitting: scaleToFill;
        rect { bounds: bounds; fill: palette.text; }
    }
}
```

- The drawing is not clipped to its `viewBox`: with `scaleToFill`, what falls
  outside the component is clipped by the component as usual.
- A `viewport` does the same for its children inside any rectangle, and clips
  them to it. Without a `viewBox`, their `bounds` is the rectangle moved to the
  origin. `bounds` defaults to the bounds around the viewport.
- A `let` declared outside keeps reading the bounds it was declared in.

## Paint set by groups

A `group` can set the paint of the shapes inside it, so they only say what they
do differently. The paint is inherited by the shapes written inside the group,
and by nested groups:

```yui
group {
    fill: palette.surface; stroke: palette.outline; strokeWidth: 2; color: palette.accent;

    rect { bounds: rect (2, 2, 20, 20); }                    // filled and stroked by the group
    circle { center: point (12, 12); radius: 5; fill: currentColor; stroke: none; }
    path { data: "M 2 12 L 22 12"; fill: none; }             // only stroked

    group { strokeWidth: 1; line { from: point (2, 2); to: point (22, 22); } }
}
```

- A group sets `fill`, `fillGradient`, `fillPattern`, `fillOpacity`, `stroke`,
  `strokeGradient`, `strokePattern`, `strokeOpacity`, `strokeWidth`,
  `strokeMiterLimit`, `dash`, `dashOffset`, `cap`, `join`, `feather`, `font`,
  `textAnchor`, `letterSpacing` and the three markers.
- A fill of any kind replaces the inherited fill of any kind, and the same goes
  for strokes. `fill: none`, `stroke: none` and `markerStart: none` remove them.
- A shape with an inherited fill and a stroke of its own is filled and stroked.
- `color` names a color for the group, read as `currentColor` by the paint of
  the group and of everything inside it. A shape can set its own.
- A paint can read `objectBounds`, the bounding box of the shape it paints, even
  when a group sets it: `fillGradient: sky.mappedTo (objectBounds)` stretches one
  gradient over every shape. It is not available to a `text` placed from an `anchor`.
- Text is only stroked by a `stroke` color, and markers are only inherited by
  `path` and `line` shapes outside a `repeat`.

Nothing is inherited unless a group sets it, so drawings without paint on their
groups work as before.

## Reusing drawings

A `drawing` written inside another is a definition: it paints nothing until a
`use` paints it, as often as needed. It is the `<symbol>` and `<use>` of SVG:

```yui
drawing Toolbar {
    drawing Leaf {
        property tone: color = palette.accent;
        viewBox: rect (0, 0, 10, 10);
        path { data: "M 5 0 L 10 10 L 0 10 Z"; fill: tone; }
        circle { center: point (5, 7); radius: 1; }           // painted with the paint of the use
    }

    use Leaf { bounds: rect (2, 2, 8, 8); fill: palette.text; }
    use Leaf { bounds: rect (14, 2, 8, 8); tone: palette.outline; fill: none; stroke: palette.text; opacity: 0.5; }
}
```

- `use` takes `bounds` (the bounds around it by default), `transform`,
  `opacity`, `blend`, `visible`, `color`, the [paint a group sets](#paint-set-by-groups)
  and the properties the drawing declares.
- Inside the drawing, `bounds` is the `bounds` of the `use`, or its `viewBox`
  mapped into them. The drawing is not clipped.
- The drawing sees the `let`s and the definitions visible where it is written,
  and the paint and the `currentColor` of where it is used.
- A property the `use` doesn't set has its default: it is not looked up on the
  component, and array properties can't be set.
- A definition is visible to the elements after it, in its scope and in nested
  ones. `use` also finds the other drawings of the document, or of the
  stylesheet, by name; from C++, `DrawingStyle::create` takes a function finding
  them. A drawing that ends up using itself fails to compile.

## Patterns and markers

A `pattern` is a tile, repeated to fill or stroke the shapes naming it with
`fillPattern` or `strokePattern`. A `marker` is drawn at the vertices of the
`path` and `line` shapes naming it with `markerStart`, `markerMid` or
`markerEnd`:

```yui
drawing Chart {
    pattern Dots {
        tile: rect (0, 0, 4, 4);                              // where a tile is, in the coordinates of the shape
        circle { center: bounds.center; radius: 1; fill: palette.accent; }
    }

    marker Arrow {
        size: size (6, 6); ref: point (3, 3); viewBox: rect (0, 0, 6, 6);
        path { data: "M 0 0 L 6 3 L 0 6 Z"; fill: palette.text; }
    }

    rect { bounds: bounds.reduced (4); fillPattern: Dots; }
    path { data: "M 4 20 L 12 8 L 20 14"; fill: none; stroke: palette.text; strokeWidth: 2; markerMid: Arrow; markerEnd: Arrow; }
}
```

| `pattern`       | Meaning                                                              |
|-----------------|----------------------------------------------------------------------|
| `tile`          | Required: the rectangle of one tile. Inside the pattern, `bounds` is the tile moved to the origin. |
| `viewBox`, `fitting`, `justification` | Optional: the coordinates the tile is drawn in, mapped into it. |
| `transform`     | Optional: transforms the tiles in the shape.                         |

| `marker`        | Meaning                                                              |
|-----------------|----------------------------------------------------------------------|
| `size`          | The size of the marker, `size (3, 3)` by default.                    |
| `ref`           | The point of the marker placed on the vertex, `point (0, 0)` by default. |
| `viewBox`       | Optional: the coordinates the marker is drawn in, mapped into `size`. |
| `orient`        | `auto` (the default) follows the direction of the path, `autoStartReverse` also turns the marker at the start around, an angle in radians is fixed. |
| `units`         | `strokeWidth` (the default) scales the marker by the stroke width of the shape, `userSpace` doesn't. |

- Both are definitions, like nested drawings: visible to the elements after
  them, and painted with the paint of where they are written.
- A tile is drawn once into an image and repeated. A tile reading the component
  is drawn again on every paint. Its content is clipped to the tile.
- `markerStart` is drawn at the first vertex of every sub-path, `markerEnd` at
  the last one unless the sub-path is closed, `markerMid` at the others. A shape
  inside a `repeat` can't have markers.

## Clipping to shapes

A `clip`, or a `layer`, clips to the shapes of the `clipPath` blocks inside it,
which are not painted. It is the `<clipPath>` of SVG:

```yui
clip {
    clipPath {
        circle { center: point (12, 12); radius: 8; }
        rect { bounds: rect (0, 0, 12, 24); transform: transform.rotation (10deg); }
    }

    rect { bounds: bounds; fill: palette.accent; }            // clipped to the circle and the rectangle together
}
```

The outlines of the shapes of a `clipPath` are added up, and several `clipPath`
blocks intersect. A `clipPath` holds shapes, `let`s and `repeat`s; a `text` has
no outline to clip to.

## Filters

A `layer` holding a `filter` block is drawn through it: its elements are painted
into a picture covering the `bounds` of the filter, the primitives of the filter
run on that picture one after the other, and the result of the last one is
painted with the `opacity`, the `blend` and the clips of the layer. It is the
`<filter>` of SVG:

```yui
layer {
    filter {
        bounds: rect (0, 0, 24, 24);                           // what is filtered: the bounds around it when omitted

        blur { in: sourceAlpha; deviation: 2 * amount; result: shadow; }
        offset { in: shadow; offset: point (1, 2); result: shadow; }
        composite { in: source; in2: shadow; }                 // the layer over its blurred, moved alpha
    }

    circle { center: point (12, 12); radius: 6; fill: palette.accent; }
}
```

| Primitive      | Properties                                           | What it makes                                             |
|----------------|------------------------------------------------------|-----------------------------------------------------------|
| `blur`         | `deviation`                                          | Its input blurred, by a standard deviation.               |
| `offset`       | `offset` (point)                                     | Its input moved.                                          |
| `flood`        | `color`                                              | A fill of a color.                                        |
| `colorMatrix`  | `saturate`, `hueRotate`, `luminanceToAlpha`, `matrix` | Its input with other colors: less or more saturated (1 leaves them), hues turned by an angle, black with the luminance as alpha, or a text of the 20 numbers of a matrix. |
| `composite`    | `in2`, `operator`, `k1` ... `k4`                     | `in` combined with `in2`: `over` (default), `in`, `out`, `atop`, `xor`, `lighter`, or `arithmetic` (`k1 * in * in2 + k2 * in + k3 * in2 + k4`). |
| `blend`        | `in2`, `mode`                                        | `in` blended over `in2` (`multiply`, `screen`, ...).      |
| `morphology`   | `radius`, `dilate`                                   | Its input thinned by a radius, or fattened with `dilate: true`. |
| `lighting`     | `scale`, `constant`, `specular`, `exponent`, `color`, `azimuth`, `elevation`, `position`, `height` | The alpha of its input lit as a surface `scale` high: matte, or its highlights with `specular: true`. The light is far away along `azimuth` and `elevation`, or at a `position` and a `height`. |
| `displacement` | `in2`, `scale`, `xChannel`, `yChannel`               | Its input with every pixel moved by up to `scale`, following the `red`, `green`, `blue` or `alpha` of `in2`. |
| `dropShadow`   | `offset` (point), `deviation`, `color`               | Its input over its shadow.                                |

- Every primitive accepts `in` and `result`. A primitive reads the result of
  the one before it, the first one reading the layer, unless `in` (and `in2`)
  name `source` (the layer), `sourceAlpha` (the layer as black with its alpha)
  or the `result` of an earlier primitive.
- `deviation`, `radius`, `offset`, `scale`, `position`, `height`, the angles in
  radians, the colors and the `mode` are expressions, so a filter follows the
  properties of the component. `in`, `in2`, `result`, `operator`, `dilate`,
  `specular`, `luminanceToAlpha`, `matrix`, `xChannel` and `yChannel` are fixed.
- Lengths are in the coordinates of the layer. What a primitive puts outside
  the `bounds` of the filter is lost, so leave room for blurs and shadows.
- `linearColors: true` on the `filter` runs it on linear colors, as SVG does by
  default, instead of the sRGB colors drawings are made of.
- The picture is kept from one paint to the next when the elements of the layer
  read nothing of the component (no property, palette color or `bounds`), and is
  drawn again on every paint otherwise. The filter only runs again when the
  picture or one of its values changes. It needs a GPU: without one the layer is
  drawn as it is.
- A layer with a `filter` can't have a `mask`: put it inside the masked layer.

## Importing an SVG

`YuiSvgImporter` writes an SVG as a drawing made of the elements above, so its
shapes can then be bound to the component, animated or edited:

```cpp
const auto imported = YuiSvgImporter::importSvg (File ("logo.svg"));

if (imported.wasOk())
{
    file.replaceWithText (imported.toText());                    // drawing Imported { ... }
    auto style = DrawingStyle::create (component, imported.drawing);
}
```

```yui
drawing Imported {
    viewBox: rect (0, 0, 24, 24);

    let skyGradient: linearGradient (#4488ff, point (0, 0), #ffffff, point (0, 1));

    group {
        fill: #000000;                                           // SVG shapes are black unless they say otherwise

        rect { bounds: rect (2, 2, 20, 20); cornerSize: size (4, 2); fillGradient: skyGradient.mappedTo (objectBounds); }
        path { data: "M 2 12 L 22 12"; fill: none; stroke: #ff0000; strokeWidth: 1; }
    }
}
```

The drawing paints what a `Drawable` paints for the same SVG, see the
[supported SVG features](../graphics/svg.md#supported-svg-features). The
`warnings` of the result list what it paints differently, mainly:

- a pattern is clipped to its tile and covers its shape, while `Drawable` lets
  the content of a tile overflow and, with a `patternTransform`, may leave part
  of the shape empty;
- text is written with `defaultFont` unless `Options::fontExpression` returns
  the font to use for a family, a size, a weight and a style;
- a filter blurs and thins evenly: the largest of the two lengths SVG can give
  along x and y is used;
- an image that isn't a file or a `data:` source, such as one on the network,
  has to be given to the drawing with an
  [image resolver](#using-a-drawing-from-c).

An element with a `filter` becomes a `layer` drawn through a [filter](#filters)
whose `bounds` are the region SVG gives it around that element. A filter ending
by blending with `BackgroundImage` sets the `blend` of the group instead.

The values are written as they are in the SVG. With `Options::colorProperties`
every color becomes a [property](#properties) instead, named `color1`, `color2`
and so on, that the component sets with `setProperty` or a stylesheet assigns:

```yui
drawing Imported {
    viewBox: rect (0, 0, 24, 24);

    property color1: color = #000000;
    property color2: color = #ff0000;

    group {
        fill: color1;

        path { data: "M 2 12 L 22 12"; fill: none; stroke: color2; strokeWidth: 1; }
    }
}
```

Anything else is parameterized by hand: declare a property and read it where a
value was written, for example `strokeWidth: 1 + 3 * amount`. The `Svg`
graphics example does both live: drop an SVG on it, edit the drawing, and every
property it declares gets a control.

To show an SVG as it is, without editing it, the `svg` element is simpler.

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
  `rectangle`, `transform`, `font` or an array of scalar types (`number[]`, ...).
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
list them with `DrawingStyle::getProperties()`.

## Errors

`DrawingStyle::create` fails with a message that starts with the path of the
offending element and property:

```text
Knob/layer[0]/arc#value.to: Unknown symbol 'avv'
drawing/circle[0]: missing the required property 'radius'
drawing/repeat[0]/layer[0]: a 'layer' can't be nested inside a repeat
```

Elements are numbered by type within their parent, or named by their `id`. The
root is named by its `name` property, or `drawing`. Passing an optional
`YuiDiagnostic*` to `DrawingStyle::create` also populates the offending child
`DataTree` element and property `Identifier`, so `YuiDocument`, `YuiBuilder` and
`StyleContext` resolve errors to the exact child line and column through the
parser's `SourceMap`.

## Using a drawing from C++

`YuiParser::parse` turns `.yui` text into the drawing document, and
`DrawingStyle::create` takes the `drawing` tree inside it:

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

Because `DrawingStyle` is a `ComponentStyle`, you can also invoke `paint`
directly when painting by hand:

```cpp
if (style.wasOk())
    style.getReference()->paint (g, theme, slider);
```

An `image` loads the file its `source` names, or a `data:` source. To take
images from anywhere else, give the drawing a resolver: it is asked first, once
for each source, and returning nothing leaves the source to be loaded as usual.

```cpp
style.getReference()->setImageResolver ([this] (const String& source) -> std::optional<Image>
{
    if (const auto found = downloadedImages.find (source); found != downloadedImages.end())
        return found->second;

    return std::nullopt;
});
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

- A semi-transparent or masked `layer` is rendered offscreen at the logical size
  of the component, so it can look soft on high-density displays.
- Nested `repeat` blocks are not supported; pass a dynamic `number[]` array
  property (`repeat i in 0 .. len (lanes)`) to draw variable-length collections.
- The tile of a `pattern` is an image: it is drawn at the size it has in the
  component, and looks soft when the shape it fills is scaled up much further.
- Text has no outline: it can't be part of a `clipPath`, and has no
  `objectBounds` when placed from an `anchor`.

## See also

- [The `.yui` language](yui-language.md) - the syntax of every block.
- [Stylesheets](yui-styles.md) - selecting drawings and setting their
  properties from rules.
- [How YUI fits together](yui-architecture.md) - how drawings share their
  compilation and where their values come from.
- [Reactive bindings](yui-reactivity.md) - the same expressions keeping
  properties up to date.
