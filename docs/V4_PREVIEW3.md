# V4 Preview.3 — immutable historical export

`4.0.0-preview.3` is experimental. Stable recommendation remains **v3.1.0**.
Preview.2 is its frozen production ancestry. The provisional API, LS1 family and
snapshot wire format are subject to the Preview.5 compatibility decision.
Preview.3 adds export, not a new live ordering algorithm.

## Two separate worlds

| Live order | Historical export |
|---|---|
| `LksOrder` / `LksManagedOrder` | `LksSnapshot` |
| contextual resident handles | ordinal rows and copied bytes |
| mutate sequence with stable resident handles | immutable once successfully captured |
| no standalone position key | caller-buffer LS1 key formatting |
| handles cannot be serialized or database keys | canonical, serializable historical domain |

Snapshot storage contains no item pointer, handle, block, comparator context,
former container pointer or Path. It owns one namespace, row offset/length
metadata and a packed association byte buffer. Ordinals are implicit row indices.
It does not store N copies of namespace text or N materialized keys.

An application chooses a nonempty namespace of 1..UINT32_MAX arbitrary bytes.
The library copies it; it does not generate IDs or guarantee uniqueness. Do not
reuse one namespace for different states. Reuse for identical exported state is
valid only according to application policy. Duplicate associations are legal.
If repeated objects require distinct occurrence identities, encode that yourself.
The in-memory provenance marker is never an export namespace.

## Capture and association lifetime

`lks_order_snapshot_capture(order, options, &snapshot)` and
`lks_managed_order_snapshot_capture(managed, options, &snapshot)` require an output
pointer, clear it first, and return a caller-owned snapshot on success. Capture
copies options for the operation but retains neither the descriptor nor context.
Namespace is copied before callbacks. NULL/empty namespace is INVALID_ARGUMENT.

Optional `LksSnapshotAssociationFn(item, uint64_t ordinal, context, &data, &size)`
runs **exactly once per row** in established logical order. No callback means
empty associations. Nonzero size requires non-NULL data. Callback status propagates
without publishing a partial object. Returned memory must remain valid through
the library's immediate copy after callback return: callback-local stack storage
is invalid. A reusable caller-owned scratch buffer is valid because each span is
copied before the next invocation. Item/context remain caller-owned.

Callbacks must return normally, without longjmp or exceptions across C frames.
Same-source getters return NULL/zero; status operations reject REENTRANT and
destroy is ignored while callback runs. Recursive capture is rejected. Other
independent containers may be used. Managed capture performs no comparisons.
Capture performs one threaded traversal, no per-row block ranking, no Path
generation and no revision changes. Handles and cursors remain valid, including
on failure. Work is **O(N + A + callback work)**, where A is copied byte count.
Association buffer geometric growth can retain spare capacity; it is not an
allocation per row. All allocation failures clean temporary state.

## Provenance and currentness

A live core gets one separately allocated private marker only at the final
successful step of its first capture. Failed first capture leaves no persistent
marker. Core and snapshots retain references to the same immutable identity.
The core's logical revision is copied into each captured snapshot.

`lks_order_snapshot_is_current` / `lks_managed_order_snapshot_is_current` require
both identical marker and identical revision. Queries, capture, failed mutations
and actual move no-ops preserve currentness. Actual insert/remove/move makes
earlier captures stale. Another source is never current. Loaded snapshots have
no marker/revision provenance and are never current, even if restored sequence
and namespace are identical. Fresh restored/imported cores establish their own
lazy identity on capture.

Reference increments check capacity. GCC/Clang use C17 atomic_size_t with CAS
retain and acquire-release decrement; MSVC uses Interlocked long operations.
Final release frees the marker exactly once. Live source may be destroyed before
all historical snapshots. Snapshot destruction releases only its own reference.
Atomic bookkeeping does **not** make mutable containers thread-safe.

Completed snapshots permit concurrent read operations when the application
protects their lifetime. Destruction must not race with readers. Mutable sources
and diagnostic allocator instrumentation remain caller-serialized.

## Public immutable read and output rules

| Function | Result |
|---|---|
| `lks_snapshot_destroy` | free owned bytes/marker reference; NULL harmless |
| `lks_snapshot_count` | size_t row count; NULL returns 0 |
| `lks_snapshot_namespace` | borrowed bytes and size |
| `lks_snapshot_association` | borrowed bytes/size at size_t row; zero size returns NULL data |
| `lks_snapshot_key_length` | key characters excluding NUL; NULL returns 0 |
| `lks_snapshot_key_format` | format existing row into caller buffer |
| `lks_snapshot_key_validate` | strict canonical LS1 check |
| `lks_snapshot_key_compare` | strict same-domain ordinal comparison |
| `lks_snapshot_serialized_size` | exact blob byte count; NULL returns 0 |
| `lks_snapshot_serialize` | write exact blob into caller buffer |
| `lks_snapshot_deserialize` | strict borrowed-input parse to owned snapshot |

Views last until snapshot destruction. No item accessor exists. Required view
outputs clear to NULL/0 on failure; invalid pointers/indices return
INVALID_ARGUMENT. Format/serialize allocate nothing and leave undersized output
unchanged with BUFFER_TOO_SMALL. NULL buffers are INVALID_ARGUMENT. Parsing,
restore and capture clear required object outputs on every failure.

## Normative LS1 grammar (version 1)

```text
key = "LS1." namespace_hex "." ordinal_hex
namespace_hex = 2*K lowercase hexadecimal characters, 1 <= K <= UINT32_MAX
ordinal_hex = exactly 16 lowercase hexadecimal characters
hex alphabet = "0123456789abcdef"
```

