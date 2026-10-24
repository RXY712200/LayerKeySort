# Fresh measured results

All tables derive from committed raw observations; no old timing rows are included.

## Completed/limited scale matrix

| Scale | Production processes | Allocation processes | Structural processes | Policy skips |
|---|---:|---:|---:|---|
| 100,000 | 30 | 10 | 10 | none in planned scale matrix |
| 1,000,000 | 32 | 10 | 10 | none in planned scale matrix |
| 3,000,000 | 18 | 6 | 3 | history capped at 1m |
| 10,000,000 | 12 | 5 | 2 | history and sanitizer omitted |

10k warmups are separate and excluded. Primary seed 17; 1m anchor/random holdouts use seed 712200 and remain separate from timing summaries. At 10m append/random/duplicates have two production repeats; other families have one. Two snapshots plus loaded state and restored live order are capped at 1m.

## Growth throughput and scaling

Total phase seconds; median [minimum, maximum]. Parentheses give number of fresh processes. Each growth phase contains N inserts.

| Profile | 100k | 1m | 3m | 10m | 10m ns/insert |
|---|---:|---:|---:|---:|---:|
| append | 0.006115 [0.005678, 0.008760] (3) | 0.058681 [0.058654, 0.062679] (3) | 0.175323 [0.175101, 0.175545] (2) | 0.611243 [0.604993, 0.617492] (2) | 61.1 |
| prepend | 0.006453 [0.005519, 0.006855] (3) | 0.061507 [0.059815, 0.062919] (3) | 0.171723 [0.169690, 0.173756] (2) | 0.588016 [0.588016, 0.588016] (1) | 58.8 |
| anchor | 0.006358 [0.006357, 0.006372] (3) | 0.066172 [0.064442, 0.067447] (3) | 0.195305 [0.194342, 0.196268] (2) | 0.679762 [0.679762, 0.679762] (1) | 68.0 |
| local | 0.007780 [0.007539, 0.008076] (3) | 0.082745 [0.080107, 0.083114] (3) | 0.243603 [0.235238, 0.251968] (2) | 0.790017 [0.790017, 0.790017] (1) | 79.0 |
| ascending | 0.007281 [0.007012, 0.007411] (3) | 0.077603 [0.077525, 0.077753] (3) | 0.246368 [0.244712, 0.248023] (2) | 0.871224 [0.871224, 0.871224] (1) | 87.1 |
| descending | 0.008732 [0.008146, 0.008924] (3) | 0.091805 [0.086420, 0.093465] (3) | 0.271827 [0.269959, 0.273695] (2) | 0.931163 [0.931163, 0.931163] (1) | 93.1 |
| random | 0.026931 [0.026883, 0.030445] (3) | 0.804755 [0.781542, 0.841642] (3) | 4.416295 [4.292552, 4.540038] (2) | 23.042261 [22.385721, 23.698801] (2) | 2304.2 |
| duplicates | 0.015299 [0.015033, 0.015943] (3) | 0.173094 [0.172265, 0.179721] (3) | 0.543408 [0.542922, 0.543894] (2) | 1.959025 [1.929077, 1.988974] (2) | 195.9 |
| equal | 0.007398 [0.006920, 0.007764] (3) | 0.077261 [0.077138, 0.083470] (3) | 0.243541 [0.243408, 0.243673] (2) | 0.874671 [0.874671, 0.874671] (1) | 87.5 |

## Append-origin editing phases

100k or 200k moves per phase depending on scale; churn rows contain twice that many API calls. Values are **ns per API operation**, including replay bookkeeping, sampling and periodic safety checks. Sampled churn latency brackets encompass a remove+insert pair and must not be compared as single-call quantiles.

| Phase | 100k | 1m | 3m | 10m | 10m largest sampled bracket (us) |
|---|---:|---:|---:|---:|---:|
| local_move | 19.6 | 128.1 | 168.5 | 201.9 | 3.5 |
| hotspot_move | 100.0 | 488.7 | 669.2 | 992.5 | 5.9 |
| front_back_move | 103.4 | 485.3 | 550.8 | 777.3 | 44.2 |
| uniform_move | 117.1 | 702.0 | 850.6 | 1039.4 | 9.5 |
| uniform_churn_1 | 94.3 | 471.5 | 554.4 | 669.9 | 4.2 |
| hotspot_churn | 83.9 | 85.5 | 89.3 | 109.9 | 1.5 |
| uniform_churn_2 | 103.4 | 389.0 | 373.3 | 428.3 | 4.7 |
| uniform_churn_3 | 92.8 | 370.9 | 368.3 | 397.3 | 32.6 |
| half_delete | 68.9 | 205.1 | 141.3 | 95.7 | 7.6 |
| regrow | 37.6 | 58.8 | 55.7 | 59.3 | 5.3 |

## Allocation and occupancy at growth checkpoints

Requested bytes exclude allocator metadata. Wrapper RSS contains its tracking headers; production RSS is a separate observation. Structural objects contain extra diagnostic state.

