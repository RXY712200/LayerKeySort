# Full Step 2 — Snapshot/History optimization delivery

2026-10-25. **IMPLEMENTATION READY FOR INDEPENDENT REVIEW**. This report does not
approve adoption, integration, a release or final project PASS.

## 1. Source baselines and delivery scope

Repository: `RXY712200/LayerKeySort`. Branch: `optimization/full-history-step2`.

|Reference|Exact commit|
|---|---|
|Original public main|`043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`|
|Full Stable v4.0.0|`80004ae732daf696f7e9192e417de0edb0b2aafa`|
|Mandatory accepted Step 1 base|`19ca51bc616a1b6a2d28530bc0932dc7c4a255ce`|
|Million-scale research|`d9d3b370fb8daf134eadc26c64e3b30aae12ec75`|
|Independent consumer evaluation|`bf27502f18de5988cd2d1c797ddc3e0ff48d0b03`|
|Initial Step 2 representation|`a3e136a77004c8fd04d6696e4b7923e4b1855e27`|
|Final production code / codec refinement|`7ac34f54dd7c7c544605efd0f8d97dbc4f02cd12`|
|Corrected process-order protocol|`a29e9472aebd4a3b7c0ec8cd50d94d5337662ed1`|

The final evidence commit encloses this report; its exact remote SHA is supplied
in the delivery response. Its production source matches the final code pin above.
The v4.0.0 annotated tag object remains
`83d67d3ef5dacaf6117db320d9bbea68b199e23e`.

The isolated worktree/branch starts exactly at accepted Step 1, not old main.
A uses original-main/Stable production. Windows compiled A from the existing
million-scale research checkout, whose production/header/manifest are identical;
Linux checks out original main directly. B is accepted Step 1. C is this combined
Step 1 + Step 2 candidate. **B versus C is the primary comparison.**

Read the full Step 1 report, Snapshot modules/contracts, compatibility/freeze,
existing tests, V4 decisions, current Issues3/4 and prior consumer/large-scale
history data. The owner accepted Step 1 as an isolated candidate; this task does
not change that branch or its evidence. Main, Stable, Mini, issues and protected
settings are unchanged. No tag, release, PR, merge or Step 3 work is included.
A branch-only read-only research workflow was added because initial local RAM
could not safely admit benchmarks; it is not a production/release/governance gate.

## 2. Architecture analysis and selected representation

Previously each historical row stored `offset,size` even though all association
spans form one contiguous owned vector. Replace that redundant pair with one
cumulative end word. On these x64 builds: **16 ->8 bytes/row**. Offset is zero
for row0 and the previous row's end otherwise; size is current end minus offset.
Repeated ends represent empty associations. Public arbitrary-row access remains
constant-work; sequential serialize/restore carry the previous end in a local.

Group the known-size header, row vector and copied namespace into **one**
allocation. These fields always expire together, so this grouping does not have
the independently expiring resident-slab pinning problem rejected in Step 1.
Association bytes remain separately growable, then shrink to exact useful size
before publication. No shared history, COW, deduplication, borrowed wire views,
new refcount scheme or new persistent format is introduced.

The constructor initializes the header and copied namespace; every row end is
written during capture/load before publication. Redundant full-row zero-fill is
removed. Partial objects are only destroyed, never inspected or returned. All
sum/product bounds are checked before allocation. [METHOD.md](METHOD.md) gives
the precise arithmetic and lifetime argument.

Old owned storage is `72+D+16N+C`; new is `72+D+8N+A` on measured x64, where D
is namespace length, A useful association bytes, C geometric capacity. A shared
source marker is additional and counted once. For all-empty histories the generic
layout still stores8N end bytes; no minimal-space claim is made. Capture remains
full O(N+A) work and each snapshot owns its own bytes independently.

Association callbacks still run once per occurrence and are copied immediately.
No counting/replay pass is permitted by the contract. Final shrink is a new
fallible preparation point, **before** provenance commit; it is not silently
ignored on OOM. Sources, existing snapshots and revisions remain untouched on
failure. Loaded snapshots still have no source marker. Serialization writes the
same u64 lengths/bytes, never native row structs. Resolve-first ordinal callbacks
remain before core/resident construction. Scratch remains a separate N-pointer
vector; Step 1's resident allocation strategy is unchanged.

## 3. Actual code and bounded decisions

