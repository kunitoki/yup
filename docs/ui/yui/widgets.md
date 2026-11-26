# Widgets in drawings

A `drawing:` rule replaces what paints a widget, and keeps everything else:
its layout, its input, its value. This page lists what each widget tells a
drawing, so that the drawing can paint it whole.

A drawing paints what it likes of the widget. Where it paints nothing, or a
translucent color, what is behind the widget shows: a drawing needs no
background of its own.

A drawing reads three kinds of things from the component it paints:

- its **properties**, by name: its state (`value`, `checked`, `hovered`), the
  colors and sizes a stylesheet set, and the geometry listed below
- its **series**, arrays read where the widget keeps them: see
  [Data series](drawing.md#data-series)
- its **parts**, what a drawing has no shape for, painted by the theme and
  placed with a `part` element: see
  [Parts of a widget](drawing.md#parts-of-a-widget)

Every component also has `bounds`, `enabled`, `hovered`, `pressed` and
`focused`, and the roles of its palette as `palette.accent` and the
others. `yup_yui_tool schema` prints the full list for every type, with the
series and the parts.

## Controls

The geometry a widget computes for itself is read-only: the drawing paints
where the widget tells, and the pointer hits what is painted.

```yui
drawing Fader {
    let y: trackBounds.centerY;
    let x: trackBounds.x + normalized * trackBounds.width;

    line { from: point (trackBounds.x, y); to: point (trackBounds.right, y); stroke: palette.outline; strokeWidth: 4; cap: round; }
    line { from: point (trackBounds.x, y); to: point (x, y); stroke: palette.accent; strokeWidth: 4; cap: round; }
    circle { center: point (x, y); radius: dragging ? 9 : 7; fill: palette.text; }
}

drawing Switch {
    let slot: trackBounds.reduced (4);

    rect { bounds: slot; cornerRadius: slot.height / 2; fill: checked ? palette.accent : palette.surfaceRaised; }
    ellipse { bounds: thumbBounds.reduced (4); fill: palette.text; }
}

style {
    Slider       { drawing: Fader; }
    SwitchButton { drawing: Switch; }
}
```

| Widget | Properties a drawing reads | Parts |
| --- | --- | --- |
| `TextButton` | `text`, `textBounds` | `text` |
| `ToggleButton` | `text`, `checked` | `text` |
| `SwitchButton` | `checked`, `vertical`, `trackBounds` (the switch, of the size its style gives it and centered), `thumbBounds` (it slides when toggled) | - |
| `Label` | `text`, `justification`, `strokeWidth` | `text`, `textOutline` |
| `ComboBox` | `text`, `open`, `items`, `selectedIndex`, `textBounds`, `arrowBounds` | `text` (takes `bounds`, goes in `textBounds` without) |
| `Slider` | `value`, `normalized`, `minValue`, `maxValue`, `minNormalized`, `maxNormalized`, `dragging`, `sliderType`, `trackBounds`, `thumbBounds`, `thumbSize` | - |
| `Slider` with `incDecButtons` | `decrementBounds`, `incrementBounds`, `pressedStep`, `decimalPlaces` (the theme shows 3 at most) | - |
| `ScrollBar` | `thumbBounds`, `trackBounds`, `thumbHovered`, `dragging`, `scrollingNeeded`, `visibilityMode` | - |
| `ProgressBar` | `progress`, `indeterminate`, `phase` | - |
| `Image` | `source`, `fitting`, `justification`, `imageOpacity` | `image` (takes `bounds`) |
| `ResizableDivider`, `ResizableCorner`, `JoystickComponent` | Their state, see the schema | - |

- `thumbBounds` of a `SwitchButton` and `phase` of an indeterminate
  `ProgressBar` move at every frame while they animate: the widget repaints,
  and the drawing reads where they are.
- A linear `Slider` tells its track and its thumb: `thumbBounds` is centered
  where `normalized` puts the value along `trackBounds`, as large as
  `thumbSize`, and is what a press grabs. A press on the track beside it brings
  it under the pointer. The two thumbs of a two-value slider are at
  `minNormalized` and `maxNormalized`.
- A `SwitchButton` and a stepper keep their size in a larger component and
  are centered in it: `trackBounds` tells where they are. A press beside the
  switch, in the room the component has around it, is not a press of the
  switch.

## Text

What an editor shows is laid out, scrolled and edited by the editor. A drawing
paints the frame and places the parts:

```yui
drawing Field {
    rect { bounds: bounds.reduced (1); cornerRadius: 6; fill: palette.surface;
           stroke: focused ? palette.accent : palette.outline; strokeWidth: focused ? 2 : 1; }

    part selection { fill: palette.accent.withAlpha (0.35); }
    part text { fill: palette.text; }
    part caret { fill: palette.accent; }
}
```

| Widget | Properties a drawing reads | Parts |
| --- | --- | --- |
| `TextEditor` | `text`, `readOnly`, `multiLine`, `textBounds`, `caretBounds`, `caretVisible`, `hasSelection`, `scrollOffset` | `selection`, `text`, `caret` |
| `CodeEditor` | `gutterWidth`, `textBounds`, and its settings | `content` |

- The parts of a `TextEditor` are where the editor puts them. `caret` paints
  nothing while the caret is hidden, so it needs no `visible`.
- A `CodeEditor` is one part, `content`: the editor as the theme paints it,
  with the colors of its scheme. A drawing frames it, or paints over it.

## Lists, trees and tabs

Rows and tabs are components: each is painted by its own drawing, and tells
where its icon and its text go.

```yui
drawing ListRow {
    rect { bounds: bounds; fill: selected ? palette.accent : (hovered ? palette.text.withAlpha (0.06) : #00000000); }
    part icon { }
    text { bounds: textBounds; text: text; justification: centerLeft; fill: selected ? palette.onAccent : palette.text; }
}

style {
    ListBoxItem { drawing: ListRow; }
}
```

| Widget | Properties a drawing reads | Parts |
| --- | --- | --- |
| `ListBox` | `orientation`, `scrollPosition`, `refreshing`, `refreshProgress`, `refreshIndicatorSize` | - |
| `ListBoxItem` | `text`, `selected`, `hovered`, `iconPosition`, `hasIcon`, `iconBounds`, `textBounds` | `icon` (takes `bounds`) |
| `TreeViewRow` | `text`, `depth`, `selected`, `hovered`, `openFraction`, `hasIcon`, `hasCustomContent`, `disclosureBounds`, `iconBounds`, `textBounds`, `indentSize`, `indentGuidesVisible` | `icon` (takes `bounds`) |
| `TabBar` | `orientation`, `variant`, `flipped`, `selectedTab`, `indicatorBounds` (it slides to the selected tab), `focusIndicatorVisible` | - |
| `TabButton` | `text`, `iconGlyph`, `selected`, `closable`, `closeButtonOver`, `hasIcon`, `iconBounds`, `textBounds`, `closeBounds`, `arrowBounds` | `icon` (takes `bounds`) |

- An icon given as a glyph is a text: draw it with
  `text { bounds: iconBounds; text: iconGlyph; font: theme.iconFont; }`.
- The rows of a `ListBox` made with a custom model are whatever components the
  model returns, painted by their own style.

## Menus

The items of a `PopupMenu` are not components. The menu tells them as series
of the same length, read with one index:

```yui
drawing Menu {
    rect { bounds: bounds; cornerRadius: 6; fill: palette.surfaceRaised; }

    repeat i in 0 .. len (itemTexts) {
        rect { bounds: itemBounds[i].reduced (2, 1); cornerRadius: 3; fill: palette.accent.withAlpha (0.3);
               visible: itemHovered[i] && itemEnabled[i] && ! itemSeparator[i]; }

        line { from: point (itemBounds[i].x + 8, itemBounds[i].centerY); to: point (itemBounds[i].right - 8, itemBounds[i].centerY);
               stroke: palette.outline; visible: itemSeparator[i]; }

        text { bounds: itemBounds[i].reduced (12, 2); text: itemTexts[i]; justification: centerLeft;
               fill: itemEnabled[i] ? palette.text : palette.textMuted; visible: ! itemSeparator[i] && ! itemCustom[i]; }

        text { bounds: itemBounds[i].reduced (12, 2); text: itemShortcuts[i]; justification: centerRight;
               fill: palette.textMuted; visible: ! itemSeparator[i]; }
    }
}

style {
    PopupMenu { drawing: Menu; }
}
```

| Series | Type | Tells of each item |
| --- | --- | --- |
| `itemBounds` | `rectangle[]` | Its area, empty when it is scrolled out of the menu. |
| `itemTexts`, `itemShortcuts` | `string[]` | Its text, and the shortcut beside it. |
| `itemColors` | `color[]` | The color it asks for its text, transparent when it takes the one of the menu. |
| `itemEnabled`, `itemTicked`, `itemHovered` | `boolean[]` | Whether it can be chosen, is ticked, is under the pointer or the keyboard. |
| `itemSeparator`, `itemSubMenu`, `itemActive` | `boolean[]` | Whether it is a separator, opens a menu, and has that menu open. |
| `itemCustom` | `boolean[]` | Whether it is a component of its own, which paints itself. |

The menu also has `needsScrolling`, `canScrollUp`, `canScrollDown`,
`scrollUpBounds` and `scrollDownBounds`, for the arrows of a menu taller than
its room.

## Audio

The displays of `yup_audio_gui` tell what they show as series, so the same
numbers draw a line, an area or a shader:

```yui
drawing Spectrum {
    rect { bounds: bounds; cornerRadius: 6; fill: palette.surface; }

    repeat i in 0 .. bounds.width {
        let x: bounds.x + i;

        vertex { at: point (x, bounds.bottom - sample (levels, i / bounds.width) * bounds.height);
                 base: point (x, bounds.bottom); smooth: 0.5;
                 fill: palette.accent.withAlpha (0.25); stroke: palette.accent; strokeWidth: 1.5; }
    }
}

drawing Keys {
    repeat i in 0 .. len (keyNotes) {
        rect { bounds: keyBounds[i].reduced (0.5); cornerRadius: 2; visible: ! keyBlack[i];
               fill: keyPressed[i] ? palette.accent : palette.text; }
    }

    repeat i in 0 .. len (keyNotes) {
        rect { bounds: keyBounds[i]; cornerRadius: 2; visible: keyBlack[i];
               fill: keyPressed[i] ? palette.accent : palette.surfaceRaised; }
    }
}

style {
    SpectrumAnalyzer { drawing: Spectrum; }
    MidiKeyboard     { drawing: Keys; }
}
```

| Widget | Series and properties a drawing reads |
| --- | --- |
| `SpectrumAnalyzer` | `levels` (`number[]`): 512 levels from 0 to 1 between `minDecibels` and `maxDecibels`, evenly spaced along the displayed frequency axis. |
| `KMeter` | `peakDb`, `averageDb`, `peakHoldDb`, `clipping`, `rangeMinDb`, `rangeMaxDb`. |
| `MidiKeyboard` | `keyBounds`, `keyNotes`, `keyBlack`, `keyPressed`, `keyHovered`: one value for each visible key, from the lowest. The black keys lie over the white ones, so they are drawn after them. |
| `PitchWheel`, `ModWheel` | `value`. |

- A `SpectrumAnalyzer` with a drawing paints nothing of its own: the grid and
  the labels are the drawing's to add.
- The states behind these widgets are given from C++, see
  [Setting up components from C++](documents.md#setting-up-components-from-c).

## What stays with the widget

- `Spectrogram`, `CartesianPlane` and `AudioView` paint themselves: what they
  show is not one series.
- The drop indicator of a `TreeView` is painted over its rows, in the color of
  `dropIndicatorColor`.
- `Artboard` paints its Rive content.

## See also

- [Data-driven drawing](drawing.md) - the elements of a drawing.
- [Stylesheets](styles.md) - selecting the widgets a drawing paints.
- [Shaders](shaders.md) - a series as the input of a shader.
- [Component styling](../component-styling.md) - declaring series and parts
  for a component of your own.
