# Touch and Trackpad Gestures

YUP recognizes the gestures people expect on phones, tablets and trackpads: tap and double tap,
long press, pinch, rotate, multi-finger pan and swipes with fling velocity. A component asks for
the gestures it handles and receives them through virtual callbacks, next to its usual mouse
callbacks.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## Opting in

Gestures are opt-in per component. By default a component wants none, and every finger keeps
arriving as a parallel `mouseDown` / `mouseDrag` / `mouseUp` with its own touch index, exactly as
before, so multi-touch widgets such as `MidiKeyboardComponent` are unaffected.

```cpp
class MapView : public yup::Component
{
public:
    MapView()
    {
        setWantedGestures (yup::GestureType::pinch | yup::GestureType::rotate | yup::GestureType::pan);
    }

    void gesturePinch (const yup::GestureEvent& event) override
    {
        zoom *= event.getScaleDelta();
        repaint();
    }

    void gestureRotate (const yup::GestureEvent& event) override
    {
        angle += event.getRotationDelta();
        repaint();
    }

    void gesturePan (const yup::GestureEvent& event) override
    {
        offset += event.getTranslationDelta();
        repaint();
    }

private:
    float zoom = 1.0f, angle = 0.0f;
    yup::Point<float> offset;
};
```

A gesture goes to the deepest component under its start point (the finger, or the centroid of
the fingers) that wants that gesture type. Components that don't want it are skipped and the
search walks up to their parents, so a container can handle pinches while its children keep
handling taps. Invisible or disabled components receive nothing.

The callbacks come from `GestureListener`, which `Component` derives from. Any other object can
derive from it too and receive the gestures of a component with `addGestureListener`, after the
component itself has seen them:

```cpp
struct PinchLogger : yup::GestureListener
{
    void gesturePinch (const yup::GestureEvent& event) override
    {
        DBG ("scale " << event.getScale());
    }
};

PinchLogger logger;
view.addGestureListener (&logger); // view still needs setWantedGestures (GestureType::pinch)
```

## Gesture types

| `GestureType` | Callback | Fingers | Notes |
|---|---|---|---|
| `tap` | `gestureTap` | 1 or more | `getTapCount()` is 2 for a double tap. |
| `longPress` | `gestureLongPress` | 1 | A still finger held for the long-press delay. |
| `pinch` | `gesturePinch` | 2, or a trackpad | `getScale()` since it began, `getScaleDelta()` since the last event. |
| `rotate` | `gestureRotate` | 2 | Radians, positive clockwise on screen. |
| `pan` | `gesturePan` | 2 or more | `getTranslation()` / `getTranslationDelta()`, `getVelocity()` on end. |
| `swipe` | `gestureSwipe` | 1 or more | A fast flick, with `getSwipeDirection()` and `getVelocity()`. |

Pinch, rotate and pan from the same fingers can run at the same time, as in map applications.
`getTouchCount()` tells two- from three-finger pans and swipes. `GestureType::all` asks for
everything.

## Phases

Continuous gestures (pinch, rotate, pan) report `GestureEvent::Phase::began`, then `changed`
while the fingers move, then `ended` when they lift. If a finger is canceled by the system, or
the number of fingers changes, the running gestures end and start again from the new fingers.
Discrete gestures (tap, long press, swipe) arrive once, with the `ended` phase.

Positions are relative to the component receiving the gesture. Velocities are in pixels per
second, measured over the last 100 ms of movement.

## Mouse events while a gesture runs

When a pinch, rotate or pan starts with two or more fingers, the fingers it uses are handed over
to the gesture: each gets a final `mouseUp` and no further mouse events until it lifts. Single
finger taps, long presses and swipes don't take the finger over, so a one-finger drag still
reaches `mouseDrag`.

A component that does something on `mouseUp` should ignore the up that closes a finger taken by
a gesture. Count the touches seen in `mouseDown` and treat the up as a click only when a single
finger was down, as the Scene3D example does.

## Trackpads

A pinch on a trackpad (macOS, Wayland, X11) arrives in `gesturePinch` at the mouse position, with
`isTrackpad()` returning true and a touch count of 0. Two-finger trackpad scrolling keeps
arriving as `mouseWheel`.

## Using the recognizer directly

`GestureRecognizer` is the platform-free state machine behind the component callbacks. Feed it
touches with `touchDown`, `touchMove`, `touchUp` and `touchCancel`, passing the time in
milliseconds, and call `advanceTo` to let a long press fire while nothing moves. It emits
`GestureEvent`s through its callback. Thresholds such as tap slop, the double-tap interval, the
long-press delay and the swipe minimum velocity live in `GestureRecognizer::Options`.

```cpp
yup::GestureRecognizer::Options options;
options.longPressDelay = 400.0;

yup::GestureRecognizer recognizer ([] (const yup::GestureEvent& event) { /* ... */ }, options);
recognizer.touchDown (0, { 10.0f, 10.0f }, 0.0);
recognizer.touchUp (0, { 10.0f, 10.0f }, 80.0); // emits a tap
```

## Python

The same API is available from Python:

```python
class Canvas(yup.Component):
    def __init__(self):
        super().__init__()
        self.setWantedGestures(yup.GestureType.pinch | yup.GestureType.pan)

    def gesturePinch(self, event):
        print(event.getScaleDelta())

    def gesturePan(self, event):
        print(event.getTranslationDelta())
```
