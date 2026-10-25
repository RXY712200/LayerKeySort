# Independent combined source review

Reviewed 2026-10-25 against actual source, not only earlier completion reports.
A: `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95` (production identical to Stable).
B: `19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`.
C production: `291305dca03d2d7599c88b6e6cebf2cc426e1790`.
The complete A..C production diff is seven private files. No public declaration,
wire fixture, production source manifest, allocator or managed comparator logic
changed. Step 3 initially needs no corrective production change; measurements
and regression results determine the final recommendation separately.

## Resident and block ownership

Read the entire diff in `src/lks_order_internal.h`, `order.c`, `order_bulk.c`,
and adjacent rotations, detach, placement, location repair, remove, move,
cursor and managed-facade paths.

- A live resident always belongs to exactly one live block. Blocks cannot be
  transferred between containers. The owner field is initialized in both
  `v4_new_block(order)` and `lks_order_bulk_build` before publication.
- Resident allocation remains individual, so stable addresses and immediate
  reclamation are unchanged. Physical AVL operations transplant blocks, not
  resident arrays; no new pointer copying or order-maintenance algorithm exists.
- Split/merge/redistribution call `v4_locations` for surviving arrays before
  retiring an emptied block. Cross-block moves place the existing record before
  source repair; no callback or allocation observes the transient state.
- Foreign-live-handle checks derive owner through `handle->block->owner`.
  NULL and current same-source callback reentry behavior match the baseline.
  Expired handles remain invalid C lifetime usage; this is not a dangling-pointer
  detector. No test intentionally dereferences an expired handle.
- Required spare-block allocation still precedes unlink/commit. A failed
  allocation frees only unpublished records/spares and does not alter a source.
- Managed lookup reads resident items directly for comparisons; its algorithm,
  comparator-call count and equal-run upper bound are unchanged. Locate may call
  public previous/last helpers, so an extra ownership indirection is a plausible
  cost path, not proof of the cause of a measured slowdown.
- Public item/traversal helpers now derive ownership through a block. This adds
  an indirection while resident working storage shrinks. Mixed timing is an
  expected tradeoff to measure rather than assume away.
- Snapshot capture's iterator reads `record->item` directly, not the public
  owner-derived getter. Prior capture regressions cannot simply be attributed
  to an added getter lookup.

## Snapshot row invariant and grouped allocation

Read complete `src/snapshot.c`, `snapshot_wire.c`, `snapshot_restore.c`,
`lks_snapshot_internal.h`, unchanged `snapshot_key.c` and shared Group capture.

- For each fully prepared row i, end[i] is cumulative owned association bytes.
  end[i] >= end[i-1]; repeated ends represent empty spans. Row zero starts at
  zero. Published rows are immutable, so independent access remains constant-work.
- The grouped allocation contains the header, N rows, then namespace bytes.
  A struct's alignment is at least each member's alignment and its size is a
  multiple of its own alignment. The header contains size_t fields; the row is
  one size_t. Thus `s+1` is suitably aligned. The added layout probe independently
  checks the condition and runtime alignment against each native baseline.
- Product/sum checks precede `sizeof(*s)+N*sizeof(row)+D`. Existing wire/key
  reservations remain, so 24+D+8N and key lengths cannot wrap. Narrow size_t
  platforms reject unrepresentable counts/lengths before allocation or casts.
- Only the header is cleared. Capture/deserialization fill every row before
  publication. Failure cleanup frees owned allocations and does not read rows.
- `snap_append` bounds size against remaining wire capacity before addition;
  capacity doubling checks the limit before multiplication. The association
  length grows monotonically. Capacity can differ from size only after a
  nonempty append, so final trim cannot call realloc(pointer,0).
- Each association callback runs once per occurrence in ordinal order. Its
  returned borrowed bytes are copied immediately, before the next callback.
  No pre-count/replay/purity assumption was introduced. Descriptor copying and
  same-source callback guards are unchanged.
- Final trim is a fallible prepare operation. On realloc failure, the old buffer
  remains owned by the unpublished snapshot and cleanup frees it. Source marker
  retain/create occurs only after rows and trim succeed. Existing source,
  snapshots, revision, handles and cursors remain unchanged on failure.
- Exact published capacity describes Full requested storage, not allocator usable
  size or OS RSS. A native realloc may copy bytes or temporarily hold old/new
  storage; requested API accounting does not measure those internal costs.

## Provenance, wire and restoration

- Source markers remain separate atomic reference-counted allocations. Source
  destruction releases one reference, independently retained snapshots release
  theirs; loaded snapshots have no marker. Currentness still compares source
  identity and revision, not equal bytes or recycled source addresses.
- MSVC interlocked and C17 atomic paths are unchanged. Snapshot reads require an
  externally protected lifetime; live mutation remains externally serialized.
  No stronger concurrency guarantee is inferred from marker atomics.
- LS1 and LKS4SNP1 output uses unchanged explicit byte encoding. Native row layout
  is never serialized. Replacing size with `end-offset` preserves every length
  word and payload. Deserialization validates the complete input and cumulative
  sizes before allocating, then constructs the same owned spans.
- Resolver callbacks finish before resident/core bulk construction. The N-pointer
  scratch is retained. Failed resolution/build never frees caller-owned items.
  Fresh handles are constructed for the new explicit order; historical handles
  are neither serialized nor resurrected.
- V3 LK1 import remains unchanged: parse, stable sort, duplicate-coordinate
  rejection, bulk build, temporary cleanup. Public fixtures/API hashes remain
  a separate executable gate, not just this source argument.
- Immutable Group/Batch own their arrays and borrow caller items. Their capture
  adapter shares the Snapshot preparation routine and its marker transaction;
  neither step changes sorting/merge stability or source array ownership.

## Complexity and maintenance disposition

No algorithm or policy redesign is required by this review. The public frozen
structural complexity statements continue to exclude allocator/callback cost.
Capture remains O(N + association bytes), independent histories remain full
copies, resolve-first scratch remains O(N), and Resident allocation count is
unchanged. Restore residents/blocks remain those of accepted Step 1.

The changes remove redundant metadata and tie storage with one lifetime together.
They introduce one additional fallible final trim and ownership indirection;
existing failpoint and movement invariants must remain regression requirements.
No extra production abstraction, runtime policy selector or test-only ABI exists.
No source defect was established by this static review. That conclusion alone
is insufficient for adoption: fresh cross-platform tests, OOM campaigns, memory
and adverse-performance measurements are documented in the final report.

## Bounded validation-tool correction

The final recommendation retains the same production code. Step 3 additionally
corrects tools/validate_distribution.py to always configure Debug, matching its
build/install commands when Windows implicitly selects a single-config generator.
The initial failure is preserved; default Ninja and explicit MinGW/MSVC consumer
paths pass after correction. This is not an API/algorithm/package-content change.
