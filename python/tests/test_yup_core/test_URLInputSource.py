import yup

#==================================================================================================

def test_url_input_source_constructor():
    url = yup.URL("https://github.com")
    input_source = yup.URLInputSource(url)
    assert input_source is not None
    assert input_source.hashCode() != 0

#==================================================================================================

def test_create_stream():
    url = yup.URL("https://github.com/kunitoki/popsicle")
    input_source = yup.URLInputSource(url)
    stream = input_source.createInputStream()
    assert stream is not None

#==================================================================================================

def test_create_stream_with_post_data():
    url = yup.URL("https://github.com")
    input_source = yup.URLInputSource(url)
    stream = input_source.createInputStreamFor("kunitoki/popsicle")
    assert stream is not None
