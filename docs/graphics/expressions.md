# Graphics expressions

`yup_graphics` extends the [expression language](../core/expressions.md) with
the graphics primitives, and adds `ExpressionDrawing` to draw shapes whose
geometry and style are computed from runtime data - charts, meters,
visualizations, or any data-driven vector content.

## Graphics types

`addGraphicsExpressionTypes (context)` registers these types:

| Type | C++ type | Constructors | Highlights |
|---|---|---|---|
| `point` | `Point<float>` | `point (x, y)`, `[x, y]` | `x y length`, `distanceTo angleTo dot normalized translated scaled lerp transformed onCircle`, `+ - * /`, swizzles `p.yx` |
| `size` | `Size<float>` | `size (w, h)`, `[w, h]` | `width height`, `* /` by a number |
| `rect` | `Rectangle<float>` | `rect (x, y, w, h)`, `[x, y, w, h]`, `rect (point, size)` | `x y width height left top right bottom centerX centerY center position size topLeft ...`, `withX ... withCenter withSizeKeepingCenter translated scaled reduced enlarged contains intersects intersection unionWith largestFittingSquare transformed` |
| `line` | `Line<float>` | `line (x1, y1, x2, y2)`, `line (point, point)` | `start end length`, `pointAt translated transformed reversed` |
| `color` | `Color` | `#ff8800`, `#f80`, `#ff880080` (literals), `color (0xffff0000)`, `rgb`, `rgba` (0-255 ints or 0-1 floats), `hsl`, `hsla` | `r g b a hue saturation luminance` (0-1), `withAlpha withMultipliedAlpha brighter darker contrasting inverted mix` |
| `transform` | `AffineTransform` | `transform ()`, `transform (a, b, c, d, tx, ty)` (the values of an SVG `matrix`), `transform.translation`, `transform.rotation`, `transform.scaling`, `transform.shearing` | `translated rotated scaled followedBy inverted` |
| `path` | `Path` | `path ()`, `path ("M 0 0 L 10 10 Z")` (SVG path data) | `bounds length isEmpty`, `pointAt` (0-1 along the path) `moveTo lineTo quadTo cubicTo close addRectangle addRoundedRectangle addEllipse addLine addArc addTriangle addPolygon addStar appendPath transformed scaledToFit withRoundedCorners withNonZeroWinding` |
| `font` | `Font` | `font ()` (empty, usually provided as an input) | `height isEmpty`, `withHeight withAxisValue` |
| `gradient` | `ColorGradient` | `linearGradient (color, point, color, point)`, `radialGradient (color, center, color, edge)` or `(color, center, color, radius)`, `angularGradient (color, center, color, direction)` | `withStop (color, delta)` (adds a stop between the ends), `withAlpha withMultipliedAlpha withTransform`, `withSpread (spread)`, `mappedTo (rect)` |
| `pattern` | `PatternPaint` | `pattern ()` (no paint, usually provided as an input) | `isEmpty` |

An array of numbers (`float[]`) is the point, the size or the rect it is given
to, and `dash (...)` takes one of any length. Written in place, `[a, b, ...]`
has to hold the numbers of what it makes, two for a point or a size and four
for a rect: a wrong count is an error when compiling, and the right one
compiles to the constructor, so `[x, y, w, h]` runs what `rect (x, y, w, h)`
runs. An array given as an input reads zeros for the numbers it lacks. An
array has none of the members of what it makes: `rect ([0, 0, 100, 50]).center`
names the rect first.

It also registers enumerations, whose values are used by name:

| Enumeration | Values |
|---|---|
| `cap` | `butt round square` |
| `join` | `miter round bevel` |
| `strokePosition` | `inside center outside` |
| `blend` | `srcOver screen overlay darken lighten colorDodge colorBurn hardLight softLight difference exclusion multiply hue saturation color luminosity additive` |
| `justification` | `left right horizontalCenter top bottom verticalCenter topLeft topRight bottomLeft bottomRight centerLeft centerTop center centerRight centerBottom` |
| `spread` | `pad reflect repeat` |
| `textAnchor` | `start middle end` |

Names shared by several enumerations (`round`, `center`) resolve to the type of
the output they are assigned to, such as `strokeCap = round`.

Every `Colors` constant is a constant named `colors.<name>`, spelled as in C++:
`colors.transparentBlack`, `colors.aliceblue`. It is folded when compiling, so
`colors.red.darker (0.1)` costs no more than a `#` literal.

