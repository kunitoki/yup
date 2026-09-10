import pytest

import yup

"""
ScrollBar is pure state plus one callback: the visible range it reports, the mode that decides
when it hides itself, and the position it hands back when the user scrolls. None of that needs
a native window, so these tests drive the public API directly.

What is *not* covered: the drag behaviour, which needs real mouse events from the platform.

The file runs inside yup.TestApplication, like the other widget tests. Nothing in ScrollBar's
state path reads the theme, so this is about keeping the widget tests uniform rather than a
requirement of what is asserted here; the bar is given a size so the thumb geometry it derives
along the way is the geometry of a real scroll bar.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


def make_scrollbar(orientation=yup.ScrollBar.Orientation.vertical):
    bar = yup.ScrollBar(orientation)
    bar.setSize(200.0, 12.0)

    return bar


def test_defaults():
    bar = make_scrollbar()

    assert bar.getOrientation() == yup.ScrollBar.Orientation.vertical
    assert bar.getVisibilityMode() == yup.ScrollBar.VisibilityMode.autoHide
    assert bar.isAutoHide() is True
    assert bar.getScrollBarWidth() == 12.0
    assert bar.isDragging() is False
    assert bar.isThumbHovered() is False


def test_orientation_can_be_chosen_and_changed():
    assert make_scrollbar(yup.ScrollBar.Orientation.horizontal).getOrientation() == yup.ScrollBar.Orientation.horizontal

    bar = make_scrollbar()
    bar.setOrientation(yup.ScrollBar.Orientation.horizontal)

    assert bar.getOrientation() == yup.ScrollBar.Orientation.horizontal


# ==============================================================================
# Range
# ==============================================================================

def test_range_limits_round_trip():
    bar = make_scrollbar()
    bar.setRangeLimits(-50.0, 150.0)

    assert bar.getRangeMinimum() == -50.0
    assert bar.getRangeMaximum() == 150.0


def test_current_range_round_trips():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(10.0, 30.0)

    assert bar.getCurrentRangeStart() == 10.0
    assert bar.getCurrentRangeEnd() == 30.0
    assert bar.getCurrentRangeSize() == 20.0


def test_current_range_is_clamped_to_the_limits():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(-50.0, 500.0)

    assert bar.getCurrentRangeStart() == 0.0
    assert bar.getCurrentRangeEnd() == 100.0


def test_scrolling_is_only_needed_when_content_exceeds_the_viewport():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)

    bar.setCurrentRange(0.0, 100.0)
    assert bar.isScrollingNeeded() is False

    bar.setCurrentRange(0.0, 40.0)
    assert bar.isScrollingNeeded() is True


# ==============================================================================
# Scrolling
# ==============================================================================

def test_scroll_by_moves_the_window_and_keeps_its_size():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(10.0, 30.0)

    bar.scrollBy(5.0)

    assert bar.getCurrentRangeStart() == 15.0
    assert bar.getCurrentRangeSize() == 20.0


def test_scroll_by_clamps_at_the_end_of_the_range():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(10.0, 30.0)

    bar.scrollBy(1000.0)

    assert bar.getCurrentRangeEnd() == 100.0
    assert bar.getCurrentRangeSize() == 20.0


def test_set_current_range_start_keeps_the_window_size():
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(10.0, 30.0)

    bar.setCurrentRangeStart(50.0)

    assert bar.getCurrentRangeStart() == 50.0
    assert bar.getCurrentRangeEnd() == 70.0


def test_the_position_callback_receives_the_new_start():
    positions = []
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(0.0, 20.0)
    bar.onScrollPositionChanged = positions.append

    bar.setCurrentRangeStart(30.0)

    assert positions == [30.0]


def test_the_position_callback_is_skipped_when_asked_not_to_notify():
    positions = []
    bar = make_scrollbar()
    bar.setRangeLimits(0.0, 100.0)
    bar.setCurrentRange(0.0, 20.0)
    bar.onScrollPositionChanged = positions.append

    bar.setCurrentRangeStart(30.0, yup.NotificationType.dontSendNotification)

    assert positions == []
    assert bar.getCurrentRangeStart() == 30.0


# ==============================================================================
# Visibility
# ==============================================================================

def test_visibility_modes_round_trip():
    bar = make_scrollbar()

    bar.setVisibilityMode(yup.ScrollBar.VisibilityMode.alwaysHidden)

    assert bar.getVisibilityMode() == yup.ScrollBar.VisibilityMode.alwaysHidden
    assert bar.isAutoHide() is False


def test_auto_hide_is_a_shorthand_for_the_visibility_mode():
    bar = make_scrollbar()

    bar.setAutoHide(False)

    assert bar.isAutoHide() is False
    assert bar.getVisibilityMode() == yup.ScrollBar.VisibilityMode.alwaysVisible

    bar.setAutoHide(True)

    assert bar.isAutoHide() is True
    assert bar.getVisibilityMode() == yup.ScrollBar.VisibilityMode.autoHide


def test_the_width_is_kept_at_one_pixel_minimum():
    bar = make_scrollbar()

    bar.setScrollBarWidth(24.0)
    assert bar.getScrollBarWidth() == 24.0

    bar.setScrollBarWidth(0.0)
    assert bar.getScrollBarWidth() == 1.0


# ==============================================================================
# Style identifiers
# ==============================================================================

def test_style_ids_are_exposed():
    assert str(yup.ScrollBar.Style.trackColorId) == "scrollBarTrack"
    assert str(yup.ScrollBar.Style.thumbColorId) == "scrollBarThumb"
    assert str(yup.ScrollBar.Style.thumbHoverColorId) == "scrollBarThumbHover"
    assert str(yup.ScrollBar.Style.thumbDraggingColorId) == "scrollBarThumbDragging"


def test_style_is_not_constructible():
    # The nested Style struct only holds static members, so it is exposed for its ids only.
    try:
        yup.ScrollBar.Style()
    except TypeError:
        pass
    else:
        raise AssertionError("ScrollBar.Style should not be constructible")
