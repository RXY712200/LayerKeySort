# Full 4.2.0-rc.1 validation and evidence map

[简体中文](IMPLEMENTATION_VALIDATION.zh-CN.md) · [Internals](IMPLEMENTATION_INTERNALS.md) · [Implementation audit](design/V4_2_0_RC1_DOCUMENTATION_AUDIT.md)

Use RC validation tree `d1132a26ca7b2c65bfd512fa2eb29f1b16793c08`, which contains both Preview.3 regressions. Operational source is `fe00536e7bd7f4e5128432ea4a29e54158074903`. Published Stable is Full 4.1.0 at `70d7f0ef5857b54381b863d292a213909d87459f`. [Preview.3 source and runs](V4_2_0_PREVIEW3.md) · [Full API index](IMPLEMENTATION_API_REFERENCE.zh-CN.md) · [Walkthrough](IMPLEMENTATION_WALKTHROUGH.zh-CN.md)

## 1. Confirm the validation checkout

~~~sh
git clone https://github.com/RXY712200/LayerKeySort.git
cd LayerKeySort
git checkout d1132a26ca7b2c65bfd512fa2eb29f1b16793c08
git rev-parse HEAD
git status --short
~~~

Full Stable, the Full candidate, Mini, Workbench and historical V3 regression targets represent distinct evidence scopes.

## 2. CMake targets and build options

Root CMake includes `cmake/ProductionSources.cmake` for `LKS_SOURCES`. It defines STATIC `layerkeysort` and `LayerKeySort::layerkeysort`, with C17 and compiler C extensions disabled.

| Option | Use |
| --- | --- |
| `LKS_BUILD_TESTS` | ON in the root build and OFF when embedded by default. |
| `LKS_BUILD_EXAMPLES` | Public examples, ON in the root and OFF when embedded. |
| `LKS_BUILD_BENCHMARKS` | Additional benchmark targets and tests, OFF by default. |
| `LKS_ENABLE_SANITIZERS` | ASan/UBSan when supported by GNU/Clang. |
| `LKS_BUILD_CAPACITY_SCREEN` | Private B64/B128/B256 variants, outside the product ABI. |
| `LKS_INSTALL` | Install the static library, public header and CMake package. |

The diagnostic and old benchmark paths use additional instrumentation and legacy regression sources through `layerkeysort_diagnostics` and `layerkeysort_legacy_bench`. The ordinary Full target excludes legacy `gap.c/tree.c/group.c/bulk.c`.

~~~sh
cmake -S . -B build -DLKS_BUILD_TESTS=ON -DLKS_BUILD_EXAMPLES=ON -DLKS_BUILD_BENCHMARKS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug -N
ctest --test-dir build -C Debug --output-on-failure
~~~

The 39-test baseline in [Validation](VALIDATION.md) assumes benchmark and Python options. Use the actual test listing and logs.

## 3. Source, tests and assertions

`tests/v4_snapshot.c` contains both Preview.3 changes. `754b3088` compares offset-one writer output to literal golden bytes and decodes the unaligned buffer. `3ac82d26` temporarily clears two interior restored tags, checks invariants, tagged comparison and both fallback signs, restores the tags, and traverses all items in both directions. The same-revision copy-isolation test is retained.

| Implementation | Test and purpose |
| --- | --- |
| Order index and Handle lifetime | [v4_order.c](../tests/v4_order.c): reference sequence, boundaries, invariants, two-child deletion, shared block owner and insert OOM. |
| Move, cursor and revision | [v4_mutation.c](../tests/v4_mutation.c): randomized/adversarial and distant moves, no-ops, cursor invalidation and cross-block OOM. |
| ManagedOrder | [v4_mutation.c](../tests/v4_mutation.c): duplicates, sorted insertion, neighbors, comparison calls, callback interaction and OOM. |
| Snapshot and restore | [v4_snapshot.c](../tests/v4_snapshot.c): empty rows, history retention, callbacks and associations, Resolver failure, limits and OOM. |
| LS1 and binary wire | [v4_snapshot.c](../tests/v4_snapshot.c), [v4_rc_torture.c](../tests/v4_rc_torture.c): canonical forms, exact vectors and malformed input. |
| Bulk restore | [v4_snapshot.c](../tests/v4_snapshot.c): large and empty restore, new order and cleanup on failure. |
| Group, Batch and sort | [v4_immutable_group.c](../tests/v4_immutable_group.c): stable construction and equality, merging, multiple OOM points and Group snapshots. |
| Atomic markers | [v4_marker_threads.c](../tests/v4_marker_threads.c): marker lifetime under the tested synchronization conditions. |
| Frozen public API | [v4_api_compile.c](../tests/v4_api_compile.c), [v4_freeze.c](../tests/v4_freeze.c): typed reference and link checks for 64 functions and representative semantics. |
| Legacy import and parsing | [v4_snapshot.c](../tests/v4_snapshot.c), [parser_torture.c](../tests/parser_torture.c): V3 Path and LK1 boundaries. |
| Historical V3 ordering | [consumer_roundtrip.c](../tests/consumer_roundtrip.c), v2_main.c and related private targets: legacy behavior, separate from V4 consumer integration. |
| Long workload and tracing | tests/soak.c, benchmarks/v4_trace.c and benchmarks/workload_simulator.c: repeated edits and defined workload families. |

