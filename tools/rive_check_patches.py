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

"""Checks that every rive patch in the update manifest is present in thirdparty/rive.

A patch is present when its `already_contains` text and its patched `replace.to`
text are both found in the target file. Exits non-zero with the patch note on the
first missing patch, so an in-tree edit without a manifest entry (or the reverse)
is caught before `rive_update` silently drops it.
"""

from __future__ import annotations

import sys

from rive_update import DEFAULT_MANIFEST, REPO_ROOT, load_manifest, patch_text, read_text


def main() -> int:
    manifest = load_manifest(DEFAULT_MANIFEST)
    dep = next(d for d in manifest["dependencies"] if d["name"] == "rive")
    destination_root = REPO_ROOT / dep["destination"]

    for patch in dep.get("patches", []):
        path = destination_root / patch["path"]
        note = patch.get("note", patch["path"])

        if not path.exists():
            print(f"missing patch target {patch['path']}: {note}", file=sys.stderr)
            return 1

        text = read_text(path)
        if patch["already_contains"] not in text or patch_text(patch["replace"]["to"]) not in text:
            print(f"patch not applied in {patch['path']}: {note}", file=sys.stderr)
            return 1

    print("ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
