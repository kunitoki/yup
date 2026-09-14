# Artboards (Rive)

YUP embeds [Rive](https://rive.app) artboards as ordinary components. Five
classes cooperate:

- `ArtboardFile` — a loaded `.riv` binary, and the factory for everything below.
- `Artboard` — the `Component` that fits, advances, draws and hits-tests an
  artboard from that file.
- `ArtboardNode` — a read-only handle to one named node inside the artboard.
- `ArtboardViewModel` — a data *schema* authored in the Rive editor.
- `ArtboardViewModelInstance` — a live set of values for that schema, bound to
  an artboard to drive its data bindings.

## Loading a file

`ArtboardFile::load` returns a `ResultValue`, so failures carry a message
instead of a null pointer. It needs a `rive::Factory`, which the graphics
context supplies:

```cpp
auto result = yup::ArtboardFile::load (yup::File ("dashboard.riv"), factory);

if (result.failed())
{
    YUP_DBG (result.getErrorMessage());
    return;
}

auto file = result.getValue();      // std::shared_ptr<ArtboardFile>
```

A file is always held through a shared pointer. Every handle created from it —
schemas, instances — keeps it alive, so the file outlives everything that reads
from it.

There are four overloads: from a `File` or an `InputStream`, each with and
without an asset callback.

### Resolving out-of-band assets

Images, fonts and audio can live outside the `.riv`. Pass an
`AssetLoadCallback` to supply their bytes:

```cpp
auto result = yup::ArtboardFile::load (
    riveFile,
    factory,
    [&] (const yup::ArtboardFile::AssetInfo& info,
         yup::Span<const yup::uint8> inBandBytes,
         rive::Factory& assetFactory)
    {
        // info.uniqueFilename is the authored name decorated with the asset id
        // ("logo-1234.png"). It is a bare file name to look for, not a path, and
        // nothing on disk is guaranteed to match it.
        auto onDisk = assetDirectory.getChildFile (info.uniqueFilename);
        if (! onDisk.existsAsFile())
            return false;   // fall back to the in-band bytes, if any

        // ... decode with assetFactory and hand the result to the asset ...
        return true;
    });
```

Returning `false` is not an import failure — a file whose assets all go
unresolved still loads.

## Displaying an artboard

`Artboard` is a `Component`. Give it a file and add it to a parent:

```cpp
auto artboard = std::make_unique<yup::Artboard> ("dashboard", file);
artboard->setFitting (yup::Fitting::scaleToFit);
artboard->setJustification (yup::Justification::center);
addAndMakeVisible (*artboard);
```

`setFile` takes an optional artboard name; with none, the file's default
artboard is loaded. Passing `nullptr` unloads:

```cpp
artboard->setFile (file, "Mobile");   // a specific artboard
artboard->setFile (nullptr);          // unload
```

### Fitting and justification

`setFitting` decides how the artboard's own size is fitted into the component's
bounds, and `setJustification` where the result sits when the fit leaves spare
room. Both take the shared `yup::Fitting` and `yup::Justification` types, so the
same vocabulary applies here as everywhere else in YUP:

| `std::optional<Fitting>` | Effect |
|---|---|
| `Fitting::scaleToFit` | Uniform scale to fit inside the bounds (the default). |
| `Fitting::scaleToFill` | Uniform scale to cover the bounds, cropping the overflow. |
| `Fitting::fitWidth` / `fitHeight` | Uniform scale to match one axis. |
| `Fitting::centerInside` | Like `scaleToFit`, but never scales above 1.0. |
| `Fitting::fill` | Stretch on both axes; aspect ratio is not preserved. |
| `Fitting::none` | No scaling. |
| `std::nullopt` | Hand the component's size to the artboard and let its own Rive layout constraints resolve it. |

Rive has no equivalent for `Fitting::tile`, `centerCrop`, `stretchWidth` and
`stretchHeight`; the artboard accepts them and falls back to `scaleToFit`.

Justification has no visible effect for `Fitting::fill` or for `std::nullopt`,
which always consume the full bounds. It is a bitfield, so an axis with no flag
set is centered on that axis — `Justification::left` alone means left-and-vertically-centered.

Use `std::nullopt` for artboards authored with Rive's layout engine — that is
what makes nodes reflow when the component resizes:

```cpp
artboard->setFitting (std::nullopt);   // the artboard lays itself out
```

### Advancing and pausing

`Artboard` advances itself from `Component::refreshDisplay` each frame. Call
`advanceAndApply` directly only when driving it manually (in tests, or for a
fixed-step render).

```cpp
artboard->setPaused (true);              // stop advancing
artboard->shouldPauseWhenHidden (false); // keep advancing while off-screen
artboard->durationSeconds();             // the scene's length
```

## State machine inputs and events

When the artboard is driven by a state machine, its inputs are reachable by
name:

```cpp
artboard->setBoolInput ("hovered", true);
artboard->setNumberInput ("progress", 0.42);
artboard->triggerInput ("pulse");
```

`getAllInputs()` snapshots every input as an `Array<var>` of `DynamicObject`s
with `id`, `type` (`"number"`, `"boolean"` or `"trigger"`) and, for the two
stateful types, `value`. `setAllInputs()` applies such a snapshot back, matching
by `id` and ignoring unknown entries — so a snapshot can be saved and restored,
or moved between artboards:

```cpp
auto snapshot = artboard->getAllInputs();
// ... later ...
artboard->setAllInputs (snapshot);
```

Rive state machines also report *events* carrying custom properties. Observe
them with `onPropertyChanged`, or by overriding `propertyChanged`:

```cpp
artboard->onPropertyChanged = [] (yup::Artboard&,
                                  const yup::String& eventName,
                                  const yup::String& propertyName,
                                  const yup::var& oldValue,
                                  const yup::var& newValue)
{
    YUP_DBG (eventName << "." << propertyName << " = " << newValue.toString());
};
```

The queue is drained after every advance and after every pointer interaction.
Only genuine changes are reported: the artboard remembers the last value seen
per event and skips repeats.

## Nodes

`findNode` resolves a named node from the `.riv` and returns a refcounted,
read-only handle:

```cpp
if (auto node = artboard->findNode ("knob-panel"))
{
    node->getBounds();          // in component coordinates
    node->getViewTransform();   // world transform through the artboard fit
    node->getTypeName();        // "Shape", "LayoutComponent", ...

    for (auto child : node->getChildren())
        YUP_DBG (child->getName());
}
```

Handles are invalidated by `clear()`, `setFile()` and by destroying the
artboard. After that `isValid()` returns false and every accessor returns a safe
default (empty string, empty rectangle, identity transform, null pointer) — a
stale handle never dereferences a freed Rive object.

`getTypeName()` is empty both for an invalid handle and for a node whose type is
outside the mapped set; use `getTypeKey()` to tell the two apart.

### Reacting to node movement

Register a listener to be told when a node's bounds or on-screen orientation
change — on reflows, resizes and animation frames:

```cpp
artboard->setNodeBoundsListener ("knob-panel",
                                 [] (yup::Artboard&,
                                     const yup::String& name,
                                     const yup::ArtboardNode::Ptr& node)
                                 {
                                     YUP_DBG (name << " -> " << node->getBounds().toString());
                                 });

artboard->clearNodeBoundsListener ("knob-panel");
```

The handle passed in is the artboard's cached one, so no allocation happens per
event.

### Attaching components to nodes

Rather than repositioning a component by hand, let a node drive it. This is how
you overlay real YUP widgets on a Rive-authored layout:

```cpp
artboard->attachComponentToNode ("knob-panel", &knob);

// Or keep the component's own size and just follow the node's centre:
artboard->attachComponentToNode (
    "needle",
    &readout,
    yup::Artboard::NodeAttachmentOptions()
        .withMode (yup::Artboard::NodeAttachmentOptions::Mode::trackPosition)
        .withApplyTransform (true)
        .withJustificationPivot (yup::Justification::center)
        .withJustificationAnchor (yup::Justification::center));
```

`fillNode` (the default) sets the component's bounds to the node's.
`trackPosition` keeps the component's size and pins its `pivot` point to the
node's `anchor` point. `withApplyTransform (true)` additionally rotates the
component around that pivot to follow the node.

Ownership stays with the caller, and the artboard watches for the component's
destruction, so deleting an attached component without detaching it first is
safe. A component follows exactly one node: attaching it again moves it.

In `trackPosition` mode the component keeps its own size, and that size is what
the `pivot` is measured against — so resizing it re-derives its position
immediately, and you can set the size before or after attaching:

```cpp
readout.setSize (120.0f, 80.0f);   // before or after attaching, same result
```

In `fillNode` mode the node owns the size, so resizing the component yourself is
simply overwritten.

## Data binding with ViewModels

A Rive artboard can be designed against a *ViewModel* — a named set of typed
properties. Ask the artboard which schema it wants, create an instance, and bind
it:

```cpp
const auto schemaName = artboard->getViewModelName();

auto instance = file->createArtboardViewModelInstance (schemaName);
artboard->bindViewModelInstance (instance);
```

`.riv` files may also ship pre-authored instances; clone one by name to start
from the authored values rather than from zero:

```cpp
auto instance = file->createArtboardViewModelInstance ("Main", "Main");
```

The instance must come from the same `ArtboardFile` as the artboard; binding one
from another file fails.

### Reading and writing properties

Values are addressed by name, or by a dotted path into nested viewmodels and
lists:

```cpp
instance->setNumberProperty ("score", 120.0);
instance->setStringProperty ("player.name", "Ada");
instance->setColorProperty ("theme.accent", yup::Colors::hotpink);
instance->setEnumProperty ("state", "expanded");
instance->trigger ("celebrate");

auto score = instance->getNumberProperty ("score");        // std::optional<double>
auto name  = instance->getStringProperty ("player.name");  // std::optional<String>
```

There is also an untyped pair, `getProperty` / `setProperty`, working in `var`s:
booleans map to `var(bool)`, numbers to `var(double)`, strings to `var(String)`,
colors to `var(int64)` holding ARGB, and enums to `var(String)` of the selected
option.

Writes are applied to the artboard's data bindings on the next
`advanceAndApply()`.

Unknown paths and type mismatches both yield `nullopt` (or an empty `var`). Use
`hasProperty` when you need to tell them apart.

### Lists

List properties hold viewmodel instances and can be resized at runtime:

```cpp
instance->getListSize ("items");                    // -1 if not a list
instance->addListItem ("items", "ItemViewModel");   // append
instance->addListItemAt ("items", 0, "ItemViewModel");
instance->swapListItems ("items", 0, 1);
instance->removeListItem ("items", 0);
instance->clearListItems ("items");

auto first = instance->getListItem ("items", 0);
```

Items are also reachable through paths. `"items.2"` names the item's instance
and resolves through `hasProperty` and `getNestedInstance`; `"items.2.quantity"`
names a value inside it and works with every accessor:

```cpp
instance->getNumberProperty ("items.2.quantity");
instance->getNestedInstance ("items.2");
```

Because a path segment of digits is always read as an index, a property named
literally `"2"` cannot be addressed through a path.

### Observing changes

One callback covers the whole instance graph — nested viewmodels and list items
included:

```cpp
instance->setPropertyChangedCallback (
    [] (yup::ArtboardViewModelInstance&,
        const yup::String& path,
        const yup::var& value)
    {
        YUP_DBG (path << " = " << value.toString());
    });
```

It fires for writes made through the handle *and* for values the artboard writes
back through its output bindings. Structural list changes notify too, reporting
the list's own path and an empty `var`.

Mutating the instance from inside the callback is supported, as is replacing or
clearing the callback itself. The one rule: do not release the instance's last
reference from within it.

### Inspecting the schema

`ArtboardViewModel` describes a schema without instantiating it — useful for
building generic inspectors:

```cpp
auto schema = file->getArtboardViewModel ("Main");

for (int i = 0; i < schema->getNumProperties(); ++i)
{
    const auto property = schema->getPropertyAt (i);

    YUP_DBG (property.name << " : " << (int) property.type);

    if (property.type == yup::ArtboardViewModel::PropertyType::enumType)
        YUP_DBG (property.enumValues.joinIntoString (", "));
}
```

`isInput` and `isOutput` say which direction a property is bound in.
`getInstanceNames()` lists the authored instances available to clone.

## Threading

Artboards and their handles are not internally synchronized.

`Artboard` advances from `refreshDisplay`, which YUP runs on the render thread
while holding the message manager lock — so it never runs concurrently with the
message thread, but everything the advance reaches runs *on that thread*: node
bounds listeners, bounds updates pushed into attached components,
`onPropertyChanged`, and the `PropertyChangedCallback` of a bound instance. A
slow callback stalls rendering; move real work onto the message thread or a
background thread.
