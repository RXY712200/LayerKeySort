# Preview.2 preliminary mutation screen

One Windows x64 GCC 16.2 Release capture, taken on 2026-10-06 (Asia/Singapore).
Source version: `4.0.0-preview.2`; B128; deterministic seed 17; 20000 residents
and measured operations. This is a sanity screen, not final performance acceptance.

- `production.csv`: normal library; timing and comparator calls. Zero diagnostic
  columns mean unavailable, not zero structural work or allocation.
- `diagnostics.csv`: separately instrumented library, including allocations and
  local/index work. Its timings include instrumentation; do not mix with production.

Build benchmarks ON; run `layerkeysort_v4_mutation_smoke` and
`layerkeysort_v4_mutation_diagnostics` without arguments to reproduce workloads.
Argument `smoke` uses the fixed smaller CI case. QPC timings include per-operation
clock/check overhead; nearest-index p50/p99 and maximum are samples from one run,
not realtime bounds. Very short samples may round to zero.

The harness verifies resident count/traversal and managed sorted/stable equality.
Independent exact move/managed oracles are in the separate regression suite.
See [full methodology](../../../docs/V4_PREVIEW2.md) for operation definitions,
allocation/counter interpretation and limitations. Hardware/OS scheduling and
allocator effects are uncontrolled; these rows are not a universal ranking.
