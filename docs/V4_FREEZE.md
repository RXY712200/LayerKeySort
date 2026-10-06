# V4.0 freeze — final Preview.5

`4.0.0-preview.5` is the final experimental Preview, not RC or Stable.
Stable remains v3.1.0. Next: Stage 5 — Release Candidate. No Preview.6 exists.

**Freeze commit:** `5e4fbbf6d02bff6cba1f049b357ddb8bd329e0aa`. The later release tag identifies the
final docs/packaging candidate, whose API must match this freeze.
[Manifest](../tests/fixtures/v4_public_api.json): 64 functions, 14 types, explicit
statuses, callback signatures and public fields. tools/verify_v4_api.py checks
all header tokens except comments/release text. RC must not silently update it.

## Source/ABI and V3 policy

Installed layerkeysort.h is V4-only: contextual live/managed order, flat Groups,
snapshots, sort/status and strict legacy import. Public Path/Tree/OrderedTree,
physical navigation and public LK1 codecs are removed. V3.1.0 supplies those
contracts separately. Private migration headers are not installed. Legacy live
Tree/Group/gap/bulk sources compile only for historical tests/benchmarks, excluded
from production/amalgamation. Import retains published strict LK1 decoding.

4.x preserves source names/signatures and declaration layouts: LksComparator,
LksSnapshotOptions and LksV3Lk1ImportEntry field types/order. Same architecture,
toolchain and ABI settings preserve those layouts. Normal integration rebuilds
static sources; no universal MSVC/GCC/Clang, CRT, packing or cross-architecture
binary ABI is promised. Never persist native structs. No prebuilt shared ABI is
shipped. V5 can make documented major source changes. New 4.x APIs require a
deliberate post-4.0 revision, not an unreviewed addition during RC freeze.

| Preview.4 | Final Preview.5 | Reason |
|---|---|---|
| LksImmutableGroup | LksGroup | One flat immutable V4 model |
| LksImmutableGroupBatch | LksGroupBatch | One V4 Batch model |
| 14 lks_immutable_group_* calls | lks_group_* / lks_group_batch_* | Remove transitional prefix |
| V3 live/Path/old Group APIs | Removed from primary header | Major semantic break; V3 remains available |
| LEVEL_LIMIT / NOT_IMPLEMENTED | Removed; numbers 4/6 unused | V3-only results |

No source aliases. Other V4 names remain because they distinguish real workflows.
Allocated cursors remain useful for mutation detection. Capacity, rank, topology,
marker, revision and diagnostic/failpoint APIs are private, not compatibility ABI.

## Status freeze

| Name | Value |
|---|---:|
| LKS_STATUS_OK | 0 |
| LKS_STATUS_INVALID_ARGUMENT | 1 |
| LKS_STATUS_OUT_OF_MEMORY | 2 |
| LKS_STATUS_BUFFER_TOO_SMALL | 3 |
| LKS_STATUS_NOT_FOUND | 5 |
| LKS_STATUS_INTERNAL_ERROR | 7 |
| LKS_STATUS_ALREADY_EXISTS | 8 |
| LKS_STATUS_CAPACITY_LIMIT | 9 |
| LKS_STATUS_INVALIDATED | 10 |
| LKS_STATUS_REENTRANT | 11 |
| LKS_STATUS_DOMAIN_MISMATCH | 12 |

Status descriptions are static borrowed strings. Unknown integers, including 4/6,
return "Unknown status". NOT_FOUND permits callback-reported missing application
resolution. ALREADY_EXISTS reports duplicate legacy coordinates on import.
INTERNAL_ERROR remains defensive, not undefined-input detection. No RC appends,
renumbers or reinterprets statuses without reopening freeze.

## Ownership/lifetime

| Data | Ownership/expiry |
|---|---|
| Order/ManagedOrder | Caller owns result; library owns structure, borrows items; destroy never frees items |
| Items | Caller owns; live while resident/read by callbacks |
| Handle | Borrowed resident occurrence; own removal/source destruction expires all copies |
| Cursor | Caller owns; borrows source; destroy before source; destruction leaves source intact |
| Group | Caller owns flat copied pointer array; borrows items |
| Batch/chunks | Caller owns Batch, Batch owns chunks; never destroy borrowed group_at view |
| Merge result | Fresh owned Group independent of source lifetimes |
| Snapshot | Caller owns immutable copied rows/bytes; survives source/items |
| Comparator context | Caller owns; Group/sort borrow per operation; ManagedOrder retains descriptor, borrows context until destroy |
| Association callback span | Borrowed until immediate copy after return; must remain valid then |
| Resolver item result | Caller owns non-NULL item; restored order borrows it; rollback never frees it |
| Namespace input | Copied; nonempty application-selected domain |
| Snapshot namespace/association views | Borrowed until snapshot destruction |
| Serialize buffer | Caller owns; library writes bytes, retains no buffer |
| Legacy key/input array | Borrowed for import call, not retained |

