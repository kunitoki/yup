# Component Styling

YUP separates appearance from logic via `ComponentStyle`, colors, and metrics.
These cascade: a component inherits values from its parent and from the global
`ApplicationTheme`.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

The styling system has three layers:

| Layer | Scope | How to set |
|---|---|---|
| **Per-component** | This component only | `comp.setColor()`, `comp.setMetric()`, `comp.setStyle()` |
| **Type-wide** | All instances of a widget type | `theme->setComponentStyle<T>(style)` |
| **Theme defaults** | Fallback for all components | `theme->setColor()`, `theme->setMetric()` |

Resolution walks : component overrides → parent chain → theme defaults.
If nothing is found, the paint function uses a hardcoded fallback.

---

## Component colors

Colors are identified by `Identifier` strings (e.g. `"background"`,
`"border"`). Each widget class exposes style IDs as `static const` members in a
nested `Style` struct.

### Setting per-component colors

```cpp
comp.setColor ("background", Colors::darkSlateBlue);
comp.setColor ("border", std::nullopt); // remove override → inherit
```

### Reading colors

```cpp
// Get the override on THIS component only (no parent walk)
std::optional<Color> c = comp.getColor ("background");

// Walk parent chain, then fall back to ApplicationTheme defaults
std::optional<Color> c = comp.findColor ("background");
```

### Setting theme-level defaults

Theme defaults affect all components that haven't overridden the color locally:

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
theme->setColor (TextButton::Style::backgroundColorId, Colors::cornflowerBlue);
```

Theme-level color resolution via `theme->findColor(component, id)`:
1. Calls `component.findColor(id)` (walks parent chain for component overrides).
2. If not found, checks the theme's `defaultColors` map.

### Widget style IDs

Every themable widget exposes color IDs:

| Widget | IDs (partial list) |
|---|---|
| `TextButton::Style` | `backgroundColorId`, `backgroundPressedColorId`, `textColorId`, `textPressedColorId`, `outlineColorId`, `outlineFocusedColorId` |
| `ToggleButton::Style` | `backgroundColorId`, `backgroundToggledColorId`, `textColorId`, `textToggledColorId`, `borderColorId`, `borderToggledColorId` |
| `SwitchButton::Style` | `switchColorId`, `switchOffBackgroundColorId`, `switchOnBackgroundColorId` |
| `Slider::Style` | `backgroundColorId`, `trackColorId`, `thumbColorId`, `thumbOverColorId`, `thumbDownColorId`, `textColorId` |
| `Label::Style` | `textFillColorId`, `textStrokeColorId`, `backgroundColorId`, `outlineColorId` |
| `TextEditor::Style` | `backgroundColorId`, `textColorId`, `caretColorId`, `selectionColorId`, `outlineColorId`, `focusedOutlineColorId` |
| `ComboBox::Style` | `backgroundColorId`, `textColorId`, `borderColorId`, `arrowColorId`, `focusedBorderColorId` |
| `ScrollBar::Style` | `trackColorId`, `thumbColorId`, `thumbHoverColorId`, `thumbDraggingColorId` |
| `ProgressBar::Style` | `backgroundColorId`, `foregroundColorId` |
| `ListBox::Style` | `backgroundColorId`, `outlineColorId`, `rowBackgroundColorId`, `selectedRowBackgroundColorId`, `hoveredRowBackgroundColorId` |
| `ListBoxItem::Style` | `textColorId`, `textColorSelectedId`, `backgroundColorId`, `backgroundColorSelectedId`, `backgroundColorHoveredId` |
| `PopupMenu::Style` | `menuBackground`, `menuBorder`, `menuItemText`, `menuItemTextDisabled`, `menuItemBackground`, `menuItemBackgroundHighlighted`, `menuItemBackgroundActiveSubmenu` |
| `DocumentWindow::Style` | `backgroundColorId` |
| `MidiKeyboardComponent::Style` | `whiteKeyColorId`, `whiteKeyPressedColorId`, `whiteKeyShadowColorId`, `blackKeyColorId`, `blackKeyPressedColorId`, `blackKeyShadowColorId`, `keyOutlineColorId` |
| `KMeterComponent::Style` | `backgroundColorId`, `greenZoneColorId`, `amberZoneColorId`, `redZoneColorId`, `averageLevelColorId`, `peakLevelColorId`, `peakLevelClipColorId`, `peakHoldColorId` |
| `AudioGraphComponent::Style` | `backgroundColorId`, `gridColorId` |
| `AudioGraphNodeView::Style` | `shadowColorId`, `accentBackgroundColorId`, `bodyBackgroundColorId`, `headerBackgroundColorId`, `textColorId`, `subtitleTextColorId`, `parameterBackgroundColorId`, `parameterValueBackgroundColorId`, `portHoleColorId` |

---

## Component metrics

Metrics are numeric values (corner radius, padding, spacing) following the same
cascading pattern as colors.

### Setting per-component metrics

```cpp
comp.setMetric ("corner-radius", 8.0f);
comp.setMetric ("padding", std::nullopt); // remove override
```

### Reading metrics

```cpp
// Local only
std::optional<float> r = comp.getMetric ("corner-radius");

// Walk parent chain + theme fallback
std::optional<float> r = comp.findMetric ("corner-radius");
```

### Setting theme-level metric defaults

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
theme->setMetric ("corner-radius", 4.0f);
```

