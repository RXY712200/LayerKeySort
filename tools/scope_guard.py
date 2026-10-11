#!/usr/bin/env python3
"""Fail-closed PR file-ownership check. Execute only from a trusted base checkout.

Only read-only GitHub REST calls. No PR head checkout, dynamic imports,
subprocesses, shell interpolation, elevated secrets or write API requests.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


class ScopeError(ValueError):
    """Reject uncertain or unauthorized change inventory."""


OWNERS = frozenset(("full", "mini", "consumer", "repo"))
STATUSES = frozenset(("added", "removed", "modified", "renamed", "copied", "changed"))
SHA40 = re.compile(r"^[0-9a-f]{40}$")
REPOSITORY = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
MAX_FILES = 3000


def path_owner(path: str, policy: dict) -> str:
    if (not isinstance(path, str) or not path or len(path) > 4096
            or path.startswith("/") or "\\" in path or "\x00" in path
            or any(ord(ch) < 32 or ord(ch) == 127 for ch in path)
            or any(segment in ("", ".", "..") for segment in path.split("/"))):
        raise ScopeError(f"invalid path: {path!r}")
    exact = policy["exact"]
    if path in exact:
        return exact[path]
    matches = [
        rule for rule in policy["prefixes"]
        if path.startswith(rule["path"])
    ]
    if not matches:
        raise ScopeError(f"unknown path (deny by default): {path!r}")
    matches.sort(key=lambda rule: len(rule["path"]), reverse=True)
    return matches[0]["owner"]


def validate_policy(policy: dict) -> None:
    if not isinstance(policy, dict) or policy.get("schema") != 1:
        raise ScopeError("unsupported ownership policy schema")
    exact, prefixes = policy.get("exact"), policy.get("prefixes")
    if not isinstance(exact, dict) or not isinstance(prefixes, list):
        raise ScopeError("malformed ownership policy")
    if not exact or not prefixes:
        raise ScopeError("empty ownership policy")
    for path, owner in exact.items():
        if not isinstance(path, str) or owner not in OWNERS:
            raise ScopeError("invalid exact ownership rule")
        # Validate path spelling without consulting the rules.
        path_owner(path, {"exact": {path: owner}, "prefixes": []})
    seen = set()
    for rule in prefixes:
        if not isinstance(rule, dict):
            raise ScopeError("invalid prefix rule")
        prefix, owner = rule.get("path"), rule.get("owner")
        if (not isinstance(prefix, str) or not prefix.endswith("/")
                or owner not in OWNERS or prefix in seen):
            raise ScopeError("invalid/duplicate prefix rule")
        seen.add(prefix)
        path_owner(prefix + "probe", {"exact": {}, "prefixes": [{"path": prefix, "owner": owner}]})


def changed_inventory(files: list, policy: dict) -> tuple[list[str], set[str]]:
    if not isinstance(files, list) or not files or len(files) > MAX_FILES:
        raise ScopeError("empty or excessive changed-file inventory")
    seen = set()
    inventory = []
    owners = set()
    for f in files:
        if not isinstance(f, dict):
            raise ScopeError("malformed changed-file entry")
        status, path = f.get("status"), f.get("filename")
        if status not in STATUSES:
            raise ScopeError(f"unsupported file change status: {status!r}")
        owner = path_owner(path, policy)
        previous = f.get("previous_filename")
        if status == "renamed" and not previous:
            raise ScopeError("rename missing previous filename")
        if previous is not None and status not in ("renamed", "copied"):
            raise ScopeError("unexpected previous filename")
        if previous is not None:
            old_owner = path_owner(previous, policy)
            owners.add(old_owner)
            key = f"{status}:{previous} -> {path}"
        else:
            key = f"{status}:{path}"
        if path in seen or key in inventory:
            raise ScopeError("duplicate changed-file entry")
        seen.add(path)
        owners.add(owner)
        inventory.append(key)
    return sorted(inventory), owners


def authorize(files: list, policy: dict, exceptions: dict, *,
              pr_number: int, head_sha: str, base_sha: str) -> tuple[str, int]:
    validate_policy(policy)
    if not isinstance(exceptions, dict) or exceptions.get("schema") != 1:
        raise ScopeError("invalid exception policy schema")
    entries = exceptions.get("exceptions")
    if not isinstance(entries, list):
        raise ScopeError("invalid exception list")
    inventory, owners = changed_inventory(files, policy)
    if len(owners) == 1:
        return next(iter(owners)), len(files)

    matching = [
        e for e in entries
        if (isinstance(e, dict) and e.get("pr_number") == pr_number
            and e.get("head_sha") == head_sha
            and e.get("base_sha") == base_sha
            and e.get("files") == inventory)
    ]
    if len(matching) != 1:
        raise ScopeError(
            "cross-product changes require one precommitted trusted-base exception "
            "for this exact PR, head SHA, base SHA and file inventory: "
            + ", ".join(sorted(owners))
        )
    return "cross-product (exact trusted-base exception)", len(files)


def api_json(repository: str, path: str, token: str):
    url = f"https://api.github.com/repos/{repository}{path}"
    request = Request(
        url, headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2022-11-28",
            "User-Agent": "layerkeysort-trusted-scope-guard",
        },
    )
    try:
        with urlopen(request, timeout=25) as response:
            raw = response.read(6_000_001)
        if len(raw) > 6_000_000:
            raise ScopeError("API reply exceeded size limit")
        return json.loads(raw)
    except (HTTPError, URLError, TimeoutError, ValueError) as error:
        raise ScopeError(f"GitHub read-only API request failed: {path}: {error}") from error


def read_pr_inventory(repository: str, pr_number: int, token: str,
                      expected_head: str, expected_base: str,
                      expected_base_ref: str) -> list:
    path = f"/pulls/{pr_number}"
    pr = api_json(repository, path, token)
    if not isinstance(pr, dict):
        raise ScopeError("invalid PR metadata")
    if (pr.get("number") != pr_number or pr.get("state") != "open"
            or pr.get("base", {}).get("repo", {}).get("full_name") != repository
            or pr.get("base", {}).get("ref") != expected_base_ref
            or pr.get("base", {}).get("sha") != expected_base
            or pr.get("head", {}).get("sha") != expected_head):
        raise ScopeError("PR identity, base or head changed; retry on the new SHA")
    expected_count = pr.get("changed_files")
    if (type(expected_count) is not int
            or not 1 <= expected_count <= MAX_FILES):
        raise ScopeError("invalid or unenumerable PR file count")
    files = []
    for page in range(1, (MAX_FILES // 100) + 2):
        data = api_json(repository, f"{path}/files?per_page=100&page={page}", token)
        if not isinstance(data, list) or len(data) > 100:
            raise ScopeError("invalid paginated changed-file response")
        files.extend(data)
        if len(files) > MAX_FILES:
            raise ScopeError("GitHub changed-file enumeration exceeded API limit")
        if len(data) < 100:
            break
    if len(files) != expected_count:
        raise ScopeError(
            f"incomplete PR changed-file enumeration: API={len(files)}, "
            f"metadata={expected_count}"
        )
    current = api_json(repository, path, token)
    if (current.get("head", {}).get("sha") != expected_head
            or current.get("base", {}).get("sha") != expected_base
            or current.get("changed_files") != expected_count):
        raise ScopeError("PR changed during enumeration; refuse stale authorization")
    return files


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true",
                        help="validate ownership map without API requests")
    parser.add_argument("--policy", default="docs/governance/ownership.json")
    parser.add_argument("--exceptions", default="docs/governance/scope-exceptions.json")
    args = parser.parse_args()
    try:
        policy = json.loads(Path(args.policy).read_text(encoding="utf-8"))
        exceptions = json.loads(Path(args.exceptions).read_text(encoding="utf-8"))
        validate_policy(policy)
        if args.self_check:
            if exceptions.get("schema") != 1 or not isinstance(exceptions.get("exceptions"), list):
                raise ScopeError("invalid exception policy schema")
            print("Scope Guard policy self-check: PASS")
            return 0
        repository = os.environ.get("GITHUB_REPOSITORY", "")
        head = os.environ.get("SCOPE_HEAD_SHA", "")
        base = os.environ.get("SCOPE_BASE_SHA", "")
        base_ref = os.environ.get("SCOPE_BASE_REF", "")
        token = os.environ.get("GH_TOKEN", "")
        pr_raw = os.environ.get("SCOPE_PR_NUMBER", "")
        if (not REPOSITORY.fullmatch(repository) or not SHA40.fullmatch(head)
                or not SHA40.fullmatch(base) or base_ref != "main"
                or not pr_raw.isdecimal() or not 1 <= int(pr_raw) < 2**31 or not token):
            raise ScopeError("missing or invalid trusted PR event context")
        n = int(pr_raw)
        files = read_pr_inventory(repository, n, token, head, base, base_ref)
        owner, count = authorize(files, policy, exceptions,
                                 pr_number=n, head_sha=head, base_sha=base)
        message = f"PASS: PR #{n} modifies {count} files in {owner}; head {head}"
        print(message)
        summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if summary:
            with open(summary, "a", encoding="utf-8") as out:
                out.write(f"### Scope Guard PASS\n\n{message}\n")
        return 0
    except (OSError, json.JSONDecodeError, ScopeError, TypeError, KeyError) as error:
        print(f"Scope Guard FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
