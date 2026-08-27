# Component

{{ pagedepth: 2 }}

The `Component` class is the foundation of all GUI elements in YUP. Every
visible or interactive element — a button, a slider, a window, or a custom
drawing surface — derives from `Component`.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Overview

A `Component` is a rectangular region with:

- **Position and size** relative to its parent.
- **Appearance** controlled by overriding `paint()` and `paintOverChildren()`.
- **A tree of children** — each child is also a `Component`, forming a
  hierarchy. A component with no children is a leaf.
- **Enablement and visibility** — can be hidden, disabled, or transparent.
- **Input handling** — mouse and keyboard events.
- **Optional GPU effects** — post-processing via `ComponentEffect`.
- **Optional caching** — rasterise to a GPU texture via `setCachedToTexture()`.

Components form a tree. The root component lives on a platform-native window
(created with `addToDesktop()`). Children are positioned relative to their
parent, and the entire hierarchy paints from root to leaves every frame.

---

## Creating components

### Constructor

```cpp
class MyComponent : public Component
{
public:
    MyComponent()
    {
        // ... setup ...
    }
};
```

Optionally pass an ID string to help identify the component at runtime:

```cpp
class MyButton : public Component
{
public:
    MyButton()
        : Component ("MyButton")
    {
    }
};
```

```{tip}
The component ID is optional but useful for debugging and theme lookups. There
is no built-in `findChildWithID()` — to locate children by ID, iterate
`getNumChildComponents()` / `getChildComponent()` and compare `getComponentID()`.
```

### Destructor

The destructor automatically removes this component from its parent (if any) and
deletes all child components. Override `virtual ~Component()` only if you need
custom cleanup.

### Leak detection

YUP components include a leak detector. Use the standard macro:

```cpp
class MyComponent : public Component
{
public:
    // ...

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyComponent)
};
```

---

## Parent/child tree

Components form a tree. There is no separate "layout manager" — you build the
hierarchy by calling `addChildComponent()` or `addAndMakeVisible()`.

### Adding children

```cpp
// Add a child (invisible by default — call setVisible(true) to show it).
parent.addChildComponent (child);

// Add a child and make it visible immediately.
parent.addAndMakeVisible (child);

// Insert at a specific z-order index.
parent.addAndMakeVisible (child, 0); // 0 = behind all other children
```

```{note}
`addChildComponent` adds the child in a *hidden* state. `addAndMakeVisible` is
equivalent to `addChildComponent` followed by `child.setVisible(true)`. Use
`addAndMakeVisible` for children you always want to show, and
`addChildComponent` when you need to control visibility later (e.g. a popup
overlay).
```

### Removing children

```cpp
parent.removeChildComponent (child);  // by reference
parent.removeChildComponent (&child); // by pointer
parent.removeChildComponent (0);      // by index
parent.removeAllChildren();           // removes all
```

### Querying the tree

```cpp
int num = parent.getNumChildComponents();
Component* firstChild = parent.getChildComponent (0);
int index = parent.getIndexOfChildComponent (&child); // -1 if not found

Component* topLevel = comp.getTopLevelComponent();
Component* parent = comp.getParentComponent();
bool hasParent = comp.hasParent();
```

### Walking up to typed parents

```cpp
auto* window = comp.getParentComponentWithType<MyWindow>();
```

### Hit-testing

```cpp
const Point<float> clickPoint { 100, 200 };
Component* hit = parent.findComponentAt (clickPoint);
```

Return value is the deepest visible child under the point, or `nullptr`.

### Lifecycle callbacks

Override these to react to tree changes:

```cpp
void parentHierarchyChanged() override; // added to or removed from a parent
void childrenChanged() override;        // child added or removed
```

---

## Visibility and enablement

| Method | Description |
|---|---|
| `setVisible(bool)` | Show or hide the component. |
| `isVisible()` | Returns `true` if the component is marked visible (regardless of parents). |
| `isShowing()` | Returns `true` if the component **and all ancestors** are visible. |
| `setEnabled(bool)` | Enable or disable user interaction. Disabled components can paint but do not receive mouse events. |
| `isEnabled()` | Returns `true` if enabled. |

