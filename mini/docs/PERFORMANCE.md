# Initial performance and memory baseline — v1.0.0-rc.1

Measured locally on 2026-10-08: Windows x64, MSYS2 UCRT64 GCC 16.2.0,
CMake 4.4.4, MinGW Makefiles, Release (-O3 -DNDEBUG), C17 with
-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror. No LTO or sanitizer.
These are single-run observations, not portable throughput guarantees or a
comparison against another product. No speculative optimization was performed.

## Reproduce

From this independent project directory, add the matching generator if needed:

```sh
cmake -S . -B bench-build -DCMAKE_BUILD_TYPE=Release -DLKS_MINI_STRICT=ON -DLKS_MINI_BENCHMARKS=ON
cmake --build bench-build --config Release
./bench-build/lks_mini_baseline
```

For MinGW use -G "MinGW Makefiles" and run bench-build/lks_mini_baseline.exe.
The harness uses only the public header/library, standard C17 allocation and
timespec_get(TIME_UTC). Output is CSV in milliseconds of elapsed wall time.
The UTC clock is not guaranteed monotonic and may adjust. Short measurements
have clock quantization, scheduling, cache, allocator and run-to-run noise; no
warmup distribution, confidence interval, CPU affinity or allocator-inclusive
memory measurement is claimed. Create/destroy and handle-array allocation are
outside each recorded interval. Original clock() results below its resolution
were discarded and replaced with these timespec_get measurements.

## Workloads and observations

Sizes are 128, 2,048 and 32,768 live occurrences. Payloads are NULL. Tail/head
insertion starts empty and reaches n. Removal starts at n and drains to empty.
Known-handle movement repeats front/back of the same last occurrence 20,000
times (40,000 moves, n unchanged). Traversal performs 20 complete forward walks
(20n next calls plus endpoint queries). Comparison checks head against tail 200
times, a deliberately long-distance O(n) workload. Mixed work repeats 20,000
front insertions, moves to back and removals (60,000 calls, n or n+1 live).
Order creation/destruction is excluded from workload timings.

| Live size n | Workload | Operations | Measured total ms |
| ---: | --- | ---: | ---: |
| 128 | tail_insert | 128 | 0.004530 |
| 128 | known_handle_move | 40000 | 0.037670 |
| 128 | traverse | 2560 | 0.002384 |
| 128 | head_tail_compare | 200 | 0.014305 |
| 128 | mixed_insert_move_remove | 60000 | 0.432730 |
| 128 | remove | 128 | 0.001907 |
| 128 | head_insert | 128 | 0.002623 |
| 2048 | tail_insert | 2048 | 0.066519 |
| 2048 | known_handle_move | 40000 | 0.037432 |
| 2048 | traverse | 40960 | 0.054598 |
| 2048 | head_tail_compare | 200 | 0.566483 |
| 2048 | mixed_insert_move_remove | 60000 | 0.435591 |
| 2048 | remove | 2048 | 0.033379 |
| 2048 | head_insert | 2048 | 0.053644 |
| 32768 | tail_insert | 32768 | 0.926018 |
| 32768 | known_handle_move | 40000 | 0.037193 |
| 32768 | traverse | 655360 | 1.703501 |
| 32768 | head_tail_compare | 200 | 14.157295 |
| 32768 | mixed_insert_move_remove | 60000 | 0.407219 |
| 32768 | remove | 32768 | 0.390053 |
| 32768 | head_insert | 32768 | 0.819921 |

Comparison and traversal increase with collection size in this sample; repeated
known-handle movement is approximately size-independent in this cache-warm case.
These observations are consistent with the documented algorithms but do not
establish general latency, scaling constants or performance superiority.

## Structural memory measurement

The private test prints actual sizeof values without changing the public ABI:
local x64 GCC reports order = 24 bytes and node = 32 bytes. Thus the structural
estimate for n live occurrences is 24 + 32n bytes:

| n | Struct-only bytes |
| ---: | ---: |
| 128 | 4,120 |
| 2,048 | 65,560 |
| 32,768 | 1,048,600 |

This excludes allocator headers, alignment/size-class overhead, fragmentation,
reserved pages, caller objects and caller handle arrays (the benchmark uses one
pointer per occurrence). Actual allocated/resident memory was not measured.
Other ABIs may yield different sizeof values. Count allocation calls separately
from memory consumption: one order allocation plus one allocation per insertion;
movement/comparison do not allocate. Optional future allocator-inclusive studies
are measurement improvements, not a claim of an existing defect.

## v1.0.0 applicability

The recorded RC.1 source and benchmark remain unchanged in Stable preparation,
so this remains the initial reproducible baseline, not a new Stable timing claim.
The target release identity is v1.0.0; it has not been published. Reexecute the
commands for observations on another candidate or machine and retain the
compiler, options and clock limitations. No allocator-inclusive measurement
or cross-platform performance guarantee is added.
