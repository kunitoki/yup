# Widgets

The components that ship with the framework, beyond the buttons, sliders,
labels and text editors covered by [component basics](component-basics.md).

- [Artboards (Rive)](artboard.md) - `ArtboardFile`, `Artboard`, `ArtboardNode`
  and ViewModel data binding.
- [Code editor](code-editor.md) - `CodeDocument`, `SyntaxDefinition`,
  `CodeTokeniser` and the syntax-highlighting `CodeEditor`.
- [Joystick](joystick.md) - `JoystickComponent`, an XY pad moving a thumb over
  two ranged axes, with overlapping thumb-only joysticks for graph nodes.
- [List box](list-box.md) - `ListBox` and `ListBoxModel`, custom rows, change
  notifications, touch scrolling and pull-to-refresh.
- [MIDI keyboard](midi-keyboard.md) - `MidiKeyboardComponent`, multitouch
  playing, scroll buttons and the available / visible key ranges.
- [Resizable divider](resizable-divider.md) - `ResizableDivider`, a draggable
  bar splitting two panes, with limits, resize policies, snapping, collapsing
  and keyboard control.
- [Tabs](tabs.md) - `TabBar`, `TabButton` and `TabComponent`: segmented
  controls and tabbed pages with animated selection, reordering, overflow and
  closable tabs.
- [Tree view](tree-view.md) - `TreeView` and `TreeViewItem`, lazy loading,
  custom content, drag and drop reordering, `DataTree` mirroring and saved
  openness state.

## Typing the value of a control

A `Slider` drags finer while a modifier key is held, and goes back to its
default value on a click with another one:

```cpp
slider.setMouseDragSensitivity (0.3);
slider.setFineDragScale (0.1);                         // ten times slower...
slider.setFineDragModifiers (KeyModifiers::shiftMask); // ...while Shift is held, for a drag and the wheel
slider.setResetModifiers (KeyModifiers::altMask);      // Alt+click brings the default value back
```

A double click does the same, unless a `ComponentListener` takes it by
returning true from `componentDoubleClicked`, which any component can be
listened for. `InPlaceTextEditor` is what to open then: a single line
`TextEditor` over a component, in the interface that component is in. Return,
a press elsewhere and the loss of the keyboard focus commit, Escape cancels:

```cpp
bool componentDoubleClicked (Component&, const MouseEvent&) override
{
    editor.onCommit = [this] (const String& text) { slider.setValue (text.getDoubleValue()); };
    editor.open (valueLabel, String (slider.getValue()));
    return true;
}
```

A `TextEditor` tells the same with `onReturnKey`, `onEscapeKey` and
`onFocusLost`. In `.yui` all of this is a handler, see
[Editing a value as text](yui/documents.md#editing-a-value-as-text).

Every widget can be painted by a `.yui` drawing instead of the theme:
[Widgets in drawings](yui/widgets.md) lists the geometry, the data series and
the parts each one gives a drawing.

```{toctree}
:hidden:
:maxdepth: 1

Artboards (Rive) <artboard>
Code Editor <code-editor>
Joystick <joystick>
List Box <list-box>
MIDI Keyboard <midi-keyboard>
Resizable Divider <resizable-divider>
Tabs <tabs>
Tree View <tree-view>
```
