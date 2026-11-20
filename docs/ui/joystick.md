# Joystick

`JoystickComponent` is an XY pad: a thumb moving over the component area that
controls two values at once. Each axis has its own `NormalisableRange` (bounds,
interval and skew), a default value and a current value.

```cpp
yup::JoystickComponent pad;

pad.setRange (yup::JoystickComponent::Axis::x, { 20.0, 20000.0, 0.0, 0.2 });
pad.setRange (yup::JoystickComponent::Axis::y, { -24.0, 24.0 });
pad.setDefaultValue (yup::JoystickComponent::Axis::x, 1000.0);
pad.setDefaultValue (yup::JoystickComponent::Axis::y, 0.0);
pad.resetToDefaults (yup::dontSendNotification);

pad.onValueChanged = [this] (double frequency, double gain)
{
    updateBand (frequency, gain);
};

addAndMakeVisible (pad);
```

The thumb center spans the whole component: the left edge is the start of the
x range and the bottom edge the start of the y range, so the y value grows
toward the top. `getNormalizedValue (axis)` returns the value mapped to `0..1`
through the range and `getThumbPosition()` the thumb center in local
coordinates.

| Method | Purpose |
| --- | --- |
| `setRange`, `getRange` | The range of an axis. Changing it constrains the default and the value. |
| `setValue`, `setValues`, `getValue` | The values, snapped to the range. `setValues` notifies once for both axes. |
| `setDefaultValue`, `resetToDefaults` | The values restored on double click or snap back. |
| `setSnapBack` | Return to the defaults when the thumb is released, like a spring-loaded stick. |
| `setGrabMode` | `GrabMode::anywhere` (default) or `GrabMode::thumb`, see below. |
| `setThumbSize` | The thumb diameter, 16 points by default. |

## Interaction

- With `GrabMode::anywhere`, a click moves the thumb under the pointer and
  dragging follows it.
- With `GrabMode::thumb`, only the thumb reacts to the mouse and it drags
  relatively from where it was grabbed. The rest of the area lets the clicks
  through, so several joysticks can cover the same area, like the band nodes
  of an equalizer drawn over a response graph.
- A double click resets both values to their defaults.
- The arrow keys move the thumb by one interval of the range, or by 1% of a
  continuous range, ten times that with Shift.

`onValueChanged (x, y)` reports the values, while `onDragStarted` and
`onDragEnded` bracket a drag.

## Styling

| Color id | Used for |
| --- | --- |
| `Style::backgroundColorId` | The pad fill. |
| `Style::outlineColorId` | The pad border. |
| `Style::guideColorId` | The cross lines through the thumb. |
| `Style::thumbColorId` | The idle thumb. |
| `Style::thumbOverColorId` | The hovered thumb. |
| `Style::thumbDownColorId` | The dragged thumb. |

The default theme only draws the thumb in `GrabMode::thumb`, since the area
belongs to whatever is under it.

## In YUI

`JoystickComponent` is available in `.yui` files, with the per-axis properties
`xMinimum`, `xMaximum`, `xInterval`, `xSkew`, `xDefault`, `xValue` and the
read-only `xNormalized` (and the same for `y`), plus `grabMode`, `thumbSize`,
`snapBack`, the `dragging` state and the `onValueChanged`, `onDragStarted` and
`onDragEnded` signals. A drawing can read `xNormalized` and `yNormalized` to
paint a custom node.

```
JoystickComponent {
    id: lowBand
    grabMode: thumb
    xMinimum: 20; xMaximum: 20000; xSkew: 0.2
    yMinimum: -24; yMaximum: 24
    xValue <=> eq.lowFreq
    yValue <=> eq.lowGain
}
```
