#!/usr/bin/env python3
"""
YUP Layout Rectangles Demo

Demonstrates Rectangle positioning math for layout calculations.
"""

import yup_init
import yup


class LayoutRectanglesComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        # Component bounds are floats, but this demo works in whole pixels, so
        # measure them down to ints before doing the layout arithmetic below.
        w = int(self.getWidth())
        h = int(self.getHeight())

        # The theme font is what text is measured with - never build one inline.
        font = yup.ApplicationTheme.getGlobalTheme().getDefaultFont().withHeight(14.0)

        # Main area (centered): the window bounds inset by 40 on every side. The
        # inset is taken with removeFrom* rather than by subtracting from w and h,
        # so a window narrower than the margin leaves an empty area instead of a
        # negative one - a negative extent is what trips the jlimit assertion
        # inside removeFrom* itself.
        main_area = yup.Rectangle[int](0, 0, w, h)
        main_area.removeFromTop(40)
        main_area.removeFromBottom(40)
        main_area.removeFromLeft(40)
        main_area.removeFromRight(40)

        # A window that small has no frame left to lay out, or to draw text into.
        if main_area.isEmpty():
            return

        g.setStrokeColor(yup.Colors.darkgray)
        g.setStrokeWidth(1)
        g.strokeRect(main_area.toFloat())

        # Chunk the main area up: the sidebar comes off the left, then what is
        # left is the content area, which splits into a header and a body.
        # removeFrom* shrinks the rectangle it is called on and returns the chunk
        # it took, so work on a copy and keep main_area whole for the readout.
        remaining = yup.Rectangle[int](main_area)
        gap = 10

        # Sidebar (left 25%)
        sidebar = remaining.removeFromLeft(remaining.getWidth() // 4)
        g.setFillColor(yup.Colors.darkblue.withAlpha(0.3))
        g.fillRect(sidebar.toFloat())

        # Content area (right 75%), which the header and body are taken from
        remaining.removeFromLeft(gap)
        g.setFillColor(yup.Colors.darkgreen.withAlpha(0.3))
        g.fillRect(remaining.toFloat())

        # Header within content
        header = remaining.removeFromTop(40)
        g.setFillColor(yup.Colors.darkred.withAlpha(0.3))
        g.fillRect(header.toFloat())

        # Body below header
        remaining.removeFromTop(gap)
        body = remaining
        g.setFillColor(yup.Colors.gray.withAlpha(0.3))
        g.fillRect(body.toFloat())

        # Text only goes into a chunk that has room for it: a window only just big
        # enough for the frame leaves some of the chunks empty.
        def fittedText(text, area, justification):
            if not area.isEmpty():
                g.fillFittedText(text, font, area.toFloat(), justification)

        # Labels
        g.setFillColor(yup.Colors.white)
        fittedText("Sidebar", sidebar, yup.Justification.center)
        fittedText("Header", header, yup.Justification.center)
        fittedText("Content Body", body, yup.Justification.center)

        # Dimensions info: an inset strip along the bottom of the window. Once the
        # frame is non-empty this strip is too, so it needs no emptiness check.
        info_area = yup.Rectangle[float](0, 0, w, h)
        info_area.removeFromBottom(10)
        info_area.removeFromLeft(10)
        info_area.removeFromRight(10)

        g.setFillColor(yup.Colors.lightgray)
        info = f"Window: {w}x{h} | Main: {main_area.getWidth()}x{main_area.getHeight()}"
        g.fillFittedText(info, font, info_area.removeFromBottom(20), yup.Justification.left)


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        LayoutRectanglesComponent,
        name="Layout Rectangles",
        width=600,
        height=450,
    )
