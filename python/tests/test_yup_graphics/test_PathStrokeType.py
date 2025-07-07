from math import isclose
import yup

"""
#==================================================================================================

def test_constructor():
    p1 = yup.PathStrokeType(1.0)
    assert isclose(p1.getStrokeThickness(), 1.0)
    assert p1.getJointStyle() == yup.PathStrokeType.mitered
    assert p1.getEndStyle() == yup.PathStrokeType.butt

    p2 = yup.PathStrokeType(2.0, yup.PathStrokeType.beveled)
    assert isclose(p2.getStrokeThickness(), 2.0)
    assert p2.getJointStyle() == yup.PathStrokeType.beveled
    assert p2.getEndStyle() == yup.PathStrokeType.butt

    p3 = yup.PathStrokeType(3.0, yup.PathStrokeType.beveled, yup.PathStrokeType.square)
    assert isclose(p3.getStrokeThickness(), 3.0)
    assert p3.getJointStyle() == yup.PathStrokeType.beveled
    assert p3.getEndStyle() == yup.PathStrokeType.square

    x1 = yup.PathStrokeType(p1)
    assert x1 == p1
    assert x1 != p2

    x2 = yup.PathStrokeType(p2)
    assert x2 == p2
    assert x2 != p3

    x3 = yup.PathStrokeType(p3)
    assert x3 == p3
    assert x3 != p1

#==================================================================================================

def test_stroke_tickness():
    p = yup.PathStrokeType(1.0)
    p.setStrokeThickness(2.5)
    assert isclose(p.getStrokeThickness(), 2.5)

#==================================================================================================

def test_joint_style():
    p = yup.PathStrokeType(1.0)
    p.setJointStyle(yup.PathStrokeType.beveled)
    assert p.getJointStyle() == yup.PathStrokeType.beveled

#==================================================================================================

def test_end_style():
    p = yup.PathStrokeType(1.0)
    p.setEndStyle(yup.PathStrokeType.square)
    assert p.getEndStyle() == yup.PathStrokeType.square

#==================================================================================================

def test_create_stroked_path():
    source_path = yup.Path()
    source_path.addRectangle(0, 0, 100, 100)
    assert source_path.toString() == "m 0 100 l 0 0 100 0 100 100 z"

    dest_path = yup.Path()
    p = yup.PathStrokeType(1.0)
    p.createStrokedPath(dest_path, source_path)
    assert dest_path.toString() == "m -0.5 100 l -0.5 -0.5 100.5 -0.5 100.5 100.5 -0.5 100.5 z m 0 99.5 l 99.5 99.5 99.5 0.5 0.5 0.5 0.5 99.5 z"

    p = yup.PathStrokeType(0.1)
    p.createStrokedPath(dest_path, source_path)
    assert dest_path.toString() == "m -0.05 100 l -0.05 -0.05 100.05 -0.05 100.05 100.05 -0.05 100.05 z m 0 99.95 l 99.95 99.95 99.95 0.05 0.05 0.05 0.05 99.95 z"

    dest_path = yup.Path()
    p = yup.PathStrokeType(10.0)
    p.createStrokedPath(dest_path, source_path, yup.AffineTransform.translation(-50, -50), 10.0)
    assert dest_path.toString() == "m -55 50 l -55 -55 55 -55 55 55 -55 55 z m -50 45 l 45 45 45 -45 -45 -45 -45 45 z"
"""
