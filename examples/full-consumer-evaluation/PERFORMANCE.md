# Performance and memory evaluation

## Method and attribution

Windows x64 build 26300; AMD Ryzen 9 9955HX, 16 cores/32 logical processors; 16,304,992,256 bytes physical RAM. MSYS2 UCRT64 GCC 16.2.0, C17 Release `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`; CMake 4.4.3. Full source is unchanged from Stable. Five fresh-process timed repeats after one complete discarded warmup per size; separate fresh allocation-instrumented run. No CPU affinity, governor control, host isolation or clock-overhead subtraction. These are one-host observations, not guarantees or an external-library ranking.

Each operation bracket includes its public API call, status assertion and timer overhead. `phase_seconds` additionally includes consumer index selection and sample bookkeeping. No command parsing, independent oracle or file output is in live-mutation brackets. Data preparation and post-phase validation are outside brackets. Callback association/resolver work is part of capture/restore because those calls invoke it. Destructor work is outside restore/deserialization brackets but inside phase wall time. Buffered-file rows include file open/read-or-write/close, not library serialization; no fsync/durability claim. Cleanup/empty rows have no per-call brackets: their zero bracket values mean **unmeasured**, not zero work; use their phase wall fields.

QPC samples are strongly quantized (many 0 or 0.1 microsecond results). Per-call p99 is the sample at floor((count-1)*0.99), not a statistical confidence bound. Very short-call sums are overhead/quantization sensitive. Maxima include scheduling and allocator delays; they cannot establish a worst case. `summary.csv` reports min/median/max totals across five processes, while raw `timings.csv` preserves each process, phase wall, p99 and observed maximum. Workloads run in the fixed order below: later phases inherit earlier layouts/caches; compare matching phases/sizes only.

## Workloads

- **64 residents:** small interactive document, 20k moves per drag phase.
- **10,000 residents:** medium document, 100k moves per phase.
- **100,000 residents:** large document, 100k moves per phase.
- `grow_back`: N new occurrences in an initially empty order.
- `drag_hotspot`: alternating occurrence IDs 0/N-1 immediately before one middle anchor; after initial positioning this repeatedly reorders the small neighborhood. It is not 100k distant drags.
- `distant_drag`: ID i mod N moved to front; predominantly distant cyclic rearrangement.
- `local_pair_drag`: each selected handle moves after its current next neighbor (last handle uses previous, a no-op). Anchor lookup is outside the API bracket, inside phase wall.
- `churn`: 2 x move-count removal/insertion pairs. Three further cycles each have move-count pairs. Constant resident count; 500k pairs / 1m remove-or-insert calls across these phases at 100k residents.
- `retained_16_captures`: 16 complete snapshots with identical order and 8-byte per-row associations retained together, a deliberate frequent-save/history memory probe. It does not claim real-user cadence.
- `serialize`: 16 caller-buffer exports; `buffered_file_save/load`: 8 writes/reads of one file using warm OS caches.
- `deserialize` and `restore`: 8 operations, destruction outside the brackets.
- `managed_duplicates`: N stable comparator-managed arrivals with 257 deterministic priority keys (`i*7919 mod 257`).

A standalone independent sequence-model document scenario reaches 2,048 residents; the large timing driver checks initial ordering, live handle-to-item identity/count after churn and managed sortedness, not an independent full oracle after every 100k-resident mutation. Correctness claims rely on those distinct evidence layers, not timings alone. Runs are operation-count stress, not a days-long uptime or allocator-soak test.

## Timed matrix

All values below are **total operation-bracket milliseconds**; operation counts are per process. Each cell is median [minimum, maximum] across five processes. Smaller p99/max samples are in the raw CSV; maxima below are the largest observed bracket across those processes, not pure library latency.

