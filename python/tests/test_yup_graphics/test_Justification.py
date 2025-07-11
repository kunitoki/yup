import yup

"""
#==================================================================================================

def test_constructor():
    b = yup.Justification(yup.Justification.centred)
    assert b.getFlags() == yup.Justification.centred

    b = yup.Justification(yup.Justification.verticallyCentred | yup.Justification.horizontallyJustified)
    assert b.getFlags() == yup.Justification.verticallyCentred | yup.Justification.horizontallyJustified
    assert b.testFlags(yup.Justification.verticallyCentred)
    assert b.testFlags(yup.Justification.horizontallyJustified)
    assert b.testFlags(yup.Justification.centred)
    assert not b.testFlags(yup.Justification.left)
    assert not b.testFlags(yup.Justification.right)

#==================================================================================================

def test_comparison():
    b = yup.Justification(yup.Justification.centred)
    c = yup.Justification(yup.Justification.left)
    assert b == yup.Justification.centred
    assert b != yup.Justification.left
    assert c != yup.Justification.centred
    assert c == yup.Justification.left
    assert b == b
    assert b != c

#==================================================================================================

def test_get_only_flags():
    b = yup.Justification(yup.Justification.verticallyCentred | yup.Justification.horizontallyJustified)
    assert b.getOnlyVerticalFlags() == yup.Justification.verticallyCentred
    assert b.getOnlyHorizontalFlags() == yup.Justification.horizontallyJustified

#==================================================================================================

def test_apply_to_rectangle_int():
    b = yup.Justification(yup.Justification.left)

    outer = yup.Rectangle[int](10, 10, 100, 100)
    inner = yup.Rectangle[int](20, 20, 20, 20)

    result = b.appliedToRectangle(inner, outer)
    assert result == yup.Rectangle[int](10, 10, 20, 20)

    result = b.appliedToRectangle(outer, inner)
    assert result == yup.Rectangle[int](20, 20, 100, 100)

#==================================================================================================

def test_apply_to_rectangle_float():
    b = yup.Justification(yup.Justification.bottomRight)

    outer = yup.Rectangle[float](10, 10, 100, 100)
    inner = yup.Rectangle[float](20, 20, 20, 20)

    result = b.appliedToRectangle(inner, outer)
    assert result == yup.Rectangle[float](90, 90, 20, 20)

    result = b.appliedToRectangle(outer, inner)
    assert result == yup.Rectangle[float](-60, -60, 100, 100)
"""
