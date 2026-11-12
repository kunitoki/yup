import pytest

import yup

#==================================================================================================

def test_construct_default():
    e = yup.EdgeSpacing[int]()
    assert e.getLeft() == 0
    assert e.getTop() == 0
    assert e.getRight() == 0
    assert e.getBottom() == 0
    assert e.isZero()
    assert e.isUniform()

def test_construct_uniform():
    e = yup.EdgeSpacing[int](5)
    assert e.getLeft() == 5
    assert e.getTop() == 5
    assert e.getRight() == 5
    assert e.getBottom() == 5
    assert e.isUniform()

def test_construct_horizontal_vertical():
    e = yup.EdgeSpacing[int](8, 4)
    assert e.getLeft() == 8
    assert e.getRight() == 8
    assert e.getTop() == 4
    assert e.getBottom() == 4

def test_construct_four_edges():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    assert e.getLeft() == 1
    assert e.getTop() == 2
    assert e.getRight() == 3
    assert e.getBottom() == 4

def test_construct_copy():
    e1 = yup.EdgeSpacing[int](1, 2, 3, 4)
    e2 = yup.EdgeSpacing[int](e1)
    assert e1 == e2

#==================================================================================================

def test_totals():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    assert e.getHorizontal() == 4
    assert e.getVertical() == 6

def test_withers():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    assert e.withLeft(10) == yup.EdgeSpacing[int](10, 2, 3, 4)
    assert e.withTop(10) == yup.EdgeSpacing[int](1, 10, 3, 4)
    assert e.withRight(10) == yup.EdgeSpacing[int](1, 2, 10, 4)
    assert e.withBottom(10) == yup.EdgeSpacing[int](1, 2, 3, 10)

#==================================================================================================

def test_subtracted_from_rectangle():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    r = yup.Rectangle[int](0, 0, 100, 100)
    assert e.subtractedFrom(r) == yup.Rectangle[int](1, 2, 96, 94)

def test_added_to_rectangle():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    r = yup.Rectangle[int](10, 10, 100, 100)
    assert e.addedTo(r) == yup.Rectangle[int](9, 8, 104, 106)

def test_rectangle_reduced_overload():
    e = yup.EdgeSpacing[int](10, 10, 10, 10)
    r = yup.Rectangle[int](0, 0, 100, 100)
    assert r.reduced(e) == yup.Rectangle[int](10, 10, 80, 80)

def test_rectangle_enlarged_overload():
    e = yup.EdgeSpacing[int](5, 5, 5, 5)
    r = yup.Rectangle[int](10, 10, 100, 100)
    assert r.enlarged(e) == yup.Rectangle[int](5, 5, 110, 110)

#==================================================================================================

def test_arithmetic():
    a = yup.EdgeSpacing[int](1, 2, 3, 4)
    b = yup.EdgeSpacing[int](10, 20, 30, 40)
    assert a + b == yup.EdgeSpacing[int](11, 22, 33, 44)
    assert b - a == yup.EdgeSpacing[int](9, 18, 27, 36)
    assert a * 2 == yup.EdgeSpacing[int](2, 4, 6, 8)

def test_negation():
    e = yup.EdgeSpacing[int](1, -2, 3, -4)
    assert -e == yup.EdgeSpacing[int](-1, 2, -3, 4)

#==================================================================================================

def test_string():
    e = yup.EdgeSpacing[int](1, 2, 3, 4)
    assert e.toString() == "1, 2, 3, 4"

def test_equality():
    a = yup.EdgeSpacing[int](1, 2, 3, 4)
    b = yup.EdgeSpacing[int](1, 2, 3, 4)
    c = yup.EdgeSpacing[int](1, 2, 3, 5)
    assert a == b
    assert a != c
