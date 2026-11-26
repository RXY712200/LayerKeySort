# Full consumer handoff and change ledger

[简体中文](FULL_CONSUMER_HANDOFF.zh-CN.md) · [Original bilingual-source record](FULL_CONSUMER_HANDOFF.md)

## Full 4.2.0-rc.1 candidate

- From Preview.3 `168debfbd5dd82ef005a4af2fc6a9c06fc9abdd0` to verification baseline `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`. Operational code remains `fe00536e7bd7f4e5128432ea4a29e54158074903`. Only the two public version strings changed. See the [reconstruction ledger](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md).
- The live occurrence, borrowed payload/context, callback/reentry, revision, failure publication, LS1, LKS4SNP1, LK1 import and restore contracts are unchanged. Source, install and amalgamation distribution continue to use the public header.
- The existing native dependency substitution evidence still applies to the same operational code. No OrdWeave adapter or database core change is indicated by the metadata update. Its official dependency remains Full v4.1.0. Before an approved upgrade, rerun exact-pin compilation, logical order, capture/reopen/recovery and the consumer allocation or transaction failure checks.

- Documentation packaging at `a13bab620ea289a64188c58af1b916f11cfb7d2b` adds the complete Chinese instructions to generated `README.txt`. Generated C/header, five-member layout, API and wire bytes are unchanged. Source/install/amalgamation C/C++ consumers and reproducibility/checksum checks passed. Package bytes have a new digest; see the reconstruction ledger. No adapter or database-core change is required.


## Preview.3 consolidated verification — development candidate

