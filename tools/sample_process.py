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

"""Samples a running process with the macOS `sample` tool and prints the report.

The process is found by name (the newest one when several match) unless a pid
is given. The report is written to a unique file in /tmp, printed, and kept so
it can be shared.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import uuid
from pathlib import Path


def find_pid(name: str) -> int | None:
    """Return the pid of the newest process named exactly `name`, or None."""
    result = subprocess.run(["pgrep", "-n", "-x", name], capture_output=True, text=True)
    output = result.stdout.strip()
    return int(output) if result.returncode == 0 and output else None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--name", default="example_graphics", help="process name to sample (default: example_graphics)")
    parser.add_argument("--pid", type=int, help="pid to sample, overrides --name")
    parser.add_argument("--seconds", type=int, default=5, help="sampling duration in seconds (default: 5)")
    args = parser.parse_args()

    if sys.platform != "darwin":
        print("error: the `sample` tool is only available on macOS", file=sys.stderr)
        return 1

    pid = args.pid if args.pid is not None else find_pid(args.name)
    if pid is None:
        print(f"error: no running process named '{args.name}'", file=sys.stderr)
        return 1

    report = Path("/tmp") / f"{args.name}-{pid}-{uuid.uuid4().hex[:8]}.txt"

    print(f"Sampling pid {pid} for {args.seconds}s...", file=sys.stderr)
    result = subprocess.run(["sample", str(pid), str(args.seconds), "-file", str(report)], stdout=subprocess.DEVNULL)
    if result.returncode != 0 or not report.exists():
        print(f"error: sample failed with exit code {result.returncode}", file=sys.stderr)
        return result.returncode or 1

    print(report.read_text(errors="replace"))
    print(f"Report saved to {report}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