Lifecycle overrides:

```cpp
virtual void visibilityChanged();
virtual void enablementChanged();
```

```{tip}
When creating a component that should start invisible (e.g. a popup menu), use
`addChildComponent()` rather than calling `addAndMakeVisible` then `setVisible(false)`.
This avoids an unnecessary repaint and a spurious `visibilityChanged` call.
```

---

## Position and size

Every component has bounds relative to its parent. The position (x, y) is the
top-left corner in the parent's coordinate space.

### Setting bounds

```cpp
// By coordinates
comp.setBounds (10, 20, 300, 200);

// By rectangle
comp.setBounds ({ 10, 20, 300, 200 });

// Position and size separately
comp.setPosition ({ 10, 20 });
comp.setSize (300, 200);
```

### Getting bounds

```cpp
Rectangle<float> bounds = comp.getBounds();              // relative to parent
Rectangle<float> local  = comp.getLocalBounds();         // (0, 0, width, height)
Rectangle<float> screen = comp.getScreenBounds();        // in screen coords
Rectangle<float> top    = comp.getBoundsRelativeToTopLevelComponent();
Rectangle<float> safe   = comp.getSafeAreaBounds();      // excluding notches etc.
```

### Convenience accessors

```cpp
float x = comp.getX();
float y = comp.getY();
float w = comp.getWidth();
float h = comp.getHeight();
Point<float> center = comp.getCenter();
float cx = comp.getCenterX();
float cy = comp.getCenterY();
```

Setters: `setCenter()`, `setTopLeft()`, `setTopRight()`, `setBottomLeft()`,
`setBottomRight()`, `setCenterX()`, `setCenterY()`.

### Proportional sizing

```cpp
float halfWidth  = comp.proportionOfWidth (0.5f);
float halfHeight = comp.proportionOfHeight (0.5f);
```

### Resized callback

Override `resized()` to respond to size changes — this is where you set child
bounds:

```cpp
void MyComponent::resized() override
{
    header.setBounds (0, 0, getWidth(), 40);
    body  .setBounds (0, 40, getWidth(), getHeight() - 40);
}
```

```{warning}
Do **not** call `repaint()` inside `resized()`. `setBounds()` already triggers a
repaint of the old and new areas.
```

### Coordinate conversion

```cpp
// Between local and screen
Point<float> screenPt = comp.localToScreen ({ 0, 0 });
Point<float> localPt  = comp.screenToLocal (screenPt);

// Between components (handles all transforms in the hierarchy)
Point<float> p = compA.getLocalPoint (compB, pointInB);
Point<float> p = compA.getRelativePoint (compB, localInA);
Rectangle<float> r = compA.getLocalArea (compB, rectInB);
Rectangle<float> r = compA.getRelativeArea (compB, localInA);
```

---

## Painting

Painting in YUP is done through two virtual methods called by the framework
every frame (or on demand via `repaint()`). You paint using the `Graphics` class
from `yup_graphics`.

### `paint()` — draw the component itself

```cpp
void MyComponent::paint (Graphics& g) override
{
    g.setFillColor (Colors::cornflowerBlue);
    g.fillRoundedRectangle (getLocalBounds(), 8.0f);
}
```

Runs in this order: parent `paint()` → children `paint()` →
`paintOverChildren()`.

### `paintOverChildren()` — draw on top of children

```cpp
void MyComponent::paintOverChildren (Graphics& g) override
{
    // Draw a border or overlay that sits above all children.
    g.setStrokeColor (Colors::black);
    g.drawRoundedRectangle (getLocalBounds().reduced (0.5f), 8.0f, 1.0f);
}
```

### `repaint()` — trigger a redraw

