import pytest
import yup

"""
#==================================================================================================

def test_rectangle_list_default_constructor():
    rect_list = yup.RectangleList[int]()
    assert rect_list.isEmpty() == True
    assert rect_list.getNumRectangles() == 0

#==================================================================================================

def test_rectangle_list_copy_constructor():
    original = yup.RectangleList[int](yup.Rectangle[int](10, 20, 30, 40))
    copy = yup.RectangleList[int](original)
    assert copy.getNumRectangles() == 1
    assert copy.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_one_rectangle_constructor():
    rect = yup.Rectangle[int](10, 20, 30, 40)
    rect_list = yup.RectangleList[int](rect)
    assert rect_list.getNumRectangles() == 1
    assert rect_list.getRectangle(0) == rect

#==================================================================================================

def test_rectangle_list_assignment_operator():
    original = yup.RectangleList[int](yup.Rectangle[int](10, 20, 30, 40))
    copy = yup.RectangleList[int]()
    copy = original
    assert copy.getNumRectangles() == 1
    assert copy.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_move_constructor():
    original = yup.RectangleList[int](yup.Rectangle[int](10, 20, 30, 40))
    moved = yup.RectangleList[int](original)
    assert moved.getNumRectangles() == 1
    assert moved.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_move_assignment_operator():
    original = yup.RectangleList[int](yup.Rectangle[int](10, 20, 30, 40))
    moved = yup.RectangleList[int]()
    moved = original
    assert moved.getNumRectangles() == 1
    assert moved.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_is_empty():
    rect_list = yup.RectangleList[int]()
    assert rect_list.isEmpty() == True
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.isEmpty() == False

#==================================================================================================

def test_rectangle_list_get_num_rectangles():
    rect_list = yup.RectangleList[int]()
    assert rect_list.getNumRectangles() == 0
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.getNumRectangles() == 1

#==================================================================================================

def test_rectangle_list_get_rectangle():
    rect = yup.Rectangle[int](10, 20, 30, 40)
    rect_list = yup.RectangleList[int](rect)
    assert rect_list.getRectangle(0) == rect

#==================================================================================================

def test_rectangle_list_clear():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.clear()
    assert rect_list.isEmpty() == True

#==================================================================================================

def test_rectangle_list_add_rectangle():
    rect_list = yup.RectangleList[int]()
    rect = yup.Rectangle[int](10, 20, 30, 40)
    rect_list.add(rect)
    assert rect_list.getNumRectangles() == 1
    assert rect_list.getRectangle(0) == rect

#==================================================================================================

def test_rectangle_list_add_values():
    rect_list = yup.RectangleList[int]()
    rect_list.add(10, 20, 30, 40)
    assert rect_list.getNumRectangles() == 1
    assert rect_list.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_add_without_merging():
    rect_list = yup.RectangleList[int]()
    rect_list.addWithoutMerging(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.getNumRectangles() == 1
    assert rect_list.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_add_another_list():
    rect_list1 = yup.RectangleList[int]()
    rect_list1.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list2 = yup.RectangleList[int]()
    rect_list2.add(yup.Rectangle[int](50, 60, 70, 80))
    rect_list1.add(rect_list2)
    assert rect_list1.getNumRectangles() == 2

#==================================================================================================

def test_rectangle_list_subtract_rectangle():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.subtract(yup.Rectangle[int](10, 20, 15, 20))
    assert rect_list.getNumRectangles() == 2

#==================================================================================================

def test_rectangle_list_subtract_another_list():
    rect_list1 = yup.RectangleList[int]()
    rect_list1.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list2 = yup.RectangleList[int]()
    rect_list2.add(yup.Rectangle[int](10, 20, 15, 20))
    result = rect_list1.subtract(rect_list2)
    assert result == True

#==================================================================================================

def test_rectangle_list_clip_to_rectangle():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    result = rect_list.clipTo(yup.Rectangle[int](5, 15, 25, 35))
    assert result == True

#==================================================================================================

def test_rectangle_list_clip_to_another_list():
    rect_list1 = yup.RectangleList[int]()
    rect_list1.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list2 = yup.RectangleList[int]()
    rect_list2.add(yup.Rectangle[int](5, 15, 25, 35))
    result = rect_list1.clipTo(rect_list2)
    assert result == True

#==================================================================================================

def test_rectangle_list_get_intersection_with():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    dest_region = yup.RectangleList[int]()
    result = rect_list.getIntersectionWith(yup.Rectangle[int](5, 15, 25, 35), dest_region)
    assert result == True

#==================================================================================================

def test_rectangle_list_swap_with():
    rect_list1 = yup.RectangleList[int]()
    rect_list1.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list2 = yup.RectangleList[int]()
    rect_list2.add(yup.Rectangle[int](50, 60, 70, 80))
    rect_list1.swapWith(rect_list2)
    assert rect_list1.getRectangle(0) == yup.Rectangle[int](50, 60, 70, 80)
    assert rect_list2.getRectangle(0) == yup.Rectangle[int](10, 20, 30, 40)

#==================================================================================================

def test_rectangle_list_contains_point():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.containsPoint(yup.Point[int](15, 25)) == True
    assert rect_list.containsPoint(35, 75) == False

#==================================================================================================

def test_rectangle_list_contains_rectangle():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.containsRectangle(yup.Rectangle[int](15, 25, 5, 5)) == True
    assert rect_list.containsRectangle(yup.Rectangle[int](0, 0, 100, 100)) == False

#==================================================================================================

def test_rectangle_list_intersects_rectangle():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    assert rect_list.intersectsRectangle(yup.Rectangle[int](45, 75, 10, 10)) == False
    assert rect_list.intersectsRectangle(yup.Rectangle[int](15, 25, 10, 10)) == True

#==================================================================================================

def test_rectangle_list_intersects_another_list():
    rect_list1 = yup.RectangleList[int]()
    rect_list1.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list2 = yup.RectangleList[int]()
    rect_list2.add(yup.Rectangle[int](45, 75, 10, 10))
    assert rect_list1.intersects(rect_list2) == False
    rect_list2.add(yup.Rectangle[int](15, 25, 10, 10))
    assert rect_list1.intersects(rect_list2) == True

#==================================================================================================

def test_rectangle_list_get_bounds():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.add(yup.Rectangle[int](50, 60, 70, 80))
    bounds = rect_list.getBounds()
    assert bounds == yup.Rectangle[int](10, 20, 110, 120)

#==================================================================================================

def test_rectangle_list_consolidate():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.add(yup.Rectangle[int](40, 20, 30, 40))
    rect_list.consolidate()
    assert rect_list.getNumRectangles() == 1
    assert rect_list.getRectangle(0) == yup.Rectangle[int](10, 20, 60, 40)

#==================================================================================================

def test_rectangle_list_offset_all_point():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.offsetAll(yup.Point[int](10, 10))
    assert rect_list.getRectangle(0) == yup.Rectangle[int](20, 30, 30, 40)

#==================================================================================================

def test_rectangle_list_offset_all_values():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.offsetAll(10, 10)
    assert rect_list.getRectangle(0) == yup.Rectangle[int](20, 30, 30, 40)

#==================================================================================================

def test_rectangle_list_scale_all():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.scaleAll(2)
    assert rect_list.getRectangle(0) == yup.Rectangle[int](20, 40, 60, 80)

#==================================================================================================

def test_rectangle_list_transform_all():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    transform = yup.AffineTransform().scaled(2)
    rect_list.transformAll(transform)
    assert rect_list.getRectangle(0) == yup.Rectangle[int](20, 40, 60, 80)

#==================================================================================================

def test_rectangle_list_to_path():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    path = rect_list.toPath()
    assert isinstance(path, yup.Path)

#==================================================================================================

def test_rectangle_list_iterators():
    rect_list = yup.RectangleList[int]()
    rect_list.add(yup.Rectangle[int](10, 20, 30, 40))
    rect_list.add(yup.Rectangle[int](50, 60, 70, 80))
    rects = [r for r in rect_list]
    assert rects == [yup.Rectangle[int](10, 20, 30, 40), yup.Rectangle[int](50, 60, 70, 80)]

#==================================================================================================

def test_rectangle_list_ensure_storage_allocated():
    rect_list = yup.RectangleList[int]()
    for x in range(10):
        rect_list.add(yup.Rectangle[int](10 + x * 20, 20 + x * 10, 5, 5))
    rect_list.ensureStorageAllocated(20)
    assert rect_list.getNumRectangles() == 10
"""