All the types except `path` support `==` and `!=`. Call it before adding
functions that use these types. It also adds `polar (center, radius, angle)`,
the point at `angle` radians from the positive x axis towards positive y, so
`polar (bounds.center, 40, 135deg)` places a knob indicator, and
`mix (a, b, t)` interpolates two points or two colors.

A gradient is clamped beyond its ends unless `withSpread (repeat)` or
`withSpread (reflect)` continues it. `mappedTo (rect)` places a gradient written
for the unit square in a rectangle, so one gradient can follow the bounds of
every shape it paints:

```
let sky = linearGradient (#48f, point (0, 0), #fff, point (0, 1));
fillGradient = sky.mappedTo (rect (10, 10, 80, 40));
```

Paths are values: every method returns a modified copy, so they chain:

```
path().moveTo (0, 0).lineTo (bounds.width, value * bounds.height).close()
```

Paths built only from constants are created once, when compiling. Paths that
depend on inputs are rebuilt on every evaluation, which allocates.

```cpp
ExpressionContext context;
addGraphicsExpressionTypes (context);
auto bounds = context.addInput<Rectangle<float>> ("bounds");
auto value = context.addInput<float> ("value");

auto program = CompiledExpression::compile (R"(
    let square = bounds.largestFittingSquare.reduced (4);
    knob = square.center.onCircle (square.width / 2, value * tau);
    color = hsl (value, 0.7, 0.5).withAlpha (0.8);
)", context);
```

## ExpressionDrawing

`ExpressionDrawing` interprets a program's outputs as drawing commands, one
shape per row. Assign the outputs you need:

| Output | Type | Meaning |
|---|---|---|
| `rect` | `rect` | Rectangle, rounded when `cornerRadius > 0`. `topLeftRadius`, `topRightRadius`, `bottomLeftRadius` and `bottomRightRadius` round single corners, and `cornerSize` (a `size`) rounds them all elliptically instead. |
| `ellipse` | `rect` | Ellipse inscribed in the rectangle. |
| `point` | `point` | Dot of `radius` (default 2). |
| `line` | `line` | Line, drawn with `stroke` (or `fill`). |
| `path` | `path` | Path, filled with its own winding rule. |
| `vertex`, `series` | `point`, `int` | One vertex per row, connected into one polyline per `series` (default 0). Polylines are drawn after the other shapes, with the style of their first row. |
| `label`, `labelRect` | `string`, `rect` | Text fitted in the rectangle, drawn when a font is passed to `draw` or assigned to `labelFont`. Assign both, or `labelAnchor` instead of `labelRect`. |
| `labelAnchor`, `labelTextAnchor` | `point`, `textAnchor` | Places the label from a point on its baseline, at the size of its font, instead of fitting it in `labelRect`. The text starts (default), is centered or ends at the point. |
| `labelColor`, `labelStroke` | `color` | Label fill and stroke colors. |
| `labelFont`, `labelJustification`, `labelLetterSpacing` | `font`, `justification`, `float` | Label font, placement in `labelRect` (default `center`) and extra space after every glyph. |
| `fill`, `stroke` | `color` | Shapes are filled when a fill is set or no stroke is set, and stroked when a stroke is set. |
| `fillGradient`, `strokeGradient` | `gradient` | Gradients used instead of `fill` and `stroke`. |
| `fillPattern`, `strokePattern` | `pattern` | A `PaintSource` tiled over the shapes, used instead of `fill` and `stroke`. |
| `fillOpacity`, `strokeOpacity` | `float` | Opacity of the fill or of the stroke alone. |
| `strokeWidth`, `strokeMiterLimit`, `strokeCap`, `strokeJoin`, `strokePosition` | `float`, `float`, `cap`, `join`, `strokePosition` | Stroke style. |
| `opacity`, `tint`, `feather`, `blend` | `float`, `color`, `float`, `blend` | Opacity (multiplies the current one), tint, edge blur and blend mode. |
| `transform` | `transform` | Transforms the shapes of the row, before the transform of the `Graphics`. |
| `clip`, `clipPath` | `rect`, `path` | Clips the row, in drawing coordinates (not moved by `transform`). |
| `cornerRadius`, `radius` | `float` | Style values. |
| `visible` | `bool` | Rows with `visible == false` are skipped. |

