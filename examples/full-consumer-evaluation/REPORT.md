# Full v4.0.0 comprehensive consumer evaluation

Evaluation date: **2026-10-20**. Evaluator: Codex. Status: evidence delivered for
independent ChatGPT acceptance; no acceptance decision is made here.

## A. Executive engineering assessment

**Full is usable for the tested single-process editor ordering workflows. No
Full correctness, integration or documentation defect was confirmed.** The
bounded document consumer exercises live editing, comparator-managed ordering,
immutable batch results and historical persistence using only the real public
API. No evidence here justifies replacing V4's architecture.

This is a conditional engineering conclusion: generated execution is not genuine
human adoption; finite operation-count tests are not a universal correctness,
latency, allocator or uptime guarantee. Large snapshot/history costs are
reproduced and correctly released, not newly discovered leaks. Required lifetimes
and application identity/resolver logic are real integration responsibilities.

No production fix, API change, version, release, Issue edit or PR is proposed as
an automatic next step. Only this isolated Full-owned subtree changes.

## B. Baseline and scope

- Starting remote main: `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95` after fetch.
- Full Stable peeled commit: `80004ae732daf696f7e9192e417de0edb0b2aafa`.
- Annotated tag object: `83d67d3ef5dacaf6117db320d9bbea68b199e23e`.
- Branch: `research/full-comprehensive-consumer-audit`, based on that remote main.
- Original checkout was clean on `docs/mini-phase3-windows-human-guide` at
  `cf443d38eee538f04f95815287701132fcbbc77c`; left in place. A separate worktree
  was created; no previous work was reset or overwritten.
- Stable versus main has no changes in production `src/`, `include/`, `cmake/`,
  root CMake or existing tests. Public contract remains 64 functions/14 types,
  frozen LS1/LKS4SNP1 and strict V3 import. Static source rebuild is the supported
  ABI model; no cross-compiler binary ABI claim.
- Reviewed API/header, Architecture, Usage, Integration, Compatibility, Stable
  freeze/record, Validation, Development, current/historical benchmark evidence,
  snapshot/wire/migration source paths and contracts. Historical freeze prose
  retains its Preview.5 context; it is not mistaken for current release status.
- Issues #3/#4 and their comments were retrieved completely. All 47 #4 entries
  are reconciled in HISTORY.md. #1/#2 inform historical lineage; #7 is excluded
  as a Full roadmap. No Mini consumer, source or tests were used for evaluation.

## C. Consumer and independent verification

The standalone terminal application owns named/visible layer objects and
occurrence IDs. Multiple occurrences may share a payload object. It supports
relative and endpoint insertion/movement, retirement, historical capture,
buffered order-file save/load and restore into a fresh order. `show` displays
business objects; `list` displays order. See README.md for a runnable scene.

It deliberately is not a complete document-storage product: Full serializes
order associations, while this process keeps the object/ID registry. Payloads
and document-level cross-session identity remain application-owned.

Correctness has two separate models:

1. A C flat occurrence-ID sequence, without Full headers/symbols. It predicts
   each command before calling the adapter. Complete forward/reverse order,
   count/endpoints, item pointers and handle identity are checked afterwards;
   all pairs <=32 residents and deterministic sampled pairs above that size.
2. A Python list model generating command expectations without reading Full's
   actual state. Every output sequence/error result is checked independently.

The regular three-seed test uses 12k generated steps per seed. The extended
campaign uses **120k steps per seed / 360k total**, checking **430,529 processed
commands** including invalid/recovery/save/restore commands. Seeds are 17,
712200 and 43801. A separate 5,123-command scene reaches 2,048 live residents,
performs 2,048 arbitrary moves and 1,024 removals, then restores the saved order.
The named-layer business scene checks shared instances, visibility and restored
order. All passed. These are generated/replay engineering observations.

No dangling handles are passed. Removal clears the application's mapping.
Successful restore clears every old mapping and installs fresh handles.
Source destruction precedes final historical-row validation to exercise detached
snapshot lifetime. Initial and post-phase large measurement checks are separate
from the smaller independent per-command oracle; they are not presented as the
same coverage.

## D. Capability, boundary and failure results

| Family | Fresh consumer evidence |
|---|---|
| Live order | Empty/single, all four insertion/move forms, no-ops/self/adjacency, duplicate payloads, stable surviving handles, removal, endpoints, both traversals, same-source comparisons |
| Managed order | Bound context, stable equality, first-equal/lower/greater locate including absent query, remove/edit/reinsert, cursor, compare, capture/currentness, same-source reentry guard |
| Immutable utilities | Stable sort/build, unchanged input array, self-merge Base precedence, Batch chunk precedence and separately owned merged result after Batch destruction, Group capture/currentness |
| Historical state | Per-occurrence copied IDs, retained multiple states, namespace-derived LS1, buffer defaults, serialize/load/restore/edit, no loaded provenance, source destruction, association data unchanged by later payload edit |
| Migration | Frozen negative/ZERO/positive LK1 vectors, duplicate-coordinate rejection, importer OOM rollback; complete existing golden gates also passed |
| Supported invalid inputs | NULL item, live foreign handles, missing resolver result, malformed commands/files, invalid key case/domain, insufficient buffer unchanged, truncated/trailing/flag-invalid wire |

