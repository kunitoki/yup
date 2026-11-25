# Component

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

Components form a tree: you build the hierarchy by calling
`addChildComponent()` or `addAndMakeVisible()`. Parenting and layout are
separate concerns — adding a child does not position it. You can set bounds by
hand from `resized()`, or hand the work to one of the layout containers
described in [Components Layout](component-layout.md).

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

To find the component that would actually receive a mouse event at a point
(honoring `setWantsMouseEvents()`, see below), use `findComponentAtForMouseEvent()`
instead — it skips a component or subtree that opted out of mouse events in
favor of the sibling underneath it.

Each component decides for itself whether a point is inside it, by way of
`hitTest()`. The default accepts the whole of `getLocalBounds()`; override it to
carve away the parts that should let events through to whatever is behind them:

```cpp
bool RoundButton::hitTest (float x, float y) override
{
    return getLocalBounds().getCenter().distanceTo ({ x, y }) <= getWidth() / 2.0f;
}
```

This is the same test the mouse uses, so a point the override rejects is
delivered to the component behind instead. An override can only take area away:
a point outside the component's bounds never reaches it, because its parent
rejects the point before `hitTest()` is consulted.

### Lifecycle callbacks

Override these to react to tree changes:

```cpp
void parentHierarchyChanged() override;                        // added to or removed from a parent
void childrenChanged() override;                               // child added or removed
void childBoundsChanged (Component* child) override;           // a direct child moved or was resized
void parentSizeChanged() override;                             // this component's parent was resized
void indexInParentChildrenChanged (int oldI, int newI) override; // this component's z-order changed
```

A single `setBounds()` on a child reports one `childBoundsChanged()`, not one
for the move and one for the resize, so a parent can lay itself out from that
callback without doing the work twice. `parentSizeChanged()` runs after the
parent's own `resized()`, so any layout the parent performs is already reflected
in this component's bounds, and it only reaches direct children — it does not
propagate down the whole subtree.

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

To react to a *parent* being resized rather than this component, override
`parentSizeChanged()`; to react to a *child* changing bounds, override
`childBoundsChanged()`.

### Coordinate conversion

```cpp
// Between local and screen
Point<float> screenPt = comp.localToScreen ({ 0, 0 });
Point<float> localPt  = comp.screenToLocal (screenPt);

// Between components (follows transforms, effects and parent projections)
Point<float> p = compA.getLocalPoint (compB, pointInB);
Point<float> p = compA.getRelativePoint (compB, localInA);
Rectangle<float> r = compA.getLocalArea (compB, rectInB);
Rectangle<float> r = compA.getRelativeArea (compB, localInA);

// One step at a time
std::optional<Point<float>> inChild  = child.getLocalPointFromParent (pointInParent);
std::optional<Point<float>> inParent = child.getParentPointFromLocal (pointInChild);
Point<float> inComp = comp.getLocalPointFromTopLevel (pointInWindow);
```

Rectangles are converted as the bounding box of their mapped corners.

---

## Painting

Painting in YUP is done through two virtual methods called by the framework
every frame (or on demand via `repaint()`). You paint using the `Graphics` class
from `yup_graphics`.

### `paint()` — draw the component itself

```cpp
void MyComponent::paint (Graphics& g) override
{
    g.setFillColor (Colors::cornflowerblue);
    g.fillRoundedRect (getLocalBounds(), 8.0f);
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
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (getLocalBounds().reduced (0.5f), 8.0f);
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

### Multiple dirty areas (`RepaintMode`)

When several disjoint areas of a window are dirty in the same frame, the framework
repaints each dirty rectangle in isolation: a parent shared by several dirty
rectangles is painted once, clipped to those rectangles, and only the parts of the
hierarchy that overlap a dirty rectangle are repainted. Anything between two distant
dirty rectangles is left untouched, so it is not redrawn even though it did not
change. This is the default (`RepaintMode::disjointRegions`).

The older behaviour — collapsing every dirty rectangle into a single bounding box,
which repaints everything in between — remains available as a safe fallback:

```cpp
auto opts = ComponentNative::Options()
                .withRepaintMode (ComponentNative::RepaintMode::boundingBox);

rootComponent.addToDesktop (opts);
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

### Paint margin

A paint margin gives a component room around its bounds to paint into, such as
a drop shadow, without changing its bounds, layout or hit testing. `paint()`
may draw into it, and `repaint()` covers it, so nothing is left behind when the
component moves, resizes or hides:

```cpp
comp.setPaintMargin (8.0f, 8.0f, 8.0f, 12.0f); // left, top, right, bottom
auto area = comp.getPaintBounds();              // local bounds grown by the margin
```

In a `.yui` document it is the `paintMargin` property, see
[Styles](yui/styles.md).

