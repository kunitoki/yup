#!/usr/bin/env python3
"""
Smart CI for the YUP repository.

Classifies the files changed on the current branch, parses the yup and
thirdparty module declaration headers (dependencies / optionalDeps / platform
Deps / testDeps lines) into a dependency graph and the examples CMakeLists into
example -> module mappings, then prints the set of components that must be built
and tested for the change:

  - "full_build": true when a global file (build system, workflow, config,
    test infrastructure) changed, when a file matches no rule, when a changed
    component cannot be resolved confidently, when git cannot produce the diff,
    or when --full is passed. The workflow then builds and runs everything.
  - "tests": the yup modules whose tests must run. Passed to cmake as
    YUP_TEST_MODULES, which links their optional and test deps on its own.
  - "examples": the examples that changed or that depend on an affected module
    or thirdparty target.
  - "example_targets": with --platform, the build targets of each example.

With --platform, the "platforms" section of the config restricts the result to
that platform: native files of other platforms are ignored, only that platform's
<platform>Deps header lines count, and only the examples it builds are listed.

Exactly one scope selects the changed files:

  - --since-last-green JOB: the files changed since the last push run of the CI
    workflow on this branch where every job of the platform caller JOB (named JOB
    or "JOB / ...") succeeded or was skipped. Failed or cancelled work is never
    green, so it stays in the diff until it passes. Needs `gh` and a token.
  - --since SHA: the files changed since an explicit ancestor of HEAD.
  - --base REF: the files changed since the merge-base with REF.
  - --files FILE...: an explicit list, skipping git.
  - --full: everything.

--since-last-green and --since fall back to the origin/main diff when no
usable ancestor is found.

Usage:
    python3 .github/tools/smart_ci.py --since-last-green Linux --platform linux \
        --config .github/smart_ci_config.json --output affected.json
    python3 .github/tools/smart_ci.py --since HEAD~1 --platform linux --output affected.json
    python3 .github/tools/smart_ci.py --base origin/main \
        --config .github/smart_ci_config.json --output affected.json
    python3 .github/tools/smart_ci.py --files modules/yup_core/yup_core.h \
        --platform mac --config .github/smart_ci_config.json --output affected.json

The --files form skips git and is handy to preview what a change would trigger.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict, deque
from pathlib import Path
from urllib.parse import quote

REPO_ROOT = Path(__file__).resolve().parents[2]
EMPTY_SHA = "0" * 40
CI_WORKFLOW = "ci.yml"
PLAN_JOB = "plan"
MAX_RUNS = 30

MODULE_NAME_RE = re.compile(r"^yup_")
HEADER_DEP_LINE_RE = re.compile(r"\s*(dependencies|[A-Za-z]+Deps):\s*(.*)")


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------
def run_git(args):
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True)


def get_changed_files(base):
    """Return the files changed between base and HEAD (git diff base...HEAD), or None when git cannot diff."""
    if base == EMPTY_SHA:
        base = "origin/main"

    if base.startswith("origin/"):
        branch = base.split("/", 1)[1]
        try:
            run_git(["fetch", "origin", branch])
        except subprocess.CalledProcessError as error:
            print(f"Warning: could not fetch origin/{branch}: {error.stderr.strip()}", file=sys.stderr)

    try:
        result = run_git(["diff", "--name-only", f"{base}...HEAD"])
    except subprocess.CalledProcessError as error:
        print(f"Warning: could not diff against '{base}', forcing a full build: {error.stderr.strip()}", file=sys.stderr)
        return None

    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def is_ancestor(sha):
    return bool(sha) and subprocess.run(["git", "merge-base", "--is-ancestor", sha, "HEAD"], capture_output=True).returncode == 0


def get_changed_files_since(sha):
    """Return the files changed between sha and HEAD, or the origin/main diff when sha is not an ancestor of HEAD."""
    if not is_ancestor(sha):
        print(f"Warning: '{sha}' is not an ancestor of HEAD, diffing against origin/main", file=sys.stderr)
        return get_changed_files("origin/main")

    return get_changed_files(sha)


def gh_api(endpoint, jq, paginate=False):
    command = ["gh", "api", "-X", "GET", endpoint, "--jq", jq] + (["--paginate"] if paginate else [])
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return [line.split("\t") for line in result.stdout.splitlines() if line]


def find_last_green(job_name):
    """Return the head sha of the newest push run on this branch that is green for job_name, or None."""
    repo = os.environ.get("GITHUB_REPOSITORY") or "{owner}/{repo}"
    branch = os.environ.get("GITHUB_REF_NAME") or run_git(["rev-parse", "--abbrev-ref", "HEAD"]).stdout.strip()

    try:
        runs = gh_api(f"repos/{repo}/actions/workflows/{CI_WORKFLOW}/runs"
                      f"?branch={quote(branch, safe='')}&event=push&status=completed&per_page={MAX_RUNS}",
                      ".workflow_runs[] | [.id, .head_sha, .conclusion] | @tsv")

        for run_id, head_sha, conclusion in runs:
            if conclusion == "cancelled" or not is_ancestor(head_sha):
                continue

            jobs = gh_api(f"repos/{repo}/actions/runs/{run_id}/jobs?per_page=100",
                          '.jobs[] | [.name, .conclusion // ""] | @tsv', paginate=True)
            if [PLAN_JOB, "success"] not in jobs:
                continue

            platform_jobs = [result for name, result in jobs if name == job_name or name.startswith(f"{job_name} / ")]
            if platform_jobs and all(result in ("success", "skipped") for result in platform_jobs):
                print(f"Last green run for {job_name}: {run_id} ({head_sha})")
                return head_sha
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Warning: could not query the CI runs, diffing against origin/main: {getattr(error, 'stderr', '') or error}", file=sys.stderr)
        return None

    print(f"Warning: no green run for {job_name} on '{branch}', diffing against origin/main", file=sys.stderr)
    return None


# ---------------------------------------------------------------------------
# Dependency graph
# ---------------------------------------------------------------------------
class ModuleGraph:
    def __init__(self):
        self.yup_modules = set()
        self.tp_names = set()
        self.examples = set()
        self.dependents = defaultdict(set)       # dep -> {yup/thirdparty modules using it, required or optional}
        self.test_dependents = defaultdict(set)  # dep -> {yup modules whose tests use it}
        self.example_deps = {}                   # example -> {deps}


def parse_module_header(header, platform_deps=None):
    """Return (dependency_tokens, test_dependency_tokens) from a module declaration block.

    platform_deps names the <platform>Deps lines to honour; None honours all of them.
    """
    try:
        text = header.read_text(errors="ignore")
    except OSError:
        return set(), set()

    match = re.search(r"BEGIN_YUP_MODULE_DECLARATION(.*?)END_YUP_MODULE_DECLARATION", text, re.DOTALL)
    block = match.group(1) if match else text

    deps = set()
    test_deps = set()
    for line in block.splitlines():
        dep_match = HEADER_DEP_LINE_RE.match(line)
        if not dep_match:
            continue
        key = dep_match.group(1)
        tokens = set(dep_match.group(2).replace(",", " ").split())
        if key == "testDeps":
            test_deps |= tokens
        elif key in ("dependencies", "optionalDeps") or platform_deps is None or key in platform_deps:
            deps |= tokens
    return deps, test_deps


def build_graph(root, platform_deps=None):
    graph = ModuleGraph()

    modules_dir = root / "modules"
    thirdparty_dir = root / "thirdparty"
    examples_dir = root / "examples"

    graph.yup_modules = {
        entry.name for entry in modules_dir.iterdir()
        if entry.is_dir() and MODULE_NAME_RE.match(entry.name)
    } if modules_dir.is_dir() else set()

    graph.tp_names = {
        entry.name for entry in thirdparty_dir.iterdir() if entry.is_dir()
    } if thirdparty_dir.is_dir() else set()

    graph.examples = {
        entry.name for entry in examples_dir.iterdir()
        if entry.is_dir() and (entry / "CMakeLists.txt").is_file()
    } if examples_dir.is_dir() else set()

    known = graph.yup_modules | graph.tp_names

    # An optional consumer compiles against its dep whenever both are linked,
    # so optional edges propagate changes exactly like required ones.
    for module in known:
        folder = modules_dir if module in graph.yup_modules else thirdparty_dir
        deps, test_deps = parse_module_header(folder / module / f"{module}.h", platform_deps)

        for dep in deps & known:
            graph.dependents[dep].add(module)

        if module in graph.yup_modules:
            for dep in test_deps & known:
                graph.test_dependents[dep].add(module)

    # Examples: yup:: module references plus bare thirdparty target names.
    for example in graph.examples:
        text = (examples_dir / example / "CMakeLists.txt").read_text(errors="ignore")
        deps = set(re.findall(r"yup::(yup_\w+)", text))
        deps &= graph.yup_modules
        for tp in graph.tp_names:
            if re.search(rf"\b{re.escape(tp)}\b", text):
                deps.add(tp)
        graph.example_deps[example] = deps

    return graph


# ---------------------------------------------------------------------------
# Classification
# ---------------------------------------------------------------------------
def classify_file(file_path, config, platform_skip=()):
    """Return the (kind, name) hits for a file from the first matching rule, or None when no rule matches."""
    if any(re.search(pattern, file_path) for pattern in platform_skip):
        return []

    hits = []
    for rules in config.get("patterns", {}).values():
        for rule in rules:
            match = re.match(rule["pattern"], file_path)
            if not match:
                continue

            for target in rule.get("targets", []):
                def substitute(group_match):
                    try:
                        return match.group(int(group_match.group(1))) or ""
                    except IndexError:
                        return ""
                target = re.sub(r"\$(\d)", substitute, target)
                if ":" in target:
                    kind, name = target.split(":", 1)
                else:
                    kind, name = target, target
                hits.append((kind, name))
            return hits
    return None


def reverse_closure(start, reverse):
    result = set(start)
    queue = deque(result)
    while queue:
        node = queue.popleft()
        for dependent in reverse.get(node, ()):
            if dependent not in result:
                result.add(dependent)
                queue.append(dependent)
    return result


# ---------------------------------------------------------------------------
# Main computation
# ---------------------------------------------------------------------------
def compute_affected(changed_files, config, graph, platform=None):
    """changed_files of None means the diff is unknown, which is a full build."""
    platform = platform or {}
    platform_skip = platform.get("skip", [])
    platform_examples = platform.get("examples", {})
    supported_examples = set(platform_examples) & graph.examples if platform else graph.examples

    changed_yup = set()
    changed_tp = set()
    changed_examples = set()
    test_components = set()
    full_build = changed_files is None

    for file_path in changed_files or []:
        hits = classify_file(file_path, config, platform_skip)
        if hits is None:
            print(f"Warning: '{file_path}' matches no rule, forcing a full build", file=sys.stderr)
            full_build = True
            continue

        for kind, name in hits:
            if kind == "all":
                full_build = True
            elif kind == "module":
                changed_yup.add(name)
            elif kind == "tests":
                if name == "all":
                    full_build = True
                else:
                    test_components.add(name)  # a test component is named after its module
            elif kind == "examples":
                changed_examples.add(name)
            elif kind == "thirdparty":
                changed_tp.add(name)

    # Unresolvable components are a full rebuild: better to over-test than miss.
    for component in changed_yup:
        if component not in graph.yup_modules:
            print(f"Warning: module '{component}' is not a known yup module, forcing a full build", file=sys.stderr)
            full_build = True
    for component in test_components:
        if component not in graph.yup_modules:
            print(f"Warning: test component '{component}' is not a known yup module, forcing a full build", file=sys.stderr)
            full_build = True
    for component in changed_tp:
        if component not in graph.tp_names:
            print(f"Warning: thirdparty target '{component}' is not known, forcing a full build", file=sys.stderr)
            full_build = True
    for component in changed_examples:
        if component not in graph.examples:
            print(f"Warning: example '{component}' is not known, forcing a full build", file=sys.stderr)
            full_build = True

    def result(full, tests, examples):
        examples = sorted(examples & supported_examples)
        return {
            "full_build": full,
            "tests": sorted(tests) if platform.get("tests", True) else [],
            "examples": examples,
            "example_targets": {example: platform_examples[example] for example in examples} if platform else {},
        }

    if full_build:
        return result(True, set(), graph.examples)

    # A code change (yup or thirdparty) affects every transitive dependent. Tests
    # also rerun when a module their testDeps name is affected; test-file changes
    # are scoped to their own module only.
    code_affected = reverse_closure(changed_yup | changed_tp, graph.dependents)
    test_set = (code_affected | test_components) & graph.yup_modules
    for dep in code_affected:
        test_set |= graph.test_dependents.get(dep, set())

    # Examples rebuild when they changed directly or depend on an affected
    # module or thirdparty target (test-only changes do not affect examples).
    examples = set(changed_examples)
    for example, deps in graph.example_deps.items():
        if deps & code_affected:
            examples.add(example)

    return result(False, test_set, examples)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Smart CI for YUP")
    scope = parser.add_mutually_exclusive_group(required=True)
    scope.add_argument("--since-last-green", metavar="PLATFORM_JOB", help="Diff against the last green CI push run of this caller job")
    scope.add_argument("--since", metavar="SHA", help="Diff against an ancestor of HEAD")
    scope.add_argument("--base", metavar="REF", help="Diff against the merge-base with this ref")
    scope.add_argument("--files", nargs="*", help="Explicit changed files (skips git)")
    scope.add_argument("--full", action="store_true", help="Force a full build (skips git)")
    parser.add_argument("--config", default=".github/smart_ci_config.json", help="Classification config")
    parser.add_argument("--output", required=True, help="Output JSON file")
    parser.add_argument("--platform", help="Restrict the result to a platform of the config's 'platforms' section")
    args = parser.parse_args()

    config_path = Path(args.config)
    if not config_path.is_file():
        print(f"Error: config file not found: {config_path}", file=sys.stderr)
        sys.exit(1)
    config = json.loads(config_path.read_text())

    platform = None
    if args.platform:
        platform = config.get("platforms", {}).get(args.platform)
        if platform is None:
            print(f"Error: platform '{args.platform}' is not in the config", file=sys.stderr)
            sys.exit(1)

    if args.full:
        changed_files = None
    elif args.files is not None:
        changed_files = args.files
    elif args.since_last_green is not None:
        last_green = find_last_green(args.since_last_green)
        changed_files = get_changed_files_since(last_green) if last_green else get_changed_files("origin/main")
    elif args.since is not None:
        changed_files = get_changed_files_since(args.since)
    else:
        changed_files = get_changed_files(args.base)
    if changed_files is not None:
        print(f"Found {len(changed_files)} changed files:")
        for file_path in changed_files:
            print(f"  {file_path}")

    graph = build_graph(REPO_ROOT, platform.get("deps", []) if platform else None)
    result = compute_affected(changed_files, config, graph, platform)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2))

    print(f"Full build: {result['full_build']}")
    print(f"Tests: {', '.join(result['tests']) or '(none)'}")
    print(f"Examples: {', '.join(result['examples']) or '(none)'}")
    print(f"Output written to {output_path}")


if __name__ == "__main__":
    main()
