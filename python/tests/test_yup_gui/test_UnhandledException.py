"""Regression tests for Python exceptions escaping C++-invoked overrides.

An exception raised inside a Python override that C++ calls back into - `timerCallback`,
`paint`, `messageCallback` - is a `pybind11::error_already_set`, which is a `std::exception`.
YUP already has the machinery to route one to the application: the dispatch loops wrap their
body in `YUP_TRY`/`YUP_CATCH_EXCEPTION`, which calls `YUPApplicationBase::sendUnhandledException`,
which calls `unhandledException` on the application - and `PyYUPApplication` forwards that to
Python with the traceback rebuilt.

The macOS dispatch loops were the gap: `yup_MessageManager.cpp` is guarded by
`#if ! (YUP_MAC || YUP_IOS || YUP_WASM)`, and the `.mm` replacements had no C++ catch at all,
only an `@catch (NSException*)` - a disjoint set. So on macOS the exception escaped the loop
entirely and `unhandledException` was unreachable.

These tests fail on macOS before that fix and pass after it.
"""

import pytest

import yup

from utilities import pump_until


class RaisingTimer(yup.Timer):
    """Raises from the C++ -> Python callback, the way a faulty `paint` would."""

    def __init__(self, message):
        super().__init__()
        self.message = message
        self.timesCalled = 0

    def timerCallback(self):
        self.timesCalled += 1
        raise ValueError(self.message)


def make_recording_application(recorded):
    class Application(yup.YUPApplication):
        def getApplicationName(self):
            return "UnhandledExceptionTestApp"

        def getApplicationVersion(self):
            return "1.0"

        def initialise(self, commandLineParameters: str):
            pass

        def shutdown(self):
            pass

        def unhandledException(self, exception, sourceFilename, lineNumber):
            recorded.append((exception, sourceFilename, lineNumber))

    return Application


@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter has no test application")
def test_exception_in_timer_callback_reaches_unhandledException():
    recorded = []

    with yup.TestApplication(make_recording_application(recorded)) as app:
        assert not yup.MessageManager.getInstance().hasStopMessageBeenSent(), \
            "the shared dispatch loop was already latched shut by an earlier test"

        timer = RaisingTimer("boom from timerCallback")
        timer.startTimer(1)

        try:
            reached = pump_until(app, lambda: len(recorded) > 0)
            timesCalled = timer.timesCalled
        finally:
            timer.stopTimer()

    # Split the two failure modes: a timer that never fired is not a routing problem.
    assert timesCalled > 0, "timerCallback never fired at all"
    assert reached, "unhandledException was never called - the exception escaped the dispatch loop"

    exception, sourceFilename, lineNumber = recorded[0]
    assert isinstance(exception, ValueError)
    assert "boom from timerCallback" in str(exception)
    assert isinstance(sourceFilename, str) and sourceFilename
    assert isinstance(lineNumber, int) and lineNumber > 0


@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter has no test application")
def test_dispatch_loop_survives_a_raising_override():
    """Catching is only half of it - the loop has to keep dispatching afterwards."""
    recorded = []

    with yup.TestApplication(make_recording_application(recorded)) as app:
        assert not yup.MessageManager.getInstance().hasStopMessageBeenSent(), \
            "the shared dispatch loop was already latched shut by an earlier test"

        timer = RaisingTimer("boom, repeatedly")
        timer.startTimer(1)

        try:
            assert pump_until(app, lambda: len(recorded) > 0), "first exception never reported"

            # An independent callback posted *after* the failure must still land, which it
            # cannot if the exception unwound the loop or latched the quit flag.
            landed = []
            yup.MessageManager.callAsync(lambda: landed.append(True))

            delivered = pump_until(app, lambda: bool(landed))
            timesCalledAfterFailure = timer.timesCalled
        finally:
            timer.stopTimer()

    assert delivered, "the loop stopped dispatching after an override raised"
    assert timesCalledAfterFailure > 1, "the timer was never invoked again after it raised"
