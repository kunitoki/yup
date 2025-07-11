import yup

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
    assert p.getBounds().toNearestInt().toInt() == yup.Rectangle[int](0, 0, 10, 10)

#==================================================================================================

def test_get_bounds_transformed():
    p = yup.Path()
    p.lineTo(10.0, 10.0)
    assert not p.getBounds().isEmpty()

    t = yup.AffineTransform.translation(10.0, 10.0)
    assert p.getBoundsTransformed(t).toNearestInt().toInt() == yup.Rectangle[int](10, 10, 10, 10)

#==================================================================================================

def test_round_trips():
    p = make_box_path(10.0)

    x = yup.Path()
    x.fromString(str(p))
    assert x.toString() == p.toString()

#==================================================================================================

def test_isEmpty():
    p = yup.Path()
    assert p.isEmpty()
    assert not bool(p)

    p.lineTo(10.0, 10.0)
    assert not p.isEmpty()
    assert bool(p)

    p.clear()
    assert p.isEmpty()
    assert not bool(p)

#==================================================================================================

def test_sub_path_operations():
    p = yup.Path()

    # Test startNewSubPath
    p.startNewSubPath(0.0, 0.0)
    p.lineTo(10.0, 10.0)
    assert not p.isEmpty()

    # Test with Point parameter
    p.startNewSubPath(yup.Point[float](5.0, 5.0))
    p.lineTo(15.0, 15.0)

    # Test closeSubPath
    p.closeSubPath()
    assert p.isExplicitlyClosed()

#==================================================================================================

def test_closed_path_detection():
    p = yup.Path()
    p.moveTo(0.0, 0.0)
    p.lineTo(10.0, 0.0)
    p.lineTo(10.0, 10.0)
    p.lineTo(0.0, 10.0)
    p.lineTo(0.0, 0.0)

    # Not explicitly closed but geometrically closed
    assert not p.isExplicitlyClosed()
    assert p.isClosed(0.001)

    # Explicitly close it
    p.close()
    assert p.isExplicitlyClosed()

#==================================================================================================

def test_add_triangle():
    p = yup.Path()

    # Test with coordinates
    p.addTriangle(0.0, 0.0, 10.0, 0.0, 5.0, 10.0)
    assert not p.isEmpty()

    # Test with Point objects
    p2 = yup.Path()
    p2.addTriangle(yup.Point[float](0.0, 0.0), yup.Point[float](10.0, 0.0), yup.Point[float](5.0, 10.0))
    assert not p2.isEmpty()

#==================================================================================================

def test_path_verb_enum():
    # Test that the enum values exist and are accessible
    assert hasattr(yup.Path, 'Verb')
    assert hasattr(yup.Path.Verb, 'MoveTo')
    assert hasattr(yup.Path.Verb, 'LineTo')
    assert hasattr(yup.Path.Verb, 'QuadTo')
    assert hasattr(yup.Path.Verb, 'CubicTo')
    assert hasattr(yup.Path.Verb, 'Close')

#==================================================================================================

def test_path_segment():
    # Test creating segments
    point = yup.Point[float](10.0, 20.0)
    control1 = yup.Point[float](5.0, 5.0)
    control2 = yup.Point[float](15.0, 15.0)

    # Test MoveTo/LineTo segment
    segment1 = yup.Path.Segment(yup.Path.Verb.MoveTo, point)
    assert segment1.verb == yup.Path.Verb.MoveTo
    assert segment1.point == point

    # Test QuadTo segment
    segment2 = yup.Path.Segment(yup.Path.Verb.QuadTo, point, control1)
    assert segment2.verb == yup.Path.Verb.QuadTo
    assert segment2.point == point
    assert segment2.controlPoint1 == control1

    # Test CubicTo segment
    segment3 = yup.Path.Segment(yup.Path.Verb.CubicTo, point, control1, control2)
    assert segment3.verb == yup.Path.Verb.CubicTo
    assert segment3.point == point
    assert segment3.controlPoint1 == control1
    assert segment3.controlPoint2 == control2

    # Test Close segment
    close_segment = yup.Path.Segment.close()
    assert close_segment.verb == yup.Path.Verb.Close

#==================================================================================================

def test_path_iteration():
    p = yup.Path()
    p.moveTo(0.0, 0.0)
    p.lineTo(10.0, 10.0)
    p.close()

    # Test that we can iterate over the path
    segments = list(p)
    assert len(segments) == 3  # moveTo, lineTo, close

    # Test first segment
    assert segments[0].verb == yup.Path.Verb.MoveTo
    assert segments[0].point == yup.Point[float](0.0, 0.0)

    # Test second segment
    assert segments[1].verb == yup.Path.Verb.LineTo
    assert segments[1].point == yup.Point[float](10.0, 10.0)

    # Test third segment
    assert segments[2].verb == yup.Path.Verb.Close
