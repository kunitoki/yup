# Fonts & Text Layout

Text drawing in YUP uses `Font` for the font resource, `StyledText` for rich
pre-laid-out text, and a `Graphics` call for rendering. Fonts are obtained via
`ApplicationTheme`, not constructed inline.

## Font

`Font` wraps a Rive font resource behind a fluent, immutable API. Load it from
a file or memory block, query metrics, and derive variants with different sizes
or variable-axis settings.

### Loading a font

```cpp
Font font;
font.loadFromFile (File ("/path/to/Font.otf"));
// or from a memory buffer:
font.loadFromData (fontBytes);
```

### Font metrics

```cpp
float ascent  = font.getAscent();
float descent = font.getDescent();
float height  = font.getHeight();
int   weight  = font.getWeight();
bool  italic  = font.isItalic();
```

### Height and fluent variants

The `height` property is the rendered size. Use `withHeight()` to derive a new
font at a different size without mutating the original:

```cpp
auto big   = font.withHeight (24.0f);
auto small = font.withHeight (10.0f);
```

### Variable font axes

For variable fonts (OpenType variation axes), query available axes and set
values to obtain derived instances:

```cpp
for (int i = 0; i < font.getNumAxis(); ++i)
{
    auto axis = font.getAxisDescription (i);
    // axis->tagName, axis->minimumValue, axis->maximumValue, axis->defaultValue
}

// Adjust an axis on a copy
auto wideWeight = font.withAxisValue ("wght", 700.0f);
auto condensed = font.withAxisValue ("wdth", 75.0f);

// Bulk set with initializer list
auto custom = font.withAxisValues ({
    { "wght", 650.0f },
    { "slnt", -10.0f }
});
```

Each axis has a 4-character OpenType tag (e.g. `"wght"`, `"wdth"`, `"slnt"`,
`"ital"`). `resetAxisValue(tag)` / `resetAllAxisValues()` restore the default
values.

### OpenType features

```cpp
auto withLigatures = font.withFeature ({ "liga", 1 });
auto withFeatures  = font.withFeatures ({
    { "liga", 1 },   // standard ligatures on
    { "kern", 1 }    // kerning on
});
```

Features are keyed by 4-character tag; a value of `1` enables the feature,
`0` disables it.

## Themed font access

YUP components do not instantiate `Font` directly. Get the default font from the
global `ApplicationTheme`, and combine it with `withHeight` for a specific size:

```cpp
auto font     = ApplicationTheme::getGlobalTheme()->getDefaultFont();
auto bodyFont = font.withHeight (14.0f);
auto titleFont = font.withHeight (24.0f);
```

## StyledText

`StyledText` is a pre-laid-out, optionally rich text block — a batch built from
one or more font/style runs. Use it when you need mixed styling in a single
paragraph or want to measure text before drawing.

```cpp
StyledText text;
{
    auto mod = text.startUpdate();
    mod.appendText ("Hello ", font.withHeight (18.0f));
    mod.appendText ("bold", boldFont.withHeight (18.0f));   // different font/size
    mod.setHorizontalAlign (StyledText::center);
    mod.setMaxSize ({ 200.0f, 0.0f });                      // width cap; 0 = auto height
}
```

Key layout controls set via the `TextModifier`:

| Property              | Values                | Description                                          |
| --------------------- | --------------------- | ---------------------------------------------------- |
| `HorizontalAlign`     | `left` / `center` / `right` / `justified` | Paragraph alignment.    |
| `VerticalAlign`       | `top` / `middle` / `bottom`               | Block vertical alignment. |
| `Overflow`            | `visible` / `ellipsis`                    | How overflowing text is handled. |
| `Wrap`                | `wrap` / `noWrap`                         | Word wrapping.                      |
| `ParagraphSpacing`    | `float`                                   | Extra space between paragraphs.    |
| `MaxSize`             | `Size<float>`                             | Bounding size (0 = unconstrained). |
| `LetterSpacing`       | `float`                                   | Extra space between glyphs.        |
| `LineHeight`          | `float` (-1 = automatic)                  | Line height multiplier.            |

## Drawing text

Simple single-font text uses `fillFittedText` / `strokeFittedText` directly on
`Graphics`:

```cpp
auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (16.0f);

g.setFillColor (Colors::white);
g.fillFittedText ("Hello YUP", font, textBounds, Justification::center);

g.setStrokeColor (Colors::black);
g.setStrokeWidth (1.0f);
g.strokeFittedText ("Hello YUP", font, textBounds, Justification::center);
```

For styled text built as a `StyledText` block:

```cpp
g.fillFittedText (styledText, textBounds);
g.strokeFittedText (styledText, textBounds);
```

## See also

- [The Graphics class](graphics-class.md) — `fillFittedText`, `strokeFittedText`.
- [How to draw](drawing.md) — drawing text examples.
- [SVG](svg.md) — SVG text elements use the `ParseOptions::fontResolver` to
  supply custom fonts.