### 3.1 Specific contracts and limits

| Contract | Asserted behavior | Boundary |
| --- | --- | --- |
| Removal ends one Handle lifetime | `remove_model` in v4_order.c verifies returned item and removes that Handle from the model. `random_campaign` periodically calls `verify` to compare surviving addresses, items, traversal both ways and comparison. | No access to freed Handles. Full verification is periodic. |
| No-op, revision and capacity | `cursor_tests` runs endpoint/self/adjacent no-ops under failure injection without allocation or revision changes. A cursor survives a failed insert. At UINT64_MAX, no-op move_front succeeds and actual move_back returns CAPACITY_LIMIT. | These are reached cases, not every capacity/API combination. |
| Cross-block move OOM | `move_oom` fails spare allocation for a full destination, checks original blocks, revision, live bytes/blocks and Handle sequence, and reads the unchanged first item through a cursor. | Also checks nonfull destination and same-block moves without allocation. |
| Multiple occurrences of the same item | `boundaries_and_checker` inserts the same address and removes distinct saved Handles, checking repair and invariants. | Cross-container application recovery when compared fields mutate is separate. |
| Resolver failure midstream | `large_and_edges` and `resolve_status_fail` return NOT_FOUND at first, middle and last rows; output remains NULL and Full live bytes unchanged. `resolve_fail` checks first-row OK with NULL yields INVALID_ARGUMENT. | Immediate source return establishes stop behavior. The test does not separately count every subsequent callback or establish application rollback. |
| Busy getters and destroy | `reentry_compare` in managed_interactions checks REENTRANT, NULL/0 and shared Handle getters, and attempts busy destroy. `associate` in callbacks_lifetime checks getters and nested capture; outer operation succeeds. | Applies to listed calls. |
| Busy Group and Batch | `compare` and `association` under `campaign` check NULL/0, nested capture/merge_all REENTRANT and busy destroy, followed by successful use and cleanup. | This evidence does not establish safe concurrent merge/capture. |
| Empty restore and association storage | `capture_restore` covers count zero. `associate` returns file-scope scratch. | No test supplies an expired callback-local array. Empty restore permitting NULL Resolver is corroborated by header and implementation. |

