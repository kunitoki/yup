from math import isclose

import yup

#==================================================================================================

def _point(x, y):
    return yup.Point[float](float(x), float(y))

#==================================================================================================

def _straight_line():
    # A cubic bezier that happens to be the straight line from (0, 0) to (1, 0), with
    # control points at the thirds so the parametrisation is uniform.
    return yup.CubicBezier(_point(0, 0), _point(1 / 3, 0), _point(2 / 3, 0), _point(1, 0))

#==================================================================================================

def test_default_construction_is_all_zeros():
    curve = yup.CubicBezier()

    assert curve.p0() == _point(0, 0)
    assert curve.p3() == _point(0, 0)

#==================================================================================================

def test_control_points_round_trip():
    curve = yup.CubicBezier(_point(0, 0), _point(1, 2), _point(3, 4), _point(5, 6))

    assert curve.p0() == _point(0, 0)
    assert curve.p1() == _point(1, 2)
    assert curve.p2() == _point(3, 4)
    assert curve.p3() == _point(5, 6)

#==================================================================================================

def test_from_points_matches_the_constructor():
    curve = yup.CubicBezier.fromPoints(_point(0, 0), _point(1, 2), _point(3, 4), _point(5, 6))

    assert curve.p1() == _point(1, 2)
    assert curve.p3() == _point(5, 6)

#==================================================================================================

def test_point_at_the_endpoints_is_exact():
    curve = _straight_line()

    assert curve.pointAt(0.0) == _point(0, 0)
    assert curve.pointAt(1.0) == _point(1, 0)

#==================================================================================================

def test_point_at_the_middle():
    curve = _straight_line()

    midpoint = curve.pointAt(0.5)

    assert isclose(midpoint.getX(), 0.5, abs_tol=1e-5)
    assert isclose(midpoint.getY(), 0.0, abs_tol=1e-5)

#==================================================================================================

def test_length_of_a_straight_line_is_its_span():
    curve = _straight_line()

    assert isclose(curve.length(), 1.0, abs_tol=1e-4)

#==================================================================================================

def test_angle_along_a_horizontal_line_is_zero():
    curve = _straight_line()

    assert isclose(curve.angleAt(0.5), 0.0, abs_tol=1e-5)

#==================================================================================================

def test_derivative_is_a_point():
    curve = _straight_line()

    derivative = curve.derivative(0.5)

    # The derivative of the uniform straight line is constant and points along +x.
    assert derivative.getX() > 0.0
    assert isclose(derivative.getY(), 0.0, abs_tol=1e-5)

#==================================================================================================

def test_t_at_length_inverts_length():
    curve = _straight_line()
    total = curve.length()

    assert isclose(curve.tAtLength(total * 0.5, total), 0.5, abs_tol=1e-3)
    assert isclose(curve.tAtLength(total * 0.5), 0.5, abs_tol=1e-3)

#==================================================================================================

def test_split_produces_two_halves_that_meet_in_the_middle():
    curve = _straight_line()

    firstHalf, secondHalf = curve.split()

    assert firstHalf.p0() == curve.p0()
    assert secondHalf.p3() == curve.p3()
    assert isclose(firstHalf.length() + secondHalf.length(), curve.length(), abs_tol=1e-4)

#==================================================================================================

def test_split_at_length():
    curve = _straight_line()

    left, right = curve.splitAtLength(curve.length() * 0.25)

    assert isclose(left.length(), curve.length() * 0.25, abs_tol=1e-3)
    assert isclose(left.length() + right.length(), curve.length(), abs_tol=1e-3)

#==================================================================================================

def test_parameter_split_left_returns_the_left_portion():
    curve = _straight_line()

    left = curve.parameterSplitLeft(0.5)

    # parameterSplitLeft mutates the curve in place, so `curve` is now the right half
    # and the returned curve is the left one.
    assert left.p0() == _point(0, 0)
    assert curve.p3() == _point(1, 0)

#==================================================================================================

def test_on_interval_selects_a_sub_curve():
    curve = _straight_line()

    middle = curve.onInterval(0.25, 0.75)

    assert isclose(middle.pointAt(0.0).getX(), 0.25, abs_tol=1e-5)
    assert isclose(middle.pointAt(1.0).getX(), 0.75, abs_tol=1e-5)
    assert isclose(middle.length(), 0.5, abs_tol=1e-4)
