import pytest

import yup

"""
#==================================================================================================

def test_construct_default():
    c = yup.ColourGradient()
    assert c.getNumColours() == 0
    assert c.isOpaque()
    assert c.isRadial == False
    assert c.point1 == yup.Point[float](0, 0) or c.point1 == yup.Point[float](987654, 0)
    assert c.point2 == yup.Point[float](0, 0)

#==================================================================================================

def test_copy_construct():
    c1 = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    c2 = yup.ColourGradient(c1)
    assert c1 == c2
    assert not (c1 != c2)

#==================================================================================================

def test_construct_coordinates_linear():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    assert c.isRadial == False
    assert c.point1 == yup.Point[float](0, 0)
    assert c.point2 == yup.Point[float](1, 1)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_construct_points_linear():
    c = yup.ColourGradient(yup.Colours.black, yup.Point[float](0.0, 0.0), yup.Colours.white, yup.Point[float](1.0, 1.0), False)
    assert c.isRadial == False
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

#==================================================================================================

def test_construct_coordinates_radial():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, True)
    assert c.isRadial == True
    assert c.point1 == yup.Point[float](0, 0)
    assert c.point2 == yup.Point[float](1, 1)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_construct_points_radial():
    c = yup.ColourGradient(yup.Colours.black, yup.Point[float](0.0, 0.0), yup.Colours.white, yup.Point[float](1.0, 1.0), True)
    assert c.isRadial == True
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

#==================================================================================================

def test_colour_at_position():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    assert c.getColourAtPosition(0.0) == yup.Colours.black
    assert c.getColourAtPosition(0.5) == yup.Colour(127, 127, 127)
    assert c.getColourAtPosition(1.0) == yup.Colours.white

#==================================================================================================

def test_get_colour_position():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    assert c.getColourPosition(0) == pytest.approx(0.0)
    assert c.getColourPosition(1) == pytest.approx(1.0)

#==================================================================================================

def test_set_colour():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    c.setColour(0, yup.Colours.red)
    c.setColour(1, yup.Colours.green)
    assert c.getColour(0) == yup.Colours.red
    assert c.getColour(1) == yup.Colours.green

#==================================================================================================

def test_add_remove_colour():
    c = yup.ColourGradient()
    assert c.getNumColours() == 0

    c.addColour(0.0, yup.Colours.yellow)
    assert c.getNumColours() == 1
    assert c.getColour(0) == yup.Colours.yellow

    c.addColour(0.5, yup.Colours.green)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.yellow
    assert c.getColour(1) == yup.Colours.green

    c.addColour(1.0, yup.Colours.red)
    assert c.getNumColours() == 3
    assert c.getColour(0) == yup.Colours.yellow
    assert c.getColour(1) == yup.Colours.green
    assert c.getColour(2) == yup.Colours.red

    c.removeColour(1)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.yellow
    assert c.getColour(1) == yup.Colours.red

    c.clearColours()
    assert c.getNumColours() == 0

#==================================================================================================

@pytest.mark.skip(reason="JUCE bug, assert raises when removing index 0")
def test_remove_colour():
    c = yup.ColourGradient()
    assert c.getNumColours() == 0

    c.addColour(0.0, yup.Colours.yellow)
    assert c.getNumColours() == 1
    assert c.getColour(0) == yup.Colours.yellow

    c.removeColour(0)
    assert c.getNumColours() == 0

#==================================================================================================

def test_vertical_values():
    c = yup.ColourGradient.vertical(yup.Colours.black, 0.0, yup.Colours.white, 1.0)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_vertical_rectangle_int():
    c = yup.ColourGradient.vertical(yup.Colours.black, yup.Colours.white, yup.Rectangle[int](0, 0, 100, 100))
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_vertical_rectangle_float():
    c = yup.ColourGradient.vertical(yup.Colours.black, yup.Colours.white, yup.Rectangle[float](0, 0, 100, 100))
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

#==================================================================================================

def test_horizontal_values():
    c = yup.ColourGradient.horizontal(yup.Colours.black, 0.0, yup.Colours.white, 1.0)
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_horizontal_rectangle_int():
    c = yup.ColourGradient.horizontal(yup.Colours.black, yup.Colours.white, yup.Rectangle[int](0, 0, 100, 100))
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

def test_horizontal_rectangle_float():
    c = yup.ColourGradient.horizontal(yup.Colours.black, yup.Colours.white, yup.Rectangle[float](0, 0, 100, 100))
    assert c.getNumColours() == 2
    assert c.getColour(0) == yup.Colours.black
    assert c.getColour(1) == yup.Colours.white

#==================================================================================================

def test_is_invisible_is_opaque():
    c = yup.ColourGradient(yup.Colours.black, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    assert not c.isInvisible()
    assert c.isOpaque()

    c = yup.ColourGradient(yup.Colours.transparentBlack, 0.0, 0.0, yup.Colours.white, 1.0, 1.0, False)
    assert not c.isInvisible()
    assert not c.isOpaque()

    c = yup.ColourGradient(yup.Colours.transparentBlack, 0.0, 0.0, yup.Colours.transparentWhite, 1.0, 1.0, False)
    assert c.isInvisible()
    assert not c.isOpaque()
"""
