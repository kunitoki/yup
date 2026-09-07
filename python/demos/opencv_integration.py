#!/usr/bin/env python3
"""
YUP OpenCV Integration Demo

Demonstrates using OpenCV for image processing and displaying
results in a YUP window using Component painting.
Port of popsicle's opencv_integration.py.

NOTE: Requires 'opencv-python' and 'numpy'.
    pip install opencv-python numpy
"""

import yup_init
import yup
import math

try:
    import cv2
    import numpy as np
except ImportError:
    raise ImportError(
        "This demo requires opencv-python and numpy. "
        "Install with: pip install opencv-python numpy"
    )


class OpenCVComponent(yup.Component):
    """Displays OpenCV-processed data in a YUP window."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

        # Use OpenCV to generate interesting data
        self.circles = self._generateCircles()

    def _generateCircles(self):
        """Use OpenCV to detect/analyze patterns."""
        # Create a synthetic image with OpenCV
        w, h = 500, 400
        img = np.zeros((h, w), dtype=np.uint8)

        # Draw shapes
        cv2.circle(img, (100, 100), 40, 255, -1)
        cv2.circle(img, (250, 200), 60, 255, -1)
        cv2.circle(img, (400, 100), 30, 255, -1)
        cv2.circle(img, (150, 300), 45, 255, -1)
        cv2.circle(img, (350, 300), 35, 255, -1)

        # Find circles with Hough transform
        circles = cv2.HoughCircles(
            img, cv2.HOUGH_GRADIENT, 1, 20,
            param1=50, param2=30, minRadius=0, maxRadius=0,
        )

        if circles is not None:
            return [(int(c[0]), int(c[1]), int(c[2])) for c in circles[0]]
        return []

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()

        # Draw detected circles
        colors = [
            yup.Colors.red, yup.Colors.green, yup.Colors.blue,
            yup.Colors.yellow, yup.Colors.cyan, yup.Colors.magenta,
        ]

        for i, (cx, cy, r) in enumerate(self.circles):
            color = colors[i % len(colors)]
            g.setStrokeColor(color)
            g.setStrokeWidth(3)
            g.strokeEllipse(
                yup.Rectangle[float](
                    cx - r, cy - r,
                    r * 2, r * 2,
                )
            )
            g.setFillColor(color.withAlpha(0.3))
            g.fillEllipse(
                yup.Rectangle[float](
                    cx - r, cy - r,
                    r * 2, r * 2,
                )
            )

        # Draw title
        g.setFillColor(yup.Colors.white)
        font = yup.Font(yup.FontOptions(18.0))
        g.fillFittedText(
            f"OpenCV + YUP - {len(self.circles)} circles detected",
            font,
            yup.Rectangle[float](0, h - 40, w, 30),
            yup.Justification.centred,
        )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        OpenCVComponent,
        name="OpenCV Integration",
        width=550,
        height=450,
    )
