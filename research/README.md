# Phase 2: one finite-family slack prototype

Research only. `-DLKS_RESEARCH_SLACK=ON` selects this algorithm for both the
production-timed and diagnostic libraries. OFF retains the v3.1 algorithm.
No public API, Path, LK1, version, removal, or workload/RNG change is made.

## Capacity model and proof

For a non-ZERO reference, consider depths D=1..min(reference depth,6).
Direction, all absolute levels, and the first D-min(D,2) slots are fixed to
the reference's corresponding steps. Only the final min(D,2) slots vary.
Their big-endian base-65536 digits define ranks 0..65536^min(D,2)-1.
If the root is variable and negative, its digit is 65535-root_slot.
Each family is finite, enumerable, and injective; within it public Path order
equals integer rank order. Levels are fixed, so only the first differing slot
matters; negative-root reversal is exactly the digit transformation.

For actual unchanged exterior bounds L,R, binary search computes lower = first
rank whose Path is strictly greater than L, and upper = first rank whose Path
is greater than or equal to R. Absent bounds use 0/the family size. Parents,
descendants, different levels, other prefixes and directions are compared by
the full Path rules, not formatted text. The usable half-open interval is
[lower,upper); capacity C=upper-lower, or zero for an inverted/empty interval.
This is the exact capacity of this constrained family, **not** arbitrary Path
space. Family membership is also determined exactly by projection plus equality.

For n selected residents, population P=n+1 includes the pending item, whether
old resident Paths belong to the family or not. Diagnostic membership counting
answers how many family coordinates are occupied before repair. After repair
exactly P are occupied, leaving C-P free. No unchanged exterior resident can
occupy the open interval because the selected nodes form a contiguous ordered
range. This is eviction into a new family, not subtraction of unrelated slots.

Require stride s=floor(C/(P+1)) >=16 before allocating any replacement Path.
Assign sorted ranks lower+s*(i+1)-1, i=0..P-1. The first gap has s-1 unused
coordinates, every internal gap s-1, and the last at least s. Thus every gap
has at least 15 enumerable spare coordinates immediately after commit. The
product is bounded by C<=2^32; uint64_t rank arithmetic avoids 32-bit overflow.
Levels are copied without arithmetic. A same-depth repair is justified by this
reserve, not equal-depth acceptance alone.

The reserve is **not** an amortized guarantee, a forecast of hotspot demand,
or a promise that future direct candidates stay in this family. A fixed level
budget and stateless reserve can still induce repeated costly repairs. Testing
that distinction is the purpose of this prototype.

## Multiscale choice and transaction

Use the existing contiguous logical windows 8,16,32,...,whole collection.
At each scale try families in increasing depth from the predecessor reference,
then the successor reference. Prepare only when a family meets the reserve.
Otherwise expand without speculative replacement-Path generation. Scratch
arrays and range inspection still incur real work. If no family works at full
range, retain the baseline sparse bulk fallback and its shallower-depth check.
Do not change the ordinary repair trigger or endpoint insertion policy.

Prepare all replacement Paths, validate strict ordering and unchanged bounds,
then allocate the new node. Only afterward replace keys and link the node with
the existing allocation-free AVL commit. Relative resident ranks remain fixed.
No persistent slack metadata is introduced: recompute on the next insertion.
Remove neither allocates nor changes survivor coordinates.

## Reproduction

Configure two separate build directories with identical compiler/options,
`LKS_BUILD_BENCHMARKS=ON`, and selector OFF/ON. Run CTest in each. Then:

```text
python research/run_slack_screen.py --baseline build/theory-baseline --prototype build/theory-slack --output benchmarks/results
```

The driver imports the frozen `benchmarks/run_workloads.py` pairing/oracles,
selects the 20 requested screening scenarios, and refuses evidence overwrite.
Use `--full` only if screening warrants the frozen 52-scenario campaign.
It records exact source/binary hashes; benchmark outputs are not CI workloads.
Private rewritten-byte counters sum old resident Path objects and allocated
storage (including spare capacity), not serialized bytes or new Path sizes.
