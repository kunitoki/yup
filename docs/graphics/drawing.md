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

g.setFillColor (Colors::lightgrey);
g.fillRect (bounds);

g.setStrokeColor (Colors::black);
g.setStrokeWidth (1.0f);
g.strokeRect (bounds.reduced (0.5f));  // inset by half-width for sharp pixels
```

## Draw a rounded button

```cpp
auto rect = getLocalBounds().to<float>().reduced (4.0f);

// Fill
g.setFillColor (isHover ? Colors::cornflowerblue : Colors::slategray);
g.fillRoundedRect (rect, 6.0f);

// Border
g.setStrokeColor (Colors::white.withMultipliedBrightness (0.8f));
g.setStrokeWidth (1.0f);
g.strokeRoundedRect (rect.reduced (0.5f), 6.0f);
```

## Draw a circle

```cpp
auto bounds = getLocalBounds().to<float>();
auto side   = std::min (bounds.getWidth(), bounds.getHeight());
Rectangle<float> square { bounds.getX(), bounds.getY(), side, side };

g.setFillColor (Colors::tomato);
g.fillEllipse (square);
```

## Draw a line

`strokeLine` takes two endpoints. The second overload accepts `Point`.

```cpp
g.setStrokeColor (Colors::white);
g.setStrokeWidth (2.0f);
g.strokeLine (x1, y1, x2, y2);

// or
g.strokeLine (Point<float>{ x1, y1 }, Point<float>{ x2, y2 });
```

## Draw text

Fonts come from `ApplicationTheme`. Use `fillFittedText` with a `Justification`
to position the text inside a rectangle.

```cpp
auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();
auto textBounds = getLocalBounds().to<float>().reduced (8.0f);

g.setFillColor (Colors::white);
g.fillFittedText ("Label", font, textBounds, Justification::center);
```

For outlined text, use `strokeFittedText`. For rich text (multiple fonts or
colors), construct a `StyledText` and pass it to the overloaded variant.

## Draw an image

```cpp
// Natural size at a point
g.drawImageAt (icon, { 4.0f, 4.0f });

// Scaled into a rectangle
g.drawImage (screenshot, targetRect);
```

## Draw a GPU-calculated texture

When you already have a `GpuTexture::Ptr` from a canvas or render pass, draw it
directly — no `Image` allocation needed:

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

## Apply a clip

```cpp
auto saved = g.saveState();

g.setClipPath (roundedBounds);
g.setFillColor (Colors::hotpink);
g.fillAll();                          // only fills inside roundedBounds

// saved.restore() called at end of scope
```

## Use a transform

```cpp
auto saved = g.saveState();

g.addTransform (AffineTransform::rotation (yup::MathConstants<float>::halfPi, cx, cy));
g.setFillColor (Colors::orange);
g.fillPath (arrowShape);

// transform undone on restore
```

## Draw with a blend mode

```cpp
auto saved = g.saveState();

g.setBlendMode (BlendMode::Multiply);
g.setFillColor (Colors::yellow.withAlpha (0.5f));
g.fillAll();

// state restored
```

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

## See also

- [Graphics class reference](graphics-class.md) — the full API surface.
- [Primitives](primitives.md) — `Color`, `Point`, `Rectangle`, `Path`,
  `AffineTransform`, `ColorGradient`, `StrokeType`.
- [RHI](rhi/index.md) — step down for custom GPU pipelines and render passes.
