# Full Snapshot/History Step 2 candidate

Base: accepted Step 1 `19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`.
This isolated branch is not merged, released or independently accepted.

**IMPLEMENTATION READY FOR INDEPENDENT REVIEW**. Implementation, bounded
measurement, regression validation and evidence collection are complete. This
is a candidate delivery, not adoption approval or a final project PASS.

Read [REPORT.md](REPORT.md) for the complete findings and explicit regressions,
[METHOD.md](METHOD.md) for representation and measurement rules, and
[VALIDATION.md](VALIDATION.md) for build/evidence verification commands.
[FILES.txt](FILES.txt) lists the complete Step 2 change inventory;
[SHA256SUMS.txt](SHA256SUMS.txt) hashes canonical Git blobs in that inventory.

The primary comparison is accepted Step 1 versus Step 2 on the same host/epoch.
Final Windows and Linux rounds contain 616 completed native processes and two
safely skipped upper-scale cases. Earlier exploratory rounds remain archived
separately. Do not combine timings across runner CPUs or protocol revisions.
The main benefit is lower owned Snapshot memory; adverse lookup, movement,
destruction and allocation-traffic results remain disclosed in the report.

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
