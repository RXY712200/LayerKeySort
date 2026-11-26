# LayerKeySort v4.2.0 — proposed release notes

[简体中文](V4_2_0_RELEASE_NOTES.zh-CN.md)

Preparation only. Intended release name: **LayerKeySort v4.2.0**. Intended annotated tag: `v4.2.0`, at the separately approved final integrated SHA. Intended GitHub flags: draft=false, prerelease=false, Latest=true. No publication date or actual release is assigned here.

Full v4.2.0 is a compatible C17 update for dynamic ordering, stable pointer-array sorting, immutable ordered batches and historical snapshots. It retains the 64-function / 14-type source API, LS1 and LKS4SNP1 contracts, stable occurrence Handles and caller-owned payloads.

## Changes

- Ordinary mutation updates only affected record locations.
- Cross-block precedence uses bounded labels, with height/parent AVL fallback for untagged blocks; local structural work stays bounded by block policy and balanced-index maintenance.
- Snapshot wire uses portable fixed-width conversion, keeping exact bytes and validation before allocation.
- Offset-one wire and restored mixed-tag regressions strengthen the retained implementation's coverage.
- Complete bilingual user and implementation guides explain practical workflows, shared-item mutation, concurrency, failure recovery, distribution and compatibility.

## Evidence

Archived matched Windows/Linux runs of the unchanged operational implementation show median Full/4.1 elapsed ratios 0.629106/0.640047 overall and 0.197718/0.219173 compare-heavy; 45/45 wins per host. These describe the tested controls and workloads. Native tag-range OM wins several update-heavy cases. The tested endpoint flat-history composition is faster in its lifecycle comparison; bulk Treap restore remains unmeasured. No fresh Stable benchmark is claimed. See [complete evidence](FULL_RC1_EVIDENCE.md).

Strict C17, API/wire fixtures, failure injection, seven examples and C/C++ source/install/amalgamation consumers form the validation package. Final exact-head platform results are recorded in the preparation delivery; final integrated assets must be rebuilt and verified before publication.

## Source and assets

Acquire source from the approved exact commit; a future published tag may be used only after it exists. The planned assets are `LayerKeySort-4.2.0-amalgamation.zip` and `LayerKeySort-4.2.0-SHA256SUMS.txt`. The ZIP contains `layerkeysort.h`, `layerkeysort.c`, `example.c`, `LICENSE`, `README.txt`; compile the implementation as C17 and link C/C++ clients. The reviewed preparation ZIP SHA-256 is `1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb`. Reconfirm it against the final integrated source rather than copying an unverified value to a Release.

## Practical limits

Per-occurrence allocation, block slack, callbacks, O(N+A) history export, retained snapshots and restore scratch remain real costs. Applications retain identity, transactions, callback side effects and synchronization. No universal speed advantage, hard end-to-end latency, general binary ABI, database or CRDT is supplied. Mini and Workbench are unchanged; downstream dependency adoption is separate.
