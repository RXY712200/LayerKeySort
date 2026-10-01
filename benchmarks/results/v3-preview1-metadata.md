# V3 Preview.1 direct stable-V2 comparison metadata

- Date: 2026-10-01 (Asia/Shanghai).
- Machine: Windows 11, AMD Ryzen 9 9955HX, 32 logical processors.
- Compiler: MSYS2 UCRT64 GCC 16.2.0, both checked-out versions.
- Build: CMake `Release`, `LKS_BUILD_TESTS=OFF`,
  `LKS_BUILD_BENCHMARKS=ON`; actual production flags in both builds were
  `-O2 -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG -std=c17`.
- Stable source: clean detached `v2.0.0` checkout at
  `9fb7a9b0ae8d71cbd7410822e37702f6a6dc4d88`.
- V3 source: `v3-preview1-core-architecture` development branch with
  preferred direct depth 6 and open-end direct allowance 8.
- Input seed: `0x91A30D47` for every row. One warmup and three measured
  repetitions per row; median/minimum/maximum are wall-clock milliseconds.
- `tree` measures comparator-managed insertion only; input creation, result
  validation, and destruction are outside the timed interval. V2 invokes
  `lks_tree_insert_item`; V3 invokes `lks_ordered_tree_insert`.
- Diagnostic builds were separate from timed builds and used
  `LKS_ENABLE_ALLOC_DIAGNOSTICS` and `LKS_BENCH_DIAGNOSTICS` with
  `-O2 -DNDEBUG -std=c17 -Wall -Wextra -Wpedantic -Werror`. Diagnostic
  timings must not be compared with the CSV timing rows.

The results are one-machine workload measurements, not general speed or
complexity guarantees. V3 Preview.1 was not published when recorded.
