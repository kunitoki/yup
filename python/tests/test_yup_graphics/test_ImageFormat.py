import pytest

import yup

#==================================================================================================

def test_pixel_format_values():
    assert yup.PixelFormat.Grayscale.name == "Grayscale"
    assert yup.PixelFormat.RGB.name == "RGB"
    assert yup.PixelFormat.RGBA.name == "RGBA"

#==================================================================================================

def test_options_defaults():
    options = yup.ImageFormat.Options()
    assert options.parseMetadata is False
    assert options.parseRawChunks is False

def test_options_with_metadata_and_raw_chunks():
    options = yup.ImageFormat.Options()
    options.withMetadata(True)
    options.withRawChunks(True)
    assert options.parseMetadata is True
    assert options.parseRawChunks is True

def test_options_chaining():
    options = yup.ImageFormat.Options().withMetadata(True).withRawChunks(False)
    assert options.parseMetadata is True
    assert options.parseRawChunks is False

#==================================================================================================

def test_mode_enum():
    assert yup.ImageFormat.Mode.forReading.name == "forReading"
    assert yup.ImageFormat.Mode.forWriting.name == "forWriting"

#==================================================================================================

def test_png_format_basics():
    if not hasattr(yup, "PngImageFormat"):
        pytest.skip("PNG support not compiled in")

    fmt = yup.PngImageFormat()
    assert fmt.isCompressed()
    assert len(fmt.getFormatName()) > 0
    assert len(fmt.getQualityOptions()) >= 0

def test_jpeg_format_is_compressed():
    if not hasattr(yup, "JpegImageFormat"):
        pytest.skip("JPEG support not compiled in")

    fmt = yup.JpegImageFormat()
    assert fmt.isCompressed()
    assert len(fmt.getFormatName()) > 0

def test_bmp_format_is_not_compressed():
    if not hasattr(yup, "BmpImageFormat"):
        pytest.skip("BMP support not compiled in")

    fmt = yup.BmpImageFormat()
    assert not fmt.isCompressed()

#==================================================================================================

def test_manager_register_default_formats():
    manager = yup.ImageFormatManager()
    manager.registerDefaultFormats()
    manager.registerDefaultFormats(yup.ImageFormatType.all)

def test_image_format_type_flags():
    assert yup.ImageFormatType.all.name == "all"
    assert yup.ImageFormatType.png.name == "png"

#==================================================================================================

def test_image_repr():
    assert "Image" in repr(yup.Image(4, 4))
