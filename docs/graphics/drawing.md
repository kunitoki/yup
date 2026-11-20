# How to Draw

A practical, task-oriented guide to the 2D `Graphics` API. Each section solves
one real drawing problem with code you can paste into a `Component::paint`.
Start from the [primitives](primitives.md) and [Graphics class
reference](graphics-class.md) for the full API.

## Fill a background

```cpp
void MyComponent::paint (Graphics& g) override
{
    g.setFillColor (Colors::darkslategray);
    g.fillAll();
}
```

`fillAll()` covers the current drawing area (by default, the full component
bounds).

## Draw a rectangle with a border

```cpp
auto bounds = getLocalBounds().to<float>();

g.setFillColor (Colors::lightgray);
g.fillRect (bounds);

g.setStrokeColor (Colors::black);
g.setStrokeWidth (1.0f);
g.strokeRect (bounds.reduced (0.5f));  // inset by half-width for sharp pixels
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_rectangle_light.jpg#gh-light-mode-only" alt="A filled rectangle with a one pixel border" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_rectangle_dark.jpg#gh-dark-mode-only" alt="A filled rectangle with a one pixel border" width="400">
</p>

## Draw a rounded button

```cpp
auto rect = getLocalBounds().to<float>().reduced (4.5f);  // half-pixel inset for a sharp border

// Fill
g.setFillColor (isHover ? Colors::cornflowerblue : Colors::slategray);
g.fillRoundedRect (rect, 6.0f);

// Border
g.setStrokeColor (Colors::white.withMultipliedBrightness (0.8f));
g.setStrokeWidth (1.0f);
g.strokeRoundedRect (rect, 6.0f);
```

Fill and stroke the same rectangle with the same radii: insetting only the border
leaves its corners out of line with the fill, which then shows through around them.

Pass four radii (top-left, top-right, bottom-left, bottom-right) to round each
corner on its own, for example for a tab:

```cpp
g.fillRoundedRect (rect, 14.0f, 14.0f, 0.0f, 0.0f);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_rounded-button_light.jpg#gh-light-mode-only" alt="Rounded buttons in their normal and hover colors, and a shape with only its top corners rounded" width="560">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_rounded-button_dark.jpg#gh-dark-mode-only" alt="Rounded buttons in their normal and hover colors, and a shape with only its top corners rounded" width="560">
</p>

## Draw a circle

```cpp
auto bounds = getLocalBounds().to<float>();
auto side   = std::min (bounds.getWidth(), bounds.getHeight());
Rectangle<float> square { bounds.getX(), bounds.getY(), side, side };

g.setFillColor (Colors::tomato);
g.fillEllipse (square);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_circle_light.jpg#gh-light-mode-only" alt="A filled circle" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_circle_dark.jpg#gh-dark-mode-only" alt="A filled circle" width="400">
</p>

## Draw a line

`strokeLine` takes two endpoints. The second overload accepts `Point`.

```cpp
g.setStrokeColor (Colors::white);
g.setStrokeWidth (2.0f);
g.strokeLine (x1, y1, x2, y2);

// or
g.strokeLine (Point<float>{ x1, y1 }, Point<float>{ x2, y2 });
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_line_light.jpg#gh-light-mode-only" alt="A two pixel wide line" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_line_dark.jpg#gh-dark-mode-only" alt="A two pixel wide line" width="400">
</p>

## Draw text

Fonts come from `ApplicationTheme`. Use `fillFittedText` with a `Justification`
to position the text inside a rectangle.

```cpp
auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();
auto textBounds = getLocalBounds().to<float>().reduced (8.0f);

g.setFillColor (Colors::white);
g.fillFittedText ("Label", font, textBounds, Justification::center);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_text_light.jpg#gh-light-mode-only" alt="Text centered in its bounds" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_text_dark.jpg#gh-dark-mode-only" alt="Text centered in its bounds" width="400">
</p>

For outlined text, use `strokeFittedText`. For rich text (multiple fonts or
colors), construct a `StyledText` and pass it to the overloaded variant.

## Draw an image

```cpp
// Natural size at a point
g.drawImageAt (icon, { 4.0f, 4.0f });

// Scaled into a rectangle
g.drawImage (screenshot, targetRect);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_image_light.jpg#gh-light-mode-only" alt="An image scaled into a square and stretched into a wide rectangle" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_image_dark.jpg#gh-dark-mode-only" alt="An image scaled into a square and stretched into a wide rectangle" width="400">
</p>

## Draw a GPU-calculated texture

When you already have a `GpuTexture::Ptr` from a canvas or render pass, draw it
directly - no `Image` allocation needed:

```cpp
g.drawTexture (offscreenCanvas->asTexture(), getLocalBounds().to<float>());
```

## Draw with a gradient

```cpp
ColorGradient gradient {
    Colors::firebrick, 0.0f, 0.0f,
    Colors::darkorange, 0.0f, (float) getHeight(),
    ColorGradient::Linear
};

g.setFillColorGradient (gradient);
g.fillRoundedRect (getLocalBounds().to<float>().reduced (4.0f), 8.0f);
```

Add extra stops for complex multi-color gradients.

A `Radial` gradient spreads from its first point out to the distance of the
second, and an `Angular` one sweeps a full turn around its first point, starting
towards the second. Here `center` is the middle of the shape and `edge` a point
on its right side:

