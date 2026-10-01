# V3 Preview.2 direct comparison metadata

- Date: 2026-10-01 (Asia/Shanghai).
- Machine: Windows 11, AMD Ryzen 9 9955HX, 32 logical processors.
- Compiler: MSYS2 UCRT64 GCC 16.2.0, C17, `-O2 -Wall -Wextra
  -Wpedantic -Werror -O3 -DNDEBUG` through CMake Release.
- Stable source: detached `v2.0.0` at
  `9fb7a9b0ae8d71cbd7410822e37702f6a6dc4d88`.
- Preview.1 source: clean detached published tag at
  `6fd34ca709ded89e0784e7c33cf08a6603ac9df1`.
- Preview.2 source: `v3-preview2-relabel-optimization` working candidate,
  later committed without changing the measured algorithm.
- Primary seed: `0x91A30D47`; holdout: `0xDEADBEEF`. Each timing row has
  one warmup and three measured repetitions, reporting median/min/max ms.
- `tree` measures insertion only; input generation, Tree creation, validation,
  and destruction are outside the interval. Stable V2 uses
  `lks_tree_insert_item`; V3 uses `lks_ordered_tree_insert`.
- Diagnostic binaries are separate `-O2 -DNDEBUG` builds with
  `LKS_ENABLE_ALLOC_DIAGNOSTICS` and `LKS_BENCH_DIAGNOSTICS`. Their timings
  are **not** compared with timing CSV rows. Preview.1's temporary diagnostic
  build adds only cumulative attempted-region and generated-Path counters;
  its detached source worktree was restored clean after compilation.
- The CSVs and diagnostic log are observations on one machine. They do not
  establish a worst-case or amortized complexity guarantee.

## Direct result summary

| Workload | N | Stable V2 median ms | Preview.1 median ms | Preview.2 median ms |
| --- | ---: | ---: | ---: | ---: |
| Ascending | 100,000 | 303.922 | 362.676 | 54.104 |
| All equal | 100,000 | 303.207 | 362.617 | 53.426 |
| 32-value duplicates | 100,000 | 424.473 | 107.261 | 109.607 |
| Random unique | 100,000 | 97.487 | 96.812 | 98.830 |
| Alternating | 10,000 | 189.293 | 75.047 | 73.228 |
| Descending | 10,000 | 2.984 | 2.982 | 3.080 |
| Ascending | 300,000 | 3343.667 | 3616.003 | 505.805 |

The primary duplicate and random medians regress by approximately 2.2% and
2.1% against Preview.1, respectively. On the independent holdout seed,
Preview.2 instead improves both. Descending is approximately 3.3% slower in
this short test; the measured difference is about 0.1 ms. The result is not a
universal improvement claim.

Ascending 100,000 diagnostic observations: Preview.1 attempted 100 relabels,
including five full-range relabels of 292,764 old nodes in total. It visited
719,468 region nodes across all attempts and generated 719,634 candidate
Paths. Preview.2 attempted no relabel, visited no relabel region nodes, and
generated no relabel Paths. Its 99,935 burst-mode endpoint inserts used
300,001 library allocation calls versus Preview.1's 1,739,366; cumulative
requested bytes were 12,628,392 versus 155,027,832; peak live bytes were
10,702,928 versus 22,181,704. Mean/P95/P99 Path depth changed from
3.987/6/7 to 2.051/3/4. Comparator calls (1,568,929), AVL rotations
(99,983), and final height (17) were unchanged. Stable V2's nine physical
rebuilds rebuilt 522,876 nodes on this workload.

Random 100,000 remains a costly relabel case in both V3 versions: one
full-range relabel of 4,230 old nodes survives. Preview.2's relabelled-node
total fell from 8,606 to 6,302, while both used 1,544,756 comparator calls,
69,754 rotations, and final AVL height 20. Alternating 10,000 relabelled-node
work is unchanged at 119,789 old nodes and one 189-node full-range relabel.

The endpoint benefit is not a guarantee that relabel disappears. A separate
ascending 300,000 diagnostic run triggered one 261,578-node full-range
relabel in Preview.2, versus 20 full-range relabels involving 3,319,695 old
nodes in Preview.1. Preview.2 still visited 523,714 attempted region nodes
and briefly reached candidate depth nine before relabel. This is a real
single-operation cost cliff and remains a core performance concern.

See the full `v3-preview2-diagnostics.txt` for allocation, depth, failed
attempt, and other distributions. The earlier captured Preview.1 publication
measurements remain in their original files and were not overwritten.
