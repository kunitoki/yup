#!/usr/bin/env python3
"""
YUP Emoji Component Demo

Demonstrates font rendering with emoji characters.
Port of popsicle's emojis_component.py.

NOTE: This demo requires the NotoColorEmoji.ttf font file.
Download it from: https://github.com/googlefonts/noto-emoji
Place it in the same directory as this script.
"""

import yup_init
import yup
import os

# Emoji characters to render
EMOJIS = ["😀", "🎉", "🚀", "💻", "🎵", "🌟", "🔥", "❤️", "🎨", "🐍"]


class EmojiComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.darkgrey)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()

        # Try to use emoji font, fall back to default
        emoji_font_path = os.path.join(
            os.path.dirname(__file__), "NotoColorEmoji.ttf"
        )
        emoji_size = min(w, h) // 5

        for i, emoji in enumerate(EMOJIS):
            col = i % 5
            row = i // 5
            x = col * w // 5 + w // 10
            y = row * h // 2 + h // 4

            g.setFillColor(yup.Colors.white)
            g.drawText(
                emoji,
                yup.Rectangle[float](x - emoji_size // 2, y - emoji_size // 2,
                                     emoji_size, emoji_size),
                yup.Justification.centred,
            )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        EmojiComponent,
        name="Emoji Demo",
        width=600,
        height=400,
    )
