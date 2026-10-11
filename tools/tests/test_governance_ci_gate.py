#!/usr/bin/env python3
"""Verify fail-closed gate decisions, including all GitHub skipped conditions."""
from __future__ import annotations

import importlib.util
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("governance_ci_gate", ROOT / "tools" / "governance_ci_gate.py")
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


class GateTest(unittest.TestCase):
    def test_every_expected_job_must_pass(self):
        sample = {
            "windows-msvc": {"result": "success"},
            "ubuntu": {"result": "success"},
            "macos-appleclang": {"result": "success"},
        }
        self.assertEqual(
            ("macos-appleclang", "ubuntu", "windows-msvc"),
            module.require_success(sample, "windows-msvc ubuntu macos-appleclang"),
        )

    def test_missing_extra_skipped_and_neutral_rejected(self):
        sample = {"mini": {"result": "success"}}
        for bad in (
            {},
            {"mini": {"result": "failure"}},
            {"mini": {"result": "cancelled"}},
            {"mini": {"result": "skipped"}},
            {"mini": {"result": "neutral"}},
            {"mini": {"result": None}},
            {"mini": {}},
            {"mini": {"result": "success"}, "surprise": {"result": "success"}},
            {"mini": None},
            {"mini": "success"},
            None,
            [],
        ):
            with self.subTest(bad=bad):
                with self.assertRaises(module.GateFailure):
                    module.require_success(bad, "mini")
        with self.assertRaises(module.GateFailure):
            module.require_success(sample, "mini consumer")
        with self.assertRaises(module.GateFailure):
            module.require_success(sample, "mini mini")

    def test_unsupported_job_shapes_are_rejected(self):
        with self.assertRaises(module.GateFailure):
            module.require_success({"mini": "success"}, "mini")
        with self.assertRaises(module.GateFailure):
            module.require_success({"mini": {"result": "success"}}, "")
        with self.assertRaises(module.GateFailure):
            module.require_success({"mini": {"result": "success"}}, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
