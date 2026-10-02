# Developing LayerKeySort

The current branch is an unreleased `v3.0.0-rc.1` candidate based on
the published Preview.5 snapshot. Stable `v2.0.0` remains available. The 2.x
compatibility policy is unchanged.

## Repository layout

- `include/` — public header.
- `src/` — production C implementation and private headers.
- `tests/` — property, stress, deterministic soak, and public API validation code.
- `benchmarks/` — optional current public-API benchmark harness and matrix runner.
- `examples/` — managed Tree first use, basic sort, and dynamic layer-list public API examples.
- `demo/` — executable validation/demo entry point.
- `docs/` — user documentation, visualizer, and site assets.
- `LayerKeySort.slnx` — Visual Studio solution.
- `LayerKeySort.vcxproj` — Visual Studio project and build configurations.
- `CMakeLists.txt` — reusable production library, example, and test targets.
- `.github/workflows/ci.yml` — MSVC, GCC, Clang, and AppleClang validation.

## Toolchain

Both Visual Studio and CMake compile as C17. The CMake matrix also targets
GCC and Clang on Ubuntu and AppleClang on macOS. New production C source
belongs in both build systems. Consumer setup is in [INTEGRATION.md](INTEGRATION.md).

## CMake build and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`layerkeysort` contains production source only. All three example targets link
that library. `layerkeysort_tests` and `layerkeysort_soak` link a separately
compiled diagnostic variant so allocation counters and single-shot fault
injection remain usable without shared mutable instrumentation in ordinary
production allocations.
`-DLKS_BUILD_BENCHMARKS=ON` adds a production-linked public-API harness and
three small CTest smoke cases. See [BENCHMARKS.md](BENCHMARKS.md) for timed
results and limits; diagnostic counter builds are separate from timed builds.
For a supported GCC or Clang toolchain, `-DLKS_ENABLE_SANITIZERS=ON` enables
AddressSanitizer and UndefinedBehaviorSanitizer. Diagnostic fault injection is
process-global test state and carries no concurrent-test guarantee.

The `layerkeysort_mutation_soak` CTest case runs 30,000 deterministic mixed
operations: 22,500 explicit-Path operations with seed `0x6B47C291`, followed
by 7,500 comparator-ordered operations with seed `0xA9172E63`. The two phases
keep comparator invariants explicit. The flat reference model checks item/Path
associations, strict Path order, Tree size, returned item pointers, AVL
balance, and compatible comparator order. The test exercises gap generation,
insert/remove/rekey/find, same-Path rekey, canonical display/LK1 round trips,
and absent-Path failures; it checks diagnostic live allocations after each
phase. Every 4,096 steps it drains the active set to avoid making the soak a
Path-depth stress test (the existing deep-Path tests cover that separately).
Run a larger manual correctness soak with:

```sh
./build/layerkeysort_soak --operations 100000
./build/layerkeysort_soak --operations 500000
```

For multi-configuration generators, include the configuration directory, such
as `build/Release/`. Operation counts are validation scale, not speed claims.

## Build configurations

The Visual Studio project defines **Debug**, **Release**, and **ASan** configurations for x64. Open `LayerKeySort.slnx` in Visual Studio and select the configuration and x64 platform before building.

## Running the validation runner

The Visual Studio executable entry point is `demo/main.c`. With no argument it
runs the V2 focused tests, public API smoke, deterministic property tests,
stress tests, and OOM tests. The CMake test executable uses `tests/v2_main.c`
for the same preview suite. `tests/benchmark.c` preserves the historical v1
benchmark evidence and remains available through its existing focused runner
switches; its exact v1 baseline expectations do not describe V2 Path output.

The entry point also handles the existing focused arguments below; these spellings are read from `demo/main.c`:

- `--stage14.1-only`
- `--stage14.2-only`
- `--stage14.2-oom-only`
- `--stage14.2-release-smoke`
- `--stage14.3-frozen-smoke`
- `--public-api-usage-smoke`

Pass an argument through Visual Studio’s Debugging / Command Arguments setting, or run the built executable with that argument. The project is configured as a console application and its output name follows the project configuration. These runner options are development validation hooks, not a stable public command-line interface.

## AddressSanitizer note

On Windows, the ASan runtime DLL `clang_rt.asan_dynamic-x86_64.dll` may need its MSVC runtime directory present in the current process `PATH` when launching an ASan-built executable. This is an environment/runtime lookup note, not a LayerKeySort defect. For a one-off PowerShell launch, prepend the appropriate installed MSVC runtime directory to the current process only:

```powershell
$env:PATH = "<MSVC runtime directory>;$env:PATH"
./path/to/LayerKeySort.exe
```

This does not change the persistent system PATH. Do not obtain runtime DLLs from unofficial download sites.

## Validation coverage

The repository contains public API smoke coverage, deterministic property tests, stress tests, allocation-failure and out-of-memory tests, and frozen regression checks. The Visual Studio project provides Debug, Release, and AddressSanitizer builds. The committed project also contains older development entry points and benchmark code; their presence does not make them public API.

## V3 manual and managed architecture

`LksTree` is the manual coordinate container. `LksOrderedTree` binds a
comparator descriptor at creation and contains one private `LksTree` index;
both use the same Path-keyed AVL mechanics. The ordered wrapper does not expose
the manual index, arbitrary Path insert, or rekey. Removing an exact Path
cannot disturb the relative comparator order of remaining items. Caller item
objects and comparator context remain borrowed.

