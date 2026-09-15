# Component Drag and Drop

YUP delivers files, text and other MIME payloads from the operating system into the
application, and lets components accept them as drops. Drag-and-drop targets are an opt-in
mixin rather than a `Component` base: a component that wants drops derives from
`DragAndDropTarget` in addition to `Component`, so `Component` itself carries no
drag-and-drop surface. There is no drag source API for in-app drag operations — a drag is
always started by the operating system.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

The payload is carried by `DragAndDropData`. Drop targets implement the
`DragAndDropTarget` mixin, and the platform bridge (SDL) translates OS drag-and-drop events
into `DragAndDropTarget::dispatchItemDrop()` / `dispatchItemDragEnter()` /
`dispatchItemDragMove()` / `dispatchItemDragExit()` calls. No macOS or Windows
platform-specific implementation exists at this time — the inbound drag path is SDL-only.

The flow:

1. The user drags files or text from the OS into a YUP window.
2. The platform layer builds a `DragAndDropData` payload.
3. The dispatcher walks from the deepest component under the cursor up to the root,
   resolving each component to a `DragAndDropTarget` with a `dynamic_cast` — components
   that are not targets are skipped.
4. Interested targets receive `itemDragEnter` / `itemDragMove`, and either `itemDropped`
   (if released) or `itemDragExit` (if the drag leaves).

---

## `DragAndDropData` — Payload Class

An immutable value type representing the payload delivered during a drag-and-drop
operation, modelled as a set of MIME-typed blobs. Built using fluent `with*` methods.

### Construction

```cpp
DragAndDropData data; // empty
```

### Fluent builders (immutable — return a copy)

```cpp
auto data = DragAndDropData()
                .withFiles  (fileList)          // text/uri-list, file:// URIs
                .withText   ("hello world")     // text/plain;charset=utf-8
                .withUris   (uriList)           // text/uri-list
                .withImage  (image)             // image/png
                .withMimeData ("application/x-my-type", block)
                .withNativeObject (var (myObject)); // same-process only
```

Each `with*` method copies the current object, sets the entry, and returns the copy. The
original is never modified. Setting empty data removes the entry, so a payload entry is
present exactly when it is non-empty.

### Getters and inspection

```cpp
String      text  = data.getText();
Array<File> files = data.getFiles();
StringArray uris  = data.getUris();
Image       image = data.getImage();
MemoryBlock block = data.getMimeData ("application/x-my-type");
StringArray types = data.getMimeTypes();

bool hasText  = data.hasText();
bool hasFiles = data.hasFiles();
bool hasUris  = data.hasUris();
bool hasImage = data.hasImage();
bool empty    = data.isEmpty();
```

The getters return by value because they decode from the MIME store. An empty string `""`
does **not** count as having text — `hasText()` returns `false`.

`withNativeObject()` / `getNativeObject()` carry an arbitrary `var` alongside the MIME
store. This is a same-process, zero-copy escape hatch for handing a live C++ object to a
drop target; it is never transported across a process or application boundary, so a target
must treat it as empty for OS-originated drags.

---

## `DragAndDropTarget` — the opt-in mixin

A component opts in to receiving drops by deriving from `DragAndDropTarget` alongside
`Component`:

```cpp
class DroppableArea : public Component,
                      public DragAndDropTarget
{
    // ...
};
```

A target must also be a `Component` (`getTargetComponent()` returns it). Callbacks can be
overridden as virtual methods or assigned as `std::function` members
(`onIsInterestedInDragSource`, `onItemDropped`, `onItemDragEnter`, `onItemDragMove`,
`onItemDragExit`); the virtual runs first and the function afterwards, so both mechanisms
work and the boolean-returning pairs OR-combine.

### isInterestedInDragSource — opt-in gate

```cpp
virtual bool isInterestedInDragSource (const DragAndDropSourceDetails& details);
```

Defaults to `false`. A target **must** answer `true` to receive any other callback. Both
`isVisible()` and `isEnabled()` are checked before this is called — invisible or disabled
components are skipped entirely.

### itemDropped — handle the drop

```cpp
virtual bool itemDropped (const DragAndDropSourceDetails& details);
```

Return `true` to stop bubbling; return `false` to let the payload bubble up to parent
components.

### Drag-over tracking

