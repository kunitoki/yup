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

"""Tells where the time of a Ninja build went: the tool behind `just build_stats`.

It reads what the last build of BUILD_DIR compiled, the outputs newer than the `.build_stats_start`
file the recipe touches before building, and their times in `.ninja_log`. It prints the wall time of
the build and the time of its steps added up, by group (tests, YUP modules, third party, examples,
link and other) and for the slowest steps.

Built with the `-ftime-trace` flag of clang (the FLAGS of the recipe), the C++ sources also have a
trace next to their object, and it prints the time of the compiler phases of each group: the
frontend, of which parsing the included headers and instantiating templates, and the backend. A
precompiled header can only save the time spent parsing the headers it holds, so the headers are
listed by where they come from (the time spent in each one, without the headers it includes) and by
cost (with the headers it includes, added up over the sources including it).

The trace is not free: clang takes minutes on a source holding a large data array, such as an
embedded font, and the times of a traced build are not the ones of a normal build. Such sources are
listed and left out of the phases.

`--name` saves the times in `build/perf/build-<name>.json`, and `--baseline` compares with the ones
saved under another name.
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from functools import cache
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_DIR = REPO_ROOT / "build" / "perf"
START_FILE = ".build_stats_start"
GROUPS = ["tests", "YUP modules", "third party", "examples", "link and other"]  # Examples are what is neither a test nor a module
ORIGINS = ["system", "third party", "YUP", "project"]
PHASES = ["Frontend", "InstantiateFunction", "InstantiateClass", "Backend"]
TRACE_OVERHEAD = "EvaluateAsRValue"  # What a traced clang spends its time on for each element of a data array


def group_of(output: str) -> str:
    """Returns the group of a build output, from its path in the build tree."""
    if not output.endswith((".o", ".obj")):
        return "link and other"
    if "/thirdparty/" in output or "_deps/" in output or "/externals/" in output:
        return "third party"
    if "/modules/" in output:
        return "YUP modules"
    return "tests" if output.startswith("tests/") else "examples"


@cache
def origin_of(header: str) -> str:
    """Returns where a header comes from: the system (SDK, toolchain), a third party, the YUP modules or the project."""
    try:
        relative = Path(header).resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return "system"
    if relative.startswith("modules/"):
        return "YUP"
    return "third party" if relative.startswith(("thirdparty/", "build/")) else "project"


def read_steps(build_dir: Path) -> tuple[dict[str, float], float]:
    """Returns the seconds each output of the last build took and the seconds the build took, from the Ninja log."""
    start = (build_dir / START_FILE).stat().st_mtime_ns
    steps = {}
    ends = {}
    for line in (build_dir / ".ninja_log").read_text().splitlines():
        fields = line.split("\t")
        if len(fields) == 5 and int(fields[2]) >= start:
            steps[fields[3]] = (int(fields[1]) - int(fields[0])) / 1000.0
            ends[fields[3]] = int(fields[1]) / 1000.0
    return steps, max(ends.values(), default=0.0)


def read_trace(trace: Path, phases: dict[str, float], self_times: dict[str, float], headers: dict[str, list[float]]) -> float:
    """Adds the compiler phases of a -ftime-trace file and the time spent in each header it included.

    Returns the seconds the trace itself cost instead, when that is most of the compilation.
    """
    events = json.loads(trace.read_text())["traceEvents"]
    totals = {event["name"][6:]: event["dur"] / 1e6 for event in events if event.get("name", "").startswith("Total ")}
    if totals.get(TRACE_OVERHEAD, 0.0) > 0.5 * totals.get("ExecuteCompiler", 0.0):
        return totals[TRACE_OVERHEAD]

    for phase in PHASES:
        phases[phase] += totals.get(phase, 0.0)

    # Each include is a begin event followed by its end event, written when it ends: the ones it includes come before it
    includes = []
    for event in events:
        if event.get("name") == "Source" and event["ph"] == "b":
            includes.append([event["ts"], 0, event["args"]["detail"]])
        elif event.get("name") == "Source" and event["ph"] == "e" and includes:
            includes[-1][1] = event["ts"]

    stack: list[list] = []
    for begin, end, header in sorted(includes, key=lambda include: (include[0], -include[1])):
        while stack and stack[-1][0] <= begin:
            stack.pop()
        duration = (end - begin) / 1e6
        self_times[origin_of(header)] += duration
        if stack:
            self_times[stack[-1][1]] -= duration
        headers[header][0] += duration
        headers[header][1] += 1
        stack.append([end, origin_of(header)])

    return 0.0


def collect(build_dir: Path) -> dict:
    """Returns the times of the last build of a build directory."""
    steps, wall = read_steps(build_dir)
    start = (build_dir / START_FILE).stat().st_mtime_ns
    groups = {group: {"steps": 0, "seconds": 0.0, "traced": 0, "phases": defaultdict(float), "parsing": defaultdict(float)} for group in GROUPS}
    headers: dict[str, list[float]] = defaultdict(lambda: [0.0, 0])
    slowed = []

    for output, seconds in steps.items():
        group = groups[group_of(output)]
        group["steps"] += 1
        group["seconds"] += seconds

        trace = (build_dir / output).with_suffix(".json")
        if output.endswith(".o") and trace.is_file() and trace.stat().st_mtime_ns >= start:
            overhead = read_trace(trace, group["phases"], group["parsing"], headers)
            if overhead:
                slowed.append([output, overhead])
            else:
                group["traced"] += 1

    return {
        "wall": wall,
        "seconds": sum(steps.values()),
        "groups": groups,
        "slowest": sorted(steps.items(), key=lambda step: -step[1])[:15],
        "slowed": slowed,
        "headers": sorted(([header, seconds, int(count)] for header, (seconds, count) in headers.items()), key=lambda header: -header[1])[:25],
    }


def print_report(stats: dict, baseline: dict | None) -> None:
    def versus(seconds: float, before: float | None) -> str:
        return "" if not before else f"   {(seconds - before) / before:+7.1%} from {before:.1f} s"

    steps = sum(group["steps"] for group in stats["groups"].values())
    print(f"\nBuilt {steps} steps in {stats['wall']:.1f} s{versus(stats['wall'], baseline and baseline['wall'])}")
    print(f"Steps added up: {stats['seconds']:.1f} s{versus(stats['seconds'], baseline and baseline['seconds'])}")

    print(f"\n{'Group':<16}{'steps':>7}{'seconds':>10}{'share':>8}")
    for name, group in stats["groups"].items():
        if group["steps"]:
            before = baseline and baseline["groups"].get(name, {}).get("seconds")
            print(f"{name:<16}{group['steps']:>7}{group['seconds']:>10.1f}{group['seconds'] / stats['seconds']:>8.1%}{versus(group['seconds'], before)}")

    print("\nSlowest steps")
    for output, seconds in stats["slowest"]:
        print(f"{seconds:>8.1f} s  {output}")

    traced = {name: group for name, group in stats["groups"].items() if group["traced"]}
    if not traced:
        return

    if stats["slowed"]:
        print("\nSlowed down by the trace and left out of what follows")
        for output, seconds in stats["slowed"]:
            print(f"{seconds:>8.1f} s  {output}")

    print(f"\nCompiler phases of the C++ sources, in seconds\n{'Group':<16}{'sources':>8}{'frontend':>10}{'headers':>9}{'templates':>11}{'backend':>9}")
    for name, group in traced.items():
        phases = group["phases"]
        templates = phases["InstantiateFunction"] + phases["InstantiateClass"]
        print(f"{name:<16}{group['traced']:>8}{phases['Frontend']:>10.1f}{sum(group['parsing'].values()):>9.1f}{templates:>11.1f}{phases['Backend']:>9.1f}")

    print(f"\nTime parsing headers by where they come from, in seconds\n{'Group':<16}" + "".join(f"{origin:>13}" for origin in ORIGINS))
    for name, group in traced.items():
        print(f"{name:<16}" + "".join(f"{group['parsing'][origin]:>13.1f}" for origin in ORIGINS))

    print("\nMost expensive headers, with the ones they include and added up over the sources including them")
    for header, seconds, count in stats["headers"]:
        path = Path(header)
        print(f"{seconds:>8.1f} s {count:>4}x  {path.relative_to(REPO_ROOT) if path.is_relative_to(REPO_ROOT) else header}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("build_dir", type=Path, help="the Ninja build directory")
    parser.add_argument("--name", help="save the times under this name")
    parser.add_argument("--baseline", help="compare with the times saved under this name")
    args = parser.parse_args()

    if not (args.build_dir / START_FILE).is_file() or not (args.build_dir / ".ninja_log").is_file():
        print(f"error: {args.build_dir} has no {START_FILE} and .ninja_log: build it with `just build_stats`", file=sys.stderr)
        return 1

    baseline = None
    if args.baseline:
        baseline_file = OUTPUT_DIR / f"build-{args.baseline}.json"
        if not baseline_file.is_file():
            print(f"error: no baseline {baseline_file}", file=sys.stderr)
            return 1
        baseline = json.loads(baseline_file.read_text())

    stats = collect(args.build_dir)
    print_report(stats, baseline)

    if args.name:
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        (OUTPUT_DIR / f"build-{args.name}.json").write_text(json.dumps(stats, indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
