import time
import pytest

import yup

#==================================================================================================

class CustomTimer(yup.Timer):
    timesCalled = 0

    def timerCallback(self):
        self.timesCalled += 1

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_single_timer(juce_app):
    t = CustomTimer()
    assert not t.isTimerRunning()

    repeats = 2
    milliseconds = 100

    t.startTimer(milliseconds)
    assert t.getTimerInterval() == milliseconds
    assert t.isTimerRunning()
    assert t.timesCalled == 0

    while t.timesCalled < repeats:
        juce_app.processEvents(milliseconds)

    assert t.isTimerRunning()
    assert t.timesCalled >= repeats

    t.stopTimer()
    assert not t.isTimerRunning()

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_call_pending_timers_synchronously(juce_app):
    t = CustomTimer()

    t.startTimer(100)
    assert t.timesCalled == 0
    juce_app.processEvents(1)
    #assert t.timesCalled == 0

    time.sleep(0.2)

    yup.Timer.callPendingTimersSynchronously()
    #assert t.timesCalled >= 1

    t.stopTimer()

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_call_after_delay(juce_app):
    milliseconds = 100
    called = False

    def callback():
        nonlocal called
        called = True

    yup.Timer.callAfterDelay(milliseconds, callback)
    assert not called

    while not called:
        juce_app.processEvents(20)

    assert called
