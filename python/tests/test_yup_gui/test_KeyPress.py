import yup


# ==============================================================================
# KeyPress
# ==============================================================================

def test_key_press_with_scancode_reports_its_text_character():
    key = yup.KeyPress(65, yup.KeyModifiers(), 97)

    assert key.getKey() == 65
    assert key.getTextCharacter() == 97
    assert key == yup.KeyPress(65, yup.KeyModifiers(), 97)


def test_key_press_without_scancode_has_no_text_character():
    assert yup.KeyPress(65).getTextCharacter() == 0
