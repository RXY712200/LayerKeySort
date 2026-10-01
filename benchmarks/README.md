# Current benchmark harness (Preview.4)

Stable v2.0.0 leaves the captured Preview.4/Stage 5 timing data
unchanged. Mutation soak is a separate correctness test.

`current_benchmark.c` exercises public APIs and prints one CSV row:

```text
operation,distribution,n,warmups,repetitions,median_ms,min_ms,max_ms,output_bytes
```

Times use C17 `timespec_get(TIME_UTC)` wall clock. Each process performs the
requested warmups and independent measured runs on the same deterministic
input. The primary value is the median; the minimum and maximum show spread.
Input preparation, output verification, and destruction of a result Tree or
Group are outside the measured interval. Tree creation is outside its insert
interval. A parse interval includes destruction of each parsed Path. Path
format/parse cases time 100 calls per run; there `n` means Path depth and
`output_bytes` excludes NUL. Mutation setup creates an initial Tree and
candidate Paths before timing. `mut_mixed` performs three operations for each
of `n/2` iterations; the other mutation cases perform `n` operations.

The harness checks sorted order, stable equality where the API promises it,
Tree size, mutation item identity, or Path round trip outside the timing.
`qsort` is checked only for sorted order because standard C does not require
stability. Timings are not correctness evidence; run the normal tests too.

## Build and run

The optional CMake target is enabled with `-DLKS_BUILD_BENCHMARKS=ON`.
Configure an optimized Release build and run `layerkeysort_benchmark`. With
tests also enabled, CTest runs three small benchmark self-checks.

For the documented GCC comparisons, compile all production `src/*.c` files
with the same GCC and `-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`.
Compile the same harness against each checked-out version's `include/` and
`src/`; define `LKS_BENCH_PREVIEW3` only when compiling against released
Preview.3, whose public header lacks later APIs. Keep baseline copies and
build outputs outside the primary checkout. The harness does not require
diagnostic instrumentation for timing.

`run_matrix.ps1` takes an executable, version label, output CSV path, and
`Common` or `Current` profile. `Common` uses only Preview.3-compatible public
operations; `Current` adds larger scales, mutations, and Path text/key cases.
The optional seed argument to the C executable supports independent holdout
runs. For example:

```powershell
./benchmarks/run_matrix.ps1 -Executable ./layerkeysort_benchmark.exe `
  -Version current -Output ./current.csv -Profile Current `
  -Warmups 1 -Repetitions 7
```

The diagnostic build is separate from timed results. Compile the harness and
library with `LKS_ENABLE_ALLOC_DIAGNOSTICS` and `LKS_BENCH_DIAGNOSTICS`, add
`src/` to the include path, and run `tree` cases with one repetition. Its
`diag` record reports, in order: distribution, count, comparator calls,
comparator search steps, rotations, AVL height, repair attempts, successes,
fallbacks, expansions, maximum region nodes, relabelled nodes, deeper
acceptances, full rebuilds, allocation calls, cumulative requested bytes,
average/P95/P99 Path depth, average/maximum display length, average/maximum
LK1 length, peak live bytes, live Tree-node bytes, live Path-object bytes,
live Path-step bytes, and Tree-node `sizeof`. With
`LKS_BENCH_STAGE5_DIAGNOSTICS`, an additional `rebuild_reasons` record gives
gap-limit attempts, depth-limit attempts, and nodes rebuilt by successful
full rebuilds. These counters are private and are not a public ABI.

For a comparable comparator-call count on Preview.3, Stage 4, and current,
compile each version with `LKS_BENCH_COUNT_COMPARISONS` and run the same `tree`
case. This counter build is never used for the published timing table.

See [the benchmark report](../docs/BENCHMARKS.md) for the measured machine,
workloads, results, limitations, and distinctions between versions.
The captured 30-row Preview.3 and 61-row Stage 4/5 matrices are in
[`results/`](results/). Each row records the observed range as well as median;
new runs should be compared as distributions rather than expected to reproduce
identical millisecond text.