| Profile / scale | Allocation calls | Requested bytes | Requested B/resident | Production working set B | Blocks | Mean occupancy | Height |
|---|---:|---:|---:|---:|---:|---:|---:|
| append / 100,000 | 103,125 | 5,024,480 | 50.245 | 15,245,312 | 3124 | 32.01 | 12 |
| anchor / 100,000 | 103,125 | 5,024,480 | 50.245 | 15,245,312 | 3124 | 32.01 | 12 |
| random / 100,000 | 102,262 | 4,519,936 | 45.199 | 14,716,928 | 2260 | 44.25 | 13 |
| duplicates / 100,000 | 103,094 | 5,005,824 | 50.058 | 15,245,312 | 3092 | 32.34 | 13 |
| append / 1,000,000 | 1,031,250 | 50,249,480 | 50.249 | 104,300,544 | 31249 | 32.00 | 15 |
| anchor / 1,000,000 | 1,031,250 | 50,249,480 | 50.249 | 104,300,544 | 31249 | 32.00 | 15 |
| random / 1,000,000 | 1,022,450 | 45,109,728 | 45.110 | 98,963,456 | 22448 | 44.55 | 17 |
| duplicates / 1,000,000 | 1,031,216 | 50,229,072 | 50.229 | 104,300,544 | 31214 | 32.04 | 16 |
| append / 3,000,000 | 3,093,750 | 150,749,480 | 50.250 | 301,944,832 | 93749 | 32.00 | 17 |
| anchor / 3,000,000 | 3,093,750 | 150,749,480 | 50.250 | 301,940,736 | 93749 | 32.00 | 17 |
| random / 3,000,000 | 3,067,336 | 135,323,152 | 45.108 | 286,179,328 | 67334 | 44.55 | 19 |
| duplicates / 3,000,000 | 3,093,719 | 150,730,824 | 50.244 | 301,940,736 | unmeasured | unmeasured | unmeasured |
| append / 10,000,000 | 10,312,500 | 502,499,480 | 50.250 | 993,800,192 | 312499 | 32.00 | 19 |
| anchor / 10,000,000 | 10,312,500 | 502,499,480 | 50.250 | 993,804,288 | 312499 | 32.00 | 19 |
| random / 10,000,000 | 10,224,454 | 451,080,064 | 45.108 | 941,383,680 | unmeasured | unmeasured | unmeasured |
| duplicates / 10,000,000 | 10,312,466 | 502,479,072 | 50.248 | 993,796,096 | unmeasured | unmeasured | unmeasured |

## Selected historical operations

Medians in milliseconds, one operation per phase per process (no per-call percentile). Restore includes resolver callbacks and scratch cleanup. Requested peak is the process-lifetime Full high-water mark, not a phase-only peak.

| Phase | 100k ms | 1m ms | 1m phase allocations / reallocations | 1m requested live B | 1m cumulative requested peak B |
|---|---:|---:|---:|---:|---:|
| capture | 0.852 | 17.236 | 5 / 17 | 74,638,184 | 74,638,184 |
| retained_two | 0.716 | 18.293 | 4 / 17 | 99,026,880 | 99,026,880 |
| serialize | 0.266 | 2.574 | 0 / 0 | 99,026,880 | 99,026,880 |
| deserialize | 0.911 | 9.429 | 4 / 0 | 123,026,968 | 123,026,968 |
| restore | 3.263 | 34.873 | 1015627 / 0 | 164,152,032 | 172,152,032 |
| restored_destroy | 1.222 | 27.260 | 0 / 0 | 123,026,968 | 172,152,032 |
| history_destroy | 0.512 | 2.865 | 0 / 0 | 50,249,488 | 172,152,032 |
| library_destroy | 2.181 | 32.328 | 0 / 0 | 0 | 172,152,032 |

## Long-lifetime churn: 100 cycles

Each process performs **10 million remove/reinsert pairs / 20 million API calls**, no resident growth. Every tenth cycle plus first/final has full forward/reverse identity verification. Two production processes and one allocation/structural process per scale. Cycle times are sequential observations, not independent trials.

| Scale | Production median cycle ms | Production max cycle ms | Allocation live B cycle 1 / 50 / 100 | Allocation working set B cycle 1 / 50 / 100 |
|---|---:|---:|---|---|
| 100,000 | 18.657 | 36.066 | 4,590,568 / 4,598,744 / 4,594,656 | 18,649,088 / 18,649,088 / 18,649,088 |
| 1,000,000 | 100.251 | 138.852 | 45,910,944 / 45,972,264 / 45,969,344 | 130,068,480 / 130,068,480 / 130,068,480 |

## Correctness and safety summary

- Completed matrix processes: 158 including 10 warmups. Eight skipped-attempt records retain six initial memory admission deferrals (all subsequently completed on retry) plus two history-policy omissions.
- Exact matrix checkpoints: 848. All counts/sequence digests agree across modes and repetitions for identical scale/profile/seed/checkpoint.
- Long-churn processes: 8, exact matching checkpoints: 96; every completed process reported zero tracked cleanup where instrumentation exists.
- Smallest phase-observed available physical memory: 3.583 GiB. This is not a continuously measured system minimum.
- Maximum observed location assignments per diagnostic API operation: 160; maximum distinct index blocks per operation: 44. These are observations, not latency guarantees.
- Recorded final statuses contain zero failed executions and zero budget aborts. Completed is an observed process status, not an independent acceptance decision.
- Raw `residents` is the model cardinality; the library_destroy row retains the former source cardinality. `live_residents_derived` explicitly becomes zero at destruction. Empty latency samples mean unmeasured, not zero latency.
- No 3m/10m history campaign or 10m sanitizer campaign was executed. The upper live-order target was actually executed, not inferred.