20,000 deterministic one-byte wire mutations produced **5,000 valid canonical
round trips and 15,000 clean rejections**. Accepted payload/namespace changes are
not incorrectly called parser failures. Every accepted mutated blob reserialized
byte-for-byte; rejected results published no snapshot. All strict truncation
lengths of the representative image were rejected. Full's existing golden/
hostile-parser tests supplement this finite external campaign.

GNU linker interception outside the library swept **279 failing allocation
points across 18 scenarios**, continuing each to success. Coverage includes
constructors, ordinary/full-block insertion, cross-block movement, capture,
deserialization, restore, Group build/merge, Batch build/merge-all, stable sort,
managed insertion, cursor and V3 import. Failed calls returned OOM, initialized
their outputs, left relevant source/model state unchanged and preserved live
requested totals. Every scenario ended at zero tracked live allocations/bytes.
Removal and already-adjacent movement consumed **zero allocation attempts**
while failure of the next allocation was armed. Not every internal capacity
branch or every allocation layout is exhaustively covered.

An association callback returning NOT_FOUND while attempting guarded reentry
left the source and prior snapshot currentness intact. A resolver failure
returned no partial restored order. Callback side effects themselves are not
claimed to roll back.

**Confirmed workflow limitation, not defect:** managed remove/edit/reinsert is
three application actions. OOM on reinsertion leaves the caller's edited object
nonresident; explicit retry recovers. Full's per-operation atomicity does not
promise an atomic business transaction across those actions.

## E. Performance and memory

PERFORMANCE.md gives exact definitions, full min/median/max distributions and
raw-data links. The 64/10k/100k-resident matrix uses one warmup plus five fresh
timed processes per size, with separate allocation runs. Scenarios cover growth,
small-neighborhood hotspot drags, distant/local drags, one million remove/insert
calls at 100k residents, repeated churn cycles, 16 retained snapshots, export,
buffered I/O, deserialization, restore and managed duplicate priorities.

Observed requested live bytes plateau across churn cycles and reach zero at
cleanup. Sixteen 100k-row histories add 42,378,632 requested bytes; retaining
them is materially more expensive than a single order. One corresponding wire
image is 1,600,040 bytes. Those costs follow owned row/association copies.

Per-call timer quantization, scheduling and wrapper effects are reported;
working set is whole-process memory, not Full-only allocation. Allocator
metadata/fragmentation/retained-RSS causality is **unmeasured**. File operations
are separate from serialization and do not establish durable-save latency.
Sanitizer timings are not mixed into performance results.

## F. Usability findings and demand

| Concrete experience | Classification / priority | Disposition |
|---|---|---|
| Occurrence-ID -> handle map, clearing removal/restoration mappings | Appropriate application identity/lifetime ownership; Medium app responsibility | Keep; no registry API demanded |
| Association callback only receives item and ordinal; duplicate payload occurrences need extra app identity context | Expected callback/application design; Low friction | Use expected occurrence sequence, not item pointer as unique identity |
| Per-state namespace uniqueness | Application persistence responsibility; Medium discipline | Corrected evaluator's initial slot-only namespaces; no Full defect |
| Managed edit recovery can leave a nonresident item after OOM | Documented multi-call boundary; Medium application recovery | Tested retry; optional worked example, no automatic atomic-edit API |
| Group sequence immutable but payload objects borrowed | Accepted ownership; Low explanation cost | Test pointer-order stability and separately copied snapshot bytes |
| Cursor must die before source, actual mutation invalidates it | Clear contract; Low bookkeeping | Independent scene passes, no lifetime API change |
| No public rank/index/topology counters | Deliberate abstraction, None defect | Consumer did not require private access; no new inspection API |
| General document persistence/atomic file replacement absent | Application infrastructure/non-goal, None Full defect | State limitations; do not treat LKS4SNP1 as a database |
| Supported integration paths | No defect reproduced | Official ZIP, add_subdirectory and installed target all pass |

Evaluator corrections: explicit pointer-to-boolean checks in the first strict
measurement build, canonical LK1 fixtures taken from frozen published vectors,
per-capture domains instead of slot reuse, and separation of file-I/O metric
labels from library-operation claims. A Windows runner slash mistake prevented
one ASan measurement invocation; it was corrected and rerun. An evidence
metadata helper's locale decoding error was corrected without modifying GitHub
or product files. These are evaluator/tooling issues, not Full findings. Affected
final test configurations were rerun; superseded trial captures remain local.

