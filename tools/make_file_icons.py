#!/usr/bin/env python3

# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

"""Writes the file icons of the YUP VS Code extensions.

Every file type shows the YUP icon, the tile and its "Y!" taken from logos/yup_color_icon.svg,
with a tile gradient of its own, so the types are told apart in the explorer and on the editor
tabs. The gradients are the table below:

    uv run python tools/make_file_icons.py
"""

from __future__ import annotations

import re
import struct
import xml.etree.ElementTree as ElementTree
import zlib
from math import cos, pi, sin
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOGO = ROOT / "logos/yup_color_icon.svg"

SIZE = 256
SAMPLES_PER_PIXEL = 6
CURVE_STEPS = 16

# The gradient of each tile, from its top left corner to its bottom right one.
# Still free for the next file types: ("#45dd9b", "#0b9a68") green and ("#a985ff", "#6a3cf0") violet.
ICONS = {
    "modules/yup_dsp_jit/tools/vscode-ydsp/images/ydsp-icon.png": ("#36b3ff", "#0a6fe8"),  # .ydsp
    "modules/yup_dsp_jit/tools/vscode-ydsp/images/ydsp-project-icon.png": ("#ffb22e", "#f06400"),  # .ydsp-project
    "modules/yup_gui_markup/tools/vscode-yui/images/yui-icon.png": ("#ff6cb0", "#d11f78"),  # .yui
}

Matrix = tuple[float, float, float, float, float, float]
Polygon = list[tuple[float, float]]

IDENTITY: Matrix = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
NUMBER = r"[-+]?(?:\d*\.\d+|\d+\.?)(?:[eE][-+]?\d+)?"


def multiply(outer: Matrix, inner: Matrix) -> Matrix:
    a, b, c, d, e, f = outer
    g, h, i, j, k, m = inner
    return (a * g + c * h, b * g + d * h, a * i + c * j, b * i + d * j, a * k + c * m + e, b * k + d * m + f)


def transformed(matrix: Matrix, x: float, y: float) -> tuple[float, float]:
    a, b, c, d, e, f = matrix
    return (a * x + c * y + e, b * x + d * y + f)


def parse_transform(text: str | None) -> Matrix:
    matrix = IDENTITY

    for name, arguments in re.findall(r"(\w+)\s*\(([^)]*)\)", text or ""):
        values = [float(value) for value in re.findall(NUMBER, arguments)]

        if name == "matrix":
            matrix = multiply(matrix, tuple(values))
        elif name == "translate":
            matrix = multiply(matrix, (1.0, 0.0, 0.0, 1.0, values[0], values[1] if len(values) > 1 else 0.0))
        elif name == "scale":
            matrix = multiply(matrix, (values[0], 0.0, 0.0, values[-1], 0.0, 0.0))
        else:
            raise ValueError(f"unsupported transform '{name}'")

    return matrix


def path_polygons(data: str, matrix: Matrix) -> list[Polygon]:
    """Flattens the subpaths of a path made of move, line, cubic curve and close commands."""
    tokens = re.findall(rf"[A-Za-z]|{NUMBER}", data)
    polygons: list[Polygon] = []
    points: Polygon = []
    x = y = 0.0
    command = ""
    index = 0

    def take() -> float:
        nonlocal index
        index += 1
        return float(tokens[index - 1])

    while index < len(tokens):
        if tokens[index].isalpha():
            command = tokens[index]
            index += 1

        relative = command.islower()
        kind = command.upper()

        if kind == "Z":
            polygons.append(points)
            points = []
            continue

        if kind == "M":
            if points:
                polygons.append(points)

            x, y = (x + take(), y + take()) if relative else (take(), take())
            points = [transformed(matrix, x, y)]
            command = "l" if relative else "L"
        elif kind == "L":
            x, y = (x + take(), y + take()) if relative else (take(), take())
            points.append(transformed(matrix, x, y))
        elif kind == "H":
            x = x + take() if relative else take()
            points.append(transformed(matrix, x, y))
        elif kind == "V":
            y = y + take() if relative else take()
            points.append(transformed(matrix, x, y))
        elif kind == "C":
            control = [take() for _ in range(6)]
            if relative:
                control = [value + (x if i % 2 == 0 else y) for i, value in enumerate(control)]

            x1, y1, x2, y2, x3, y3 = control
            for step in range(1, CURVE_STEPS + 1):
                t = step / CURVE_STEPS
                u = 1.0 - t
                points.append(
                    transformed(
                        matrix,
                        u * u * u * x + 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t * x3,
                        u * u * u * y + 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t * y3,
                    )
                )

            x, y = x3, y3
        else:
            raise ValueError(f"unsupported path command '{command}'")

    if points:
        polygons.append(points)

    return polygons


