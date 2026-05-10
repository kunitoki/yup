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

from __future__ import annotations

import argparse
import fnmatch
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
DEFAULT_MANIFEST = SCRIPT_DIR / "rive_update_manifest.json"
SHA_RE = re.compile(r"^[0-9a-fA-F]{7,40}$")
PREMAKE_GITHUB_RE = re.compile(r"dependency\.github\s*\(\s*['\"]([^'\"]+)['\"]\s*,\s*['\"]([^'\"]+)['\"]\s*\)")
YUP_C_FILE_HEADER = """/*
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
*/"""


@dataclass
class CopyStats:
    copied: int = 0
    unchanged: int = 0
    removed: int = 0


@dataclass
class DependencyResult:
    name: str
    ref: str
    version: str
    checkout: str
    copy_stats: list[CopyStats] = field(default_factory=list)
    generated_files: int = 0
    notes: list[str] = field(default_factory=list)

    @property
    def copied(self) -> int:
        return sum(item.copied for item in self.copy_stats)

    @property
    def unchanged(self) -> int:
        return sum(item.unchanged for item in self.copy_stats)

    @property
    def removed(self) -> int:
        return sum(item.removed for item in self.copy_stats)


def run(command: list[str], cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    print("+ " + " ".join(command))
    try:
        return subprocess.run(command, cwd=cwd, check=check, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError as error:
        if error.stdout:
            print(error.stdout, end="")
        if error.stderr:
            print(error.stderr, end="", file=sys.stderr)
        raise


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def posix(path: Path) -> str:
    return path.as_posix()


def is_relative_to(path: Path, parent: Path) -> bool:
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False


def matches_pattern(value: str, pattern: str) -> bool:
    if "/" not in pattern:
        if "/" in value:
            return False
        return fnmatch.fnmatchcase(value, pattern)

    if fnmatch.fnmatchcase(value, pattern):
        return True
    if pattern.startswith("**/"):
        return fnmatch.fnmatchcase(value, pattern[3:])
    return False


def matches_any(value: str, patterns: list[str]) -> bool:
    return any(matches_pattern(value, pattern) for pattern in patterns)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text_if_changed(path: Path, text: str) -> bool:
    if path.exists() and read_text(path) == text:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return True


def generated_file_text(entry: dict[str, Any]) -> str:
    parts: list[str] = []

    file_header = entry.get("file_header")
    if file_header == "yup_c":
        parts.append(YUP_C_FILE_HEADER)
    elif file_header:
        raise SystemExit(f"Unknown generated file header '{file_header}' in {entry['path']}")

    if "content" in entry:
        parts.append(str(entry["content"]).rstrip())
    elif "lines" in entry:
        parts.append("\n".join(entry["lines"]).rstrip())
    else:
        raise SystemExit(f"Generated file entry is missing content or lines: {entry['path']}")

    return "\n\n".join(part for part in parts if part) + "\n"


def process_generated_files(manifest: dict[str, Any]) -> int:
    changed = 0
    for entry in manifest.get("generated_files", []):
        path = (REPO_ROOT / entry["path"]).resolve()
        if not is_relative_to(path, REPO_ROOT):
            raise SystemExit(f"Generated file path escapes repository root: {path}")
        if write_text_if_changed(path, generated_file_text(entry)):
            changed += 1
    return changed


def parse_dep_override(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError("--dep must be formatted as name=ref")
    name, ref = value.split("=", 1)
    name = name.strip()
    ref = ref.strip()
    if not name or not ref:
        raise argparse.ArgumentTypeError("--dep must include both name and ref")
    return name, ref


def resolve_refs(manifest: dict[str, Any], rive_ref: str | None, dep_overrides: list[tuple[str, str]]) -> dict[str, str]:
    refs = {dep["name"]: dep.get("ref", "") for dep in manifest["dependencies"]}
    if rive_ref:
        for dep in manifest["dependencies"]:
            if dep.get("follow_rive_ref"):
                refs[dep["name"]] = rive_ref

    known = set(refs)
    for name, ref in dep_overrides:
        if name not in known:
            raise SystemExit(f"Unknown dependency override '{name}'. Known dependencies: {', '.join(sorted(known))}")
        refs[name] = ref
    return refs


def ref_to_version(ref: str) -> str:
    value = ref.strip()
    for prefix in ("refs/tags/", "refs/heads/", "origin/"):
        if value.startswith(prefix):
            value = value.removeprefix(prefix)

    if value in ("main", "master", "develop", "trunk") or SHA_RE.fullmatch(value):
        return "0.0.0"

    libpng_match = re.fullmatch(r"libpng(\d)(\d+)", value)
    if libpng_match:
        return f"{libpng_match.group(1)}.{int(libpng_match.group(2))}"

    if value == "rive_changes_v2_0_1_2":
        return "2.0.2.1"

    value = re.sub(r"^(rive_changes_|rive_|release[-_/])", "", value)
    if value.startswith("v") and len(value) > 1 and value[1].isdigit():
        value = value[1:]

    numeric_match = re.search(r"\d+(?:[._-]\d+)+", value)
    if numeric_match:
        return numeric_match.group(0).replace("_", ".").replace("-", ".")

    numeric_match = re.search(r"\d+", value)
    if numeric_match:
        return numeric_match.group(0)

    return "0.0.0"


def git_status(paths: list[Path]) -> list[str]:
    rel_paths = [posix(path.relative_to(REPO_ROOT)) if path.is_absolute() else posix(path) for path in paths]
    command = ["git", "status", "--porcelain", "--", *rel_paths]
    completed = run(command, cwd=REPO_ROOT)
    return [line for line in completed.stdout.splitlines() if line.strip()]


def fail_on_dirty(manifest: dict[str, Any]) -> None:
    paths = [REPO_ROOT / dep["destination"] for dep in manifest["dependencies"]]
    metadata_path = manifest.get("metadata_path")
    if metadata_path:
        paths.append(REPO_ROOT / metadata_path)
    dirty = git_status(paths)
    if dirty:
        print("Managed vendor paths already have changes:")
        for line in dirty:
            print(f"  {line}")
        raise SystemExit("Pass --allow-dirty to update on top of these changes.")


def clone_dependency(dep: dict[str, Any], ref: str, work_dir: Path) -> Path:
    checkout_dir = work_dir / "repos" / dep["name"]
    if checkout_dir.exists():
        shutil.rmtree(checkout_dir)
    checkout_dir.parent.mkdir(parents=True, exist_ok=True)

    if SHA_RE.fullmatch(ref):
        checkout_dir.mkdir(parents=True)
        run(["git", "init"], cwd=checkout_dir)
        run(["git", "remote", "add", "origin", dep["repo"]], cwd=checkout_dir)
        run(["git", "fetch", "--depth", "1", "origin", ref], cwd=checkout_dir)
        run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=checkout_dir)
    else:
        clone = run(["git", "clone", "--depth", "1", "--branch", ref, dep["repo"], str(checkout_dir)], check=False)
        if clone.returncode != 0:
            if checkout_dir.exists():
                shutil.rmtree(checkout_dir)
            run(["git", "clone", "--depth", "1", dep["repo"], str(checkout_dir)])
            run(["git", "fetch", "--depth", "1", "origin", ref], cwd=checkout_dir)
            run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=checkout_dir)

    return checkout_dir


def checkout_hash(checkout_dir: Path) -> str:
    completed = run(["git", "rev-parse", "HEAD"], cwd=checkout_dir)
    return completed.stdout.strip()


def parse_rive_dependency_premake(rive_checkout: Path, dep: dict[str, Any]) -> tuple[str, str]:
    config = dep["rive_dependency_premake"]
    premake_path = rive_checkout / config["file"]
    expected_project = config["project"]

    if not premake_path.exists():
        raise SystemExit(f"Missing Rive dependency declaration for {dep['name']}: {premake_path}")

    for project, ref in PREMAKE_GITHUB_RE.findall(read_text(premake_path)):
        if project == expected_project:
            return f"https://github.com/{project}.git", ref

    raise SystemExit(f"Could not find dependency.github('{expected_project}', ...) in {premake_path}")


def iter_source_files(source_dir: Path) -> list[Path]:
    return sorted(path for path in source_dir.rglob("*") if path.is_file())


def should_copy(rel: str, include_globs: list[str], exclude_globs: list[str]) -> bool:
    if include_globs and not matches_any(rel, include_globs):
        return False
    return not matches_any(rel, exclude_globs)


def remove_empty_dirs(path: Path) -> None:
    if not path.exists():
        return
    for directory in sorted((item for item in path.rglob("*") if item.is_dir()), key=lambda item: len(item.parts), reverse=True):
        try:
            directory.rmdir()
        except OSError:
            pass


def remove_work_dir(work_dir: Path) -> None:
    resolved = work_dir.resolve()
    forbidden = {
        REPO_ROOT.resolve(),
        Path.home().resolve(),
        Path("/").resolve(),
        Path("/tmp").resolve(),
        Path("/private/tmp").resolve(),
    }
    if resolved in forbidden or len(resolved.parts) < 3:
        raise SystemExit(f"Refusing to remove unsafe work directory: {resolved}")
    shutil.rmtree(resolved)


def copy_tree(checkout_dir: Path, destination_root: Path, copy_rule: dict[str, Any], global_excludes: list[str]) -> CopyStats:
    source_dir = (checkout_dir / copy_rule["from"]).resolve()
    destination_dir = (destination_root / copy_rule["to"]).resolve()

    if not source_dir.exists():
        raise SystemExit(f"Missing upstream source directory: {source_dir}")
    if not is_relative_to(destination_dir, destination_root):
        raise SystemExit(f"Copy destination escapes dependency root: {destination_dir}")

    include_globs = copy_rule.get("include_globs", [])
    exclude_globs = global_excludes + copy_rule.get("exclude_globs", [])
    preserve_globs = copy_rule.get("preserve_globs", [])
    copied_paths: set[Path] = set()
    stats = CopyStats()

    for source in iter_source_files(source_dir):
        rel = posix(source.relative_to(source_dir))
        if not should_copy(rel, include_globs, exclude_globs):
            continue

        destination = destination_dir / rel
        if not is_relative_to(destination, destination_dir):
            raise SystemExit(f"Copy target escapes expected tree: {destination}")

        copied_paths.add(destination.resolve())
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.exists() and destination.read_bytes() == source.read_bytes():
            stats.unchanged += 1
            continue

        shutil.copy2(source, destination)
        stats.copied += 1

    if destination_dir.exists():
        for existing in sorted(path for path in destination_dir.rglob("*") if path.is_file()):
            rel = posix(existing.relative_to(destination_dir))
            if existing.resolve() in copied_paths or matches_any(rel, preserve_globs):
                continue
            existing.unlink()
            stats.removed += 1

    remove_empty_dirs(destination_dir)
    return stats


def replace_module_version(header_path: Path, version: str) -> bool:
    if not header_path.exists():
        return False
    text = read_text(header_path)
    new_text = re.sub(r"(^\s*version:\s+).*$", rf"\g<1>{version}", text, count=1, flags=re.MULTILINE)
    if new_text == text:
        return False
    return write_text_if_changed(header_path, new_text)


def patch_text(value: str | list[str]) -> str:
    if isinstance(value, list):
        return "\n".join(value)
    return value


def apply_dependency_patches(destination_root: Path, dep: dict[str, Any]) -> list[str]:
    notes: list[str] = []
    for patch in dep.get("patches", []):
        path = (destination_root / patch["path"]).resolve()
        if not is_relative_to(path, destination_root):
            raise SystemExit(f"Patch path escapes dependency root: {path}")
        if not path.exists():
            raise SystemExit(f"Missing patch target: {path}")

        text = read_text(path)
        already_contains = patch.get("already_contains")
        if already_contains and already_contains in text:
            continue

        replacement = patch["replace"]
        old = patch_text(replacement["from"])
        new = patch_text(replacement["to"])
        if old not in text:
            raise SystemExit(f"Could not apply patch '{patch.get('note', patch['path'])}' to {path}")

        if write_text_if_changed(path, text.replace(old, new, 1)):
            notes.append(patch.get("note", f"patched {patch['path']}"))

    return notes


def generate_include_lines(block: dict[str, Any]) -> list[str]:
    if "entries" in block:
        return list(block["entries"])

    root = REPO_ROOT / block["root"]
    include_globs = block.get("include_globs", [])
    exclude_globs = block.get("exclude_globs", [])
    include_overrides = block.get("include_overrides", {})
    prefix = block.get("prefix", "")
    paths: list[str] = []

    if not root.exists():
        raise SystemExit(f"Include generation root does not exist: {root}")

    for source in iter_source_files(root):
        rel = posix(source.relative_to(root))
        if should_copy(rel, include_globs, exclude_globs):
            include_path = f"{prefix}/{rel}" if prefix else rel
            paths.append(include_path)

    lines: list[str] = []
    for path in sorted(paths):
        override = include_overrides.get(path)
        if override:
            lines.extend(override)
        else:
            lines.append(f'#include "{path}"')
    return lines


def find_generated_range(text: str, start_marker: str, end_marker: str) -> tuple[int, int] | None:
    start = text.find(start_marker)
    if start < 0:
        return None
    start_line_end = text.find("\n", start)
    if start_line_end < 0:
        raise SystemExit(f"Malformed include block marker: {start_marker}")
    end = text.find(end_marker, start_line_end)
    if end < 0:
        raise SystemExit(f"Missing include block end marker: {end_marker}")
    return start, text.find("\n", end) + 1


def find_insert_range(text: str, block: dict[str, Any]) -> tuple[int, int]:
    insert_after = block["insert_after"]
    after = text.find(insert_after)
    if after < 0:
        raise SystemExit(f"Cannot find insert_after in {block['path']}: {insert_after}")
    start = after + len(insert_after)
    if start < len(text) and text[start] == "\r":
        start += 1
    if start < len(text) and text[start] == "\n":
        start += 1

    insert_before = block.get("insert_before")
    if not insert_before:
        return start, len(text.rstrip())

    end = text.find(insert_before, start)
    if end < 0:
        raise SystemExit(f"Cannot find insert_before in {block['path']}: {insert_before}")
    while end > 0 and text[end - 1] == "\n":
        end -= 1
    return start, end


def order_include_lines(generated_lines: list[str], current_text: str) -> list[str]:
    if not current_text.strip():
        return generated_lines

    remaining: dict[str, int] = {}
    for line in generated_lines:
        remaining[line] = remaining.get(line, 0) + 1

    ordered: list[str] = []

    for line in current_text.splitlines():
        stripped = line.strip()
        if remaining.get(stripped, 0) > 0:
            ordered.append(stripped)
            remaining[stripped] -= 1

    for line in generated_lines:
        if remaining.get(line, 0) > 0:
            ordered.append(line)
            remaining[line] -= 1

    return ordered


def regenerate_include_block(block: dict[str, Any]) -> bool:
    path = REPO_ROOT / block["path"]
    text = read_text(path)
    start_marker = block["start_marker"]
    end_marker = block["end_marker"]

    generated_range = find_generated_range(text, start_marker, end_marker)
    if generated_range:
        start, end = generated_range
        current_include_text = text[start:end]
        leading_newline = ""
    else:
        start, end = find_insert_range(text, block)
        current_include_text = text[start:end]
        leading_newline = "\n"

    include_lines = order_include_lines(generate_include_lines(block), current_include_text)

    replacement = leading_newline + "\n".join([start_marker, *include_lines, end_marker, ""])

    new_text = text[:start] + replacement + text[end:]
    return write_text_if_changed(path, new_text)


def included_targets(path: Path, search_roots: list[Path]) -> list[Path]:
    include_re = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
    targets: list[Path] = []
    for line in read_text(path).splitlines():
        match = include_re.match(line)
        if not match:
            continue
        include_path = match.group(1)
        candidates = [root / include_path for root in search_roots]
        targets.append(next((candidate for candidate in candidates if candidate.exists()), candidates[0]))
    return targets


def validate_include_targets(path: Path, block: dict[str, Any] | None = None) -> None:
    search_roots = [path.parent]
    if block:
        search_roots.extend(REPO_ROOT / root for root in block.get("search_roots", []))

    missing = [target for target in included_targets(path, search_roots) if not target.exists()]
    if missing:
        print(f"Missing include targets in {path.relative_to(REPO_ROOT)}:")
        for target in missing:
            print(f"  {target.relative_to(REPO_ROOT)}")
        raise SystemExit("Generated include target validation failed.")


def process_include_blocks(manifest: dict[str, Any]) -> int:
    changed = 0
    for block in manifest.get("include_blocks", []):
        path = REPO_ROOT / block["path"]
        if block.get("validate_existing_includes"):
            validate_include_targets(path, block)
            continue

        if regenerate_include_block(block):
            changed += 1
        validate_include_targets(path, block)
    return changed


def selected_shader_targets(shader_config: dict[str, Any], skipped_targets: set[str]) -> list[str]:
    targets = shader_config.get("targets", [])
    unknown = skipped_targets.difference(targets)
    if unknown:
        raise SystemExit(f"Unknown shader target(s) for --skip-shader-target: {', '.join(sorted(unknown))}")
    return [target for target in targets if target not in skipped_targets]


def preflight_shader_generation(dep: dict[str, Any], checkout_dir: Path, skipped_targets: set[str]) -> None:
    shader_config = dep.get("shader_generation")
    if not shader_config:
        return

    shader_dir = checkout_dir / shader_config["source_dir"]
    if not shader_dir.exists():
        raise SystemExit(f"Missing shader source directory: {shader_dir}")

    missing: dict[str, list[str]] = {}
    required_tools = shader_config.get("required_tools", {})
    for target in selected_shader_targets(shader_config, skipped_targets):
        missing_tools = [tool for tool in required_tools.get(target, []) if shutil.which(tool) is None]
        if missing_tools:
            missing[target] = missing_tools

    if not missing:
        return

    lines = ["Missing tools required for Rive shader generation:"]
    for target, tools in missing.items():
        lines.append(f"  {target}: {', '.join(tools)}")
    lines.append("")
    lines.append("Install the missing tools, or intentionally skip affected targets, for example:")
    for target in missing:
        lines.append(f"  --skip-shader-target {target}")
    raise SystemExit("\n".join(lines))


def run_shader_generation(dep: dict[str, Any], checkout_dir: Path, skipped_targets: set[str]) -> int:
    shader_config = dep.get("shader_generation")
    if not shader_config:
        return 0

    shader_dir = checkout_dir / shader_config["source_dir"]
    if not shader_dir.exists():
        raise SystemExit(f"Missing shader source directory: {shader_dir}")

    preflight_shader_generation(dep, checkout_dir, skipped_targets)
    venv_dir = shader_dir / "cooker"
    run([sys.executable, "-m", "venv", str(venv_dir)])
    python_bin = venv_dir / ("Scripts/python.exe" if os.name == "nt" else "bin/python3")
    package = shader_config.get("python_package")
    if package:
        run([str(python_bin), "-m", "pip", "install", package])

    site_packages = next((venv_dir / "lib").glob("python*/site-packages"), None)
    flags = [f"--ply-path={site_packages}"] if site_packages else []

    targets = selected_shader_targets(shader_config, skipped_targets)
    for target in targets:
        run(["make", f"FLAGS={' '.join(flags)}", target], cwd=shader_dir)

    generated_dir = shader_dir / shader_config["output_dir"]
    destination = REPO_ROOT / shader_config["destination"]
    if not generated_dir.exists():
        raise SystemExit(f"Shader generation did not produce: {generated_dir}")

    count = 0
    destination.mkdir(parents=True, exist_ok=True)
    copied_paths: set[Path] = set()
    for source in iter_source_files(generated_dir):
        rel = source.relative_to(generated_dir)
        target = destination / rel
        copied_paths.add(target.resolve())
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != source.read_bytes():
            shutil.copy2(source, target)
        count += 1

    if not skipped_targets:
        for existing in sorted(path for path in destination.rglob("*") if path.is_file()):
            if existing.resolve() not in copied_paths:
                existing.unlink()
        remove_empty_dirs(destination)
    return count


def write_metadata(manifest: dict[str, Any], results: list[DependencyResult]) -> None:
    metadata_path = REPO_ROOT / manifest["metadata_path"]
    payload = {
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "dependencies": [
            {
                "name": result.name,
                "ref": result.ref,
                "version": result.version,
                "checkout": result.checkout,
            }
            for result in results
        ],
    }
    write_text_if_changed(metadata_path, json.dumps(payload, indent=2, sort_keys=True) + "\n")


def check_available_refs(manifest: dict[str, Any], refs: dict[str, str], work_dir: Path) -> None:
    metadata_path = REPO_ROOT / manifest.get("metadata_path", "")
    current: dict[str, Any] = {}
    if metadata_path.exists():
        current = json.loads(read_text(metadata_path))

    rive_dep = next((dep for dep in manifest["dependencies"] if dep["name"] == "rive"), None)
    if not rive_dep:
        raise SystemExit("Manifest must contain the Rive runtime dependency named 'rive'.")

    work_dir.mkdir(parents=True, exist_ok=True)
    rive_checkout = clone_dependency(rive_dep, refs["rive"], work_dir)
    current_by_name = {item["name"]: item for item in current.get("dependencies", [])}
    for dep in manifest["dependencies"]:
        name = dep["name"]
        requested = refs[name]
        print(f"{name}:")
        print(f"  current:   {current_by_name.get(name, {}).get('ref', 'unknown')}")
        if dep.get("rive_dependency_premake") and not requested:
            repo, requested = parse_rive_dependency_premake(rive_checkout, dep)
            print(f"  requested: {requested} from Rive runtime {dep['rive_dependency_premake']['file']}")
        else:
            repo = dep["repo"]
            print(f"  requested: {requested}")
        remote = run(["git", "ls-remote", repo, requested], check=False)
        status = "found" if remote.returncode == 0 and remote.stdout.strip() else "not found as exact remote ref"
        print(f"  remote:    {status}")
        for copy_rule in dep.get("copies", []):
            print(f"  copy:      {copy_rule['from']} -> {dep['destination']}/{copy_rule['to']}")


def update_dependencies(
    manifest: dict[str, Any],
    refs: dict[str, str],
    work_dir: Path,
    skipped_shader_targets: set[str],
) -> list[DependencyResult]:
    work_dir.mkdir(parents=True, exist_ok=True)
    global_excludes = manifest.get("global_exclude_globs", [])
    results: list[DependencyResult] = []
    rive_dep = next((dep for dep in manifest["dependencies"] if dep["name"] == "rive"), None)
    if not rive_dep:
        raise SystemExit("Manifest must contain the Rive runtime dependency named 'rive'.")

    rive_ref = refs["rive"]
    rive_checkout = clone_dependency(rive_dep, rive_ref, work_dir)
    checkout_cache: dict[tuple[str, str], Path] = {(rive_dep["repo"], rive_ref): rive_checkout}

    for dep in manifest["dependencies"]:
        if dep.get("shader_generation"):
            preflight_shader_generation(dep, rive_checkout, skipped_shader_targets)

    for dep in manifest["dependencies"]:
        name = dep["name"]
        ref = refs[name]
        destination_root = REPO_ROOT / dep["destination"]
        destination_root.mkdir(parents=True, exist_ok=True)

        source_dep = dep
        if name == "rive":
            checkout_dir = rive_checkout
        elif dep.get("follow_rive_ref") and dep["repo"] == rive_dep["repo"] and ref == rive_ref:
            checkout_dir = rive_checkout
        elif dep.get("rive_dependency_premake") and not ref:
            repo, ref = parse_rive_dependency_premake(rive_checkout, dep)
            source_dep = dict(dep)
            source_dep["repo"] = repo
            cache_key = (repo, ref)
            if cache_key not in checkout_cache:
                checkout_cache[cache_key] = clone_dependency(source_dep, ref, work_dir)
            checkout_dir = checkout_cache[cache_key]
        else:
            cache_key = (dep["repo"], ref)
            if cache_key not in checkout_cache:
                checkout_cache[cache_key] = clone_dependency(dep, ref, work_dir)
            checkout_dir = checkout_cache[cache_key]

        version = ref_to_version(ref)
        result = DependencyResult(name=name, ref=ref, version=version, checkout=checkout_hash(checkout_dir))

        for copy_rule in dep.get("copies", []):
            result.copy_stats.append(copy_tree(checkout_dir, destination_root, copy_rule, global_excludes))

        result.notes.extend(apply_dependency_patches(destination_root, dep))

        if dep.get("module_header") and dep.get("module_version_from_ref"):
            changed = replace_module_version(REPO_ROOT / dep["module_header"], version)
            if changed:
                result.notes.append(f"updated module declaration version to {version}")

        result.generated_files = run_shader_generation(dep, checkout_dir, skipped_shader_targets)
        if result.generated_files:
            result.notes.append(f"copied {result.generated_files} generated shader files")
        if dep.get("shader_generation") and skipped_shader_targets:
            result.notes.append(f"skipped shader targets: {', '.join(sorted(skipped_shader_targets))}")

        results.append(result)

    generated_file_changes = process_generated_files(manifest)
    if generated_file_changes:
        results.append(
            DependencyResult(
                name="generated-files",
                ref="generated",
                version="",
                checkout="",
                notes=[f"regenerated {generated_file_changes} manifest generated files"],
            )
        )

    include_changes = process_include_blocks(manifest)
    if include_changes:
        results.append(
            DependencyResult(
                name="include-lists",
                ref="generated",
                version="",
                checkout="",
                notes=[f"regenerated {include_changes} amalgamated include lists"],
            )
        )

    write_metadata(manifest, [result for result in results if result.checkout])
    return results


def print_summary(results: list[DependencyResult]) -> None:
    print("")
    print("Rive vendor update summary")
    print("==========================")
    for result in results:
        print(f"{result.name}:")
        if result.ref:
            print(f"  ref:       {result.ref}")
        if result.version:
            print(f"  version:   {result.version}")
        if result.checkout:
            print(f"  checkout:  {result.checkout}")
        if result.copy_stats:
            print(f"  copied:    {result.copied}")
            print(f"  unchanged: {result.unchanged}")
            print(f"  removed:   {result.removed}")
        if result.generated_files:
            print(f"  generated: {result.generated_files}")
        for note in result.notes:
            print(f"  note:      {note}")
    print("")
    print("Review notes:")
    print("  - Inspect the unstaged diff before committing.")
    print("  - Confirm generated shader outputs are expected for the host platform.")
    print("  - Run project configure/build/tests separately after review.")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Refresh vendored Rive and Rive-adjacent dependencies.")
    parser.add_argument("--rive-ref", help="Rive runtime ref used for rive, rive_renderer, and rive_decoders.")
    parser.add_argument("--dep", action="append", default=[], type=parse_dep_override, help="Override dependency ref as name=ref.")
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST), help="Path to the Rive update manifest.")
    parser.add_argument("--work-dir", default="build/rive-update", help="Directory for checkouts and generated intermediates.")
    parser.add_argument("--keep-work-dir", action="store_true", help="Leave the work directory in place after the run.")
    parser.add_argument("--allow-dirty", action="store_true", help="Allow pre-existing changes under managed vendor paths.")
    parser.add_argument("--check", action="store_true", help="Report planned operations and remote ref availability without modifying files.")
    parser.add_argument(
        "--skip-shader-target",
        action="append",
        default=[],
        help="Skip one Rive shader Makefile target, e.g. spirv when glslangValidator is unavailable.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = load_manifest(Path(args.manifest))
    refs = resolve_refs(manifest, args.rive_ref, args.dep)
    work_dir = (REPO_ROOT / args.work_dir).resolve()

    if args.check:
        if work_dir.exists() and not args.keep_work_dir:
            remove_work_dir(work_dir)
        try:
            check_available_refs(manifest, refs, work_dir)
        finally:
            if work_dir.exists() and not args.keep_work_dir:
                remove_work_dir(work_dir)
        return 0

    if not args.allow_dirty:
        fail_on_dirty(manifest)

    if work_dir.exists() and not args.keep_work_dir:
        remove_work_dir(work_dir)

    try:
        results = update_dependencies(manifest, refs, work_dir, set(args.skip_shader_target))
        print_summary(results)
    finally:
        if work_dir.exists() and not args.keep_work_dir:
            remove_work_dir(work_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