Required outputs must be valid distinct storage, not alias live input arrays,
object storage or descriptors. Copy source/destination spans must not overlap.
Dangling pointers, double destroy, invalid provenance, read/destroy races and
exception/longjmp escape are caller errors, not detectable statuses.

### Handle/cursor contracts

Handles survive unrelated insert/remove, own move, split/merge/redistribution and
index rotations. Own removal/source destruction expires them; never test a stale
handle after removal. No stale registry, persistent handle, business identity or
cross-container move. Valid foreign handles return INVALID_ARGUMENT. Pointer
numeric value is not order. Cursor borrows source/captures generation: actual
mutation invalidates; failure/query/no-op does not. Mismatch checked before
cached handle access. Reverse must be 0/1; next returns OK+NULL at end,
INVALIDATED+NULL after mutation. Cursor is not thread-safe and cannot outlive source.

### Unified callback/thread contract

Comparators/context remain callable, consistent, and resident comparison data
stable. Editing data does not automatically reorder ManagedOrder/Group: remove,
edit/reinsert or rebuild. Merge requires compatible ordered sources.
Callbacks run against complete guarded state, never partial commit. Same-source
status APIs return REENTRANT, getters zero/NULL, destroy ignored while busy.
Handle-only access retains its documented resident lifetime. Unrelated container
operations may proceed. Callback returns normally, no escaping C++ exception or
longjmp. Sort/build have no published source; keep input arrays/descriptors alive.
Restore reads an immutable snapshot that must remain alive; no partially built
output is exposed. Application callback side effects are not rolled back.

Mutable source operations require caller serialization, including reads against
mutation/callback activity. Group read/read concurrency with protected lifetime
is supported; merge/capture private marker/guard activity is serialized against
reads/destruction. Snapshot immutable reads may be concurrent with protected
lifetime; destroy/read race is invalid. Marker atomics are bookkeeping, not locks.

## Group/Batch

Build stable-sorts copied non-NULL item pointers; input unchanged, empty array
valid with a required comparator. No Path/index. item_at O(1). Fresh merge takes
Base before Incoming equality; self-merge legal. Batch owns stable sorted input
chunks, size>0; merge-all preserves earlier-chunk precedence and result outlives
Batch. Group capture uses revision 0; lazy marker only committed on success.
Historical snapshot survives source/item destruction; loaded snapshot never current.
Source provenance cannot be reconstructed from LS1/wire.

## LS1 version 1 — frozen

```text
LS1.<nonempty even-length lowercase namespace hex>.<16 lowercase ordinal hex digits>
```

Namespace represents 1..UINT32_MAX arbitrary bytes; alphabet 0123456789abcdef.
Ordinal unsigned uint64, exactly 16 hex digits including leading zeroes. No spaces,
uppercase, missing fields or trailing data. Validation accepts full uint64 range
independent of local addressability; formatter requires an existing snapshot row.
Namespace uniqueness is application responsibility; same bytes mean same domain.
LS1 is historical ordinal only, never live handle/business identity.
Canonical same-domain sign(strcmp) equals ordinal order under bytewise ASCII
collation. Cross-domain semantic compare returns DOMAIN_MISMATCH and clears result;
arbitrary cross-domain strcmp has no semantic order. Linguistic/case-insensitive
collations unsupported. 4.x preserves meaning, exact formatter output, canonical
acceptance, namespace and ordinal semantics. Incompatible future changes need a
new family/version, not reinterpretation of LS1.

## LKS4SNP1 — frozen

All bytes are octets, CHAR_BIT==8. Exact unpadded sequence:

```text
8 ASCII bytes LKS4SNP1
u32 BE flags=0
u32 BE namespace_length (>0)
u64 BE row_count
namespace bytes
repeat row_count:
    u64 BE association_length
    association bytes
```

Binary-transparent arbitrary payload including NUL; empty associations allowed.
Empty snapshot has 0 rows/nonempty namespace. Unknown magic/flags, truncation,
count/length overflow, addressability overflow or trailing bytes: INVALID_ARGUMENT.
Full syntax/counting validation precedes allocation. Smaller SIZE_MAX host rejects
unaddressable data without truncation or partial publication. OOM returns output
NULL. Deserialized snapshots own immutable historical data, no marker/currentness.
Re-serialize reproduces exact bytes; LS1 reconstructed from namespace/ordinal.
4.x freezes widths/endian/order/flags=0/no-trailing/reconstruction. Incompatible
formats require new versions, never guessed flags. No integrity/checksum/compression
inside blob; application supplies transport integrity.

