import traceback as _tb
"""
YUP Animated Component Demo

Demonstrates a self-animating component using Timer + repaint().
"""
import yup_init
import yup
import math
import time

class AnimatedComponent(yup.Component):

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
        cx = w / 2
        cy = h / 2
        elapsed = time.perf_counter() - self.startTime
        radius = min(w, h) / 4
        ball_y = cy + math.sin(elapsed * 3.0) * radius * 0.5
        ball_scale = 1.0 + math.sin(elapsed * 5.0) * 0.2
        r = radius * 0.15 * ball_scale
        gradient = yup.ColorGradient(yup.Colors.red, yup.Point[float](cx - r, ball_y - r),
                                        yup.Colors.yellow, yup.Point[float](cx + r, ball_y + r),
                                        yup.ColorGradient.Linear)
        g.setFillColorGradient(gradient)
        g.fillEllipse(cx - r, ball_y - r, r * 2, r * 2)
        for i in range(6):
            angle = elapsed * 2.0 + i * math.pi / 3
            x = cx + math.cos(angle) * radius * 0.6
            y = cy + math.sin(angle) * radius * 0.6
            size = 15 + math.sin(elapsed * 4.0 + i) * 8
            hue = (i / 6.0 + elapsed * 0.5) % 1.0
            g.setFillColor(yup.Color.fromHSV(hue, 0.8, 1.0, 1.0))
            g.fillRect(x - size / 2, y - size / 2, size, size)

if __name__ == '__main__':
    yup_init.START_YUP_COMPONENT(AnimatedComponent, name='Animated Component', width=600, height=450)
