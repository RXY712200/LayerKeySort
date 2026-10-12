#!/usr/bin/env python3
"""Require every named GitHub Actions upstream job to finish with success."""
from __future__ import annotations

import json
import os
import sys


class GateFailure(ValueError):
    pass


def require_success(needs: object, expected: str) -> tuple[str, ...]:
    if not isinstance(needs, dict) or not isinstance(expected, str):
        raise GateFailure("invalid GitHub needs context or expected job IDs")
    required = expected.split()
    if not required or len(required) != len(set(required)):
        raise GateFailure("missing/duplicate expected upstream job")
    if set(needs) != set(required):
        raise GateFailure(
            f"missing/unexpected upstream job: expected {sorted(required)}, "
            f"got {sorted(needs)}"
        )
    for job_id in required:
        state = needs[job_id]
        if not isinstance(state, dict) or state.get("result") != "success":
            observed = state.get("result") if isinstance(state, dict) else "invalid"
            raise GateFailure(f"required {job_id} did not succeed: {observed}")
    return tuple(sorted(required))


def main() -> int:
    try:
        need_json = os.environ["NEEDS_JSON"]
        expected = os.environ["REQUIRED_JOB_IDS"]
        passed = require_success(json.loads(need_json), expected)
        print("Gate PASS:", ", ".join(passed))
        return 0
    except (KeyError, ValueError, TypeError, json.JSONDecodeError) as error:
        print(f"Gate FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