Production edits: private Snapshot header and three existing C modules
(`snapshot`, `snapshot_wire`, `snapshot_restore`). No production source module,
public declaration, status, version, frozen vector or source manifest was added
or removed. The new regression in the existing Snapshot test file checks reused
callback storage, empty spans, exact retained capacity, callback errors, lifetime
and the new final-trim OOM path. `FILES.txt` is the complete inventory.

One representation was implemented, with one limited codec refinement after
initial Linux100k variable serialization was slower. The refinement carries an
end offset forward, avoiding a requested previous-row lookup. A measurement
scheduling bug was then found: case-index rotation plus even-repeat reversal
could cancel. The final collector rotates by repetition only; live guard cases
have four production repeats. All old data remain in the exploratory ZIPs and
readable summary CSVs. They are not mixed into final medians.

Rejected/unimplemented alternatives are reasoned tradeoffs, not measured winners:

- pre-counting or replaying callbacks breaks invocation/immediate-copy semantics;
- retaining caller/wire views breaks independently owned historical bytes;
- caching unchanged-order captures assumes callback purity not promised by4.x;
- shared/COW/deduplicated history adds lifetime/concurrency machinery without a
  demonstrated need for this assignment;
- owned-wire plus an index retains extra wire length words; borrowing the image
  would break lifetime guarantees;
- final flattening into one all-inclusive allocation adds a full copy and peak;
- adaptive-width/compressed indices add lookup/overflow branches; one native end
  already halves row metadata;
- overwriting snapshot rows with resolver pointers violates immutability;
- streaming resident construction or resolver replay changes resolve-first
  observable failure/callback ordering. Separate pointer scratch is retained.

## 4. Environment and evidence controls

Windows11 build26300 x64, Ryzen9 9955HX,16C/32T,16,304,992,256 bytes physical RAM;
GCC16.2.0 UCRT64 Release C17 `-O3 -DNDEBUG`. Strict GCC core validation uses
`-Wall -Wextra -Wpedantic -Werror`. MSVC19.51 x64 additionally validates Debug,
Release and ASan. CMake4.4.3/Python3.13.14 are the local tools.

Final Linux runner: Ubuntu24.04 x86_64, kernel6.17 Azure, **4 vCPUs** presented
with AMD EPYC7763 model,16,766,414,848 bytes physical RAM, GCC13.3.0,
CMake3.31.6/Python3.12.3. All three variants are built with the same C17/O3,
DNDEBUG and strict warning settings before any timing. Earlier Linux rounds used
different EPYC models; cross-round absolute timings are not a causal experiment.
Only same-host/same-round source comparisons are used for final tables.

Initial local RAM was about2GiB; research smoke tests exited on their guard,
not a sanitizer finding. After apps were closed, available RAM recovered, finally
to about8.1GiB. Guards remained: conservative simultaneous envelope, at most40%
of available physical RAM,3GiB cap,3GiB reserve,2GiB in-driver guard. No unrelated
process was terminated. Final100k/1m/3m cases were admitted on both hosts; a fixed
low-retention10m case was additionally admitted on Windows. Variable10m exceeded
the unchanged cap and was safely skipped. Earlier skips are preserved.

Production, allocation-wrapper and private storage probes are separate fresh
processes. Native CRT realloc is used, not a wrapper that always copies.
Instrumentation overhead is not Full requested storage, and private timings are
not production timings. Whole-process WS/RSS includes caller fixtures, allocator,
runtime and instrumentation. It is not Full-only memory or fragmentation.
Requested peaks are API-accounting peaks, not continuous OS transient peaks.

Callbacks and allocation are inside end-to-end capture/restore timing. Oracle
preparation/scans and caller wire allocation are outside. The indirect callback
replay is a separate calibration, never subtracted. Key probes include their
expected-string validation. Timed-phase sums also include calibration/source
phases and must not be relabeled pure history cost. Full process elapsed time is
recorded separately. No significance test, hardware counter attribution or
universal latency/complexity guarantee is claimed.

## 5. Final matched ID8 legacy workload: B / C milliseconds

This is the unchanged prior history driver, with two captures and its own fixed
ID8 associations/namespace. It must not be mixed with the generated-byte driver.
Small-scale medians use5 fresh processes per version;1m uses3.

