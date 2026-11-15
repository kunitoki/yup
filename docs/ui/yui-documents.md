# Complete UIs in `.yui`

A `.yui` document can describe a whole interface: the component tree with its
layout, reusable composite components, shared tokens, event handlers and the
application data the widgets edit. `YuiDocument` loads such a document and
gives back a live component tree.

```yui
tokens { property spacing: number = 10; }

component LabeledKnob : Column {
    property label: string = "VALUE";
    property alias value <=> knob.value;
    signal onChanged (value: number);

    Slider { id: knob; sliderType: rotaryVerticalDrag; flexGrow: 1; onValueChanged: root.onChanged (value); }
    Label  { text: root.label; preferredHeight: 24; }
}

Row {
    padding: spacing;
    gap: spacing;

    LabeledKnob { id: cutoff; label: "CUTOFF"; value <=> params.cutoff;
                  onChanged: action ("changed", "Cutoff", value); }
    TextButton  { text: "Reset"; onClick: action ("reset"); }
}
```

## Loading a document

```cpp
#include <yup_gui/yup_gui.h>

YuiDocument document;
document.setModel ("params", paramsProvider);
document.setAction ("reset", [this] (const Array<var>&) { resetParams(); });

const auto result = document.loadFromString (text, [&] (const String& path) -> std::optional<String>
{
    return folder.getChildFile (path).loadFileAsString();
});

if (result.wasOk())
    addAndMakeVisible (*document.getRoot());
```

`loadFromString` parses the text and its imports, builds the stylesheet of its
`palette` and `style` blocks, creates the components and attaches the
stylesheet to the root. The document owns the components: keep it alive as long
as the root is shown, and lay the root out like any other child.

When the text has a syntax error, an invalid stylesheet or an unknown component
type, the interface already loaded is kept, so a live editor keeps showing the
last good version, and the failed result carries the error with its location.
Values that can't be applied, such as an unknown property, don't make loading
fail. Both are listed by `getDiagnostics()`, each a `YuiDiagnostic` holding the
message, the import path (empty for the main text), the line and the column:

```cpp
for (const auto& problem : document.getDiagnostics())
    editor.setErrorLine (problem.line - 1, true); // lines and columns start at 1

DBG (YuiDiagnostic::join (document.getDiagnostics())); // "style.yui:7:9: Unknown cursor 'finger'"
```

Style rules are checked when they start matching, so a `:hover` rule with a bad
value is only met once the pointer is over a component. Such late problems are
added to the list and reported to `YuiDocument::Listener::documentDiagnosed`,
once each, while the problems of a load come with `documentLoaded`.

Elements with an `id` are found from C++ with `findById`. Only the IDs of the
document itself are visible, not those inside `component` blocks:

```cpp
if (auto* cutoff = document.findById ("cutoff"))
    cutoff->setProperty ("label", "FREQ");
```

## Showing a document

`YuiView` is a component owning a document and a [`YuiLoader`](yui-hot-reload.md)
filling it. After each load the root is its child, filling its bounds, so the
interface follows the size and the enabled state of the view, and a root
replaced by a reload takes the place of the previous one:

```cpp
YuiView view;
view.getDocument().setModel ("params", paramsProvider);
view.getLoader().loadFile (folder.getChildFile ("ui.yui"));
addAndMakeVisible (view);

enabledToggle.onClick = [&] { view.setEnabled (enabledToggle.getToggleState()); };
```

To host the root another way, a `YuiDocument::Listener` hears about every load,
successful or not, including the loads of undo and redo.

## Reloading

Loading again updates the interface in place, which makes live editing cheap
and safe for the application:

- Elements are matched by `id`, then by type and position, so the components
  still in the document are kept with their pointers. A pointer from
  `findById` stays valid across reloads as long as its element is there.
- A constant value written the same way as before isn't set again, so what the
  user changed since, such as a dragged slider, is kept. Change the text of a
  value to apply it again.
- Expressions, `<=>` links and handlers are always applied again.
- A value the document no longer writes goes back to what a new component has,
  and the components of removed elements are destroyed.
- Changing the type of an element creates a new component.

Reloads can be undone. With an `UndoManager` set, each successful load is one
transaction, and undoing it loads the previous document in place:

```cpp
document.setUndoManager (&undoManager);
document.loadFromString (editedText);

undoManager.undo(); // back to the previous document, keeping the components
```

`YuiTreeDiff::apply` is the patch behind this, usable on its own to update any
`.yui` tree from another, optionally recording the changes in an
`UndoManager`.

To load from files, reload when they change on disk, or load files embedded in
the binary, use a `YuiLoader`, see [Hot reloading](yui-hot-reload.md).

## Tokens

A `tokens` block declares typed values shared by the whole document:

```yui
tokens {
    property spacing: number = 10;
    property rowHeight: number = 24;
}
```

Any element reads a token by its bare name, as in `gap: spacing;`, unless it
has a property of the same name. Tokens are properties of the root component,
so changing one from C++ updates everything reading it:

```cpp
document.getRoot()->setProperty ("spacing", 16);
```

## Composite components

A `component` block declares a new type made of other components:

```yui
component LabeledKnob : Column {
    property label: string = "VALUE";
    property alias value <=> knob.value;
    signal onChanged (value: number);

    class: labeled;
    gap: 2;

    Slider { id: knob; ... }
    Label  { text: root.label; }
}
```

- An instance *is* a component of the base type (here a `Column`), with extra
  properties, so stylesheets match it as `LabeledKnob` or `Column`, and C++
  reaches it as a `ColumnComponent`.
- `property name: type = default;` declares a typed property, such as a
  `number`, `integer`, `boolean`, `string`, `color`, `point`, `rectangle` or an
  array of them (`number[]`). Inside the block it is read as
  `root.label`; from outside it is set like any other property.
