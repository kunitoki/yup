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

"""Checks that every vendored dependency patch in the update manifest is present in-tree.

The files touched by a dependency's patches are copied to a scratch directory and the
patches are reverse-applied there, last to first. Exits non-zero with the patch name on
the first patch that does not reverse cleanly, so an in-tree edit without a matching
patch (or the reverse) is caught before `rive_update` silently drops it.
"""

from __future__ import annotations

import re
import shutil
import sys
import tempfile
from pathlib import Path

from rive_update import DEFAULT_MANIFEST, REPO_ROOT, dependency_patch_paths, git_apply, load_manifest

PATCH_TARGET_RE = re.compile(r"^diff --git a/(\S+) b/\S+$", re.MULTILINE)


def check_dependency(dep: dict, scratch: Path) -> bool:
    destination_root = REPO_ROOT / dep["destination"]
    patch_paths = dependency_patch_paths(dep)

    for patch_path in patch_paths:
        for target in PATCH_TARGET_RE.findall(patch_path.read_text(encoding="utf-8")):
            source = destination_root / target
            if not source.exists():
                print(f"missing patch target {dep['destination']}/{target}: {patch_path.name}", file=sys.stderr)
                return False
            copy = scratch / target
            copy.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, copy)

    for patch_path in reversed(patch_paths):
        result = git_apply(patch_path, scratch, "--reverse")
        if result.returncode != 0:
            print(f"patch not applied in {dep['destination']}: {patch_path.name}\n{result.stderr}", file=sys.stderr)
            return False

    return True


def main() -> int:
    manifest = load_manifest(DEFAULT_MANIFEST)
    for dep in manifest["dependencies"]:
        if not dep.get("patches"):
            continue
        with tempfile.TemporaryDirectory() as scratch:
            if not check_dependency(dep, Path(scratch)):
                return 1

    print("ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
