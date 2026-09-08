from math import isclose

import yup

"""
FlexBox behaves exactly as the C++ side does - these tests mirror the ones in
tests/yup_gui/yup_FlexBox.cpp so a binding that silently drops a property shows
up here rather than in user code.

A FlexItem only stores a raw pointer to its component, so every test keeps its
components alive in a local list for the whole layout call. Building an item
from a temporary - yup.FlexItem(yup.Component()) - would leave the item holding
a dangling pointer once the expression ends.
"""

TOLERANCE = 0.001

#==================================================================================================

def make_components(count: int):
    return [yup.Component() for _ in range(count)]

def bounds_of(component):
    return (component.getX(), component.getY(), component.getWidth(), component.getHeight())

def assert_bounds(component, x, y, width, height):
    actual = bounds_of(component)
    expected = (x, y, width, height)

    for a, e in zip(actual, expected):
        assert isclose(a, e, abs_tol=TOLERANCE), f"{actual} != {expected}"

#==================================================================================================

def test_construct_empty():
    box = yup.FlexBox()

    assert box.flexDirection == yup.FlexBox.Direction.row
    assert box.flexWrap == yup.FlexBox.Wrap.noWrap
    assert box.alignItems == yup.FlexBox.AlignItems.stretch
    assert box.justifyContent == yup.FlexBox.JustifyContent.flexStart
    assert box.alignContent == yup.FlexBox.AlignContent.stretch
    assert isclose(box.gap, 0.0)
    assert box.items.isEmpty()

#==================================================================================================

def test_construct_with_direction():
    box = yup.FlexBox(yup.FlexBox.Direction.column)

    assert box.flexDirection == yup.FlexBox.Direction.column

#==================================================================================================

def test_construct_with_all_arguments():
    box = yup.FlexBox(yup.FlexBox.Direction.columnReverse,
                      yup.FlexBox.Wrap.wrap,
                      yup.FlexBox.AlignItems.center,
                      yup.FlexBox.JustifyContent.spaceEvenly,
                      yup.FlexBox.AlignContent.flexEnd)

    assert box.flexDirection == yup.FlexBox.Direction.columnReverse
    assert box.flexWrap == yup.FlexBox.Wrap.wrap
    assert box.alignItems == yup.FlexBox.AlignItems.center
    assert box.justifyContent == yup.FlexBox.JustifyContent.spaceEvenly
    assert box.alignContent == yup.FlexBox.AlignContent.flexEnd

#==================================================================================================

def test_items_are_stored_by_reference():
    # def_readwrite hands back the live Array, so mutating it must be visible on the box.
    components = make_components(1)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0))

    assert box.items.size() == 1
    assert len(box.items) == 1

#==================================================================================================

def test_sentinel_is_minus_one_not_zero():
    item = yup.FlexItem()

    assert isclose(item.width, -1.0)
    assert isclose(item.height, -1.0)
    assert isclose(item.flexBasis, -1.0)
    assert isclose(item.flexBasisPercent, -1.0)

#==================================================================================================

def test_explicit_zero_width_is_honoured():
    components = make_components(2)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(0.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(100.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 50))

    assert_bounds(components[0], 0, 0, 0, 50)
    assert_bounds(components[1], 0, 0, 100, 50)

#==================================================================================================

def test_flex_one_one_zero_shares_space_equally():
    components = make_components(3)

    box = yup.FlexBox()
    for component in components:
        box.items.add(yup.FlexItem(component).withFlex(1.0).withFlexBasis(0.0))

    box.performLayout(yup.Rectangle[float](0, 0, 300, 50))

    assert_bounds(components[0], 0, 0, 100, 50)
    assert_bounds(components[1], 100, 0, 100, 50)
    assert_bounds(components[2], 200, 0, 100, 50)

#==================================================================================================

def test_stretch_fills_cross_axis_in_single_line_container():
    # The auto-sized item fills the container's cross size, while the item with an
    # explicit height keeps it and sits at the cross start.
    components = make_components(2)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0).withHeight(20.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 80))

    assert_bounds(components[0], 0, 0, 50, 80)
    assert_bounds(components[1], 50, 0, 50, 20)

