from math import isclose

import pytest

import yup

"""
Grid behaves exactly as the C++ side does - these tests mirror the ones in
tests/yup_gui/yup_Grid.cpp, and every expected rectangle here is the same one
asserted there.

A GridItem only stores a raw pointer to its component, so every test keeps its
components alive in a local list for the whole layout call.
"""

TOLERANCE = 0.001

#==================================================================================================

def make_components(count: int):
    return [yup.Component() for _ in range(count)]

def assert_bounds(component, x, y, width, height):
    actual = (component.getX(), component.getY(), component.getWidth(), component.getHeight())
    expected = (x, y, width, height)

    for a, e in zip(actual, expected):
        assert isclose(a, e, abs_tol=TOLERANCE), f"{actual} != {expected}"

def add_columns(grid, count, size):
    for _ in range(count):
        grid.templateColumns.add(yup.Grid.TrackInfo.px(size))

def add_rows(grid, count, size):
    for _ in range(count):
        grid.templateRows.add(yup.Grid.TrackInfo.px(size))

def add_items(grid, components):
    for component in components:
        grid.items.add(yup.GridItem(component))

#==================================================================================================

def test_construct_empty():
    grid = yup.Grid()

    assert grid.templateColumns.isEmpty()
    assert grid.templateRows.isEmpty()
    assert grid.items.isEmpty()
    assert grid.autoFlow == yup.Grid.AutoFlow.row
    assert grid.justifyItems == yup.Grid.AlignItems.stretch
    assert grid.alignItems == yup.Grid.AlignItems.stretch
    assert isclose(grid.autoRows, 40.0)
    assert isclose(grid.autoColumns, 100.0)

#==================================================================================================

def test_gap_shorthand_sentinels_default_to_minus_one():
    grid = yup.Grid()

    assert isclose(grid.gap, 0.0)
    assert isclose(grid.rowGap, -1.0)
    assert isclose(grid.columnGap, -1.0)

#==================================================================================================

def test_track_info_is_factory_only():
    # The default constructor is private on purpose, so there is no py::init.
    with pytest.raises(TypeError):
        yup.Grid.TrackInfo()

#==================================================================================================

def test_track_info_factories_build_min_max_pairs():
    # minimum / maximum hand back a reference into the TrackInfo, so bind the track to a
    # name first - chaining yup.Grid.TrackInfo.px(120.0).minimum off the temporary would
    # read through a dead object.
    fixed = yup.Grid.TrackInfo.px(120.0)
    assert fixed.minimum.type == yup.Grid.TrackInfo.SizeType.pixels
    assert isclose(fixed.minimum.value, 120.0)
    assert isclose(fixed.maximum.value, 120.0)
    assert not fixed.isFractional()

    flexible = yup.Grid.TrackInfo.fr(2.0)
    assert flexible.isFractional()
    assert isclose(flexible.maximum.value, 2.0)

    ranged = yup.Grid.TrackInfo.minmax(yup.Grid.TrackInfo.px(50.0), yup.Grid.TrackInfo.fr(1.0))
    assert isclose(ranged.minimum.value, 50.0)
    assert ranged.isFractional()

#==================================================================================================

def test_fixed_tracks_place_items():
    components = make_components(2)

    grid = yup.Grid()
    add_columns(grid, 2, 100.0)
    add_rows(grid, 1, 50.0)
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 0, 0, 100, 50)
    assert_bounds(components[1], 100, 0, 100, 50)

#==================================================================================================

def test_fr_tracks_account_for_column_gap():
    components = make_components(3)

    grid = yup.Grid()
    for _ in range(3):
        grid.templateColumns.add(yup.Grid.TrackInfo.fr(1.0))
    add_rows(grid, 1, 50.0)
    grid.columnGap = 30.0
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    # The two 30px gaps come off the top: (300 - 60) / 3 = 80 per track.
    assert_bounds(components[0], 0, 0, 80, 50)
    assert_bounds(components[1], 110, 0, 80, 50)
    assert_bounds(components[2], 220, 0, 80, 50)

#==================================================================================================

def test_minmax_grows_to_its_maximum():
    components = make_components(2)

    grid = yup.Grid()
    grid.templateColumns.add(yup.Grid.TrackInfo.minmax(yup.Grid.TrackInfo.px(50.0),
                                                       yup.Grid.TrackInfo.px(200.0)))
    grid.templateColumns.add(yup.Grid.TrackInfo.px(100.0))
    add_rows(grid, 1, 50.0)
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 0, 0, 200, 50)
    assert_bounds(components[1], 200, 0, 100, 50)