| Phase | 64 residents | 10,000 residents | 100,000 residents | Largest observed at 100k (us) |
|---|---:|---:|---:|---:|
| grow_back | 0.0046 [0.0041, 0.0087] | 0.6900 [0.6831, 0.8717] | 6.6291 [6.3665, 7.1125] | 205.4 |
| drag_hotspot | 0.3772 [0.3725, 0.5802] | 1.8715 [1.8624, 2.1009] | 1.8717 [1.8624, 2.0836] | 188.3 |
| distant_drag | 0.5970 [0.5823, 0.7933] | 5.5877 [5.4445, 5.9464] | 7.5740 [7.3345, 7.8851] | 203.3 |
| local_pair_drag | 0.3729 [0.3717, 0.6129] | 2.1386 [2.1135, 2.4805] | 2.2727 [2.2057, 2.4445] | 183.5 |
| churn | 4.0285 [3.9180, 4.1486] | 17.0392 [16.1950, 17.3690] | 16.9718 [16.1409, 17.5540] | 186.2 |
| churn_cycle_1 | 1.9568 [1.9132, 2.2090] | 8.2195 [8.1345, 8.9338] | 8.4829 [8.0773, 8.7475] | 215.0 |
| churn_cycle_2 | 1.9499 [1.9194, 2.0741] | 8.2544 [8.1086, 8.6426] | 8.5294 [8.0850, 8.6251] | 206.1 |
| churn_cycle_3 | 1.9175 [1.8919, 1.9624] | 8.2958 [8.2161, 8.7831] | 8.4973 [8.0755, 8.6333] | 224.9 |
| retained_16_captures | 0.0354 [0.0333, 0.0390] | 1.3463 [1.3044, 1.8674] | 11.0167 [10.4039, 11.5300] | 1177.0 |
| serialize | 0.0020 [0.0019, 0.0021] | 0.2801 [0.2729, 0.2856] | 3.3040 [3.0842, 3.7147] | 574.9 |
| buffered_file_save | 1.6344 [1.6017, 2.2585] | 2.5680 [2.3349, 2.9355] | 5.3933 [4.6432, 9.7977] | 6272.5 |
| buffered_file_load | 0.2554 [0.2519, 0.2779] | 0.3129 [0.3041, 0.3165] | 0.6324 [0.5743, 0.8525] | 387.9 |
| deserialize | 0.0071 [0.0066, 0.0131] | 0.6285 [0.6236, 0.6325] | 7.4099 [7.2898, 7.7586] | 1205.4 |
| restore | 0.0185 [0.0178, 0.0381] | 2.2721 [2.2214, 2.7120] | 25.5725 [24.6565, 26.2544] | 3910.9 |
| managed_duplicates | 0.0072 [0.0069, 0.0157] | 1.6167 [1.5766, 1.9689] | 19.1263 [19.0444, 20.2629] | 197.3 |

## Requested memory and allocation evidence

GNU `--wrap` interception touches malloc/calloc/realloc/free references from linked library objects without private library symbols. App arrays/sample storage/file buffers are outside the active Full allocation scopes. A maximally aligned tracking header changes allocator size classes and adds overhead; instrumented timings/RSS are **not** production timing or metadata measurements. Realloc interception uses allocate-copy-free; requested traffic is the sum of successful requested sizes, not OS committed traffic. It supplies deterministic call failpoints and resource accounting, not a production custom allocator.

100k-resident separate instrumented run:

| Phase | New allocations | Frees | Live requested bytes after phase | Requested traffic in phase | Whole-process working set |
|---|---:|---:|---:|---:|---:|
| empty | 1 | 0 | 64 | 64 | 6,803,456 |
| grow_back | 103,124 | 0 | 5,024,480 | 5,024,416 | 18,014,208 |
| drag_hotspot | 0 | 1 | 5,023,896 | 0 | 18,018,304 |
| distant_drag | 3,029 | 3,122 | 4,969,584 | 1,768,936 | 18,284,544 |
| local_pair_drag | 0 | 0 | 4,969,584 | 0 | 18,288,640 |
| churn | 200,000 | 200,000 | 4,969,584 | 6,400,000 | 20,688,896 |
| churn_cycle_1 | 100,000 | 100,000 | 4,969,584 | 3,200,000 | 20,709,376 |
| churn_cycle_2 | 100,000 | 100,000 | 4,969,584 | 3,200,000 | 20,709,376 |
| churn_cycle_3 | 100,000 | 100,000 | 4,969,584 | 3,200,000 | 20,709,376 |
| retained_16_captures | 289 | 224 | 47,348,216 | 59,154,824 | 59,211,776 |
| serialize | 0 | 0 | 47,348,216 | 0 | 60,813,312 |
| buffered_file_save | 0 | 0 | 47,348,216 | 0 | 60,923,904 |
| buffered_file_load | 0 | 0 | 47,348,216 | 0 | 60,932,096 |
| deserialize | 32 | 32 | 47,348,216 | 19,200,704 | 61,743,104 |
| restore | 812,520 | 812,520 | 47,348,216 | 39,302,848 | 61,124,608 |
| history_released | 0 | 64 | 4,969,592 | 0 | 20,553,728 |
| live_released | 0 | 103,032 | 0 | 0 | 10,731,520 |
| managed_duplicates | 103,032 | 0 | 4,970,784 | 4,970,688 | 20,656,128 |
| cleanup | 0 | 103,034 | 0 | 0 | 10,727,424 |

### Interpretation

- Growing 100k records makes 103,124 allocations in the insertion phase, in
  addition to the order itself. Read-only source inspection explains the
  5,024,480 bytes: 64-byte core + 100,000 x 32-byte records + 3,124 x 584-byte
  blocks on this ABI. This is an implementation explanation, not a consumer
  dependency or portable structure-size guarantee. End splitting produces
  approximately half-full blocks; slack and per-record allocation are distinct.
- After drags, live Full requests settle at 4,969,584 bytes. Initial 200k churn
  pairs and three further 100k-pair cycles do not increase that live total.
  Later-cycle working set plateaus in this run. This does not establish absence
  of fragmentation under every allocator or days-long workload.
- Sixteen retained histories add 42,378,632 requested bytes including the shared
  marker. Source inspection explains the row array and geometrically grown
  association buffer: 800,000 useful association bytes occupy 1,048,576 capacity
  per snapshot here. The extra capacity is about 3.98 MB across 16 snapshots.
  It is a concrete capacity cost, not a leak or automatic reason to replace the
  representation. Capture traffic is larger than retained bytes because buffers
  grow. Whole snapshot copies, not live resident allocation, dominate this phase.
- One wire image is 1,600,040 bytes (24-byte header + 16 namespace bytes + 100k
  x (8-byte length + 8-byte association)). Sixteen images total 25,600,640 bytes;
  wire size is not the same as resident snapshot allocation.
- Releasing history leaves one 8-byte source marker, released with the source.
  Releasing the source reaches zero tracked allocations/bytes; managed cleanup
  independently reaches zero. Remaining working set includes runtime, app
  fixtures, touched global latency samples and OS/allocator retention.
- RSS/working set never means Full-only memory. Metadata, fragmentation and
  allocator retention were **not individually attributed**. ASan's quarantined
  working set is intentionally excluded from production comparisons.
- Repeated captures of the same unchanged order are avoidable application work;
  real save cadence and retention budgets must be supplied before considering
  incremental export/deduplication. Deletion/moves do not require snapshots.

## Engineering conclusion

The measured consumer has practical live-mutation costs on this host and no
observed coordinate relabel cliff; V4 has no resident coordinates to relabel.
Snapshot/history work is visibly larger and scales with explicitly retained
state. No deployment budget or genuine user workload was supplied, so this is
fitness evidence for the tested workflows, not universal suitability or latency
certification. No speculative block/pool/snapshot redesign follows from these
measurements. See DEPENDENCIES.md for conditional proposals and HISTORY.md for
current-versus-historical severity.
