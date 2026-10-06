> Historical Stage-3 design record. Current final API/wire/policy contract is
> [V4_FREEZE](../V4_FREEZE.md); original provisional names/policies below are historical.

# V4 Stage 3 decisions and adversarial review

> Imported by content from commit `3c0373f4bd62287766a84e79e9ee5dbc03194e38`
> onto production baseline `c33afa2ac08248bffa6dde51bd590a5d799b394a`.
> No research commits were merged or cherry-picked. The Stage 3 status and
> evidence below are historical; current implementation scope is documented in
> [V4 Preview.1](../V4_PREVIEW1.md).

Status: selected architecture, **not production implementation/adoption**.
Specification: [V4_ARCHITECTURE.md](V4_ARCHITECTURE.md).
Baseline: `cbf1862c63fa4e23bef4f1383df7e2b464051ae0`.
Local branch: `v4-architecture-definition`. Production remains v3.1.0.

## Evidence provenance and classification

- **Experimental evidence:** Phase1/Phase2 reports, actual F/I/S code, semantic
  trace generator/replayer, immutable captured artifacts and existing tests.
- **Selected decisions:** the block core/facade, occupancy rule, pointer handles,
  lifetime marker, snapshot-domain separation and V3 concept disposition below.
- **Assumptions/preconditions:** callers respect C lifetimes, supply stable
  comparators, serialize mutable-core access, provide durable occurrence mapping
  and unique export namespaces, and accept explicit full snapshot cost.
- **Unresolved implementation details:** signatures/status mapping, struct packing,
  wire grammar, portable atomic backend, exact field-write envelope and target
  platform space/performance. No unresolved choice of primary architecture.

Reviewed code/contracts: `include/layerkeysort.h`, Tree and OrderedTree in
`src/tree.c`, private tree/group/path headers, Path construction/comparison/text,
`src/path_order_key.c`, Group/Batch and `src/sort.c`, allocator ownership/failure
behavior, API/compatibility/migration/development/design-history documents.
Research-specific review included `models.h`, `models.c`, `flat_attempt.py`,
`campaign.py`, `replay.c` and prototype tests: it was not a summary-only review.

Permanent captured evidence within this branch history:

- [Phase1 report](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ROOT_CAUSE_PHASE1.md) and
  [capture](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-root-cause-phase1/).
