# Measurement and correctness boundaries

## Timing

QueryPerformanceCounter is used on this Windows host. The final sampler takes
one rotated position per **256-operation stratum**, at offset
`(73 * stratum + 17) mod 256`, at every scale. It avoids synchronization with
the locate workload's every-fourth missing query. At 10m growth it retains about
39k brackets; at 100k about 390. Small-sample p99/maxima cannot establish rare
event frequencies. Quantiles are sample indices floor((n-1)*q), not confidence
bounds. Observed samples at/near the 0.1 us clock quantum are resolution-limited.

Phase wall timing surrounds the full API replay. It includes necessary
handle-map reads/writes, status/result checks, loop/anchor generation, no-op
probe calls, sparse timing and periodic memory/time-budget checks. It excludes
item allocation/key generation, expected-model preparation, qsort reference
construction, phase summaries, working-set queries and full verification.
It is not a cycle-perfect isolated library measurement. Public API brackets
isolate selected calls more closely, with timer overhead retained and disclosed.

Churn brackets contain **remove plus insert**, not one API call. Its normalized
throughput denominator counts both calls. Sampled maxima are not the maximum of
every operation. Capture/serialize/load/restore/teardown use one whole-phase
duration and no per-call samples; zero samples mean unmeasured percentiles.
Total child wall time in runs.json includes fixture/oracle preparation,
verification, all cleanup and process launch/capture overhead.

No pinning, power-mode control, OS isolation or timer-overhead subtraction is
used. Fresh child processes run sequentially. Fixed profile order introduces
possible order/cache effects; later edit phases intentionally inherit earlier
layouts. Compare matching profiles/phases/seeds/counts, not unrelated states.
Repetitions are limited at upper scales and shown rather than hidden.

### Pilot correction

The initial complete exploratory matrix used a scale-dependent sample density.
Its regular sample offset also overrepresented missing queries in locate
latency. Those data are **not the final performance evidence**. The final
matrix was recollected after fixing both issues. Raw pilots remain local with
their own source hashes. This was a research-harness correction, not a Full
defect or an optimization of Full. No favorable pilot row was substituted into
the final data. The earlier consumer's 100k timing numbers are historical and
are not copied or treated as a controlled speed comparison.

## Independent reference model

The expected sequence is an array-linked list of integer occurrence IDs,
separate from Full's blocks, index, handle addresses and traversal outputs.
Each expected transition is completed before the corresponding phase replay.
Construction and every edit phase end with full forward and reverse traversal,
all expected IDs/items/handle pointers, endpoints and exact counts. Selected
comparisons distributed through the whole order and adjacent first-region
comparisons check index order. Source handles are not inspected after expiry.

Managed expected order comes from independent qsort of IDs by (integer key,
arrival ID), making equality precedence explicit. Locate expected bounds use
that sequence, not Full getters. All key fields remain unchanged while resident.
Snapshot restore is checked against the same expected full ID sequence with
new handles; source-destruction cleanup is separately observed.

The standalone C oracle is tested against Python list transitions for 20k
random operations at moderate size, without linking Full. Its comparison
checker also rejects a deliberately corrupted observation. This tests oracle
logic but is not independent human review of the evaluator.

At million scales verification is **exact at checkpoints**, not after every
mutation. Intermediate transient errors that repair themselves before a
checkpoint are not ruled out. Stored sequence digests are compact receipts;
the check itself compares every entry rather than relying on hash equality.
Hashes/counts are cross-checked across production/allocation/private runs for
identical seeds. The long campaign checks complete sequences every tenth
cycle, plus first/final, while every API result is checked.

## Memory and instrumentation

Application fixture requests: 16N item bytes, 8N handle-map bytes, 8N ID-model
bytes, plus a bounded 12-byte-per-operation command trace and 512 KiB latency
buffer. Managed reference sorting temporarily adds 4N IDs, freed before growth.
Locate allocates another 4N sorted-ID reference array outside timing. The long
driver uses 8N payloads + 8N handles + 8N model + two bounded ID trace arrays.
These are not Full-only allocation bytes.

GNU wrappers intercept the unchanged production library's allocation calls in
separate processes. App allocations happen outside active scopes. Tracking
headers maintain fundamental alignment and change actual allocator size
classes. Realloc uses **real CRT realloc** in this study, unlike the earlier
consumer's allocate-copy-free interceptor. Calls/requests/peaks are Full-only;
RSS and underlying CRT/OS allocation behavior are not thereby Full-only.

The diagnostic executable recompiles unchanged production sources using the
existing LKS_ENABLE_ALLOC_DIAGNOSTICS opt-in, with a private adapter confined to
probe.c. Its core object is 4,280 bytes larger on this ABI. Its allocator also
has tracking-header overhead. Private counters are not linked into production
timing. Location assignments count the library's resident-location updates,
not every byte copied or CPU store. Index writes count distinct touched index
blocks per operation, summed across operations; splits/merges/redistributions/
rotations and occupancy histograms are separately recorded.

Requested peak is the Full process-lifetime high-water mark, not a per-phase
peak. Traffic sums successful allocation/reallocation requested sizes, not OS
committed traffic. Phase working set is whole-process sampled working set,
not PeakWorkingSetSize, a continuous peak, allocator metadata or fragmentation.
No subtraction of app requests from RSS is sold as allocator overhead.

Raw `residents` is oracle cardinality and retains the previous sequence on the
library_destroy row. The derived live-resident field becomes zero there. All
instrumented teardown checks require zero requested bytes and allocations;
production has no allocator counters and reports NA/unavailable instead.

## Safety and non-goals

Planning envelopes are intentionally larger than expected requests and include
payloads/maps/model, Full records/blocks, allocator/header slack, samples and
temporary memory. Estimates and current physical availability are recorded
before **every process**, including each upper-scale repetition. A smaller
completed pilot is required before escalation. The estimates are safety
policies, not measurement or proofs of maximum usage.

History is capped at 1m, with two captures, one loaded snapshot and one restored
live source. Larger history was deliberately not attempted; a 3m history might
fit on this host but was outside the chosen multi-state envelope. A 10m history
planning estimate is about 3.07 GB before reserve constraints. No enormous
sanitizer campaign, OOM exhaustion, invalid stale handle, concurrent mutation or
power-loss experiment is performed. Single-host finite stress does not prove
universal worst-case or amortized operation bounds.