---

## `ComponentStyle` — per-type paint styles

A `ComponentStyle` is a reference-counted object that paints a specific
component type using a given theme. Each widget type gets its own
`ComponentStyle` registered on the theme.

### Creating a custom paint style

Use the `createStyle<T>(callback)` factory:

```cpp
auto customButtonStyle = ComponentStyle::createStyle<TextButton> (
    [] (Graphics& g, const ApplicationTheme& theme, const TextButton& b)
    {
        auto bgColor = theme.findColor (b, TextButton::Style::backgroundColorId)
                           .value_or (Colors::gray);

        g.setFillColor (bgColor);
        g.fillRoundedRectangle (b.getLocalBounds(), 6.0f);

        // Draw text, border, etc.
    });
```

The callback receives the `Graphics` context, the current `ApplicationTheme`,
and a typed reference to the component. It is called whenever the component
needs to repaint.

### Attaching a style to a component

```cpp
// Per-instance override
comp.setStyle (customButtonStyle);
ComponentStyle::Ptr s = comp.getStyle();
```

### Registering a type-wide style on the theme

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
theme->setComponentStyle<TextButton> (customButtonStyle);
```

This makes all `TextButton` instances use the custom style, unless a specific
instance has called `comp.setStyle()` with its own override.

### Removing a style

```cpp
comp.setStyle (nullptr); // revert to type-wide or default
```

### `styleChanged()` callback

When a component's style changes (via `setStyle`, or when a color or metric
override is set), the virtual `styleChanged()` method is called. Override it to
repaint:

```cpp
void MyButton::styleChanged() override
{
    repaint();
}
```

---

## `ApplicationTheme`

The global theme singleton holds fonts, default colors/metrics, and
type→style mappings.

### Creating and setting the global theme

```cpp
auto theme = createThemeVersion1(); // built-in theme (Roboto + FontAwesome7)
ApplicationTheme::setGlobalTheme (std::move (theme));
```

### Accessing the global theme

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
```

### Theme fonts

```cpp
theme->setDefaultFont (myFont);
theme->setDefaultIconFont (myIconFont); // FontAwesome7

const Font& font     = theme->getDefaultFont();
const Font& iconFont = theme->getDefaultIconFont();
```

### Batch color/metric registration

```cpp
theme->setColors ({
    { TextButton::Style::backgroundColorId, Colors::cornflowerblue },
    { Slider::Style::thumbColorId, Colors::orange },
});

theme->setMetrics ({
    { "corner-radius", 8.0f },
    { "padding", 4.0f },
});
```

### Finding values through the theme

Theme `findColor` and `findMetric` check the component's parent chain first,
then fall back to theme defaults:

```cpp
// These first call component.findColor/findMetric, then check theme defaults
auto c = theme->findColor (comp, "background");
auto m = theme->findMetric (comp, "corner-radius");

// Static convenience wrappers on the global theme:
auto c = ApplicationTheme::findComponentColor (comp, "background");
auto m = ApplicationTheme::findComponentMetric (comp, "corner-radius");
```

---

## Icons

The built-in theme (`createThemeVersion1()`) registers FontAwesome 7 as the icon
font. Icons are rendered as text using the icon font. ~600+ icon glyphs are
available as `YUP_ICON_*` macros:

```cpp
#include <yup_gui/themes/theme_v1/yup_ThemeVersion1_Icons.h>

String iconText = YUP_ICON_GEAR;        // ⚙
String checkmark = YUP_ICON_CIRCLE_CHECK; // ✅
```

To draw an icon, obtain the theme's icon font and draw the glyph as text:

```cpp
void MyButton::paint (Graphics& g) override
{
    auto theme = ApplicationTheme::getGlobalTheme();
    g.setFont (theme->getDefaultIconFont());
    g.setFillColor (findColor (TextButton::Style::textColorId)
                        .value_or (Colors::white));
    g.drawText (YUP_ICON_GEAR, getLocalBounds(), Justification::centred);
}
```

---

## Full example — custom styled button

```cpp
class StyledPanel : public Component
{
public:
    StyledPanel()
    {
        addAndMakeVisible (button);
        button.setTitle ("Save");
    }

    void resized() override
    {
        button.setBounds (getLocalBounds().reduced (20));
    }

private:
    TextButton button;

    static inline auto kCustomStyle = ComponentStyle::createStyle<TextButton> (
        [] (Graphics& g, const ApplicationTheme& theme, const TextButton& b)
        {
            auto bgColor = b.findColor (TextButton::Style::backgroundColorId)
                               .value_or (Colors::darkSlateBlue);

            g.setFillColor (bgColor);
            g.fillRoundedRectangle (b.getLocalBounds(), 12.0f);

            g.setFont (theme.getDefaultFont());
            g.setFillColor (Colors::white);
            g.drawText (b.getTitle(), b.getLocalBounds(), Justification::centred);
        });

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyledPanel)
};

// Register the custom style (e.g., in main or plugin setup):
auto theme = createThemeVersion1();
theme->setComponentStyle<TextButton> (StyledPanel::kCustomStyle);
ApplicationTheme::setGlobalTheme (std::move (theme));
```

---

## Related

- [Component basics](component-basics.md) — `setColor`, `setMetric`, `setStyle` APIs
- [Component effects (shaders)](component-effects.md)
