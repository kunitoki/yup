import yup


# ==============================================================================
# Slider
# ==============================================================================

def test_slider_repr_shows_its_value():
    slider = yup.Slider(yup.SliderType.LinearHorizontal)
    assert repr(slider).startswith("<yup.Slider")
    assert " value=" in repr(slider)
