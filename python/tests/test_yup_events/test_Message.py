import pytest

import yup

#==================================================================================================

timesCalled = 0

class Message(yup.Message):
    def messageCallback(self):
        global timesCalled
        timesCalled += 1

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_construct_and_post(juce_app):
    global timesCalled

    m = Message()
    assert timesCalled == 0

    m.post()
    assert timesCalled == 0

    next(juce_app)
    assert timesCalled == 1
