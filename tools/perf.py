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

"""Times the Scene3D demo target frames on the GPU: the tool behind `just perf` (macOS, Metal).

ACTION `run` builds and runs the benchmark through `just test`, with the disabled tests enabled,
and writes its JSON to `build/perf/<gpu>-<config>.json`. `compare` does the same, then has the
benchmark compare each row with the baseline of this GPU, `tests/data/yup_3d/perf/<gpu>.json`.
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
BASELINE_DIR = REPO_ROOT / "tests" / "data" / "yup_3d" / "perf"
OUTPUT_DIR = REPO_ROOT / "build" / "perf"
DEFAULT_FILTER = "SceneRendererBenchmarkTests.DISABLED_DemoTargetFrameTimes"
BENCHMARK_MODULE = "yup_renderer"
DEFAULT_MAX_LOAD = 4.0


def run_just(just: str, *arguments: str, env: dict[str, str] | None = None) -> int:
    """Runs a just recipe from the repository root and returns its exit code."""
    return subprocess.run([just, *arguments], cwd=REPO_ROOT, env=env).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("action", choices=["run", "record", "compare"], help="run, record the baseline, or compare with it")
    parser.add_argument("config", nargs="?", default="Release", help="the build configuration")
    parser.add_argument("filter", nargs="?", default=DEFAULT_FILTER, help="the gtest filter")
    parser.add_argument("--just", default="just", help="the just executable to build and test with")
    args = parser.parse_args()

    if sys.platform != "darwin":
        print("just perf runs on macOS only (Metal)", file=sys.stderr)
        return 1

    if args.action == "record" and args.config != "Release":
        print(f"The baseline is recorded in Release, not {args.config}", file=sys.stderr)
        return 1

    gpu = subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"], check=True, capture_output=True, text=True).stdout.strip()
    name = gpu.lower().replace(" ", "-")
    baseline = BASELINE_DIR / f"{name}.json"
    output = OUTPUT_DIR / f"{name}-{args.config}.json"

    env = dict(os.environ)
    env.pop("YUP_PERF_BASELINE", None)

    if args.action == "compare":
        if not baseline.is_file():
            print(f"No baseline for {gpu} at {baseline}: record it first with 'just perf record'", file=sys.stderr)
            return 1

        env["YUP_PERF_BASELINE"] = str(baseline)

    if args.action == "record":
        output = OUTPUT_DIR / f"{name}-record.json"
        output.unlink(missing_ok=True)

        # Configured and built first by a test run matching no test, so the load check sees the machine as the timing will
        if run_just(args.just, "test", args.config, "-*", BENCHMARK_MODULE) != 0:
            return 1

        max_load = float(env.get("YUP_PERF_MAX_LOAD", DEFAULT_MAX_LOAD))
        load = os.getloadavg()[0]
        if load >= max_load:
            print(f"The 1-minute load average is {load:.2f}: record on an idle machine, below {max_load:g}", file=sys.stderr)
            return 1

    output.parent.mkdir(parents=True, exist_ok=True)
    env.update({"GTEST_ALSO_RUN_DISABLED_TESTS": "1", "YUP_PERF_GPU": gpu, "YUP_PERF_OUTPUT": str(output)})

    result = run_just(args.just, "test", args.config, args.filter, BENCHMARK_MODULE, env=env)
    if args.action != "record":
        return result

    # The baseline is replaced only by a run that passed and wrote its results
    if result != 0 or not output.is_file():
        print(f"The run failed, skipped or wrote no results: {baseline} is unchanged", file=sys.stderr)
        return 1

    os.replace(output, baseline)
    print(f"Recorded {baseline}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
