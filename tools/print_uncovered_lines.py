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

from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path


DEFAULT_REPO = "kunitoki/yup"

DEFAULT_TARGETS = [
    "yup_GpuPipeline.cpp",
    "yup_GpuRenderPass.cpp",
    "yup_GpuCanvas.cpp",
    "yup_GpuBuffer.cpp",
    "yup_GpuTexture.cpp",
    "yup_Image.cpp",
    "yup_TypeErasedObject.h",
    "yup_Graphics.cpp",
    "yup_GpuPipelineCache.cpp",
    "yup_GpuFrame.cpp",
]


def get_github_token() -> str:
    """Return the GitHub token from environment variables."""
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if not token:
        print(
            "Error: GITHUB_TOKEN or GH_TOKEN environment variable is not set.",
            file=sys.stderr,
        )
        sys.exit(1)
    return token


def _report_github_http_error(exc: urllib.error.HTTPError, url: str) -> None:
    """Print a user-friendly error for a failed GitHub API call."""
    code = exc.code
    body = exc.read().decode(errors="replace")
    detail = ""

    # Try to extract the "message" field from the JSON body.
    try:
        body_data = json.loads(body)
        if isinstance(body_data, dict):
            detail = body_data.get("message", "")
    except (json.JSONDecodeError, TypeError):
        pass

    if code == 401:
        print(
            "Error: authentication failed (HTTP 401).\n"
            "  → Your GITHUB_TOKEN is invalid or expired.\n"
            "  → Generate a new token at https://github.com/settings/tokens",
            file=sys.stderr,
        )
        if detail:
            print(f"  API message: {detail}", file=sys.stderr)
    elif code == 403:
        print(
            "Error: access denied (HTTP 403).\n"
            "  → Your token may lack permission to read this repository or its actions.\n"
            "  → For a public repo, the token needs at least 'actions:read' scope.\n"
            "  → For a private repo, the token also needs 'repo' scope.",
            file=sys.stderr,
        )
        if detail:
            print(f"  API message: {detail}", file=sys.stderr)
    elif code == 404:
        print(
            f"Error: resource not found (HTTP 404).\n"
            f"  → Double-check the PR number and repository name.\n"
            f"  → URL: {url}",
            file=sys.stderr,
        )
        if detail:
            print(f"  API message: {detail}", file=sys.stderr)
    else:
        print(
            f"Error: GitHub API request failed ({code}).",
            file=sys.stderr,
        )
        if detail:
            print(f"  {detail}", file=sys.stderr)
        else:
            print(f"  {body}", file=sys.stderr)


def github_api_request(url: str, token: str) -> object:
    """Make an authenticated GET request to the GitHub API and return the parsed JSON."""
    req = urllib.request.Request(url)
    req.add_header("Authorization", f"Bearer {token}")
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("X-GitHub-Api-Version", "2022-11-28")

    try:
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read())
    except urllib.error.HTTPError as exc:
        _report_github_http_error(exc, url)
        sys.exit(1)
    except urllib.error.URLError as exc:
        print(f"Error: could not reach GitHub API: {exc.reason}", file=sys.stderr)
        sys.exit(1)


def download_file(url: str, dest: Path, token: str) -> None:
    """Download a file from *url* and save it to *dest*.

    GitHub artifact downloads return a 302 redirect to Azure blob storage.
    We handle the redirect manually so the Authorization header is never
    forwarded to the external host.
    """

    class _NoRedirectHandler(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, req, fp, code, msg, headers, newurl):
            return None  # suppress automatic redirect following

    opener = urllib.request.build_opener(_NoRedirectHandler)

    req = urllib.request.Request(url)
    req.add_header("Authorization", f"Bearer {token}")
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("X-GitHub-Api-Version", "2022-11-28")

    try:
        resp = opener.open(req)
        dest.write_bytes(resp.read())
    except urllib.error.HTTPError as exc:
        if exc.code in (301, 302, 303, 307):
            redirect_url = exc.headers.get("Location")
            if not redirect_url:
                print(
                    "Error: artifact download returned a redirect without a Location header.",
                    file=sys.stderr,
                )
                sys.exit(1)
            # Second request — no Authorization header, the URL is pre-signed.
            try:
                with urllib.request.urlopen(redirect_url) as resp2:
                    dest.write_bytes(resp2.read())
            except urllib.error.HTTPError as exc2:
                _report_github_http_error(exc2, redirect_url)
                sys.exit(1)
            except urllib.error.URLError as exc2:
                print(
                    f"Error: could not download from redirect URL: {exc2.reason}",
                    file=sys.stderr,
                )
                sys.exit(1)
        else:
            _report_github_http_error(exc, url)
            sys.exit(1)
    except urllib.error.URLError as exc:
        print(f"Error: could not download artifact: {exc.reason}", file=sys.stderr)
        sys.exit(1)