```cpp
virtual void itemDragEnter (const DragAndDropSourceDetails& details);
virtual void itemDragMove  (const DragAndDropSourceDetails& details);
virtual void itemDragExit  (const DragAndDropSourceDetails& details);
```

`itemDragEnter` — the drag enters the target's area. `itemDragMove` — the drag moves within
it. `itemDragExit` — the drag leaves it (no meaningful position is provided).

All positions in `DragAndDropSourceDetails::localPosition` are in the target's local coordinates. For
enter/move, **all** interested ancestors in the parent chain are notified (bubbling does
not stop). For exit, all previously interested ancestors receive the call.

### `DragAndDropSourceDetails`

```cpp
struct DragAndDropSourceDetails
{
    DragAndDropData          data;              // the payload being dragged
    WeakReference<Component> sourceComponent;   // null for OS-originated drags
    Point<float>             localPosition;     // in the target's coordinates
    DragAndDropActions       allowedActions;    // copy | move | link
    DragAndDropAction        suggestedAction = DragAndDropAction::copy;
};
```

`sourceComponent` is a `WeakReference<Component>`: it reads as null both for an OS-originated
drag and once the source has been destroyed, so check it before use rather than assuming the
source outlives the drag.

`DragAndDropAction` is `none` / `copy` / `move` / `link`, and `DragAndDropActions` is a
`FlagSet` of those (see `dragAndDropActionCopy` etc.).

### Bubbling behaviour

`itemDropped`: bubbling stops when a target returns `true`. The deepest interested target
is tried first; if it returns `false`, its parent gets a chance, and so on up to the root.

`itemDragEnter` / `itemDragMove` / `itemDragExit`: **all** interested ancestors are
notified. Bubbling does not stop.

The static `DragAndDropTarget::dispatch*` entry points implement this walk and are intended
for the platform backends; application code normally only implements the callbacks.

### `DragAndDropTargetComponent`

Deriving from `Component` and `DragAndDropTarget` is enough for most call sites, but some
places need a single concrete type that is nameable on its own — factories, containers, and the
language bindings. `DragAndDropTargetComponent` (in the same header) is exactly that:

```cpp
class MyTarget : public DragAndDropTargetComponent
{
    // ...
};
```

---

## Python

The Python module exposes `yup.DragAndDropData`, `yup.DragAndDropAction` /
`yup.DragAndDropActions`, `yup.DragAndDropSourceDetails` and `yup.DragAndDropTargetComponent`.
A Python drop target subclasses the last of those and overrides `isInterestedInDragSource` /
`itemDropped` / `itemDragEnter` / `itemDragMove` / `itemDragExit`, or assigns the
`onIsInterestedInDragSource` / `onItemDropped` / `onItemDragEnter` / `onItemDragMove` /
`onItemDragExit` callables:

```python
class DropTarget(yup.DragAndDropTargetComponent):
    def isInterestedInDragSource(self, details):
        return details.data.hasFiles()

    def itemDropped(self, details):
        for file in details.data.getFiles():
            print("dropped", file.getFullPathName())
        return True
```

`details` is a borrowed view, valid only for the duration of the call — copy it with
`yup.DragAndDropSourceDetails(details)` if you need to keep it.

---

## Usage example

```cpp
class DroppableArea : public Component,
                      public DragAndDropTarget
{
public:
    bool isInterestedInDragSource (const DragAndDropSourceDetails& details) override
    {
        return details.data.hasFiles() || details.data.hasText();
    }

    bool itemDropped (const DragAndDropSourceDetails& details) override
    {
        if (details.data.hasFiles())
        {
            for (const auto& file : details.data.getFiles())
                Logger::writeToLog ("Dropped file: " + file.getFullPathName());
            return true;
        }
        if (details.data.hasText())
        {
            insertText (details.data.getText());
            return true;
        }
        return false;
    }

    void itemDragEnter (const DragAndDropSourceDetails&) override
    {
        highlight = true;
        repaint();
    }

    void itemDragMove (const DragAndDropSourceDetails& details) override
    {
        lastDragPosition = details.localPosition;
        repaint();
    }

    void itemDragExit (const DragAndDropSourceDetails&) override
    {
        highlight = false;
        repaint();
    }

private:
    bool highlight = false;
    Point<float> lastDragPosition;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DroppableArea)
};
```

---

## Related

- [Component basics](component-basics.md) — the parent/child tree and input
  handling
