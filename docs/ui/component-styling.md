# Component Styling

YUP separates appearance from logic via `ComponentStyle`, colors, and properties.
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
| **Per-component** | This component only | `comp.setColor()`, `comp.setProperty()`, `comp.setStyle()` |
| **Type-wide** | All instances of a widget type | `theme->setComponentStyle<T>(style)` |
| **Theme defaults** | Fallback for all components | `theme->setColor()`, `theme->setProperty()` |
| **Palette** | Colors derived from a handful of source colors | `theme->setPalette()` |
| **Stylesheet** | Components matching selectors in a subtree | `comp.setStyleSheet()` |

Color resolution walks: component overrides → parent chain → theme defaults → palette.
A [stylesheet](yui-styles.md) value sits right after the explicit value of each
component on the way, so values set in C++ always win over it.
If nothing is found, the paint function falls back to the palette roles.

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

// Walk the parent chain only, this never reaches the theme
std::optional<Color> c = comp.findColor ("background");

// Walk the parent chain, then the theme defaults, then the theme palette
std::optional<Color> c = ApplicationTheme::findComponentColor (comp, "background");
```

### Setting theme-level defaults

Theme defaults affect all components that haven't overridden the color locally:

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
theme->setColor (TextButton::Style::backgroundColorId, Colors::cornflowerBlue);
```

