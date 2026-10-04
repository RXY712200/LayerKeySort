# Design history

This is an archive of V2 and Preview-era engineering decisions, representation measurements, and scope dispositions. It is not the current V3 public contract or current managed-insertion architecture. For those, use [API](../API.md), [Compatibility](../COMPATIBILITY.md), and [Architecture](../ARCHITECTURE.md). References to "current" inside archived sections refer to the development stage described there.
The V3 design review also evaluated bounded block/indirection labels against
geometric in-place relabeling. Blocks might reduce large relabels but would
add a second order representation, block split invariants, and lookup
synchronization. V3 retained geometric relabeling and later added endpoint
carry. This records that decision, not a future architecture mandate.

## Preserved Path contracts and stable V2 design notes

### Coding constraints

V2 Preview.3 uses the full public slot domain `0..65535` (65,536 values) while
retaining the compact numeric `uint16_t` slot array and aligned `size_t` level
array in one Path step allocation. The public API accepts/returns slots as
`unsigned int`; constructors and append operations validate the range before
narrowing. The `uint16_t` representation covers the complete public domain.
Path storage grows with step count and capacity; formatted text is generated
on demand and is never stored in each Path.

`src/slot_codec.c` owns the fixed-width three-character radix-54 conversion.
Its exact rank-ordered alphabet is
`23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz`:
`2..9`, then allowed uppercase letters, then allowed lowercase letters.
The digits are **codec ranks**, so `'2'` is rank zero. Their ASCII byte order
matches rank order, enabling token-only lexical monotonicity without locale
rules. The codec omits visually confusable `0/O/o` and `1/I/i/L/l`; it uses no
punctuation so tokens are easier to copy through logs, URLs, configuration,
and shells, and `/` remains a structural separator. See [API.md](../API.md) for
the complete character order and public text contract.

`src/path_text.c` alone formats the Path text: one direction character,
three codec characters per step, one `/` between steps, and minimal decimal
metadata for a nonzero first absolute level or later level delta greater than
one. `/` is a separator, never the old unary level count. The formatter has
one canonical output per valid Path. V2 Preview.4 `lks_path_parse()` accepts
exactly that output; `src/path_order_key.c` owns a separate versioned durable
key. Historical Preview-to-Preview display-text compatibility was not promised;
published stable 2.x and the then-proposed stable 3.x boundary preserve the
documented canonical grammar. A zero-based chain
with consecutive levels uses exactly `4 * depth` text characters. In the
general case, add the decimal digits of emitted level metadata to that
per-step cost. This is separate from internal numeric Path storage.

| Depth | Preview.2 text length | Preview.3 text length |
| ---: | ---: | ---: |
| 1 | 3 | 4 |
| 2 | 6 | 8 |
| 4 | 15 | 16 |
| 8 | 45 | 32 |
| 16 | 153 | 64 |
| 32 | 561 | 128 |
| 64 | 2145 | 256 |

These verified consecutive-level examples show a small shallow-text cost
and shorter deep text. Skipped levels cost decimal metadata digits rather
than a number of slashes proportional to the level. The focused tests also
format a first level of `SIZE_MAX` with decimal metadata. Levels are
`size_t`; later levels must be strictly increasing and their deltas positive,
so a step at `SIZE_MAX` cannot have a following step.

`src/lks_policy_internal.h` owns policy values, separate from Path storage
and text encoding. Its sparse bulk rule is integer division
`((index + 1) * 65536) / (blocks + 1)` for block indexes `0..blocks-1`.
It distributes roots through the full coordinate space and leaves both
endpoints unused. With one block, the slot is 32768. Bulk recursion uses at
most 26 logical Path blocks, independent of physical index shape and codec
radix 54; the goal is insertion room. The initial online slot is 32768.
Endpoint target spacing 10, preferred/hard depths 4/6, and local windows
8/16/32/64 are provisional placement heuristics, not correctness limits.
The minimum midpoint span of 2 is the arithmetic requirement for an integer
slot strictly between two existing slots. Stage 4 removed a redundant repair
gain threshold and renamed the logical block and midpoint concepts without
changing generated Path layouts. Stage 5 evaluated depth/window variants and
retained these values after cross-workload and memory checks; they remain
provisional rather than formally optimal.

### Observed Preview.3 effect

| Ascending online, N=1024 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| Maximum depth | 4 | 1 |
| Repair attempts | 563 | 0 |
| Local successes | 38 | 0 |
| Abandoned repairs | 525 | 0 |
| Accepted deeper Paths | 520 | 0 |
| Full rebuilds | 5 | 0 |
| Peak live bytes | 210176 | 90200 |

| All-equal online, N=128 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| Maximum depth | 5 | 1 |
| Local repairs | 5 | 0 |
| Peak live bytes | 22760 | 11352 |

The wider slot domain substantially reduced congestion in these measured
workloads. Forced local-repair and fallback regression cases still matter.
These samples do not establish that every workload stays at depth one, that
repair is obsolete, or that worst-case complexity has been solved.

### Historical merge code

The public `lks_group_merge`, established in V2 and retained in V3, stably merges the ordered item sequences,
placing comparator-equal Base items before Incoming items, then builds a new
Group with fresh Paths. Neither source Group changes. The private historical
v1 owned-base helper instead preserves an exclusively owned Base object and
plans Incoming coordinates; it remains only to compare against v1 regression
evidence. `tests/property.c` exercises it under
`LKS_ENABLE_V1_REGRESSION_HELPERS`. The ordinary CMake production library
excludes that gate; the separate diagnostic target and Visual Studio test
configurations enable it. The frozen `tests/benchmark.c` source is retained
for historical tag analysis but excluded from the current Visual Studio build:
its ChildBlock measurements are not meaningful for the new representation.
The former `--stage14.3-frozen-smoke` runner belongs to that historical
benchmark; current validation uses the retained V2 regression fixtures, new
V3 ordered-container tests, and
`--public-api-usage-smoke`. The helper is not public API.

