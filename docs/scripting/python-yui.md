# `.yui` interfaces from Python

The classes that load and drive `.yui` documents are bound more or less
one-to-one with the C++ API described under
[Complete UIs in `.yui`](../ui/yui/documents.md), so those pages remain the
reference for what each call means. This page covers only what is different in
Python. The `Yui*` classes are bound when `yup_gui_markup` is part of the build,
as it is in the wheel.

```python
import yup

document = yup.YuiDocument()
document.setAction("reset", lambda arguments: print("reset", arguments))

result = document.loadFromString("""
    Column {
        Slider { id: cutoff; value: 0.4; }
        TextButton { text: "Reset"; onClick: action ("reset", cutoff.value); }
    }""")

if result.failed():
    print(yup.DataTreeDiagnostic.join(document.getDiagnostics()))

cutoff = document.findById("cutoff")      # a yup.Slider
cutoff.setProperty("value", 0.8)
```

A document needs a running application, as any component does: the theme its
widgets read only exists while one is initialised.

## Components belong to the document

`getRoot()` and `findById()` return the components the document created, as
their Python class (`findById("cutoff")` above is a `yup.Slider`), or `None`.
The document owns them: Python only borrows them, and a borrowed component
keeps its document alive. A reload that no longer declares a component
destroys it, so look components up again after loading rather than keeping
them across loads.

To show the interface, add the root to a component of yours, or use a
`yup.YuiView`, which owns a document and a loader and shows the root itself:

```python
view = yup.YuiView()
view.getDocument().setModel("params", provider)
view.getLoader().loadFile(yup.File("ui/main.yui"))
parent.addAndMakeVisible(view)
```

## Functions where C++ takes callbacks

| Call | The Python function |
|---|---|
| `document.setAction (name, action)` | Takes the list of the values of the call. `None` removes the action. |
| `document.loadFromString (text, resolveImport)`, `YuiParser.parse (text, resolveImport)` | Takes the path of an import and returns its text, or `None` when it can't be found. |
| `loader.loadResource (path, findResource)` | Takes the path of a file and returns its `bytes`, or `None`. |
| `loader.onLoad` | Takes the `Result` of each load, the reloads started by watching included. |

## Models

A model is a `yup.BindingProvider`. `yup.DataTreeBindingProvider` exposes the
properties of a `DataTree`, as in C++:

```python
params = yup.DataTree(yup.Identifier("params"))
transaction = params.beginTransaction()
transaction.setProperty(yup.Identifier("cutoff"), 0.65)
transaction.commit()

provider = yup.DataTreeBindingProvider(params)
document.setModel("params", provider)     # before loading
```

A Python class can be a model too. It overrides `getProperties`, `getValue` and
`setValue`, and reports every change with `notifyValueChanged`. `findProperty`,
`beginGesture` and `endGesture` are optional:

```python
class Settings(yup.BindingProvider):
    def __init__(self):
        super().__init__()
        self.gain = 0.5

    def getProperties(self):
        return [yup.BindingProvider.Property("gain", yup.ComponentProperty.Type.number)]

    def getValue(self, name):
        return self.gain

    def setValue(self, name, value):
        self.gain = value
        self.notifyValueChanged(name)
```

The document keeps the provider given to `setModel` alive, and the undo manager
given to `setUndoManager`. A `YuiLoader` keeps its document alive.

A change made to `document.getTree()` is applied from the message loop. A
script that runs none applies it with `document.applyPendingChanges()`.

## Failures

- `loadFromString`, `load` and the loader return a `Result`, as in C++. The
  problems are in `document.getDiagnostics()`, each with its `message`, `line`,
  `column`, `source` and `severity`.
- `yup.YuiParser.parseStyleSheet (text)` returns the stylesheet and raises a
  `ValueError` carrying the first problem, where C++ returns a `ResultValue`.
- `yup.YuiParser.parse (text)` returns a `ParseResult` with its `document`,
  `diagnostics` and `wasOk()`.

## Reflection

`component.getTypeInfo()` returns the `yup.ComponentTypeInfo` of a component,
with its `ComponentProperty` and `ComponentSignal` descriptors, and
`yup.ComponentTypeRegistry.getInstance()` finds the types a `.yui` file can
name, once `yup.registerBuiltInComponentTypes()` was called:

```python
info = yup.Slider().getTypeInfo()
print(info.getTypeName(), [str(p.name) for p in info.getAllProperties()])
```

Style classes and stylesheets work on any component, whether it comes from a
document or not: `addStyleClass`, `removeStyleClass`, `hasStyleClass`,
`setStyleClasses`, `getStyleClasses`, `setStyleSheet` and `getStyleSheet`.

## Not bound

- `YuiDocument.createSchema`, the document listeners, the source map of a
  parsed text and `YuiBuilder`.
- `ComponentTypeRegistry.registerType`, and what a descriptor calls into C++
  with: the getter and setter of a `ComponentProperty` and the `connect` of a
  `ComponentSignal`. Use `getProperty`, `setProperty` and the callbacks of the
  widgets instead.
- The `SVGDocument` overload of `YuiSvgImporter.importSvg`. The ones taking the
  text of an SVG or a `yup.File` are.

## See also

- [Complete UIs in `.yui`](../ui/yui/documents.md) - documents, actions and
  models.
- [Hot reloading `.yui` files](../ui/yui/hot-reload.md) - `YuiLoader`.
- [Tools for `.yui` files](../ui/yui/tooling.md) - checking and previewing
  files from the command line.
