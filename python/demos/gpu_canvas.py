#!/usr/bin/env python3
"""
YUP GPU Canvas Demo

Follows the OffscreenRenderDemo pattern: renders 2D content to an
offscreen GpuCanvas via beginDraw()/commit(), then composites the
result onto the screen with drawImage().
"""

import yup_init
import yup
import math
import time


class CanvasWindow(yup.DocumentWindow):
    def __init__(self):
        super().__init__()
        self.setTitle("GPU Canvas Demo")
        self.component = CanvasComponent()
        self.addAndMakeVisible(self.component)

    def resized(self):
        self.component.setBounds(self.getLocalBounds())

    def userTriedToCloseWindow(self):
        yup.YUPApplication.getInstance().systemRequestedQuit()


class CanvasComponent(yup.Component):
    CANVAS_SIZE = 512

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self._ctx = None
        self._canvas = None
        self._image = None
        self._startTime = time.perf_counter()
        self.timer = yup.Timer(self.onTimer)
        self.timer.startTimerHz(30)

    def onTimer(self):
        self.repaint()

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        pass

    # ------------------------------------------------------------------
    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()

        if self._ctx is None:
            self._ctx = g.getGraphicsContext()

        if self._ctx is None or not self._ctx.isGpuAvailable():
            return

        if self._canvas is None:
            self._canvas = yup.GpuCanvas.create(self._ctx, self.CANVAS_SIZE, self.CANVAS_SIZE)
        if self._canvas is None:
            return

        # ---- 2D draw to offscreen canvas ----
        g2 = self._canvas.beginDraw()
        try:
            w = self._canvas.getWidth()
            h = self._canvas.getHeight()
            t = time.perf_counter() - self._startTime

            g2.setFillColor(yup.Colors.black)
            g2.fillAll()

            for i in range(8):
                angle = t * 2.0 + i * math.pi / 4
                cx = w / 2 + math.cos(angle) * 150
                cy = h / 2 + math.sin(angle) * 150
                r = 20 + math.sin(t * 3 + i) * 10
                hue = (i / 8.0 + t * 0.2) % 1.0
                g2.setFillColor(yup.Color.fromHSV(hue, 0.8, 1.0, 0.8))
                g2.fillEllipse(cx - r, cy - r, r * 2, r * 2)

            g2.setStrokeColor(yup.Colors.white)
            g2.setStrokeWidth(3)
            g2.strokeRect(yup.Rectangle[float](10, 10, w - 20, h - 20))

            font = yup.Font(yup.FontOptions(28.0))
            g2.setFillColor(yup.Colors.white)
            g2.fillFittedText(
                "GPU Canvas", font,
                yup.Rectangle[float](0, h - 50, w, 40),
                yup.Justification.centred,
            )
        finally:
            self._canvas.commit()

        self._image = self._canvas.asImage()

        # ---- Composite to screen ----
        if self._image.isValid():
            iw = float(self._image.getWidth())
            ih = float(self._image.getHeight())
            w = float(self.getWidth())
            h = float(self.getHeight())
            scale = min(w / iw, h / ih) * 0.9
            dw = iw * scale
            dh = ih * scale
            g.drawImage(
                self._image,
                yup.Rectangle[float]((w - dw) / 2, (h - dh) / 2, dw, dh),
            )

        apiNames = {0: "Headless", 1: "OpenGL", 2: "OpenGL ES",
                    3: "Direct3D", 4: "Metal", 5: "WebGPU"}
        api = apiNames.get(self._ctx.getApi(), "?")
        font = yup.Font(yup.FontOptions(14.0))
        g.setFillColor(yup.Colors.white.withAlpha(0.7))
        g.fillFittedText(
            f"GPU: {api}", font,
            yup.Rectangle[float](10, h - 25, 200, 20),
            yup.Justification.left,
        )


class Application(yup.YUPApplication):
    def getApplicationName(self):
        return "GPU Canvas Demo"

    def getApplicationVersion(self):
        return "1.0"

    def initialise(self, commandLineParameters: str):
        self.window = CanvasWindow()

        def show():
            yup.Process.makeForegroundProcess()
            self.window.setVisible(True)
            self.window.centreWithSize(yup.Size[int](640, 640))

        yup.MessageManager.callAsync(show)

    def shutdown(self):
        del self.window

    def systemRequestedQuit(self):
        self.quit()


if __name__ == "__main__":
    yup.START_YUP_APPLICATION(Application)
