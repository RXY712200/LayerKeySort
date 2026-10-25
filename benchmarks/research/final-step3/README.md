# Full final integrated Step 3 acceptance

This isolated assignment starts at accepted Step 2
`291305dca03d2d7599c88b6e6cebf2cc426e1790`, including accepted Step 1
`19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`.

**Engineering recommendation: A — READY FOR COMPATIBLE ADOPTION.** This is
not final project PASS or merge/release authority. Both accepted optimizations
are retained; no corrective production change was justified. One narrow
Windows distribution-validation configuration bug was corrected.

Read [REPORT.md](REPORT.md) for the complete English acceptance report, including
all paired numeric tables and explicit regressions. [SOURCE_REVIEW.md](SOURCE_REVIEW.md)
records direct source analysis. [VALIDATION.md](VALIDATION.md) gives commands;
[FILES.txt](FILES.txt) and [INTEGRATED_FILES.txt](INTEGRATED_FILES.txt) give the
Step 3 and combined inventories. [SHA256SUMS.txt](SHA256SUMS.txt) identifies all
integrated changed Git blobs except itself.

Fresh evidence:1,548 completed native processes across Windows/Linux,36 safely
skipped attempts,11,802 matching checkpoint fingerprints, full byte/order/restore
oracles and actual cross-platform CI. Performance is mixed: Linux lookup,
large-domain keys, some serialization/movement/destruction phases and wide-span
request traffic remain explicit accepted costs. No universal speedup is claimed.

A is exact original main `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`;
B is accepted Step 1; C includes both steps. All three use the SAME existing
Step 2 workload drivers/oracles (unchanged), built against their own private
headers. Build every binary before timing. No historical report or data changes.

```sh
cmake -S benchmarks/research/final-step3 -B ../step3-a -DFULL_ROOT=/absolute/baseline-a -DCMAKE_BUILD_TYPE=Release '-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror'
cmake --build ../step3-a --parallel 4
ctest --test-dir ../step3-a --output-on-failure
```

Repeat with separate B/C roots and directories, then:

```sh
python benchmarks/research/final-step3/collect.py ../step3-a ../step3-b ../step3-c ../step3-results
python benchmarks/research/final-step3/analyze.py ../step3-results/capture-live ../step3-results/analysis-live
```

Analyze each of `live`, `history`, `long`, `shrink`, `upper`. `--suites` accepts
a comma-separated subset; output run IDs prevent overwriting or rerunning
recorded attempts. A skipped/aborted case is preserved, never called passing.

The unchanged admission cap is min(3 GiB, 40% available), plus a 3 GiB reserve.
Existing driver runtime guards and a 360-second child timeout remain. No unsafe
10m sanitizer/high-retention campaign is required. Production and instrumentation
run in separate processes. Three-source order rotates by repetition. Repeated
regression cases receive six fresh production processes per source. Report all
timings and absolute costs; do not select favorable repetitions.

The optional branch-only workflow has read-only permissions and supplies fresh
matched Linux measurements. Existing production CI remains the cross-platform
gate. Neither workflow grants merge/release authority or human acceptance.
