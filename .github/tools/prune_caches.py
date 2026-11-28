#!/usr/bin/env python3
"""
Cache pruning for the YUP repository, which has 10 GB of GitHub Actions cache
for every branch and platform. Once that is exceeded GitHub evicts the least
recently used caches, whole platforms at a time, and their jobs build from
scratch.

Deletes, in this order:

  - the caches of deleted branches and of pull requests that are not open;
  - the build trees a configure job hands to the build jobs of its workflow run
    ("<platform>-build-<sha>-<run id>"), once that run has completed;
  - all but the newest compiler cache of each key on each ref: .github/actions/ccache
    saves a new one on every run ("ccache-<key>-<timestamp>");
  - when the rest is still over the budget, the least recently used caches of the
    refs that are not protected, until it fits. They restore the ones of the
    protected ref instead.

The budget is well under the 10 GB: a run saves its caches before the ones they
replace can be deleted here, and what GitHub would evict to make room for them
are the caches of the protected ref, which the other refs stop restoring once
they have their own.

Needs `gh` and a token allowed to write actions (reading is enough for --dry-run).

Usage:
    python3 .github/tools/prune_caches.py --dry-run
    python3 .github/tools/prune_caches.py --budget-gb 6.5 --protected-ref refs/heads/main
"""

import argparse
import json
import os
import re
import subprocess
import sys


BUILD_TREE = re.compile(r"^[a-z]+-build-[0-9a-f]{40}-(\d+)$")
COMPILER_CACHE = re.compile(r"^(ccache-.+)-\d{4}-\d{2}-\d{2}T[0-9:.]+Z$")
BRANCH_REF = "refs/heads/"
PULL_REF = re.compile(r"^refs/pull/(\d+)/merge$")
GIGABYTE = 1e9


def gh_api(endpoint, jq=None, paginate=False, method="GET"):
    command = ["gh", "api", "-X", method, endpoint] + (["--jq", jq] if jq else []) + (["--paginate"] if paginate else [])
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    return [line for line in result.stdout.splitlines() if line]


def build_tree_run(cache):
    """Return the id of the workflow run a build tree cache belongs to, or None for the other caches."""
    match = BUILD_TREE.match(cache["key"])
    return int(match.group(1)) if match else None


def plan_deletions(caches, branches, open_pulls, finished_runs, budget, protected_ref):
    """Return the (cache, reason) pairs to delete and the bytes left, caches being the entries of the caches API."""
    newest = {}
    for cache in caches:
        match = COMPILER_CACHE.match(cache["key"])
        if match:
            generation = (cache["ref"], match.group(1))
            newest[generation] = max(newest.get(generation, ""), cache["key"])

    deletions = []
    kept = []
    for cache in caches:
        ref, key = cache["ref"], cache["key"]
        pull = PULL_REF.match(ref)
        compiler = COMPILER_CACHE.match(key)

        if ref.startswith(BRANCH_REF) and ref[len(BRANCH_REF):] not in branches:
            deletions.append((cache, "branch deleted"))
        elif pull and int(pull.group(1)) not in open_pulls:
            deletions.append((cache, "pull request not open"))
        elif build_tree_run(cache) in finished_runs:
            deletions.append((cache, "run completed"))
        elif compiler and newest[(ref, compiler.group(1))] != key:
            deletions.append((cache, "newer cache saved"))
        else:
            kept.append(cache)

    remaining = sum(cache["size_in_bytes"] for cache in kept)
    for cache in sorted(kept, key=lambda cache: cache["last_accessed_at"]):
        if remaining <= budget:
            break
        if cache["ref"] != protected_ref:
            deletions.append((cache, "over budget"))
            remaining -= cache["size_in_bytes"]

    return deletions, remaining


def is_run_completed(repo, run_id):
    try:
        return gh_api(f"repos/{repo}/actions/runs/{run_id}", ".status") == ["completed"]
    except subprocess.CalledProcessError:
        return True  # The run was deleted


def main():
    parser = argparse.ArgumentParser(description="Cache pruning for YUP")
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY") or "{owner}/{repo}", help="Repository as owner/name")
    parser.add_argument("--budget-gb", type=float, default=6.5, help="Size the caches are kept under")
    parser.add_argument("--protected-ref", default="refs/heads/main", help="Ref whose caches are never deleted for the budget")
    parser.add_argument("--dry-run", action="store_true", help="Print what would be deleted without deleting it")
    args = parser.parse_args()

    try:
        caches = [json.loads(line) for line in gh_api(f"repos/{args.repo}/actions/caches?per_page=100", ".actions_caches[] | tojson", paginate=True)]
        branches = set(gh_api(f"repos/{args.repo}/branches?per_page=100", ".[].name", paginate=True))
        open_pulls = {int(number) for number in gh_api(f"repos/{args.repo}/pulls?state=open&per_page=100", ".[].number", paginate=True)}
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Error: could not query the repository: {getattr(error, 'stderr', '') or error}", file=sys.stderr)
        sys.exit(1)

    if not branches:
        print("Error: the repository lists no branch, not deleting anything", file=sys.stderr)
        sys.exit(1)

    runs = {build_tree_run(cache) for cache in caches} - {None}
    finished_runs = {run_id for run_id in runs if is_run_completed(args.repo, run_id)}

    deletions, remaining = plan_deletions(caches, branches, open_pulls, finished_runs, args.budget_gb * GIGABYTE, args.protected_ref)

    action = "Would delete" if args.dry_run else "Deleting"
    for cache, reason in deletions:
        print(f"{action} {cache['size_in_bytes'] / 1e6:7.1f} MB  {cache['ref']}  {cache['key']}  ({reason})")
        if args.dry_run:
            continue

        try:
            gh_api(f"repos/{args.repo}/actions/caches/{cache['id']}", method="DELETE")
        except subprocess.CalledProcessError as error:
            print(f"::warning::Could not delete the cache {cache['key']}: {error.stderr.strip()}")
            remaining += cache["size_in_bytes"]

    total = sum(cache["size_in_bytes"] for cache in caches)
    print(f"{len(caches)} caches of {total / GIGABYTE:.2f} GB, {len(deletions)} to delete, {remaining / GIGABYTE:.2f} GB left"
          f" (budget {args.budget_gb:.2f} GB)")

    if remaining > args.budget_gb * GIGABYTE:
        print(f"::warning::The caches are still over the budget of {args.budget_gb:.2f} GB:"
              " lower the max-size of the largest ones")


if __name__ == "__main__":
    main()
