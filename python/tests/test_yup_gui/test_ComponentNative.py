import yup


def test_component_native_frame_pacing_mode_is_bound():
    assert yup.ComponentNative.FramePacingMode.automatic is not None
    assert yup.ComponentNative.FramePacingMode.off is not None
    assert yup.ComponentNative.FramePacingMode.software is not None
    assert yup.ComponentNative.FramePacingMode.presentationDriven is not None


def test_component_native_options_accept_frame_pacing_settings():
    options = yup.ComponentNative.Options()
    chained = options.withFramePacingMode(
        yup.ComponentNative.FramePacingMode.presentationDriven
    ).withMaximumFramesInFlight(3)

    assert chained is options