|Host|Rows|First capture|Second capture|Serialize|Deserialize|Restore|
|---|---:|---:|---:|---:|---:|---:|
|Windows|100,000|1.085 / 0.692|0.786 / 0.511|0.263 / 0.257|0.940 / 0.831|3.046 / 2.779|
|Windows|1,000,000|14.552 / 14.117|14.819 / 14.824|2.762 / 2.900|9.825 / 8.508|33.739 / 33.020|
|Linux|100,000|1.831 / 1.491|1.738 / 1.411|1.088 / 0.960|2.284 / 1.913|3.962 / 4.000|
|Linux|1,000,000|12.686 / 11.959|12.575 / 12.246|5.754 / 4.725|16.677 / 15.705|37.673 / 36.819|

On Windows100k, fresh A/B/C second capture is **0.688 /0.786 /0.511ms**.
The Step 1 slowdown versus A was14.2% in this run, not the earlier27.5% factor;
C lowers the phase35.0% versus B. Linux100k C lowers it18.8%; Windows1m is
essentially flat (+0.03%), Linux1m is2.6% lower. This is measured mitigation,
not proof that the historical regression is universally eliminated.

The capture iterator reads resident items directly and does not perform public
handle owner derivation. Extra owner lookup therefore does not explain the
second-capture regression. Heap/layout/cache/scheduling causes remain incompletely
attributed; row/capacity changes address a separate demonstrated storage cost.

## 6. Generated-byte capture workloads

Total milliseconds spent in all K capture calls per process, then median across
fresh processes; callback generation and copying are included. Model verification
is outside. These are distinct from legacy ID8 and from full process time.

|Host|Rows|K|Association kind|Namespace bytes|B total capture ms|C total capture ms|C/B|
|---|---:|---:|---|---:|---:|---:|---:|
|Windows|100,000|16|eight|16|28.241|27.402|0.970|
|Windows|100,000|2|empty|1|2.005|1.796|0.896|
|Windows|100,000|8|fixed256|16|140.741|137.798|0.979|
|Windows|100,000|2|legacy|2|1.085|0.692|0.638|
|Windows|100,000|2|none|1|1.240|0.629|0.508|
|Windows|100,000|8|one|4096|9.392|9.183|0.978|
|Windows|100,000|16|variable|16|87.389|84.352|0.965|
|Windows|100,000|4|wide|4096|240.513|223.303|0.928|
|Windows|1,000,000|16|eight|16|510.454|448.153|0.878|
|Windows|1,000,000|2|empty|1|31.809|29.161|0.917|
|Windows|1,000,000|4|fixed256|4096|666.512|630.705|0.946|
|Windows|1,000,000|2|legacy|2|14.552|14.117|0.970|
|Windows|1,000,000|2|none|1|15.416|13.977|0.907|
|Windows|1,000,000|4|one|4096|82.310|82.172|0.998|
|Windows|1,000,000|8|variable|16|505.367|486.007|0.962|
|Windows|3,000,000|4|eight|16|363.604|338.871|0.932|
|Windows|3,000,000|2|variable|4096|409.402|406.032|0.992|
|Linux|100,000|16|eight|16|54.034|48.496|0.898|
|Linux|100,000|2|empty|1|3.455|2.700|0.782|
|Linux|100,000|8|fixed256|16|99.942|98.581|0.986|
|Linux|100,000|2|legacy|2|1.831|1.491|0.814|
|Linux|100,000|2|none|1|1.979|1.234|0.624|
|Linux|100,000|8|one|4096|21.995|19.211|0.873|
|Linux|100,000|16|variable|16|111.442|108.234|0.971|
|Linux|100,000|4|wide|4096|144.946|144.762|0.999|
|Linux|1,000,000|16|eight|16|460.436|443.325|0.963|
|Linux|1,000,000|2|empty|1|25.693|23.589|0.918|
|Linux|1,000,000|4|fixed256|4096|364.226|366.735|1.007|
|Linux|1,000,000|2|legacy|2|12.686|11.959|0.943|
|Linux|1,000,000|2|none|1|14.526|11.597|0.798|
|Linux|1,000,000|4|one|4096|91.652|88.403|0.965|
|Linux|1,000,000|8|variable|16|396.286|393.971|0.994|
|Linux|3,000,000|4|eight|16|324.895|312.740|0.963|
|Linux|3,000,000|2|variable|4096|278.733|281.736|1.011|

Absent callbacks (`none`) and callbacks returning empty spans (`empty`) remain
distinct. Other lengths are1/8/fixed256, variable0..128 and wide0..2048 bytes.
First two captures preserve order; capture3 follows a10%-collection rotation when
K>2. Namespace1/16/4096 cases are included. Per-capture min/median/max, useful
bytes, callback counts and keys are in the inclusive phase/comparison CSVs.
Source-marker currentness, immutability and ordinal/byte order checks run for all.

