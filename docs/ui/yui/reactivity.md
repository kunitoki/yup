# Reactive bindings

A property set from an expression in a `.yui` document is a *binding*, not a
one-off assignment. The expression is evaluated when the UI is built, and then
again whenever anything it reads changes, so a value written once stays correct
for the life of the UI.

```yui
Row {
    gap: 12;

    Slider { id: gain; value: 0.25; flexGrow: 1; }
    Label  { id: readout; text: format ("%.2f", gain.value); preferredWidth: 48; }
}
```

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/reactivity_binding_light.jpg#gh-light-mode-only" alt="A slider next to a label following its value" width="440">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/reactivity_binding_dark.jpg#gh-dark-mode-only" alt="A slider next to a label following its value" width="440">
</p>

The label follows the slider with no listener and no glue code: the expression
`gain.value` reads a property of another element, so it is re-evaluated each
time that property changes.

## What an expression can read

Expressions resolve their symbols through `DrawingScope`, the same way drawings
do:

| Symbol             | Resolves to                                      |
|--------------------|--------------------------------------------------|
| `value`, `width`   | a property of the component being written        |
| `self.value`       | the same, written explicitly                     |
| `parent.width`     | a property of the parent component               |
| `gain.value`       | a property of the element whose `id` is `gain`   |
| `palette.accent`   | a role of the component's palette                |
| `theme.accent`     | a role of the palette of the application theme   |
| `theme.dark`       | true when that palette is a dark one             |

Only properties that are reflected (see [component
styling](../component-styling.md)) can be read, including the read-only state
properties `hovered`, `pressed` and `focused`, which makes interaction states
usable as plain values:

```yui
Slider { id: knob; }
Label  { text: "tweaking"; opacity: knob.pressed ? 1 : 0.3; }
```

A binding is evaluated again when a component tells that a property it reads
changed. Every property set by a document, by another binding or with
`setProperty()` is told. So are the value of a `Slider` or of a wheel, the text
of a `TextEditor`, the selection of a `ListBox`, a `ComboBox` or a `TabBar`,
the state of a toggle and the levels of a `KMeter`, when the user or the widget
changes them. What a widget computes for its painting is not told: its
geometry, such as `trackBounds`, a scroll offset, the phase of an animation,
and its [series](drawing.md#data-series). The drawing of the widget reads them
each time it paints. From C++, change a property a binding reads with
`setProperty()`: some widgets have setters that change it without telling.

The whole element tree is created before any value is applied, so an expression
can reference an element declared further down the document.

## How updates run

`BindingGraph` records every property an expression reads while compiling it,
and listens to the components those properties belong to. A change marks the
bindings that depend on it dirty and evaluates them immediately, in dependency
order, before the call that caused the change returns. Updates are iterative
rather than recursive, so a long chain of bindings costs a loop and not a deep
stack.

Expressions must be free of side effects, since the graph decides when and how
often they run. A binding that keeps retriggering itself is removed after a few
passes and reported through `BindingGraph::getDiagnostics`, so a cycle can
never hang the UI.

An expression that reads nothing which can change, such as `width: 200;` or
`color: #202020;`, is evaluated once and no binding is kept.

## Bindings break on an explicit value

A binding is dropped as soon as something else writes the property it feeds,
whether from code or from the user dragging the widget. An explicit value
always wins, and the property keeps the value it was given:

```cpp
auto* readout = document.findById<Label> ("readout");
readout->setProperty ("text", "muted");    // the binding on text is gone
```

A change the property only inherits is not a write. A component whose `enabled`
is bound keeps its binding, and the state the binding gave it, when a parent is
disabled and enabled again around it.

Bindings are also dropped when the component they write to, or any component
they read from, is deleted.

A two-way binding, written `value <=> other.value;`, is a link rather than a
binding: it keeps both properties equal whichever side is written, and is not
broken by explicit values. See [Complete UIs in `.yui`](documents.md).

## Using bindings from C++

The graph works on any component tree, not only on documents, so a hand-written
UI can use it directly:

```cpp
BindingGraph graph;
auto resolve = [&] (const String& id) { return findChildWithID (id); };
graph.bind (meter, "value", "gain.value * 2", ComponentProperty::Type::number, resolve);
```

`YuiBuilder` owns one graph for everything it builds, reachable with
`YuiBuilder::getBindings()` (or `YuiDocument::getBuilder().getBindings()`),
where bindings can be inspected, removed with `unbind`, or added to the built
tree.

## See also

- [How YUI fits together](architecture.md) - diagrams of the binding classes
  and how a change propagates.
- [Complete UIs in `.yui`](documents.md) - tokens, composite components,
  handlers, two-way bindings and models.
- [Data-driven drawing](drawing.md) - the expressions and the scope
  bindings share with drawings.
- [Component styling](../component-styling.md) - the reflected property model
  expressions read and write.