```cpp
comp.repaint();                        // mark entire component dirty
comp.repaint ({ 10, 10, 100, 100 });  // mark a sub-rectangle dirty
comp.repaint (10, 10, 100, 100);      // x, y, w, h overload
```

```{warning}
`repaint()` must not be called from inside `paint()`. The framework asserts on
this in debug builds.
```

### Optimising paint with opacity

If a component is fully opaque (covers its background entirely), inform the
framework so it can skip painting occluded ancestors:

```cpp
comp.setOpaque (true);
```

YUP will skip painting behind it where possible. Only use this when you paint
every pixel of the component's bounds with a fully opaque fill.

### Unclipped rendering

By default, child `paint()` is clipped to the component's bounds. To allow
painting outside (e.g. a shadow that bleeds beyond the bounds):

```cpp
comp.enableRenderingUnclipped (true);
```

### `refreshDisplay()` — continuous animation

Override `refreshDisplay()` for per-frame animation (timers, property animation,
etc.):

```cpp
void MySpinner::refreshDisplay (double lastFrameTimeSeconds) override
{
    rotation += 360.0f * static_cast<float> (lastFrameTimeSeconds);
    repaint();
}
```

This is called every frame for the entire component tree. The
`lastFrameTimeSeconds` parameter is the wall-clock time elapsed since the
previous frame.

---

## Transform and opacity

### Affine transforms

```cpp
comp.setTransform (AffineTransform::rotation (0.785f));
comp.setTransform (AffineTransform::scale (2.0f));
comp.setTransform (AffineTransform::translation (50, 50));

AffineTransform t = comp.getTransform();
bool isXformed = comp.isTransformed(); // true if transform is not identity
```

Transforms affect the component's painting and its children recursively. Mouse
hit-testing accounts for the transform automatically.

To get the cumulative transform between two components:

```cpp
AffineTransform aToB = compA.getTransformToComponent (&compB);
AffineTransform bToA = compA.getTransformFromComponent (&compB);
AffineTransform toScreen = comp.getTransformToScreen();
```

### Opacity

```cpp
comp.setOpacity (0.5f);   // 50% transparent (applies to children too)
float alpha = comp.getOpacity();
```

The painted opacity is multiplied up the tree: a child with 0.5 opacity inside a
parent with 0.5 opacity renders at 0.25 overall.

---

## Z-order

Child components are drawn in z-order (back to front). Index 0 is behind
everything, higher indices are in front.

```cpp
comp.toFront (false);           // bring to front, don't steal keyboard focus
comp.toFront (true);            // bring to front AND grab keyboard focus
comp.toBack();                  // send to back
comp.raiseAbove (&otherComp);
comp.lowerBelow (&otherComp);
comp.raiseBy (1);               // move up by 1 position
comp.lowerBy (2);               // move down by 2 positions
```

---

## Mouse and keyboard

### Mouse events

Override mouse callbacks. Position is always in component-local coordinates:

```cpp
void mouseEnter      (const MouseEvent& event) override;
void mouseExit       (const MouseEvent& event) override;
void mouseDown       (const MouseEvent& event) override;
void mouseUp         (const MouseEvent& event) override;
void mouseMove       (const MouseEvent& event) override;
void mouseDrag       (const MouseEvent& event) override;
void mouseDoubleClick(const MouseEvent& event) override;
void mouseWheel      (const MouseEvent& event, const MouseWheelData& wheel) override;
```

By default these are no-ops. To receive mouse events, a component must opt in:

```cpp
comp.setWantsMouseEvents (true, true);
//                        ^^^^  ^^^^
//                        self  children
```

The first flag controls whether this component receives mouse events on itself.
The second controls whether its children can receive them (useful for
mouse-blocking overlays).

### Multitouch (mobile and Emscripten on a mobile browser)

On iOS, Android and Emscripten in a mobile browser, every finger is delivered
through the same mouse callbacks, with the left button held. The first finger
behaves exactly like a mouse; each additional finger produces parallel
`mouseDown` / `mouseDrag` / `mouseUp` events. Distinguish fingers with
`MouseEvent::isTouch()` and `MouseEvent::getTouchIndex()` - a dense, zero-based
index that stays stable for a finger while it is in contact:

```cpp
class DragHandle : public Component
{
    int activeFinger = -1;

    void mouseDown (const MouseEvent& e) override
    {
        if (e.isTouch() && e.getTouchIndex() > 0)   // track only extra fingers
            activeFinger = e.getTouchIndex();
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (e.isTouch() && e.getTouchIndex() == activeFinger)
            dragTo (e.getPosition());
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (e.isTouch() && e.getTouchIndex() == activeFinger)
            activeFinger = -1;
    }
};
```

Notes:

- Touch events are only generated on `YUP_MOBILE` and `YUP_EMSCRIPTEN` targets.
- `MouseEvent::getPressure()` reports the touch pressure in the range 0.0-1.0
  (0.0 for mouse events).
- Only the first finger takes keyboard focus, so an additional finger can never
  steal it mid-gesture.
- A finger cancelled by the system (e.g. a palm resting on the screen) is
  delivered as a regular `mouseUp`.

For a two-finger pinch, remember the position of every finger in `mouseDown`
and compare the distance between them in `mouseDrag`:

```cpp
std::map<int, Point<float>> fingers; // touchIndex -> current position

void mouseDown (const MouseEvent& e) override
{
    if (e.isTouch())
        fingers[e.getTouchIndex()] = e.getPosition();
}

void mouseDrag (const MouseEvent& e) override
{
    if (e.isTouch())
    {
        fingers[e.getTouchIndex()] = e.getPosition();

        if (fingers.size() == 2)
        {
            auto it = fingers.begin();
            const auto a = it->second;
            const auto b = (++it)->second;
            // The distance between a and b is the current pinch span.
        }
    }
}

void mouseUp (const MouseEvent& e) override
{
    if (e.isTouch())
        fingers.erase (e.getTouchIndex());
}
```

### Mouse cursor

```cpp
comp.setMouseCursor (MouseCursor::PointingHandCursor);
// or override:
virtual MouseCursor getMouseCursor() const override;
```

### Keyboard focus

```cpp
comp.setWantsKeyboardFocus (true);
comp.takeKeyboardFocus();
comp.leaveKeyboardFocus();
bool hasFocus = comp.hasKeyboardFocus();
```

When a component is clicked, focus walks from the clicked component up through
its parents until it finds one that wants focus. To prevent a component from
grabbing focus on click:

```cpp
comp.setClickingGrabFocus (false);
```

Override `focusGained()` and `focusLost()` to react to focus changes:

```cpp
void MyTextEditor::focusGained() override { showCaret = true; repaint(); }
void MyTextEditor::focusLost()   override { showCaret = false; repaint(); }
```

### Keyboard input

```cpp
virtual void keyDown   (const KeyPress& keys, const Point<float>& pos) override;
virtual void keyUp     (const KeyPress& keys, const Point<float>& pos) override;
virtual void textInput (const String& text) override;
```

### Mouse listeners (external objects)

Instead of subclassing to receive mouse events, attach a `MouseListener`:

```cpp
comp.addMouseListener (myListener);
comp.removeMouseListener (myListener);
```

---

```{seealso}
- [Drag and drop](component-drag-and-drop.md) — receiving external file/text drops
```
```

---

```{seealso}
- [Component styling](component-styling.md) — colors, metrics, and `ComponentStyle`
```

---

## Component listeners

Attach a `ComponentListener` to observe lifecycle events and paint metrics:

```cpp
struct MyListener : public ComponentListener
{
    void componentMoved (Component& c) override       { /* ... */ }
    void componentResized (Component& c) override     { /* ... */ }
    void componentBeingDeleted (Component& c) override { /* ... */ }
    void componentPaintCompleted (Component& c,
                                   const ComponentPaintMetrics& m) override
    {
        // m.repaintArea, m.totalTicks, m.selfTicks, m.childrenTicks, etc.
    }
};

