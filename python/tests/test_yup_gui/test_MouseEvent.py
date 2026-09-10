import yup

"""
MouseEvent is the value type the mouse hooks on Component and MouseListener receive, so
these tests cover the event itself: how it is built, what it reports, and the
with*/without* builders, which return modified copies rather than mutating the event.

A real click or drag is produced by the SDL backend, before the event ever reaches Python,
so nothing here goes through a live window - the platform-independent part of the type is
what is exercised. The builders make that possible: they are how a Python caller builds the
exact event a callback would otherwise have been handed.
"""


def make_event(buttons=yup.MouseEvent.leftButton, position=(10.0, 20.0)):
    return yup.MouseEvent(buttons, yup.KeyModifiers(), yup.Point[float](*position))


def position_of(event):
    position = event.getPosition()

    return (position.getX(), position.getY())


# ==============================================================================
# Construction and buttons
# ==============================================================================

def test_default_event_has_no_buttons_and_is_not_a_touch():
    event = yup.MouseEvent()

    assert event.getButtons() == yup.MouseEvent.noButtons
    assert event.isAnyButtonDown() is False
    assert event.isTouch() is False
    assert event.getTouchIndex() == -1
    assert event.getPressure() == 0.0


def test_buttons_are_reported_per_button():
    event = make_event(yup.MouseEvent.middleButton)

    assert event.isLeftButtonDown() is False
    assert event.isMiddleButtonDown() is True
    assert event.isRightButtonDown() is False
    assert event.isAnyButtonDown() is True


def test_all_buttons_reports_every_button_down():
    event = make_event(yup.MouseEvent.allButtons)

    assert event.isLeftButtonDown() is True
    assert event.isMiddleButtonDown() is True
    assert event.isRightButtonDown() is True


def test_buttons_are_reachable_on_the_class_and_on_the_enum():
    assert yup.MouseEvent.Buttons.leftButton == yup.MouseEvent.leftButton
    assert yup.MouseEvent.Buttons.allButtons == yup.MouseEvent.allButtons


def test_with_buttons_returns_a_copy():
    event = make_event(yup.MouseEvent.leftButton)
    withMiddle = event.withButtons(yup.MouseEvent.middleButton)

    assert event.isMiddleButtonDown() is False
    assert withMiddle.isLeftButtonDown() is True
    assert withMiddle.isMiddleButtonDown() is True


def test_without_buttons_removes_only_the_given_button():
    event = make_event(yup.MouseEvent.allButtons)
    withoutMiddle = event.withoutButtons(yup.MouseEvent.middleButton)

    assert withoutMiddle.isMiddleButtonDown() is False
    assert withoutMiddle.isLeftButtonDown() is True
    assert withoutMiddle.isRightButtonDown() is True
    assert event.isMiddleButtonDown() is True


# ==============================================================================
# Position
# ==============================================================================

def test_position_is_reported():
    assert position_of(make_event(position=(3.5, -4.25))) == (3.5, -4.25)


def test_with_position_returns_a_copy():
    event = make_event(position=(1.0, 2.0))
    moved = event.withPosition(yup.Point[float](5.0, 6.0))

    assert position_of(event) == (1.0, 2.0)
    assert position_of(moved) == (5.0, 6.0)


def test_with_translated_position_offsets_the_position():
    translated = make_event(position=(10.0, 20.0)).withTranslatedPosition(yup.Point[float](-1.5, 2.5))

    assert position_of(translated) == (8.5, 22.5)


def test_screen_position_is_the_position_without_a_source_component():
    event = make_event(position=(3.0, 4.0))

    assert event.getSourceComponent() is None
    assert event.getScreenPosition() == event.getPosition()


# ==============================================================================
# Modifiers
# ==============================================================================

def modifier_flag(predicate):
    """The first flag value the given predicate reports as down.

    KeyModifiers keeps its masks private, so rather than hardcode one the tests discover
    a flag that turns the predicate on, which is also what ties the two bindings together.
    """
    for flags in range(1, 8192):
        if predicate(yup.KeyModifiers(flags)):
            return flags

    raise AssertionError("no modifier flag turned the predicate on")