Namespace bytes are rendered high nibble first, in supplied byte order. Ordinal
is unsigned 64-bit, most significant nibble first. No whitespace, alternate
case, extra separators, suffix or embedded NUL is allowed; the C string's final
NUL is not part of the key. Key length is `21 + 2*K` characters. Example:

```text
LS1.00112233445566778899aabbccddeeff.0000000000000000
LS1.00112233445566778899aabbccddeeff.0000000000000001
```

For canonical **same-version, same-namespace** keys, the common prefix is equal
and the fixed-width ordinal's first differing digit has unsigned numeric order.
Ordered lowercase hex digits preserve that order under bytewise ASCII strcmp.
Thus ordinal and strcmp comparison signs agree. Use an ASCII-byte-preserving
database collation; case-insensitive/linguistic collation is not the contract.
Semantic compare validates both operands; different namespaces return appended
DOMAIN_MISMATCH (enum value 12), with output 0. Cross-domain byte ordering carries
no live residence meaning. Validator accepts the whole uint64 ordinal domain;
formatter formats only rows actually present in its snapshot.

## Normative snapshot blob (version 1)

| Offset | Field |
|---|---|
| 0 | eight ASCII bytes `LKS4SNP1` |
| 8 | big-endian u32 flags, exactly zero |
| 12 | big-endian u32 namespace length K, nonzero |
| 16 | big-endian u64 row count N |
| 24 | K namespace bytes |
| 24+K onward | N repetitions: big-endian u64 association length, then exactly those bytes |

No padding or trailing bytes. Empty snapshot still requires namespace. Row
ordinals follow physical wire occurrence sequence. No revision, marker, items,
handles, topology or LS1 strings are serialized. Wire size is `24+K+8*N+A`.
Fields are encoded by shifts/byte writes, independent of native endian/structs.

Parser validates the complete structure before allocation: magic, flags,
nonempty namespace, available bytes, row count, each length, checked totals and
exact end. Row count must fit UINT64 and local SIZE_MAX, allocation dimensions
must fit SIZE_MAX, association lengths must fit SIZE_MAX. Impossible declared
counts/lengths cannot cause speculative huge allocations. A smaller-address-space
reader rejects unaddressable data without truncation. Capture also checks that
key and serialized sizes fit local size_t (including key's terminating NUL).
Malformed/unrepresentable input returns INVALID_ARGUMENT; OOM returns
OUT_OF_MEMORY and NULL output. Deserialize then serialize is byte-identical.

The blob provides structural interpretation, **not** checksum, authentication,
secrecy or detection of arbitrary association-byte bit flips. Applications needing
those properties must wrap it externally. No filesystem/database dependency.

## Restore and V3 migration

`lks_snapshot_restore_order(snapshot, resolve, context, &order)` invokes
`LksSnapshotResolveFn(association, size, uint64_t ordinal, context, &item)` once
per row. Nonempty snapshots require a resolver; successful calls require non-NULL
borrowed item. Duplicate item pointers are distinct legal occurrences. Resolver
failure propagates; earlier resolved items remain caller-owned. Resolve all items
into temporary pointer storage before any unpublished structural build.

Private bulk construction is O(N): choose ceil(N/128) blocks, distribute counts
as evenly as possible (multi-block 64..128; sole 1..128), allocate stable records,
thread blocks and directly construct balanced AVL parent/height/subtree metadata.
No per-item AVL search, rotations or relabel. For 129 rows the blocks are 65/64.
Index recursion is O(log blocks); traversal, cleanup and parsing are iterative.
Any failed build destroys partial owned structure and publishes nothing.

`LksV3Lk1ImportEntry { const char *key; void *item; }` with
`lks_order_import_v3_lk1(entries, count, &order)` requires non-NULL key/items,
strictly calls the existing published LK1 parser, sorts actual decoded Paths,
rejects duplicate coordinates with ALREADY_EXISTS and bulk builds fresh V4
order. Unsorted input and repeated item pointers at distinct coordinates are
legal. Zero count allows NULL entries. No Path/string is retained. Import cost
includes decoding and stable O(N log N) sorting, not just bulk construction.
V3 Path/LK1/Tree/OrderedTree implementations and golden vectors are unchanged.

## Example, integration and evidence

Build and run `layerkeysort_snapshot_example` from [examples/snapshot.c](../examples/snapshot.c).
It captures business IDs, prints LS1, serializes, changes/destroys source, reads
historical associations, loads and resolves a fresh order. It uses public API
only. Source/add_subdirectory, offline FetchContent, installed C/C++ and generated
amalgamation consumers exercise all 17 new functions. The header has 107 public
functions (59 V3 + 48 provisional V4).

`layerkeysort_v4_snapshot` tests LS1/wire fixed goldens and corruption/truncation,
currentness, managed no-comparator capture, callback guards/failure, historical
marker/refcount capacity, OOM cleanup, restoration block boundaries and 100k
rows, plus 10k import and a 10,001-step LK1 with SIZE_MAX terminal level.
`layerkeysort_v4_snapshot_smoke` screens export frequency and storage separately
from mutation; see [benchmark notes](../benchmarks/results/v4-preview3-smoke/README.md).
Evidence is workload/platform-specific, not a proof of realtime or overall
complexity. Snapshot storage is O(N+A+K), with geometric association capacity.

## Deferred scope

Preview.4: V4 Group/Batch integration and Group capture. Preview.5: final API/wire
compatibility freeze. Not implemented here: incremental export, DB/filesystem
adapters, application identity ownership, persistent live handles, public
rank/select, container locks, crypto, CRDT or final whole-V4 performance acceptance.
Snapshots export a historical sequence; they do not make full export free or
turn an ordinal into permanent object identity.
