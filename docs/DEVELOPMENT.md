# V4 Stable maintenance

[简体中文](DEVELOPMENT.zh-CN.md)

V4.0.0 completes the fixed V4 Preview.1–5 → RC.1 → Stable development cycle.

[Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) is the current published Stable,
including the accepted internal storage optimizations.
See [4.1.0 record](V4_1_0.md), [historical Stable record](V4_STABLE.md),
[RC record](V4_RC1.md) and [freeze](V4_FREEZE.md).

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

## Mandatory implementation-documentation gate

**A Full implementation or contract change is not complete until its technical documentation is consistent with the target source SHA. This is a mandatory PR/acceptance gate, including private-only improvements that do not change the public API.** Do not defer documentation to a later PR or silently leave old mechanism descriptions behind.

For **every change to Full production code, public headers, ordering/ownership/error semantics, private algorithms/structures, allocation behavior, wire/restore, or build/distribution behavior**, review **all four** current-source implementation guides in the **same PR**:

| Guide | Review and synchronize |
| --- | --- |
| [Implementation walkthrough](IMPLEMENTATION_WALKTHROUGH.md) | Architecture and step-by-step execution, real call chains, relevant source modules, before/after behavior |
| [Full public API index and contracts](IMPLEMENTATION_API_REFERENCE.md) | All actual symbols, signatures/behavior, return codes, ownership, lifetime, callbacks, mapping to source; explicitly mark unaffected public API |
| [Internal algorithms and data structures](IMPLEMENTATION_INTERNALS.md) | Changed private helpers, invariants, Block/AVL, memory management, commit/OOM and hot paths |
| [Validation and source-evidence map](IMPLEMENTATION_VALIDATION.md) | Actual test functions/assertions, new/removed coverage, commands run, CI links, unverified behavior and resource evidence |

Contributors making a Full implementation or contract change must:

1. Pin the **old Full source SHA**, verify the **target PR HEAD**, and establish the actual change from the exact diff and affected source/tests. Record the evidence examined and any unverified behavior.
2. Inspect **each of the four guides**, revise all affected explanations and examples, and ensure the documented **applicable Full implementation SHA**, release/development status, and independently verified scope are accurate. If a guide's behavior or API mapping is unchanged, record it explicitly in the PR synchronization matrix rather than inventing changes; still ensure its current-version applicability and review status are not stale.
3. Update the [Full consumer handoff and change ledger](FULL_CONSUMER_HANDOFF.md) ([English edition](FULL_CONSUMER_HANDOFF.en.md)) **in the same PR**: old/new exact SHA, modified functions/structures/call paths, real tests, and a **database core / intermediary-adapter impact judgment** (none, retest, adapter changes, possible core changes, or unknown, with reasons). The database owns its storage, transactions, persistent IDs, and adoption decision; Full remains independent.
4. Update other current-facing API, Architecture, Compatibility, Usage, Integration, Development, README, examples, migration or release documentation **only when affected**. Never rewrite historical Stable/Preview/RC benchmarks, frozen fixtures or already-published facts to make a new change look older.
5. Include a **four-row document synchronization matrix** in the PR evidence: guide, reviewed/updated sections or confirmed unchanged parts, affected source paths/functions, applicable SHA, tests and limitations. Recheck final **remote HEAD** after implementation.
6. **FAIL / no merge** if any of the four guides is unreviewed or misleading, the ledger has no matching record, the database/adapter impact is omitted, or code and documentation source SHAs disagree. Repair within the same step/PR before claiming PASS, even if tests and CI otherwise succeed.

A **documentation-only** change without implementation/contract changes does not create a fake new algorithm baseline or require cosmetic rewriting of all four guides; update only documents truly affected while preserving valid source provenance. Normal Full development still requires independently reviewed tests, compatibility and GitHub branch protections; documentation alone is not sufficient for PASS.

**Current Full candidate:** The untagged 4.2.0-rc.1 branch freezes the accepted operational implementation and reconstructs its documentation and verification. Published Stable remains 4.1.0. OrdWeave keeps its official 4.1.0 dependency. See the [candidate evidence](FULL_RC1_EVIDENCE.md).

## Documentation truth model

Do not rewrite historical records to match the present.

Historical files such as:

- V4 Preview reports,
- V4 RC.1 report,
- archived V3 research,
- captured benchmark result directories,
- old CHANGELOG entries

must preserve what was true at that time.

Current-facing README, API, Usage, Architecture, Compatibility, Integration,
Validation and Development documents must distinguish the merged source version
from the latest *published* Stable rather than advertising an unavailable release.

Rule:

> **correct current truth without rewriting historical truth.**

## Evidence priority after V4

Preview.4 is historical V4 development evidence. The [Full 4.1.0 record](V4_1_0.md)
and [final A/B/C research](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md)
are the newer internally validated 100k–10m evidence, including regressions.
The fixed three-step optimization cycle is closed. Future internal research may
address real costs without waiting for external adoption; a new major version
still requires evidence of an incompatible need.

A future V5/V6 should exist only when real correctness, compatibility, user or workload
evidence justifies a major break.

Long-term Full / Mini / Embedded ideas are roadmap concepts, not current implementation scope.
