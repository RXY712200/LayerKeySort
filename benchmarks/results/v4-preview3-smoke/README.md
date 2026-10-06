# V4 Preview.3 export smoke, 2026-10-06

Source: Preview.3 snapshot implementation based on frozen Preview.2 commit
`2967cc6429c6af7a0c5a8d9fabdeb31ed2bf0642`; accompanying release commit contains
this harness and these rows. AMD Ryzen 9 9955HX (16 cores/32 logical processors),
Windows x64, GCC 16.2.0 UCRT64, CMake Release, strict C17 warnings as errors.
One process/run per executable, no repeated timing median or external comparison.
Timings are engineering smoke, not final V4 performance acceptance.

Reproduce with benchmarks enabled:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON
cmake --build build
./build/layerkeysort_v4_snapshot_smoke
./build/layerkeysort_v4_snapshot_diagnostics
```

`production.csv` is uninstrumented timing. Allocation/resident-delta fields are
`NA`, not zero; normal production has no allocator counters. `diagnostics.csv`
contains requested allocation and live-byte counts; its timings include
instrumentation and must not be treated as production timings. Neither file
measures process RSS, allocator headers, fragmentation or peak application memory.

## Cases and field interpretation

Capture rows use the CSV header. N is 10k/100k/1M; namespace is 16 zero bytes;
association payload is 0/16/64 bytes from one persistent scratch span. Input is
known ordered data built with the private bulk builder to isolate export from
live insertion cost. Every capture still traverses all rows and copies payload.
Timers separately bracket capture, format all N keys, serialize, deserialize,
and restore (resolver maps to one repeated caller pointer). Key loop uses one
reused output buffer. Blob allocation occurs outside serialize timing.

Diagnostic capture allocations include namespace, row metadata, geometric byte
buffer growth, snapshot object and lazy first marker. Resident delta includes
snapshot object and the new shared marker, not the preexisting source records.
Row metadata is two size_t values (16 bytes here). Association logical bytes and
capacity are separate; namespace is stored once. `all_key_bytes` is hypothetical
materialization of all keys without NULs, **not** resident snapshot storage.
At one million rows, diagnostic resident deltas are 16,000,096 / 32,777,312 /
83,108,960 bytes for 0/16/64-byte payloads; wire sizes are 8,000,040 /
24,000,040 / 72,000,040 bytes. Materialized keys would occupy 53,000,000 bytes.

Frequency rows have a separate schema:
`frequency,mutations,cadence,export_count,total_ms,capture_and_destroy_ms`.
20k append mutations, empty associations, capture/destroy every 10000/1000/100
mutations or never. The collection grows, so sum of exported rows differs by
cadence. These scenarios demonstrate explicit cost placement. Production totals
were 1.3344 / 1.3214 / 1.8027 / 5.2702 ms. Small inversions are timing noise;
the 200-export case performed much more copying than the 2-export case.
Snapshot/export is O(N+A) work, not free work shifted invisibly into mutations.

Import rows: `import,N,elapsed_ms`. 10k/100k unsorted LK1 coordinates, levels
N..1, strict decode/sort/bulk build including owned Path cleanup. Production
times 2.2324 / 27.8017 ms include O(N log N) sorting; only final bulk construction
is O(N). Restore timers resolve every occurrence first, then directly build blocks
and balanced index without per-item AVL searches or rotations.

Capture production times at 1M were 16.1759 / 19.8548 / 34.0987 ms for 0/16/64
bytes; restore 37.6800 / 35.3811 / 32.4922 ms. These single-machine measurements
are not realtime, upper-bound, universal speed or concurrency guarantees.

CTest runs the same harness with `smoke` (512 rows; 1000 mutations; 512/1024 import)
to catch functional regressions. It has no timing acceptance thresholds. Historical
Preview.1/2 captures remain unchanged. Preview.4 retains final Group/Batch scope;
whole-V4 final performance acceptance remains separate.
