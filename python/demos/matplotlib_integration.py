#!/usr/bin/env python3
"""
YUP Matplotlib Integration Demo

Renders a Matplotlib figure in a separate process - Matplotlib is not thread safe, so the
chart is built and rasterised off the user interface thread - hands the resulting PNG back
through a queue, and fades it into the window while a star spinner turns behind it.

NOTE: Requires 'matplotlib' and 'numpy'.
    uv pip install matplotlib numpy
"""

import io
import multiprocessing
import queue
import time

import yup_init
import yup

try:
    import numpy as np
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_agg import FigureCanvasAgg
except ImportError:
    raise ImportError(
        "This demo requires matplotlib and numpy (uv pip install matplotlib numpy)"
    )


#: How long the fade of the freshly rendered chart takes, in milliseconds.
FADE_IN_MILLIS = 3000.0

#: How often the rendering process is polled for the finished chart.
POLL_HZ = 24


def make_plot(fig):
    x = np.linspace(0, 10, 11)
    y = [3.9, 4.4, 10.8, 10.3, 11.2, 13.1, 14.1, 9.9, 13.9, 15.1, 12.5]

    a, b = np.polyfit(x, y, deg=1)
    y_est = a * x + b
    y_err = x.std() * np.sqrt(1 / len(x) + (x - x.mean()) ** 2 / np.sum((x - x.mean()) ** 2))

    ax = fig.add_subplot(111)
    ax.plot(x, y_est, "-")
    ax.fill_between(x, y_est - y_err, y_est + y_err, alpha=0.2)
    ax.plot(x, y, "o", color="tab:brown")


def generate_plot_png(q, width, height):
    """Builds the figure, rasterises it to PNG bytes and puts them on the queue."""
    fig = Figure(figsize=(width / 100, height / 100), dpi=100)
    canvas = FigureCanvasAgg(fig)

    make_plot(fig)
    canvas.draw()

    with io.BytesIO() as img_buf:
        canvas.print_png(img_buf)
        q.put(img_buf.getbuffer().tobytes())


class DrawableImage(yup.Component):
    """Child component that paints a single image, faded in by its parent.

    Deliberately not opaque: an opaque child covering its parent's whole area makes YUP
    skip the parent's paint() (Component::hasOpaqueChildCoveringArea), and that paint is
    what draws the white background and the spinner behind this chart.
    """

    def __init__(self):
        yup.Component.__init__(self)

        self.setOpaque(False)
        self.image = yup.Image()

    def setImage(self, image: yup.Image):
        self.image = image
        self.repaint()

    def getImage(self) -> yup.Image:
        return self.image

    def paint(self, g: yup.Graphics):
        if self.image.isValid():
            g.drawImage(self.image, self.getLocalBounds())


class MainContentComponent(yup.Component, yup.Timer):
    width = 600
    height = 400
    time = 0.0

    def __init__(self):
        yup.Component.__init__(self)
        yup.Timer.__init__(self)

        self.process = None
        self.poller = None

        self.drawableImage = DrawableImage()
        self.drawableImage.setOpacity(0.0)
        self.addAndMakeVisible(self.drawableImage)

        self.setSize(float(self.width), float(self.height))
        self.setOpaque(True)

        self.fadeStartTime = None

        self.q = multiprocessing.Queue()
        self.process = multiprocessing.Process(target=generate_plot_png, args=[self.q, self.width, self.height])
        self.process.start()

        self.startTimerHz(POLL_HZ)

    def __del__(self):
        self.stopTimer()

        if self.process is not None:
            if self.process.is_alive():
                self.process.terminate()

            self.process.join()

    def timerCallback(self):
        self.pollForChart()

    def pollForChart(self):
        if not self.drawableImage.getImage().isValid():
            # No chart yet - poll the rendering process until it hands one over.
            try:
                image_data = self.q.get_nowait()
            except queue.Empty:
                pass
            else:
                self.drawableImage.setImage(self.createImageFromBuffer(image_data))
                self.fadeStartTime = time.perf_counter()
        elif not self.isFadingIn():
            self.stopTimer()

        if self.fadeStartTime is not None:
            self.drawableImage.setOpacity(self.fadeInProgress())

        self.time += yup.degreesToRadians(6.0)
        self.repaint()

    def createImageFromBuffer(self, image_data) -> yup.Image:
        return yup.Image.loadFromData(image_data)

    def fadeInProgress(self) -> float:
        if self.fadeStartTime is None:
            return 0.0

        elapsed = (time.perf_counter() - self.fadeStartTime) * 1000.0

        return min(elapsed / FADE_IN_MILLIS, 1.0)

    def isFadingIn(self) -> bool:
        return self.drawableImage.getImage().isValid() and self.fadeInProgress() < 1.0

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.white)
        g.fillAll()

        if self.isFadingIn():
            b = self.getLocalBounds()
            center = yup.Point[float](b.getCenterX(), b.getCenterY())

            p = yup.Path()
            p.addStar(center, 5, 25, 60, self.time)

            g.setFillColor(yup.Colors.blueviolet)
            g.fillPath(p)

    def resized(self):
        self.drawableImage.setBounds(self.getLocalBounds())


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        MainContentComponent,
        name="Matplotlib Example",
        width=MainContentComponent.width,
        height=MainContentComponent.height,
    )
