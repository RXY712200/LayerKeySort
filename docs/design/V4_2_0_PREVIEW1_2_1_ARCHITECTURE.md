# Full Preview.1.2.1 architecture decision

## Actual baseline and cost model

Preview.1.2 at `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` is the primary implementation. Its stable Handle is independently allocated. A threaded B64 block stores resident pointers; the resident stores block and local position. Normal placement and removal shift a suffix, followed by position writes. Same-block moves repair the affected interval. A cross-block move prepares any new block before unlinking, places at its final destination, then repairs an underfull source using its final neighbors. Splits, redistributions and merges can move resident pointers and rewrite both position and owner. The minimum occupancy is half capacity except for the single-block state. Bulk restore produces newly allocated Handles, even tag spacing and a balanced index. Tags never enter LS1 or LKS4SNP1.

AVL refresh maintains height and subtree block counts through structural attach/detach and rotations. Managed search depends on the index and comparator callback; the cheap tagged compare path does not run that comparator. A cross-block comparison uses two tags if present, otherwise the height-guided AVL fallback. The previous matched public compare probe measured 3.891 ns at 100k on its Windows host. The previous subtree-count ablation was essentially flat, so this extension retains metadata and full propagation. Dropping height propagation without an independent structural proof would threaten current invariants.

## Measured bottleneck map

The existing deterministic native trace reports allocations, location assignments, touched index blocks, structural events and managed comparisons separately from foreground timing. These are work proxies; no hardware cache counters were captured. At 10k initial occurrences and 5k operations, the local B64 diagnostic reported 276,797 position assignments, 460 splits, 225 merges and 6,038 index touches in long moves, versus 44,888 assignments, 467 splits and 4,395 index touches at the endpoint. Counts include initial construction. In the managed random trace the diagnostic reported 190,112 comparator calls and 379,347 assignments. Endpoint and hotspot insertion still include resident allocations, so elapsed insertion cannot be assigned to array copying alone.

`index_blocks` retains its original AVL structural meaning and does not include tag stores. The structure probe reports cumulative label stores and repairs, the maximum number of stores on one attachment, block occupancy, current zero-tag population, longest cluster, and observed comparison routes. Per-operation tail timing includes timer resolution and ordinary allocator/runner interference. Profile families and repeated hotspot/long replay expose structural and locality tradeoffs independently of the read-heavy control.

## Screened designs

| Direction | Actual screen and tradeoff |
| --- | --- |
| Boundary transfer | Replace two-array scratch staging with a direct copy of only the boundary residents and, when needed, one overlapping shift of retained right residents. Keep all existing location repairs because the source may already have stale local indexes after removal. Keep the same B64 burn-in, split/merge thresholds, AVL attach/detach and tag policy. |
| B32 and B128 | Change only the private even capacity. The half-full invariant, transient overflow slot and bulk calculations derive from it. B32 shortens the suffix while increasing the number of block objects and attachment operations. B128 increases shifted runs and trades fewer index nodes against longer per-block work. |
| Tag recovery | Preserve bounded 64-block relabel and test 100k concentration, shrink and overlapping recovery of two deliberately untagged blocks. A fallback pair remains correct even across multiple zeros; the tagged subsequence must remain strictly increasing. A successful overlapping attachment can restore the tags; every scenario need not reclaim all zero labels. |
| AVL and Handle representation | Retain the current index and stable separately allocated Handles. Prior count-removal and sparse slab retention analyses gave no basis for a new production variant here. This study has neither a measured lifetime-safe pool nor allocator/cache causal isolation. |

In the transfer prototype, concatenation preserves left then right order. If the target left count grows, a prefix of the right moves left; if it shrinks, a suffix of the left becomes the right prefix. Remaining occupancy matches the accepted algorithm. The two arrays are independently allocated, so boundary copies are disjoint. Repair can run after allocation-free source unlinking and requires no new fallible operation. For both capacity variants an independently checked resident sequence and existing mutation model verify the invariants. All prototypes stay in the research workspace. No third-party code was imported.

## Applied diagnostic change

At `c977b0fd14a501d769b9783bfdcc61266bd1b5ae`, `v4_check_node` records the last nonzero tag during the in-order AVL traversal, which already cross-checks the threaded order. Zero leaves the prior value in effect. The check rejects equal or descending nonzero tags across a zero block. It is compiled only in diagnostic builds and adds one word to the checker state, with no new per-block storage or traversal. The regression restores modified tags and the original saturation fixture still verifies AVL comparison. No production version or operational algorithm change is selected.

The [protocol](../../benchmarks/research/full-v4.2.0-preview.1.2.1/PROTOCOL.md) and [decision report](../V4_2_0_PREVIEW1_2_1.md) bind the decisions to measured results.
