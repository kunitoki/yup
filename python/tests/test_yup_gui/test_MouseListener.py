import yup

"""
MouseListener is the interface Component derives from in C++, and the way a standalone
Python object observes mouse input.

The platform is what drives the callbacks, so nothing here goes through a live window: the
bound entry points are called directly, which is exactly the path the trampoline routes back
into the Python overrides. That makes these tests cover the dispatch contract without
needing a native component, and MouseEvent itself is covered by test_MouseEvent.py.
"""


def make_event(buttons=yup.MouseEvent.leftButton, position=(10.0, 20.0)):
    return yup.MouseEvent(buttons, yup.KeyModifiers(), yup.Point[float](*position))


class RecordingListener(yup.MouseListener):
    def __init__(self):
        super().__init__()
        self.log = []

    def mouseEnter(self, event):
        self.log.append(("mouseEnter", event))

    def mouseExit(self, event):
        self.log.append(("mouseExit", event))

    def mouseDown(self, event):
        self.log.append(("mouseDown", event))

    def mouseMove(self, event):
        self.log.append(("mouseMove", event))

    def mouseDrag(self, event):
        self.log.append(("mouseDrag", event))

    def mouseUp(self, event):
        self.log.append(("mouseUp", event))

    def mouseDoubleClick(self, event):
        self.log.append(("mouseDoubleClick", event))

    def mouseWheel(self, event, wheelData):
        self.log.append(("mouseWheel", event, wheelData))


# ==============================================================================
# Python overrides receive the events
# ==============================================================================

def test_overrides_receive_every_mouse_event():
    listener = RecordingListener()
    event = make_event()

    listener.mouseEnter(event)
    listener.mouseExit(event)
    listener.mouseDown(event)
    listener.mouseMove(event)
    listener.mouseDrag(event)
    listener.mouseUp(event)
    listener.mouseDoubleClick(event)

    assert [entry[0] for entry in listener.log] == [
        "mouseEnter",
        "mouseExit",
        "mouseDown",
        "mouseMove",
        "mouseDrag",
        "mouseUp",
        "mouseDoubleClick",
    ]


def test_override_receives_the_event_the_caller_passed():
    listener = RecordingListener()
    event = make_event(yup.MouseEvent.rightButton, (1.5, 2.5))

    listener.mouseDown(event)

    _, received = listener.log[0]

    assert received.getButtons() == yup.MouseEvent.rightButton
    assert received.getPosition().getX() == 1.5
    assert received.getPosition().getY() == 2.5


def test_override_receives_the_wheel_data():
    listener = RecordingListener()
    wheel = yup.MouseWheelData(1.0, -2.0)

    listener.mouseWheel(make_event(), wheel)

    _, receivedEvent, receivedWheel = listener.log[0]

    assert receivedEvent.getButtons() == yup.MouseEvent.leftButton
    assert receivedWheel.getDeltaX() == 1.0
    assert receivedWheel.getDeltaY() == -2.0


# ==============================================================================
# The default implementations
# ==============================================================================

def test_default_implementations_do_nothing():
    listener = yup.MouseListener()
    event = make_event()
    wheel = yup.MouseWheelData()

    assert listener.mouseEnter(event) is None
    assert listener.mouseExit(event) is None
    assert listener.mouseDown(event) is None
    assert listener.mouseMove(event) is None
    assert listener.mouseDrag(event) is None
    assert listener.mouseUp(event) is None
    assert listener.mouseDoubleClick(event) is None
    assert listener.mouseWheel(event, wheel) is None


def test_a_partial_override_only_receives_what_it_overrides():
    class MoveOnly(yup.MouseListener):
        def __init__(self):
            super().__init__()
            self.moves = 0

        def mouseMove(self, event):
            self.moves += 1

    listener = MoveOnly()

    # The other callbacks fall through to the (empty) C++ implementations.
    listener.mouseDown(make_event())
    listener.mouseUp(make_event())
    listener.mouseMove(make_event())

    assert listener.moves == 1


# ==============================================================================
# Registration with a Component
# ==============================================================================

def test_a_component_is_a_mouse_listener():
    # Component derives from MouseListener in C++ and the bindings declare that base, so a
    # component can be handed to anything that expects a listener.
    assert issubclass(yup.Component, yup.MouseListener)


def test_listener_can_be_added_and_removed():
    component = yup.Component()
    listener = RecordingListener()

    component.addMouseListener(listener)
    component.removeMouseListener(listener)


def test_a_component_can_be_registered_as_a_listener():
    # Two distinct components, rather than one registered with itself: addMouseListener keeps
    # the listener alive for as long as the component lives, because the C++ listener list only
    # holds weak references. A component registered with itself would therefore reference
    # itself, and that cycle can only be broken by a garbage collection pass - which does not
    # happen before the process exits and its leak detector runs, so the C++ component would
    # outlive the test. pybind11 has no way to undo keep_alive, so removeMouseListener does not
    # release it either.
    owner = yup.Component()
    listener = yup.Component()

    owner.addMouseListener(listener)
    owner.removeMouseListener(listener)


def test_removing_a_listener_that_was_never_added_is_harmless():
    component = yup.Component()

    component.removeMouseListener(RecordingListener())
