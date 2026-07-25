#!/usr/bin/env python3
"""
YUP Hot Reload Demo - Component

The dynamically reloaded component used by hotreload_main.py.
Edit this file while hotreload_main.py is running to see live updates.

Port of popsicle's hotreload_component.py.
"""

import yup
import math
import time


class DynamicComponent(yup.Component):
    """A component that can be hot-reloaded. Edit and save to see changes."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)
        self.startTime = time.perf_counter()

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        self.repaint()

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()
        elapsed = time.perf_counter() - self.startTime

        # Draw a moving pattern
        cx = w / 2
        cy = h / 2
        radius = min(w, h) / 3

        # Rotating circles
        for i in range(12):
            angle = elapsed + i * math.pi / 6
            x = cx + math.cos(angle) * radius
            y = cy + math.sin(angle) * radius * 0.6

            r = 10 + math.sin(elapsed * 3 + i) * 5
            hue = (i / 12.0 + elapsed * 0.2) % 1.0

            g.setFillColor(yup.Color.fromHSV(hue, 0.8, 1.0, 1.0))
            g.fillEllipse(x - r, y - r, r * 2, r * 2)

        # Title
        g.setFillColor(yup.Colors.white)
        g.drawText(
            "Hot Reload Component - Edit me! 🔄",
            yup.Rectangle[float](0, 20, w, 40),
            yup.Justification.centred,
        )

        # Timestamp
        t = time.strftime("%H:%M:%S")
        g.drawText(
            f"Last loaded: {t}",
            yup.Rectangle[float](0, h - 40, w, 30),
            yup.Justification.centred,
        )
