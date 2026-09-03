#!/usr/bin/env python3
"""
YUP OpenCV Video Demo

Displays webcam feed processed with OpenCV in a YUP window.
Port of popsicle's opencv_video.py.

NOTE: Requires 'opencv-python' and 'numpy'.
    pip install opencv-python numpy
"""

import yup_init
import yup
import threading
import time

try:
    import cv2
    import numpy as np
except ImportError:
    raise ImportError(
        "This demo requires opencv-python and numpy. "
        "Install with: pip install opencv-python numpy"
    )


class VideoComponent(yup.Component):
    """Displays a webcam feed with OpenCV processing."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

        self.cap = None
        self.frame = None
        self.fps = 0
        self.last_time = time.perf_counter()
        self.frame_count = 0

        # Start capture thread
        self.running = True
        self.thread = threading.Thread(target=self._captureLoop, daemon=True)

        try:
            self.cap = cv2.VideoCapture(0)
            if self.cap.isOpened():
                self.thread.start()
            else:
                print("No webcam found. Using test pattern.")
                self.cap = None
        except Exception as e:
            print(f"Could not open webcam: {e}")
            self.cap = None

    def _captureLoop(self):
        """Background thread for video capture."""
        while self.running and self.cap and self.cap.isOpened():
            ret, frame = self.cap.read()
            if ret:
                # Process frame: convert to grayscale and detect edges
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                edges = cv2.Canny(gray, 50, 150)

                # Convert edges back to BGR for display
                self.frame = cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR)

                self.frame_count += 1
                now = time.perf_counter()
                elapsed = now - self.last_time
                if elapsed >= 1.0:
                    self.fps = self.frame_count / elapsed
                    self.frame_count = 0
                    self.last_time = now
            else:
                time.sleep(0.01)

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        self.repaint()

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()

        if self.frame is not None:
            # Draw edge-detected frame
            frame_h, frame_w = self.frame.shape[:2]

            # Draw edges as lines in the component
            scale_x = float(w) / frame_w
            scale_y = float(h - 30) / frame_h
            scale = min(scale_x, scale_y)

            g.setStrokeColor(yup.Colors.green)
            g.setStrokeWidth(1)

            # Simple visualization: draw detected edge points
            step = 4  # Downsample for performance
            for y in range(0, frame_h, step):
                for x in range(0, frame_w, step):
                    if self.frame[y, x, 0] > 128:  # Edge pixel
                        px = x * scale + (w - frame_w * scale) / 2
                        py = y * scale + 10
                        g.strokeLine(px, py, px + 1, py + 1)

        # Draw info
        font = yup.Font(yup.FontOptions(14.0))
        g.setFillColor(yup.Colors.white)

        if self.cap is None or not self.cap.isOpened():
            g.fillFittedText(
                "No webcam available - showing test pattern",
                font,
                yup.Rectangle[float](0, h - 30, w, 25),
                yup.Justification.centred,
            )
        else:
            g.fillFittedText(
                f"Webcam [Edge Detection] | FPS: {self.fps:.1f}",
                font,
                yup.Rectangle[float](0, h - 30, w, 25),
                yup.Justification.centred,
            )

    def __del__(self):
        self.running = False
        if self.cap:
            self.cap.release()


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        VideoComponent,
        name="OpenCV Video",
        width=640,
        height=520,
    )