## 7. Retained history, requested peak and association storage

Allocation-only Full bytes, equal on both x64 GCC hosts. History bytes are final
capture live requests minus source-build live requests, including one shared
marker. Peaks include source, histories, loaded state, temporary scratch and a
fresh restored core; they exclude caller wire/items and allocator headers.

|Rows|K|B owned history|C owned history|B whole-lifecycle peak|C whole-lifecycle peak|
|---|---:|---:|---:|---:|---:|
|100,000|16|42,378,632|25,601,416|53,153,552|35,576,336|
|1,000,000|16|390,219,144|256,001,416|497,968,768|355,751,040|
|3,000,000|4|326,218,088|192,000,360|649,467,712|491,249,984|

At100k/K16 the history saving is16,777,216 bytes (**39.6%**); at1m/K16 it is
134,217,728 bytes (**34.4%**). The former exactly accounts for the previously
measured42,378,632-byte cost rather than treating it as a leak. Row metadata is
halved for all cases; published association capacity equals useful A in C.

|Rows / span|Useful A per snapshot|B capacity|C capacity|B row bytes|C row bytes|
|---|---:|---:|---:|---:|---:|
|100,000 / eight|800,000|1,048,576|800,000|1,600,000|800,000|
|1,000,000 / eight|8,000,000|8,388,608|8,000,000|16,000,000|8,000,000|
|1,000,000 / variable|63,999,939|67,108,864|63,999,939|16,000,000|8,000,000|
|1,000,000 / fixed256|256,000,000|268,435,456|256,000,000|16,000,000|8,000,000|
|100,000 / wide|102,389,071|134,217,728|102,389,071|1,600,000|800,000|

Useful association bytes are still copied once per callback span. Reallocation
internal copying is unmeasured. Requested traffic can regress: final shrink adds
an A-sized realloc request, offset by8N fewer row bytes. Fixed-eight traffic is
therefore approximately unchanged; wider spans can increase requested traffic
substantially despite reduced retained memory. These request sizes are not actual
copied bytes. Full-owned history remains proportional to deliberately retained
copies; allocator/OS page retention is separate.

## 8. Actual10m Windows case

Two snapshots, eight-byte associations, namespace16, two fresh production
processes per A/B/C plus allocation/private runs. This is a bounded admitted
upper-scale case, not evidence for all10m histories. The variable-length10m
case was skipped because its envelope exceeded3GiB, even after RAM was freed.

|Phase|B ms|C ms|C/B|
|---|---:|---:|---:|
|capture #0|295.279|276.582|0.937|
|capture #1|297.185|270.296|0.910|
|serialize #0|32.780|27.704|0.845|
|deserialize #0|106.579|88.766|0.833|
|restore #0|395.764|399.095|1.008|
|restore_destroy #0|196.926|198.356|1.007|
|history_destroy #0|14.291|8.598|0.602|

B: owned two-snapshot history **588,435,640 bytes**; whole requested peak **1,665,935,264 bytes**.

C: owned two-snapshot history **320,000,184 bytes**; whole requested peak **1,317,499,808 bytes**.

C saves256MiB in those two retained histories and also saves the loaded row
vector. Association/wire values are exact; these are actual executions, not
extrapolations. There is no10m sanitizer or high-retention/variable10m claim.

## 9. Restore and destruction

Restore still allocates N individual stable resident records, ceil(N/64) blocks,
one core and the separate N-pointer vector. Nonempty allocation count is
`N+ceil(N/64)+2`:101,565 at100k;1,015,627 at1m;3,046,877 at3m;
10,156,252 at10m. B and C counts/temporary bytes match. Resolver order is verified
before fresh-handle core checks. Caller items are never freed by Full.

The pointer scratch remains8N on x64 (8MB at1m,80MB at10m). No claim is made that
all restore memory is scratch or that resident allocation was solved here.
Loaded state/retained histories shrink, reducing total restore-context memory.
Generic restore timings are mixed; legacy1m is2.1% lower on Windows and2.3%
lower on final Linux, while Linux3m eight-byte restore #1 is7.6% slower.

Partial histories are destroyed before the source; surviving histories and a
loaded snapshot are validated after caller wire corruption/free and source/item
destruction. All remaining histories are then destroyed. Every instrumented
process reaches zero tracked Full bytes/blocks. Grouped allocation reduces free
calls but does not guarantee faster destruction: meaningful destructor
regressions are listed below. No accumulating churn leak was observed.