Initial RC strict GCC Debug and Release each ran 39/39 locally. The [initial Full PR CI](https://github.com/RXY712200/LayerKeySort/actions/runs/45518848531) passed six configurations with 39/39 each and extended sanitizers. Later repair evidence must use its exact HEAD.

### 3.2 Distinguish similarly named tests

The historical `consumer_roundtrip.c` explicitly uses private V3 declarations. The marker-thread test establishes lifetime for specialized synchronized cases, not unrestricted concurrent read/write access to a live Order.

## 4. Frozen API, persistent bytes and failure injection

~~~sh
python tools/verify_v4_api.py
~~~

The verifier uses [v4_public_api.json](../tests/fixtures/v4_public_api.json), including typed consumer references. The frozen surface is 64 functions and 14 types. Changing the manifest requires an authorized contract change.

Fixed-width big-endian Preview.2 source is `fe00536e7bd7f4e5128432ea4a29e54158074903`; existing golden bytes and rejection cases remain. `same_revision_distinct_snapshots` changes caller data and namespace between two captures and verifies independent history after the order and caller data are destroyed. See the [Preview.2 report](V4_2_0_PREVIEW2.md) for measurements, platforms and limits.

`tools/verify_v4_wire.py` exercises the separate `tests/v4_wire_probe.c` executable with [v4_wire_golden.json](../tests/fixtures/v4_wire_golden.json) and V3 LK1 goldens. CTest target `layerkeysort_v4_wire_golden` runs this comparison. Preserve golden constants independently of the tested encoder. [Fixture hashes](../tests/fixtures/v4_fixture_sha256.json) detect unreviewed changes. [Freeze](V4_FREEZE.md) specifies width, endianness and compatibility.

`lks_alloc_test_fail_on_attempt(k)` injects a failure in diagnostic builds. Order, mutation, snapshot and immutable Group tests exercise allocations and check cleanup, promised failure atomicity and NULL output where applicable. Application callback side effects remain the application's responsibility.

## 5. Distribution and external consumers

Supported paths are source integration via add_subdirectory or offline FetchContent, install plus `find_package(LayerKeySort 4 CONFIG REQUIRED)` and link `LayerKeySort::layerkeysort`, and the official two-file amalgamation built as C17 for C or C++ programs.

`tools/amalgamate.py` uses the production source manifest and required private headers, checking agreement with MSVC source lists. Maintain one generated version. `tools/validate_distribution.py` compiles source, installed and two-file C/C++ consumers. `tools/validate_release_assets.py` builds twice, checks byte equality and SHA-256, and exercises corrupted and extra-file rejection.

~~~sh
python tools/validate_distribution.py
python tools/validate_release_assets.py
~~~

The initial RC executed these checks. For its exact options and ZIP digest, see the [candidate record](V4_2_0_RC1.md).

## 6. Performance and memory evidence

Pin source, system, compiler, optimization, count, operation mix, comparator and allocator cost, block policy and any snapshot/retention conditions. Report unhelpful samples too. Final lifecycle analysis separates summed timed phases from process wall; historical inputs without a wall column only reproduce the earlier metric. A nested WAL flush is not an extra duration to add to the parent.

Requested live bytes, peak requested bytes and RSS are distinct. Separate same-block and cross-block moves, long drags and local hotspots. Full capture and wire serialization cost O(N+A), independent of live movement's local structure costs. The historical 4.1.0 study retains both memory benefits and timing regressions. Real traces can help but are not necessary for every technical audit.

## 7. Interpreting failures and limits

For a golden mismatch inspect offsets and field widths before changing fixtures. For restored order compare failures inspect parent links, threaded links and tag monotonicity. For injection failures, confirm the intended point was reached, then check output, previous container state and teardown. Verify platform jobs against exact run and head.

Green tests cannot establish stale-pointer safety, arbitrary user comparator consistency, C++ exceptions escaping C, unsynchronized mutable Order concurrency, hard realtime latency, automatic disk files, WAL, crash recovery or in-place disk page updates without loading RAM. Database storage and transaction tests must demonstrate database recovery semantics.

## 8. Synchronize documentation and tests after changes

| Change | Update |
| --- | --- |
| Block/AVL/Handle/Move | [Internals](IMPLEMENTATION_INTERNALS.md) §§2–8, walkthrough §§2–4 and Order/mutation tests |
| ManagedOrder | Internals §7, API index and managed mutation tests |
| Snapshot/LS1/wire/restore | Internals §§9–11, API/compatibility/migration, goldens and snapshot tests |
| Group/Batch/sort | Internals §12, API index and stable-order/Group tests |
| Legacy LK1 import | Internals §13, V3 fixtures and migration guidance; preserve prior releases |
| Build/install/amalgamation | This guide §§2/5, integration and distribution verification |
| Public API | Review frozen 4.x contract and version before changing the manifest, API docs and consumer tests |
| Full upgrade for external consumers | Record exact SHAs, interface/internal differences and adapter action in the [handoff ledger](FULL_CONSUMER_HANDOFF.en.md) |

[PR #24](https://github.com/RXY712200/LayerKeySort/pull/24) merged 2026-11-23 at 06:02:54 UTC. Candidate additions are specific to the review branch.

## 9. Documentation evidence and boundaries

The source-bound handbook inventories the 16 production C modules, principal private headers, 64 public APIs, key private branches, tests, build/distribution helpers and source navigation. This repair reuses recorded RC and Preview.3 executions at their exact SHAs. It does not rerun historical experiments or prove correctness for all inputs. Production changes, Stable contract changes, merge, tag and release are outside this repair. A new execution result requires its actual test SHA, commands, platform and logs.
