import math
from math import isclose

import yup

"""
#==================================================================================================

def test_construct_empty():
    a = yup.AffineTransform()
    assert a.isIdentity()

#==================================================================================================

def test_is_singularity():
    a = yup.AffineTransform(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
    assert a.isSingularity()

#==================================================================================================

def test_construct_with_values():
    a = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    assert not a.isIdentity()
    assert isclose(a.mat00, 1.0)
    assert isclose(a.mat01, 2.0)
    assert isclose(a.mat02, 3.0)
    assert isclose(a.mat10, 4.0)
    assert isclose(a.mat11, 5.0)
    assert isclose(a.mat12, 6.0)

#==================================================================================================

def test_copy_constructor():
    original = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    copied = yup.AffineTransform(original)
    assert original == copied

#==================================================================================================

def test_assignment_operator():
    original = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    assigned = yup.AffineTransform()
    assigned = original
    assert original == assigned

#==================================================================================================

def test_equality():
    a = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    b = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    assert a == b

#==================================================================================================

def test_inequality():
    a = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    b = yup.AffineTransform(6.0, 5.0, 4.0, 3.0, 2.0, 1.0)
    assert a != b

#==================================================================================================

def test_transform_point():
    a = yup.AffineTransform(2.0, 0.0, 1.0, 0.0, 2.0, 2.0)
    x, y = a.transformPoint(1.0, 1.0)
    assert isclose(x, 3.0)
    assert isclose(y, 4.0)

#==================================================================================================

def test_transform_points_two_points():
    a = yup.AffineTransform(2.0, 0.0, 1.0, 0.0, 2.0, 2.0)
    x1, y1, x2, y2 = a.transformPoints(1.0, 1.0, 2.0, 2.0)
    assert isclose(x1, 3.0)
    assert isclose(y1, 4.0)
    assert isclose(x2, 5.0)
    assert isclose(y2, 6.0)

#==================================================================================================

def test_transform_points_three_points():
    a = yup.AffineTransform(2.0, 0.0, 1.0, 0.0, 2.0, 2.0)
    x1, y1, x2, y2, x3, y3 = a.transformPoints(1.0, 1.0, 2.0, 2.0, 3.0, 3.0)
    assert isclose(x1, 3.0)
    assert isclose(y1, 4.0)
    assert isclose(x2, 5.0)
    assert isclose(y2, 6.0)
    assert isclose(x3, 7.0)
    assert isclose(y3, 8.0)

#==================================================================================================

def test_translated():
    a = yup.AffineTransform(1.0, 0.0, 1.0, 0.0, 1.0, 1.0)
    b = a.translated(2.0, 3.0)
    assert isclose(b.getTranslationX(), 3.0)
    assert isclose(b.getTranslationY(), 4.0)

#==================================================================================================

def test_translation_static():
    a = yup.AffineTransform.translation(2.0, 3.0)
    assert isclose(a.getTranslationX(), 2.0)
    assert isclose(a.getTranslationY(), 3.0)

#==================================================================================================

def test_rotated():
    a = yup.AffineTransform.rotation(1.5708) # 90 degrees in radians
    assert not a.isOnlyTranslation()

#==================================================================================================

def test_scaled():
    a = yup.AffineTransform.scale(2.0, 3.0)
    assert isclose(a.mat00, 2.0)
    assert isclose(a.mat11, 3.0)

#==================================================================================================

def test_sheared():
    a = yup.AffineTransform.shear(1.0, 2.0)
    assert isclose(a.mat01, 1.0)
    assert isclose(a.mat10, 2.0)

#==================================================================================================

def test_inverted():
    a = yup.AffineTransform().scaled(2.0, 2.0)
    b = a.inverted()
    assert a.followedBy(b).isIdentity()

#==================================================================================================

def test_from_target_points():
    a = yup.AffineTransform.fromTargetPoints(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    assert a.isSingularity()  # Assuming these points result in a singular matrix

#==================================================================================================

def test_followed_by():
    a = yup.AffineTransform(1.0, 2.0, 3.0, 4.0, 5.0, 6.0)
    b = yup.AffineTransform(2.0, 0.0, 1.0, 0.0, 2.0, 2.0)
    c = a.followedBy(b)
    assert isclose(c.mat00, 2.0)
    assert isclose(c.mat01, 4.0)
    assert isclose(c.mat02, 7.0)
    assert isclose(c.mat10, 8.0)
    assert isclose(c.mat11, 10.0)
    assert isclose(c.mat12, 14.0)

#==================================================================================================

def test_is_only_translation():
    a = yup.AffineTransform.translation(2.0, 3.0)
    assert a.isOnlyTranslation()

#==================================================================================================

def test_get_translation_x():
    a = yup.AffineTransform.translation(2.0, 3.0)
    assert isclose(a.getTranslationX(), 2.0)

#==================================================================================================

def test_get_translation_y():
    a = yup.AffineTransform.translation(2.0, 3.0)
    assert isclose(a.getTranslationY(), 3.0)

#==================================================================================================

def test_get_determinant():
    a = yup.AffineTransform(2.0, 1.0, 3.0, 0.0, -1.0, 2.0)
    assert isclose(a.getDeterminant(), -2.0)

#==================================================================================================

def test_vertical_flip():
    height = 5.0
    a = yup.AffineTransform.verticalFlip(height)
    assert isclose(a.mat11, -1.0)
    assert isclose(a.mat12, height)

#==================================================================================================

def test_transform_points_flip():
    a = yup.AffineTransform(-1, 0, 0, 0, -1, 0)
    assert not a.isIdentity()
    assert isclose(a.mat00, -1.0)
    assert isclose(a.mat01, 0.0)
    assert isclose(a.mat02, 0.0)
    assert isclose(a.mat10, 0.0)
    assert isclose(a.mat11, -1.0)
    assert isclose(a.mat12, 0.0)

    x, y = a.transformPoint(2, 1)
    assert x == -2
    assert y == -1

    x, y = a.transformPoint(2.1, 1.1)
    assert isclose(x, -2.1, rel_tol=1e-06)
    assert isclose(y, -1.1, rel_tol=1e-06)

    x, y = a.transformPoint(-100, -1)
    assert x == 100
    assert y == 1

    x, y = a.transformPoint(-100.99, -1.99)
    assert isclose(x, 100.99, rel_tol=1e-06)
    assert isclose(y, 1.99, rel_tol=1e-06)

#==================================================================================================

def test_transform_translate():
    a = yup.AffineTransform().translation(111.0, 111.0)
    assert a.isOnlyTranslation()
    assert isclose(a.getTranslationX(), 111.0)
    assert isclose(a.getTranslationY(), 111.0)

    a = yup.AffineTransform().scale(0.5)
    assert not a.isOnlyTranslation()
    assert isclose(a.getTranslationX(), 0.0)
    assert isclose(a.getTranslationY(), 0.0)

#==================================================================================================

def test_legacy():
    scale1 = 1.5
    scale2 = 1.3

    transform = yup.AffineTransform.scale(scale1) \
        .followedBy(yup.AffineTransform.rotation(yup.degreesToRadians(72.0))) \
        .followedBy(yup.AffineTransform.translation(100.0, 20.0)) \
        .followedBy(yup.AffineTransform.scale(scale2))

    assert yup.approximatelyEqual(math.sqrt(math.fabs(transform.getDeterminant())), scale1 * scale2)
"""
