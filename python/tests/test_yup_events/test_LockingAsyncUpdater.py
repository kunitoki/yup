import pytest

import yup

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_single_trigger(juce_app):
    timesCalled = 0

    def callback():
        nonlocal timesCalled
        timesCalled += 1

    a = yup.LockingAsyncUpdater(callback)
    assert not a.isUpdatePending()

    a.triggerAsyncUpdate()
    assert a.isUpdatePending()
    assert timesCalled == 0
    next(juce_app)
    assert timesCalled == 1

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_multiple_trigger(juce_app):
    timesCalled = 0

    def callback():
        nonlocal timesCalled
        timesCalled += 1

    a = yup.LockingAsyncUpdater(callback)

    a.triggerAsyncUpdate()
    a.triggerAsyncUpdate()
    a.triggerAsyncUpdate()
    a.triggerAsyncUpdate()
    assert a.isUpdatePending()
    assert timesCalled == 0
    next(juce_app)
    assert not a.isUpdatePending()
    assert timesCalled == 1

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_cancel_single_trigger(juce_app):
    timesCalled = 0

    def callback():
        nonlocal timesCalled
        timesCalled += 1

    a = yup.LockingAsyncUpdater(callback)

    a.triggerAsyncUpdate()
    assert a.isUpdatePending()
    assert timesCalled == 0
    a.cancelPendingUpdate()
    assert not a.isUpdatePending()
    next(juce_app)
    assert not a.isUpdatePending()
    assert timesCalled == 0

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_handle_update_now(juce_app):
    timesCalled = 0

    def callback():
        nonlocal timesCalled
        timesCalled += 1

    a = yup.LockingAsyncUpdater(callback)

    a.handleUpdateNowIfNeeded()
    assert not a.isUpdatePending()
    assert timesCalled == 0

    a.triggerAsyncUpdate()
    assert a.isUpdatePending()
    a.handleUpdateNowIfNeeded()
    assert not a.isUpdatePending()
    assert timesCalled == 1
