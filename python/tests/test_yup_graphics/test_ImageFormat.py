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

#==================================================================================================

def test_options_repr():
    options = yup.ImageFormat.Options()
    options.parseMetadata = True
    options.parseRawChunks = False
    assert repr(options).endswith("(metadata, no-raw-chunks)")

    options.parseMetadata = False
    options.parseRawChunks = True
    assert repr(options).endswith("(no-metadata, raw-chunks)")

def test_possible_pixel_formats():
    # A format without Python overrides reports no pixel formats.
    assert yup.ImageFormat().getPossiblePixelFormats() == []

    if hasattr(yup, "PngImageFormat"):
        assert len(yup.PngImageFormat().getPossiblePixelFormats()) > 0

def test_manager_reads_with_options_and_writes_with_settings(tmp_path):
    if not hasattr(yup, "PngImageFormat"):
        pytest.skip("PNG support not compiled in")

    from utilities import get_test_data_file

    source = get_test_data_file("images/file_example.png")
    if not source.existsAsFile():
        pytest.skip("tests/data/images/file_example.png is not available")

    manager = yup.ImageFormatManager()
    manager.registerDefaultFormats()

    reader = manager.createReaderFor(source, yup.ImageFormat.Options())
    assert reader is not None
    assert reader.readImage().isValid()
    del reader

    output = yup.File(str(tmp_path / "written.png"))
    writer = manager.createWriterFor(output, yup.PixelFormat.RGBA, yup.StringPairArray(), 0)
    assert writer is not None
    assert writer.writeImage(yup.Image(4, 4, yup.PixelFormat.RGBA))
    del writer

    assert output.existsAsFile()