[Wire corpus](../tests/fixtures/v4_wire_golden.json): seven LS1, four binary vectors,
strict V3 import plus 17 frozen published LK1 vectors. Existing snapshot torture/OOM tests remain gates. Published V3
LK1 goldens in tests/path_external.c remain private migration evidence.
Fixture hashes: tests/fixtures/v4_fixture_sha256.json. Expected bytes are reviewed
constants, never generated from encoder. Both 32/64-bit jobs use identical fixtures;
RC cannot update expected bytes without explicit freeze revocation.

## Build/package support

Tested targets: Windows MSVC x64/x86, Linux GCC/Clang, macOS AppleClang; C17 source,
C++17 consumer. Linux Clang ASan/UBSan and local MSVC ASan validate safety.
Implementation-as-C++ and every other C17 platform are not blanket-supported.
CMake>=3.21: add_subdirectory/offline FetchContent/install/export,
find_package(LayerKeySort CONFIG REQUIRED), LayerKeySort::layerkeysort. Embedded
examples/tests/install default OFF. No Python in ordinary library/installed/
amalgamated consumption. Numeric package 4.0.0/SameMajorVersion means source
rebuild compatibility, not Stable status; header says 4.0.0-preview.5.
ZIP fixed timestamps and exact five members: public header, C implementation,
V4 example, LICENSE, README.txt; SHA256SUMS covers ZIP. Reproduction/hash/membership/
tamper gates mandatory. Installed/amalgamated API match manifest, no private includes.

## Accepted costs/debt

B64 / minimum 32 is private policy, not wire/API/handle constant. No new search/tuning.
Future 4.x may change private capacity only while preserving frozen semantics and
all published structural/suitability claims. This is not authorization for RC tuning.
Ordinary local/index structural target O(B+log M), managed search adds comparator
calls; allocation/callback/platform and O(N+A) export costs are separate. This is
not a whole-library theorem or worst-case latency guarantee.

| Concern | Disposition |
|---|---|
| Per-record allocation, B64 slack, local writes | Accepted cost |
| Comparator/callback cost | Suitability limit; application responsibility |
| Full O(N+A) export, association growth, retained history, external LS1 text | Accepted cost/export-heavy suitability limit |
| Group/Batch sort/merge scratch | Accepted temporary memory |
| Allocator/RSS retention | Accepted platform behavior; zero requested cleanup remains gate |
| V3 large synchronous relabel/live Path growth | Superseded by V4 live model |
| Formal whole-library bounds | Partially addressed structural assertions; broader work deferred |
| Real-user traces | Future validation debt; synthetic data is not field usage |
| Stale-handle misuse detection | Deferred; dangling inputs caller error |
| Incremental export/persistent handles/broader platforms | Future major work, absent here |

Preview.4 remains convergence evidence. Its pooled distant-drag summary is not a
single-condition median:100k resident: 29.053 ms;100001 resident: 28.182 ms. Raw rows
unchanged. Freeze requires focused regression, not optimization reruns. Default
local MSVC Release executable disappearance is unconfirmed external behavior:
no repository deletion step, no disabled protection. Known-good Release-with-
symbols/Debug/ASan and GitHub Windows validate build/package.

## RC policy/entry

RC may fix correctness, sanitizer, portability, documentation, tests, packaging,
and performance defects while preserving frozen source/semantic/wire contracts.
No casual rename/signature/layout/status/wire changes, architecture or feature
additions. Incompatible necessary correction requires explicit freeze revocation.
Entry requires ancestry, manifest/goldens/OOM/consumer tests, full platform CI,
reproducible assets, accepted evidence and no known correctness blocker.
Tests are engineering evidence, not proof for every caller/workload. No Preview.6.

## API minimality disposition

| Family | Supported workflow / reason retained |
|---|---|
| Sort/status | Stable pointer sorting without a container; deterministic status descriptions |
| Explicit order insertion/removal/moves | Application relative order and resident occurrence lifecycle |
| Handle traversal/item/compare | Logical iteration and same-source order without physical topology |
| Managed order facade/getters/locate | Bound comparator semantics with no mutable core escape |
| Cursor | Explicit mutation detection before dereferencing a cached handle |
| Group/Batch reads/build/merge | Flat immutable processing, stable chunk/source equality |
| Capture/currentness and snapshot views | Historical copied identity and source-currentness distinction |
| LS1 format/validate/compare | Canonical same-domain external order without live coordinate semantics |
| Wire serialize/load/restore | Portable historical persistence and fresh live reconstruction |
| Strict V3 import | Supported one-way coordinate migration, no parallel V3 public live API |

No public function exposes blocks, revisions, source markers, AVL navigation,
allocator counters or internal bulk construction. Similar facade getters preserve
source ownership boundaries instead of exposing the mutable core.
