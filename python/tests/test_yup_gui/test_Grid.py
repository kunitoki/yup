import pytest
import yup


# ==============================================================================
# GridItem (value object — no app needed)
# ==============================================================================

def test_grid_item_default():
    item = yup.GridItem()
    assert item is not None
    assert item.column == 0
    assert item.row == 0
    assert item.columnSpan == 1
    assert item.rowSpan == 1


def test_grid_item_with_column():
    item = yup.GridItem().withColumn(3)
    assert item.column == 3


def test_grid_item_with_row():
    item = yup.GridItem().withRow(2)
    assert item.row == 2


def test_grid_item_with_span():
    item = yup.GridItem().withColumnSpan(2).withRowSpan(3)
    assert item.columnSpan == 2
    assert item.rowSpan == 3


def test_grid_item_with_margin():
    item = yup.GridItem().withMargin(6.0)
    assert item.marginLeft == 6.0


# ==============================================================================
# Grid (value object — no app needed for empty layout)
# ==============================================================================

def test_grid_default():
    grid = yup.Grid()
    assert grid is not None
    assert grid.autoRows == 40.0
    assert grid.autoColumns == 100.0


def test_grid_track_info():
    px = yup.TrackInfo.px(120.0)
    assert px.pixelSize == 120.0

    fr = yup.TrackInfo.fr(2.0)
    assert fr.fraction == 2.0

    auto = yup.TrackInfo.auto_()
    assert auto.isAuto is True


def test_grid_empty_layout():
    grid = yup.Grid()
    grid.performLayout(yup.Rectangle[int](0, 0, 300, 200))

