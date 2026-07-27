import pytest

import yup

#==================================================================================================

def test_construct_default():
    s = yup.Size[int]()
    assert s.getWidth() == 0
    assert s.getHeight() == 0

def test_construct_with_dimensions():
    s = yup.Size[int](10, 20)
    assert s.getWidth() == 10
    assert s.getHeight() == 20

def test_construct_copy():
    s1 = yup.Size[int](15, 25)
    s2 = yup.Size[int](s1)
    assert s1 == s2
    assert s1.getWidth() == s2.getWidth()
    assert s1.getHeight() == s2.getHeight()

#==================================================================================================

def test_width_accessors():
    s = yup.Size[int](10, 20)

    # Test getter
    assert s.getWidth() == 10

    # Test setter
    s.setWidth(30)
    assert s.getWidth() == 30
    assert s.getHeight() == 20  # Height unchanged

    # Test withWidth
    s2 = s.withWidth(40)
    assert s2.getWidth() == 40
    assert s2.getHeight() == 20
    assert s.getWidth() == 30  # Original unchanged

def test_height_accessors():
    s = yup.Size[float](10, 20)

    # Test getter
    assert s.getHeight() == 20

    # Test setter
    s.setHeight(30)
    assert s.getHeight() == 30
    assert s.getWidth() == 10  # Width unchanged

    # Test withHeight
    s2 = s.withHeight(40)
    assert s2.getHeight() == 40
    assert s2.getWidth() == 10
    assert s.getHeight() == 30  # Original unchanged

#==================================================================================================

def test_state_checking():
    # Test isZero
    zero_size = yup.Size[float](0, 0)
    assert zero_size.isZero()

    non_zero_size = yup.Size[float](10, 20)
    assert not non_zero_size.isZero()

    # Test isEmpty
    empty_width = yup.Size[float](0, 20)
    assert empty_width.isEmpty()

    empty_height = yup.Size[float](10, 0)
    assert empty_height.isEmpty()

    assert zero_size.isEmpty()
    assert not non_zero_size.isEmpty()

    # Test isVerticallyEmpty
    assert not empty_width.isVerticallyEmpty()
    assert empty_height.isVerticallyEmpty()
    assert not zero_size.isVerticallyEmpty()
    assert not non_zero_size.isVerticallyEmpty()

    # Test isHorizontallyEmpty
    assert empty_width.isHorizontallyEmpty()
    assert not empty_height.isHorizontallyEmpty()
    assert not zero_size.isHorizontallyEmpty()
    assert not non_zero_size.isHorizontallyEmpty()

    # Test isSquare
    square_size = yup.Size[float](10, 10)
    assert square_size.isSquare()
    assert not non_zero_size.isSquare()

#==================================================================================================

def test_utility_methods():
    s = yup.Size[int](10, 20)

    # Test area
    assert s.area() == 200

    # Test reverse
    original_width = s.getWidth()
    original_height = s.getHeight()
    s.reverse()
    assert s.getWidth() == original_height
    assert s.getHeight() == original_width

    # Test reversed
    s2 = yup.Size[int](30, 40)
    s3 = s2.reversed()
    assert s3.getWidth() == 40
    assert s3.getHeight() == 30
    assert s2.getWidth() == 30  # Original unchanged
    assert s2.getHeight() == 40

#==================================================================================================

def test_enlarge_methods():
    s = yup.Size[int](10, 20)

    # Test enlarge uniform
    s.enlarge(5)
    assert s.getWidth() == 15
    assert s.getHeight() == 25

    # Test enlarge separate
    s.enlarge(2, 3)
    assert s.getWidth() == 17
    assert s.getHeight() == 28

    # Test enlarged uniform
    s2 = yup.Size[int](10, 20)
    s3 = s2.enlarged(5)
    assert s3.getWidth() == 15
    assert s3.getHeight() == 25
    assert s2.getWidth() == 10  # Original unchanged
    assert s2.getHeight() == 20

    # Test enlarged separate
    s4 = s2.enlarged(2, 3)
    assert s4.getWidth() == 12
    assert s4.getHeight() == 23
    assert s2.getWidth() == 10  # Original unchanged
    assert s2.getHeight() == 20

#==================================================================================================

def test_reduce_methods():
    s = yup.Size[int](20, 30)

    # Test reduce uniform
    s.reduce(5)
    assert s.getWidth() == 15
    assert s.getHeight() == 25

    # Test reduce separate
    s.reduce(2, 3)
    assert s.getWidth() == 13
    assert s.getHeight() == 22

    # Test reduced uniform
    s2 = yup.Size[int](20, 30)
    s3 = s2.reduced(5)
    assert s3.getWidth() == 15
    assert s3.getHeight() == 25
    assert s2.getWidth() == 20  # Original unchanged
    assert s2.getHeight() == 30

    # Test reduced separate
    s4 = s2.reduced(2, 3)
    assert s4.getWidth() == 18
    assert s4.getHeight() == 27
    assert s2.getWidth() == 20  # Original unchanged
    assert s2.getHeight() == 30

#==================================================================================================