## G. Consolidated inventory and history

No confirmed new production correctness/security defect, supported integration
defect or current contract-documentation defect is asserted. No risk percentage
or adoption frequency is invented.

| ID | Parent concern | Evidence status / category | Current action priority | Recommendation |
|---|---|---|---|---|
| P1 | External human editor evidence | Still missing; generated integration added | Medium | Retain gap, obtain genuine traces/budgets |
| P2 | Snapshot/history representation | Confirmed accepted copying/capacity/retention cost | Medium | Merge export and history planning; app cadence first |
| P3 | Resident allocation | Confirmed traffic; allocator/RSS causal claims unverified | Low for Full changes | Retain cost, attribute before storage experiments |
| P4 | Block slack/local maintenance | Confirmed architecture cost, no demonstrated practical blocker | Low | Keep implementation; no capacity sweep |
| P5 | Application lifetime/recovery | Concrete usable but nontrivial responsibilities | Medium application / Low upstream | Keep mappings/retry policy; optional worked guidance |
| P6 | Portability/allocator/uptime breadth | Evidence gaps, not failures | Low | Demand-driven future validation |
| P7 | Whole-library formal/latency model | No universal theorem; existing structural description honest | Low | Optional analysis; no stronger claim from measurements |
| P8 | Stale registries/custom allocator/bindings/packages | Optional capabilities, not needed here | Low or None | Defer until concrete demand |
| P9 | Thread locks/database/CRDT/persistent handles | Deliberate unsupported semantics | None | Keep product boundary |
| P10 | Old live Path/relabel/prefix/equal-scan/move gaps | Resolved/superseded historical issues | None | Archive from active list; never reopen from legacy reports |
| P11 | MSVC disappearing executable | Historical unverified environment concern, not reproduced | Low | Preserve history, investigate only with reproducer |
| P12 | Older GitHub positioning topics | Confirmed metadata-only mismatch | Low | Optional owner decision; not edited |

HISTORY.md dispositions every Issue #4 entry and maps Issue #3 active families.
It preserves first-known versions, historical severity, resolution versions,
current evidence/reproducibility, root/impact, change types and dependencies.
Unknown original versions or causes remain unknown. One entry's historical
Highest severity does not make it an urgent current production change.

## H. Dependency-based recommendations and compatibility risks

DEPENDENCIES.md provides the root-cause map and A-H analysis for conditional
snapshot, resident-storage, application-guidance and block/index proposals.
The evidence does not connect every RSS symptom to one proven root cause.
Shared use of malloc is not proof that snapshots and live records need the same
replacement architecture.

Immediate work is independent acceptance of this evidence and clear application
recovery/history policy. Conditional future work begins with genuine workload
budgets. Try cadence/retention before incremental snapshot redesign and allocator
attribution before pooling. Keep optional defenses and non-goals deferred.

Any internal improvement must preserve stable resident addresses through legal
mutation, removal expiry, caller-owned items/contexts, prepare/validate/commit
atomicity, snapshot-owned immutable bytes/views, fresh restore handles, frozen
64-function/14-type 4.x declarations/statuses, exact LS1/LKS4SNP1 and strict V3
migration meaning. Pools could change memory retention; deduplication could add
shared lifetime state; neither is automatically a smaller or safer change.

## I. Validation and limitations

Fresh executed matrix, exact logs and local artifact hashes are in EVIDENCE.md.
GCC Debug/Release, MSVC Debug/Release, MSVC ASan, official ZIP and installed
consumers passed. Existing unmodified GCC/MSVC Full suites each passed 39/39;
all seven public examples ran. No MSVC executable disappearance reproduced.

GCC sanitizer linking failed for missing `libasan`/`libubsan`, an environment
limitation rather than a test pass or observed library sanitizer defect. No
fresh Clang/Linux/macOS/Win32 evaluation, UBSan/LSan campaign, actual concurrent
mutation, days-long uptime test, power-loss persistence test or practical
SIZE_MAX/revision-exhaustion campaign is claimed. MSVC ASan has no equivalent
leak-proof claim here; zero requested cleanup comes from the separate GNU
instrumented runs. No dangling-pointer UB was intentionally invoked.

## J. Git delivery

All new tracked paths are under `examples/full-consumer-evaluation/`. Production,
public header, fixtures, existing tests/build definitions, CI/governance, other
product lines, tags, releases and issues are unchanged. No merge or PR is made.

One evaluation commit is pushed on the research branch; exact local/remote SHA,
ahead/behind, current main and clean worktree are recorded in the delivery
response. No report tries to embed its own commit SHA recursively. Use `git
rev-parse HEAD` and the branch commit permalink for immutable evidence.

LayerKeySort Full comprehensive consumer evaluation complete. Awaiting independent ChatGPT acceptance and the user's remediation decisions.
