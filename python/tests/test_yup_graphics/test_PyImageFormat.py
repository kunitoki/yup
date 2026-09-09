import pytest

import yup

#==================================================================================================
# A tiny, fully Python-implemented image format ("YUPR": raw RGBA with a 12-byte header).
#==================================================================================================

_MAGIC = b"YUPR"


def _encode_payload(image: yup.Image) -> bytes:
    width = image.getWidth()
    height = image.getHeight()
    assert image.getPixelFormat() == yup.PixelFormat.RGBA

    return _MAGIC + width.to_bytes(4, "big") + height.to_bytes(4, "big") + image.getRawData()


class YupRawReader(yup.ImageFormatReader):
    def readImage(self) -> yup.Image:
        data = self.getSourceBytes()
        assert data[:4] == _MAGIC
        width = int.from_bytes(data[4:8], "big")
        height = int.from_bytes(data[8:12], "big")

        image = yup.Image(width, height, yup.PixelFormat.RGBA)
        offset = 12
        for y in range(height):
            for x in range(width):
                i = offset + (y * width + x) * 4
                r, g, b, a = data[i], data[i + 1], data[i + 2], data[i + 3]
                image.setPixelColor(x, y, yup.Color(a, r, g, b))

        return image


class YupRawWriter(yup.ImageFormatWriter):
    def writeImage(self, image: yup.Image) -> bool:
        self.writeRawData(_encode_payload(image))
        return True


class YupRawFormat(yup.ImageFormat):
    def getFormatName(self) -> str:
        return "YUP Raw Image"

    def getFileExtensions(self, mode) -> list:
        return [".yupr"]

    def getPossiblePixelFormats(self) -> list:
        return [yup.PixelFormat.RGBA]

    def canHandleFile(self, file, mode) -> bool:
        # Real detection happens in canHandleStream(); accepting every file here
        # is fine for the tests below, which only use .yupr files.
        return True

    def canHandleStream(self, stream, mode) -> bool:
        if mode != yup.ImageFormat.Mode.forReading:
            return False

        stream.setPosition(0)
        data = yup.ImageFormatReader.readAllBytes(stream)
        stream.setPosition(0)

        return data.startswith(_MAGIC)

    def createReaderFor(self, stream, options=None):
        stream.setPosition(0)
        data = yup.ImageFormatReader.readAllBytes(stream)
        stream.setPosition(0)

        if not data.startswith(_MAGIC):
            return None

        return YupRawReader(data, self.getFormatName())

    def createWriterFor(self, stream, pixelFormat, metadataValues=None, qualityOptionIndex=0):
        return YupRawWriter(stream, self.getFormatName(), pixelFormat)


#==================================================================================================
# Helpers

def _make_test_image(width=3, height=2) -> yup.Image:
    image = yup.Image(width, height, yup.PixelFormat.RGBA)
    for y in range(height):
        for x in range(width):
            value = (x * 3 + y * 5) & 0xFF
            image.setPixelColor(x, y, yup.Color(255, value, (value + 40) & 0xFF, (value + 80) & 0xFF))
    return image


def _assert_same_pixels(expected: yup.Image, actual: yup.Image):
    assert actual.getWidth() == expected.getWidth()
    assert actual.getHeight() == expected.getHeight()
    for y in range(expected.getHeight()):
        for x in range(expected.getWidth()):
            assert actual.getPixelColor(x, y) == expected.getPixelColor(x, y)


#==================================================================================================

def test_format_subclass_overrides():
    fmt = YupRawFormat()
    assert fmt.getFormatName() == "YUP Raw Image"
    assert fmt.getFileExtensions(yup.ImageFormat.Mode.forReading) == [".yupr"]
    assert fmt.getPossiblePixelFormats() == [yup.PixelFormat.RGBA]
    assert not fmt.isCompressed()

#==================================================================================================

def test_reader_direct_decode():
    source = _make_test_image()
    reader = YupRawReader(_encode_payload(source), "YUP Raw Image")

    decoded = reader.readImage()
    _assert_same_pixels(source, decoded)

#==================================================================================================

def test_in_memory_writer_encode():
    source = _make_test_image()

    # (formatName, pixelFormat) constructs the Python subclass writer around an
    # internal buffer; writeImage() runs the Python override.
    writer = YupRawWriter("YUP Raw Image", yup.PixelFormat.RGBA)
    assert writer.writeImage(source)
    data = writer.getOutputBytes()

    assert data[:4] == _MAGIC
    assert len(data) == 12 + source.getWidth() * source.getHeight() * 4

#==================================================================================================

def test_manager_registration_and_detection():
    manager = yup.ImageFormatManager()
    manager.registerDefaultFormats()
    manager.registerFormat(YupRawFormat())

    extensions = [str(e) for e in manager.getFormatFileExtensions()]
    assert ".yupr" in extensions

#==================================================================================================

def test_register_format_rejects_non_format():
    manager = yup.ImageFormatManager()
    with pytest.raises(TypeError):
        manager.registerFormat(yup.Image(2, 2))

#==================================================================================================

def test_format_dispatch_through_manager(tmp_path):
    manager = yup.ImageFormatManager()
    manager.registerFormat(YupRawFormat())

    source = _make_test_image()
    outputFile = yup.File(str(tmp_path / "output.yupr"))

    # Encode: the manager hands a file stream to the Python format, whose
    # createWriterFor() returns the Python subclass writer.
    writer = manager.createWriterFor(outputFile)
    assert writer is not None
    assert writer.writeImage(source)
    writer.flush()
    del writer

    assert outputFile.existsAsFile()
    assert outputFile.getSize() == 12 + source.getWidth() * source.getHeight() * 4

    # Decode: the manager builds a reader through the Python overrides, and
    # readImage() dispatches back to the Python subclass after the round trip.
    reader = manager.createReaderFor(outputFile)
    assert reader is not None
    assert reader.getFormatName() == "YUP Raw Image"

    decoded = reader.readImage()
    _assert_same_pixels(source, decoded)
