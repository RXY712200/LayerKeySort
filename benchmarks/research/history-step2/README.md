# Full Snapshot/History Step 2 candidate

Base: accepted Step 1 `19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`.
This isolated branch is not merged, released or independently accepted.

The compact snapshot candidate and regression tests are implemented. The
matched comparison/evidence delivery is still being prepared; this intermediate
commit is published to run its branch-only research workflow, not as the final
completion report. Read [METHOD.md](METHOD.md) for representation, cost model,
contracts and planned measurements. Final results will be recorded here before
the assignment is declared ready for independent review.

Build the same driver against three **absolute** source directories:

```sh
cmake -S benchmarks/research/history-step2 -B ../history-a -DFULL_ROOT=/absolute/original-main -DCMAKE_BUILD_TYPE=Release
cmake -S benchmarks/research/history-step2 -B ../history-b -DFULL_ROOT=/absolute/accepted-step1 -DCMAKE_BUILD_TYPE=Release
cmake -S benchmarks/research/history-step2 -B ../history-c -DCMAKE_BUILD_TYPE=Release
cmake --build ../history-a --parallel 4
cmake --build ../history-b --parallel 4
cmake --build ../history-c --parallel 4
python3 benchmarks/research/history-step2/collect.py ../history-a ../history-b ../history-c ../capture-history
python3 benchmarks/research/history-step2/analyze.py ../capture-history ../analysis-history
```

On Windows use the same GCC/MinGW generator/compiler for every variant. The
`host.h` adapter changes only time/RSS/available-memory observation and retains
the operation sequences. Raw Step 1 evidence and drivers at their original paths
are not edited. Research-wrapper/private timings are not production timings.

The optional, branch-only `Full history Step 2 research` workflow checks out
exact A/B/C sources, builds all variants before timing, runs the fixed collector
sequentially, verifies independent oracles and uploads compact raw artifacts.
It requires no write permissions and never changes refs/releases/issues. It is
research infrastructure, not a new production integration or release gate.