The margin is still clipped to the parent, and desktop components paint within
their bounds only. An [effect](component-effects.md), a stylesheet `filter:` and
the texture of a cached component hold the margin too. A `PopupMenu` shown
inside a parent component uses it for its drop shadow.

### Per-frame updates

A `FrameAttachment` calls a function once per frame of the window the component
is in, before it is painted, with the time elapsed since the previous frame. Use
it for animation, kinetic scrolling and anything else that moves with the
display:

```cpp
class MySpinner : public yup::Component
{
public:
    void paint (yup::Graphics& g) override;

private:
    void advance (double elapsedSeconds)
    {
        rotation += 360.0f * static_cast<float> (elapsedSeconds);
        repaint();
    }

    float rotation = 0.0f;
    yup::FrameAttachment frameAttachment { *this, yup::bindFront (&MySpinner::advance, this) };
};
```

- The attachment follows the component: the function is called while the
  component is in a window, whichever window that is, and no longer once the
  component leaves it or is deleted.
- A frame callback does not paint anything by itself: call `repaint()` when what
  is shown changed.
- To get frames only for a while, hold a `std::optional<yup::FrameAttachment>`
  and reset it when the animation ends. The function may reset its own
  attachment.
- A window with no attachment and nothing to repaint does no work per frame, so
  attach only what needs every frame.
- Any object can own one, not just the component: pass the component whose
  window should drive it.

---

## Threading

A window is served by two threads:

- The **message thread** runs the event loop: mouse, keyboard and touch
  handlers, timers and `MessageManager::callAsync`.
- The **render thread** of the window runs its frames: the `FrameAttachment`
  functions first, then `paint()` and `paintOverChildren()` when something needs
  repainting.

The render thread holds the message manager lock while it calls into the
components, which keeps the message thread waiting. The two never run component
code at the same time, so what a mouse handler or a timer shares with `paint()`
needs no lock of its own, and children can be added or removed from the message
thread at any time.

| Part of a frame | Thread | Message manager locked | Graphics context current |
|---|---|---|---|
| `FrameAttachment` functions | Render | Yes | No |
| `paint()`, `paintOverChildren()`, effects | Render | Yes | Yes |
| Submitting the frame to the GPU and presenting it | Render | No | Yes |

What follows from this:

- **Keep frame functions and `paint()` short.** The message thread, and with it
  all input, waits for them.
- **Callbacks run on the thread that caused them.** A `setBounds()` from a frame
  function calls `resized()` on the render thread, still with the lock held.
- **Do GPU work in `paint()`**, where the graphics context is current. Anywhere
  else go through `ComponentNative::runWithGraphicsContext()`.
- **The frame is submitted after the lock is released.** What `paint()` drew is
  still in use while the message thread runs again. A `Path` can be changed
  right away: it then changes an outline of its own.
- **Other threads are not covered.** Hand data from audio and worker threads
  over with atomics, a queue or a lock, or take a `MessageManagerLock`, which
  keeps out the message thread and the frames alike.
- **A nested message loop lets frames run.** A handler that runs a modal loop
  sees frames before it returns, so bring its state up to date first.
- On the web there is no render thread: frames run on the main thread, between
  messages.

See [the message thread](../core/multithreading/message-thread.md) for the lock
and for posting work to the message thread.

---

## Transform and opacity

### Affine transforms

```cpp
comp.setTransform (AffineTransform::rotation (0.785f));
comp.setTransform (AffineTransform::scaling (2.0f));
comp.setTransform (AffineTransform::translation (50, 50));

AffineTransform t = comp.getTransform();
bool isXformed = comp.isTransformed(); // true if transform is not identity
```

<p align="center">
  <img class="light-only rounded" src="../_static/images/ui/component-basics_transform_light.jpg#gh-light-mode-only" alt="A component rotated, scaled and translated from where it was placed" width="640">
  <img class="dark-only rounded" src="../_static/images/ui/component-basics_transform_dark.jpg#gh-dark-mode-only" alt="A component rotated, scaled and translated from where it was placed" width="640">
</p>

Transforms are applied in the component's local coordinates, before its position
in the parent, and they compose down the hierarchy: a transformed parent carries
its children with it. Painting, hit-testing, mouse and drag-and-drop events, and
the coordinate conversions all follow the composed transform, so a click lands on
the child that is displayed under the pointer.

Inside `paint()` the mapping from local coordinates to the render target is
`g.getTransform().translated (g.getDrawingArea().getTopLeft())`: the linear part
of the composed transform lives in the transform and the translation in the
drawing area, so untransformed components see an identity transform exactly as
before. Clips set with `Graphics::setClipPath()` use the same local coordinates
as every draw call.

