import pytest

import yup

#==================================================================================================

def test_create_returns_valid_metadata():
    metadata = yup.ImageMetadata.create()
    assert metadata is not None
    assert "ImageMetadata" in repr(metadata)

#==================================================================================================

def test_default_values():
    metadata = yup.ImageMetadata.create()
    assert metadata.dpiX == 0.0
    assert metadata.dpiY == 0.0
    assert metadata.getOrientation() == 0
    assert metadata.getCreationDate() == ""
    assert metadata.getCameraMake() == ""
    assert metadata.getCameraModel() == ""
    assert metadata.getImageDescription() == ""
    assert metadata.getCopyright() == ""
    assert metadata.getSoftware() == ""

#==================================================================================================

def test_dpi_roundtrip():
    metadata = yup.ImageMetadata.create()
    metadata.dpiX = 300.0
    metadata.dpiY = 150.0
    assert metadata.dpiX == 300.0
    assert metadata.dpiY == 150.0

#==================================================================================================

def test_raw_chunks():
    metadata = yup.ImageMetadata.create()
    assert not metadata.hasRawChunk("xmp")
    assert metadata.getRawChunk("xmp") is None

#==================================================================================================

def test_gps_coordinates_default():
    metadata = yup.ImageMetadata.create()
    lat, lon = metadata.getGpsCoordinates()
    assert lat == 0.0
    assert lon == 0.0

#==================================================================================================

def test_attachable_to_image():
    image = yup.Image(2, 2)
    metadata = yup.ImageMetadata.create()
    metadata.dpiX = 72.0
    image.setMetadata(metadata)
    assert image.hasMetadata()
    assert image.getMetadata().dpiX == 72.0
