import pytest

import yup

#==================================================================================================

def test_construct_default_is_invalid():
    image = yup.Image()
    assert not image.isValid()
    assert image.getWidth() == 0
    assert image.getHeight() == 0

#==================================================================================================

def test_construct_with_dimensions():
    image = yup.Image(4, 3)
    assert image.isValid()
    assert image.getWidth() == 4
    assert image.getHeight() == 3
    assert image.getPixelFormat() == yup.PixelFormat.RGBA
    assert image.getPixelStride() == 4

def test_construct_with_format():
    image = yup.Image(2, 2, yup.PixelFormat.RGB)
    assert image.isValid()
    assert image.getPixelFormat() == yup.PixelFormat.RGB
    assert image.getPixelStride() == 3

#==================================================================================================

def test_copy_construct_sees_same_data():
    image = yup.Image(2, 2)
    image.fill(0xFFFF0000)

    copy = yup.Image(image)
    assert copy.isValid()
    assert copy.getPixel(0, 0) == 0xFFFF0000
    assert copy.getPixel(1, 1) == 0xFFFF0000

#==================================================================================================

def test_fill_and_get_pixel_roundtrip():
    image = yup.Image(3, 3)
    image.fill(0xFF00FF00)
    assert image.getPixel(0, 0) == 0xFF00FF00
    assert image.getPixel(2, 2) == 0xFF00FF00

def test_set_pixel_roundtrip():
    image = yup.Image(2, 2)
    image.setPixel(1, 0, 0xFF0000FF)
    assert image.getPixel(0, 0) == 0
    assert image.getPixel(1, 0) == 0xFF0000FF

def test_set_pixel_color_roundtrip():
    image = yup.Image(2, 2)
    color = yup.Color(255, 0, 0, 255)  # (alpha, red, green, blue)
    image.setPixelColor(0, 1, color)
    assert image.getPixelColor(0, 1) == color

def test_clear_zeroes_image():
    image = yup.Image(2, 2)
    image.fill(0xFFFFFFFF)
    image.clear()
    assert image.getPixel(0, 0) == 0
    assert image.getPixel(1, 1) == 0

#==================================================================================================

def test_get_pixel_data():
    image = yup.Image(4, 4)
    image.fill(0xFF123456)

    pixelData = image.getPixelData()
    assert pixelData.getWidth() == 4
    assert pixelData.getHeight() == 4
    assert pixelData.getPixelFormat() == yup.PixelFormat.RGBA
    assert pixelData.getPixelStride() == 4
    assert pixelData.getPixel(0, 0) == 0xFF123456

def test_get_pixel_data_mutates_image():
    image = yup.Image(2, 2)
    pixelData = image.getPixelData()
    pixelData.setPixel(1, 1, 0xFFABCDEF)
    assert image.getPixel(1, 1) == 0xFFABCDEF

def test_get_raw_data_size():
    image = yup.Image(4, 3, yup.PixelFormat.RGBA)
    raw = image.getRawData()
    assert len(raw) == 4 * 3 * 4

    gray = yup.Image(4, 3, yup.PixelFormat.Grayscale)
    assert len(gray.getRawData()) == 4 * 3

#==================================================================================================

def test_duplicate_is_independent_copy():
    image = yup.Image(2, 2)
    image.fill(0xFF00FF00)

    copy = image.duplicate()
    assert copy.isValid()
    copy.setPixel(0, 0, 0xFF000000)
    assert image.getPixel(0, 0) == 0xFF00FF00
    assert copy.getPixel(0, 0) == 0xFF000000

#==================================================================================================

def test_metadata_attach():
    image = yup.Image(1, 1)
    assert not image.hasMetadata()

    metadata = yup.ImageMetadata.create()
    metadata.dpiX = 72.0
    image.setMetadata(metadata)

    assert image.hasMetadata()
    assert image.getMetadata().dpiX == 72.0

#==================================================================================================

def test_load_from_data_rejects_garbage():
    with pytest.raises(ValueError):
        yup.Image.loadFromData(b"this is definitely not a valid image payload")

def test_load_from_data_empty_raises():
    with pytest.raises(ValueError):
        yup.Image.loadFromData(b"")

#==================================================================================================

def test_repr():
    assert "3x2" in repr(yup.Image(3, 2))
    assert "null" in repr(yup.Image())
    assert "valid" in repr(yup.Image(3, 2))
