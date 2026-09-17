import yup


def test_graphics_context_timing_types_are_bound():
    capabilities = yup.GraphicsContextFrameTimingCapabilities()
    timing_info = yup.GraphicsContextFrameTimingInfo()

    assert capabilities.hasPresentationTiming is False
    assert capabilities.hasFrameLatencyWait is False
    assert capabilities.hasGpuCompletionTiming is False
    assert capabilities.hasMaximumFramesInFlight is False
    assert capabilities.presentBlocksForDisplay is False

    assert timing_info.hasSubmissionTimestamps is False
    assert timing_info.hasGpuCompletionTimestamp is False
    assert timing_info.hasPresentationTimestamp is False
    assert timing_info.presentationCount == 0


def test_component_native_graphics_context_returns_empty_default_timing(juce_app):
    component = yup.Component()
    component.setSize(32.0, 32.0)
    component.addToDesktop(yup.ComponentNative.Options())

    try:
        native = component.getNativeComponent()
        assert native is not None

        context = native.getGraphicsContext()
        assert context is not None

        capabilities = context.getFrameTimingCapabilities()
        timing_info = context.getLastFrameTimingInfo()

        assert capabilities.hasPresentationTiming is False
        assert capabilities.hasFrameLatencyWait is False
        assert capabilities.hasGpuCompletionTiming is False
        assert capabilities.hasMaximumFramesInFlight is False
        assert capabilities.presentBlocksForDisplay is False

        assert timing_info.hasSubmissionTimestamps is False
        assert timing_info.hasGpuCompletionTimestamp is False
        assert timing_info.hasPresentationTimestamp is False
        assert timing_info.presentationCount == 0
    finally:
        component.removeFromDesktop()
