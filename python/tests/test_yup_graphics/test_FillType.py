import pytest
import yup

"""
from .utilities import get_logo_image

#==================================================================================================

def test_default_constructor():
    f = yup.FillType()
    assert f.colour == yup.Colour(0xff000000)
    assert f.gradient is None
    assert f.image == yup.Image()
    assert f.transform == yup.AffineTransform()
    assert f.isColour()
    assert not f.isGradient()
    assert not f.isTiledImage()

#==================================================================================================

def test_colour_constructor():
    f = yup.FillType(yup.Colours.red)
    assert f.colour == yup.Colours.red
    assert f.gradient is None
    assert f.image == yup.Image()
    assert f.transform == yup.AffineTransform()
    assert f.isColour()
    assert not f.isGradient()
    assert not f.isTiledImage()

#==================================================================================================

def test_gradient_constructor():
    f = yup.FillType(yup.ColourGradient.vertical(yup.Colours.red, 0.0, yup.Colours.green, 1.0))
    assert f.colour == yup.Colour(0xff000000)
    assert f.gradient == yup.ColourGradient.vertical(yup.Colours.red, 0.0, yup.Colours.green, 1.0)
    assert f.image == yup.Image()
    assert f.transform == yup.AffineTransform()
    assert not f.isColour()
    assert f.isGradient()
    assert not f.isTiledImage()

#==================================================================================================

def test_image_constructor():
    f = yup.FillType(get_logo_image(), yup.AffineTransform.translation(10.0, 10))
    assert f.colour == yup.Colour(0xff000000)
    assert f.gradient is None
    assert f.image == get_logo_image()
    assert f.transform == yup.AffineTransform.translation(10.0, 10)
    assert not f.isColour()
    assert not f.isGradient()
    assert f.isTiledImage()

#==================================================================================================

def test_copy_constructor():
    f1 = yup.FillType(yup.Colours.red)
    f2 = yup.FillType(f1)
    assert f1.colour == f2.colour
    assert f1 == f2
    assert not (f1 != f2)

#==================================================================================================

def test_set_colour():
    f = yup.FillType(yup.ColourGradient.vertical(yup.Colours.red, 0.0, yup.Colours.green, 1.0))
    f.setColour(yup.Colours.red)
    assert f.isColour()
    assert not f.isGradient()

#==================================================================================================

def test_set_gradient():
    f = yup.FillType(yup.Colours.red)
    f.setGradient(yup.ColourGradient.vertical(yup.Colours.red, 0.0, yup.Colours.green, 1.0))
    assert not f.isColour()
    assert f.isGradient()

#==================================================================================================

def test_set_image():
    f = yup.FillType(yup.Colours.red)
    f.setTiledImage(get_logo_image(), yup.AffineTransform.translation(10.0, 10))
    assert not f.isColour()
    assert f.isTiledImage()

#==================================================================================================

def test_set_opacity():
    f = yup.FillType(yup.Colours.red)
    assert not f.isInvisible()
    assert f.getOpacity() == pytest.approx(1.0)

    f.setOpacity(0.0)
    assert f.isColour()
    assert f.getOpacity() == pytest.approx(0.0)
    assert f.isInvisible()

#==================================================================================================

def test_transformed():
    f = yup.FillType(get_logo_image(), yup.AffineTransform())
    result = f.transformed(yup.AffineTransform.translation(10.0, 10))
    assert result.transform == yup.AffineTransform.translation(10.0, 10)
"""
