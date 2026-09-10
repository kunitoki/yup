import yup

"""
These tests cover the Component API surface that only existed in C++ before it was
bound: the safe area, paint profiling suppression, style metrics, cached-to-texture
rendering, component effects and component listeners.

Two parts of that surface are deliberately not covered:

- "snapshotToImage" / "snapshotToTexture" need a real GPU context and a component with
  a non-zero size, and "ComponentEffect.apply" is only reached through them.
- "componentBeingDeleted" fires from the middle of the C++ destructor, so observing it
  from a test means calling into Python while the component is being torn down.
"""

METRIC_ID = "componentTests_metric"


def bounds_tuple(bounds):
    return (bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight())


def assert_bounds(bounds, x, y, width, height):
    actual = bounds_tuple(bounds)
    expected = (x, y, width, height)

    for a, e in zip(actual, expected):
        assert abs(a - e) < 0.001, f"{actual} != {expected}"


# ==============================================================================
# Safe area
# ==============================================================================

def test_safe_area_bounds_are_local_bounds_without_a_parent():
    component = yup.Component()
    component.setSize(120.0, 40.0)

    assert_bounds(component.getSafeAreaBounds(), 0.0, 0.0, 120.0, 40.0)


def test_safe_area_bounds_are_clipped_to_the_parent_safe_area():
    parent = yup.Component()
    parent.setSize(100.0, 100.0)

    child = yup.Component()
    child.setBounds(50.0, 20.0, 100.0, 50.0)
    parent.addChildComponent(child)

    # The child sticks out of its parent, so only the visible part is safe.
    assert_bounds(child.getSafeAreaBounds(), 0.0, 0.0, 50.0, 50.0)


# ==============================================================================
# Paint profiling
# ==============================================================================

def test_paint_profiling_is_enabled_by_default():
    component = yup.Component()

    assert component.isPaintProfilingDisabled() is False


def test_paint_profiling_can_be_disabled():
    component = yup.Component()
    component.setPaintProfilingDisabled(True)

    assert component.isPaintProfilingDisabled() is True


# ==============================================================================
# Style metrics
# ==============================================================================

def test_metric_is_unset_by_default():
    component = yup.Component()

    assert component.getMetric(yup.Identifier(METRIC_ID)) is None
    assert component.findMetric(yup.Identifier(METRIC_ID)) is None


def test_metric_round_trip():
    component = yup.Component()
    component.setMetric(yup.Identifier(METRIC_ID), 4.0)

    assert component.getMetric(yup.Identifier(METRIC_ID)) == 4.0
    assert component.findMetric(yup.Identifier(METRIC_ID)) == 4.0


def test_metric_override_can_be_removed():
    component = yup.Component()
    component.setMetric(yup.Identifier(METRIC_ID), 4.0)
    component.setMetric(yup.Identifier(METRIC_ID), None)

    assert component.getMetric(yup.Identifier(METRIC_ID)) is None


def test_find_metric_walks_up_to_the_parent():
    parent = yup.Component()
    parent.setMetric(yup.Identifier(METRIC_ID), 6.0)

    child = yup.Component()
    parent.addChildComponent(child)

    assert child.getMetric(yup.Identifier(METRIC_ID)) is None
    assert child.findMetric(yup.Identifier(METRIC_ID)) == 6.0


# ==============================================================================
# Cached to texture
# ==============================================================================

def test_cached_to_texture_is_disabled_by_default():
    component = yup.Component()

    assert component.isCachedToTexture() is False


def test_cached_to_texture_can_be_enabled():
    component = yup.Component()
    component.setCachedToTexture(True)

    assert component.isCachedToTexture() is True


# ==============================================================================
# Component effects
# ==============================================================================

class NoopEffect(yup.ComponentEffect):
    def __init__(self):
        super().__init__()

    def apply(self, g, inputTexture, bounds):
        pass


def test_component_has_no_effect_by_default():
    component = yup.Component()

    assert component.getComponentEffect() is None


def test_component_effect_round_trip():
    component = yup.Component()
    component.setComponentEffect(NoopEffect())

    assert isinstance(component.getComponentEffect(), yup.ComponentEffect)


def test_component_effect_can_be_cleared():
    component = yup.Component()
    component.setComponentEffect(NoopEffect())
    component.setComponentEffect(None)

    assert component.getComponentEffect() is None


def test_component_keeps_the_effect_alive():
    component = yup.Component()

    # The effect is held by a reference counted pointer, so it outlives the temporary
    # Python wrapper the call was made with instead of leaving the component dangling.
    component.setComponentEffect(NoopEffect())

    assert component.getComponentEffect() is not None


# ==============================================================================
# Component listeners
# ==============================================================================

class RecordingListener(yup.ComponentListener):
    def __init__(self, log):
        super().__init__()
        self.log = log

    def componentMoved(self, component):
        self.log.append("moved")

    def componentResized(self, component):
        self.log.append("resized")


def test_listener_receives_resized():
    log = []
    component = yup.Component()
    component.addComponentListener(RecordingListener(log))

    component.setSize(30.0, 20.0)

    assert log == ["resized"]


def test_listener_receives_moved():
    log = []
    component = yup.Component()
    component.addComponentListener(RecordingListener(log))

    component.setBounds(10.0, 15.0, 30.0, 20.0)

    assert log == ["resized", "moved"]


def test_listener_stops_receiving_after_removal():
    log = []
    component = yup.Component()
    listener = RecordingListener(log)
    component.addComponentListener(listener)
    component.removeComponentListener(listener)

    component.setSize(30.0, 20.0)

    assert log == []


def test_component_keeps_the_listener_alive():
    log = []
    component = yup.Component()

    # Nothing else references the listener, and the listener list only stores weak
    # references, so only the component's own reference can keep it registered.
    component.addComponentListener(RecordingListener(log))

    component.setSize(30.0, 20.0)

    assert log == ["resized"]


# ==============================================================================
# ComponentPaintMetrics
# ==============================================================================

def test_paint_metrics_default_to_zero():
    metrics = yup.ComponentPaintMetrics()

    assert metrics.selfTicks == 0
    assert metrics.childrenTicks == 0
    assert metrics.frameworkTicks == 0
    assert metrics.totalTicks == 0
    assert metrics.renderContinuous is False
    assert metrics.selfPaintSkipped is False
