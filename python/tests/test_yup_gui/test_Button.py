import yup


# ==============================================================================
# Button
# ==============================================================================

def test_button_construction():
    btn = yup.Button()
    assert btn is not None


def test_button_initial_state():
    btn = yup.Button()
    assert btn.isButtonOver() is False
    assert btn.isButtonDown() is False


def test_button_onclick_default():
    btn = yup.Button()
    assert btn.onClick is None