def resolve_coverage_from_pr(
    pr_number: int,
    repo: str,
    workflow_name: str,
) -> Path:
    """Fetch the coverage artifact from a GitHub Actions PR run.

    Returns the path to the extracted ``coverage_final.info`` file.
    """
    token = get_github_token()

    # ------------------------------------------------------------------
    # 1. Get the PR head SHA
    # ------------------------------------------------------------------
    pr_url = f"https://api.github.com/repos/{repo}/pulls/{pr_number}"
    pr_data = github_api_request(pr_url, token)

    if not isinstance(pr_data, dict):
        print(f"Error: unexpected API response for PR #{pr_number}", file=sys.stderr)
        sys.exit(1)

    head_sha = pr_data.get("head", {}).get("sha")
    if not head_sha:
        print(f"Error: could not determine head SHA for PR #{pr_number}", file=sys.stderr)
        sys.exit(1)

    print(f"PR #{pr_number} head SHA: {head_sha}")

    # ------------------------------------------------------------------
    # 2. Find completed / successful workflow runs for that commit
    # ------------------------------------------------------------------
    runs_url = (
        f"https://api.github.com/repos/{repo}/actions/runs"
        f"?head_sha={head_sha}&status=completed&conclusion=success&per_page=100"
    )
    runs_data = github_api_request(runs_url, token)

    if not isinstance(runs_data, dict) or not runs_data.get("workflow_runs"):
        print(
            f"Error: no completed/successful workflow runs found for PR #{pr_number}",
            file=sys.stderr,
        )
        sys.exit(1)

    # Prefer runs whose workflow name contains the *workflow_name* substring.
    matching = [
        r
        for r in runs_data["workflow_runs"]
        if workflow_name.lower() in r.get("name", "").lower()
    ]
    runs = matching if matching else runs_data["workflow_runs"]

    run = runs[0]  # API returns newest first
    print(f"Using workflow run: {run['name']} (id={run['id']})")

    # ------------------------------------------------------------------
    # 3. Find the "coverage-reports" artifact
    # ------------------------------------------------------------------
    artifacts_url = run["artifacts_url"]
    artifacts_data = github_api_request(artifacts_url, token)

    if not isinstance(artifacts_data, dict) or not artifacts_data.get("artifacts"):
        print(f"Error: no artifacts found in workflow run {run['id']}", file=sys.stderr)
        sys.exit(1)

    coverage_artifact = None
    for artifact in artifacts_data["artifacts"]:
        if artifact.get("name") == "coverage-reports":
            coverage_artifact = artifact
            break

    if not coverage_artifact:
        available = [a.get("name") for a in artifacts_data["artifacts"]]
        print(
            f"Error: 'coverage-reports' artifact not found. "
            f"Available: {available}",
            file=sys.stderr,
        )
        sys.exit(1)

    print(
        f"Downloading artifact: {coverage_artifact['name']} "
        f"(id={coverage_artifact['id']}, size={coverage_artifact['size_in_bytes']} bytes)"
    )

    # ------------------------------------------------------------------
    # 4. Download and extract the artifact zip
    # ------------------------------------------------------------------
    tmp_dir = Path(tempfile.mkdtemp(prefix="yup_coverage_"))
    zip_path = tmp_dir / "coverage-reports.zip"

    download_file(coverage_artifact["archive_download_url"], zip_path, token)

    with zipfile.ZipFile(zip_path, "r") as zf:
        zf.extractall(tmp_dir)

    # ------------------------------------------------------------------
    # 5. Locate coverage_final.info inside the extracted tree
    # ------------------------------------------------------------------
    info_files = list(tmp_dir.rglob("coverage_final.info"))
    if not info_files:
        print(
            "Error: coverage_final.info not found in the extracted artifact.",
            file=sys.stderr,
        )
        sys.exit(1)

    coverage_path = info_files[0]
    print(f"Using coverage file: {coverage_path}")
    return coverage_path


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Print uncovered line numbers from an LCOV .info coverage file."
    )

    # ------------------------------------------------------------------
    # Source selection (one must be provided)
    # ------------------------------------------------------------------
    source_group = parser.add_mutually_exclusive_group()
    source_group.add_argument(
        "coverage_file",
        nargs="?",
        type=Path,
        help="Path to the LCOV .info coverage file.",
    )
    source_group.add_argument(
        "--pr",
        type=int,
        metavar="NUMBER",
        help="PR number to fetch the coverage artifact from GitHub Actions.",
    )

    parser.add_argument(
        "targets",
        nargs="*",
        help="Source file names or path fragments to report. "
        "Uses the default graphics targets when omitted.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Report every source file found in the coverage file.",
    )
    parser.add_argument(
        "--repo",
        default=DEFAULT_REPO,
        metavar="OWNER/REPO",
        help=f"GitHub repository to query when using --pr (default: {DEFAULT_REPO}).",
    )
    parser.add_argument(
        "--workflow",
        default="coverage",
        metavar="NAME",
        help="Substring to match against workflow run names (default: 'coverage').",
    )

    args = parser.parse_args()

    if args.pr is None and args.coverage_file is None:
        parser.error("either a coverage_file path or --pr must be provided")

    if args.coverage_file is not None and not args.coverage_file.is_file():
        parser.error(f"coverage file does not exist: {args.coverage_file}")

    return args


