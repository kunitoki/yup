from math import isclose

import yup

#==================================================================================================

def _point(x, y):
    return yup.Point[float](float(x), float(y))

#==================================================================================================

def test_default_construction_is_linear():
    easing = yup.Easing()

    assert easing.isLinear()
    assert not easing.isHold()
    assert easing == yup.Easing.linear()

#==================================================================================================

def test_control_points_round_trip():
    easing = yup.Easing(0.25, 0.1, 0.75, 0.9)

    assert isclose(easing.getX1(), 0.25, abs_tol=1e-6)
    assert isclose(easing.getY1(), 0.1, abs_tol=1e-6)
    assert isclose(easing.getX2(), 0.75, abs_tol=1e-6)
    assert isclose(easing.getY2(), 0.9, abs_tol=1e-6)

#==================================================================================================

def test_curve_spans_the_unit_square():
    curve = yup.Easing(0.25, 0.1, 0.75, 0.9).getCurve()

    assert curve.p0() == _point(0, 0)
    assert curve.p3() == _point(1, 1)
    assert isclose(curve.p1().getX(), 0.25, abs_tol=1e-6)
    assert isclose(curve.p2().getY(), 0.9, abs_tol=1e-6)

#==================================================================================================

def test_evaluate_follows_the_curve():
    easing = yup.Easing.easeInOut()
    curve = easing.getCurve()

    for s in (0.1, 0.3, 0.5, 0.7, 0.9):
        point = curve.pointAt(s)
        assert isclose(easing.evaluate(point.getX()), point.getY(), abs_tol=1e-4)

#==================================================================================================

def test_evaluate_clamps_to_the_unit_range():
    easing = yup.Easing.easeIn()

    assert easing.evaluate(-1.0) == 0.0
    assert easing.evaluate(2.0) == 1.0

#==================================================================================================

def test_hold_stays_at_the_start():
    easing = yup.Easing.hold()

    assert easing.isHold()
    assert easing.evaluate(0.5) == 0.0

#==================================================================================================

def test_from_tangents_matches_the_constructor():
    easing = yup.Easing.fromTangents(_point(0.42, 0), _point(0.58, 1))

    assert easing == yup.Easing.easeInOut()
    assert easing != yup.Easing.easeIn()
