import sys
import pytest

import yup

#==================================================================================================

@pytest.mark.skipif(sys.platform == "linux", reason="Linux doesn't always have a working libcurl in CI")
def test_url_input_source_constructor():
    url = yup.URL("https://github.com")
    input_source = yup.URLInputSource(url)
    assert input_source is not None
    assert input_source.hashCode() != 0

#==================================================================================================

@pytest.mark.skipif(sys.platform == "linux", reason="Linux doesn't always have a working libcurl in CI")
def test_create_stream():
    url = yup.URL("https://github.com/kunitoki/yup")
    input_source = yup.URLInputSource(url)
    stream = input_source.createInputStream()
    assert stream is not None

#==================================================================================================

@pytest.mark.skipif(sys.platform == "linux", reason="Linux doesn't always have a working libcurl in CI")
def test_create_stream_with_post_data():
    url = yup.URL("https://github.com")
    input_source = yup.URLInputSource(url)
    stream = input_source.createInputStreamFor("kunitoki/yup")
    assert stream is not None
