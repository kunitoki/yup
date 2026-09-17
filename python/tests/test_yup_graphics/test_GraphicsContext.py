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


def test_graphics_context_timing_accessors_are_bound():
    assert hasattr(yup.GraphicsContext, "getFrameTimingCapabilities")
    assert hasattr(yup.GraphicsContext, "getLastFrameTimingInfo")