## 10. Live Order guardrails

No changes to accepted Step 1 live-core, managed facade, bulk-resident builder,
allocator or public header. Canonical source hashes and54 paired diagnostic
phases per host agree exactly for allocations/frees, requested resident memory,
comparisons, occupancy, splits/merges/rotations, index/location work and heights.
Four fresh production repetitions per guard case use the corrected order.

|Host|Rows|Managed random growth B/C ms|Random locate B/C ms|Local moves B/C ms|Broad moves B/C ms|
|---|---:|---:|---:|---:|---:|
|Windows|100,000|27.230 / 27.132|25.335 / 25.633|1.839 / 2.162|12.009 / 12.112|
|Windows|1,000,000|690.167 / 672.829|85.664 / 86.230|22.323 / 21.613|101.438 / 104.811|
|Windows|3,000,000|4044.383 / 3926.472|117.543 / 115.667|30.568 / 30.750|150.904 / 152.687|
|Linux|100,000|47.929 / 47.383|46.924 / 46.381|4.605 / 4.686|28.832 / 29.562|
|Linux|1,000,000|1306.918 / 1279.354|168.546 / 157.806|43.327 / 40.832|157.566 / 159.977|
|Linux|3,000,000|6125.496 / 6077.456|206.411 / 238.973|54.538 / 50.845|182.009 / 179.843|

**These timings are not all regressions-free.** Final Linux3m random locate is
15.8% slower; Windows100k local movement is17.5% slower (about0.322ms over the
phase). Windows1m random growth is2.5% lower; Linux1m is2.1% lower. Unchanged
source/work counts do not erase adverse timings or establish their cause.
No hardware/link-layout/scheduler attribution was isolated. The Linux lookup
pocket is an explicit adoption risk for independent review, not an advertised
improvement or a reason to silently tune unrelated live algorithms in Step 2.

Long guard campaigns have two production processes per source/size plus
allocation/private processes,100 cycles of100k remove/reinsert pairs at100k/1m.
Each process executes10m pairs; exact full order/handle checkpoints remain.
Severe1m90%/99% shrink/regrowth and repeated scans also retain all oracles.
Requested resident bytes, blocks and reclamation match B/C. Complete cycle
series, worst observed cycles, process WS/RSS and scan/destruction timings are
preserved. Source-stability checks include fresh handles after restore and live
handle identity after legal moves; expired handles are never probed.

## 11. Improvements, regressions and causal limits

Demonstrated improvements: half-sized row metadata, two fewer initial metadata
allocations, exact published association capacity, smaller retained histories,
smaller loaded snapshots and substantial requested peak reductions. Small matched
legacy capture/decode commonly improves. No wire/key-size reduction is claimed.

Important remaining adverse results (final-order medians):

- Windows100k/fixed256 source destruction2.833 ->5.307ms (+87.3%);
- Windows100k/variable partial-history destruction2.308 ->3.463ms (+50.1%);
- Windows1m legacy serialization2.762 ->2.900ms (+5.0%);
- Linux1m no-association loaded destruction0.151 ->0.299ms (+98.7%);
- Linux1m/fixed256 history destruction4.607 ->5.339ms (+15.9%);
- Linux3m/eight restore #1:127.607 ->137.260ms (+7.6%);
- ordinary guard pockets described above, plus increased requested traffic for
  wide association spans. Full ranges/all phases remain accessible.

The initial Linux100k variable serialization slowdown (+71.8%) and its first
refined/pre-order result (+60.7%) are preserved. Final-order data must not be used
to claim that scalar carry alone cured it: host CPUs and scheduling protocols
changed between epochs. No further production tuning round followed. The generic
representation is selected as a worthwhile **memory/ownership/complexity tradeoff**,
not as universally faster or automatically acceptable for lookup-sensitive users.

## 12. Validation and evidence integrity

GCC C17 strict x64 Debug/Release:39/39 each. MSVC x64 Debug/Release:39/39 each.
MSVC ASan RelWithDebInfo:39/39. New research harness:20/20 under GCC and MSVC ASan;
Linux A/B/C self-tests:20/20 each. Seven public examples and source/FetchContent,
installed C17/C++ and twice-generated deterministic amalgamation consumers pass.
Frozen API remains64 functions/14 types. LS1/LKS4SNP1/LK1 architecture-independent
vectors pass unchanged. GCC Windows ASan/UBSan runtime linking is unavailable;
fresh link-probe errors are retained, not claimed as a pass. Linux Clang
ASan/UBSan and extended campaigns pass through the existing CI.

