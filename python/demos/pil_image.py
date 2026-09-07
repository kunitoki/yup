#!/usr/bin/env python3
"""
YUP PIL/Pillow Image Demo

Demonstrates generating images with PIL/Pillow and displaying them
in a YUP window using Component painting.
Port of popsicle's pil_image.py.

NOTE: Requires 'Pillow' (pip install Pillow).
"""

import yup_init
import yup
import math

try:
    from PIL import Image as PILImage, ImageDraw, ImageFilter
except ImportError:
    raise ImportError(
        "This demo requires Pillow. Install with: pip install Pillow"
    )


class PILComponent(yup.Component):
    """Displays a PIL-generated image using YUP Graphics."""

    def __init__(self):
        yup.Component.__init__(self)
        self.setOpaque(True)

        # Generate an image with PIL
        self.pattern = self._generatePattern()

    def _generatePattern(self):
        """Use PIL to create a procedural image."""
        w, h = 300, 300
        img = PILImage.new("RGBA", (w, h), (0, 0, 0, 255))
        draw = ImageDraw.Draw(img)

        # Draw concentric circles
        for i in range(5):
            r = 20 + i * 25
            color = (
                int(50 + i * 40),
                int(100 + i * 30),
                int(150 + i * 20),
                200,
            )
            draw.ellipse(
                [w // 2 - r, h // 2 - r, w // 2 + r, h // 2 + r],
                outline=color,
                width=3,
            )

        # Draw a pattern of rectangles
        for i in range(20):
            x = (i * 37) % w
            y = (i * 23) % h
            color = (
                int(255 - i * 10) % 256,
                int(100 + i * 7) % 256,
                int(50 + i * 15) % 256,
                150,
            )
            draw.rectangle([x, y, x + 30, y + 30], fill=color, outline=(255, 255, 255, 100))

        # Apply some filters
        img = img.filter(ImageFilter.SMOOTH)

        # Extract pixel data as RGB values for drawing
        pixels = []
        for y in range(h):
            row = []
            for x in range(w):
                r, g, b, a = img.getpixel((x, y))
                if a > 0:
                    row.append((x, y, r, g, b, a))
            pixels.append(row)

        return {"width": w, "height": h, "pixels": pixels}

    def paint(self, g: yup.Graphics):
        g.setFillColor(yup.Colors.black)
        g.fillAll()

        w = self.getWidth()
        h = self.getHeight()

        # Draw the PIL-generated pattern
        pw = self.pattern["width"]
        ph = self.pattern["height"]
        ox = (w - pw) / 2
        oy = (h - ph) / 2

        # Draw using a downsampled grid for performance
        step = 4
        for y in range(0, ph, step):
            for x in range(0, pw, step):
                try:
                    _, _, r, g, b, a = self.pattern["pixels"][y][x]
                    color = yup.Color.fromRGBA(r, g, b, min(a, 255))
                    g.setFillColor(color)
                    g.fillRect(
                        ox + x / pw * pw,
                        oy + y / ph * ph,
                        step, step,
                    )
                except (IndexError, ValueError):
                    pass

        # Title
        font = yup.Font(yup.FontOptions(18.0))
        g.setFillColor(yup.Colors.white)
        g.fillFittedText(
            "PIL/Pillow + YUP",
            font,
            yup.Rectangle[float](0, 10, w, 30),
            yup.Justification.centred,
        )


if __name__ == "__main__":
    yup_init.START_YUP_COMPONENT(
        PILComponent,
        name="PIL Image Demo",
        width=400,
        height=400,
    )
