#!/usr/bin/env python3
"""
YUP Drawables Demo

Demonstrates path drawing, color gradients, and stroke styles.
Port of popsicle's drawables.py.
"""

import yup_init
import yup
import math


class DrawablesComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        bounds = yup.Rectangle[float](0, 0, self.getWidth(), self.getHeight())

        # --- Filled rectangle
        g.setFillColor(yup.Colors.darkblue)
        g.fillRect(20, 20, 150, 80)

        # --- Stroked rectangle
        g.setStrokeColor(yup.Colors.lightgreen)
        g.setStrokeWidth(3)
        g.strokeRect(200, 20, 150, 80)

        # --- Filled ellipse
        g.setFillColor(yup.Colors.orange)
        g.fillEllipse(20, 130, 150, 80)

        # --- Color gradient
        gradient = yup.ColorGradient(
            yup.Colors.red,
            yup.Colors.blue,
            yup.Point[float](400, 20),
            yup.Point[float](550, 100),
            False,
        )
        g.setFillColor(gradient)
        g.fillRoundedRect(380, 20, 170, 80, 10)

        # --- Custom path with stroke
        path = yup.Path()
        path.startNewSubPath(20, 250)
        path.lineTo(80, 220)
        path.lineTo(140, 250)
        path.lineTo(170, 300)
        path.lineTo(110, 310)
        path.lineTo(50, 310)
        path.lineTo(20, 300)
        path.closeSubPath()

        g.setStrokeColor(yup.Colors.yellow)
        g.setStrokeWidth(2)
        g.strokePath(path)

        g.setFillColor(yup.Colors.yellow.withAlpha(0.3))
        g.fillPath(path)

        # --- Lines
        g.setStrokeColor(yup.Colors.white)
        g.setStrokeWidth(1)
        for i in range(8):
            x = 200 + i * 45
            g.strokeLine(x, 250, x + 30, 320)

        # --- Dashed lines (using custom path)
        g.setStrokeColor(yup.Colors.cyan)
        g.setStrokeWidth(2)
        g.strokeLine(200, 350, 550, 350)

        # --- Text
        g.setFillColor(yup.Colors.white)
        g.drawText("YUP Drawables Demo", bounds, yup.Justification.centredBottom)


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        DrawablesComponent,
        name="Drawables",
        width=600,
        height=450,
    )
