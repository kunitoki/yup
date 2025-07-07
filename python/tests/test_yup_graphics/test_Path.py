import yup

"""
#==================================================================================================

def make_box_path(size: float) -> yup.Path:
    p = yup.Path()
    p.startNewSubPath(0.0, 0.0)
    p.lineTo(size, 0.0)
    p.lineTo(size, size)
    p.lineTo(0.0, size)
    p.closeSubPath()
    return p

#==================================================================================================

def test_constructor():
    p = yup.Path()
    assert p.isEmpty()
    assert p == p

#==================================================================================================

def test_copy_constructor():
    p = yup.Path()
    p.lineTo(10.0, 10.0)
    assert not p.isEmpty()
    assert p != yup.Path()

    x = yup.Path(p)
    assert not x.isEmpty()
    assert x != yup.Path()
    assert x == p

#==================================================================================================

def test_get_bounds():
    p = yup.Path()
    assert p.getBounds().isEmpty()

    p.lineTo(10.0, 10.0)
    assert not p.getBounds().isEmpty()
    assert p.getBounds().toNearestInt() == yup.Rectangle[int](0, 0, 10, 10)

#==================================================================================================

def test_get_bounds_transformed():
    p = yup.Path()
    p.lineTo(10.0, 10.0)
    assert not p.getBounds().isEmpty()

    t = yup.AffineTransform.translation(10.0, 10.0)
    assert p.getBoundsTransformed(t).toNearestInt() == yup.Rectangle[int](10, 10, 10, 10)

#==================================================================================================

def test_contains():
    p = make_box_path(10.0)

    assert p.contains(5, 5)
    assert p.contains(5, 5, 0.001)
    assert not p.contains(15, 5)
    assert not p.contains(10.1, 5.0)
    assert p.contains(9.0, 5.0)

    assert p.contains(yup.Point[float](5, 5))
    assert p.contains(yup.Point[float](5, 5), 0.001)
    assert not p.contains(yup.Point[float](15, 5.0))
    assert not p.contains(yup.Point[float](10.1, 5.0))
    assert p.contains(yup.Point[float](9.0, 5.0))

#==================================================================================================

def test_intersects_line():
    p = make_box_path(10.0)

    assert p.intersectsLine(yup.Line[float](-10, -10, 20, 20))
    assert not p.intersectsLine(yup.Line[float](-10, -10, -10, 20))

#==================================================================================================

def test_round_trips():
    p = make_box_path(10.0)

    x = yup.Path(str(p))
    assert x == p
"""
