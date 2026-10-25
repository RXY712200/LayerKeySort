# Full final integrated Step 3 acceptance

This isolated assignment starts at accepted Step 2
`291305dca03d2d7599c88b6e6cebf2cc426e1790`, including accepted Step 1
`19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`.

The initial commit supplies fresh validation infrastructure; it is not a final
acceptance claim. A complete report and compact raw evidence follow before
delivery. No production change, merge, release or final project PASS is implied.

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