#==================================================================================================

def test_justify_content_flex_end_does_not_overflow_when_items_grow():
    components = make_components(2)

    box = yup.FlexBox()
    box.justifyContent = yup.FlexBox.JustifyContent.flexEnd
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0).withFlex(1.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    assert_bounds(components[0], 0, 0, 150, 40)
    assert_bounds(components[1], 150, 0, 50, 40)

#==================================================================================================

def test_space_around_puts_half_a_share_at_the_edges():
    components = make_components(2)

    box = yup.FlexBox()
    box.justifyContent = yup.FlexBox.JustifyContent.spaceAround
    for component in components:
        box.items.add(yup.FlexItem(component).withWidth(50.0))

    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    # 100 free over 2 items: 25 at each edge, 50 between.
    assert_bounds(components[0], 25, 0, 50, 40)
    assert_bounds(components[1], 125, 0, 50, 40)

#==================================================================================================

def test_space_evenly_puts_an_equal_share_everywhere():
    components = make_components(2)

    box = yup.FlexBox()
    box.justifyContent = yup.FlexBox.JustifyContent.spaceEvenly
    for component in components:
        box.items.add(yup.FlexItem(component).withWidth(50.0))

    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    assert_bounds(components[0], 100.0 / 3.0, 0, 50, 40)
    assert_bounds(components[1], 50.0 + 200.0 / 3.0, 0, 50, 40)

#==================================================================================================

def test_gap_separates_items():
    components = make_components(3)

    box = yup.FlexBox()
    box.gap = 10.0
    for component in components:
        box.items.add(yup.FlexItem(component).withWidth(50.0))

    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    assert_bounds(components[0], 0, 0, 50, 40)
    assert_bounds(components[1], 60, 0, 50, 40)
    assert_bounds(components[2], 120, 0, 50, 40)

#==================================================================================================

def test_row_and_column_gaps_are_independent():
    components = make_components(4)

    box = yup.FlexBox()
    box.flexWrap = yup.FlexBox.Wrap.wrap
    box.alignContent = yup.FlexBox.AlignContent.flexStart
    box.columnGap = 10.0
    box.rowGap = 30.0
    for component in components:
        box.items.add(yup.FlexItem(component).withWidth(50.0).withHeight(20.0))

    box.performLayout(yup.Rectangle[float](0, 0, 120, 200))

    # Two items per line (50 + 10 + 50 = 110 fits in 120), lines 30 apart.
    assert_bounds(components[0], 0, 0, 50, 20)
    assert_bounds(components[1], 60, 0, 50, 20)
    assert_bounds(components[2], 0, 50, 50, 20)
    assert_bounds(components[3], 60, 50, 50, 20)

#==================================================================================================

def test_padding_reduces_the_content_area():
    components = make_components(1)

    box = yup.FlexBox()
    box.setPadding(10.0)
    box.items.add(yup.FlexItem(components[0]).withFlex(1.0).withFlexBasis(0.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 100))

    assert_bounds(components[0], 10, 10, 180, 80)

#==================================================================================================

def test_set_padding_with_two_arguments():
    box = yup.FlexBox()
    box.setPadding(10.0, 20.0)

    assert isclose(box.paddingLeft, 10.0)
    assert isclose(box.paddingRight, 10.0)
    assert isclose(box.paddingTop, 20.0)
    assert isclose(box.paddingBottom, 20.0)

#==================================================================================================

def test_auto_margin_pushes_an_item_to_the_end():
    components = make_components(2)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0).withAutoMargins(True, False, False, False))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    assert_bounds(components[0], 0, 0, 50, 40)
    assert_bounds(components[1], 150, 0, 50, 40)

#==================================================================================================

