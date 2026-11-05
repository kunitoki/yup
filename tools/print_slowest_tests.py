#!/usr/bin/env python3

# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright(c) 2026 - kunitoki@gmail.com
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

"""Prints the slowest test cases and suites of the gtest XML reports in a folder.

The test runners write one report per module when GTEST_OUTPUT=xml:<folder>/ is set.
"""

from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path, help="folder holding the gtest XML reports")
    parser.add_argument("--count", type=int, default=20, help="number of test cases and suites to print")
    args = parser.parse_args()

    cases: list[tuple[float, str]] = []
    suites: Counter[str] = Counter()

    for report in sorted(args.folder.glob("*.xml")):
        try:
            root = ET.parse(report).getroot()
        except ET.ParseError as error:
            print(f"Skipping {report.name}: {error}")
            continue

        module = report.stem
        print(f"{module}: {root.get('tests', '?')} tests in {float(root.get('time', 0)):.2f} s")

        for case in root.iter("testcase"):
            seconds = float(case.get("time", 0))
            suite = f"{module}/{case.get('classname', '')}"
            cases.append((seconds, f"{suite}.{case.get('name', '')}"))
            suites[suite] += seconds

    if not cases:
        print(f"No test reports found in {args.folder}")
        return 0

    print(f"\nSlowest {args.count} test cases:")
    for seconds, name in sorted(cases, reverse=True)[:args.count]:
        print(f"  {seconds:8.3f} s  {name}")

    print(f"\nSlowest {args.count} test suites:")
    for name, seconds in suites.most_common(args.count):
        print(f"  {seconds:8.3f} s  {name}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
