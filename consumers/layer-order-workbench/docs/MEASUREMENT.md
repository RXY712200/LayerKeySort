# Phase 2 measurement method

Independent consumer engineering evidence for released Mini 1.0.0, source SHA
`3f52798b83b6580aa4b89846b6403b0bd7b15f0a`. No new library version, production
algorithm change, Full comparison or human adoption claim. Tools use public Mini
API and its real library target only; original Phase 1 sources/tests remain intact.

## Reproduce

```sh
cmake -S consumers/layer-order-workbench -B build/wb-evidence -DMINI_SOURCE=/absolute/mini -DCMAKE_BUILD_TYPE=Release
cmake --build build/wb-evidence --config Release
ctest --test-dir build/wb-evidence -C Release --output-on-failure
python consumers/layer-order-workbench/measure/collect.py --measure build/wb-evidence/workbench_measure --memory build/wb-evidence/workbench_memory --workbench build/wb-evidence/layer_order_workbench --mini /absolute/mini --build build/wb-evidence --output /new/evidence-directory --repeats 15
```

MSVC executables are in `build/wb-evidence/Release/` with `.exe`; MinGW uses
`-G "MinGW Makefiles"`, `.exe` and no configuration subdirectory. Mini may come
from the verified official Mini-only ZIP. Output directory must not exist.
Python uses the standard library. Fewer than 15 repeats and sanitizer timing
rows are rejected. Debug and Sanitizer run correctness checks, separately from
optimized Release evidence. CI instruments both Mini and tools on Linux.

`workbench_measure --verify-all` verifies the same 36 generated inputs step by
step through unchanged `wb_execute`/array Oracle. It checks duplicates/NULL,
ID/handle relations, movement/no-ops and retired-ID error recovery. Schema tests
independently check command/API counts, end sizes, interval bounds, invalid options
and statistics. Original six Phase 1 tests remain (five without GNU wrapping).

## Fixed generated workloads

Each runs at n=16/128/2048. No human trace is synthesized. Payloads cycle
NULL/object 1/object 2, repeatedly referencing the two owned business objects.
Exact commands, comparison directions/distances and sizes are retained.

| Case | Initial → final | Commands | Distribution |
|---|---|---:|---|
| sequential-insert | 0 → n | n | tail |
| head-tail-insert | 0 → n | n | alternate tail/head |
| adjacent-move | n → n | 64 | middle before left/after right neighbor |
| distant-move | n → n | 64 | middle to front/back |
| valid-noop | n → n | 64 | before self/already-next neighbor |
| remove-reinsert | n → n | 64 | middle removal, fresh tail ID; minimum n−1 |
| forward-traverse | n → n | 64 | full walks; n+1 primary API calls each |
| reverse-traverse | n → n | 64 | full reverse walks |
| self-compare | n → n | 64 | identical ID |
| near-compare | n → n | 64 | positions i mod (n−1), adjacent, alternating direction |
| far-compare | n → n | 64 | endpoints, alternating direction |
| mixed-editor | n → n | 72 | six complete 12-command cycles; minimum n−1 |

Mixed cycle: middle-to-front, front-to-back, endpoint compare, quarter removal,
tail reinsertion, adjacent move, front-after-tail, self compare, forward/reverse
walks, self-anchor no-op, adjacent compare. Weights are not adjusted after seeing
results. Comparison scans forward from the first handle, so preceding second
handles can require reaching the tail even at distance one. O(n) is a worst-case
boundary, not a claim every comparison grows with n.

## Timing definitions

A `api_ns`: primary real public API intervals, including small dispatch/status
and traversal-loop bookkeeping. Mapping/Oracle/parse/output are excluded. Actual
API counts distinguish one traversal command from its n+1 calls.

B `map_ns`: direct-index object/ID/handle lookup and lifecycle/reference updates.
This adapter follows Phase 1's mapping pattern, but does not measure the entire
planning/validation overhead of `wb_execute`.

C `oracle_ns`: array transitions plus unchanged `wb_verify` after each command.
Its extra Mini read/comparison calls are validation cost, not A's primary calls.

D `parse_trace_output_ns`: parsing, semantic serialization, formatted output
and per-command local temporary-file flushes. This is not terminal rendering,
fsync or durable persistence. Pipeline listing prints IDs, not the full object-name UI.

