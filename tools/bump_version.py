#!/usr/bin/env python3
#
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

"""
Bump YUP version across the entire codebase.

Updates:
  1. modules/yup_core/system/yup_StandardHeader.h   (YUP_MAJOR/MINOR/BUILDNUMBER defines)
  2. modules/yup_*/yup_*.h                           (version: field in BEGIN_YUP_MODULE_DECLARATION)
  3. tests/CMakeLists.txt                             (target_version variable)
  4. examples/*/CMakeLists.txt                        (target_version variable)

Usage:
  bump_version.py 1.1.0                     # Set exact version
  bump_version.py --major 2                 # Set major to 2, keep minor/build
  bump_version.py --minor 5                 # Set minor to 5, keep major/build
  bump_version.py --build 42                # Set build to 42, keep major/minor
  bump_version.py --major 2 --minor 0       # Combine flags
  bump_version.py --dry-run 1.1.0           # Preview changes without writing
"""

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent

STANDARD_HEADER = ROOT / "modules/yup_core/system/yup_StandardHeader.h"
MODULE_HEADERS = sorted(ROOT.glob("modules/yup_*/yup_*.h"))
CMAKE_FILES = sorted(
    list(ROOT.glob("tests/CMakeLists.txt"))
    + list(ROOT.glob("examples/*/CMakeLists.txt"))
)


def parse_current_version():
    """Extract (major, minor, build) from yup_StandardHeader.h."""
    text = STANDARD_HEADER.read_text()
    major = re.search(r"#define\s+YUP_MAJOR_VERSION\s+(\d+)", text)
    minor = re.search(r"#define\s+YUP_MINOR_VERSION\s+(\d+)", text)
    build = re.search(r"#define\s+YUP_BUILDNUMBER\s+(\d+)", text)
    if not (major and minor and build):
        sys.exit(f"ERROR: Could not parse version defines in {STANDARD_HEADER}")
    return int(major.group(1)), int(minor.group(1)), int(build.group(1))


def build_version_string(major, minor, build):
    return f"{major}.{minor}.{build}"


def update_standard_header(new_major, new_minor, new_build, dry_run):
    """Update YUP_MAJOR_VERSION, YUP_MINOR_VERSION, YUP_BUILDNUMBER in yup_StandardHeader.h."""
    text = STANDARD_HEADER.read_text()

    replacements = [
        (r"#define YUP_MAJOR_VERSION \d+", f"#define YUP_MAJOR_VERSION {new_major}"),
        (r"#define YUP_MINOR_VERSION \d+", f"#define YUP_MINOR_VERSION {new_minor}"),
        (r"#define YUP_BUILDNUMBER \d+", f"#define YUP_BUILDNUMBER {new_build}"),
    ]

    new_text = text
    for pattern, replacement in replacements:
        new_text = re.sub(pattern, replacement, new_text)

    if new_text != text:
        print(f"  {STANDARD_HEADER.relative_to(ROOT)}")
        if not dry_run:
            STANDARD_HEADER.write_text(new_text)
        return True
    return False


def update_module_headers(new_version_str, dry_run):
    """Update version: field in all module headers under modules/yup_*/yup_*.h."""
    updated = False
    for header in MODULE_HEADERS:
        text = header.read_text()
        new_text = re.sub(
            r"(version:\s+)\S+",
            rf"\g<1>{new_version_str}",
            text,
        )
        if new_text != text:
            print(f"  {header.relative_to(ROOT)}")
            if not dry_run:
                header.write_text(new_text)
            updated = True
    return updated


def update_cmake_files(new_version_str, dry_run):
    """Update set (target_version "...") in tests/ and examples/*/CMakeLists.txt."""
    updated = False
    for cmake_file in CMAKE_FILES:
        text = cmake_file.read_text()
        new_text = re.sub(
            r'(set\s*\(\s*target_version\s+)"[^"]*"',
            rf'\1"{new_version_str}"',
            text,
        )
        if new_text != text:
            print(f"  {cmake_file.relative_to(ROOT)}")
            if not dry_run:
                cmake_file.write_text(new_text)
            updated = True
    return updated


def main():
    parser = argparse.ArgumentParser(description="Bump YUP version across the codebase.")
    parser.add_argument("version", nargs="?", help="New version string, e.g. 1.1.0")
    parser.add_argument("--major", type=int, help="Set major version")
    parser.add_argument("--minor", type=int, help="Set minor version")
    parser.add_argument("--build", type=int, help="Set build number")
    parser.add_argument("--dry-run", action="store_true", help="Preview changes without writing files")

    args = parser.parse_args()

    current = parse_current_version()

    if args.version:
        parts = args.version.split(".")
        if len(parts) != 3 or not all(p.isdigit() for p in parts):
            sys.exit("ERROR: version must be in MAJOR.MINOR.BUILD format (e.g. 1.1.0)")
        new_major, new_minor, new_build = int(parts[0]), int(parts[1]), int(parts[2])
    else:
        new_major = args.major if args.major is not None else current[0]
        new_minor = args.minor if args.minor is not None else current[1]
        new_build = args.build if args.build is not None else current[2]

    if (new_major, new_minor, new_build) == current:
        print(f"Version already at {build_version_string(*current)}. Nothing to do.")
        return

    new_version_str = build_version_string(new_major, new_minor, new_build)
    old_version_str = build_version_string(*current)

    if args.dry_run:
        print(f"[DRY RUN] Would bump from {old_version_str} to {new_version_str}")
    else:
        print(f"Bumping from {old_version_str} to {new_version_str}")

    print("Files to update:")

    update_standard_header(new_major, new_minor, new_build, args.dry_run)
    update_module_headers(new_version_str, args.dry_run)
    update_cmake_files(new_version_str, args.dry_run)

    if args.dry_run:
        print(f"\n[DRY RUN] No files were modified. Remove --dry-run to apply.")
    else:
        print(f"\nDone. Version bumped to {new_version_str}.")


if __name__ == "__main__":
    main()
