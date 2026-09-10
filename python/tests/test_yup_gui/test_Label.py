import yup


# ==============================================================================
# Label
# ==============================================================================

def test_label_style_ids_are_exposed():
    assert yup.Label.Style.textFillColorId == yup.Identifier("Label_textFillColorId")
    assert yup.Label.Style.textStrokeColorId == yup.Identifier("Label_textStrokeColorId")
    assert yup.Label.Style.backgroundColorId == yup.Identifier("Label_backgroundColorId")
    assert yup.Label.Style.outlineColorId == yup.Identifier("Label_outlineColorId")
    assert yup.Label.Style.textHeightProportionMetricId == yup.Identifier("Label_textHeightProportionMetricId")


def test_label_style_ids_are_valid_identifiers():
    # The ids are Identifiers, so they render as the strings the widget's theme style looks up.
    assert str(yup.Label.Style.backgroundColorId) == "Label_backgroundColorId"
    assert yup.Label.Style.backgroundColorId.isValid() is True


def test_label_set_color_with_style_id():
    label = yup.Label("Header")
    label.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkblue)
    assert label.getColor(yup.Label.Style.backgroundColorId) == yup.Colors.darkblue


def test_label_clear_color_with_style_id():
    label = yup.Label("Header")
    label.setColor(yup.Label.Style.backgroundColorId, yup.Colors.darkblue)
    label.setColor(yup.Label.Style.backgroundColorId, None)
    assert label.getColor(yup.Label.Style.backgroundColorId) is None


def test_label_style_is_not_constructible():
    # The nested Style struct only holds static members, so it is exposed for its ids only.
    try:
        yup.Label.Style()
    except TypeError:
        pass
    else:
        raise AssertionError("Label.Style should not be constructible")