def test_no_modifiers_are_down_by_default():
    modifiers = make_event().getModifiers()

    assert modifiers.isShiftDown() is False
    assert modifiers.isControlDown() is False
    assert modifiers.isCommandDown() is False
    assert modifiers.isAltDown() is False


def test_with_modifiers_returns_a_copy():
    shiftFlag = modifier_flag(lambda modifiers: modifiers.isShiftDown())

    event = make_event()
    shifted = event.withModifiers(yup.KeyModifiers(shiftFlag))

    assert event.getModifiers().isShiftDown() is False
    assert shifted.getModifiers().isShiftDown() is True
    assert position_of(shifted) == position_of(event)


# ==============================================================================
# The component an event refers to
# ==============================================================================

def test_source_component_is_none_unless_given():
    assert yup.MouseEvent().getSourceComponent() is None


def test_constructor_keeps_the_source_component():
    component = yup.Component()
    event = yup.MouseEvent(yup.MouseEvent.leftButton, yup.KeyModifiers(), yup.Point[float](1.0, 2.0), component)

    assert event.getSourceComponent() is component


def test_with_source_component_returns_a_copy():
    component = yup.Component()
    event = make_event()
    retargeted = event.withSourceComponent(component)

    assert event.getSourceComponent() is None
    assert retargeted.getSourceComponent() is component


def test_with_relative_position_to_none_is_a_copy():
    event = make_event(position=(7.0, 8.0))
    relative = event.withRelativePositionTo(None)

    assert position_of(relative) == (7.0, 8.0)


def test_with_relative_position_to_a_parentless_component_keeps_the_position():
    component = yup.Component()
    event = make_event(position=(7.0, 8.0))
    relative = event.withRelativePositionTo(component)

    assert position_of(relative) == (7.0, 8.0)
    assert relative.getSourceComponent() is component


# ==============================================================================
# Touch
# ==============================================================================

def test_default_event_is_a_mouse_event():
    event = yup.MouseEvent()

    assert event.isTouch() is False
    assert event.getTouchIndex() == -1
    assert event.getPressure() == 0.0


def test_with_touch_index_marks_the_event_as_touch():
    event = make_event().withTouchIndex(3)

    assert event.isTouch() is True
    assert event.getTouchIndex() == 3


def test_with_touch_index_rejects_a_negative_index():
    # A negative index means "not a touch", which the constructor normalises.
    event = make_event().withTouchIndex(-2)

    assert event.isTouch() is False
    assert event.getTouchIndex() == -1


def test_with_pressure_returns_a_copy():
    event = make_event()
    pressed = event.withPressure(0.5)

    assert event.getPressure() == 0.0
    assert pressed.getPressure() == 0.5


# ==============================================================================
# Last mouse down
# ==============================================================================

def test_last_mouse_down_defaults_to_the_origin():
    assert make_event().getLastMouseDownPosition() == yup.Point[float](0.0, 0.0)


def test_with_last_mouse_down_position_returns_a_copy():
    event = make_event()
    withDown = event.withLastMouseDownPosition(yup.Point[float](3.0, 4.0))

    assert position_of(event) == (10.0, 20.0)
    assert withDown.getLastMouseDownPosition() == yup.Point[float](3.0, 4.0)


def test_with_last_mouse_down_time_returns_a_copy():
    now = yup.Time.getCurrentTime()
    withTime = make_event().withLastMouseDownTime(now)

    assert withTime.getLastMouseDownTime() == now


# ==============================================================================
# Comparison
# ==============================================================================

def test_events_with_the_same_state_are_equal():
    assert make_event() == make_event()
    assert make_event() != make_event(yup.MouseEvent.rightButton)


def test_an_event_equals_itself_after_a_no_op_builder():
    event = make_event()

    assert event.withButtons(yup.MouseEvent.noButtons) == event
    assert event.withTranslatedPosition(yup.Point[float](0.0, 0.0)) == event
