# MIDI Keyboard

`MidiKeyboardComponent` (module `yup_audio_gui`) draws a piano keyboard that
plays into a `MidiKeyboardState` and shows the notes held in it, whether they
come from the mouse, a touch screen, the computer keyboard or a MIDI input.

```cpp
yup::MidiKeyboardState keyboardState;
yup::MidiKeyboardComponent keyboard (keyboardState, yup::MidiKeyboardComponent::horizontalKeyboard);

keyboard.setAvailableRange (36, 84); // C1 to C5 can be reached
keyboard.setLowestVisibleKey (48);   // show 48..84, scrollable down to 36
keyboard.setMidiChannel (1);
keyboard.setVelocity (0.7f);
addAndMakeVisible (keyboard);
```

## Mouse and touch

Every pointer plays its own note: the mouse and each finger press, slide and
release independently, so chords can be held with several fingers and lifting
one finger leaves the others sounding. A note held by two fingers sounds once
and stops when the last one lets go.

The velocity grows from the back edge of the key (quiet) to its front edge
(loud), scaled by `setVelocity()`. Call `setMidiVelocitySensitive (false)` to
play every press at the `setVelocity()` value.

Subclasses can intercept presses by overriding `mouseDownOnKey()`,
`mouseDraggedToKey()` (return `false` to suppress the note) and
`mouseUpOnKey()`.

## Available and visible range

- `setAvailableRange (lo, hi)` sets the keys that can be reached, and shows all
  of them.
- `setVisibleRange (lo, hi)`, `setLowestVisibleKey()` and
  `setHighestVisibleKey()` choose the window shown, always inside the available
  range.
- `onVisibleRangeChanged` is called whenever the window moves or is resized.

While the window doesn't show the whole available range, a scroll button at each
end of the keyboard shifts it by an octave. A button dims once it can't scroll
any further. `setScrollButtonsVisible (false)` removes them, and
`setScrollButtonWidth()` changes their size. The mouse wheel scrolls by single
white keys and Ctrl/Cmd + wheel zooms, both kept inside the available range.

## Computer keyboard

With keyboard focus, the keys `zsxdcvgbhnjmq2w3er5t6y7ui9o0p` play consecutive
semitones from the octave set by `setKeyPressBaseOctave()` (3 by default, so
`z` plays note 36). `setKeyboardKeys()` changes the mapping.

## Appearance

- `setOrientation()` lays the keys out horizontally or vertically (facing left
  or right).
- `setBlackNoteLengthProportion()` and `setBlackNoteWidthProportion()` size the
  black keys relative to the white ones.
- `setOctaveForMiddleC()` names the octaves in the key labels (note 60 is "C3"
  by default). Override `getWhiteNoteText()` for custom labels.
- `setMidiChannelsToDisplay()` picks the channels shown as pressed.

Colors come from `MidiKeyboardComponent::Style`, see
[Component styling](component-styling.md). The scroll buttons are
`MidiKeyboardComponent::ScrollButton` components, with a theme style of their own.
