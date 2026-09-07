#!/usr/bin/env python3
"""
YUP-o-matic: A demo showing the START_YUP_COMPONENT helper pattern.

This is a simplified version demonstrating the new convenience API
introduced in yup_init.py. It draws random colored rectangles that
bounce around the window.
"""

import yup_init
import yup


class MainContentComponent(yup.Component):
    """Draws 100 random colored rectangles with random positions."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

    def refreshDisplay(self, lastFrameTimeSeconds: float):
        self.repaint()

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        g.setStrokeWidth(1)

        random = yup.Random.getSystemRandom()
        rect = yup.Rectangle[float](0, 0, 20, 20)

        for _ in range(100):
            rect.setCenter(
                random.nextFloat() * self.getWidth(),
                random.nextFloat() * self.getHeight(),
            )

            g.setStrokeColor(
                yup.Color.fromRGBA(
                    random.nextInt(255),
                    random.nextInt(255),
                    random.nextInt(255),
                    255,
                )
            )

            g.strokeRect(rect)


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        MainContentComponent,
        name="YUP-o-matic",
        width=800,
        height=600,
    )
