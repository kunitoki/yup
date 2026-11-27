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
#include <yup_gui_markup/yup_gui_markup.h>

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
fail. Both are listed by `getDiagnostics()`, each a `DataTreeDiagnostic` holding the
message, the import path (empty for the main text), the line and the column:

```cpp
for (const auto& problem : document.getDiagnostics())
    editor.setErrorLine (problem.line - 1, true); // lines and columns start at 1

DBG (DataTreeDiagnostic::join (document.getDiagnostics())); // "style.yui:7:9: Unknown cursor 'finger'"
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

`YuiView` is a component owning a document and a [`YuiLoader`](hot-reload.md)
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
successful or not, including the loads of undo and redo, and about each time
the interface is built again without one, for a change made to its tree or to
the range of a [`repeat`](#repeating-components).

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
- The stylesheet is kept when the `style`, `palette`, `drawing`, `shader` and
  `keyframes` blocks are unchanged, the `tokens` and `component` blocks still
  declare the same properties with the same types, and each of these blocks is
  still at the same place among the blocks of the document: running transitions
  and animations go on, and a class changed by the reload transitions as it
  would from code. What else a `tokens` or `component` block holds, the value of
  a token or the components a `component` is made of, can change too. Any other
  change to these blocks restyles the interface from scratch.

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

The loaded tree, `document.getTree()`, can also be changed in place, as a
visual editor does to write an edit without going through the text. The
interface is built again once the changes are over, from the message loop,
whatever their number, and the listeners are told as for a load. A tool or a
test with no message loop calls `document.applyPendingChanges()` to have it
done at once. The stylesheet is kept as it is by a reload, so an edit that
leaves the style alone restarts no transition. Problems found after an element
was added, removed or moved this way have no line and column until a text is
loaded again, and until then each change makes the stylesheet again.

To load from files, reload when they change on disk, or load files embedded in
the binary, use a `YuiLoader`, see [Hot reloading](hot-reload.md).

## Reading files through the document

A document also reads the files its elements name: the shaders of `shader`
blocks, the images, the models and the textures of a [3D scene](scenes.md). They are read
through the document, so they come from where its text comes from, and code
reads its own files the same way:

```cpp
// From anything that has a component of the document
if (const auto bytes = YuiDocument::readAssetFor (&component, "tables/curve.bin"))
    load (*bytes);

// Reading again when the file changes
struct CurveUser : YuiDocument::Listener
{
    void documentLoaded (YuiDocument&, const Result&) override {}

    void documentAssetChanged (YuiDocument& document, const String& path) override
    {
        if (path != "tables/curve.bin")                 // the path as it was read
            return;

        if (const auto bytes = document.readAsset (path))
            load (*bytes);
    }
};
```

- `readAsset (path)` asks the asset reader of the document first, which a
  `YuiLoader` installs to serve its files, its overrides and its embedded
  resources. Else it reads the file: an absolute path as it is, a relative one
  under the folder of the main file, then under the working directory.
- `setAssetReader()` serves files from anywhere else, an archive for example,
  for a document loaded without a `YuiLoader`. Returning nothing leaves the
  file to be read as usual.
- `getAssetFile (path)` is the file a path stands for, for what has to follow
  the references of a file. It is no file for a document loaded from embedded
  resources.
- `notifyAssetChanged (path)` tells the listeners that a file changed. A
  `YuiLoader` calls it for the files it watches, and then loads the document
  again. Call it yourself for what nobody else watches.

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

Style rules read them too, so a document keeps its sizes and colors in one
place:

```yui
tokens {
    property spacing: number = 12;
    property radius: number = 14;
    property brand: color = #ff8800;
}

