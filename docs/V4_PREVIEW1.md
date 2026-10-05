# V4 Preview.1: explicit live order

**Local, unreleased `4.0.0-preview.1`; latest public Stable remains v3.1.0.**
This Preview implements the explicit-order core only. Existing V3 Tree,
OrderedTree, Path, LK1, Group and Batch APIs remain available and unchanged.
V4 functionality below is a separate API, not a new representation inside V3.

## Start here

Application code includes only `layerkeysort.h`. Link the ordinary
`LayerKeySort::layerkeysort` CMake target or use the production source manifest.
From this branch:

```sh
cmake -S . -B build/preview1 -DLKS_BUILD_BENCHMARKS=ON
cmake --build build/preview1
ctest --test-dir build/preview1 --output-on-failure
./build/preview1/layerkeysort_order_example
```

With a Visual Studio generator, build with `--config Release`, add `-C Release`
to CTest and run under `build/preview1/Release/`. See the complete public-header
example [live_order.c](../examples/live_order.c): create, place items, traverse,
compare, remove and destroy. A locally installed Preview package uses numeric
CMake version 4.0.0; the header carries the complete prerelease version. Numeric
package matching is not an API-freeze or stable-release guarantee.

## Provisional API: 14 added functions

| Operation | API |
|---|---|
| Own container | `lks_order_create`, `lks_order_destroy` |
| Count | `lks_order_size` |
| End insertion | `lks_order_insert_front`, `lks_order_insert_back` |
| Relative insertion | `lks_order_insert_before`, `lks_order_insert_after` |
| Exact occurrence removal | `lks_order_remove` |
| Endpoints | `lks_order_first`, `lks_order_last` |
| Logical neighbors | `lks_order_next`, `lks_order_previous` |
| Borrow item | `lks_order_item` |
| Contextual precedence | `lks_order_compare` |

The header now has 73 public functions, including 59 coexisting V3 functions.
`LksOrder` and `LksOrderHandle` are opaque. Insert outputs a
`const LksOrderHandle *`; required output pointer becomes NULL on failure.
Items must be non-NULL. Relative anchors must be live in the same container.
The same item pointer can have multiple resident occurrences and distinct handles.

Creation returns NULL on OOM; destroy(NULL) does nothing. Size(NULL) is zero;
endpoint/getter/neighbor access with NULL returns NULL. Empty order and traversal
past an endpoint return NULL. Compare requires two live same-container handles
and an output integer; it clears that output to zero on failure. Remove accepts
optional item output, initialized NULL on failure. Live foreign handles are
INVALID_ARGUMENT. Dangling handles are caller lifetime errors, not detectable
missing-item queries. The appended CAPACITY_LIMIT status covers count/revision
exhaustion; old enum values are not renumbered.

## Live handles and ownership

Container owns records/blocks; application owns items. Library never frees an
item. Handles identify one residence and keep their addresses through unrelated
insertion/removal, own-block shifts/splits, neighboring redistribution/merge and
AVL rotations. Own removal expires every copy; destruction expires all handles.
Reinsertion is a new residence even if memory is reused.

Handles are **not Paths**, persistent IDs, serialization keys or pointer-value
ordering labels. Compare through the container. Caller business identity stays
outside the library. No V4 export or persistence API exists yet.

Forward/reverse traversal follows logical order across invisible block boundaries.
An insertion can change an observed adjacency without invalidating surviving
handles. To remove during traversal, save next before removing current. Do not
use removed current afterward. There is no revision-checked cursor in Preview.1.
Serialize access against mutation/destruction; no internal synchronization.

## Actual storage and maintenance

`src/order.c` implements an implicit AVL of blocks with parent/child links,
heights, subtree block counts, threaded neighbors and cached endpoints. Each
block has 128 committed pointer slots plus one scratch slot. Records are separate
stable allocations with owner, block, local index and borrowed item pointer.
No Path, label, serialized key or materialized global block rank exists here.

Global order is AVL in-order block sequence concatenated with each block's
local array. Same-block comparison uses local indices; cross-block comparison
computes block ranks through parent/subtree counts without allocating.

Empty container has zero blocks. Sole block may have 1..128 residents; otherwise
all blocks have 64..128. Insert 129 into a full block splits 64/65. Removing to 63
uses immediate right neighbor, otherwise left: merge if pair fits 128, otherwise
redistribute into balanced halves. One operation touches at most two local arrays;
there is no repair cascade or maintenance backlog. Physical index navigation and
local slot/block identities are not public.

Prepare record and optional spare block before insertion. Commit only shifts,
updates locations, links and balances; no allocation or callbacks. Failed
preparation preserves order/count/revision and prior handles. Remove, merge,
redistribution and compare allocate nothing. Private revision is uint64_t,
increments once per mutation, never wraps; it is not a public export feature.
Destruction is iterative along the block thread.

For fixed B128 and M blocks, structural insertion/removal targets
O(B+log(1+M)); same-block compare O(1), cross-block O(log(1+M)); neighbors/count
O(1), full traversal/destruction O(N). Allocator latency is not bounded. Per-item
allocation and recurring bounded local updates remain costs. These are structural
properties, not realtime or whole-library amortized performance guarantees.

## Validation and preliminary smoke methodology

[v4_order.c](../tests/v4_order.c) uses an independent flat occurrence sequence.
Three seeds run 40000 mixed steps each, checking private invariants every step
and exact forward/reverse sequence plus sampled comparisons every 17 steps.
Additional cases include 100000 endpoint inserts/drain and four 8192-item
front/hotspot/alternating campaigns. All resident handles are retained in the
reference model until their own removal, verifying association/address stability.

