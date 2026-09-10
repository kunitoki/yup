import yup

#==================================================================================================

class CollectingLogger(yup.Logger):
    """A Python logger that records everything the current-logger plumbing sends it."""

    def __init__(self):
        super().__init__()
        self.messages = []

    def logMessage(self, message):
        self.messages.append(message)


#==================================================================================================

def test_python_logger_receives_written_messages():
    logger = CollectingLogger()
    yup.Logger.setCurrentLogger(logger)

    try:
        yup.Logger.writeToLog("hello from the test")

        assert logger.messages == ["hello from the test"]
    finally:
        yup.Logger.setCurrentLogger(None)

#==================================================================================================

def test_current_logger_is_the_registered_instance():
    logger = CollectingLogger()
    yup.Logger.setCurrentLogger(logger)

    try:
        assert yup.Logger.getCurrentLogger() is logger
    finally:
        yup.Logger.setCurrentLogger(None)

#==================================================================================================

def test_writing_without_a_logger_does_not_reach_a_cleared_one():
    logger = CollectingLogger()
    yup.Logger.setCurrentLogger(logger)
    yup.Logger.setCurrentLogger(None)

    try:
        yup.Logger.writeToLog("should not be collected")

        assert logger.messages == []
    finally:
        yup.Logger.setCurrentLogger(None)

#==================================================================================================

def test_output_debug_string_is_callable():
    # Writes to the platform's debug output; there is nothing to assert beyond it not
    # throwing, but it is part of the Logger surface exposed to Python.
    yup.Logger.outputDebugString("debug output from the test")

#==================================================================================================

def test_file_logger_writes_and_appends_to_its_file(tmp_path):
    logFile = yup.File(str(tmp_path / "yup-test.log"))
    logger = yup.FileLogger(logFile, "welcome from the test")

    assert logger.getLogFile() == logFile

    logger.logMessage("first message")
    logger.logMessage("second message")

    contents = logFile.loadFileAsString()

    assert "welcome from the test" in contents
    assert "first message" in contents
    assert "second message" in contents

#==================================================================================================

def test_file_logger_can_be_the_current_logger(tmp_path):
    logFile = yup.File(str(tmp_path / "yup-current.log"))
    logger = yup.FileLogger(logFile, "welcome")

    yup.Logger.setCurrentLogger(logger)

    try:
        yup.Logger.writeToLog("routed through the current logger")
    finally:
        yup.Logger.setCurrentLogger(None)

    assert "routed through the current logger" in logFile.loadFileAsString()