- `property alias value <=> knob.value;` exposes a property of a part under a
  new name, with the type of the original.
- `signal onChanged (value: number);` declares a signal, emitted by a handler
  inside the block and handled where the component is used.
- Inside the block, `root` is the instance and the IDs are private: every
  instance has its own `knob`, and the document can't see it.

Where the component is used, values override those written in the block,
classes add to the block's, and children are added after the block's own:

```yui
LabeledKnob { label: "RESO"; class: accent; value <=> params.resonance; }
```

`document.findTypeInfo ("LabeledKnob")` returns the declared type, to list its
properties and signals from C++.

## Handlers and actions

A property named after a signal is a handler. It holds one call, run every time
the signal is emitted:

```yui
TextButton { text: "Reset"; onClick: action ("reset"); }
Slider     { onValueChanged: action ("changed", "Cutoff", value); }
Slider     { onValueChanged: root.onChanged (value); }
```

- `action ("name", arguments...)` runs the function registered with
  `setAction`, passing the values of the arguments. With no arguments, the
  action receives the arguments of the signal instead.
- `root.signal (arguments...)` or `id.signal (...)` emits a signal declared by
  a `component` block.

Arguments are expressions evaluated when the signal fires, against the
component the handler is written on, so `value` above is the slider's current
value. Actions are looked up when they run, so they can be registered before or
after loading.

| Component      | Signal                  | Arguments  |
|----------------|-------------------------|------------|
| `TextButton`   | `onClick`               |            |
| `ToggleButton` | `onClick`               |            |
| `SwitchButton` | `onClick`               |            |
| `Slider`       | `onValueChanged`        | the value  |
| `TextEditor`   | `onTextChange`          |            |
| `ComboBox`     | `onSelectedItemChanged` |            |
| `ListBox`      | `onSelectionChanged`    |            |
| `TabComponent` | `onTabChanged`          | the tab id |

```cpp
document.setAction ("changed", [this] (const Array<var>& args)
{
    status.setText (args[0].toString() + " changed to " + args[1].toString());
});
```

Don't load a document again synchronously from inside an action: the component
emitting the signal would be destroyed while it runs.

## Two-way bindings

`x <=> a.b;` links two properties both ways. The property first takes the value
it is linked to, then a change on either side is copied to the other:

```yui
Slider { id: fine; }
Slider { value <=> fine.value; }
```

Unlike the one-way bindings described in [Reactive
bindings](yui-reactivity.md), a link is not broken when either property is
written: dragging either slider moves the other. It goes away with either
component.

## Models

A model exposes application data to the document under a name, so widgets bind
to it like to any other element:

```yui
LabeledKnob { value <=> params.cutoff; }
Label       { text: format ("cutoff %.0f%%", params.cutoff * 100); }
```

A model is a `BindingProvider`: a set of typed values that can be read, written
and observed. `DataTreeBindingProvider` exposes the properties of a `DataTree`,
typed from the values it holds when it is created. A value holding an array
becomes an array property, of the type its elements share, read with
`params.names[0]` and `len (params.names)`:

```cpp
DataTree params ("params");
UndoManager undoManager;

{
    auto transaction = params.beginTransaction();
    transaction.setProperty ("cutoff", 0.65);
    transaction.setProperty ("resonance", 0.3);
}

DataTreeBindingProvider provider (params, &undoManager);

YuiDocument document;
document.setModel ("params", provider);
```

Set models before loading. The data lives outside the document, so values
survive any reload, even one that creates the components again, and changes
made to the tree by anything else, such as undo or loading a preset, show up in
the UI immediately.

A press on a component linked to the model starts a *gesture*, which ends on
release. Any other change, from the keyboard, the mouse wheel or code, is a
gesture of its own. `DataTreeBindingProvider` starts a new undo transaction for
each gesture, so `undoManager.undo()` reverts a whole knob drag, or one arrow
key step, at once. Other providers can use gestures to tell a host a parameter
is being automated.

### Audio parameters

In a plugin editor, `AudioParameterBindingProvider` (in `yup_audio_gui`)
exposes the parameters of a processor, each named after its ID and holding its
real value:

```cpp
MyEditor (MyProcessor& processor)
    : params (*this, processor.getParameters())
{
    startParameterListening (processor);
    document.setModel ("params", params);
}
```

```yui
Slider { minimum: 20; maximum: 20000; skew: 0.25; value <=> params.cutoff; }
```

A drag, a key press or a wheel step becomes a change gesture of the parameter,
so the host records automation, and changes made by the host or the audio
thread reach the UI on the editor's parameter dispatch. Writes and gestures on
read-only parameters, such as meters, are ignored. Give a slider the range of its parameter with `minimum`,
`maximum`, `interval` and `skew`; `minValue` and `maxValue` are the thumbs of a
two-value slider, not its range.

## See also

- [How YUI fits together](yui-architecture.md) - diagrams of the classes a
  document builds and how they relate.
- [Reactive bindings](yui-reactivity.md) - how expression values stay up to
  date.
- [Hot reloading `.yui` files](yui-hot-reload.md) - loading files and
  resources, and watching files while developing.
- [The `.yui` language](yui-language.md) - the syntax of every block.
- [Stylesheets](yui-styles.md) - selectors, palettes and transitions.
- [Data-driven drawing](yui-drawing.md) - drawings and their properties.
- [Component Layout](component-layout.md) - the `Row`, `Column`, `Grid` and
  `Stack` containers and their item properties.

The `YuiDrawing` graphics example edits a complete UI built this way, split
across `ui.yui`, `components.yui`, `knob.yui`, `synthknob.yui`, `palette.yui`
and `style.yui`. It rebuilds shortly after each keystroke, and on desktop it
reloads when one of the files is saved in another editor.
