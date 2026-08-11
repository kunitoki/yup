#!/usr/bin/env python3
"""
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================

Embed a font file as a C byte-array include (`.inc`), matching the format
produced by the `_yup_file_to_byte_array()` CMake helper
(`cmake/yup_utilities.cmake`): a single line of comma separated `0x..` bytes
with no trailing newline.

Example:
    python3 scripts/embed_font.py JetBrainsMono.ttf JetBrainsMonoFont.inc
"""

import argparse
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert a font file to a YUP .inc byte array")
    parser.add_argument("input", help="Path to the input font file (.ttf/.otf/.woff2)")
    parser.add_argument("output", help="Path to the output .inc file")
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        data = f.read()

    if not data:
        print(f"error: input file '{args.input}' is empty", file=sys.stderr)
        return 1

    # Same format as _yup_file_to_byte_array: 0x00,0x01,... on a single line.
    formatted = ",".join(f"0x{b:02x}" for b in data)

    with open(args.output, "w", newline="") as f:
        f.write(formatted)

    print(f"Wrote {len(data)} bytes to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
