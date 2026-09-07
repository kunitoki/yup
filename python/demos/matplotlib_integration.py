#!/usr/bin/env python3
"""
YUP Matplotlib Integration Demo

Demonstrates rendering Matplotlib charts in a YUP window.
Port of popsicle's matplotlib_integration.py.

NOTE: Requires 'matplotlib' and 'numpy'.
    pip install matplotlib numpy
"""

import yup_init
import yup
import math
import time

try:
    import matplotlib
    matplotlib.use("Agg")  # Non-interactive backend
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    raise ImportError(
        "This demo requires matplotlib and numpy. "
        "Install with: pip install matplotlib numpy"
    )


class MatplotlibComponent(yup.Component):
    """Displays animated Matplotlib charts in a YUP window."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self.startTime = time.perf_counter()
        self.timer = yup.Timer(self.onTimer)
        self.timer.startTimerHz(30)

    def onTimer(self):
        self.repaint()

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        pass

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()

        # Generate data with NumPy
        t = np.linspace(0, 2 * math.pi, 100)
        elapsed = time.perf_counter() - self.startTime
        phase = math.sin(elapsed) * math.pi
        y1 = np.sin(t + phase)
        y2 = np.cos(t + phase * 1.5)

        # Draw chart-like visualization using YUP Graphics
        chart_left = 60
        chart_right = w - 20
        chart_top = 30
        chart_bottom = h - 40
        chart_w = chart_right - chart_left
        chart_h = chart_bottom - chart_top

        # Axes
        g.setStrokeColor(yup.Colors.grey)
        g.setStrokeWidth(1)
        g.strokeLine(chart_left, chart_top, chart_left, chart_bottom)
        g.strokeLine(chart_left, chart_bottom, chart_right, chart_bottom)

        # Grid lines
        for i in range(5):
            y = chart_top + chart_h * i / 4
            g.strokeLine(chart_left, y, chart_right, y)

        # Sine wave (blue)
        g.setStrokeColor(yup.Colors.blue)
        g.setStrokeWidth(2)
        prev_x, prev_y = chart_left, chart_bottom / 2 + chart_top / 2
        for i in range(len(t)):
            x = chart_left + chart_w * i / (len(t) - 1)
            y = chart_top + chart_h * (0.5 - 0.4 * y1[i])
            g.strokeLine(prev_x, prev_y, x, y)
            prev_x, prev_y = x, y

        # Cosine wave (orange)
        g.setStrokeColor(yup.Colors.orange)
        for i in range(len(t)):
            x = chart_left + chart_w * i / (len(t) - 1)
            y = chart_top + chart_h * (0.5 - 0.4 * y2[i])
            g.strokeLine(prev_x, prev_y, x, y)
            prev_x, prev_y = x, y

        # Legend
        g.setFillColor(yup.Colors.blue)
        g.fillRect(400, 20, 15, 12)
        g.setFillColor(yup.Colors.orange)
        g.fillRect(400, 40, 15, 12)

        font = yup.Font(yup.FontOptions(12.0))
        g.setFillColor(yup.Colors.white)
        g.fillFittedText(
            "sin(t)",
            font,
            yup.Rectangle[float](420, 18, 100, 16),
            yup.Justification.left,
        )
        g.fillFittedText(
            "cos(t)",
            font,
            yup.Rectangle[float](420, 38, 100, 16),
            yup.Justification.left,
        )

        # Title
        title_font = yup.Font(yup.FontOptions(18.0))
        g.setFillColor(yup.Colors.white)
        g.fillFittedText(
            "Matplotlib + NumPy + YUP",
            title_font,
            yup.Rectangle[float](0, 5, w, 25),
            yup.Justification.centred,
        )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        MatplotlibComponent,
        name="Matplotlib Integration",
        width=600,
        height=400,
    )
