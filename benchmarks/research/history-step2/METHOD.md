# Step 2 method and representation review

Scope: Full private Snapshot/History, based on accepted Step 1
`19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`. No adoption, merge or release.

## Representation and correctness argument

Associations already occupy one contiguous owned byte sequence. Store each row's
cumulative end `E[i]`, with `E[-1]=0`. A row's offset is `E[i-1]`, and length is
`E[i]-E[i-1]`. Appending validates `length <= limit-total` before addition and
copies the returned callback span immediately. Thus ends are nondecreasing;
repeated ends represent zero-length associations. Every successful capture/load
initializes all ends before publication. A partial result is only destroyed,
never inspected. Removing redundant row zero-fill is safe for that reason.

Header, fixed row vector and namespace share one allocation. The header contains
`size_t`, so its size/alignment places the following one-`size_t` rows correctly;
namespace bytes follow the row vector and require no stronger alignment. Check
`domain <= SIZE_MAX-sizeof(header)` and
`count <= (SIZE_MAX-sizeof(header)-domain)/sizeof(row)` before allocation.
Existing wire, count, namespace and key-length arithmetic checks remain.

Association bytes retain geometric growth while callbacks execute, then shrink
to exact useful size before provenance is committed. Callback spans are never
retained or requested a second time. A nonzero capacity implies nonzero useful
size, so final shrinking never invokes `realloc(ptr,0)`. Shrink OOM destroys the
unpublished result and leaves source revision, marker and prior snapshots intact.
Successful views remain stable until snapshot destruction. No shared row/data
storage, inter-snapshot aliasing or new refcount is introduced. Existing source
marker reference bookkeeping is unchanged.

Deserialization still validates the complete wire image before allocation. It
fills cumulative ends while copying associations, and retains no input pointer.
Serialization reconstructs exactly the same u64 lengths and bytes. LS1 keys and
LKS4SNP1 version/flags/endian rules are unchanged. Resolve still runs exactly once
in ordinal order for every row before any resident/core construction. The
temporary pointer vector is retained, as are Step 1's separately allocated
24-byte x64 residents. This is not snapshot compression or incremental export.

## Cost model (x64 measurements must confirm actual sizes)

Let N be rows, A useful association bytes, D namespace bytes and C geometric
association capacity. Old owned bytes: `72+D+16N+C`; new: `72+D+8N+A`.
The shared source marker is additional and is counted once, not per snapshot.
Grouped allocation reduces three initial allocations to one. Loaded snapshots
already had exact association capacity; their saving is the row vector plus
two fewer allocations. Empty snapshots have no row or association allocation.

Logical callback payload copying remains A bytes per successful snapshot.
Allocator-internal copying is not measured: a real CRT realloc may move or shrink
in place. The final shrink adds one fallible allocation attempt when capacity
exceeds useful size. Requested allocation traffic may **increase** by roughly
`A-8N` versus baseline when trimming occurs; traffic is not bytes actually copied.
Published capacity slack is eliminated, but transient growth slack and allocator
internal transient memory remain. OS working set need not follow requested bytes.

Rejected directions, without claiming benchmark results for unimplemented code:

- callback pre-count/replay violates the one-invocation/immediate-copy contract;
- borrowed wire views or caller associations violate independent ownership;
- shared/COW histories add lifetime machinery without an established need;
- per-row byte allocations replace amortized contiguous growth with O(N)
  allocations; chunk staging/flattening adds storage/copy complexity;
- width-adaptive or compressed offsets add lookup/overflow branches; one native
  end word already removes half of row metadata without changing the format;
- streaming restore construction changes resolve-first failure/callback ordering.

## Controlled comparison

A: original main/Stable production sources (the immutable million-scale research
checkout has identical production content). B: accepted Step 1. C: this candidate.
Primary comparison is **B versus C**, with the same driver source, flags, case,
callback bytes, namespace, retention, timing boundaries and oracle. A is sampled
for matching eight-byte histories and the original history driver to decompose
the previously observed second-capture regression. Never mix different drivers
or the old published captures into fresh paired timing results.

The unchanged Step 1 drivers/models are copied into this new directory; their
original files/evidence remain untouched. New `history.c` uses a rotation model
and independently generated association bytes, not Full ranks/decoded output.
It checks every row, namespace and complete wire image. Callback scratch is
reused, so deferred copying would fail. It verifies exact restored item order,
reverse traversal and fresh handles while the original handles are still live.
It then corrupts/frees the caller wire and destroys the source and caller items
before validating remaining historical data. No expired handle is dereferenced.