#==================================================================================================

def test_repeat_expands_a_track():
    grid = yup.Grid()
    grid.templateColumns.addArray(yup.Grid.repeat(4, yup.Grid.TrackInfo.px(60.0)))

    assert grid.templateColumns.size() == 4
    assert isclose(grid.templateColumns[0].maximum.value, 60.0)

#==================================================================================================

def test_repeat_to_fill_counts_what_fits():
    components = make_components(3)

    grid = yup.Grid()
    grid.templateColumns.addArray(yup.Grid.repeatToFill(yup.Grid.TrackInfo.px(100.0), 350.0, 0.0, 100.0))
    add_rows(grid, 1, 50.0)

    assert grid.templateColumns.size() == 3

    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 350, 200))

    assert_bounds(components[0], 0, 0, 100, 50)
    assert_bounds(components[1], 100, 0, 100, 50)
    assert_bounds(components[2], 200, 0, 100, 50)

#==================================================================================================

def test_track_array_supports_iteration_and_indexing():
    grid = yup.Grid()
    add_columns(grid, 3, 40.0)

    assert len(grid.templateColumns) == 3
    assert isclose(grid.templateColumns[2].maximum.value, 40.0)
    assert len([track for track in grid.templateColumns]) == 3

    with pytest.raises(IndexError):
        grid.templateColumns[3]

    grid.templateColumns.clear()
    assert grid.templateColumns.isEmpty()

#==================================================================================================

def test_auto_place_constant_is_minus_one():
    assert yup.GridItem.autoPlace == -1

    item = yup.GridItem()
    assert item.column == yup.GridItem.autoPlace
    assert item.row == yup.GridItem.autoPlace

#==================================================================================================

def test_partial_placement_keeps_the_explicit_row():
    components = make_components(3)

    grid = yup.Grid()
    add_columns(grid, 3, 100.0)
    add_rows(grid, 3, 50.0)

    grid.items.add(yup.GridItem(components[0]).withRow(2))
    grid.items.add(yup.GridItem(components[1]))
    grid.items.add(yup.GridItem(components[2]).withRow(2))
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 0, 100, 100, 50)
    assert_bounds(components[1], 0, 0, 100, 50)
    assert_bounds(components[2], 100, 100, 100, 50)

#==================================================================================================

def test_auto_placement_avoids_later_explicit_items():
    components = make_components(3)

    grid = yup.Grid()
    add_columns(grid, 3, 100.0)
    add_rows(grid, 2, 50.0)

    grid.items.add(yup.GridItem(components[0]))
    grid.items.add(yup.GridItem(components[1]).withColumn(0).withRow(0))
    grid.items.add(yup.GridItem(components[2]))
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 100, 0, 100, 50)
    assert_bounds(components[1], 0, 0, 100, 50)
    assert_bounds(components[2], 200, 0, 100, 50)

#==================================================================================================

def test_column_flow_fills_columns_first():
    components = make_components(4)

    grid = yup.Grid()
    grid.autoFlow = yup.Grid.AutoFlow.column
    add_columns(grid, 2, 100.0)
    add_rows(grid, 2, 50.0)
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 0, 0, 100, 50)
    assert_bounds(components[1], 0, 50, 100, 50)
    assert_bounds(components[2], 100, 0, 100, 50)
    assert_bounds(components[3], 100, 50, 100, 50)

#==================================================================================================

def test_dense_flow_backfills_a_hole_that_sparse_flow_leaves():
    def build(flow, components):
        grid = yup.Grid()
        grid.autoFlow = flow
        add_columns(grid, 3, 100.0)
        add_rows(grid, 2, 50.0)

        grid.items.add(yup.GridItem(components[0]))
        grid.items.add(yup.GridItem(components[1]).withColumnSpan(3))
        grid.items.add(yup.GridItem(components[2]))
        return grid

    sparse_components = make_components(3)
    build(yup.Grid.AutoFlow.row, sparse_components).performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(sparse_components[0], 0, 0, 100, 50)
    assert_bounds(sparse_components[1], 0, 50, 300, 50)
    assert_bounds(sparse_components[2], 0, 100, 100, 40)

    dense_components = make_components(3)
    build(yup.Grid.AutoFlow.rowDense, dense_components).performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(dense_components[0], 0, 0, 100, 50)
    assert_bounds(dense_components[1], 0, 50, 300, 50)
    assert_bounds(dense_components[2], 100, 0, 100, 50)

