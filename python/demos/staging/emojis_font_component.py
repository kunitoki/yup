#!/usr/bin/env python3
"""
YUP Emoji Font Component Demo

Demonstrates loading a custom font and rendering text with it.
Port of popsicle's emojis_font_component.py.

NOTE: This demo requires the NotoColorEmoji.ttf font file.
Download it from: https://github.com/googlefonts/noto-emoji
Place it in the same directory as this script.
"""

import yup_init
import yup


class EmojiFontComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()

        g.setFillColor(yup.Colors.white)
        g.drawText(
            "Hello YUP! 🚀✨",
            yup.Rectangle[float](0, 20, w, 60),
            yup.Justification.centred,
        )

        g.setFillColor(yup.Colors.lightblue)
        g.drawText(
            "Python + C++ = ❤️",
            yup.Rectangle[float](0, 100, w, 60),
            yup.Justification.centred,
        )

        g.setFillColor(yup.Colors.orange)
        g.drawText(
            "🎵 Audio 🎨 Graphics 🎮 UI",
            yup.Rectangle[float](0, 180, w, 60),
            yup.Justification.centred,
        )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        EmojiFontComponent,
        name="Emoji Font Demo",
        width=600,
        height=300,
    )