def test_order_reorders_items_and_is_stable():
    components = make_components(3)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0).withOrder(1))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0).withOrder(0))
    box.items.add(yup.FlexItem(components[2]).withWidth(50.0).withOrder(0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    # The two order-0 items keep their source order, then the order-1 item.
    assert_bounds(components[1], 0, 0, 50, 40)
    assert_bounds(components[2], 50, 0, 50, 40)
    assert_bounds(components[0], 100, 0, 50, 40)

#==================================================================================================

def test_align_self_overrides_the_container_alignment():
    components = make_components(2)

    box = yup.FlexBox()
    box.alignItems = yup.FlexBox.AlignItems.flexStart
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0).withHeight(20.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0).withHeight(20.0)
                  .withAlignSelf(yup.FlexItem.AlignSelf.flexEnd))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 100))

    assert_bounds(components[0], 0, 0, 50, 20)
    assert_bounds(components[1], 50, 80, 50, 20)

#==================================================================================================

def test_column_direction_lays_out_vertically():
    components = make_components(2)

    box = yup.FlexBox(yup.FlexBox.Direction.column)
    box.items.add(yup.FlexItem(components[0]).withHeight(30.0))
    box.items.add(yup.FlexItem(components[1]).withHeight(30.0))
    box.performLayout(yup.Rectangle[float](0, 0, 100, 200))

    assert_bounds(components[0], 0, 0, 100, 30)
    assert_bounds(components[1], 0, 30, 100, 30)

#==================================================================================================

def test_row_reverse_mirrors_the_main_axis():
    components = make_components(2)

    box = yup.FlexBox(yup.FlexBox.Direction.rowReverse)
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(50.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    assert_bounds(components[0], 150, 0, 50, 40)
    assert_bounds(components[1], 100, 0, 50, 40)

#==================================================================================================

def test_percent_sizes_resolve_against_the_container():
    components = make_components(1)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidthPercent(25.0).withHeightPercent(50.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 100))

    assert_bounds(components[0], 0, 0, 50, 50)

#==================================================================================================

def test_flex_shrink_respects_min_width():
    components = make_components(2)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withWidth(150.0).withMinWidth(120.0))
    box.items.add(yup.FlexItem(components[1]).withWidth(150.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    # The first item freezes at its floor and the second absorbs the rest.
    assert_bounds(components[0], 0, 0, 120, 40)
    assert_bounds(components[1], 120, 0, 80, 40)

#==================================================================================================

def test_perform_layout_accepts_an_integer_rectangle():
    components = make_components(1)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(components[0]).withFlex(1.0).withFlexBasis(0.0))
    box.performLayout(yup.Rectangle[int](0, 0, 200, 100))

    assert_bounds(components[0], 0, 0, 200, 100)

#==================================================================================================

def test_layout_is_idempotent():
    components = make_components(2)

    box = yup.FlexBox()
    for component in components:
        box.items.add(yup.FlexItem(component).withFlex(1.0).withFlexBasis(0.0))

    area = yup.Rectangle[float](0, 0, 200, 100)
    box.performLayout(area)
    first = [bounds_of(component) for component in components]

    box.performLayout(area)
    second = [bounds_of(component) for component in components]

    assert first == second

#==================================================================================================

def test_empty_container_and_degenerate_area_do_not_throw():
    box = yup.FlexBox()
    box.performLayout(yup.Rectangle[float](0, 0, 200, 100))

    components = make_components(1)
    box.items.add(yup.FlexItem(components[0]).withFlex(1.0).withFlexBasis(0.0))
    box.performLayout(yup.Rectangle[float](0, 0, 0, 0))

    assert isclose(components[0].getWidth(), 0.0, abs_tol=TOLERANCE)

#==================================================================================================

def test_item_without_a_component_is_ignored():
    components = make_components(1)

    box = yup.FlexBox()
    box.items.add(yup.FlexItem(50.0, 20.0))
    box.items.add(yup.FlexItem(components[0]).withWidth(50.0))
    box.performLayout(yup.Rectangle[float](0, 0, 200, 40))

    # The component-less item still takes up its slot on the main axis.
    assert_bounds(components[0], 50, 0, 50, 40)

#==================================================================================================

def test_associated_component_property_round_trips():
    components = make_components(1)

    item = yup.FlexItem()
    assert item.associatedComponent is None

    item.associatedComponent = components[0]
    assert item.associatedComponent is not None
