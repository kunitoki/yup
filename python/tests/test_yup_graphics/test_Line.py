from math import isclose
import yup

"""
#==================================================================================================

def test_empty_constructor():
    p = yup.Line[int]()
    assert p.getStartX() == 0
    assert p.getStartY() == 0
    assert p.getEndX() == 0
    assert p.getEndY() == 0

    p = yup.Line[float]()
    assert isclose(p.getStartX(), 0)
    assert isclose(p.getStartY(), 0)
    assert isclose(p.getEndX(), 0)
    assert isclose(p.getEndY(), 0)

#==================================================================================================

def test_values_constructor():
    p = yup.Line[int](1, 1, 10, 10)
    assert p.getStartX() == 1
    assert p.getStartY() == 1
    assert p.getEndX() == 10
    assert p.getEndY() == 10

    p = yup.Line[float](1, 1, 10, 10)
    assert isclose(p.getStartX(), 1)
    assert isclose(p.getStartY(), 1)
    assert isclose(p.getEndX(), 10)
    assert isclose(p.getEndY(), 10)

#==================================================================================================

def test_points_constructor():
    p = yup.Line[int](yup.Point[int](1, 1), yup.Point[int](10, 10))
    assert p.getStartX() == 1
    assert p.getStartY() == 1
    assert p.getEndX() == 10
    assert p.getEndY() == 10

    p = yup.Line[float](yup.Point[float](1, 1), yup.Point[float](10, 10))
    assert isclose(p.getStartX(), 1)
    assert isclose(p.getStartY(), 1)
    assert isclose(p.getEndX(), 10)
    assert isclose(p.getEndY(), 10)

#==================================================================================================

def test_copy_constructor():
    a = yup.Line[int](yup.Point[int](1, 1), yup.Point[int](10, 10))
    p = yup.Line[int](a)
    assert p.getStartX() == 1
    assert p.getStartY() == 1
    assert p.getEndX() == 10
    assert p.getEndY() == 10

    a = yup.Line[float](yup.Point[float](1, 1), yup.Point[float](10, 10))
    p = yup.Line[float](a)
    assert isclose(p.getStartX(), 1)
    assert isclose(p.getStartY(), 1)
    assert isclose(p.getEndX(), 10)
    assert isclose(p.getEndY(), 10)

#==================================================================================================

def test_get_start_end():
    p = yup.Line[int](1, 1, 10, 10)
    assert p.getStart().x == 1
    assert p.getStart().y == 1
    assert p.getEnd().x == 10
    assert p.getEnd().y == 10

    p = yup.Line[float](1, 1, 10, 10)
    assert isclose(p.getStart().x, 1)
    assert isclose(p.getStart().y, 1)
    assert isclose(p.getEnd().x, 10)
    assert isclose(p.getEnd().y, 10)

#==================================================================================================

def test_set_start_end_values():
    p = yup.Line[int]()
    p.setStart(1, 1)
    p.setEnd(10, 10)
    assert p.getStartX() == 1
    assert p.getStartY() == 1
    assert p.getEndX() == 10
    assert p.getEndY() == 10

    p = yup.Line[float]()
    p.setStart(1, 1)
    p.setEnd(10, 10)
    assert isclose(p.getStartX(), 1)
    assert isclose(p.getStartY(), 1)
    assert isclose(p.getEndX(), 10)
    assert isclose(p.getEndY(), 10)

#==================================================================================================

def test_reversed():
    p = yup.Line[int](1, 1, 10, 10)
    p = p.reversed()
    assert p.getStartX() == 10
    assert p.getStartY() == 10
    assert p.getEndX() == 1
    assert p.getEndY() == 1

    p = yup.Line[float](1, 1, 10, 10)
    p = p.reversed()
    assert isclose(p.getStartX(), 10)
    assert isclose(p.getStartY(), 10)
    assert isclose(p.getEndX(), 1)
    assert isclose(p.getEndY(), 1)

#==================================================================================================

def test_apply_transform():
    t = yup.AffineTransform.translation(10, 10)

    p = yup.Line[int](1, 1, 10, 10)
    p.applyTransform(t)
    assert p.getStartX() == 11
    assert p.getStartY() == 11
    assert p.getEndX() == 20
    assert p.getEndY() == 20

    p = yup.Line[float](1, 1, 10, 10)
    p.applyTransform(t)
    assert isclose(p.getStartX(), 11)
    assert isclose(p.getStartY(), 11)
    assert isclose(p.getEndX(), 20)
    assert isclose(p.getEndY(), 20)

#==================================================================================================

def test_length():
    p = yup.Line[int](0, 0, 0, 10)
    assert isclose(p.getLength(), 10)
    assert isclose(p.getLengthSquared(), 100)

    p = yup.Line[int](0, 0, 10, 10)
    assert isclose(p.getLength(), 14)
    assert isclose(p.getLengthSquared(), 200)

    p = yup.Line[float](0, 0, 0, 10)
    assert isclose(p.getLength(), 10)
    assert isclose(p.getLengthSquared(), 100)

    p = yup.Line[float](0, 0, 10, 10)
    assert isclose(p.getLength(), 14.142135620117188)
    assert isclose(p.getLengthSquared(), 200)

#==================================================================================================

def test_vertical_horizontal():
    p = yup.Line[int](0, 0, 0, 10)
    assert p.isVertical()
    assert not p.isHorizontal()

    p = yup.Line[int](0, 0, 10, 0)
    assert not p.isVertical()
    assert p.isHorizontal()

    p = yup.Line[float](0, 0, 0, 10)
    assert p.isVertical()
    assert not p.isHorizontal()

    p = yup.Line[float](0, 0, 10, 0)
    assert not p.isVertical()
    assert p.isHorizontal()

#==================================================================================================

def test_get_angle():
    p = yup.Line[int](0, 0, 0, 10)
    assert isclose(yup.radiansToDegrees(p.getAngle()), 180, rel_tol=1e-06)

    p = yup.Line[int](0, 10, 0, 0)
    assert isclose(yup.radiansToDegrees(p.getAngle()), 0.0, rel_tol=1e-06)

    p = yup.Line[int](10, 10, 0, 0)
    assert isclose(yup.radiansToDegrees(p.getAngle()), -45.0, rel_tol=1e-06)

    p = yup.Line[float](0, 0, 0, 10)
    assert isclose(yup.radiansToDegrees(p.getAngle()), 180, rel_tol=1e-06)

    p = yup.Line[float](0, 10, 0, 0)
    assert isclose(yup.radiansToDegrees(p.getAngle()), 0.0, rel_tol=1e-06)

    p = yup.Line[float](10, 10, 0, 0)
    assert isclose(yup.radiansToDegrees(p.getAngle()), -45.0, rel_tol=1e-06)

#==================================================================================================

def test_to_float():
    p = yup.Line[int](1, 1, 2, 2).toFloat()
    assert isinstance(p.getStart().x, float)
    assert isinstance(p.getStart().y, float)
    assert isinstance(p.getEnd().x, float)
    assert isinstance(p.getEnd().y, float)
    assert isclose(p.getStartX(), 1)
    assert isclose(p.getStartY(), 1)
    assert isclose(p.getEndX(), 2)
    assert isclose(p.getEndY(), 2)
"""