def test_scale_methods():
    s = yup.Size[float](10.0, 20.0)

    # Test scale uniform
    s.scale(2.0)
    assert s.getWidth() == pytest.approx(20.0)
    assert s.getHeight() == pytest.approx(40.0)

    # Test scale separate
    s.scale(0.5, 0.25)
    assert s.getWidth() == pytest.approx(10.0)
    assert s.getHeight() == pytest.approx(10.0)

    # Test scaled uniform
    s2 = yup.Size[float](10.0, 20.0)
    s3 = s2.scaled(2.0)
    assert s3.getWidth() == pytest.approx(20.0)
    assert s3.getHeight() == pytest.approx(40.0)
    assert s2.getWidth() == pytest.approx(10.0)  # Original unchanged
    assert s2.getHeight() == pytest.approx(20.0)

    # Test scaled separate
    s4 = s2.scaled(0.5, 0.25)
    assert s4.getWidth() == pytest.approx(5.0)
    assert s4.getHeight() == pytest.approx(5.0)
    assert s2.getWidth() == pytest.approx(10.0)  # Original unchanged
    assert s2.getHeight() == pytest.approx(20.0)

#==================================================================================================

def test_conversion_methods():
    s = yup.Size[int](10, 20)

    # Test type conversions
    s_int = s.toInt()
    assert isinstance(s_int, yup.Size[int])
    assert s_int.getWidth() == 10
    assert s_int.getHeight() == 20

    s_float = s.toFloat()
    assert isinstance(s_float, yup.Size[float])
    assert s_float.getWidth() == pytest.approx(10.0)
    assert s_float.getHeight() == pytest.approx(20.0)

    # Test toPoint
    point = s.toPoint()
    assert isinstance(point, yup.Point[int])
    assert point.getX() == 10
    assert point.getY() == 20

    # Test toRectangle
    rect = s.toRectangle()
    assert isinstance(rect, yup.Rectangle[int])
    assert rect.getX() == 0
    assert rect.getY() == 0
    assert rect.getWidth() == 10
    assert rect.getHeight() == 20

    # Test toRectangle with coordinates
    rect2 = s.toRectangle(5, 7)
    assert isinstance(rect2, yup.Rectangle[int])
    assert rect2.getX() == 5
    assert rect2.getY() == 7
    assert rect2.getWidth() == 10
    assert rect2.getHeight() == 20

    # Test toRectangle with Point
    point_pos = yup.Point[int](3, 4)
    rect3 = s.toRectangle(point_pos)
    assert isinstance(rect3, yup.Rectangle[int])
    assert rect3.getX() == 3
    assert rect3.getY() == 4
    assert rect3.getWidth() == 10
    assert rect3.getHeight() == 20

#==================================================================================================

def test_floating_point_specific():
    s = yup.Size[float](10.4, 20.7)

    # Test roundToInt
    s_rounded = s.roundToInt()
    assert isinstance(s_rounded, yup.Size[int])
    assert s_rounded.getWidth() == 10
    assert s_rounded.getHeight() == 21

    # Test toNearestInt
    s_nearest = s.toNearestInt()
    assert isinstance(s_nearest, yup.Size[float])
    assert s_nearest.getWidth() == 10
    assert s_nearest.getHeight() == 21

#==================================================================================================

def test_string_conversion():
    s = yup.Size[int](10, 20)

    # Test toString
    str_repr = s.toString()
    assert isinstance(str_repr, str)
    assert "10" in str_repr
    assert "20" in str_repr

#==================================================================================================

def test_comparison():
    s1 = yup.Size[int](10, 20)
    s2 = yup.Size[int](10, 20)
    s3 = yup.Size[int](15, 25)

    # Test equality
    assert s1 == s2
    assert not (s1 == s3)

    # Test inequality
    assert s1 != s3
    assert not (s1 != s2)

    # Test approximatelyEqualTo
    assert s1.approximatelyEqualTo(s2)
    assert not s1.approximatelyEqualTo(s3)

#==================================================================================================

def test_operators():
    s = yup.Size[int](10, 20)

    # Test multiplication
    s2 = s * 2
    assert s2.getWidth() == 20
    assert s2.getHeight() == 40
    assert s.getWidth() == 10  # Original unchanged
    assert s.getHeight() == 20

    # Test multiplication assignment
    s *= 2
    assert s.getWidth() == 20
    assert s.getHeight() == 40

    # Test division
    s3 = s / 2
    assert s3.getWidth() == 10
    assert s3.getHeight() == 20
    assert s.getWidth() == 20  # Original unchanged
    assert s.getHeight() == 40

    # Test division assignment
    s /= 2
    assert s.getWidth() == 10
    assert s.getHeight() == 20

#==================================================================================================

def test_repr_and_str():
    s = yup.Size[int](10, 20)

    # Test __repr__
    repr_str = repr(s)
    assert isinstance(repr_str, str)
    assert "Size" in repr_str
    assert "10" in repr_str
    assert "20" in repr_str

    # Test __str__
    str_str = str(s)
    assert isinstance(str_str, str)
    assert "10" in str_str
    assert "20" in str_str