A parent can also present its children through a custom projection by overriding
`getChildPointFromLocal()` / `getLocalPointFromChild()`, which every input path
uses for each parent to child step. See [Components in 3D](component-3d.md).

Popups should be added to `getPopupParentComponent()`, the closest transformed
(or manually composited) ancestor, so they are presented with the same transform
as the component that opened them; `ComboBox` does this.

A component marked with `setPopupContainer (true)` holds the popups opened inside
it, even untransformed. Use it when a component effect is the look of a whole
interface, such as a color grade or a CRT pass, so its menus and combo box lists
are drawn through the effect and follow its input mapping too:

```cpp
interface.setComponentEffect (new InterfaceLookEffect());
interface.setPopupContainer (true);
```

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

<p align="center">
  <img class="light-only rounded" src="../_static/images/ui/component-basics_z-order_light.jpg#gh-light-mode-only" alt="Three overlapping components as added, with the first brought to the front and with the last sent to the back" width="640">
  <img class="dark-only rounded" src="../_static/images/ui/component-basics_z-order_dark.jpg#gh-dark-mode-only" alt="Three overlapping components as added, with the first brought to the front and with the last sent to the back" width="640">
</p>

A component that changes position in this list is told its old and new index:

```cpp
void MyComponent::indexInParentChildrenChanged (int oldIndex, int newIndex) override;
```

Only the component that moved is notified — the siblings that shift to make room
for it are not.

---

## Scrollable content

`Viewport` displays a component through a clipped area and automatically shows
scrollbars when the content is larger than that area:

```cpp
viewport.setViewedComponent (&content); // content keeps its own size and ownership
viewport.setScrollBarsShown (true, false); // vertical scrollbar only
viewport.scrollToShow (control.getBounds());
```

`setScrollBarsShown (showVertical, showHorizontal)` independently controls which
bars may appear. Pass `false, false` to hide both. Hidden bars reserve no space,
but wheel, touch and programmatic scrolling remain available on both axes.
To fit content without horizontal scrolling, size it to the available width
after accounting for a visible vertical scrollbar.

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
bool mouseWheel      (const MouseEvent& event, const MouseWheelData& wheel) override;
```

By default these are no-ops, and `mouseWheel()` returns `false`. To receive mouse
events, a component must opt in:

```cpp
comp.setWantsMouseEvents (true, true);
//                        ^^^^  ^^^^
//                        self  children
```

The first flag controls whether this component receives mouse events on itself.
The second controls whether its children can receive them (useful for
mouse-blocking overlays).

Mouse button and movement events go only to the component under the mouse. The
wheel goes further: it starts at the component under the pointer and travels up
the parent hierarchy until a component handles it. Return `true` to consume the
wheel or `false` to let the next enabled parent try. The default returns
`false`, so a scrollable container receives the wheel over any of its children.

```cpp
bool MyList::mouseWheel (const MouseEvent& event, const MouseWheelData& wheel)
{
    if (! canScroll())
        return false; // let the parent scroll

    scrollBy (wheel.getDeltaY());
    return true;
}
```

### Pointer events without a window

A component on no window gets no pointer event. `PointerSimulator` stands for
the pointer of a window showing a component: it finds what is under a
position, enters and leaves components as it moves, and gives a press to what
it is over until the release. Tools showing an interface with no window and
tests use it:

```cpp
PointerSimulator pointer (panel); // the component a window would show, visible

pointer.click (button.getBounds().getCenter());
pointer.drag ({ 20.0f, 55.0f }, { 180.0f, 55.0f });
pointer.wheel ({ 100.0f, 55.0f }, { 0.0f, 1.0f });