Exact boundaries test 128,129, first merge, and 63+128 redistribution to 95/96.
Tests exercise both single and double AVL rotations, two-child root-block
transplant, repeated shrink/drain, duplicate item pointers, foreign/NULL arguments,
revision/count capacity and deliberate non-dangling checker corruption.

Failpoint sweeps cover constructor, record, first block, full-block split and
split into an existing many-block index. Creation has one allocation; ordinary
insertion one, empty/full insertion two; no separately allocated AVL node or
temporary preparation allocation exists. Each earlier allocation failure is
followed by success; failures preserve exact target block bytes, revision,
handle observations and live bytes/blocks. Removal/drain runs with next allocation
set to fail and consumes zero attempts. Final diagnostic live allocation count
and bytes are zero.

Diagnostic counters are absent from the normal library. They count record
location reassignment, local arrays touched, distinct index/thread blocks touched,
splits, redistribution, merges and rotations. Per-operation tests require at most
one split/one repair, two local arrays, <=2B record location updates and a
conservative 16(h+2) index-block envelope. Index writes do not mean rewriting
residents whose computed ranks changed. Checker recursion is capped by machine
word size and visit budget; production destruction/maintenance is iterative.

Optional [v4_core_smoke.c](../benchmarks/v4_core_smoke.c), built with benchmarks
ON, is a single-machine sanity screen, not final performance evidence:

- Sequential 100000: V3 gets short predefined two-level explicit coordinates;
  V4 appends handles. This does **not** measure V3 managed relabel policy.
- Same-neighbor 2000 and random explicit 10000: identical seed 17 neighbor intents;
  V3 generates/owns application Path keys and inserts cloned coordinates;
  V4 places directly relative to handles. Application flat-array bookkeeping is
  outside timed insertion/removal intervals.
- Both engines drain by alternating front/back removal, perform 10000 precedence
  queries and verify exact traversal. V3 lacks public logical iteration, so its
  timed traversal uses the existing private ordered-fill helper with a prepared
  buffer; V4 uses public neighbor traversal. This is an explicitly different
  adapter, not a claim of identical API cost.

Windows uses QPC; portable fallback uses C17 TIME_UTC and is not a realtime timer
guarantee. Timing includes clock/check overhead, one run, no percentile inference.
No allocator/RSS or universal speedup conclusion follows. Final performance
acceptance remains later work.

### Local acceptance results

Windows x64 validation used GCC 16.2 and MSVC 19.51.36260. GCC C17 Release
with `-Wall -Wextra -Wpedantic -Werror`, MSVC Release and MSVC AddressSanitizer
each passed CTest **14/14**, including all previous V3 tests and the new V4 test.
The ASan configuration reported no sanitizer findings. Local Clang and GCC
ASan/UBSan runtime libraries were unavailable; no result is claimed for them.

Public C/C++ consumers passed source `add_subdirectory`, offline FetchContent
and installed-package builds. Generated amalgamation, reproducible packaging,
all four examples and consumers compiled against the original V3 public header
also passed. No internal header is required by application examples/consumers.

The structural campaigns checked 388599 operations, including 120000 randomized
oracle steps. Observed maxima were **191 record locations**, **two local arrays**
and **13 index/thread blocks** per operation. All rotation forms, redistribution,
merge and two-child block detachment were exercised. Every tested allocation
failure preserved committed state; final live allocations and bytes were zero.

One isolated GCC Release smoke capture is shown below, in milliseconds. Each
engine produced the same full traversal checksum for each workload. These rows
use the different public operation costs/adapters described above and are not a
general performance comparison or percentile estimate.

| Workload | Engine | N | Insert total | Remove total | Traversal | 10000 compares |
|---|---|---:|---:|---:|---:|---:|
| Sequential explicit grid | V3 | 100000 | 59.914 | 58.478 | 4.850 | 3.177 |
| Sequential explicit grid | V4 | 100000 | 8.491 | 10.568 | 1.770 | 1.240 |
| Same neighbor | V3 | 2000 | 188.994 | 127.378 | 0.136 | 80.138 |
| Same neighbor | V4 | 2000 | 0.173 | 0.238 | 0.009 | 0.350 |
| Random explicit | V3 | 10000 | 6.908 | 3.310 | 0.258 | 0.609 |
| Random explicit | V4 | 10000 | 1.111 | 1.008 | 0.050 | 0.504 |

**Preview.1 acceptance: PASS for the explicit live-order scope.** This is a
local implementation result, not publication or final V4 release acceptance.

## Architecture provenance and deferred features

[Architecture](design/V4_ARCHITECTURE.md) and [decisions](design/V4_DECISIONS.md)
were imported as document contents from
`3c0373f4bd62287766a84e79e9ee5dbc03194e38`, onto production
`c33afa2ac08248bffa6dde51bd590a5d799b394a`. No research ancestry, prototype,
instrumentation or capture was imported. Missing research links use exact-commit
permalinks. Historical Stage3 validation is not this Preview's validation.

**Not implemented:** moves, comparator facade, snapshots/export, new key format,
persistence/load, LK1 migration, V4 Group/Batch, rank/select, internal locks,
incremental export or final API/wire freeze. Existing `LksOrderedTree` is V3,
not a V4 facade. Full architecture requirements such as source-marker lifetime
apply when snapshot support is implemented; Preview.1 does not add unused marker
allocations or lifetime infrastructure.

Next Preview work should build on this core; moves and the comparator facade
are deferred to Preview.2 consideration, subject to its separately supplied
scope. Snapshot/format/persistence and final performance work remain later-stage
items, not silently assigned to Preview.2 by this implementation.