#==================================================================================================

def test_justify_content_centers_the_tracks():
    components = make_components(3)

    grid = yup.Grid()
    grid.justifyContent = yup.Grid.AlignContent.center
    add_columns(grid, 3, 60.0)
    add_rows(grid, 1, 50.0)
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 60, 0, 60, 50)
    assert_bounds(components[1], 120, 0, 60, 50)
    assert_bounds(components[2], 180, 0, 60, 50)

#==================================================================================================

def test_template_areas_place_items():
    components = make_components(4)

    grid = yup.Grid()
    add_columns(grid, 3, 100.0)
    add_rows(grid, 3, 50.0)

    result = grid.setTemplateAreas(yup.StringArray(["header header header",
                                                    "side   main   main",
                                                    "side   foot   foot"]))
    assert result.wasOk(), result.getErrorMessage()

    grid.items.add(yup.GridItem(components[0]).withArea("header"))
    grid.items.add(yup.GridItem(components[1]).withArea("side"))
    grid.items.add(yup.GridItem(components[2]).withArea("main"))
    grid.items.add(yup.GridItem(components[3]).withArea("foot"))
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 0, 0, 300, 50)
    assert_bounds(components[1], 0, 50, 100, 100)
    assert_bounds(components[2], 100, 50, 200, 50)
    assert_bounds(components[3], 100, 100, 200, 50)

#==================================================================================================

def test_template_areas_reject_ragged_rows():
    grid = yup.Grid()
    result = grid.setTemplateAreas(yup.StringArray(["a a", "b b b"]))

    assert result.failed()

#==================================================================================================

def test_named_lines_place_items():
    components = make_components(3)

    grid = yup.Grid()
    add_columns(grid, 3, 100.0)
    add_rows(grid, 2, 50.0)

    grid.setColumnLineName(0, "left")
    grid.setColumnLineName(1, "mid")
    grid.setColumnLineName(2, "right")
    grid.setRowLineName(0, "top")
    grid.setRowLineName(1, "bottom")

    grid.items.add(yup.GridItem(components[0]).withColumnStart("mid").withRowStart("top"))
    grid.items.add(yup.GridItem(components[1]).withColumnStart("left").withRowStart("bottom"))
    grid.items.add(yup.GridItem(components[2]).withColumnStart("right").withRowStart("bottom"))
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 100, 0, 100, 50)
    assert_bounds(components[1], 0, 50, 100, 50)
    assert_bounds(components[2], 200, 50, 100, 50)

#==================================================================================================

def test_margins_inset_an_item_within_its_cell():
    components = make_components(1)

    grid = yup.Grid()
    add_columns(grid, 1, 100.0)
    add_rows(grid, 1, 50.0)
    grid.items.add(yup.GridItem(components[0]).withMargin(10.0))
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    assert_bounds(components[0], 10, 10, 80, 30)

#==================================================================================================

def test_perform_layout_accepts_an_integer_rectangle():
    components = make_components(1)

    grid = yup.Grid()
    grid.templateColumns.add(yup.Grid.TrackInfo.fr(1.0))
    grid.templateRows.add(yup.Grid.TrackInfo.fr(1.0))
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[int](0, 0, 300, 200))

    assert_bounds(components[0], 0, 0, 300, 200)

#==================================================================================================

def test_layout_is_idempotent():
    components = make_components(2)

    grid = yup.Grid()
    add_columns(grid, 2, 100.0)
    add_rows(grid, 1, 50.0)
    add_items(grid, components)

    area = yup.Rectangle[float](0, 0, 300, 200)
    grid.performLayout(area)
    first = [(c.getX(), c.getY(), c.getWidth(), c.getHeight()) for c in components]

    grid.performLayout(area)
    second = [(c.getX(), c.getY(), c.getWidth(), c.getHeight()) for c in components]

    assert first == second

#==================================================================================================

def test_empty_grid_and_degenerate_area_do_not_throw():
    grid = yup.Grid()
    grid.performLayout(yup.Rectangle[float](0, 0, 300, 200))

    components = make_components(1)
    add_columns(grid, 1, 100.0)
    add_rows(grid, 1, 50.0)
    add_items(grid, components)
    grid.performLayout(yup.Rectangle[float](0, 0, 0, 0))