def parse_lcov_records(content: str) -> list[tuple[str, list[tuple[int, int]]]]:
    records = []

    for record in content.split("end_of_record"):
        source_file = None
        coverage_lines = []

        for line in record.splitlines():
            if line.startswith("SF:"):
                source_file = line[3:]
            elif line.startswith("DA:"):
                line_number, count, *_ = line[3:].split(",")
                coverage_lines.append((int(line_number), int(count)))

        if source_file is not None:
            records.append((source_file, coverage_lines))

    return records


def should_report(source_file: str, targets: list[str], report_all: bool) -> bool:
    return report_all or any(target in source_file for target in targets)


def display_name(source_file: str, targets: list[str], report_all: bool) -> str:
    if report_all:
        return source_file

    return next(target for target in targets if target in source_file)


def print_uncovered_lines(coverage_file: Path, targets: list[str], report_all: bool) -> None:
    content = coverage_file.read_text(encoding="utf-8")

    for source_file, coverage_lines in parse_lcov_records(content):
        if not should_report(source_file, targets, report_all):
            continue

        hit = [line_number for line_number, count in coverage_lines if count > 0]
        miss = [line_number for line_number, count in coverage_lines if count == 0]

        print(
            f"{display_name(source_file, targets, report_all)}: "
            f"total lines={len(coverage_lines)}, hit={len(hit)}, miss={len(miss)}"
        )

        if miss:
            print(f"  Missed: {miss}")


def main() -> None:
    args = parse_arguments()
    targets = args.targets if args.targets else DEFAULT_TARGETS

    if args.pr is not None:
        coverage_file = resolve_coverage_from_pr(args.pr, args.repo, args.workflow)
    else:
        coverage_file = args.coverage_file

    print_uncovered_lines(coverage_file, targets, args.all)


if __name__ == "__main__":
    main()