Managed insertion finds the comparator upper bound in AVL-height work. It
first generates a direct Path. At an open end, Preview.2 scans the prior
coordinate toward its root and carries a saturated slot into the nearest
ancestor with capacity, truncating the suffix. Append advances an ordinary
slot; the negative root moves in reverse slot order. Prepend allocates a new
negative-root coordinate, carrying a full root slot to the next level. The
first 64 successful consecutive endpoint inserts use spacing ten; subsequent
inserts in the same run use spacing one. An interior insert or successful
removal resets the run hint. Failed operations leave it unchanged. This
preserves interior room for mixed workloads while allowing long endpoint
runs to consume each root-level slot before requesting another level.

The provisional policy prefers direct depth at most six and accepts open-end
depth up to sixteen. When a candidate is deeper or a gap operation reports
`LEVEL_LIMIT`, relabel planning starts with eight
logical neighbors and doubles the region until it succeeds or reaches every
existing node. There is no 64-node architectural ceiling. The small-region
arrays use stack scratch; larger regions allocate checked-size scratch arrays.

For a bounded region, balanced recursive gap generation prepares a Path for
each old node and the new rank. If the region reaches the entire container,
the existing sparse bulk Path generator prepares only new coordinates, not a
replacement Tree. This avoids expensive repeated gap splitting across the
whole collection. Existing AVL nodes retain their in-order ranks. Validation
checks strict Path increase and unchanged exterior bounds before mutation.
The insertion gap also determines a vacant physical child link before commit.
Commit swaps Path pointers, links the already allocated new node, performs
allocation-free AVL rotations, and destroys old Paths. Thus an OOM during
scratch allocation, Path generation, or node allocation exposes no mutation.

The Tree has no physical `rebuild_with_item()` fallback in V3 managed online
insertion. A full-range Path relabel can still touch `n` old nodes and
construct `n + 1` Paths. Comparator upper-bound search visits `O(log n)` AVL
nodes, with comparator callback cost per visit. A direct endpoint carry scans
at most Path depth `d`, copies at most `d` steps, then AVL insertion/search
performs `O(log n)` Path comparisons (each may inspect `O(d)` steps). A
single `k`-node relabel attempt traverses `O(k + log n)` index links, prepares
`k + 1` Paths, and validates them; Path generation/compare/copy cost depends
on their actual depths. Doubling windows bound the sum of **region-node
visits** across failed attempts by a constant multiple of the final window,
but this does not bound Path-generation cost by `O(k)` alone. Complete
insertion has no claimed worst-case `O(log n)` time or formal amortized bound.

We evaluated a bounded block/indirection label strategy against geometric
in-place relabeling. Blocks might reduce large relabels but add a second order
representation, block split invariants, and lookup synchronization. The
measured geometric design removes physical reconstruction and performs well
on duplicate-heavy and alternating workloads without that complexity. It is
the one retained V3 design. Preview.2 adds endpoint carry to reduce how often
this relabel policy is entered. The preferred/open-end depths, burst threshold,
and stride are private policy hints, not validity limits or a compatibility
contract.

The V2-era regression suite still uses a diagnostic-only private bridge to
exercise the shared AVL against frozen fixtures. Public V3 tests and the soak
use `LksOrderedTree` directly. Normal production builds omit the bridge.

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
and shells, and `/` remains a structural separator. See [API.md](API.md) for
the complete character order and public text contract.

`src/path_text.c` alone formats the Path text: one direction character,
three codec characters per step, one `/` between steps, and minimal decimal
metadata for a nonzero first absolute level or later level delta greater than
one. `/` is a separator, never the old unary level count. The formatter has
one canonical output per valid Path. V2 Preview.4 `lks_path_parse()` accepts
exactly that output; `src/path_order_key.c` owns a separate versioned durable
key. Cross-Preview display-text compatibility is not promised. A zero-based chain
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

Current public V2 `lks_group_merge` stably merges the ordered item sequences,
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
candidate trials, and limitations are in [BENCHMARKS.md](BENCHMARKS.md).
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
exact normative v1 grammar and ordering proof are in [API.md](API.md).
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
[BENCHMARKS.md](BENCHMARKS.md). A presentation graphic may later use the
measured all-equal comparator-call reduction and its exact workload label;
the mixed results do not support a broad speed claim or a winner graphic.

### Core engineering constraints

- Keep implementation in portable C17 under both build systems.
- Preserve public ownership and ordering semantics; generated Path coordinates
  can change when the preview allocator policy changes.
- Preserve stable ordering and the documented ownership rules.
- Keep relevant regression coverage passing when behavior changes.

## Per-Preview documentation review

Before every future Preview commit or release, review and update as applicable
`README.md`, `docs/API.md`, `docs/USAGE.md`, `docs/DEVELOPMENT.md`,
`CHANGELOG.md`, and the active roadmap GitHub Issue. The review is mandatory
even when a file needs only a small change or none. When useful, leave a
completed Preview implementation-result comment on the active roadmap Issue,
especially if a planned problem was solved, an assumption changed, an issue
was superseded, measurements changed technical-debt priority, or work was
deliberately deferred. Review before posting; documentation work itself does
not imply a release.

## Line endings

Do not normalize line endings as part of unrelated work. Windows Git configurations may report LF/CRLF conversion notices; review whether a change is substantive before altering files.
