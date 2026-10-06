import sys
from pathlib import Path

import pytest

tests_folder = str(Path(__file__).parent)

if tests_folder not in sys.path:
    sys.path.insert(0, tests_folder)

import common
from utilities import get_runtime_data_folder, remove_directory_recursively

import yup

#==================================================================================================

def pytest_addoption(parser):
    parser.addoption("--update-rendering", action="store_true", default=False)

def pytest_generate_tests(metafunc):
    option_value = metafunc.config.option.update_rendering
    if "update_rendering" in metafunc.fixturenames and option_value:
        metafunc.parametrize("update_rendering", [option_value])

def pytest_unconfigure(config):
    if sys.gettrace() is not None:
        return

    remove_directory_recursively(get_runtime_data_folder().getFullPathName(), [".gitignore"])

#==================================================================================================

def yield_test():
    yield

@pytest.fixture
def juce_app():
    if not hasattr(yup, "TestApplication"):
        pytest.skip("yup.TestApplication requires a non-embedded build of the bindings")

    class Application(yup.YUPApplication):
        def __init__(self):
            super().__init__()

        def getApplicationName(self):
            return "TestApp"

        def getApplicationVersion(self):
            return "1.0"

        def initialise(self, commandLineParameters: str):
            yup.MessageManager.callAsync(yield_test)

        def shutdown(self):
            pass

    with yup.TestApplication(Application) as app:
        next(app)
        yield app
        next(app)

#==================================================================================================

@pytest.fixture
def gui_app(request):
    """The running application, for tests that only need one to exist.

    Under the embedded interpreter the host process already runs an application with a message
    thread, so nothing is started. Otherwise this falls back to juce_app.
    """
    if getattr(yup, "__embedded_interpreter__", False):
        yield None
    else:
        yield request.getfixturevalue("juce_app")