The following online-repair description records the **stable V2.0.0 baseline**,
not current V3 managed insertion. In stable V2 the online repair policy lives
in `src/lks_policy_internal.h`. Tree stores a
single Path-keyed AVL index; its physical shape is implementation-defined,
independent of Path hierarchy. Bulk generation first assigns sparse ordered
Paths, then builds a balanced index. Group order comes from in-order traversal.
The comparator upper-bound search directly finds the last existing equal item
in index-height work rather than scanning the equal run.

Online local repair prepares up to 64 contiguous logical-order nodes, creates
new Paths between unchanged exterior bounds, and validates depth/order before
commit. Existing nodes retain their in-order ranks, so a strictly increasing
replacement key sequence preserves the BST relations of their current shape;
the new node is then linked and rebalanced without allocation. The full
rebuild remains the correctness fallback and swaps an independently built
Tree. No ChildBlock is used by the mutable index. This also makes physical
height depend on node count rather than caller-supplied Path depth. Repair
counters are process-wide only in diagnostic builds. In V2 Preview.4, the
policy now inserts an open-end candidate directly while its depth is at most
six: repeatedly regenerating overlapping endpoint repair windows was costly
without improving the accepted coordinate. Interior gaps retain repair, and
an open-end candidate deeper than six still attempts repair before fallback.
Comparator insertion transfers its already-owned candidate Path into the node
after successful allocation. This removes a transient clone but may retain
spare Path capacity, a small measured peak-memory tradeoff. Detailed counts,
candidate trials, and limitations are in [BENCHMARKS.md](../BENCHMARKS.md).
V2 Preview.4 supports exact Path removal with allocation-free AVL successor
transplant. Rekey first allocates a replacement Path and node, then links the
replacement and detaches the old node without any further allocation. Delete
does not compact Path coordinates; AVL rotations only change physical links.
Item ownership remains with callers. Published Groups remain immutable.
V2 Preview.4 includes Stage 3's strict canonical display parser and a portable,
bytewise-sortable `LK1:` persistence key. Its levels use minimal base-256
big-endian bytes with descending-order complement and a unary byte-count
prefix; it does not serialize `size_t` or structs. `src/slot_codec.c` remains
the only production owner of the display slot alphabet and now decodes it.
The parser rejects larger-than-`SIZE_MAX` levels on a receiving platform.
Both parsers iterate over steps and abandon partial Paths on failure. The
exact normative v1 grammar and ordering proof are in [API.md](../API.md).
Golden vectors lock the v1 bytes before publication. After publication, an
incompatible representation requires a new key version. The key version is
independent of the library's Preview version.

### Stage 4 scope disposition

The current core supports serialization and bytewise ordering of individual
Path coordinates. It does not serialize a whole Tree, its physical AVL shape,
caller item payloads, or application item IDs. A Tree mutation may change
coordinates, so persistence of a coordinate does not make it an immutable
item identity. Full rebuild remains a failure-atomic correctness fallback;
Stage 5 measured and reduced a repeated endpoint repair pattern, while
adversarial workloads can still trigger costly rebuilds. No amortized bound
is asserted.

The V2 core targets single-process ordering. Concurrent independent gap
insertion, replica convergence, and CRDT semantics are optional future
architecture, not a prerequisite for this core. A public custom allocator is
also optional integration work: no present public correctness contract needs
one, and diagnostic fault injection remains private. CMake, Visual Studio,
and direct C17 source integration are supported; package-manager recipes and
whole-Tree serialization are separate optional integrations. `lks_sort()` is
a stable convenience operation; Stage 5 retained it after comparison with
platform `qsort`, which does not promise stable ordering.

### Local stabilization disposition after Stage 5

This table describes the released V2 Preview.4 core for a later roadmap review;
it does not claim that Preview.3 contains these changes.

| Area | Disposition |
| --- | --- |
| Physical Tree/Path coupling, ChildBlock movement, equal-run scan, physical-subtree repair restriction, deep physical Tree recursion | Solved by the Path-keyed AVL and logical-range repair. |
| Exact removal, rekey, display parser, portable sortable Path key | Solved for their documented public contracts. |
| Online heuristics | Evaluated on multiple workload families; current values retained provisionally, with a targeted endpoint repair rule. |
| Full-tree rebuild | Retained failure-atomic correctness fallback. Reduced endpoint repair waste, but duplicate-heavy and alternating inputs still incur measurable rebuild cost. |
| Formal amortized online-insertion bound and fixed memory ceiling | Not established; no such public guarantee is claimed. |
| Public custom allocator, package recipes, whole-Tree serialization, distributed/CRDT semantics | Optional future integrations or distinct architecture. |
| Specialized `lks_sort()` optimization | Deferred; the stable convenience implementation passed correctness and contextual `qsort` comparison. |

Benchmark evidence and complexity qualifications are in
[BENCHMARKS.md](../BENCHMARKS.md). A presentation graphic may later use the
measured all-equal comparator-call reduction and its exact workload label;
the mixed results do not support a broad speed claim or a winner graphic.