Theme-level color resolution via `theme->findColor(component, id)`:
1. Calls `component.findColor(id)` (walks parent chain for component overrides).
2. If not found, checks the colors set with `theme->setColor()`.
3. If not found, checks the colors derived from the closest palette set on the component or a parent with `Component::setPalette()`, or else from the theme palette (see [Palettes](#palettes)).

A `PopupMenu` opened relative to a component (`PopupMenu::Options::withRelativePosition()`, as a `ComboBox` opens its list) takes the palette and the `PopupMenu::Style` colors set on that component or its parents, unless the menu sets them itself, and its submenus take the menu's. Setting the menu colors on a panel styles every menu opened from inside it, even though the menu is added to a popup container higher up.

### Widget style IDs

Every themable widget exposes color IDs:

| Widget | IDs (partial list) |
|---|---|
| `TextButton::Style` | `backgroundColorId`, `backgroundPressedColorId`, `textColorId`, `textPressedColorId`, `outlineColorId`, `outlineFocusedColorId` (curated geometry: `cornerRadiusId`, `outlineWidthId`, `focusedOutlineWidthId`) |
| `ToggleButton::Style` | `backgroundColorId`, `backgroundToggledColorId`, `textColorId`, `textToggledColorId`, `borderColorId`, `borderToggledColorId` (curated geometry: `cornerRadiusId`, `outlineWidthId`, `focusedOutlineWidthId`) |
| `SwitchButton::Style` | `switchColorId`, `switchOffBackgroundColorId`, `switchOnBackgroundColorId` (curated geometry: `cornerRadiusId`, `thumbInsetId`) |
| `Slider::Style` | `backgroundColorId`, `trackColorId`, `thumbColorId`, `thumbOverColorId`, `thumbDownColorId`, `textColorId` (curated geometry: `trackWidthId`, `thumbSizeId`, `rotaryInsetId`) |
| `Label::Style` | `textFillColorId`, `textStrokeColorId`, `backgroundColorId`, `outlineColorId` (curated geometry: `textHeightProportionId`) |
| `TextEditor::Style` | `backgroundColorId`, `textColorId`, `caretColorId`, `selectionColorId`, `outlineColorId`, `focusedOutlineColorId` (curated geometry: `cornerRadiusId`, `outlineWidthId`, `focusedOutlineWidthId`) |
| `ComboBox::Style` | `backgroundColorId`, `textColorId`, `borderColorId`, `arrowColorId`, `focusedBorderColorId` (curated geometry: `cornerRadiusId`, `outlineWidthId`, `focusedOutlineWidthId`) |
| `ScrollBar::Style` | `trackColorId`, `thumbColorId`, `thumbHoverColorId`, `thumbDraggingColorId` (curated geometry: `thumbInsetId`, `cornerRadiusId`) |
| `ProgressBar::Style` | `backgroundColorId`, `foregroundColorId` (curated geometry: `cornerRadiusId`) |
| `ListBox::Style` | `backgroundColorId`, `outlineColorId`, `rowBackgroundColorId`, `selectedRowBackgroundColorId`, `hoveredRowBackgroundColorId`, `refreshIndicatorColorId` (curated geometry: `refreshIndicatorSizeId`) |
| `ListBoxItem::Style` | `textColorId`, `textColorSelectedId`, `backgroundColorId`, `backgroundColorSelectedId`, `backgroundColorHoveredId` |
| `TreeView::Style` | `indentGuideColorId`, `disclosureColorId`, `itemTextColorId`, `itemTextSelectedColorId`, `dropIndicatorColorId`, `itemHoveredColorId` (the rows also use the `ListBox::Style` colors) |
| `TabBar::Style` | `trackColorId`, `indicatorColorId`, `underlineColorId`, `focusOutlineColorId` |
| `TabButton::Style` | `textColorId`, `textSelectedColorId`, `hoveredBackgroundColorId`, `closeButtonColorId` |
| `PopupMenu::Style` | `menuBackground`, `menuBorder`, `menuItemText`, `menuItemTextDisabled`, `menuItemBackground`, `menuItemBackgroundHighlighted`, `menuItemBackgroundActiveSubmenu` |
| `DocumentWindow::Style` | `backgroundColorId` |
| `MidiKeyboardComponent::Style` | `whiteKeyColorId`, `whiteKeyPressedColorId`, `whiteKeyShadowColorId`, `blackKeyColorId`, `blackKeyPressedColorId`, `blackKeyShadowColorId`, `keyOutlineColorId`, `scrollButtonBackgroundColorId`, `scrollButtonArrowColorId` |
| `KMeterComponent::Style` | `backgroundColorId`, `greenZoneColorId`, `amberZoneColorId`, `redZoneColorId`, `averageLevelColorId`, `peakLevelColorId`, `peakLevelClipColorId`, `peakHoldColorId` |
| `SpectrumAnalyzerComponent::Style` | `backgroundTopColorId`, `backgroundBottomColorId`, `outlineColorId`, `fillColorId`, `gridColorId`, `textColorId` |
| `PitchWheelComponent::Style` | `bodyTopColorId`, `bodyBottomColorId`, `outlineColorId`, `gripColorId`, `gripOverColorId`, `gripDownColorId` |
| `ModWheelComponent::Style` | `bodyTopColorId`, `bodyBottomColorId`, `outlineColorId`, `gripColorId`, `gripOverColorId`, `gripDownColorId` |
| `AudioGraphComponent::Style` | `backgroundColorId`, `gridColorId` |
| `AudioGraphNodeView::Style` | `shadowColorId`, `accentBackgroundColorId`, `bodyBackgroundColorId`, `headerBackgroundColorId`, `textColorId`, `subtitleTextColorId`, `parameterBackgroundColorId`, `parameterValueBackgroundColorId`, `portHoleColorId` |

---

## Component properties

Properties are named values describing a component: themeable colors and curated
geometry (corner radius, spacing) plus reflected state such as a slider's `value`
or a label's `text`. Store-backed properties (colors, geometry) cascade like
colors; state-backed properties read and write the widget's own accessors and
resolve only on the owning component. Values are `var`; use the typed
`getPropertyAs<T>()` / `findPropertyAs<T>()` helpers when you need a concrete type.

### Setting per-component properties

```cpp
comp.setProperty ("corner-radius", 8.0);
comp.removeProperty ("corner-radius"); // remove a store-backed override
```

### Reading properties

```cpp
// Local only (descriptor getter for state-backed, stored value otherwise)
var r = comp.getProperty ("corner-radius");
float rf = comp.getPropertyAs<float> ("corner-radius");

// Cascade: this component, the parent chain, the theme default, then the descriptor default
var r = comp.findProperty ("corner-radius");

// Color properties also fall back to the theme colors and the closest palette
auto thumb = slider.findPropertyAs<yup::Color> ("thumbColor");

bool present = comp.hasProperty ("corner-radius");
```

### Setting theme-level property defaults

```cpp
auto theme = ApplicationTheme::getGlobalTheme();
theme->setProperty ("corner-radius", 4.0);
```

### Curated widget geometry

The default theme paints the corner radii, outline widths, slider tracks and
thumbs and similar values from the curated geometry properties listed in the
[widget style IDs](#widget-style-ids) (names and defaults in
[Stylesheets](yui-styles.md#widget-geometry)). The descriptor default is the
only place each value lives. Like colors they are keyed per widget type, so set
them on the theme, on a container for the widgets inside it, or on one widget:

```cpp
theme->setProperty (TextButton::Style::cornerRadiusId, 8.0); // every text button
panel.setProperty (Slider::Style::thumbSizeId, 12.0);        // the sliders in the panel
button.setProperty ("cornerRadius", 0.0);                    // this button
```

### Binding a reflected property to a model

State-backed properties (`value`, `text`, ...) reflect the widget's own model.
`DataTreeBindingProvider` and `BindingGraph` two-way link one of them to a
`DataTree` property, so the widget mirrors the document and writes edits back:

```cpp
DataTree model ("Filter");
DataTreeBindingProvider provider (model);
BindingGraph bindings;

Slider cutoff (Slider::LinearHorizontal);
bindings.link (cutoff, "value", provider, "cutoff");
```

On link establishment the provider seeds the component when the property holds a
value. Dragging the slider updates `model["cutoff"]`; a transaction that changes
`model["cutoff"]` updates the slider. Pass an `UndoManager::Ptr` to
`DataTreeBindingProvider` to make widget edits join the document's undo history:

```cpp
UndoManager::Ptr undo = new UndoManager();
DataTreeBindingProvider provider (model, undo);
```

### Expressions over component properties

`DrawingScope` exposes a component's reflected properties as symbols in a
`yup_expression` program, so an expression can read the live widget by name. A
bare name or `self.` refers to the bound component, `parent.` to its parent:

```cpp
ExpressionContext context;
addGraphicsExpressionTypes (context); // registers the point, color, ... types

Slider slider (Slider::LinearHorizontal);
slider.setValue (0.5);

DrawingScope scope (slider, context);
scope.install(); // resolve property names while compiling

auto compiled = CompiledExpression::compile ("out = value * 100", context);

const auto& program = compiled.getReference();
ExpressionState state (program);
scope.updateInputs (program, state); // push the slider's live values
state.evaluate();
// state.getVar (program.findOutput ("out")) is 50 when value is 0.5
```

Number, integer, boolean, string and color properties are bridged; colors need
the graphics expression types registered on the context. Install the scope before
compiling, then call `updateInputs` on each evaluation. The scope holds a weak
reference to the component, so a destroyed component simply feeds nothing. This is
the foundation data-driven drawing and styling build on.

### Painting a component from a drawing

A *drawing* paints a component from a tree of independent shapes (`rect`,
`circle`, `arc`, `line`, `text`, ...), each with its own style and with values
written as expressions over the component's properties. `DrawingStyle` installs
a drawing on a component like any other style:

```yui
drawing Knob {
    circle { center: bounds.center; radius: bounds.width / 2 - 2; fill: backgroundColor; }
    arc { center: bounds.center; radius: bounds.width / 2 - 6; from: 135deg;
          to: lerp (135deg, 405deg, normalized); stroke: trackColor; strokeWidth: 3; }
}
```

```cpp
auto style = DrawingStyle::create (slider, drawing); // the parsed `drawing` tree
if (style.wasOk())
    slider.setStyle (style.getReference());
```

See [Data-driven drawing](yui-drawing.md) for the elements, the `let`, `layer`,
`group` and `repeat` scopes, and error reporting.

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

When a component's style changes (via `setStyle`, or when a color or property
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

The global theme singleton holds fonts, default colors and properties, and
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

The built-in theme uses a 14 px default font. Buttons, combo boxes, labels, text
editors, list rows and menus all draw their text at that height (smaller only
when the widget is too short to fit it).

### Batch color/property registration

```cpp
theme->setColors ({
    { TextButton::Style::backgroundColorId, Colors::cornflowerblue },
    { Slider::Style::thumbColorId, Colors::orange },
});

theme->setProperties ({
    { "corner-radius", 8.0 },
    { "padding", 4.0 },
});
```

### Finding values through the theme

Theme `findColor` checks the component's parent chain first, then falls back to
theme defaults. Theme `findProperty` only holds the theme-wide property defaults;
the component/parent cascade lives on `Component::findProperty`:

```cpp
// findColor first calls component.findColor, then checks theme defaults
auto c = theme->findColor (comp, "background");

// findProperty returns the theme default for the id (component cascade is comp.findProperty)
auto m = theme->findProperty (comp, "corner-radius");

// Static convenience wrappers on the global theme:
auto c = ApplicationTheme::findComponentColor (comp, "background");
auto m = ApplicationTheme::findComponentProperty (comp, "corner-radius");
```

---

## Palettes

A `ThemePalette` turns a few source colors, for example the export of an online
palette generator, into the semantic roles a theme needs:

| Role | Used for |
|---|---|
| `background` | Window and page background |
| `surface` | Fields, lists and other inset areas |
| `surfaceRaised` | Buttons, menus and other raised areas |
| `outline` | Borders and separators |
| `text` / `textMuted` | Primary and secondary text |
| `accent` / `onAccent` | Selection, focus and active values, and text drawn on them |

The roles are derived deterministically (the order of the source colors does not
matter), text is always readable against the background, and missing colors are
derived, so a single source color still gives a complete palette. Palettes are
dark by default, pass `ThemePalette::Mode::light` for a light one.

```cpp
// From colors
ThemePalette palette (std::vector<Color> { Color (0xff100b00), Color (0xff85cb33), Color (0xffefffc8) });

// From text: XML with <color hex="..."> or <color r g b> elements, CSS, hex lists or palette URLs
auto result = ThemePalette::fromString ("https://coolors.co/100b00-85cb33-efffc8-a5cbc3-3b341f",
                                        ThemePalette::Mode::dark);
if (result.failed())
    YUP_DBG (result.getErrorMessage());

// Tweak a single role
palette.setColor (ThemePalette::Role::accent, Colors::orange);
```

Switch the palette of the whole application at runtime, then repaint:

```cpp
ApplicationTheme::getGlobalTheme()->setPalette (result.getReference());
topLevelComponent->repaint();
```

A theme turns the palette roles into widget colors with a palette mapping
(`theme->setPaletteMapping()`, which calls `theme->setPaletteColor()`); the
built-in theme maps every widget color it paints. You can also pass a palette
when creating it: `createThemeVersion1 (palette)`.

Colors set with `theme->setColor()` always win over the palette and survive
palette changes, and colors set on a component win over both. Custom paint
styles should read `theme.getPalette (component).getColor (role)` at paint time
so they follow palette switches and component palettes too.

### Component palettes

The theme palette is global. To give part of the interface a palette of its own,
set it on a component: the component and everything inside it are drawn from the
closest palette up the parent chain, mapped to widget colors by the theme's
palette mapping.

```cpp
// A window, a panel or a plugin editor wears its own palette
editor.setPalette (ThemePalette::fromString ("https://coolors.co/0b132b-1c2541-3a506b-5bc0be-6fffe9").getReference());

// Follow the parent again
editor.setPalette (std::nullopt);
```

This is how several plugin editors in one process, which share the global theme,
can each wear their own palette. `ApplicationTheme::getPalette (component)` returns
the palette a component is drawn with, and `Component::findPalette()` the closest
one set on it or a parent.

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

- [How YUI fits together](yui-architecture.md) - how properties, stylesheets,
  drawings and bindings relate
- [Stylesheets](yui-styles.md) - CSS-like rules setting properties on any
  component tree
- [Component basics](component-basics.md) — `setColor`, `setProperty`, `setStyle` APIs
- [Component effects (shaders)](component-effects.md)
