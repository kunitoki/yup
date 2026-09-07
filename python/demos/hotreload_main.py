#!/usr/bin/env python3
"""
YUP Hot Reload Demo - Main

Demonstrates a hot-reload pattern where the component is reloaded
from disk when the file changes. Run this script, then edit
hotreload_component.py while the window is open to see changes.

Port of popsicle's hotreload_main.py.
"""

import yup_init
import yup
import importlib
import os
import sys
import time

# Make sure the demos directory is in the path
sys.path.insert(0, os.path.dirname(__file__))


class HotReloadWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("Hot Reload Demo")
        self.component = None
        self.last_mtime = 0
        self.reload_component()
        self.timer = yup.Timer(self.checkReload)
        self.timer.startTimer(500)

    def reload_component(self):
        try:
            import hotreload_component

            importlib.reload(hotreload_component)

            if self.component:
                self.removeChildComponent(self.component)
                del self.component

            self.component = hotreload_component.DynamicComponent()
            self.addAndMakeVisible(self.component)
            self.component.setBounds(self.getLocalBounds())

            print(f"[HotReload] Component reloaded successfully")
        except Exception as e:
            print(f"[HotReload] Error reloading component: {e}")
            import traceback

            traceback.print_exc()

    def checkReload(self):
        try:
            comp_path = os.path.join(
                os.path.dirname(__file__), "hotreload_component.py"
            )
            current_mtime = os.path.getmtime(comp_path)
            if current_mtime > self.last_mtime:
                self.last_mtime = current_mtime
                print(f"[HotReload] File changed, reloading...")
                self.reload_component()
        except Exception:
            pass

    def resized(self):
        if self.component:
            self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class Application(yup.YUPApplication):
    window = None

    def getApplicationName(self):
        return "Hot Reload Demo"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = HotReloadWindow()

        def showWindow():
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](600, 450))

        yup.MessageManager.callAsync(showWindow)

    def shutdown(self):
        if self.window:
            del self.window

    def systemRequestedQuit(self):
        self.quit()


if __name__ == "__main__":
    yup.START_YUP_APPLICATION(Application)
