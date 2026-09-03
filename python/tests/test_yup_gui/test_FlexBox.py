import pytest
import yup


# ==============================================================================
# FlexItem (value object — no app needed)
# ==============================================================================

def test_flex_item_default():
    item = yup.FlexItem()
    assert item is not None
    assert item.flexGrow == 0.0
    assert item.flexShrink == 1.0


def test_flex_item_with_dimensions():
    item = yup.FlexItem(100.0, 50.0)
    assert item.width == 100.0
    assert item.height == 50.0


def test_flex_item_with_flex():
    item = yup.FlexItem().withFlex(2.0)
    assert item.flexGrow == 2.0


def test_flex_item_with_margin():
    item = yup.FlexItem().withMargin(8.0)
    assert item.marginLeft == 8.0
    assert item.marginRight == 8.0


def test_flex_item_with_order():
    item = yup.FlexItem().withOrder(5)
    assert item.order == 5


def test_flex_alignment_enum():
    assert yup.FlexAlignSelf.autoAlign is not None
    assert yup.FlexAlignSelf.center is not None
    assert yup.FlexAlignSelf.stretch is not None


# ==============================================================================
# FlexBox (value object — no app needed for empty layout)
# ==============================================================================

def test_flex_box_default():
    box = yup.FlexBox()
    assert box is not None
    assert box.flexDirection == yup.FlexDirection.row


def test_flex_box_full_constructor():
    box = yup.FlexBox(
        yup.FlexDirection.column,
        yup.FlexWrap.wrap,
        yup.FlexAlignItems.center,
        yup.FlexJustifyContent.spaceBetween,
        yup.FlexAlignContent.stretch,
    )
    assert box.flexDirection == yup.FlexDirection.column
    assert box.flexWrap == yup.FlexWrap.wrap


def test_flex_box_empty_layout():
    box = yup.FlexBox()
    box.performLayout(yup.Rectangle[int](0, 0, 200, 200))

