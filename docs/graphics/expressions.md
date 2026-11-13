# Graphics expressions

`yup_graphics` extends the [expression language](../core/expressions.md) with
the graphics primitives, and adds `ExpressionDrawing` to draw shapes whose
geometry and style are computed from runtime data - charts, meters,
visualizations, or any data-driven vector content.

## Graphics types

`addGraphicsExpressionTypes (context)` registers these types:

| Type | C++ type | Constructors | Highlights |
|---|---|---|---|
| `point` | `Point<float>` | `point (x, y)` | `x y length`, `distanceTo angleTo dot normalized translated scaled lerp transformed onCircle`, `+ - * /`, swizzles `p.yx` |
| `size` | `Size<float>` | `size (w, h)` | `width height`, `* /` by a number |
| `rect` | `Rectangle<float>` | `rect (x, y, w, h)`, `rect (point, size)` | `x y width height left top right bottom centerX centerY center position size topLeft ...`, `withX ... withCenter withSizeKeepingCenter translated scaled reduced enlarged contains intersects intersection unionWith largestFittingSquare transformed` |
| `line` | `Line<float>` | `line (x1, y1, x2, y2)`, `line (point, point)` | `start end length`, `pointAt translated transformed reversed` |
| `color` | `Color` | `#ff8800`, `#f80`, `#ff880080` (literals), `color (0xffff0000)`, `rgb`, `rgba` (0-255 ints or 0-1 floats), `hsl`, `hsla` | `r g b a hue saturation luminance` (0-1), `withAlpha withMultipliedAlpha brighter darker contrasting inverted mix` |
| `transform` | `AffineTransform` | `transform ()`, `transform.translation`, `transform.rotation`, `transform.scaling`, `transform.shearing` | `translated rotated scaled followedBy inverted` |
| `path` | `Path` | `path ()`, `path ("M 0 0 L 10 10 Z")` (SVG path data) | `bounds length isEmpty`, `pointAt` (0-1 along the path) `moveTo lineTo quadTo cubicTo close addRectangle addRoundedRectangle addEllipse addLine addArc addTriangle addPolygon addStar appendPath transformed scaledToFit withRoundedCorners withNonZeroWinding` |
| `font` | `Font` | `font ()` (empty, usually provided as an input) | `height isEmpty`, `withHeight withAxisValue` |
| `gradient` | `ColorGradient` | `linearGradient (color, point, color, point)`, `radialGradient (color, center, color, edge)` or `(color, center, color, radius)`, `angularGradient (color, center, color, direction)` | `withStop (color, delta)` (adds a stop between the ends), `withAlpha withMultipliedAlpha withTransform` |

It also registers enumerations, whose values are used by name:

| Enumeration | Values |
|---|---|
| `cap` | `butt round square` |
| `join` | `miter round bevel` |
| `strokePosition` | `inside center outside` |
| `blend` | `srcOver screen overlay darken lighten colorDodge colorBurn hardLight softLight difference exclusion multiply hue saturation color luminosity additive` |
| `justification` | `left right horizontalCenter top bottom verticalCenter topLeft topRight bottomLeft bottomRight centerLeft centerTop center centerRight centerBottom` |

Names shared by several enumerations (`round`, `center`) resolve to the type of
the output they are assigned to, such as `strokeCap = round`.

All the types except `path` support `==` and `!=`. Call it before adding
functions that use these types. It also adds `polar (center, radius, angle)`,
the point at `angle` radians from the positive x axis towards positive y, so
`polar (bounds.center, 40, 135deg)` places a knob indicator, and
`mix (a, b, t)` interpolates two points or two colors.

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
| `rect` | `rect` | Rectangle, rounded when `cornerRadius > 0`. `topLeftRadius`, `topRightRadius`, `bottomLeftRadius` and `bottomRightRadius` round single corners. |
| `ellipse` | `rect` | Ellipse inscribed in the rectangle. |
| `point` | `point` | Dot of `radius` (default 2). |
| `line` | `line` | Line, drawn with `stroke` (or `fill`). |
| `path` | `path` | Path, filled with its own winding rule. |
| `vertex`, `series` | `point`, `int` | One vertex per row, connected into one polyline per `series` (default 0). Polylines are drawn after the other shapes, with the style of their first row. |
| `label`, `labelRect` | `string`, `rect` | Text fitted in the rectangle, drawn when a font is passed to `draw` or assigned to `labelFont`. Assign both. |
| `labelColor`, `labelStroke` | `color` | Label fill and stroke colors. |
| `labelFont`, `labelJustification` | `font`, `justification` | Label font and placement (default `center`). |
| `fill`, `stroke` | `color` | Shapes are filled when a fill is set or no stroke is set, and stroked when a stroke is set. |
| `fillGradient`, `strokeGradient` | `gradient` | Gradients used instead of `fill` and `stroke`. |
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

`create` compiles the program and fails with a descriptive message when it has
no shape output or assigns only one of `label` and `labelRect`. `draw`
evaluates the rows in blocks and restores the `Graphics` state afterwards.

Consecutive rows with the same style are merged into one path and drawn with a
single call when that can't change the result: filled shapes that don't
overlap, or shapes only filled or only stroked with an opaque color. Feathered
rows never merge, and overlapping shapes don't merge when drawn with gradients,
opacity, blend modes, translucent tints or strokes off center. Prefer a shared
color over per-row colors when many rows are drawn, so they merge.

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
