#!/usr/bin/env python3
"""
Smart CI for the YUP repository.

Classifies the files changed on the current branch, parses the YUP module
declaration headers (dependencies / optionalDeps / platform Deps lines) into a
dependency graph and the examples CMakeLists into example -> module mappings,
then prints the set of components that must be built and tested for the change:

  - "full_build": true when a global file (build system, workflow, config,
    test infrastructure) changed, or when a changed component cannot be
    resolved confidently. The workflow then builds and runs everything.
  - "modules": the yup_* modules under test plus the thirdparty link targets
    they need (optional deps are not linked automatically by the build system,
    so they must be listed explicitly). Passed to cmake as YUP_TEST_MODULES.
  - "tests": the affected yup modules (informational).
  - "examples": the examples that changed or that depend on an affected module
    or thirdparty target.

Usage:
    python3 .github/tools/smart_ci.py --base origin/main \
        --config .github/smart_ci_config.json --output affected.json
    python3 .github/tools/smart_ci.py --files modules/yup_core/yup_core.h \
        --config .github/smart_ci_config.json --output affected.json

The --files form skips git and is handy to preview what a change would trigger.
"""

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict, deque
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
EMPTY_SHA = "0" * 40

MODULE_NAME_RE = re.compile(r"^yup_")
HEADER_DEP_LINE_RE = re.compile(r"\s*(?:dependencies|[A-Za-z]+Deps):\s*(.*)")


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------
def run_git(args):
    return subprocess.run(["git", *args], capture_output=True, text=True, check=True)


def get_changed_files(base):
    """Return the files changed between base and HEAD (git diff base...HEAD)."""
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
        print(f"Error: could not diff against '{base}': {error.stderr.strip()}", file=sys.stderr)
        sys.exit(1)

    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


# ---------------------------------------------------------------------------
# Dependency graph
# ---------------------------------------------------------------------------
class ModuleGraph:
    def __init__(self):
        self.yup_modules = set()
        self.tp_names = set()
        self.examples = set()
        self.required_yup = {}                       # module -> {yup deps}
        self.optional_tokens = {}                    # module -> {optional deps: yup + thirdparty}
        self.reverse_required = defaultdict(set)     # required yup dep -> {modules requiring it}
        self.reverse_optional = defaultdict(set)     # optional yup dep -> {modules optionally requiring it}
        self.reverse_tp = defaultdict(set)           # thirdparty -> {modules using it}
        self.example_deps = {}                       # example -> {deps}


def parse_module_header(header):
    """Return (required_tokens, optional_tokens) from a module declaration block."""
    try:
        text = header.read_text(errors="ignore")
    except OSError:
        return set(), set()

    match = re.search(r"BEGIN_YUP_MODULE_DECLARATION(.*?)END_YUP_MODULE_DECLARATION", text, re.DOTALL)
    block = match.group(1) if match else text

    required = set()
    optional = set()
    for line in block.splitlines():
        dep_match = HEADER_DEP_LINE_RE.match(line)
        if not dep_match:
            continue
        tokens = set(dep_match.group(1).split())
        if line.lstrip().startswith("optionalDeps"):
            optional |= tokens
        else:
            required |= tokens
    return required, optional


def build_graph(root):
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

    for module in graph.yup_modules:
        header = modules_dir / module / f"{module}.h"
        required, optional = parse_module_header(header)

        yup_required = required & graph.yup_modules
        graph.required_yup[module] = yup_required
        graph.optional_tokens[module] = optional & known

        for dep in yup_required:
            graph.reverse_required[dep].add(module)

        for dep in optional & graph.yup_modules:
            graph.reverse_optional[dep].add(module)

        for token in (required | optional) & graph.tp_names:
            graph.reverse_tp[token].add(module)

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
def classify_file(file_path, config):
    """Return the (kind, name) hits for a file, from the first matching rule."""
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
    return hits


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
def compute_affected(changed_files, config, graph):
    changed_yup = set()
    changed_tp = set()
    changed_examples = set()
    test_components = set()
    full_build = False

    for file_path in changed_files:
        for kind, name in classify_file(file_path, config):
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

    if full_build:
        return {
            "full_build": True,
            "modules": [],
            "tests": [],
            "examples": sorted(graph.examples),
        }

    # A thirdparty change affects every module that (optionally) uses it, and a
    # code change can break its dependents - required and optional alike (an
    # optional consumer compiles against the module when both are linked, so it
    # can break too). Test-file changes are scoped to their own module only.
    affected = set(changed_yup)
    for tp in changed_tp:
        affected |= graph.reverse_tp.get(tp, set())
    code_affected = reverse_closure(affected, graph.reverse_required)
    code_affected |= reverse_closure(affected, graph.reverse_optional)
    test_set = code_affected | test_components

    # Optional deps are not linked automatically by the build system, so they
    # must be listed explicitly or the corresponding features stay compiled out.
    link = set(test_set)
    for module in test_set:
        link |= graph.optional_tokens.get(module, set())
    link |= changed_tp
    modules = sorted(link & (graph.yup_modules | graph.tp_names))

    if not (test_set & graph.yup_modules):
        modules = []  # e.g. a thirdparty with no yup consumers: no test build

    # Examples rebuild when they changed directly or depend on an affected
    # module or thirdparty target (test-only changes do not affect examples).
    affected_components = code_affected | changed_tp
    examples = set(changed_examples)
    for example, deps in graph.example_deps.items():
        if deps & affected_components:
            examples.add(example)

    return {
        "full_build": False,
        "modules": modules,
        "tests": sorted(test_set),
        "examples": sorted(examples),
    }


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Smart CI for YUP")
    parser.add_argument("--base", default="main", help="Base ref/sha to diff against (default: main)")
    parser.add_argument("--config", default=".github/smart_ci_config.json", help="Classification config")
    parser.add_argument("--output", required=True, help="Output JSON file")
    parser.add_argument("--files", nargs="*", help="Explicit changed files (skips git)")
    args = parser.parse_args()

    config_path = Path(args.config)
    if not config_path.is_file():
        print(f"Error: config file not found: {config_path}", file=sys.stderr)
        sys.exit(1)
    config = json.loads(config_path.read_text())

    changed_files = args.files if args.files is not None else get_changed_files(args.base)
    print(f"Found {len(changed_files)} changed files.")

    graph = build_graph(REPO_ROOT)
    result = compute_affected(changed_files, config, graph)

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2))

    print(f"Full build: {result['full_build']}")
    print(f"Modules: {', '.join(result['modules']) or '(none)'}")
    print(f"Tests: {', '.join(result['tests']) or '(none)'}")
    print(f"Examples: {', '.join(result['examples']) or '(none)'}")
    print(f"Output written to {output_path}")


if __name__ == "__main__":
    main()
