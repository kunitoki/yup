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

from __future__ import annotations

import argparse
from pathlib import Path


DEFAULT_TARGETS = [
    "yup_GpuPipeline.cpp",
    "yup_GpuRenderPass.cpp",
    "yup_GpuCanvas.cpp",
    "yup_GpuBuffer.cpp",
    "yup_GpuTexture.cpp",
    "yup_Image.cpp",
    "yup_TypeErasedObject.h",
    "yup_Graphics.cpp",
    "yup_GpuPipelineCache.cpp",
    "yup_GpuFrame.cpp",
]


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print uncovered line numbers from an LCOV .info coverage file."
    )
    parser.add_argument(
        "coverage_file",
        type=Path,
        help="Path to the LCOV .info coverage file.",
    )
    parser.add_argument(
        "targets",
        nargs="*",
        help="Source file names or path fragments to report. Uses the default graphics targets when omitted.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Report every source file found in the coverage file.",
    )
    args = parser.parse_args()

    if not args.coverage_file.is_file():
        parser.error(f"coverage file does not exist: {args.coverage_file}")

    return args


def parse_lcov_records(content: str) -> list[tuple[str, list[tuple[int, int]]]]:
    records = []

    for record in content.split("end_of_record"):
        source_file = None
        coverage_lines = []

        for line in record.splitlines():
            if line.startswith("SF:"):
                source_file = line[3:]
            elif line.startswith("DA:"):
                line_number, count, *_ = line[3:].split(",")
                coverage_lines.append((int(line_number), int(count)))

        if source_file is not None:
            records.append((source_file, coverage_lines))

    return records


def should_report(source_file: str, targets: list[str], report_all: bool) -> bool:
    return report_all or any(target in source_file for target in targets)


def display_name(source_file: str, targets: list[str], report_all: bool) -> str:
    if report_all:
        return source_file

    return next(target for target in targets if target in source_file)


def print_uncovered_lines(coverage_file: Path, targets: list[str], report_all: bool) -> None:
    content = coverage_file.read_text(encoding="utf-8")

    for source_file, coverage_lines in parse_lcov_records(content):
        if not should_report(source_file, targets, report_all):
            continue

        hit = [line_number for line_number, count in coverage_lines if count > 0]
        miss = [line_number for line_number, count in coverage_lines if count == 0]

        print(
            f"{display_name(source_file, targets, report_all)}: "
            f"total lines={len(coverage_lines)}, hit={len(hit)}, miss={len(miss)}"
        )

        if miss:
            print(f"  Missed: {miss}")


def main() -> None:
    args = parse_arguments()
    targets = args.targets if args.targets else DEFAULT_TARGETS
    print_uncovered_lines(args.coverage_file, targets, args.all)


if __name__ == "__main__":
    main()
