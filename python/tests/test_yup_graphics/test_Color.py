import pytest

import yup

#==================================================================================================

def test_construct_default():
    c = yup.Color()
    assert c.getARGB() == 0xFF000000

def test_construct_copy():
    c1 = yup.Color(0xFF112233)
    c2 = yup.Color(c1)
    assert c1 == c2

def test_construct_rgb():
    c = yup.Color(0x11, 0x22, 0x33)
    assert c.getARGB() == 0xFF112233

def test_construct_argb():
    c = yup.Color(0x44, 0x11, 0x22, 0x33)
    assert c.getARGB() == 0x44112233

def test_construct_from_argb():
    c = yup.Color(0xFF112233)
    assert c.getARGB() == 0xFF112233

#==================================================================================================

def test_static_factory_methods():
    # Test fromHSV
    c = yup.Color.fromHSV(0.5, 0.6, 0.7, 0.8)
    assert isinstance(c, yup.Color)

    # Test fromHSL
    c = yup.Color.fromHSL(0.5, 0.6, 0.7, 0.8)
    assert isinstance(c, yup.Color)

    # Test fromString
    c = yup.Color.fromString("#FF0000")
    assert isinstance(c, yup.Color)

    # Test opaqueRandom
    c = yup.Color.opaqueRandom()
    assert isinstance(c, yup.Color)
    assert c.isOpaque()

#==================================================================================================

def test_transparency_checks():
    transparent_color = yup.Color(0x00000000)
    semi_transparent_color = yup.Color(0x80FF0000)
    opaque_color = yup.Color(0xFF00FF00)

    assert transparent_color.isTransparent()
    assert transparent_color.isSemiTransparent()
    assert not transparent_color.isOpaque()

    assert not semi_transparent_color.isTransparent()
    assert semi_transparent_color.isSemiTransparent()
    assert not semi_transparent_color.isOpaque()

    assert not opaque_color.isTransparent()
    assert not opaque_color.isSemiTransparent()
    assert opaque_color.isOpaque()

#==================================================================================================

def test_alpha_component():
    c = yup.Color(0xFF112233)

    # Test getters
    assert c.getAlpha() == 0xFF
    assert c.getAlphaFloat() == pytest.approx(1.0)

    # Test setters
    c.setAlpha(0x80)
    assert c.getAlpha() == 0x80

    c.setAlpha(0.5)
    assert c.getAlpha() == 0x80

    # Test withAlpha
    c1 = yup.Color(0xFF112233)
    c2 = c1.withAlpha(0x80)
    assert c2.getAlpha() == 0x80
    assert c1.getAlpha() == 0xFF  # Original unchanged

    c3 = c1.withAlpha(0.5)
    assert c3.getAlpha() == 0x80

    # Test withMultipliedAlpha
    c4 = c1.withMultipliedAlpha(0x80)
    assert isinstance(c4, yup.Color)

    c5 = c1.withMultipliedAlpha(0.5)
    assert isinstance(c5, yup.Color)

#==================================================================================================

def test_red_component():
    c = yup.Color(0xFF112233)

    # Test getters
    assert c.getRed() == 0x11
    assert c.getRedFloat() == pytest.approx(0.0666667, rel=1e-6)

    # Test setters
    c.setRed(0x80)
    assert c.getRed() == 0x80

    c.setRed(0.5)
    assert c.getRed() == 0x80

    # Test withRed
    c1 = yup.Color(0xFF112233)
    c2 = c1.withRed(0x80)
    assert c2.getRed() == 0x80
    assert c1.getRed() == 0x11  # Original unchanged

    c3 = c1.withRed(0.5)
    assert c3.getRed() == 0x80

#==================================================================================================

def test_green_component():
    c = yup.Color(0xFF112233)

    # Test getters
    assert c.getGreen() == 0x22
    assert c.getGreenFloat() == pytest.approx(0.1333333, rel=1e-6)

    # Test setters
    c.setGreen(0x80)
    assert c.getGreen() == 0x80

    c.setGreen(0.5)
    assert c.getGreen() == 0x80

    # Test withGreen
    c1 = yup.Color(0xFF112233)
    c2 = c1.withGreen(0x80)
    assert c2.getGreen() == 0x80
    assert c1.getGreen() == 0x22  # Original unchanged

    c3 = c1.withGreen(0.5)
    assert c3.getGreen() == 0x80

