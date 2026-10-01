# V3 Preview.2 final endpoint-policy audit

The original `04cdd0c` candidate remains recorded in
`v3-preview2-comparison.csv`, `v3-preview2-diagnostics.txt`,
`v3-preview2-holdout.csv`, and `v3-preview2-metadata.md`. It uses endpoint
direct-depth eight. The final candidate changes only that private endpoint
allowance to 16; the new focused OOM test, diagnostic `path_max` field, and
documentation do not affect the timed library algorithm.

All measurements were made on Windows 11, AMD Ryzen 9 9955HX, with MSYS2
UCRT64 GCC 16.2.0, C17, strict `-Wall -Wextra -Wpedantic -Werror` warnings,
and effective `-O3 -DNDEBUG` optimization for timed binaries. The original
candidate A uses its CMake Release binary; experiment binaries and the final
candidate were compiled from their respective sources with the same effective
optimization. The matched `v3-preview2-policy-audit.csv` should be used for
A/B/C/D timing comparisons; `v3-preview2-final-comparison.csv` confirms the
final source independently and is not a byte-for-byte replication.

`v3-preview2-policy-audit.csv` compares: A=original depth eight,
B16=depth 16, C=no endpoint depth trigger, Dlog=logarithmic size-dependent
endpoint limit. A separate experiment tested depth 10 and 12. The
`v3-preview2-policy-cross.csv` file compares A8/B16 for primary seed
`0x91A30D47` and holdout seed `0xDEADBEEF`. The final timing CSV includes
both seeds for non-monotone workloads. Timing rows use one warmup and three
measured repetitions, except all ascending 1m rows use two repetitions.
The first three-repetition primary alternating comparison varied by about
3%, so it was repeated with nine measured runs per version: A8/B16 medians
were 72.991/73.080 ms (primary) and 71.409/71.845 ms (holdout). This does
not support a material alternating regression.

`v3-preview2-final-diagnostics.txt` uses a separate diagnostic build at `-O2`
with `LKS_ENABLE_ALLOC_DIAGNOSTICS`, `LKS_BENCH_DIAGNOSTICS`, and
`LKS_BENCH_STAGE5_DIAGNOSTICS`. The new `path_max` line gives maximum final
Path depth. Diagnostic timing values are not compared to timing CSV rows.
`relabel` and `placement` retain the field definitions in
`benchmarks/README.md`. Cumulative requested bytes count successful library
allocation requests; peak live bytes are the diagnostic allocator's peak.
`v3-preview2-policy-diagnostics.txt` preserves A8/C/Dlog/E12 and final B16
depth, allocation, repair, comparator, rotation, and AVL-height records at
500k and 1m.
`v3-preview2-baseline-extra.csv` adds stable V2 and Preview.1 measurements
for equal 300k, ascending 500k, and descending 100k/300k. The 500k rows use
one warmup and two measured runs; other rows use one warmup and three.

The exact depth-nine event was observed in an instrumented detached checkout
of A. Insertion 261,579 in ascending and all-equal append produced
`0RUc/RUc/RUc/RUc/RUc/RUc/RUc/RUc/DEq`, a valid depth-nine Path, and
successfully allocated it. The predecessor existed, the successor did not,
`append_run=261577`, `prepend_run=0`, and endpoint step was one. Only the
depth-eight policy called relabel; there was no coordinate gap or allocation
failure. The same candidate would be valid for direct insertion. A repeated
the depth-nine policy event at insertions 502,918, 742,003, and 981,088,
showing a roughly fixed-size recurring cliff rather than geometrically
separated events. The first depth-17 candidate in B16 occurs near insertion
523,723. Prepend uses negative-root level carry and had no relabel through
500k.

These measurements describe this machine and these deterministic workloads.
The endpoint rule is private and provisional. A full-range relabel can still
touch `n` nodes; no worst-case or formal amortized bound for complete managed
insertion is claimed. The no-trigger variant's 1m maximum Path depth of 31
and peak 288,763,760 live bytes are the measured reason it was rejected.