E `execute_ns`: instrumented adapter loop. Separately, `wall_ns` measures the
unchanged Phase 1 executable as a subprocess on the same complete generated
script, stdout file and trace, including startup, object/setup commands, Oracle,
quit and destruction. Different scopes must not be subtracted to invent overhead.

`isolated` disables Oracle/parse/trace during timed execution; full final checking
is outside timing. `pipeline` verifies every step and measures stages. Independent
correctness precedes collection. Stage clocks perturb caches/scheduling: components
are coupled and are not independent causal contributions.

Each of 15+ repeats is a fresh process. A fresh-order warmup precedes the measured
fresh-order run in the same measurement process; both rows are retained. Phase 1
warmup is a separate process. Init/destruction are separate intervals; generation
and corpus allocation precede init. Pipeline init includes temporary-file setup.

Native clocks are QueryPerformanceCounter/CLOCK_MONOTONIC. Record nominal
resolution, minimum observed positive empty-pair interval, zero intervals among
1001 pairs, median empty pair and span count. Python wall timing uses monotonic
perf_counter_ns. Nanosecond units do not imply nanosecond accuracy. Short API/map/
no-op intervals may be dominated by clock overhead/quantization; empty pairs are
never subtracted. Summaries exclude warmups and retain count, median/min/max,
MAD and inclusive IQR. CPU affinity/frequency/load are uncontrolled; no confidence
interval, portable guarantee, causal scaling claim or best-run headline is made.

## Memory definitions

`workbench_memory` is a separate measurement executable, not the shipped app.
Sample baseline, touched application allocation, order creation, n insertions,
half deletion, reinsertion, 128 churn cycles, Mini destruction and app release.
Array/public API verification occurs between stages. Every repeat is a fresh process.

- GNU-compatible Linux/MinGW test-only symbol wrapping tracks malloc/calloc/free
  only during Mini create/insert/remove/destroy calls. Requests, request bytes,
  live/peak requested bytes and frees are actual scope-tracked observations.
  Formatting/app allocation/verification are outside tracking. Tracker storage
  is separately reported and contributes to process RSS.
- Linux malloc_usable_size reports live usable rounding, not complete metadata or
  fragmentation. Windows reports usable bytes NA. MSVC/macOS allocation wrapping
  is unsupported; its fields are NA, never a fabricated zero-allocation result.
- Linux RSS uses /proc/self/statm resident pages × sysconf page size; peak uses
  getrusage Linux KiB converted to bytes. Windows records current/peak working set
  through GetProcessMemoryInfo. macOS current RSS is unavailable (0 sentinel),
  getrusage peak bytes available. These OS metrics are not directly comparable
  or library-only memory usage.
- sizeof(Workbench) is allocated/touched. Object/map/embedded Oracle capacities
  are components, not extra allocations to sum again. Two Phase 1 trace buffers
  are an explicitly labelled estimate and not allocated by the memory tool.
  Timing corpus storage and complete Phase 1 process memory are not measured here.
- Reviewed 64-bit ABI layout estimate: 24 bytes/order, 32 bytes/occurrence, from
  three pointers+size_t and four pointers. Opaque private sizes are not obtained
  through private headers; this is an ABI-qualified estimate, corroboratable by
  actual requests, not a portable public layout guarantee.

Zero outstanding tracked allocations establishes release for this run. Retained
RSS can reflect allocator arenas/runtime/shared pages and observation overhead;
RSS staying elevated is not itself a leak. Complete allocator metadata, retained
arenas and fragmentation attribution remain unmeasured.

## Auditable outputs and limits

metadata.json records host/compiler/cache flags, Mini version/release/source hashes,
measurement commit/source hashes and source dirty state. timing-raw.csv retains
all warmup/formal rows; memory-raw.csv all lifecycle rows; summary.csv and SUMMARY.md
define statistics. workloads/ holds exact input; traces/ retains first-repeat
complete traces/output and replay checks. CI uploads ordinary Release Linux GCC
and Windows MSVC evidence, separately from sanitizer correctness.

Committed data may name an earlier measurement commit than a final data-only HEAD;
source hashes make that distinction auditable. Generated build directories can
make repository git_dirty true while measurement_source_dirty is false. This is
not a ranking of products or a basis to change Mini architecture.

Issue #7's real human sessions, representative user distribution and complete
allocator-inclusive evidence remain pending. Full adapter/snapshot comparison is
separately scoped. See [manual experience guide](MANUAL-EXPERIENCE.md).
