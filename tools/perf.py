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

"""Times the benchmarks of a suite on the GPU: the tool behind `just perf_3d` and `just perf_2d` (macOS, Metal).

The suite is `3d` by default, the Scene3D target frames of yup_renderer, or `2d` with `--suite 2d`,
the painting, restyling and document benchmarks of yup_gui.

ACTION `run` builds and runs the benchmarks through `just test`, with the disabled tests enabled,
and writes their JSON to `build/perf/<gpu>-<config>.json` (`<gpu>-2d-<config>.json` for 2D).
`compare` does the same, then has the benchmarks compare each row with the baseline of this GPU,
`tests/data/yup_3d/perf/<gpu>.json` (`tests/data/yup_gui/perf/<gpu>.json` for 2D).
`record` (Release only) builds first, refuses when the 1-minute load average is at or above
`YUP_PERF_MAX_LOAD` (default 4), runs, and replaces the baseline only when the run passed and wrote
its results. The GPU name is the SoC's (`machdep.cpu.brand_string`), lowercase with dashes for spaces
in file names. See tests/data/yup_3d/perf/README.md.

Exits non-zero on every failure: an unknown action, a refused record, a missing baseline, a failed
build or run, or a record that wrote nothing.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_DIR = REPO_ROOT / "build" / "perf"
DEFAULT_MAX_LOAD = 4.0

# A suite: the test module holding its benchmarks, the folder of tests/data keeping its baselines,
# what follows the GPU in the names of its files of build/perf, and its default gtest filter
SUITES = {
    "3d": ("yup_renderer", "yup_3d", "", "SceneRendererBenchmarkTests.DISABLED_*TargetFrameTimes"),
    "2d": ("yup_gui", "yup_gui", "-2d", "PaintBenchmarkTests.DISABLED_*"),
}


def run_just(just: str, *arguments: str, env: dict[str, str] | None = None) -> int:
    """Runs a just recipe from the repository root and returns its exit code."""
    return subprocess.run([just, *arguments], cwd=REPO_ROOT, env=env).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("action", choices=["run", "record", "compare"], help="run, record the baseline, or compare with it")
    parser.add_argument("config", nargs="?", default="Release", help="the build configuration")
    parser.add_argument("filter", nargs="?", help="the gtest filter, by default every benchmark of the suite")
    parser.add_argument("--suite", choices=sorted(SUITES), default="3d", help="the benchmarks to time")
    parser.add_argument("--just", default="just", help="the just executable to build and test with")
    args = parser.parse_args()
    module, data, suffix, default_filter = SUITES[args.suite]

    if sys.platform != "darwin":
        print("just perf runs on macOS only (Metal)", file=sys.stderr)
        return 1

    if args.action == "record" and args.config != "Release":
        print(f"The baseline is recorded in Release, not {args.config}", file=sys.stderr)
        return 1

    gpu = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"], check=True, capture_output=True, text=True).stdout.strip()
    name = gpu.lower().replace(" ", "-")
    baseline = REPO_ROOT / "tests" / "data" / data / "perf" / f"{name}.json"
    output = OUTPUT_DIR / f"{name}{suffix}-{args.config}.json"

    env = dict(os.environ)
    env.pop("YUP_PERF_BASELINE", None)

    if args.action == "compare":
        if not baseline.is_file():
            print(f"No baseline for {gpu} at {baseline}: record it first with 'just perf_{args.suite} record'", file=sys.stderr)
            return 1

        env["YUP_PERF_BASELINE"] = str(baseline)

    if args.action == "record":
        output = OUTPUT_DIR / f"{name}{suffix}-record.json"
        output.unlink(missing_ok=True)

        # Configured and built first by a test run matching no test, so the load check sees the machine as the timing will
        if run_just(args.just, "test", args.config, "-*", module) != 0:
            return 1

        max_load = float(env.get("YUP_PERF_MAX_LOAD", DEFAULT_MAX_LOAD))
        load = os.getloadavg()[0]
        if load >= max_load:
            print(f"The 1-minute load average is {load:.2f}: record on an idle machine, below {max_load:g}", file=sys.stderr)
            return 1

    output.parent.mkdir(parents=True, exist_ok=True)
    env.update({"GTEST_ALSO_RUN_DISABLED_TESTS": "1", "YUP_PERF_GPU": gpu, "YUP_PERF_OUTPUT": str(output)})

    result = run_just(args.just, "test", args.config, args.filter or default_filter, module, env=env)
    if args.action != "record":
        return result

    # The baseline is replaced only by a run that passed and wrote its results
    if result != 0 or not output.is_file():
        print(f"The run failed, skipped or wrote no results: {baseline} is unchanged", file=sys.stderr)
        return 1

    baseline.parent.mkdir(parents=True, exist_ok=True)
    os.replace(output, baseline)
    print(f"Recorded {baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
