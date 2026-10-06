# V4 Release Candidate 1 validation

RC.1 is Stage 5, not Stable. Stable remains v3.1.0. No Preview.6 exists.
Independent RC review is required before Stage 6 Stable publication.

## Starting point and freeze

Exact Preview.5 release: f413382e96175b79c117956fe4a57212785fb104.
API freeze: 5e4fbbf6d02bff6cba1f049b357ddb8bd329e0aa.
The final annotated RC tag identifies the exact validated candidate; it must be
an unrewritten descendant of Preview.5. Main remains c33afa2ac08248bffa6dde51bd590a5d799b394a.

The authoritative API/wire/migration manifests, fixture hashes and V4_FREEZE.md
are unchanged. Header edits are release-version text only: 4.0.0-rc.1. Public
surface: 64 functions, 14 types, unchanged callbacks/public fields/status values.
Status holes 4/6 remain unknown. B64/minimum32 is unchanged private policy.
No production source or algorithm was modified. No new feature family was added.

Normalized LF SHA-256 frozen evidence:

| Fixture | SHA-256 |
|---|---|
| v4_public_api.json | 57f3ef027b458632c80858d84275217cb1c2ad427aeaedbd27721e95f81708b2 |
| v4_wire_golden.json | 6be573624930b232c237dbd320318fc953cbfe0ee815de9f91ae1c805ee2e7a0 |
| v3_lk1_golden.json | 2ddbc8562ad453aeec869ad36b322794e74d055acae27b79dacab58148735c2e |

## Hostile source review

Reviewed insertion preparation, allocation-free placement, same/cross-block moves,
full destinations, source repair/retirement, threaded neighbors, AVL transplant,
rank/height/count metadata and revision checks. No allocation or comparator callback
follows irreversible live commit. Source underflow repair occurs after final
destination placement; at most one source repair. Existing rotation, two-child
retirement, redistribution, occupancy, foreign-handle and boundary tests remain.

Managed search binary-searches block maxima and then one local array: stable
upper-bound insertion and first-equal/strict-neighbor locate, no equal-run scan.
Comparator calls finish before structural commit. Descriptor copy, borrowed
context, reentry and remove/edit/reinsert tests remain.

Handles survive resident-preserving operations. Cursor generation is checked
before cached residence access; failure/no-op/query preserves it. Tests never
pass freed handles. Dangling/forged pointers and forbidden source/output aliases
remain caller errors. Repeated locate output pointers are detectably rejected;
this is tested, without promising a universal alias detector.

Snapshots retain only copied associations/namespace/rows and a detached marker,
not items, handles, blocks, contexts or former source pointers. Currentness,
empty/binary payloads, callback failure/reentry, fresh restore and source/item
expiry are tested. Atomic marker release tests use independent owned snapshots
and a protected shared read lifetime; mutable races remain unsupported.

Reviewed allocation products, block/count arithmetic, uint64 revision, marker
saturation, namespace expansion and serialized offsets on x64/x86. Constructor
bounds reserve export-size arithmetic. Wire counting validates actual available
bytes before allocation; no native structs or size_t are serialized. No unresolved
integer, lifetime, commit atomicity or parser correctness finding remains.

## Compiler analysis / issue classification

GCC 16.2 -fanalyzer and MSVC 19.51 /analyze compiled all ordinary production
modules. Four warnings were investigated; they are not represented as a clean
warning-free analysis run, and no suppression was added:

- GCC path.c:163 NULL level storage: constructor has already checked the same
  deterministic capacity-1 layout and non-NULL storage before path_levels().
- GCC order.c:224 NULL block in v4_place(): the reported move path assumes a
  caller-forged resident with NULL block. Valid handles always own a block;
  empty insertion instead requires a successfully prepared spare before commit.
- MSVC order.c:310 scratch read: both memcpy operations initialize exactly total
  records, and redistribution copies halves whose sum is total. Occupancy and
  total bounds are asserted and exercised; analysis loses their relationship.
- MSVC immutable_group.c:138 NULL merge scratch: a merge pass requires positive
  count and multiple chunks, for which both buffers were allocated successfully.
  Empty/one-chunk cases do not swap buffers or enter that copy.

Classification: no A production defect requiring repair found; no C freeze-breaking
blocker found. B accepted limitations remain below. Tests/CI were extended, not
used to conceal an incompatible correction. Ordinary GCC C17 -Wall -Wextra
-Wpedantic -Werror remains an independent clean gate.

## Symbol and integration model

The ordinary CMake target is explicitly STATIC. GCC nm found all 64 supported
functions plus 43 private lks_* text symbols, including allocator stubs and legacy
Path codecs used by migration. These are not supported API; declaration in an
archive does not make a compatibility promise. No old live Tree/OrderedTree,
Path-gap or legacy Group symbols occur in the ordinary library. Diagnostic and
historical regression libraries are separate. No DLL/SO export ABI is shipped.

C17/C++17 typed compile/link checks cover all 64 public functions. Clean source,
offline FetchContent, install/find_package and amalgamation consumers pass GCC,
MSVC x64 and Win32. Ordinary dependencies need no Python, private headers, tests,
examples or benchmark tooling. Package version remains numeric 4.0.0, with full
RC identification in the public header. Static source rebuild compatibility is
not universal cross-toolchain binary compatibility.

## Local validation matrix

