# Component Drag and Drop

YUP supports external-only drag-and-drop: the operating system delivers files or
text into the application, and `Component` subclasses handle those payloads.
There is no drag source API for in-app drag operations.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

Drag-and-drop is implemented through five virtual methods on `Component` and the
`DragAndDropData` payload class. The platform bridge (SDL) translates OS
drag-and-drop events into component dispatch calls. No macOS or Windows
platform-specific implementation exists at this time — drag-and-drop is
SDL-only.

The flow:

1. The user drags files or text from the OS into a YUP window.
2. The platform layer builds a `DragAndDropData` payload.
3. The component tree walks from the deepest child under the cursor up to the
   root, calling `isInterestedInDrag()` on each.
4. Interested components receive `itemDragEnter`, `itemDragMove`, and either
   `itemsDropped` (if released) or `itemDragExit` (if the drag leaves).

---

## `DragAndDropData` — Payload Class

An immutable value type representing the payload delivered during a drag-and-drop
operation. Built using fluent `with*` methods.

### Construction

```cpp
DragAndDropData data; // empty (no files, no text, no URIs)
```

### Fluent builders (immutable — return a copy)

```cpp
auto data = DragAndDropData()
                .withFiles (fileList)
                .withText ("hello world")
                .withUris  (uriList);
```

Each `with*` method copies the current object, sets the specified field, and
returns the copy. The original is never modified.

### Getters

```cpp
const Array<File>& files = data.getFiles();
const String&      text  = data.getText();
const StringArray& uris  = data.getUris();
```

### Inspection

```cpp
bool hasFiles   = data.hasFiles();   // files array is non-empty
bool hasText    = data.hasText();    // text string is non-empty
bool hasUris    = data.hasUris();    // URIs array is non-empty
bool empty      = data.isEmpty();    // none of the above
```

An empty string `""` does **not** count as having text — `hasText()` returns `false`.

---

## Component virtual methods

### isInterestedInDrag — opt-in gate

```cpp
virtual bool isInterestedInDrag (const DragAndDropData& data);
```

Defaults to `false`. A component **must** override this and return `true` to
receive any drag-and-drop callbacks. Both `isVisible()` and `isEnabled()` are
checked before this is called — invisible or disabled components are skipped
entirely.

### itemsDropped — handle the drop

```cpp
virtual bool itemsDropped (const Point<float>& position,
                           const DragAndDropData& data);
```

Called when the user releases the drag payload over this component. `position`
is in component-local coordinates. Return `true` to stop bubbling; return
`false` to let the payload bubble up to parent components.

### Drag-over tracking

```cpp
virtual void itemDragEnter (const DragAndDropData& data,
                            const Point<float>& position);

virtual void itemDragMove (const DragAndDropData& data,
                           const Point<float>& position);

virtual void itemDragExit (const DragAndDropData& data);
```

- `itemDragEnter` — drag enters the component's area.
- `itemDragMove` — drag moves within the component's area.
- `itemDragExit` — drag leaves the component's area. No position is provided.

All positions are in component-local coordinates. For enter/move, all interested
ancestors in the parent chain are notified (bubbling does not stop). For exit,
all previously interested ancestors receive the call.

### Bubbling behavior

`itemsDropped`: bubbling stops when a component returns `true`. The deepest
interested component is tried first; if it returns `false`, its parent gets a
chance, and so on up to the root.

`itemDragEnter`/`itemDragMove`: **all** interested ancestors are notified.
Bubbling does not stop.

---

## Usage example

```cpp
class DroppableArea : public Component
{
public:
    bool isInterestedInDrag (const DragAndDropData& data) override
    {
        return data.hasFiles() || data.hasText();
    }

    bool itemsDropped (const Point<float>& position,
                       const DragAndDropData& data) override
    {
        if (data.hasFiles())
        {
            for (auto& file : data.getFiles())
                Logger::writeToLog ("Dropped file: " + file.getFullPathName());
            return true;
        }
        if (data.hasText())
        {
            insertText (data.getText());
            return true;
        }
        return false;
    }

    void itemDragEnter (const DragAndDropData& data,
                        const Point<float>& position) override
    {
        highlight = true;
        repaint();
    }

    void itemDragMove (const DragAndDropData& data,
                       const Point<float>& position) override
    {
        lastDragPosition = position;
        repaint();
    }

    void itemDragExit (const DragAndDropData& data) override
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
