# The `.yui` language

A `.yui` file describes an interface as text: how components paint
(`drawing`), how they look in each state (`palette` and `style`), and what the
interface is made of (`tokens`, `component` and the root instance). This page
is the reference of the syntax; the guides linked from each section describe
what the blocks do.

```yui
import "knob.yui";

palette Ember { colors: "#1b1b1e #ff8800 #f2efe9"; }

style {
    :root { palette: Ember; }
    .knob { drawing: Knob; }
}

tokens { property spacing: number = 8; }

component LabeledKnob : Column {
    property label: string = "VALUE";
    property alias value <=> knob.value;

    Slider { id: knob; class: knob; flexGrow: 1; }
    Label  { text: root.label; preferredHeight: 20; }
}

Row {
    gap: spacing;
    LabeledKnob { id: cutoff; label: "CUTOFF"; value <=> params.cutoff; }
}
```

## Files

A file is a sequence of blocks, in any order:

| Block | Holds | Guide |
|---|---|---|
| `import "path";` | The blocks of another file | [below](#imports) |
| `drawing Name { ... }` | Shapes painting a component | [Data-driven drawing](yui-drawing.md) |
| `palette Name { ... }` | A palette derived from source colors | [Stylesheets](yui-styles.md#palettes) |
| `style { ... }` | Rules and `@keyframes` | [Stylesheets](yui-styles.md) |
| `tokens { ... }` | Typed values shared by the document | [Tokens](yui-documents.md#tokens) |
| `component Name : Base { ... }` | A new component type | [Composite components](yui-documents.md#composite-components) |
| `Type { ... }` | The root component instance, at most one per document | [Complete UIs](yui-documents.md) |

Line comments (`// ...`) and block comments (`/* ... */`) can go anywhere
whitespace can. Names are case sensitive: component types start with an
uppercase letter (`Slider`, `LabeledKnob`), everything else is `camelCase`.

## Elements and properties

Every block other than `import` is an element with a body in braces. A body
holds properties, written `name: value;`, and child elements, written
`type { ... }`, in any order:

```yui
Column {
    gap: 4;                                                             // a property
    Slider { id: knob; value: 0.5; flexGrow: 1; }                       // a child element
    Label  { text: format ("%.2f", knob.value); preferredHeight: 20; }
}
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/ui/yui-language_elements_light.jpg#gh-light-mode-only" alt="A slider above a label showing its value" width="360">
  <img class="dark-only rounded" src="../_static/images/ui/yui-language_elements_dark.jpg#gh-dark-mode-only" alt="A slider above a label showing its value" width="360">
</p>

A property can be set once per element. Inside a `drawing` the children are
shapes and scopes (`circle`, `layer`, ...); in the instance tree and in
`component` blocks they are components, named by their registered type
(`Slider`, `Row`, ...) or by a `component` declared in the document.

Two properties are special in the instance tree: `id` names an element so
expressions and C++ (`findById`) can reach it, and `class` gives it style
classes, separated by spaces or commas (`class: "knob large";`).

Most components simply parent their children, but a few take them in their own
way: `Viewport` makes its child the scrolled content, and `TabComponent` turns
each child into a page, with the child's `id` as the tab id and its `title` as
the text of the tab:

```yui
TabComponent {
    selectedTab: "mix";
    Column   { id: mix;  title: "Mix"; }
    Viewport { id: list; title: "List"; Column { height: 900; } }
}
```

Lists have no children either, they are filled from their `items` property;
`Image` displays a raster or SVG file (`source`, `fitting`, `justification`,
`imageOpacity`), and `Window` can serve as the root element to carry window
metadata (`title`, `resizable`, `fullScreen`):

```yui
ComboBox { items: ["sine", "square"]; selectedId: 1; }
ListBox  { items: ["left", "right"]; selectedRow: 0; }
Image    { source: "logo.svg"; fitting: scaleToFit; }
```

An element's values apply in the order its type declares the properties, base
types first, and `<=>` links after them, whatever the order in the document. A
property that constrains another therefore applies first:
`Slider { value: 1000; minimum: 20; maximum: 20000; }` sets the range before the
value.

## Values

A value is an expression of the [expression language](../core/expressions.md),
with the [graphics types](../graphics/expressions.md) available:

| Kind | Examples |
|---|---|
| Numbers, with units | `4`, `0.5`, `135deg`, `250ms`, `50%` |
| Booleans and strings | `true`, `false`, `"CUTOFF"` |
| Colors | `#ff8800`, `#ff880080`, `rgb (255, 136, 0)`, `palette.accent.darker (0.1)` |
| Enumerations | `round`, `rotaryVerticalDrag`, `center` (a bare name the property knows) |
| Graphics values | `bounds.center`, `rect (0, 0, 20, 20)`, `polar (bounds.center, r, a)` |
| Arrays | `[0.25, 0.5]`, `["low", "high"]`, `lanes[0]`, `len (lanes)` |
| Operators and calls | `enabled ? 1 : 0.4`, `lerp (a0, a1, normalized)`, `min (width, height) / 2` |

An expression reads the names around it: the properties of the component it
is written on (bare or prefixed by `self.`), `parent.`, the elements with an
`id`, `root` inside a `component` block, the document models and tokens, and
`palette.` roles. In the instance tree, a value that reads anything is a
[binding](yui-reactivity.md), kept up to date while what it reads changes.

The values of `style` and `palette` blocks are kept as written up to their `;`,
because some aren't expressions (`transition: bezelSize 150ms easeOut;`,
`drawing: Knob;`). They are compiled when the stylesheet applies them.

## Declarations

`property` declares a typed value with an optional default expression. The
types are `number`, `integer`, `boolean`, `string`, `color`, `point`,
`rectangle`, `transform`, `font`, and an array of scalar types written `type[]`:

```yui
property bezelSize: number = 0.8;
property lanes: number[] = [0.25, 0.5];
property origin: point = point (1, 2);
```

An array holds one element type. Its elements are expressions, read with
`lanes[0]` and counted with `len (lanes)`, and an index outside the array
reads a zero value. `point` and `rectangle` values are written `point (x, y)`
and `rect (x, y, w, h)` and are stored as arrays of their components, so `<=>`
links and transitions see them change element by element.

| Where | Declares |
|---|---|
| At the top of a `drawing` | An input the component, its parents or a stylesheet set ([Properties](yui-drawing.md#properties)) |
| In `tokens` | A value of the document root, read by bare name everywhere |
| In a `component` | A property of the new type, read inside as `root.name` |
| In a style rule | A default for the components the rule matches |

A `component` block also declares:

- `property alias name <=> id.property;`, exposing a property of a part;
- `signal name (argument: type, ...);`, a signal its parts emit with
  `root.name (...)`.

## Scopes in drawings

`let name: value;` names a value for the elements after it, and
`repeat i in from .. to { ... }` draws its shapes once per integer from `from`
to `to - 1`. Both only exist inside drawings, see
[Scopes](yui-drawing.md#scopes).

Inside a drawing, `drawing Name { ... }`, `pattern Name { ... }` and
`marker Name { ... }` define a drawing, a tile and a marker for the elements
after them, and `use Name { ... }` paints a drawing by name, see
[Reusing drawings](yui-drawing.md#reusing-drawings) and
[Patterns and markers](yui-drawing.md#patterns-and-markers).

## Bindings and handlers

`name <=> id.property;` links a property both ways to another, or to a model
property (`value <=> params.cutoff;`), see [Two-way
bindings](yui-documents.md#two-way-bindings).

A property named after a signal is a handler: a call, either an action
registered by the application or a signal of the enclosing component, or a
block of calls. `(a, b) =>` names the signal's arguments, and
`changed property: handler;` runs a handler when a property changes:

```yui
TextButton { text: "Reset"; onClick: action ("reset"); }
Slider     { onValueChanged: (v) => root.onChanged (v); }
TextButton { onClick: { action ("save"); root.onDone (); } }
Slider     { changed value: (v) => action ("log", v); }
```

See [Handlers and actions](yui-documents.md#handlers-and-actions).

## Imports

`import "path";` inserts the blocks of another file where it is written. Nested
imports resolve relative to the directory of the file importing them, and each
resolved path is imported once, the first time it is met, so shared files can
be imported from several places and cycles are harmless. `YuiLoader` resolves
the normalized path from the folder of the main file, or by name among embedded
resources (see [Hot reloading](yui-hot-reload.md)), while
`YuiDocument::loadFromString` passes it to its import callback.

## Errors

Syntax errors stop parsing at the first one, reported with its line and column,
prefixed by the import path when it is in an imported file:

```text
parts/style.yui:12:18: Expected ';'
```

Everything else is checked when the document is used: unknown component types
make loading fail, unknown properties and values that don't compile are
listed by `YuiDocument::getDiagnostics()`, and drawing errors name the path of
the element, as in `Knob/layer[0]/arc#value.to: Unknown symbol 'avv'`. The parser
records where each element and property is written (`ParseResult::sourceMap`),
so these problems carry a line and column too, the one of the value, the
declaration or the element they are about:

```text
style.yui:7:9: Unknown cursor 'finger'
```

## The document tree

`YuiParser::parse` turns the text into a `DataTree` of type `yui`, with one
child per block, the element types and property names as written, and the
values as their expression text. This tree is what the rest of YUI consumes,
so tools can create or edit interfaces without writing text.

`YuiWriter::write()` turns a parsed document, or any element of it, back into
text that parses to the same tree, so tools can load, edit and save `.yui`
files. Comments and the original layout are not preserved.

## See also

- [Complete UIs in `.yui`](yui-documents.md) - loading documents, tokens,
  composite components, handlers and models.
- [Stylesheets](yui-styles.md) - selectors, the cascade, palettes and
  transitions.
- [Data-driven drawing](yui-drawing.md) - shapes, scopes and drawing
  properties.
- [Layout components](component-layout.md#layout-components) - `Row`,
  `Column`, `Grid`, `Stack` and the item properties.
- [Hot reloading `.yui` files](yui-hot-reload.md) - loading files and
  resources, and watching them while developing.