#==================================================================================================

def test_blue_component():
    c = yup.Color(0xFF112233)

    # Test getters
    assert c.getBlue() == 0x33
    assert c.getBlueFloat() == pytest.approx(0.2, rel=1e-6)

    # Test setters
    c.setBlue(0x80)
    assert c.getBlue() == 0x80

    c.setBlue(0.5)
    assert c.getBlue() == 0x80

    # Test withBlue
    c1 = yup.Color(0xFF112233)
    c2 = c1.withBlue(0x80)
    assert c2.getBlue() == 0x80
    assert c1.getBlue() == 0x33  # Original unchanged

    c3 = c1.withBlue(0.5)
    assert c3.getBlue() == 0x80

#==================================================================================================

def test_hsl_color_space():
    c = yup.Color(0xFF0000FF)  # Blue

    # Test HSL getters
    hue = c.getHue()
    assert isinstance(hue, float)

    saturation = c.getSaturation()
    assert isinstance(saturation, float)

    luminance = c.getLuminance()
    assert isinstance(luminance, float)

#==================================================================================================

def test_color_manipulation():
    c = yup.Color(0xFF808080)  # Gray

    # Test brighter
    brighter_c = c.brighter()
    assert isinstance(brighter_c, yup.Color)

    brighter_c2 = c.brighter(0.5)
    assert isinstance(brighter_c2, yup.Color)

    # Test darker
    darker_c = c.darker()
    assert isinstance(darker_c, yup.Color)

    darker_c2 = c.darker(0.5)
    assert isinstance(darker_c2, yup.Color)

    # Test contrasting
    contrasting_c = c.contrasting()
    assert isinstance(contrasting_c, yup.Color)

    contrasting_c2 = c.contrasting(0.5)
    assert isinstance(contrasting_c2, yup.Color)

#==================================================================================================

def test_color_inversion():
    c = yup.Color(0xFF112233)

    # Test invert (mutating)
    original_argb = c.getARGB()
    c.invert()
    assert c.getARGB() != original_argb

    # Test inverted (non-mutating)
    c2 = yup.Color(0xFF112233)
    inverted_c = c2.inverted()
    assert isinstance(inverted_c, yup.Color)
    assert c2.getARGB() == 0xFF112233  # Original unchanged

    # Test invertAlpha (mutating)
    c3 = yup.Color(0xFF112233)
    original_alpha = c3.getAlpha()
    c3.invertAlpha()
    assert c3.getAlpha() != original_alpha

    # Test invertedAlpha (non-mutating)
    c4 = yup.Color(0xFF112233)
    inverted_alpha_c = c4.invertedAlpha()
    assert isinstance(inverted_alpha_c, yup.Color)
    assert c4.getAlpha() == 0xFF  # Original unchanged

#==================================================================================================

def test_string_conversion():
    c = yup.Color(0xFF112233)

    # Test toString
    str_repr = c.toString()
    assert isinstance(str_repr, str)

    # Test toStringRGB
    rgb_str = c.toStringRGB()
    assert isinstance(rgb_str, str)

    rgb_str_with_alpha = c.toStringRGB(True)
    assert isinstance(rgb_str_with_alpha, str)

    rgb_str_without_alpha = c.toStringRGB(False)
    assert isinstance(rgb_str_without_alpha, str)

#==================================================================================================

def test_equality():
    color1 = yup.Color(0xFF00FF00)
    color2 = yup.Color(0xFF00FF00)
    color3 = yup.Color(0xFFFF0000)

    assert color1 == color2
    assert color1 != color3
    assert not (color1 == color3)
    assert not (color1 != color2)

#==================================================================================================

def test_repr_and_str():
    c = yup.Color(0xFF112233)

    # Test __repr__
    repr_str = repr(c)
    assert isinstance(repr_str, str)
    assert "Color" in repr_str

    # Test __str__
    str_str = str(c)
    assert isinstance(str_str, str)