```cpp
ColorGradient radial { Colors::darkorange, center, Colors::firebrick, edge, ColorGradient::Radial };

ColorGradient angular { ColorGradient::Angular, {
    { Colors::firebrick, center, 0.0f },
    { Colors::darkorange, center, 0.5f },
    { Colors::firebrick, edge, 1.0f }   // ends on the first color, so the turn has no seam
} };
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_gradients_light.jpg#gh-light-mode-only" alt="Linear, radial and angular gradients" width="560">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_gradients_dark.jpg#gh-dark-mode-only" alt="Linear, radial and angular gradients" width="560">
</p>

## Draw a custom path

```cpp
Path wave;
wave.moveTo (0.0f, (float) getHeight() / 2.0f);
for (float x = 0.0f; x <= (float) getWidth(); x += 4.0f)
    wave.lineTo (x, (float) getHeight() / 2.0f + std::sin (x * 0.03f) * 20.0f);

g.setStrokeColor (Colors::mediumseagreen);
g.setStrokeWidth (2.0f);
g.strokePath (wave);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_path_light.jpg#gh-light-mode-only" alt="A sine wave stroked as a path" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_path_dark.jpg#gh-dark-mode-only" alt="A sine wave stroked as a path" width="400">
</p>

## Layer drawings at group opacity

```cpp
auto layer = g.beginTransparencyLayer (bounds, 0.6f);
if (layer.isValid())
{
    auto& lg = layer.getGraphics();

    lg.setFillColor (Colors::red);
    lg.fillEllipse (circleA);
    lg.fillEllipse (circleB);          // overlaps without double-blending

    layer.commit();
}
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_layer_light.jpg#gh-light-mode-only" alt="Two overlapping circles faded one by one, showing their overlap, and faded together in a layer" width="480">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_layer_dark.jpg#gh-dark-mode-only" alt="Two overlapping circles faded one by one, showing their overlap, and faded together in a layer" width="480">
</p>

## Apply a clip

```cpp
auto saved = g.saveState();

g.setClipPath (roundedBounds);
g.setFillColor (Colors::hotpink);
g.fillAll();                          // only fills inside roundedBounds

// saved.restore() called at end of scope
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_clip_light.jpg#gh-light-mode-only" alt="Drawing cut by a rounded clip path" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_clip_dark.jpg#gh-dark-mode-only" alt="Drawing cut by a rounded clip path" width="400">
</p>

## Use a transform

```cpp
auto saved = g.saveState();

g.addTransform (AffineTransform::rotation (yup::MathConstants<float>::halfPi, cx, cy));
g.setFillColor (Colors::orange);
g.fillPath (arrowShape);

// transform undone on restore
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_transform_light.jpg#gh-light-mode-only" alt="An arrow and its copy rotated a quarter turn around its tail" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_transform_dark.jpg#gh-dark-mode-only" alt="An arrow and its copy rotated a quarter turn around its tail" width="400">
</p>

## Draw with a blend mode

```cpp
auto saved = g.saveState();

g.setBlendMode (BlendMode::Multiply);
g.setFillColor (Colors::yellow.withAlpha (0.5f));
g.fillAll();

// state restored
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_blend-multiply_light.jpg#gh-light-mode-only" alt="A yellow band multiplied over colored stripes" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_blend-multiply_dark.jpg#gh-dark-mode-only" alt="A yellow band multiplied over colored stripes" width="400">
</p>

For a glow, add light instead of covering what is below:

```cpp
auto saved = g.saveState();

g.setBlendMode (BlendMode::Additive);
g.setAdditiveAmount (0.6f);
g.setFeather (12.0f);
g.setFillColor (Colors::cyan);
g.fillEllipse (glowArea);
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_blend-glow_light.jpg#gh-light-mode-only" alt="A feathered additive glow" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_blend-glow_dark.jpg#gh-dark-mode-only" alt="A feathered additive glow" width="400">
</p>

## Render offscreen into an Image

```cpp
Image result { 256, 256, PixelFormat::RGBA };
{
    Graphics g { someContext, result };
    g.setFillColor (Colors::cornflowerblue);
    g.fillAll();
    g.setFillColor (Colors::white);
    g.fillEllipse (Rectangle<float>{ 32.0f, 32.0f, 192.0f, 192.0f });
    g.readPixelsToImage();
}
// result.getRawData() now contains the rendered pixels
```

## Slice layout with rectangles

YUP components typically divide their bounds with the mutable `removeFrom*`
pattern:

```cpp
auto area = getLocalBounds().to<float>();

auto header  = area.removeFromTop (24.0f);
auto footer  = area.removeFromBottom (16.0f);
auto sidebar = area.removeFromLeft (80.0f);
// "area" is now the remaining central content region
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/graphics/drawing_slices_light.jpg#gh-light-mode-only" alt="An area sliced into a header, a footer, a sidebar and the remaining area" width="400">
  <img class="dark-only rounded" src="../_static/images/graphics/drawing_slices_dark.jpg#gh-dark-mode-only" alt="An area sliced into a header, a footer, a sidebar and the remaining area" width="400">
</p>

## See also

- [Graphics class reference](graphics-class.md) - the full API surface.
- [Primitives](primitives.md) - `Color`, `Point`, `Rectangle`, `Path`,
  `AffineTransform`, `ColorGradient`, `StrokeType`.
- [RHI](rhi/index.md) - step down for custom GPU pipelines and render passes.
