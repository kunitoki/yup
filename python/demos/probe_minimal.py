#!/usr/bin/env python3
"""Minimal window probe - no helper, no multiple inheritance, no timer.

Bisects "I don't see the window": each stage prints, so the last line tells you
how far the app actually got. A red window means the whole path works.
"""

import yup_init
import yup


class MinimalComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        print("[probe] component constructed", flush=True)

    def paint(self, g: yup.Graphics):
        print(f"[probe] paint {self.getWidth()}x{self.getHeight()}", flush=True)
        g.setFillColor(yup.Colors.red)
        g.fillAll()


class MinimalWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("probe minimal")
        self.component = MinimalComponent()
        self.addAndMakeVisible(self.component)
        print("[probe] window constructed", flush=True)

    def resized(self):
        print(f"[probe] resized {self.getWidth()}x{self.getHeight()}", flush=True)
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class MinimalApplication(yup.YUPApplication):
    def getApplicationName(self):
        return "probe minimal"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        print("[probe] initialise", flush=True)
        self.window = MinimalWindow()

        def showWindow():
            print("[probe] showWindow callAsync fired", flush=True)
            # YUP's MessageManager creates NSApp before SDL video init, so SDL skips
            # its own setActivationPolicy:Regular (that call sits inside `if (NSApp ==
            # nil)`). A non-bundled host like python then stays a non-UI process: the
            # window is created and painted but never displayed, and there is no Dock
            # icon. TransformProcessType fixes the policy; activate then works.
            yup.Process.setDockIconVisible(True)
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](600, 450))
            print(f"[probe] after show: {self.window.getWidth()}x{self.window.getHeight()}", flush=True)

        yup.MessageManager.callAsync(showWindow)
        print("[probe] initialise done (callAsync queued)", flush=True)

    def shutdown(self):
        print("[probe] shutdown", flush=True)
        del self.window

    def systemRequestedQuit(self):
        print("[probe] systemRequestedQuit", flush=True)
        self.quit()


if __name__ == "__main__":
    print("[probe] starting", flush=True)
    yup.START_YUP_APPLICATION(MinimalApplication)
    print("[probe] returned from START_YUP_APPLICATION", flush=True)
