#!/usr/bin/env python3
"""
YUP Layout Rectangles Demo

Demonstrates Rectangle positioning math for layout calculations.
Port of popsicle's layout_rectangles.py.
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

        w = self.getWidth()
        h = self.getHeight()

        # Main area (centered)
        main_area = yup.Rectangle[int](40, 40, w - 80, h - 80)
        g.setStrokeColor(yup.Colors.darkgrey)
        g.setStrokeWidth(1)
        g.strokeRect(main_area.to<float>())

        # Sidebar (left 25%)
        sidebar = main_area.withWidth(main_area.getWidth() // 4)
        g.setFillColor(yup.Colors.darkblue.withAlpha(0.3))
        g.fillRect(sidebar.to<float>())

        # Content area (right 75%)
        content = main_area.withLeft(sidebar.getRight() + 10)
        g.setFillColor(yup.Colors.darkgreen.withAlpha(0.3))
        g.fillRect(content.to<float>())

        # Header within content
        header = content.withHeight(40)
        g.setFillColor(yup.Colors.darkred.withAlpha(0.3))
        g.fillRect(header.to<float>())

        # Body below header
        body = content.withTop(header.getBottom() + 10)
        g.setFillColor(yup.Colors.grey.withAlpha(0.3))
        g.fillRect(body.to<float>())

        # Labels
        g.setFillColor(yup.Colors.white)
        g.drawText(
            "Sidebar",
            sidebar.to<float>(),
            yup.Justification.centred,
        )
        g.drawText(
            "Header",
            header.to<float>(),
            yup.Justification.centred,
        )
        g.drawText(
            "Content Body",
            body.to<float>(),
            yup.Justification.centred,
        )

        # Dimensions info
        g.setFillColor(yup.Colors.lightgrey)
        info = f"Window: {w}x{h} | Main: {main_area.getWidth()}x{main_area.getHeight()}"
        g.drawText(
            info,
            yup.Rectangle[float](10, h - 30, w - 20, 20),
            yup.Justification.left,
        )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        LayoutRectanglesComponent,
        name="Layout Rectangles",
        width=600,
        height=450,
    )