def rounded_rectangle(element: ElementTree.Element, matrix: Matrix) -> Polygon:
    x, y, width, height = (float(element.get(name, "0")) for name in ("x", "y", "width", "height"))
    radius_y = float(element.get("ry", element.get("rx", "0")))
    radius_x = float(element.get("rx", radius_y))
    corners = (
        (x + width - radius_x, y + radius_y, 3),
        (x + width - radius_x, y + height - radius_y, 0),
        (x + radius_x, y + height - radius_y, 1),
        (x + radius_x, y + radius_y, 2),
    )

    points = []
    for center_x, center_y, quadrant in corners:
        for step in range(CURVE_STEPS + 1):
            angle = (quadrant + step / CURVE_STEPS) * pi / 2.0
            points.append(transformed(matrix, center_x + radius_x * cos(angle), center_y + radius_y * sin(angle)))

    return points


def read_logo() -> tuple[list[Polygon], list[Polygon]]:
    """Returns the tile and the glyph of the logo as polygons in pixels."""
    root = ElementTree.parse(LOGO).getroot()
    left, top, width, height = (float(value) for value in root.get("viewBox").split())
    tile: list[Polygon] = []
    glyph: list[Polygon] = []

    def visit(element: ElementTree.Element, matrix: Matrix) -> None:
        matrix = multiply(matrix, parse_transform(element.get("transform")))
        tag = element.tag.rsplit("}", 1)[-1]

        if tag == "rect":
            tile.append(rounded_rectangle(element, matrix))
        elif tag == "path":
            glyph.extend(path_polygons(element.get("d"), matrix))

        for child in element:
            visit(child, matrix)

    visit(root, (SIZE / width, 0.0, 0.0, SIZE / height, -left * SIZE / width, -top * SIZE / height))
    return tile, glyph


def coverage(polygons: list[Polygon]) -> list[list[float]]:
    """Returns how much of each pixel the polygons cover, filled with the nonzero rule."""
    edges = [
        (x0, y0, x1, y1)
        for polygon in polygons
        for (x0, y0), (x1, y1) in zip(polygon, polygon[1:] + polygon[:1])
        if y0 != y1
    ]
    weight = 1.0 / SAMPLES_PER_PIXEL
    cover = [[0.0] * SIZE for _ in range(SIZE)]

    for row in range(SIZE * SAMPLES_PER_PIXEL):
        y = (row + 0.5) * weight
        crossings = sorted(
            (x0 + (y - y0) * (x1 - x0) / (y1 - y0), 1 if y1 > y0 else -1)
            for x0, y0, x1, y1 in edges
            if y0 <= y < y1 or y1 <= y < y0
        )
        line = cover[row // SAMPLES_PER_PIXEL]
        winding = 0

        for (start, direction), (end, _) in zip(crossings, crossings[1:]):
            winding += direction
            start, end = max(start, 0.0), min(end, float(SIZE))
            if winding == 0 or end <= start:
                continue

            first, last = int(start), int(end)
            if first == last:
                line[first] += (end - start) * weight
                continue

            line[first] += (first + 1 - start) * weight
            for x in range(first + 1, last):
                line[x] += weight
            if last < SIZE:
                line[last] += (end - last) * weight

    return cover


def write_icon(path: Path, tile: list[list[float]], glyph: list[list[float]], gradient: tuple[str, str]) -> None:
    start, end = ([int(color[i : i + 2], 16) for i in (1, 3, 5)] for color in gradient)
    rows = []

    for y in range(SIZE):
        row = bytearray([0])
        for x in range(SIZE):
            along = (x + y + 1.0) / (2.0 * SIZE)
            white = min(glyph[y][x], 1.0)
            row.extend(round((s + (e - s) * along) * (1.0 - white) + 255.0 * white) for s, e in zip(start, end))
            row.append(round(min(tile[y][x], 1.0) * 255.0))
        rows.append(bytes(row))

    def chunk(tag: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + tag + body + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(b"".join(rows), 9))
        + chunk(b"IEND", b"")
    )


def main() -> None:
    tile, glyph = (coverage(polygons) for polygons in read_logo())

    for relative_path, gradient in ICONS.items():
        write_icon(ROOT / relative_path, tile, glyph, gradient)
        print(f"{relative_path} {gradient[0]} to {gradient[1]}")


if __name__ == "__main__":
    main()