- **Baseline and current:** Preview.2 checkpoint `01bc01e2d9ddbda448d9ef0807150f419203b580`, unchanged operational source `fe00536e7bd7f4e5128432ea4a29e54158074903`. The executable verification delta is in `tests/v4_snapshot.c`. [PR #32](https://github.com/RXY712200/LayerKeySort/pull/32), [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3), [plan #27](https://github.com/RXY712200/LayerKeySort/issues/27).
- **Contract:** 64 functions/14 types. Items and comparator context are borrowed. A live Handle survives moves, and removal/destruction ends its lifetime. Preserve the existing synchronization and callback reentry requirements. No-ops preserve revision. Snapshot currentness tracks provenance/revision, and its independent bytes survive source destruction.
- **Representation and distribution:** Same LS1 namespace/ordinal, LKS4SNP1 framing and LK1 import. New tests exercise offset-one wire buffers and restored-tag precedence. Restore resolves in order and allocates fresh Handles. Failure publishes no partial object. Application callback effects and durable transactions remain consumer responsibilities. C17 source/install/amalgamation and C++ consumption stay compatible.
- **Adapter verdict:** Dependency-only native substitution on OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`; official pin remains `70d7f0ef5857b54381b863d292a213909d87459f`. All 27 generated trace conditions in both modes were covered, with checkpoint/history/reopen and the preserved 4,096-occurrence holdout. [Pinned native archive](https://github.com/RXY712200/OrdWeave/tree/ebfa5c62245f793e90fde590b577003839d3e0b1/benchmarks/research/full-preview3-20261125).
- **Database verdict:** Final/Preview.1.2 application operation latency ratios were approximately 1.002 memory and 0.997 persistent (cell ratio ranges 0.780–1.272 and 0.914–1.108). Current persistent checkpoint and reopen medians were 1.008 and 0.993, with upper cell ratios of 1.098 and 1.171. Staging/publication accounts for about 94.5% in memory. WAL accounts for 96.6% in persistence, including the nested 94.4% flush share. Global index rebuilding, semantic replay, checkpoint buffering and I/O remain application costs. The largest prior adverse checkpoint ratio was 1.102 with overlapping ranges.
- **Before an approved upgrade:** Retest the exact dependency with native application tests, both storage modes, recovery and publication failures, independent order/association checks, and consumer-specific retention and payload limits. [Stage evidence](V4_2_0_PREVIEW3.md) and [selection guide](FULL_SELECTION_GUIDE.md).

## Full Preview.1.2 → Preview.2 — 2026-11-24 — development candidate

- **From / To:** `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` (with separate diagnostics `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`) → `fe00536e7bd7f4e5128432ea4a29e54158074903`. See [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3) and [stage report and PR](V4_2_0_PREVIEW2.md). Published Stable remains v4.1.0.
- **Source:** `src/snapshot_wire.c::snap_be_read/snap_be_write` switched from generic byte loops to fixed-width unsigned assembly and individual byte stores. `lks_snapshot_deserialize` still validates the entire input before allocating and copying. `tests/v4_snapshot.c` asserts independence for snapshots with different contents at one revision.
- **Contracts:** 64 functions and 14 types are unchanged. Caller payloads, Handles, borrowed lifetimes, external synchronization, callbacks/reentry, revision, OOM, LS1, LKS4SNP1, LK1 import and restore sequencing remain compatible. C17/C++ source, installation and amalgamation remain unchanged. B64, AVL and tag fallback are retained.
- **Measurements:** Decode improvement is concentrated in row-length processing. Wider associations remain dominated by byte copying. See the stage report for alternatives, memory, end-to-end scope and adverse samples.
- **Database core and adapter:** Neither needs a change for this substitution. Isolated tests used OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`, whose official pin remains `70d7f0ef5857b54381b863d292a213909d87459f`. Rerun real application recovery before upgrading. Global index building and WAL remain database responsibilities. The report separates measured compatibility from performance attribution.

## Preview.1.2 ceiling study — 2026-11-24 — diagnostic only

- **From / To:** accepted `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` → diagnostic `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`; [engineering Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3), [study and verification](V4_2_0_PREVIEW1_2_1.md), [draft PR #30](https://github.com/RXY712200/LayerKeySort/pull/30).
- **Source:** `src/order.c::v4_check_node` verifies strict increase of nonzero tags, retaining the last value across fallback blocks in diagnostic builds. `tests/v4_mutation.c` covers equal and descending labels across an untagged block and restores them before continuing.
- **Core contract:** B64, tagged comparison, bounded repair and AVL fallback stay in Preview.1.2. Public API/errors, Handle identity/lifetime, ownership/callbacks, OOM, revision, synchronization, LS1, LKS4SNP1 and LK1 import remain the same. Restore and C17/C++ source/install/amalgamation interfaces stay compatible. There is no new version.
- **Results:** The two-host mutation and capacity screens did not support production adoption. The stage report preserves B32's Linux speed benefit, added requested memory and bulk-built label pressure, plus the other adverse cells.
- **Adapter verdict and actions:** Isolated Windows OrdWeave at `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` passed 12/12 with this diagnostic source checkout. Its official pin stays v4.1.0. No migration, dependency or adapter change is implied. See the stage report and its draft PR for exact core CI and independent-review status.

## Full Preview.1 → Preview.1.2 — 2026-11-24 — development candidate

- **From / To:** `22544cf283694dd27cef203caba9cf1838845947` → `61e9290ec5ba47b9092bcb8ae7d61f13104ea839`; [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3), [scope #27](https://github.com/RXY712200/LayerKeySort/issues/27), and [extension report and exact-head evidence](V4_2_0_PREVIEW1_2.md). Stable remains v4.1.0.
- **Source:** `src/order.c` compares two tagged blocks directly and uses the existing height-guided AVL walk when a tag is absent. `v4_tag_attach` attaches or redistributes tags for at most 64 blocks. `src/lks_order_internal.h` stores the private tag and diagnostic fields. `src/order_bulk.c` seeds tags after bulk restore. Versions are in `include/layerkeysort.h`.
- **Contract:** The 64 functions, 14 types, order, statuses, ownership, surviving Handle identity, callbacks, synchronization, OOM atomicity, revision/no-op rules, LS1, LKS4SNP1 and LK1 import are unchanged. Restore still creates fresh handles. Public source/install and amalgamation integrations remain the consumer paths.
- **Performance and resources:** A block carries one more 64-bit word. Concentrated attachment may trigger bounded relabeling or AVL fallback. See the stage report for both platform comparisons, adverse cells and scope of memory results.
- **Core and adapter:** Windows strict GCC Debug/Release passed 39/39 each. Isolated Windows OrdWeave `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` passed 12/12 with the exact candidate substituted. Its official v4.1.0 pin `70d7f0ef5857b54381b863d292a213909d87459f` remains unchanged. External independent approval is pending.
- **Consumer action:** Rebuild against the chosen revision and rerun application-specific ordering, persistence and failure checks before switching the official pin. No source conversion or key migration is required by this internal representation change.

## Preview.1 implementation ledger

- **From:** Full v4.1.0 `70d7f0ef5857b54381b863d292a213909d87459f`; development base `c108acd72b37614f4be271c9747484c2dde5eac4` has matching production source.
- **To:** provisional Full 4.2.0-preview.1 implementation `22544cf283694dd27cef203caba9cf1838845947`. The review head also carries tests, documentation and evidence.
- **Source and call paths:** in `src/order.c`, cross-block `lks_order_compare` formerly called the absolute block rank helper twice. It now follows height and parent links and decides precedence from sibling or ancestor relationships. `v4_place`, `lks_order_remove` and cross-block `v4_move` use suffix-only local updates when membership is preserved. Splits, merges and redistributions still use full location repair. The AVL and resident layouts are unchanged.
- **Contracts:** frozen API signatures, borrowed items, surviving handle addresses, callback reentry and output initialization, no-op and revision rules, allocation before mutation and OOM behavior remain applicable. LS1, LKS4SNP1 and V3 LK1 formats and semantics are unchanged. Candidate CMake and header versions differ from Stable. Consumers should rebuild against the selected dependency.
- **Performance and resources:** the production candidate reduced Windows median C-matrix cell time by 28.0% overall and 45.7% on compare-heavy traces relative to v4.1.0. Seven supplemental profiles had matching requested-memory peaks and fewer location assignments. Managed-churn median p99 increased by one 0.1 μs timer step. See [candidate evidence](V4_2_0_PREVIEW1.md) for measured scope and cross-host results.
- **Database adapter:** no API or ownership migration is indicated. Rebuild and verify ID-to-handle mapping, duplicate records, explicit order and managed indexes before any dependency change.
- **Database core:** Full does not alter database revisions, WAL policy or native checkpoint format. Isolated Windows GCC Release OrdWeave at `b12a007c50c2f9efcbee500a14d3cf09dcc74f10` substituted the candidate implementation and passed 12/12 CTests including persistence, checkpoint, history and adversarial recovery. Its official pin stays at Full v4.1.0. A future upgrade needs its own review and application-specific performance testing.
- **Verification:** Windows Release passed 39/39 with the focused precedence and location regression. The stage report and draft PR identify the exact platform/CI and analytical evidence revisions. The bounded OrdWeave run does not measure database component costs or certify power-loss behavior.

> **Ownership: LayerKeySort Full. Established: 2026-11-15. Maintainer: Full.**
>
> This is the durable handoff for database and other external consumers. Record historical decisions, exact changes, verification and upgrade actions in traceable GitHub documents. The ledger does not design a database or authorize API or release changes.
>
> Initial Full source was main at `70d7f0ef5857b54381b863d292a213909d87459f`, published as [Full v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0). Establishing the ledger did not upgrade production.

## 1. Product boundary

- Full is an independent C17 dynamic-order library. Compile and link against its public header, or use the verified release amalgamation `layerkeysort.h` + `layerkeysort.c`. No database installation is required.
- A database is a separate GitHub project with its own core and adapter: database core → database-maintained adapter → public Full API. Full maintains its library and this ledger.
- Durable record IDs, queries, indexes, storage, transactions, services and ID-to-Handle mapping belong to the database.
- Proposals for new Full APIs need a use case, a concrete existing-interface limitation and reproducible evidence. Full evaluates general ordering value and can keep database-specific behavior in the adapter.
- Mini has separate versions, API and release contracts. Its adoption is a separate decision. This ledger covers Full.
- OrdWeave implements collection ordering, native snapshots, checkpoints and WAL recovery through public Full interfaces. Pinned application source is `b12a007c50c2f9efcbee500a14d3cf09dcc74f10`; its official Full v4.1.0 pin is `70d7f0ef5857b54381b863d292a213909d87459f`. Candidate testing uses isolated copies.
- **Historical priority, 2026-11-15:** The owner paused proactive Full memory and algorithm optimization while the independent database and adapter established concrete uses and workloads. The Preview.1 authorization superseded that pause. Confirmed correctness or compatibility fixes remained possible under authorization. Future optimization requires evidence and fresh authorization.

## 2. Required guide synchronization

This is a mandatory acceptance gate for Full implementation changes. [PR #25](https://github.com/RXY712200/LayerKeySort/pull/25) integrated the initial four Chinese guides and [PR #26](https://github.com/RXY712200/LayerKeySort/pull/26) integrated their bilingual README entry points. Their operational baseline was Full v4.1.0, `70d7f0ef5857b54381b863d292a213909d87459f`. Later documentation-only commits did not change that algorithm.

| Guide | Obligations |
| --- | --- |
| [Walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md) | Actual call paths and modules, mutation, history and restoration, ownership, complexity, boundaries and workflows |
| [API index and contracts](IMPLEMENTATION_API_REFERENCE.zh-CN.md) | Added or removed functions and types, signatures, parameters and errors, lifetime, callbacks, synchronization and source mapping. The frozen 4.x API requires separate authorization to change. |
| [Internal algorithms and structures](IMPLEMENTATION_INTERNALS.zh-CN.md) | Affected modules and helpers, index invariants, blocks and AVL, allocation, failure, encoding, batch operations and legacy compatibility |
| [Validation and evidence](IMPLEMENTATION_VALIDATION.zh-CN.md) | Exact tests and assertions, commands, CI and platforms, benchmarks, distribution, gaps and work actually executed |

When changing production sources, public headers, structures, algorithms, allocation, errors, snapshot or wire behavior, builds, distribution or the effective contract, complete these actions in the same PR:

1. Identify the full previous SHA, target PR and latest HEAD, and production inventory. Determine actual differences from code and tests. Record coverage and omissions.
2. Review each guide. Align affected flows, figures, interface tables, source positions, test references, resource and performance notes, and boundaries. Mark verified unchanged sections explicitly in the PR matrix and reconcile source pins and verification status.
3. Update the ledger and affected API, architecture, development, usage, integration, compatibility, migration, examples, README and Release Notes. Choose files by impact.
4. Assess database core and adapter separately. Trace API, semantics, Handle and occurrence identity, Resolver, persistence associations, threading, OOM, performance and build changes. Mark no modification, retest, adapter modification, possible core modification or insufficient evidence. Supply the exact before/after evidence, reproduction and actions. Database maintainers decide adoption.
5. Attach an auditable four-guide matrix with sections, functions and files, previous and new SHAs, test evidence and gaps. Recheck the exact complete remote diff after new commits.
6. Missing guide review, missing ledger, missing database verdict, mismatched source SHA or conflicting documentation is a failed acceptance gate. Repair in the current PR before merging, declaring acceptance, tagging Stable or releasing.
7. Private optimizations with unchanged API still require internal, memory, failure, performance and test updates and all four guide checks. State why the adapter can stay unchanged or needs retesting. A new version number is irrelevant to this obligation.

Current guides must label the exact source SHA, independently reviewed scope and whether the source is unpublished main, Stable or RC. A newer documentation SHA does not imply a new algorithm version. Preserve historical facts, frozen formats and old test and benchmark records. Provide a historical entry point when necessary. For documentation-only changes, update the affected material, check links and current technical accuracy, and preserve source provenance. A version with unfinished documentation is unfinished.

### Database and adapter handoff

A database may own files, a transaction log, indexes, durable identifiers and its adapter. It need not use a Full storage implementation. Pin the dependency by version and full SHA. At upgrade, read this ledger, the four current guides and source diff, then inspect and test affected implementation. The guides support source review. Each repository reviews and delivers its own changes, and no cross-repository synchronization is automatic.

## 3. Ongoing handoff workflow

### Initial integration

Review the actual public header, production code, build, tests and contract at the selected version or full 40-character SHA. The ledger narrows the search.

### Each relevant Full update

Record relevant changes, including internal-only changes, as they enter main. Every release needs its own accurate upgrade handoff.

1. Record the old and new full SHAs, version and branch, PR, commits and issues. A single complete entry may cover several commits in one PR.
2. Name actual changed files, functions, structures, call chains and before/after behavior. Generic improvement labels are insufficient.
3. Assess API, parameters and status, ownership and Handle lifetime, ordering, comparison and callbacks, reentry, OOM failure atomicity, concurrency, snapshots, LS1, LKS4SNP1, legacy migration, builds and amalgamation.
4. Even with the same API, describe internal, memory, performance and failure changes. Cite measurements or regressions and mark unknowns. A single metric is insufficient for a general speedup.
5. Give a separate database adapter decision and reason: no changes, targeted retest, adapter changes, possible database core changes or insufficient evidence. Record other consumers' checks.
6. Link tests, CI, reproduction and research to their SHAs. Distinguish inspected evidence from external reports; unverified reports do not establish compatibility or acceptance.
7. Update this ledger and the mandatory four-guide matrix in the same PR. Update affected contracts, README, integration, migration and Release Notes based on actual diffs and evidence.

Private algorithm and performance changes require a description of the exact change, why the adapter can stay as is, and the workloads to retest. Mini-only changes need no Full upgrade entry unless shared build or delivery changes. A missing record, mismatched SHA or insufficient evidence leaves upgrade impact unverified. Request the missing evidence before upgrading.

### Entry template

```md
### Full <old version/ref> → <new version/ref> — <date> — <Stable/RC/Preview/development>
- Previous full SHA:
- New full SHA:
- PR / commits / issues / compare:
- Changed files (production / API / tests / tools / docs):
- Changes (functions, structures and call paths; before → after):
- API, parameters, status and ordering: changed / explicitly unchanged
- Ownership, Handles, callbacks, OOM and concurrency: changed / explicitly unchanged
- Snapshot, LS1, wire and restore: changed / explicitly unchanged
- Build, install and amalgamation: changed / explicitly unchanged
- Performance, memory and regressions (workload, data and report):
- DB adapter: no change / targeted retest / adapter change / possible core change / insufficient evidence; reason:
- Actions for other Full consumers:
- Four-guide matrix (changed and verified unchanged sections and source SHA):
- Actual database dependency/adapter tests, or explicit unverified status:
- Tests, CI, benchmark and exact test SHA:
- Review and open verification:
```

Put newest entries first. Preserve prior versions, test data, reports and published tags. Multiple code changes in one PR may share a complete entry. A new independent Full change or upgrade gets a new entry.

## 4. Full v4.1.0 baseline and database boundary

Inspect the actual source for newer versions.

| Subject | Baseline behavior | Source or contract |
| --- | --- | --- |
| API | Frozen V4 64 functions and 14 public types, C17, general-purpose library | [Public header](../include/layerkeysort.h), [API](API.md), [freeze](V4_FREEZE.md) |
| Explicit live order | Relative and endpoint insert, remove, move, traversal and order comparison. Own removal or container destruction invalidates a Handle. | [order.c](../src/order.c), [internal header](../src/lks_order_internal.h) |
| Managed order | Comparator governs ordering and locate. Update compared fields only through proper ordering changes. | [managed_order.c](../src/managed_order.c) |
| Durable IDs | Full does not assign them. Live Handles are neither durable IDs nor string sort keys. | [Public header](../include/layerkeysort.h) |
| Snapshot | Captures order and callback-supplied bytes. LS1 identifies namespace and historical row ordinal, with no cross-time durable record identity. | [snapshot.c](../src/snapshot.c), [snapshot_key.c](../src/snapshot_key.c) |
| Wire and recovery | LKS4SNP1 restores via an application Resolver and creates fresh Handles. Full does not implement database transactions. | [wire](../src/snapshot_wire.c), [restore](../src/snapshot_restore.c), [bulk](../src/order_bulk.c) |
| V3 Path/LK1 | Private codecs support migration. V4 live ordering does not maintain V3 Paths. | [migration](MIGRATION_V3_V4.md), [source list](../cmake/ProductionSources.cmake) |
| Absent facilities | Durable business keys, general field index or SQL, transactional disk storage, services, collaborative ordering, persistent live Handles and public numerical rank or nth-item indexing | [header](../include/layerkeysort.h), [API](API.md) |
| Integration | CMake or direct sources, or official two-file amalgamation. No universal dynamic binary ABI. | [Integration](INTEGRATION.md), [中文](INTEGRATION.zh-CN.md) |

A durable database record ID, Full Live Handle, Full LS1 historical key and database revision or transaction ID are separate roles. Any demand for changing durable sort keys, numerical ranks or new queries needs application evidence.

## 5. Published change baselines

### Ledger establishment — 2026-11-15 — documentation only

- Full source `70d7f0ef5857b54381b863d292a213909d87459f`; Stable `v4.1.0`.
- Added the handoff and ledger. Full and Mini production code, headers, LS1 and wire formats and existing releases were unchanged.
- Initial adoption requires choosing a dependency SHA and auditing the used source. Later upgrades can use this ledger and the Git diff.
- Checked the documentation against the reviewed source baseline. This historical entry did not add builds, CI or benchmarks.

### Full v4.0.0 Stable → v4.1.0 Stable — historical record

- From `v4.0.0` (`80004ae732daf696f7e9192e417de0edb0b2aafa`) to `v4.1.0` (`70d7f0ef5857b54381b863d292a213909d87459f`).
- [Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0), [implementation PR #19](https://github.com/RXY712200/LayerKeySort/pull/19), [changes and measurements](V4_1_0.md), [original report](https://github.com/RXY712200/LayerKeySort/blob/6325a6b16d2fe049b7bbc1227c017d6bfb5593b8/benchmarks/research/final-step3/REPORT.md).
- `src/lks_order_internal.h`, `src/order.c` and `src/order_bulk.c` moved per-Resident owner metadata to the block; separate Handle allocation, stable addresses, B64 and the block index remained. `src/lks_snapshot_internal.h`, `src/snapshot.c`, `src/snapshot_restore.c` and `src/snapshot_wire.c` adopted cumulative association offsets, coallocated header/rows/namespace, trimmed association capacity before publication and adjusted restore and wire access. `tools/validate_distribution.py` corrected Windows implicit single-configuration Debug distribution validation.
- The 64 functions, 14 types, Handle lifetime, OOM atomicity, LS1, LKS4SNP1, V3 LK1 import and callbacks stayed compatible. No database ID, dynamic durable key or general query API was added.
- Measured Full x64 requested bytes: one million appends 50,249,480 → 42,499,472 (about 15.4% less); one million with 16 retained snapshots peaked at 513,593,776 → 355,751,040 (about 30.7% less). RSS is separate. Linux random lookup, wider-namespace key formatting, serialization and churn had some regressions.
- An adapter using frozen V4 did not need renames or key conversions. Rebuild, run real workloads and verify snapshots and recovery. This historical entry did not establish real database integration compatibility.
- Exact tests and CI are in the linked release documents. The historical entry summarizes earlier evidence without a new execution.

## 6. External consumer upgrade steps

1. Identify the currently pinned Full version and full SHA and the target version and full SHA. Full uses `v*`, Mini uses `mini-v*`.
2. Read all relevant ledger entries; ask Full for missing interval details.
3. Compare actual source with `git diff --stat <old-sha> <new-sha>` and `git diff <old-sha> <new-sha> -- include/ src/ cmake/ tools/ tests/`. Follow the affected code and call paths the consumer uses. Do not omit source review on initial adoption.
4. Test database ID/Handle mapping, lifetime, insertion, movement, comparison, ManagedOrder, capture/association/restore, errors and OOM, build/link and the workload relevant to the application.
5. The database owns the decision to upgrade, change the adapter or core, and release. A Full update does not accept those changes on its behalf. Each update needs a focused review of affected source.
6. Report generic API gaps or defects with reproducible examples and use cases for independent Full evaluation.

## 7. Full maintainer acceptance checklist

- [ ] Match live remote HEAD, actual diff, production code and relevant tests. Record checked evidence and gaps. Summaries, issues and external reports are navigation aids.
- [ ] Add the relevant Full entry in the same PR or release preparation. Preserve published history.
- [ ] For any Full implementation or contract change, review all four Chinese implementation guides, synchronize affected sections, pins and evidence and attach the PR matrix. Incomplete review cannot pass.
- [ ] Identify actual function and structure changes, account for the public contract and explain impact on the database adapter and other consumers.
- [ ] Support performance claims with reproducible, fair evidence including regressions and limits.
- [ ] Maintain the public API, compatibility and integration guidance and necessary tests. Observe product separation and protected-branch PR and CI rules.
- [ ] Separate authorization is required for later stages, merge, tags or GitHub Release.

**Evidence priority:** Actual production source at the exact SHA, build and reproducible tests first; then public contracts; then README, development, integration and Issues; then historical handoff and conversation. This ledger directs consumers to evidence. Source verification remains necessary.
