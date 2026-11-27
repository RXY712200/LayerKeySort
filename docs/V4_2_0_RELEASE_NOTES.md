# LayerKeySort v4.2.0

Published Stable/Latest: [Full v4.2.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.2.0), released 2026-11-27. [C17 source ZIP](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-amalgamation.zip) · [SHA-256 manifest](https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-SHA256SUMS.txt). Released source: `v4.2.0` / `f751e1ee2afd682874281d6dc495969f9eca49eb`. [Historical 4.1 release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0).

Full v4.2.0 is a compatible C17 update for dynamic ordering, stable pointer-array sorting, immutable ordered batches and historical snapshots. It retains the 64-function / 14-type source API, LS1 and LKS4SNP1 contracts, stable occurrence Handles and caller-owned payloads.

## Changes

- Ordinary mutation updates only affected record locations.
- Cross-block precedence uses bounded labels, with height/parent AVL fallback for untagged blocks; local structural work stays bounded by block policy and balanced-index maintenance.
- Snapshot wire uses portable fixed-width conversion, keeping exact bytes and validation before allocation.
- Offset-one wire and restored mixed-tag regressions strengthen the retained implementation's coverage.
- Complete bilingual user and implementation guides explain practical workflows, shared-item mutation, concurrency, failure recovery, distribution and compatibility.

## Evidence

Archived matched Windows/Linux runs of the unchanged operational implementation show median Full/4.1 elapsed ratios 0.629106/0.640047 overall and 0.197718/0.219173 compare-heavy; 45/45 wins per host. These describe the tested controls and workloads. Native tag-range OM wins several update-heavy cases. The tested endpoint flat-history composition is faster in its lifecycle comparison; bulk Treap restore remains unmeasured. No fresh Stable benchmark is claimed. See [complete evidence](https://github.com/RXY712200/LayerKeySort/blob/f751e1ee2afd682874281d6dc495969f9eca49eb/docs/FULL_RC1_EVIDENCE.md).

Validation covers strict C17, API/wire fixtures, failure injection, seven examples and C/C++ source/install/amalgamation consumers. The final integrated source and rebuilt assets are bound to the exact commit below; CI run links are supplied in the publication manifest.

## Source and assets

Acquire released source at tag `v4.2.0` or the exact release commit below. The prepared assets are `LayerKeySort-4.2.0-amalgamation.zip` and `LayerKeySort-4.2.0-SHA256SUMS.txt`. The ZIP contains `layerkeysort.h`, `layerkeysort.c`, `example.c`, `LICENSE`, `README.txt`; compile the implementation as C17 and link C/C++ clients. The reviewed preparation ZIP SHA-256 is `1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb`. This checksum was confirmed by rebuilding and validating assets from the exact integrated commit below.

## Practical limits

Per-occurrence allocation, block slack, callbacks, O(N+A) history export, retained snapshots and restore scratch remain real costs. Applications retain identity, transactions, callback side effects and synchronization. No universal speed advantage, hard end-to-end latency, general binary ABI, database or CRDT is supplied. Mini and Workbench are unchanged; downstream dependency adoption is separate.

## Exact source and asset checksums

Source commit: [`f751e1ee2afd682874281d6dc495969f9eca49eb`](https://github.com/RXY712200/LayerKeySort/commit/f751e1ee2afd682874281d6dc495969f9eca49eb).

```text
1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb  LayerKeySort-4.2.0-amalgamation.zip
36e06a53ad41f0a01ee0435e5c81ec81b8611b61df2351f5cf322ea1acd6f68e  LayerKeySort-4.2.0-SHA256SUMS.txt
```

## Final-target validation

All six C17 configurations passed CTest 39/39 at the exact release target: MSVC x64/Win32, Linux GCC, Linux Clang, Linux Clang ASan/UBSan, and macOS AppleClang. Mini, Workbench, measurement-evidence checks and Pages also succeeded.

- [45832456582](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456582)
- [45832456787](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456787)
- [45832455183](https://github.com/RXY712200/LayerKeySort/actions/runs/45832455183)
- [45832456709](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456709)
- [45832456797](https://github.com/RXY712200/LayerKeySort/actions/runs/45832456797)
