#!/usr/bin/env python3
"""Offline, stdlib-only adversarial tests for the trusted Scope Guard."""
from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("lks_scope_guard", ROOT / "tools" / "scope_guard.py")
assert spec and spec.loader
guard = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = guard
spec.loader.exec_module(guard)
POLICY = json.loads((ROOT / "docs/governance/ownership.json").read_text())
EMPTY = {"schema": 1, "exceptions": []}
HEAD = "a" * 40
BASE = "b" * 40


def item(path, status="modified", previous=None):
    f = {"filename": path, "status": status}
    if previous is not None:
        f["previous_filename"] = previous
    return f


class ScopeGuardTest(unittest.TestCase):
    def test_valid_base_policy_and_current_tracked_files(self):
        guard.validate_policy(POLICY)
        paths = subprocess.check_output(["git", "-C", str(ROOT), "ls-files", "-z"]).split(b"\0")
        tracked = [p.decode("utf-8") for p in paths if p]
        self.assertGreater(len(tracked), 250)
        self.assertEqual([], [p for p in tracked if not guard.path_owner(p, POLICY)])

    def test_owner_classification_and_exact_overrides(self):
        samples = {
            "src/order.c": "full",
            "include/layerkeysort.h": "full",
            "tools/verify_v4_api.py": "full",
            "docs/ARCHITECTURE.md": "full",
            "mini/src/lks_mini.c": "mini",
            "tools/mini_release_assets.py": "mini",
            "tools/tests/test_mini_release_assets.py": "mini",
            ".github/workflows/mini-ci.yml": "mini",
            "consumers/layer-order-workbench/src/workbench.c": "consumer",
            ".github/workflows/workbench-ci.yml": "consumer",
            ".github/workflows/workbench-evidence.yml": "consumer",
            "README.md": "repo",
            "docs/index.html": "repo",
            "docs/FULL.md": "repo",
            "docs/PRODUCT_LINES.zh-CN.md": "repo",
            "docs/governance/ownership.json": "repo",
            "tools/scope_guard.py": "repo",
            ".github/workflows/scope-guard.yml": "repo",
        }
        for path, expected in samples.items():
            with self.subTest(path=path):
                self.assertEqual(expected, guard.path_owner(path, POLICY))

    def test_reject_unknown_and_invalid_paths(self):
        for bad in (
            "future/unknown.c", "Mini/src/lks_mini.c", "src//order.c", "",
            "/src/order.c", "src/../mini/header.h", "docs/./USAGE.md",
            "src\\order.c", "src/bad\x00name", "src/\nfail", "src/\x7f",
            "../include/layerkeysort.h", "mini/",
        ):
            with self.subTest(path=repr(bad)):
                with self.assertRaises(guard.ScopeError):
                    guard.path_owner(bad, POLICY)

    def test_single_class_accepted_without_pr_label(self):
        cases = [
            ("full", [item("src/order.c"), item("include/layerkeysort.h")]),
            ("mini", [item("mini/src/lks_mini.c"), item("tools/mini_release_assets.py")]),
            ("consumer", [item("consumers/layer-order-workbench/src/main.c"),
                          item(".github/workflows/workbench-ci.yml")]),
            ("repo", [item("README.md"), item("docs/governance/OWNERSHIP.md"),
                      item("tools/scope_guard.py")]),
        ]
        for owner, files in cases:
            result, n = guard.authorize(files, POLICY, EMPTY,
                                        pr_number=12, head_sha=HEAD, base_sha=BASE)
            self.assertEqual((owner, len(files)), (result, n))

    def test_cross_owner_rejected_even_when_claimed_repo(self):
        # Labels/PR text are deliberately absent from the authorization API.
        with self.assertRaisesRegex(guard.ScopeError, "cross-product"):
            guard.authorize([item("src/order.c"), item("README.md")], POLICY, EMPTY,
                            pr_number=12, head_sha=HEAD, base_sha=BASE)

    def test_exact_preapproved_exception(self):
        files = [item("src/order.c"), item("mini/src/lks_mini.c")]
        inventory, _ = guard.changed_inventory(files, POLICY)
        auth = {"schema": 1, "exceptions": [{
            "pr_number": 12, "head_sha": HEAD, "base_sha": BASE, "files": inventory
        }]}
        status, count = guard.authorize(files, POLICY, auth,
                                        pr_number=12, head_sha=HEAD, base_sha=BASE)
        self.assertIn("exact trusted-base", status)
        self.assertEqual(2, count)
        for kw in (
            {"pr_number": 13, "head_sha": HEAD, "base_sha": BASE},
            {"pr_number": 12, "head_sha": "c" * 40, "base_sha": BASE},
            {"pr_number": 12, "head_sha": HEAD, "base_sha": "c" * 40},
        ):
            with self.assertRaises(guard.ScopeError):
                guard.authorize(files, POLICY, auth, **kw)
        with self.assertRaises(guard.ScopeError):
            guard.authorize(files + [item("README.md")], POLICY, auth,
                            pr_number=12, head_sha=HEAD, base_sha=BASE)

    def test_rename_old_and_new_both_checked(self):
        allowed = [item("mini/foo.c", "renamed", "mini/bar.c")]
        result, _ = guard.authorize(allowed, POLICY, EMPTY,
                                    pr_number=12, head_sha=HEAD, base_sha=BASE)
        self.assertEqual("mini", result)
        with self.assertRaisesRegex(guard.ScopeError, "cross-product"):
            guard.authorize([item("src/order.c", "renamed", "mini/src/lks_mini.c")],
                            POLICY, EMPTY, pr_number=12,
                            head_sha=HEAD, base_sha=BASE)
        with self.assertRaises(guard.ScopeError):
            guard.authorize([item("mini/x.c", "renamed", "future/x.c")],
                            POLICY, EMPTY, pr_number=12,
                            head_sha=HEAD, base_sha=BASE)

    def test_reject_missing_duplicate_and_suspicious_status(self):
        for files in (
            [], [item("src/foo.c", "surprise")],
            [item("src/foo.c"), item("src/foo.c")],
            [item("src/foo.c", "renamed")],
            [item("src/foo.c", "modified", "mini/foo.c")],
            [item("src/foo.c", "removed", "mini/foo.c")],
        ):
            with self.subTest(files=files):
                with self.assertRaises(guard.ScopeError):
                    guard.authorize(files, POLICY, EMPTY,
                                    pr_number=12, head_sha=HEAD, base_sha=BASE)

    def test_forged_policy_and_exceptions_rejected(self):
        for p in ({"schema": 1, "exact": {}, "prefixes": []},
                  {"schema": 2, "exact": {}, "prefixes": []}):
            with self.assertRaises(guard.ScopeError):
                guard.validate_policy(p)
        with self.assertRaises(guard.ScopeError):
            guard.authorize([item("README.md")], POLICY, {"schema": 1, "exceptions": None},
                            pr_number=12, head_sha=HEAD, base_sha=BASE)

    def test_api_pagination_and_stale_pr_prevention(self):
        metadata = {
            "number": 12, "state": "open", "changed_files": 101,
            "head": {"sha": HEAD},
            "base": {"sha": BASE, "ref": "main",
                     "repo": {"full_name": "RXY712200/LayerKeySort"}},
        }
        filenames = [item(f"mini/test/file{i}.c") for i in range(101)]

        def response(repo, path, token):
            if path.endswith("/files?per_page=100&page=1"):
                return filenames[:100]
            if path.endswith("/files?per_page=100&page=2"):
                return filenames[100:]
            return metadata

        with mock.patch.object(guard, "api_json", side_effect=response):
            self.assertEqual(101, len(guard.read_pr_inventory(
                "RXY712200/LayerKeySort", 12, "dummy", HEAD, BASE, "main")))

        def truncated(repo, path, token):
            if "/files?" in path:
                return filenames[:99]
            return metadata

        with mock.patch.object(guard, "api_json", side_effect=truncated):
            with self.assertRaisesRegex(guard.ScopeError, "incomplete"):
                guard.read_pr_inventory(
                    "RXY712200/LayerKeySort", 12, "dummy", HEAD, BASE, "main")

        def stale(repo, path, token):
            if "/files?" in path:
                return filenames[:100] if path.endswith("page=1") else filenames[100:]
            return {**metadata, "head": {"sha": "c" * 40}}

        with mock.patch.object(guard, "api_json", side_effect=stale):
            with self.assertRaises(guard.ScopeError):
                guard.read_pr_inventory(
                    "RXY712200/LayerKeySort", 12, "dummy", HEAD, BASE, "main")


if __name__ == "__main__":
    unittest.main(verbosity=2)