comp.addComponentListener (&listener);
```

```{seealso}
[Component paint profiling](component-profiling.md)
```

---

## Caching and snapshots

Two performance/pixel-capture features are available directly on `Component`:

| Feature | Method | Purpose |
|---|---|---|
| **Texture caching** | `setCachedToTexture(true)` | Rasterise `paint()` to a GPU texture, reused across frames. |
| **Snapshot** | `snapshotToImage(ctx)` | Capture the subtree to a CPU-side `Image`. |
| **Texture snapshot** | `snapshotToTexture(ctx)` | Capture the subtree to a GPU texture, no CPU readback. |

See the dedicated guides:

- [Component caching](component-caching.md) — `setCachedToTexture` and
  `isCachedToTexture`
- [Component snapshots](component-snapshots.md) — `snapshotToImage` and
  `snapshotToTexture`

---

## Effects

Apply GPU shader post-processing (blur, pixelate, etc.) to a component's subtree:

```cpp
comp.setComponentEffect (myBlurEffect);
comp.setComponentEffect (nullptr); // remove
ComponentEffect::Ptr e = comp.getComponentEffect();
```

The subtree renders offscreen, the effect's shader composites the result. Nesting
works: a child's effect completes before the parent's effect captures it.

```{seealso}
[Component effects (shaders)](component-effects.md)
```

---

## Native window management

Components are hosted inside a platform-native window:

```cpp
// Place a top-level component on the screen
ComponentNative::Options opts;
opts.title       = "My Window";
opts.width       = 800;
opts.height      = 600;
opts.resizable   = true;
opts.dpiScale    = 2.0f;

rootComponent.addToDesktop (opts);

// Remove from the desktop
rootComponent.removeFromDesktop();

// Check desktop status
bool onDesktop = rootComponent.isOnDesktop();

// Close request
virtual void userTriedToCloseWindow() override { removeFromDesktop(); }
```

---

## DPI awareness

When a window moves between monitors or the system scale factor changes:

```cpp
void MyComponent::contentScaleChanged (float dpiScale) override
{
    // Recompute sizes, reload high-DPI images, etc.
}

float currentDpi = comp.getScaleDpi();
```

---

## Full-screen

```cpp
comp.setFullScreen (true);
comp.setFullScreen (false);
bool isFS = comp.isFullScreen();
```

---

## Properties

Arbitrary key-value data attached to a component via `NamedValueSet`:

```cpp
comp.getProperties().set ("my-key", 42);

auto* val = comp.getProperties().getVarPointer ("my-key");
if (val != nullptr && val->isInt())
    int value = static_cast<int> (*val);
```

---

## Full example

```cpp
class CounterPanel : public Component
{
public:
    CounterPanel()
        : Component ("CounterPanel")
    {
        // Button triggers increment
        button.setTitle ("Click me");
        button.setWantsMouseEvents (true, false);
        button.addMouseListener (this, true);
        addAndMakeVisible (button);

        label.setText ("Count: 0");
        label.setWantsMouseEvents (true, false);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        button.setBounds (bounds.removeFromTop (40).reduced (4));
        label.setBounds (bounds.reduced (4));
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (e.originalComponent == &button)
        {
            ++count;
            label.setText ("Count: " + String (count));
            label.repaint();
        }
    }

private:
    Component button;
    Component label;
    int count = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CounterPanel)
};
```

---

## Related

- [Drag and drop](component-drag-and-drop.md) — receiving external file/text drops
- [Component styling](component-styling.md) — colors, metrics, and `ComponentStyle`
- [Component effects (shaders)](component-effects.md) — GPU post-processing
- [Component caching](component-caching.md) — `setCachedToTexture` and `isCachedToTexture`
- [Component snapshots](component-snapshots.md) — `snapshotToImage` and `snapshotToTexture`
- [Component paint profiling](component-profiling.md) — measuring paint cost