- [Phase2 report](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/docs/research/V4_ARCHITECTURE_PHASE2.md),
  [metadata](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/metadata.json),
  [comparison](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/comparison.csv),
  [maintenance](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/maintenance.csv),
  [validation](https://github.com/RXY712200/LayerKeySort/blob/3c0373f4bd62287766a84e79e9ee5dbc03194e38/benchmarks/results/v4-architecture-phase2/validation.json).
- Frozen Phase2 measurement implementation:
  `1b8e9f39f5d13983690daba493ff3d8900c93c5d`; final reviewed research baseline
  `cbf1862c63fa4e23bef4f1383df7e2b464051ae0` contains later validation/docs.

### Evidence erratum

Phase2 prose describes five full events on the large timeline. The machine row
for `run 12 / timeline-17-100000-100000 / V / capacity 128` in `maintenance.csv`
reports **5 relabel events, 4 full relabels**, 568153 coordinate replacements
and maximum replacement region 172867. Use those captured counts. The old prose
is retained as archival record rather than silently edited. The discrepancy
does not change outcome D or imply a different mutation trace.

### What the experiments establish and do not establish

Phase1 supports a mixed generation/validation/cleanup tail cause, not an AVL-only
cause. Reduced relabel without contract change traded tails for Path/LK1 growth.
No lower-bound theorem or universal impossibility result was established.

Phase2 validated 25 scenarios, 81 paired comparisons/162 replays, 2.2 million
semantic operations and 13.56 million repeated engine operations; 144000
prototype differential steps used independent flat reference order. Failpoint
tests covered prototype insert/split/move/snapshot preparation. These historical
counts are not newly rerun timing campaigns in Stage 3.

Large timeline (100000 initial, 100000 operations, 179886 final residents):
V3 observed max 273.2439 ms, I128 0.3208 ms, S 0.0807 ms. Requested resident metadata
was 25.190/6.536/11.513 MB respectively. The prototype counts exclude shared caller
registry and allocator headers; they are not RSS or final V4 object sizes.

I has recurring bounded reindex writes: long churn recorded 143993638 item-field
assignments, not that many distinct affected items. S performed fewer index
assignments there but has 64-byte per-resident index records. I capacity 32/128/256
changed tradeoffs; no universal best capacity follows from one machine.

Exports are separate costs. Long-project identical mutation histories exported
every 100 operations took 252.73/531.25/453.41 ms total export build+cleanup for
V/I/S, versus 26.75/49.62/43.28 ms at interval 1000. That prevents claiming contextual
order makes export-heavy applications uniformly faster.

The prototype I keys AI1 encode epoch/block rank/local ordinal, S keys AS1 encode
epoch/rank. They are experimental fixtures, not final persistence designs.
Prototype owner-counter/wrap and snapshots-after-source-destroy were not complete
public lifetime contracts. Stage 3 explicitly resolves those semantics instead
of inheriting them accidentally.

## Decision 1 — Contextual live order, explicit historical export

**Selected:** order belongs to a container; live handles query that context.
Snapshots own historical row/key associations and never need rewriting later.

**Reason:** Phase1 showed generation/repair costs and coordinate-growth tradeoffs.
Phase2 I/S removed whole-resident coordinate replacement and tested exact logical
order, while complete exports still had measurable cost. Separate those two
work domains rather than silently publish changing live keys.

**Rejected:** retain every standalone live Path as canonical V4 position, treat
block labels as continuously persisted global order, or call historical keys
current without an owner/revision check.

**Cost:** live order query is contextual; current storage keys require explicit
capture. Export-heavy users may prefer V3. This is a major contract break.

## Decision 2 — One block core and exclusive comparator facade

**Selected:** explicit neighbor-based order is the storage core. Comparator facade
owns a private instance and retains stable equality/immutable binding.

**Reason:** V3 already separated manual Path and comparator invariants; Phase2
shared identity/neighbor traces demonstrate a common logical mutation engine.
No evidence justifies two maintained storage algorithms.

**Rejected:** competing I/S production engines; exposing mutable core from facade;
preserving two old types purely for source compatibility.

Managed search uses first block maximum strictly greater than query, followed
by local upper bound. Global sorted concatenation makes block maxima monotone;
duplicates crossing blocks therefore cannot cause a missed earlier greater
element or linear equal-run search. Comparator-visible data remains immutable
while resident; arbitrary moves are only explicit-core operations.

## Decision 3 — Bounded pointer arrays and implicit block AVL

**Selected:** B128 initial policy, B+1 scratch storage, stable separate records,
implicit AVL with subtree block count and threaded neighbors. Occupancy H..B
outside the single-block case; split once; underflow merges/redistributes once.

**Reason:** Phase2 I shows a compact contextual frontier. Phase1 did not identify
AVL as the main problem. Computed block ranks avoid a new top-level relabel
problem. Threads allow linear export without prototype I's per-row rank search.

**New design versus evidence:** half-full occupancy and redistribution were not
the exact prototype policy. They are selected to bound metadata by current
population during deletion churn. Algebra in the specification proves count
termination; array/index implementation still needs differential/OOM tests.

**Rejected:** growable whole-resident arrays, global block-rank labels, cascading
repair windows, arbitrary block relocation, and speculative B-tree replacement.
B128 is tunable, not a mathematical optimum or compatibility promise.

## Decision 4 — Stable pointer handles with explicit C lifetime

**Selected:** opaque resident-record pointers. Record addresses survive shifts,
block changes and rotations. Removal expires the residence; business IDs remain
application-owned. Still-live foreign owner rejected; stale freed handle invalid.

**Reason:** I/S tested stable resident identity, eliminating V3's reacquisition
after unrelated mutation. Strong stale detection would require a registry with
separate lifetime/high-water/generation rules; no measured need justifies adding
that architecture now.

**Rejected:** handles-as-Paths, serialized pointers, silent numerical pointer
order, persistent process handles and claims to detect dangling pointers.

**Cost:** caller must clear removed handles and track container lifetime.
Applications needing defensive stale-token lookup use their own business-ID map;
that is not a disguised guarantee on freed pointers.

## Decision 5 — Historical snapshot ownership and source identity

**Selected:** snapshots copy associations and keys, retaining a tiny immutable
source marker, not source container/items. Marker reference lifetime prevents
source-address ABA. Currentness requires explicit live source+revision. Revision
never wraps; actual mutations at counter capacity fail before changing state.

**Reason:** prototype snapshots established capture/immutability but did not fully
exercise source destruction. A historical archive must survive it and must not
mistake a recycled core address for its source.

**Rejected:** retained borrowed items in durable rows, dangling former-core pointer,
unsynchronized global owner counter, and wrapping revision identity.

Marker reference count needs portable atomic lifetime bookkeeping so independent
destroy operations on otherwise independent objects do not share a hidden race.
This adds no container locking. The backend remains an implementation gate.

## Decision 6 — New snapshot key family; retire primary Path/LK1

**Selected:** new versioned family with explicit export domain and flat ordinal.
Only same-domain keys have residence-order meaning. Do not freeze exact bytes
until wire specification/golden vectors are reviewed in a later Preview.

**Reason:** AI1 block ranks expose incidental topology; reusing LK1 would make
historical rank look like a live hierarchical coordinate. V3 golden vectors
protect existing published data; never change their meaning.

**Rejected:** repurpose LK1 version1, export block/local indices, compare epochs
as cross-snapshot item precedence, or claim globally unique namespaces without
an application contract. Export namespace supplied/copied by caller; distinct
states require distinct namespace, with uniqueness external to core.

**Cost:** applications must track domain and occurrence association. A new core
load preserves order, not source marker or old handles; retain historical artifact
to retain old bytes. LK1-only users migrate through the released codec/tooling.

## Decision 7 — Flat immutable Group/Batch and explicit persistence

**Selected:** retain stable immutable item sequences and chunk/equality semantics;
remove Group-owned Path/index machinery from V4 representation. Save/load logical
sequence via completed snapshot/occurrence mapping; fresh optimized blocks on load.

**Reason:** Group already publishes immutable order. Generating mutable-core
topology and hierarchical coordinates is unnecessary once export is explicit.
Persistence semantics do not require restoring private blocks or AVL shape.

**Rejected:** fake Path compatibility that restores old generation cost, save raw
structs/pointers, serialize caller payload implicitly, or promise rollback of
caller resolver/I/O side effects.

## Decision 8 — Allocation-free commit, external serialization

**Selected:** prepare records/spare blocks before mutation; pointer movement,
pair repair, rotations and revision publish allocate nothing. Snapshot/load
prepare independent results. Remove allocates nothing. Callback guard rejects
same-core reentry; no callback inside irreversible commit.

**Reason:** V2/V3 strong atomicity and I/S failpoint results justify retaining
prepare/commit, not weakening it for contextual moves.

**Rejected:** allocate after unlink, invoke comparator during partially split
state, transactional promises for arbitrary external payload/I/O, hidden locks.
Counter/count/reference-retention overflow is preparation failure, never wrap.

## Decision 9 — F migration is not the selected path

Actual `flat_attempt.py` mixed-epoch counterexample reverses two-item order:
earlier `(1,0)` versus later `(0,1)`. Budgets 1/8/32/128 failed its invariant.
No accepted F performance frontier was measured. This rejects that implementation,
not every mathematically possible independent-key order-maintenance scheme.

Choosing contextual blocks avoids mixed live key generations and deferred
completion obligations. A new theorem/encoding may motivate separate future
research; Stage 3 does not reopen an architecture contest on that speculation.

## Adversarial internal review

| Attack | Resolution / remaining gate |
|---|---|
| Hidden O(N) mutation through labels | No materialized global ranks/Paths; only bounded arrays and logarithmic index ancestors |
| Delete to sparse high-water blocks | Half-full occupancy plus one merge/redistribution; no history-sized handle registry |
| Cascading split/merge | One-record mutation, B+1 split and H-1 pair proof; implement/test exact transitions |
| Cross-move source/destination overlap | Same-block special path; cross path split first, source repair using final neighbors/locations |
| Top-level relabel recreated | AVL in-order defines block order, rank computed via counts; no numeric block labels |
| Handles invalidated by maintenance | Separate stable record allocation; only own remove/destroy expires |
| Stale handle detection overpromise | Explicit freed-pointer precondition; no unsafe owner dereference sold as validation |
| Snapshot after source/items destroyed | All associations copied; marker retained; no former-core/item pointer read |
| Source address/counter reused | Retained identity allocation plus no-wrap revision; historical marker cannot recycle |
| Cross-snapshot strcmp confusion | Domain checked semantic comparison; arbitrary raw ordering has no live meaning |
| Persist incidental block topology | Flat ordinal snapshot; fresh load layout; keys independent of B/AVL shape |
| Equality crossing block boundaries | Monotone maxima/upper bound skips equals and preserves existing equality order |
| Iterator dereferences retired block | Compare revision before cached-state dereference; core must still be live |
| OOM after cross-move unlink | Prepare spare first; no fallible commit; full failpoint tests required |
| Callback reentry/data changes | Guard same-core callbacks; stable comparator-data contract; no external transaction claim |
| Shared marker races without container locks | Atomic lifetime bookkeeping; no concurrent same-core mutation/read allowed |
| Frequent snapshots exhaust memory/time | Explicit O(N+A), retention application-owned; benchmark combined cost at multiple frequencies |
| Fragmentation defeats payload memory frontier | Measure real allocator headers/RSS/drain; prototype payload savings not promised |

### Residual risks, not architecture contradictions

1. Bounded local writes recur often; stronger occupancy repair may increase some
   churn costs. Measure and tune B without changing meaning.
2. Per-record allocations and spare-block preparation can produce allocator
   pauses/fragmentation. Structural bounds are not realtime guarantees.
3. Portable atomic marker reference management and overflow behavior need
   toolchain/concurrency tests; no process-global mutable ID workaround.
4. Save/load usability and domain/occurrence mapping are caller obligations.
   Frequent full snapshots may be a poor fit for continuously indexed databases.
5. Raw-handle lifetime misuse is invalid C behavior; no automatic stale detection.
6. AVL transplantation/thread links and cross-move repair ordering need careful
   invariant/OOM testing. No prototype test automatically proves new maintenance.
7. New wire grammar/namespace width/API/error names remain unfrozen. Semantic
   decisions above constrain them; there is no unresolved second primary design.
8. External traces, other platforms and longer histories may expose unsuitable
   applications. No claim of universal performance or formal library-wide bound.

## Stage 3 validation and exit

This stage adds only the two architecture documents. Existing production/research
sources, public header, version, LK1 vectors, captures and build configuration
remain unchanged. No new prototype is necessary to disambiguate the selected
count policy: the local occupancy argument is in the specification. It is not
a proof of a future implementation's pointer/index correctness.

Current-run validation (Windows; existing configured build directories rebuilt):

| Configuration | Result |
|---|---|
| GCC C17, -Wall -Wextra -Wpedantic -Werror, research OFF | CTest 13/13 |
| GCC C17 strict warnings, Phase1/2 ON | CTest 18/18 |
| MSVC x64 Release, Phase1/2 ON | CTest 18/18 |
| MSVC x64 RelWithDebInfo AddressSanitizer, Phase1/2 ON | CTest 18/18; no sanitizer findings |
| Three GCC production examples | All executed, exit 0 |
| Local document links / diff whitespace / source isolation | All links resolve; diff check passes; only these two documents changed |

Compiler: GCC 16.2.0; MSVC build uses the existing Visual Studio x64 configuration.
Local Clang unavailable; no new Clang or UBSan result claimed. Build directories:
`build/v4-root-normal`, `build/v4-architecture`, `build/v4-architecture-msvc`
(Release), `build/v4-architecture-asan` (RelWithDebInfo, `/fsanitize=address`).
Rebuilt with CMake, then CTest `--output-on-failure`; the GCC caches retain
`-Wall -Wextra -Wpedantic -Werror`. Examples are `layerkeysort_example`,
`layerkeysort_ordered_example`, `layerkeysort_layer_list_example`.

Tests include V3 core, mutation soak, consumer roundtrip, parser torture, RC
contracts, workload/benchmark smoke, three Phase1 strategy contracts, Phase2
I/S differential/OOM and the F counterexample. Passing these preserves the
baseline; it does not test an unimplemented V4 API or remeasure the full campaign.
No new million-insert run or external benchmark comparison is claimed.

Exit assessment: **PASS — architecture definition only**. One answer exists for
live representation/invariant, block/index policy, handles/moves, export/domain,
Path/LK1, persistence, atomicity, ownership, threading, complexity targets,
migration and non-goals. Stronger occupancy, marker lifetime and wire/API work
are explicit implementation acceptance gates. No contradiction requires reopening
the selected direction. Next work may implement this specification in separately
authorized Preview stages; this commit is not Preview.1.
