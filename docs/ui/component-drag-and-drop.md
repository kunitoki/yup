# Component Drag and Drop

YUP carries drag-and-drop in both directions. Inbound, it delivers files, text and other
MIME payloads from the operating system into the application. Outbound, application code
starts a drag carrying a payload, which follows the cursor as a ghost window and can be
dropped on any target — in the same window, in another window of the same process, or, when a
source asks for it and the platform supports it, in another application.

Both ends are opt-in mixins rather than part of `Component`: a component that accepts drops
derives from `DragAndDropTarget`, one that starts drags derives from `DragAndDropSource`, and
`Component` itself keeps no drag-and-drop surface.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

The payload is always a `DragAndDropData`. A **target** implements `DragAndDropTarget`; a
**source** implements `DragAndDropSource`. `DragAndDropManager` — one app-global instance —
owns a drag while it is in flight: the ghost window, the component currently under the
cursor, and the enter/move/exit bookkeeping.

An in-app drag:

1. The source calls `startDragging()` with a payload and an optional drag image.
2. The manager shows the ghost, takes the global mouse, and tracks the component under the
   cursor across every native window (`Desktop::findComponentAt`).
3. The dispatcher walks from the deepest component under the cursor up to the root,
   resolving each component to a `DragAndDropTarget` with a `dynamic_cast` — components
   that are not targets are skipped.
4. Interested targets receive `itemDragEnter` / `itemDragMove`, and either `itemDropped`
   (if released) or `itemDragExit` (if the drag is cancelled or leaves).
5. The manager reports the performed action back to the source through
   `dragOperationEnded`.

An OS-originated drag arrives through the same machinery: the platform backend resolves the
component under the cursor and hands it to the manager, so targets see identical callbacks
whichever direction the drag came from. No macOS or Windows platform-specific implementation
exists for inbound drags at this time — that path is SDL-only.

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

```{note}
For a drag that originated outside the application, the operating system does not report
what is being dragged until it is actually dropped, so `details.data` stays **empty** for
the whole time the drag hovers. A target that wants to react before that — to highlight
itself, say — has to accept an empty payload here.
```

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

## `DragAndDropSource` — starting a drag

A component that starts drags derives from `DragAndDropSource` alongside `Component`, and
calls `startDragging()` from its own `mouseDrag` once the gesture has moved far enough to
count as a drag rather than a click:

```cpp
class DraggableTile : public Component,
                      public DragAndDropSource
{
    void mouseDrag (const MouseEvent& event) override
    {
        if (isCurrentlyDragging())
            return;

        const auto delta = event.getPosition() - event.getLastMouseDownPosition();

        if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f) // 8px
            return;

        startDragging (DragOptions{}.withData (DragAndDropData{}.withText (name)));
    }
};
```

### `DragOptions`

```cpp
struct DragOptions
{
    DragAndDropData data;                  // required: an empty payload cannot start a drag
    Component*      dragImageComponent;    // optional live ghost; the caller keeps ownership
    Image           dragImage;             // optional static ghost
    Point<float>    imageOffset;           // the point of the ghost under the cursor
    float           imageOpacity = 0.7f;   // applied to the ghost window
    DragAndDropActions allowedActions = copy | move | link;
    bool            allowExternalDrag = false; // not honoured yet, see Limitations
};
```

The builders return a reference, so the fluent form composes:

```cpp
startDragging (DragOptions{}
                   .withData (DragAndDropData{}.withText ("hello"))
                   .withDragImageComponent (&preview, Point<float> (10.0f, 10.0f))
                   .withImageOpacity (0.8f));
```

`startDragging()` returns `false` if a drag is already in flight or if the payload is empty.

### Callbacks

```cpp
virtual void dragOperationStarted (const DragAndDropData& data);
virtual void dragOperationEnded   (const DragAndDropData& data, DragAndDropAction performed);
```

`performed` is `DragAndDropAction::none` when nothing accepted the drop. As with targets,
both can be overridden or assigned (`onDragStarted` / `onDragEnded`), so both mechanisms
work. A live drag image handed to `startDragging` is safe to destroy in
`dragOperationEnded`: the ghost window has already given it up by then.

### The ghost window

The drag image is shown in a borderless, always-on-top, transparent and non-focusable
window that follows the cursor. It is the whole of that window, so a component used as a
live ghost is expected to size itself. Transparency depends on the platform compositor:
macOS, Windows and X11/Wayland are handled, but X11 additionally needs a compositing
manager running.

### Cancelling

Escape cancels a drag in progress. The interaction stops immediately — the ghost disappears
and no drop happens when the button is released — but the source is told only when the
button comes up, because the session has to stay alive until then: the source is still
inside its own mouse gesture and would otherwise start the same drag again on the next mouse
move. `DragAndDropManager::getInstance()->cancelDrag()` does the same programmatically.

### Which action is performed

The performed action is reported as `copy`, or as `move` when Shift is held down at the drop
and the source offered `move`. Only that pair is negotiated today.

---

## `DragAndDropManager`

One app-global instance owns the drag in flight. Application code rarely touches it: it is
reachable as a singleton for cancelling a drag or querying the current one
(`isDragging()`, `getCurrentDragData()`, `getCurrentDragSourceComponent()`,
`getCurrentDragTarget()`).

