import os
import sys
import glob
import time
import traceback
from pathlib import Path
from functools import wraps
from typing import Type, Optional


__all__ = ["START_YUP_COMPONENT", "START_YUP_APPLICATION", "timeit"]


try:
	import yup

except ImportError:
    folder = (Path(__file__).parent.parent / "build")
    for ext in ["*.so", "*.pyd"]:
        path_to_search = folder / "**" / ext
        for f in glob.iglob(str(path_to_search), recursive=True):
            if os.path.isfile(f):
                sys.path.append(str(Path(f).parent))
                break

    import yup


def timeit(func):
    @wraps(func)
    def timeit_wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        result = func(*args, **kwargs)
        total_time = time.perf_counter() - start_time
        print(f'Function {func.__name__} Took {total_time:.4f} seconds') # {args} {kwargs}
        return result

    return timeit_wrapper


def START_YUP_COMPONENT(
    component_class: Type[yup.Component],
    name: str = "YUP Demo",
    width: int = 800,
    height: int = 600,
    alwaysOnTop: bool = False,
    catchExceptionsAndContinue: bool = True,
    **kwargs,
):
    """
    Convenience function to create a window with a given component.

    This wraps the boilerplate of creating a `DocumentWindow`, `YUPApplication`,
    and wiring them together. Simply pass your `Component` subclass and this
    function handles the rest.

    Args:
        component_class: The Component subclass to display.
        name: The window title and application name.
        width: Initial window width in pixels.
        height: Initial window height in pixels.
        alwaysOnTop: Whether the window should stay on top.
        catchExceptionsAndContinue: If True, exceptions in callbacks are logged instead of crashing.
        **kwargs: Additional keyword arguments passed to the component constructor.

    Example:
        >>> class MyComponent(yup.Component):
        ...     def paint(self, g):
        ...         g.setFillColor(yup.Colors.red)
        ...         g.fillAll()
        ...
        >>> if __name__ == "__main__":
        ...     START_YUP_COMPONENT(MyComponent, name="My Demo", width=400, height=300)
    """

    class DemoComponent(component_class):
        def __init__(self):
            component_class.__init__(self, **kwargs)
            self.setOpaque(True)

    class DemoWindow(yup.DocumentWindow):
        component: Optional[yup.Component] = None

        def __init__(self):
            super().__init__()

            self.setTitle(name)

            self.component = DemoComponent()
            self.addAndMakeVisible(self.component)

        def __del__(self):
            self.removeAllChildren()
            if self.component:
                del self.component

        def resized(self):
            if self.component:
                self.component.setBounds(self.getLocalBounds())

        def userTriedToCloseWindow(self):
            yup.YUPApplication.getInstance().systemRequestedQuit()

    class DemoApplication(yup.YUPApplication):
        window: Optional[DemoWindow] = None

        def __init__(self):
            super().__init__()

        def getApplicationName(self):
            return name

        def getApplicationVersion(self):
            return "1.0"

        def initialise(self, commandLineParameters: str):
            self.window = DemoWindow()

            def showWindow():
                yup.Process.makeForegroundProcess()
                self.window.setVisible(True)
                self.window.centreWithSize(yup.Size[int](width, height))

            yup.MessageManager.callAsync(showWindow)

        def shutdown(self):
            if self.window:
                del self.window

        def systemRequestedQuit(self):
            self.quit()

    yup.START_YUP_APPLICATION(DemoApplication)


# Re-export for convenience
START_YUP_APPLICATION = yup.START_YUP_APPLICATION

