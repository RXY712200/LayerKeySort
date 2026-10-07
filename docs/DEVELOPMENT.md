# V4 Stable maintenance

V4.0.0 completes the fixed V4 Preview.1–5 → RC.1 → Stable development cycle.

[Stable record](V4_STABLE.md), [RC record](V4_RC1.md) and
[freeze](V4_FREEZE.md) preserve the release evidence and compatibility decisions.

The active post-v4 roadmap is
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).
Historical Issue #2 is closed and should remain unchanged as the older roadmap record.

## 4.x maintenance rule

4.x maintenance must preserve the frozen:

- public names/signatures/layouts,
- explicit status numbers,
- ownership/lifetime semantics,
- LS1 v1 bytes and domain semantics,
- LKS4SNP1 bytes and parsing semantics.

Compatible correctness, portability, documentation and internal-performance fixes are
allowed when they preserve these contracts.

An incompatible correction is not an ordinary 4.x patch. It requires an explicitly
justified future major-version decision.

## Private implementation may evolve

The following are implementation details, not frozen compatibility:

- B64/minimum-32 block policy,
- AVL/block topology,
- allocator strategy,
- diagnostic counters,
- internal helper decomposition.

Do not expose a private optimization as public compatibility merely because it exists in
4.0.0.

## Required validation

For code changes run as applicable:

- `tools/verify_v4_api.py`,
- strict C17 build and CTest,
- all seven public examples,
- `tools/validate_distribution.py`,
- `tools/validate_release_assets.py`,
- sanitizer/platform gates appropriate to the change.

Legacy V3 regressions use private declarations only for migration/regression evidence.

## Documentation truth model

Do not rewrite historical records to match the present.

Historical files such as:

- V4 Preview reports,
- V4 RC.1 report,
- archived V3 research,
- captured benchmark result directories,
- old CHANGELOG entries

must preserve what was true at that time.

Current-facing files such as README, API, Usage, Architecture, Compatibility,
Integration, Validation and this Development guide must describe current Stable truth.

Rule:

> **correct current truth without rewriting historical truth.**

## Evidence priority after V4

Preview.4 remains the primary synthetic performance-convergence evidence.

The next high-value evidence is real application integration and implementation-independent
real workload traces. Do not create a new research cycle or major version merely because
more tuning is possible.

A future V5/V6 should exist only when real correctness, compatibility, user or workload
evidence justifies a major break.

Long-term Full / Mini / Embedded ideas are roadmap concepts, not current implementation scope.