```cpp
ExpressionContext context;
addGraphicsExpressionTypes (context);
auto values = context.addArray<float> ("values");
auto row = context.addInput<int> ("row");
auto bounds = context.addInput<Rectangle<float>> ("bounds");

auto bars = ExpressionDrawing::create (R"(
    let w = bounds.width / len (values);
    let h = values[row] * bounds.height;
    rect = rect (bounds.x + row * w + 1, bounds.bottom - h, w - 2, h);
    cornerRadius = min (3, w / 4);
    fill = hsl (0.6 - values[row] * 0.6, 0.8, 0.55);
)", context);

ExpressionState state (bars.getReference().getProgram());
state.bindArray (values, data.data(), (int) data.size());
state.bindRowIndex (row);

// In paint
state.set (bounds, getLocalBounds().to<float>());
bars.getReference().draw (g, state, (int) data.size());
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/expressions_bars_light.jpg#gh-light-mode-only" alt="Bars drawn from an array by one expression, their height and color following the values" width="560">
  <img class="dark-only rounded" src="../_static/images/graphics/expressions_bars_dark.jpg#gh-dark-mode-only" alt="Bars drawn from an array by one expression, their height and color following the values" width="560">
</p>

`create` compiles the program and fails with a descriptive message when it has
no shape output, or when `label` isn't assigned together with either `labelRect`
or `labelAnchor`. `draw` evaluates the rows in blocks and restores the `Graphics`
state afterwards.

Consecutive rows with the same style are merged into one path and drawn with a
single call when that can't change the result: filled shapes that don't
overlap, or shapes only filled or only stroked with an opaque color. Feathered
rows never merge, and overlapping shapes don't merge when drawn with gradients,
opacity, blend modes, translucent tints or strokes off center. Prefer a shared
color over per-row colors when many rows are drawn, so they merge.

### Pattern paints

A `pattern` value is a `PatternPaint`: a `PaintSource` drawing one tile, and the
transform placing the tiles in the drawing. Provide it as an input and assign it
to `fillPattern` or `strokePattern`. The source isn't owned, so keep it alive as
long as the drawing and its cache:

```cpp
auto dots = context.addInput<PatternPaint> ("dots"); // after addGraphicsExpressionTypes

auto drawing = ExpressionDrawing::create ("rect = bounds; fillPattern = dots", context);

state.set (dots, PatternPaint { &dotsSource, AffineTransform::translation (4.0f, 4.0f) });
```

### Getting the outline

`getOutline (state, numRows)` evaluates the rows and returns the shapes `draw`
would paint as one `Path`, without drawing them: use it to clip to the shapes of
a drawing, or to place content along them. Rows that are not visible are
skipped, labels have no outline, and the `transform` of every row is applied
unless the last argument is `false`:

```cpp
g.setClipPath (clipShapes.getReference().getOutline (state, numRows));
```

### Caching shapes

Pass an `ExpressionDrawing::Cache` to `draw` to keep the shapes it builds. The
program still runs on every draw, but while the outputs of every row, the number
of rows and the merging conditions (opacity, feather, blend mode, tint, stroke
position) stay the same,
the cached shapes are drawn again instead of being rebuilt, and the renderer can
reuse them:

```cpp
ExpressionDrawing::Cache cache; // one per drawing, kept alongside it

// In paint
bars.getReference().draw (g, state, (int) data.size(), nullptr, &cache);
```

`cache.wasRebuilt()` reports whether the last draw had to rebuild. Paths
assigned to `path` and `clipPath` are compared by identity, so build a new path instead of
modifying one in place; paths built per row in the program are new on every
draw, so drawings using them always rebuild.

### Using precompiled programs

Compile once and keep the drawing - never create it in `paint`. If a program is
compiled elsewhere (for example cached, or shared with other consumers), wrap it
without recompiling:

```cpp
ExpressionContext drawingContext (context);
ExpressionDrawing::declareOutputs (drawingContext); // graphics types + output conversions

auto program = CompiledExpression::compile (text, drawingContext);
auto drawing = ExpressionDrawing::create (program.getReference());
```

`declareOutputs` makes values like `strokeWidth = 2` convert to the expected
output types. Outputs with an unexpected type are ignored.

The `ExpressionChart` page of the graphics example shows animated bars, a curve
and star markers built with paths, driven by expressions.