auto* hovered = pointer.getComponentUnderPointer();
```

Positions are in the coordinates of the component given to the constructor,
and each call returns the component that got the event. `moveTo`, `press` and
`release` do one step at a time. Keyboard focus and popups need a window: a
click gives no focus, and a menu it asks for does not open.

### Multitouch (any platform with touch hardware)

On any platform with touch hardware (mobile, Emscripten in a mobile browser,
and desktop touchscreens), every finger is delivered through the same mouse
callbacks, with the left button held. The first finger
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

- `MouseEvent::getPressure()` reports the touch pressure in the range 0.0-1.0
  (0.0 for mouse events).
- A finger cancelled by the system (e.g. a palm resting on the screen) is
  delivered as a regular `mouseUp`.
- Fingers have no hover: the first finger gets `mouseEnter` just before its
  `mouseDown` and `mouseExit` right after its `mouseUp` (or cancel).

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
comp.setMouseCursor (MouseCursor::Hand);
// or override:
MouseCursor getMouseCursor() const override;
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

A container that needs to know when the focus enters or leaves its subtree
overrides `focusOfChildComponentChanged()` instead. Every ancestor of the
component whose focus changed is notified, along with what caused the change:

```cpp
void MyPanel::focusOfChildComponentChanged (Component* child, FocusChangeType cause) override
{
    ignoreUnused (cause); // focusChangedByMouseClick or focusChangedDirectly

    setHighlighted (child->hasKeyboardFocus());
}
```

Moving the focus between two components notifies the ancestors of both, so a
shared ancestor is called twice: once with the component that lost the focus and
once with the one that gained it.

### Keyboard input

```cpp
virtual bool keyDown   (const KeyPress& keys, const Point<float>& pos) override;
virtual bool keyUp     (const KeyPress& keys, const Point<float>& pos) override;
virtual void textInput (const String& text) override;
```

A key goes to the component with the keyboard focus first. Return `true` from
`keyDown()` or `keyUp()` when the component used the key; otherwise the key goes
to its parent, and so on up to the top-level component. Invisible components are
skipped, and so are disabled ones and every component inside a disabled parent. So a focused control only keeps the keys it uses, and a
container can handle what its children leave: shortcuts, or playing a
`MidiKeyboardComponent` from the computer keyboard while a knob has the focus.

```cpp
bool MyPanel::keyDown (const KeyPress& key, const Point<float>&) override
{
    if (key.getKey() == KeyPress::textZKey && key.getModifiers().isCommandDown())
    {
        undoManager.undo();
        return true;
    }

    return false; // let the components around have it
}
```

Text editors return `true` for every key that types, edits or moves the caret,
since typed characters also arrive through `textInput()`, and leave Escape and
the shortcuts they don't implement to the components around them.

Two lower-level callbacks report input that `keyDown()` may never see:

```cpp
virtual bool keyStateChanged (const KeyPress& key, bool isDown) override;
virtual void modifierKeysChanged (const KeyModifiers& modifiers) override;
```

Like `keyDown()`, `keyStateChanged()` starts at the focused component and goes up
the parents until one returns `true`. `modifierKeysChanged()` is passed to the
parent by default; an override that still wants its parents notified calls
`Component::modifierKeysChanged (modifiers)`.

`keyStateChanged()` fires once when a key goes down and once when it comes up,
including for presses consumed before they reach `keyDown()`, and auto-repeat
does not retrigger it — which makes it the reliable way to track whether a chord
key is being held. `modifierKeysChanged()` fires whenever the modifier state
changes, and modifiers are refreshed by mouse button events as well as key
events — so a click can report a modifier that changed while the keyboard was
idle. Plain mouse movement does not refresh them.

### Mouse listeners (external objects)

Instead of subclassing to receive mouse events, attach a `MouseListener`:

```cpp
comp.addMouseListener (myListener);
comp.removeMouseListener (myListener);
```

Pass `true` as the second argument of `addMouseListener()` to also receive the
events of every component nested inside `comp`. The event keeps the position and
source component of the child that received it. `MouseListener::mouseWheel()`
returns `bool` to share the callback signature with `Component`; its return value
is ignored when the object is registered as a listener.

---

```{seealso}
- [Drag and drop](component-drag-and-drop.md) — receiving external file/text drops
```
```

---

```{seealso}
- [Component styling](component-styling.md) — colors, properties, and `ComponentStyle`
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
auto opts = ComponentNative::Options()
                .withDecoration (true)
                .withResizableWindow (true)
                .withAllowedHighDensityDisplay (true)
                .withVSync (true);

rootComponent.setTitle ("My Window");
rootComponent.setBounds ({ 0, 0, 800, 600 });
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

## Window aspect ratio

A desktop window can keep its proportions while the user resizes it. The ratio is
width over height, kept for a component that is not on the desktop yet and applied
when its window is created:

```cpp
window.setWindowAspectRatio (16.0f / 9.0f);
window.setWindowAspectRatio (std::nullopt); // resize freely again
```

The ratio constrains the whole window while the user resizes it. It may not hold
for a fullscreen or maximized window, and is not applied to the browser canvas,
whose size is given by the page. Content that should keep its own proportions inside decorations or toolbars,
or in a window that ignores the ratio, still has to fit itself, for example by
scaling to the largest size that fits and centering. Standalone plugin windows set
it from editors whose `shouldPreserveAspectRatio()` returns true, and letterbox
their content when the window does not keep the ratio.

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
- [Component styling](component-styling.md) — colors, properties, and `ComponentStyle`
- [Component effects (shaders)](component-effects.md) — GPU post-processing
- [Component caching](component-caching.md) — `setCachedToTexture` and `isCachedToTexture`
- [Component snapshots](component-snapshots.md) — `snapshotToImage` and `snapshotToTexture`
- [Component paint profiling](component-profiling.md) — measuring paint cost