| Configuration | Result |
|---|---|
| GCC 16.2 strict C17 Release | CTest 39/39 |
| MSVC x64 Debug | CTest 39/39 |
| MSVC x64 Release with symbols | CTest 39/39 |
| MSVC x64 ASan RelWithDebInfo | CTest 39/39 plus long mutation/parser |
| MSVC Win32 Debug | CTest 39/39 |

All seven public examples are executed in every local configuration. Source,
FetchContent, installed and amalgamated C/C++17 consumers are separate clean
builds. Local Clang and GCC ASan/UBSan runtime libraries are unavailable; no local
coverage is claimed for them. The existing default local optimized MSVC executable
disappearance remains an unconfirmed external-environment anomaly. No protection
was disabled; known-good Release with symbols is used.

Deterministic OOM sweeps cover create/insert/split/spare move, managed creation/
insertion, cursor, snapshot/namespace/rows/association/marker, deserialize/restore,
legacy import and Group/Batch build/merge/capture. Failed applicable operations
preserve old state/handles/revision/currentness, clear outputs and leak nothing.
Allocation-free remove, no-spare moves, locate, compare, key formatting and
serialization retain their failpoint assertions.

Seven LS1, four LKS4SNP1 and 17 published LK1 golden vectors remain exact.
Win32 and x64 use identical fixtures. Existing migration covers shuffled 10k
coordinates, deep 10k-step input plus SIZE_MAX terminal level and duplicate /
malformed / overflowing rejection. Group/Batch stable equality/source/chunk
precedence, duplicate occurrences, snapshot, reentry and rollback tests pass.

## Extended campaigns

- Explicit oracle: three seeds, 200,000 random insert/remove/move steps each;
  every-operation invariant checks and periodic full sequence/handle checks retained.
- Managed seven-pattern oracle includes 100,001 equal items, duplicates and stable
  locate/removal; 5,000 remove/edit/reinsert cycles. Additional managed churn below.
- New parser campaign: three seeds x20,000 iterations, 180,471 wire inputs and
  180,000 LS1 inputs. GCC and local ASan accepted 13,262 canonical wire inputs,
  rejected 167,209, and every acceptance reserialized exactly. No partial result,
  crash or final live allocation. Truncations, byte mutations, random lengths,
  huge row/association fields, separators and embedded NULs are covered.
- Marker test: two source families, eight threads, 64 independently owned
  snapshots per family, 160,000 protected shared read iterations after source
  destruction. No unsupported mutable race is tested.

Large diagnostic traces (seed7, cadence0) preserve independent sequence oracle
and periodic internal checks; every trace drains and reports zero live allocation:

| Trace | Initial residents | Operations |
|---|---:|---:|
| endpoint | 0 | 1,000,000 |
| equal managed insertion | 0 | 300,000 |
| timeline | 100,000 | 100,000 |
| grow/shrink long churn | 4,096 | 1,000,000 |
| distant drag | 100,001 | 100,000 |
| managed churn | 4,096 | 200,000 |
| retained historical snapshots | 4,096 | 100,000 |

Metadata tracks current residence; retained snapshot storage is separately owned.
No coordinate-depth growth, history-sized live registry or deferred repair queue
exists. Cleanup removes all resident/block allocations, then source/retained
snapshots to zero. RSS retention is distinct from requested live allocation.

Focused frozen replay: timeline/churn/distant_drag/equal, 10k operations, seed7,
three normal runs and one diagnostic per family. Original initial populations
are retained (equal starts empty; others at10k). All16 maintenance/comparison/
cleanup counters match Preview.5/Preview.4 exactly across all16 replays. No timing
optimization decision or historical data rewrite was made; Preview.4 remains
principal performance evidence.

## Remote and release gates

Before tagging, the exact final candidate must pass six GitHub jobs: Windows
MSVC x64/Win32, Ubuntu GCC, Ubuntu Clang, Ubuntu Clang ASan/UBSan and macOS
AppleClang. Each runs full39 CTest tests, seven examples, API manifest and clean
consumers. The Clang sanitizer job additionally runs long mutation/parser,
100k distant moves and 200k churn. GCC validates release assets.

Two RC source assets use the frozen five-member ZIP layout: layerkeysort.h/.c,
example.c, LICENSE and README.txt. Independent generation must be byte-identical;
metadata/membership/checksum/tamper/extra-file gates apply. No binary assets.
Publication requires annotated exact-green-SHA tag, prerelease true/draft false,
GitHub size/digest comparison and downloaded-byte verification. All protected
Preview.1-5, Stable v3.1.0 and main identities must remain unchanged.

## Accepted limitations and assessment

B: per-record allocation and block slack/local writes; comparator/callback cost;
full O(N+A) export/association growth/retained history; Group scratch; allocator
RSS retention; caller-controlled lifetimes/thread serialization; no universal
binary ABI. No hard whole-library latency or formal whole-library theorem is
claimed. Real-user traces, pooling, stale-handle registry, incremental export,
persistent handles, broader embedded targets and adapters remain deferred.

Local RC entry gates pass; no known correctness/OOM/format blocker or freeze-
breaking issue exists. Independent review, exact-candidate remote gates and
published-asset verification determine release acceptance. RC is not Stable.
Stable promotion requires separate authorization and review; no new architecture
or feature is permitted under this RC task.
