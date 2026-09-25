#!/usr/bin/env python3
"""
Smart CI for the YUP repository.

Classifies the files changed on the current branch, parses the yup and
thirdparty module declaration headers (dependencies / optionalDeps / platform
Deps / testDeps lines) into a dependency graph and the examples CMakeLists into
example -> module mappings, then prints the set of components that must be built
and tested for the change:

  - "full_build": true when a global file (build system, workflow, config,
    test infrastructure) changed, when a file matches no rule, or when a changed
    component cannot be resolved confidently. The workflow then builds and runs
    everything.
  - "tests": the yup modules whose tests must run. Passed to cmake as
    YUP_TEST_MODULES, which links their optional and test deps on its own.
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
HEADER_DEP_LINE_RE = re.compile(r"\s*(dependencies|[A-Za-z]+Deps):\s*(.*)")


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
        self.dependents = defaultdict(set)       # dep -> {yup/thirdparty modules using it, required or optional}
        self.test_dependents = defaultdict(set)  # dep -> {yup modules whose tests use it}
        self.example_deps = {}                   # example -> {deps}


def parse_module_header(header):
    """Return (dependency_tokens, test_dependency_tokens) from a module declaration block."""
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
        tokens = set(dep_match.group(2).replace(",", " ").split())
        if dep_match.group(1) == "testDeps":
            test_deps |= tokens
        else:
            deps |= tokens
    return deps, test_deps


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

    # An optional consumer compiles against its dep whenever both are linked,
    # so optional edges propagate changes exactly like required ones.
    for module in known:
        folder = modules_dir if module in graph.yup_modules else thirdparty_dir
        deps, test_deps = parse_module_header(folder / module / f"{module}.h")

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
def classify_file(file_path, config):
    """Return the (kind, name) hits for a file from the first matching rule, or None when no rule matches."""
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
def compute_affected(changed_files, config, graph):
    changed_yup = set()
    changed_tp = set()
    changed_examples = set()
    test_components = set()
    full_build = False

    for file_path in changed_files:
        hits = classify_file(file_path, config)
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

    if full_build:
        return {
            "full_build": True,
            "tests": [],
            "examples": sorted(graph.examples),
        }

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

    return {
        "full_build": False,
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
    print(f"Tests: {', '.join(result['tests']) or '(none)'}")
    print(f"Examples: {', '.join(result['examples']) or '(none)'}")
    print(f"Output written to {output_path}")


if __name__ == "__main__":
    main()