New tests sweep12 capture allocation failures, including final trim after all257
callbacks, then success; prior snapshots/currentness/source size and live bytes
remain intact. Header/data deserialization failures, existing fresh-source
marker failures, bulk restore and V3 import failpoints all pass. Caller-buffer
serialization consumes no allocation attempt under injected failure. Callback
failure at first/middle/last rows propagates unchanged with no escaped object.
Reused scratch bytes/empty spans, capacity, source destruction and malformed
wire/overflow paths are covered. There is no detected correctness/OOM blocker.

Final-order native processes:314 on Windows (including12 upper runs) and302 on
Linux; two unsafe upper cases are recorded as skips. These provide **5,336**
exact checkpoint fingerprints across states/sources/modes/repeats, plus explicit
full restored-order/fresh-handle checks. Checkpoints are not per-mutation proof.
Earlier data/attempts remain preserved and are excluded from final medians.

Linux research run40163805723 and ordinary CI40163805720 succeeded for protocol
commit `a29e947...` (production identical to7ac34f5). Earlier source/CI rounds are
listed in `evidence/artifact-provenance.json`. Outer artifact ZIP SHA256 and all
inner manifests were independently rechecked locally. Final delivery CI is
reported for the final evidence commit in the completion response.

`SHA256SUMS.txt` hashes canonical committed Git blobs, including changed source
and delivered evidence, except itself. Windows checkout newline conversion is
not part of those hashes. Binary gzip/ZIP bytes are exact. `FILES.txt` is the
complete changed-file inventory. Raw bundles preserve stdout/stderr and exact
commands, repetition/order, UTC and actual admission readings. Re-run analyzer
outputs were compared with delivered summaries. See README/VALIDATION for commands.

## 13. Cost-family disposition

|Cost|Disposition|
|---|---|
|Redundant offset + size per row|RESOLVED in this candidate|
|Historical row storage|PARTIALLY IMPROVED: one word per row, including empty rows|
|Published association capacity slack|RESOLVED at Full requested-capacity level|
|Allocator rounding/internal transient memory|UNAFFECTED / not isolated|
|Separate fixed-field allocations|RESOLVED: one grouped allocation|
|Capture/decode CPU cost|PARTIALLY IMPROVED, workload/host dependent|
|Logical association copying and full capture|INTENTIONALLY RETAINED by ownership/callback contracts|
|Retained full-history proportional cost|PARTIALLY IMPROVED, still K independent copies|
|Wide-span requested allocation traffic|REGRESSED when final A request outweighs row saving|
|Whole-context restore peak|PARTIALLY IMPROVED through smaller historical state|
|Resolver scratch and resident allocation count|INTENTIONALLY RETAINED / UNAFFECTED|
|Published key and wire bytes|UNAFFECTED; frozen contracts|
|Source provenance/concurrency bookkeeping|PRESERVED, no new history sharing|
|Live-core structural/memory behavior|PRESERVED; adverse timing pockets remain|
|Formal universal performance bounds|NOT CLAIMED|

The larger representation change subsumes separate row zero-fill, fixed-field
allocation/free-count and retained-capacity symptoms; no separate micro-tuning
stages are proposed. No data structure redesign, permanent IDs, transactions,
CRDT, delta export or public allocator was added.

## 14. Remaining limitations and handoff

No final project PASS or adoption decision is made. There is no known functional,
OOM, frozen-format or source-compatibility blocker. Performance is mixed and the
Linux3m lookup/destructor pockets require an explicit acceptance judgment. Source
identity is not proof of identical timing. No continuous RSS/allocator-copy/cache
counter attribution, confidence intervals, arbitrary-duration fragmentation or
real-user trace validation is supplied. Sanitizers were not run at10m. Wide/high-
retention10m history is unmeasured; variable10m was safely omitted under the cap.

The branch contains the combined accepted Step 1 and candidate Step 2, final
code/evidence/reproduction instructions. No main/Stable/issue/governance changes,
PR, merge, tag, release or Step 3 integration/acceptance work occurred. One
read-only branch-only research workflow is included and should not be mistaken
for a new production gate. Independent owner/ChatGPT review decides adoption.