style {
    TextButton { cornerRadius: radius; }
    .brand     { backgroundColor: brand; textColor: #1b1b1e; }
}

Row {
    gap: spacing;

    TextButton { text: "Open"; flexGrow: 1; }
    TextButton { text: "Save"; flexGrow: 1; }
    TextButton { text: "Export"; class: brand; flexGrow: 1; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/documents_tokens_light.jpg#gh-light-mode-only" alt="Three buttons whose gap, corner radius and one color come from the tokens of the document" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/documents_tokens_dark.jpg#gh-dark-mode-only" alt="Three buttons whose gap, corner radius and one color come from the tokens of the document" width="560">
</p>

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
  `number`, `integer`, `boolean`, `string`, `color`, `point`, `rectangle`,
  `transform`, `font`, `drawing` (the name of a drawing or of a shader),
  `any`, or an array of expression types (`number[]`, `transform[]`, ...). Inside the block it is read as
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

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/documents_composite_light.jpg#gh-light-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/documents_composite_dark.jpg#gh-dark-mode-only" alt="Three instances of a composite component made of a rotary slider and a label" width="560">
</p>

`document.findTypeInfo ("LabeledKnob")` returns the declared type, to list its
properties and signals from C++.

## Handlers and actions

A property named after a signal is a handler, run every time the signal is
emitted. It holds one call, or a block of calls run in order:

```yui
TextButton { text: "Reset"; onClick: action ("reset"); }
Slider     { onValueChanged: action ("changed", "Cutoff", value); }
Slider     { onValueChanged: (v) => root.onChanged (v * 100); }
TextButton { onClick: { action ("save"); root.onDone (); } }
```

- `action ("name", arguments...)` runs the function registered with
  `setAction`, passing the values of the arguments. With no arguments, the
  action receives the arguments of the signal instead.
- `root.signal (arguments...)` or `id.signal (...)` emits a signal declared by
  a `component` block, converting the arguments to the types it declares.
- `(a, b) => call` names the arguments of the signal, in order. The names hide
  the properties of the component with the same name, and a handler may name
  fewer arguments than the signal has.
- `{ call; call; }` runs each call in order, `{ }` does nothing. The `;` after
  a block is optional.

Arguments are expressions evaluated when the signal fires, against the
component the handler is written on, so `value` above is the slider's current
value. Actions are looked up when they run, so they can be registered before or
after loading.

| Component      | Signal                  | Arguments          |
|----------------|-------------------------|--------------------|
| `TextButton`   | `onClick`               |                    |
| `ToggleButton` | `onClick`               |                    |
| `SwitchButton` | `onClick`               |                    |
| `Slider`       | `onValueChanged`        | `value` (number)   |
| `TextEditor`   | `onTextChange`          |                    |
| `ComboBox`     | `onSelectedItemChanged` |                    |
| `ListBox`      | `onSelectionChanged`    |                    |
| `TabComponent` | `onTabChanged`          | `tab` (string, id) |

`changed property: handler;` runs a handler whenever a property of the element
changes, from the user, code or a binding, with the new value as its argument:

```yui
Slider { id: cutoff; changed value: (v) => action ("log", v); }
Label  { changed text: { action ("textChanged"); } }
```

```cpp
document.setAction ("changed", [this] (const Array<var>& args)
{
    status.setText (args[0].toString() + " changed to " + args[1].toString());
});
```

An action can change the tree of the document: the change is applied once the
handler is over (see [Reloading](#reloading)). Don't load a document again
synchronously from inside an action: it ends the handler, whose calls left are
not run, and the component emitting the signal may be destroyed while it runs.

## Two-way bindings

`x <=> a.b;` links two properties both ways. The property first takes the value
it is linked to, then a change on either side is copied to the other:

```yui
Slider { id: fine; }
Slider { value <=> fine.value; }
```

Unlike the one-way bindings described in [Reactive
bindings](reactivity.md), a link is not broken when either property is
written: dragging either slider moves the other. It goes away with either
component.

The other side can be one element of a list, as in
`checked <=> seq.steps[2];`. The index is any expression, read once when the
link is made, and a change of the property replaces that element only. An
index outside the list links to nothing.

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

## Setting up components from C++

Some components show something a document can't write: the state a keyboard
plays into, the levels a meter reads, the audio a waveform draws. It belongs to
the application, which hands it over with an *initializer*, a function run on
each component of a type when the document creates it:

```cpp
document.setInitializer<MidiKeyboardComponent> ([this] (MidiKeyboardComponent& keyboard)
{
    keyboard.setState (&processor.getKeyboardState());
});
```

```yui
Row {
    gap: 8;

    ModWheel   { preferredWidth: 36; }
    PitchWheel { preferredWidth: 36; }

    MidiKeyboard { rangeStart: 48; rangeEnd: 84; flexGrow: 1; }
}
```

A call made once after loading, on what `findById` returns, would not last: a
reload, or a `repeat` whose range changes, creates components again, and the
new ones would be left without what the first ones were given.

With an ID, the initializer only runs on the components written with it, which
tells apart two components of one type:

```cpp
document.setInitializer<SpectrumAnalyzerComponent> ("input", [this] (SpectrumAnalyzerComponent& analyzer)
{
    analyzer.setState (&processor.getInputAnalyzerState());
});

document.setInitializer<SpectrumAnalyzerComponent> ("output", [this] (SpectrumAnalyzerComponent& analyzer)
{
    analyzer.setState (&processor.getOutputAnalyzerState());
});
```

```yui
Row {
    SpectrumAnalyzer { id: input; }
    SpectrumAnalyzer { id: output; displayType: lines; }
}
```

- An initializer runs once the whole tree is created and before the values of
  the document are applied, so what the document writes wins over what the
  initializer sets.
- It runs on the components of the type and of the types derived from it,
  which the instances of a `component` block based on the type are.
- The initializers of an ID run after the ones without, whatever the order
  they were set in, so the more specific one wins. Those of one kind run in
  the order they were set.
- The ID is the one the element is written with, so an initializer reaches the
  components inside a `component` block or a `repeat`, which `findById` does
  not.
- A component a reload keeps isn't set up again, unless its ID changed.
- Initializers are set before or after loading: one set later runs right away
  on the components already there. `clear()` keeps them, and setting a null
  function removes one.
- What a function captures must outlive the document.

Initializers work for any component, yours included once its type is
registered with `ComponentTypeRegistry`. The components of `yup_audio_gui` a
document creates are listed in
[Audio GUI](../../audio/audio-gui.md#in-yui-documents).

## Repeating components

`repeat name in from .. to { ... }` makes the components it holds once for
each integer from `from` to `to - 1`, where it is written among the children
of its parent:

```yui
Row {
    repeat i in 0 .. seq.length {
        ToggleButton {
            class: step;
            text: format ("%d", i + 1);
            checked <=> seq.steps[i];
            onClick: action ("step", i);
        }
    }
}
```

- The index is an integer, read by its name in every value, link and handler
  of the body, and in the range of a `repeat` inside it. The body holds
  components and other `repeat`s.
- An `id` in the body is local to its turn: what the turn holds reads the
  component of that turn, nothing outside the `repeat` sees it and `findById`
  does not find it. The ids around the `repeat` are read from inside.
- The range reads whatever an expression reads, such as a model, a token or a
  property. When it changes, the components are made again once the change is
  over, from the message loop: those still described are kept as they are,
  the others are added or removed.
- A `repeat` runs at most 1024 times, counted together for repeats inside one
  another: 32 turns of 32. A longer range is reported as a problem and cut
  there.
- In the body of a `component`, the range can read the properties of the
  component, as in `repeat i in 0 .. root.count`.

Here the range reads a stepper, so its buttons add and remove the ones below
it:

```yui
Column {
    gap: 10;

    Slider { id: steps; sliderType: incDecButtons; minimum: 1; maximum: 8; interval: 1; value: 6; preferredHeight: 28; }

    Row {
        gap: 4; flexGrow: 1;

        repeat i in 0 .. steps.value {
            ToggleButton { text: format ("%d", i + 1); checked: i % 3 == 0; flexGrow: 1; }
        }
    }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/documents_repeat_light.jpg#gh-light-mode-only" alt="A stepper showing 6 over a row of six numbered toggle buttons made by a repeat" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/documents_repeat_dark.jpg#gh-dark-mode-only" alt="A stepper showing 6 over a row of six numbered toggle buttons made by a repeat" width="560">
</p>

A tool or a test with no message loop calls
`document.applyPendingChanges()` to have the components made again at once. It
applies the changes made to the tree of the document too.

## Exclusive buttons and steppers

Buttons with the same `radioGroup` in the same parent are exclusive: turning
one on turns the others off, and a click on the one that is on leaves it on.
Each keeps its own `checked`, so each binds to a property of its own. It works
for `ToggleButton` and `SwitchButton`, together or apart:

```yui
Row {
    ToggleButton { text: "LFO";  radioGroup: 1; checked <=> mod.lfo; }
    ToggleButton { text: "STEP"; radioGroup: 1; checked <=> mod.step; }
}
```

A `Slider` of type `incDecButtons` is a stepper: a button at each end with the
value between them, on a line when it is wider than tall and one above the
other otherwise. A press steps the value by the `interval`, or by a hundredth
of the range when there is none, and keeps stepping while it is held. The
wheel, and a drag up or down between the buttons, change it as on other
sliders:

```yui
Slider { sliderType: incDecButtons; minimum: 1; maximum: 16; interval: 1; value <=> seq.length; }
```

Both in a row, with no model:

```yui
Row {
    gap: 8;

    ToggleButton { text: "LFO";  radioGroup: 1; checked: true; flexGrow: 1; }
    ToggleButton { text: "STEP"; radioGroup: 1; flexGrow: 1; }
    ToggleButton { text: "ENV";  radioGroup: 1; flexGrow: 1; }

    Slider { sliderType: incDecButtons; minimum: 1; maximum: 16; interval: 1; value: 8; preferredWidth: 150; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/documents_radio_light.jpg#gh-light-mode-only" alt="Three exclusive toggle buttons, the first one on, beside a stepper showing 8" width="560">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/documents_radio_dark.jpg#gh-dark-mode-only" alt="Three exclusive toggle buttons, the first one on, beside a stepper showing 8" width="560">
</p>

## See also

- [How YUI fits together](architecture.md) - diagrams of the classes a
  document builds and how they relate.
- [Reactive bindings](reactivity.md) - how expression values stay up to
  date.
- [Hot reloading `.yui` files](hot-reload.md) - loading files and
  resources, and watching files while developing.
- [The `.yui` language](language.md) - the syntax of every block.
- [Stylesheets](styles.md) - selectors, palettes and transitions.
- [Data-driven drawing](drawing.md) - drawings and their properties.
- [Component Layout](../component-layout.md) - the `Row`, `Column`, `Grid` and
  `Stack` containers and their item properties.
- [Tools for `.yui` files](tooling.md) - checking and previewing documents
  outside the application.
- [`.yui` interfaces from Python](../../scripting/python-yui.md) - documents,
  actions and models from the Python bindings.

The `YuiDrawing` graphics example (the `YUI` page) edits a complete UI built
this way, its Filter, split across `ui.yui`, `components.yui`, four knob
drawings (`knob.yui`, `haloknob.yui`, `flutedknob.yui`, `brushedknob.yui`),
`palette.yui` and `style.yui` in
[`examples/graphics/data/yui/filter`](https://github.com/kunitoki/yup/tree/main/examples/graphics/data/yui/filter).
It rebuilds shortly after each keystroke, and on desktop it reloads when one
of the files is saved in another editor.

The other pages of the example are single files, each complete by itself and
covering one area:

| Page | File | Shows |
| --- | --- | --- |
| Widgets | `widgets.yui` | Every control with the readout of what it tells |
| Lists | `lists.yui` | `ListBox`, `TreeView`, `Viewport`, `TabComponent`, `CodeEditor` |
| Layout | `layout.yui` | `Row`, `Column`, `FlexBox`, `Grid`, `Stack` and item properties |
| Styles | `styles.yui` | Selectors, tokens, scoped palettes, transitions, animations, effects |
| Bindings | `bindings.yui` | Expressions, links, composite components, handlers, `repeat` |
| Drawing | `drawing.yui` | Shapes, paints, text, scopes, filters, patterns and markers |
| Series | `series.yui` | `vertex` lines, areas and bands, `sample`, `min` and `max` |
| Parts | `parts.yui` | The `part` elements of editors, buttons, labels and images |
| Controls | `controls.yui` | Widgets painted whole from their geometry, a menu from its item series |
| Audio | `audio.yui` | The analyzer, the meter and the keyboard painted from what they tell |
| Shaders | `shaders.yui` | A series sampled by a shader as a texture |

The page gives the documents what they can't write, with
[initializers](#setting-up-components-from-c): the states of the audio
widgets. The tests load and paint every page
(`YuiExamplePagesTests`), so they are the first place to look when a widget,
a part or a series stops working.
