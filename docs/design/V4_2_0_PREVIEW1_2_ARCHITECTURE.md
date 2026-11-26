# Preview.1.2 bounded block tags

The accepted comparison at `22544cf283694dd27cef203caba9cf1838845947` finds relative block positions by height-guided AVL ancestry. The candidate at `61e9290ec5ba47b9092bcb8ae7d61f13104ea839` keeps that mechanism and adds one private unsigned 64-bit tag to each block. A pair of tagged blocks compares by unsigned tag. Same-block comparisons use resident positions. Pairs involving an untagged block use the existing AVL comparison. The AVL also continues to maintain managed search and structural order.

## Maintenance and invariants

The nonzero tags increase strictly along the threaded block list. Zero means untagged. `v4_tag_attach` examines successively wider contiguous ranges around the newly attached block, up to a width of 64. It selects a range only with tagged exterior bounds or endpoint sentinels and sufficient unsigned spacing. Reassignment puts all selected tags strictly between those bounds, so the unchanged tags still give the correct order. At an endpoint the stride is capped to leave space for subsequent growth. The arithmetic uses subtraction before division and bounds each assigned value by the upper exterior tag.

A failed local label assignment leaves the new block untagged. A subsequent successful overlapping repair may tag it. No policy guarantees that every zero will be reclaimed. An attachment examines at most 64 range widths and writes at most 64 tags. Removing a block leaves the strict ordering of the remaining tags intact. AVL rotations preserve threaded order. Resident rearrangements use existing position maintenance. `lks_order_bulk_build` initializes widely spaced tags during bulk construction; source, snapshots and persisted formats carry no tags.

Tagged cross-block comparisons require constant work. Fallback is O(log B) for B blocks. This gives a bounded additional cost per block attachment, with finite-label saturation handled by fallback. It adds one word per block and performs no allocation or callback in repair. Preparation, failure publication, revision, no-op rules, live addresses and ownership remain unchanged.

## Candidate selection

Dietz and Sleator's 1988 technical report distinguishes the cost of maintaining order labels from comparing them. Bender et al. (2002) use indirection and density-based label maintenance. The archived MIT j-hui tag-range and list-range implementations maintain labels over their own entries; this independently implemented C candidate applies bounded redistribution to Full's existing resident blocks and retains a structural fallback. Their full algorithmic guarantees do not transfer to this bounded design.

The previous midpoint-only prototype exhausted its focused gap near insertion 1,057. This extension measures real redistribution. Removing subtree block counts was screened as an ablation: release-library timing was effectively flat and the invariant tests still read the field. Counts remain. The [prespecified protocol](../../benchmarks/research/full-v4.2.0-preview.1.2/PROTOCOL.md) and [stage report](../V4_2_0_PREVIEW1_2.md) record the evidence and thresholds.