Planned cases: 100k/1m, with 3m when admitted; absent callback, zero, 1, 8,
variable 0..128, fixed256 and variable 0..2048 byte spans; namespace 1/16/4096
bytes; low retention2 and higher retention4/8/16. First/second unchanged captures
are separate rows; capture3 follows a 10%-collection rotation when retention>2.
Each process includes serialize, load, three resolve/build/destroy cycles,
partial history destruction and source-before-history teardown. Historical keys
have exact independent expected strings; aggregate key bytes are estimates of
formatting every row, not actual retained buffers. Three malformed wire probes
are additional to the existing regression/fuzz/OOM suites.

Production timings, allocation wrapping and private structural/storage probes
are separate fresh processes. The same native CRT realloc is used in both
wrapped variants. Wrapper headers and diagnostic metadata perturb memory; do not
present instrumented timings as production timings. The callback-only phase
uses an indirect call through a volatile function pointer to prevent inlining;
it is a separate calibration and is **not subtracted** from capture. Key timing
includes validation/expected-string checks and is labeled an end-to-end probe.
Oracle scans/preparation are outside measured library phases. Whole capture,
restore and destructor timings include their real callbacks and allocation work.

`collect.py` fixes the case/repetition matrix, alternates variant order and runs
one child at a time. All modes use a baseline-pessimistic simultaneous-memory
envelope, at most 40% of currently available physical memory and at most3GiB,
while preserving3GiB. The driver retains a2GiB runtime guard. Admission skips
and budget aborts are never passes. No unrelated applications are terminated,
no memory guard is relaxed and high retention is not run just to advertise scale.
Windows background activity is not controlled; medians/ranges are engineering
observations rather than universal guarantees or significance tests.

`analyze.py` recomputes every inclusive phase table, min/median/max and summed
timed-phase costs. It verifies cross-variant/mode/repeat oracle fingerprints and
tracked teardown. The sum excludes oracle scans, generation and caller-buffer
allocation; it is not whole-process elapsed time. Full process time is retained
separately in metadata. Live guard cases repeat random insertion, duplicate
locate and append-origin local/broad moves/churn; long and severe-shrink drivers
are unchanged Step 1 workloads. Allocation counts and memory must be checked
even when an ordinary timing difference is small.

## Final measurement provenance and limits

Windows initially could not meet the 3 GiB benchmark reserve. Unit/regression
builds passed, while standalone research ASan smoke tests initially exited on
the unchanged 2 GiB guard. The developer-environment rerun passed 20/20. An
isolated Ubuntu research runner supplied a second, separate matched dataset.
After the owner closed applications, available Windows RAM rose to about8.1GiB;
all planned 100k/1m/3m cases then passed admission. A bounded two-snapshot,
eight-byte case was also admitted at10m; the variable-length10m envelope still
exceeded the unchanged3GiB cap and was skipped. No unrelated process was killed.

Only one production refinement followed the initial representation: sequential
serialization/restore now carry the preceding end forward. The initially
observed Linux100k variable serialization slowdown prompted this change, but
cross-run host/ordering differences prevent attributing a timing change solely
to it. The original implementation/carry-refinement data remain in evidence.

A measurement bug was subsequently found: rotating by case index and reversing
on even repetitions could cancel within-case alternation. Final measurements
use `repeat-rotation-v2`: variant order rotates by repetition, and live guard
cases use four production repeats. Two-variant history cases have three repeats
at100k/1m and two at3m. Three-variant cases rotate their three-source order.
Earlier order schedules are preserved as pre-correction evidence, not mixed into
final medians. Final Windows upper-scale repetitions reverse the source order.

The Windows follow-up prototype retained the original skip `reason` in completed
retry records. There, `retry_of` identifies the earlier attempt; current status,
new memory readings/UTC, return code and raw output determine the retry outcome.
Those raw records are preserved verbatim. The final follow-up script labels the
old reason explicitly as `original_skip_reason`; final-order upper runs had no
pending base-case retries. No outcome or measurement was changed retroactively.

Fresh Windows measurements investigate the prior second-capture regression.
The capture iterator directly reads each record's item; it does not use the
public handle owner lookup. Therefore extra owner derivation is not a supported
explanation for that regression. Allocator/layout/cache/scheduling contributions
are not isolated. Results must distinguish the original ID8 legacy driver from
the callback-generated-byte history driver, and Windows from Linux. Linux runner
CPU models differ between rounds; only same-round A/B/C comparisons are fair.

Namespaces are fixed within each experiment to control size. Keys are validated
within one snapshot, not used to infer cross-state order when a namespace is
reused. Applications remain responsible for meaningful namespace uniqueness.
The generic representation intentionally retains one end word per row even when
all associations are empty. Full copies remain O(N+A); no delta export, dedup,
callback-purity assumption or universal performance guarantee is introduced.
