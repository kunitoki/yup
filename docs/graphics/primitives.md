# Primitives

The graphics primitives are the small, value-type building blocks every drawing
call is expressed in: points, sizes, rectangles, colors, gradients, paths, and
affine transforms. They live in `yup_graphics` and are used throughout the UI
and drawing APIs.

```{note}
YUP uses American English: it is `Color` (not `Colour`) and `center` (not
`centred`). Geometry primitives convert between numeric types with the templated
`.to<T>()` method - for example `bounds.to<float>()` - not `toFloat`.
```

## Point

`Point<ValueType>` is a 2D coordinate templated on its numeric type. It is a
`constexpr`-friendly value type with a rich set of vector operations.

```cpp
Point<float> a { 10.0f, 20.0f };
Point<float> b { 30.0f, 5.0f };

auto sum      = a + b;                 // {40, 25}
auto mid      = a.midpoint (b);        // halfway between a and b
auto dist     = a.distanceTo (b);      // Euclidean distance
auto moved    = a.translated (5, 0);   // non-mutating
auto asInt    = a.to<int>();           // convert numeric type
```

Highlights: `translate`/`translated`, `scale`/`scaled`, `rotateClockwise`,
`distanceTo`, `magnitude`, `dotProduct`, `crossProduct`, `lerp`,
`getPointOnCircumference`, and `transformed (AffineTransform)`. Every mutating
method has a non-mutating `-ed` counterpart that returns a new point.

## Size

`Size<ValueType>` holds a `width` and `height` pair with the same value-type
conventions as `Point`.

```cpp
Size<float> s { 640.0f, 480.0f };
auto scaled = s.scaled (0.5f);   // {320, 240}
auto ints   = s.to<int>();
```

## Rectangle

`Rectangle<ValueType>` is the workhorse for layout and drawing. It stores an
origin plus a size and offers a large fluent API for slicing and insetting -
ideal for laying out a `Component` inside its bounds.

```cpp
Rectangle<float> r { 0.0f, 0.0f, 200.0f, 100.0f };

auto center   = r.getCenter();
auto inset    = r.reduced (8.0f);                  // shrink by 8px on all sides
auto smaller  = r.withSizeKeepingCenter (100, 40); // keep center, resize
```

### Slicing layout

`removeFrom*` mutates the rectangle and returns the slice it removed - the
canonical way to divide an area into regions:

```cpp
auto area   = getLocalBounds().to<float>();
auto header = area.removeFromTop (30.0f);   // top strip; `area` shrinks
auto footer = area.removeFromBottom (24.0f);
auto sidebar = area.removeFromLeft (120.0f);
// `area` is now the remaining central region
```

Non-mutating variants (`withTrimmedTop`, `reducedLeft`, …) return a new
rectangle without changing the original.

## Color

`Color` is a packed 32-bit ARGB value. It is `constexpr` and cheap to copy.

```cpp
Color opaqueBlack;                    // default: 0xff000000
Color fromARGB   { 0xff3366cc };      // 0xAARRGGBB
Color fromRGB    { 51, 102, 204 };    // r, g, b (alpha = 255)
Color fromARGB2  { 128, 51, 102, 204 }; // a, r, g, b
```

Accessors include `getARGB()`, `getRGBA()`, `getBGRA()`, `isTransparent()`, and
component getters. A large set of named constants lives in the `Colors`
namespace:

```cpp
g.setFillColor (Colors::cornflowerblue);
g.setStrokeColor (Colors::white);
```

## ColorGradient

`ColorGradient` describes a `Linear` or `Radial` gradient built from color
stops. Each stop has a color, a position, and a delta along the gradient.

```cpp
ColorGradient gradient {
    Colors::black, 0.0f, 0.0f,      // start color + point
    Colors::white, 0.0f, 100.0f,    // end color + point
    ColorGradient::Linear
};

g.setFillColorGradient (gradient);
g.fillRect (bounds);
```

Gradients support a `Spread` mode - `Pad` (clamp to endpoints), `Repeat`
(tile), or `Reflect` (mirrored tile) - and any number of intermediate stops for
multi-color gradients.

## Path

`Path` is a resolution-independent vector outline built from move / line / curve
segments, or from higher-level shape helpers. Every builder method returns
`Path&`, so calls chain fluently.

```cpp
Path p;
p.moveTo (10.0f, 10.0f)
 .lineTo (90.0f, 10.0f)
 .quadTo (90.0f, 90.0f, 50.0f, 90.0f)  // control point, end point
 .cubicTo (10.0f, 90.0f, 10.0f, 50.0f, 10.0f, 10.0f)
 .close();

g.setFillColor (Colors::orange);
g.fillPath (p);
```

Shape helpers add whole primitives to a path:

```cpp
Path shapes;
shapes.addRectangle (0, 0, 100, 50);
shapes.addRoundedRectangle (0, 60, 100, 50, 8.0f);
shapes.addEllipse (0, 120, 100, 50);
shapes.addArc (arcBounds, 0.0f, MathConstants<float>::pi, true);
```

## Stroke types

Stroking is configured with three related types:

- **`StrokeType`** - bundles a `width`, a `StrokeJoin`, and a `StrokeCap`. It is
  immutable with fluent `withWidth` / `withJoin` / `withCap` builders. The
  default is width `1.0`, `Miter` join, `Butt` cap.
- **`StrokeJoin`** - how two segments meet: `Miter`, `Round`, or `Bevel`.
- **`StrokeCap`** - how open ends are drawn: `Butt`, `Round`, or `Square`.

```cpp
StrokeType stroke { 3.0f, StrokeJoin::Round, StrokeCap::Round };
g.setStrokeType (stroke);
g.strokePath (p);
```

## AffineTransform

`AffineTransform` is a 2D affine matrix used to translate, scale, rotate, and
shear geometry. Transforms compose fluently and can be applied to points, paths,
or set on the `Graphics` state.

```cpp
auto t = AffineTransform::translation (100.0f, 50.0f)
             .rotated (MathConstants<float>::halfPi)
             .scaled (2.0f);

auto movedPoint = myPoint.transformed (t);
g.setTransform (t);   // applies to subsequent drawing
```

## BlendMode

`BlendMode` selects how new drawing is composited over existing pixels:
`SrcOver` (the default), `Multiply`, `Screen`, `Overlay`, `Darken`, `Lighten`,
`ColorDodge`, `ColorBurn`, `HardLight`, `SoftLight`, `Difference`, `Exclusion`,
`Hue`, `Saturation`, `Color`, and `Luminosity`.

```cpp
g.setBlendMode (BlendMode::Multiply);
```

## See also

- [The Graphics class](graphics-class.md) - the drawing API that consumes these
  primitives.
- [How to draw](drawing.md) - a practical drawing guide.