A drag has to outlive any single component hierarchy, because the pointer can cross into
another window and can leave every YUP window entirely. So the session cannot hang off a
parent component: the manager listens for global mouse events for the duration of the drag,
and resolves the component under the cursor across all native windows.

The `handleExternalDragPosition()` / `handleExternalDrop()` / `handleExternalDragExit()`
entry points are what the platform backend calls for an OS-originated drag. They are
`@internal`, but they are how the inbound path joins the same target-resolution and
enter/move/exit bookkeeping as an in-app drag.

```{note}
`Desktop::findComponentAt()` resolves the deepest component containing a point, and among
several candidates it prefers the focused one. It does not do a full z-order walk, so where
windows or components overlap, the result may not be the visually topmost one.
```

---

## `ListBox` — a ready-made source

`ListBox` is already a `DragAndDropSource`. Dragging a row asks the model for a description
and, when that returns anything other than a default-constructed `var`, starts a drag
carrying it:

```cpp
class MyModel : public ListBoxModel
{
    var getDragSourceDescription (const Array<int>& selectedRows) override
    {
        return selectedRows.isEmpty() ? var() : String ("row ") + String (selectedRows[0]);
    }
};
```

A string description is mirrored into the `text` MIME type as well, so a target that reads
only MIME data still sees it, and the whole `var` is available to same-process targets
through `getNativeObject()`.

The drag image comes from `createDragSourceComponent()`, which can be overridden; the
default is a circle carrying the number of dragged rows:

```cpp
virtual std::unique_ptr<Component> createDragSourceComponent (const Array<int>& selectedRows);
```

Its selection is what a drag carries, so the click semantics matter:

| click | effect |
| --- | --- |
| plain | replaces the selection |
| shift | extends a range from the last plain click, holding that anchor across further shift-clicks |
| command / control | toggles the row |

Pressing a row that is already part of a multiple selection does not collapse the selection
until the mouse is released, and not at all if a drag begins — which is what lets a drag
started on one of several selected rows carry all of them.

`setDragSourceEnabled (false)` makes a list undraggable without consulting its model at all;
it is enabled by default.

---

## Python

The Python module exposes `yup.DragAndDropData`, `yup.DragAndDropAction` /
`yup.DragAndDropActions`, `yup.DragAndDropSourceDetails`, `yup.DragAndDropTargetComponent`,
and — for the source side — `yup.DragAndDropSource` with `yup.DragOptions`.

A Python drop target subclasses `DragAndDropTargetComponent` and overrides
`isInterestedInDragSource` / `itemDropped` / `itemDragEnter` / `itemDragMove` /
`itemDragExit`, or assigns the matching `on*` callables:

```python
class DropTarget(yup.DragAndDropTargetComponent):
    def isInterestedInDragSource(self, details):
        return details.data.hasFiles()

    def itemDropped(self, details):
        for file in details.data.getFiles():
            print("dropped", file.getFullPathName())
        return True
```

A source derives from `Component` and `yup.DragAndDropSource`:

```python
class DragSource(yup.Component, yup.DragAndDropSource):
    def mouseDrag(self, event):
        opts = yup.DragOptions().withData(yup.DragAndDropData().withText("hello"))
        self.startDragging(opts)

    def dragOperationEnded(self, data, performed):
        print("ended", data.getText(), performed)
```

`details` is a borrowed view, valid only for the duration of the call — copy it with
`yup.DragAndDropSourceDetails(details)` if you need to keep it.

---

## Usage example

A target that accepts files and text, and a source that drags its own name:

```cpp
class Draggable : public Component,
                  public DragAndDropSource
{
public:
    void mouseDrag (const MouseEvent& event) override
    {
        if (isCurrentlyDragging())
            return;

        const auto delta = event.getPosition() - event.getLastMouseDownPosition();

        if (delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f)
            return;

        startDragging (DragOptions{}.withData (DragAndDropData{}.withText (getName())));
    }
};

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

## Limitations

- **The native export is macOS-only so far.** `DragOptions::allowExternalDrag` opts a source in,
  and on macOS the gesture is handed to AppKit, which carries files, text and PNG images to another
  application. Windows and X11 keep such a gesture in the app until each has an implementation.
- **A source cannot be both in-app and external.** The manager decides the gesture has left the
  application when it finds no component of ours under the pointer — and crossing between two windows
  looks exactly like that. So asking for the export on something that is also dragged between windows
  would break the cross-window case; keep the two roles on separate sources.
- **`DragAndDropData`'s `var` native object is same-process only** and is never exported.
- **An OS drag reports no payload until it is dropped**, so targets cannot inspect what is
  being dragged while it hovers. See the note under `isInterestedInDragSource`.
- **`Desktop::findComponentAt()` does not do a full z-order walk**, so overlapping windows or
  components may resolve to a component that is not visually topmost.
- **The ghost's transparency needs the platform.** It is handled for macOS, Windows and
  X11/Wayland, but X11 needs a running compositing manager for the window to composite at all.
- **No lazy data providers.** Every MIME blob in a payload is an eagerly-owned `MemoryBlock`.

---

## Related

- [Component basics](component-basics.md) — the parent/child tree and input
  handling
