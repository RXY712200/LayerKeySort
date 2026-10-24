# Source, artifacts and executed validation

## Baseline/source identities

Main `043872eb5560e867aa7f8c094e0e9dbf2e8a7b95`; Full Stable
`80004ae732daf696f7e9192e417de0edb0b2aafa`; prior consumer
`bf27502f18de5988cd2d1c797ddc3e0ff48d0b03`.

`evidence/baseline.json` records source hashes and the empty Stable-to-main
production diff. `source-provenance.json` records physical and canonical-LF
research-source digests and the measured executable hashes. Binaries are local,
not committed; rebuilt PE hashes may differ with toolchain/build metadata.
The exact committed research SHA is supplied in the final delivery response.

All recorded collection-time source hashes still matched after measurement.
Analysis/docs added later are not retroactively claimed as timing inputs.
The production source/header/root CMake, private policy B64, API/wire fixtures,
existing tests, CI, governance, tags/Releases and other product files are unchanged.
The private build uses the library's existing diagnostic macro, not a patched
algorithm. No optimization candidate is implemented.

## Changed-file inventory

All additions are under `benchmarks/research/full-scale-cost/`:

| File(s) | Role |
|---|---|
| CMakeLists.txt | Standalone public/wrapped/private targets and safe small tests |
| study.c | Fixed-scale growth/edit/managed/history driver, public API calls |
| oracle.c, oracle.h | Separate integer-ID linked sequence |
| oracle_probe.c, oracle_test.py | Python list versus standalone C-oracle validation |
| probe.c, probe.h | Explicit private diagnostic adapter, or public-target stubs |
| wrap.c | Separate GNU external allocation instrumentation, real realloc |
| long_churn.c | Focused 100-cycle turnover driver |
| collect.py | Sequential scale admission, pilot dependency, repetitions and raw capture |
| collect_long.py | Bounded long-campaign collection |
| retry_deferred.py | One retry of admission deferrals under unchanged gates |
| analyze.py | Raw aggregation, digest matching, normalized tables |
| README.md, METHOD.md | Exact build/run/workload/measurement specification |
| REPORT.md, RESULTS.md | Consolidated A–S assessment and all measured result tables |
| CANDIDATES.md | Root-cause map, candidates/tradeoffs/compatibility and future work order |
| PROVENANCE.md | This inventory, validation and evidence boundary |
| evidence/observations.csv | All 1,161 final matrix phase observations |
| evidence/summary.csv | Derived primary seed-17 non-warmup min/median/max summaries |
| evidence/checkpoints.csv | All 848 exact matrix checkpoint receipts |
| evidence/process-final.csv | Complete process cleanup/after-fixture working-set observations |
| evidence/runs.json | Every admission, deferral/retry, command, seed, duration and exit status |
| evidence/long-churn.csv | All 800 long-cycle observations |
| evidence/long-checkpoints.csv | All 96 long-campaign checkpoint receipts |
| evidence/long-runs.json | Eight long-process admission/status records |
| evidence/environment.json, baseline.json, source-provenance.json | Environment and source/measurement identity |
| evidence/build-flags.json | Actual generated production/diagnostic/MSVC-ASan compiler flags |
| evidence/ctest-release.txt | Complete 12-test research Release run |
| evidence/msvc-asan.txt | Safe-size research ASan build and 12-test run |
| evidence/msvc-asan-100k.txt | 100k append/history and 1m churn-pair ASan run |
| evidence/baseline-ctest.txt | Unchanged Full regression suite, 39/39 |
| evidence/gcc-sanitizer-unavailable.txt | Fresh standalone link probe: missing ASan/UBSan runtimes |
| evidence/SHA256.json | Exact canonical committed evidence digests |

No existing file is edited. The final Git commit inventory is authoritative.
Raw evidence is compact (under 1 MiB before adding this documentation); individual
per-process CSV/stderr copies are not redundantly committed alongside aggregates.

## Observed validation

- GCC 16.2.0 C17 Release strict warning-as-error build: 12/12 research CTest.
- MSVC 19.51.36260.0 RelWithDebInfo AddressSanitizer: 12/12 research CTest.
  The generated Full-library flags include `/fsanitize=address /Zi`, not only
  the driver. Extra 100k append/history and 100k x 10-cycle turnover runs exited
  successfully with no reported ASan error. No huge sanitizer run was attempted.
- Unchanged root Full GCC Release with benchmarks ON: 39/39 CTest, including
  API/wire compatibility, consumer, hostile-input, OOM and benchmark-smoke gates.
- 158 final matrix processes including 10 warmups; eight long-campaign processes;
  no failed execution or child budget abort. Six initial admission deferrals
  later completed on retry; history at 3m/10m remains deliberately omitted.
- Every allocation/private final library teardown reached zero requested bytes
  and blocks. Production correctly reports those counters unavailable.
- Oracle counts and full sequence digests match across modes/repetitions.

GCC ASan/UBSan linking is unavailable (`-lasan`, `-lubsan` missing), freshly
checked with an independent one-line C program. No fresh Clang, Linux/macOS,
Win32, UBSan or LSan result is claimed. Safe-size MSVC ASan is not a leak theorem.
No entire assignment acceptance/PASS is asserted by these individual test results.

## Retained local artifacts

```text
C:\Users\rxy71\Documents\Codex\2026-10-06\new-chat-2\full-scale-evidence\
```

- `capture/`: separately identified exploratory pilot, including its older
  sampler source hashes. Those timings do not populate the final tables.
- `final-capture/`: every original final stdout/stderr, environment and JSONL
  run/admission ledger, including all six initial deferrals and their retries.
- `long-capture/`: every long-run stdout/stderr and raw admission/status records.
- Original Issue #3/#4 bodies/comments, baseline, full compiler logs, validation
  logs and `ARTIFACT_SHA256.json` for local raw evidence remain here.
- Sibling Release/ASan/baseline build directories contain generated executables,
  archives and caches. These reproducible intermediates are not committed.

The final response supplies the local raw-artifact manifest SHA-256. Tracked
evidence uses canonical LF; the local manifest separately hashes original bytes.
Source hashes distinguish physical/canonical text where relevant.

### Metadata limits and corrections

Pilot sampler density/lookup aliasing was corrected before recollecting the
entire final matrix. An initial CSV printf-format build error was corrected
before measured collection; missing initial executables were Not Run, not Full
failures. The final strict build/tests are the recorded validation.

Retry records intentionally preserve the original admission fields through
`retry_of`; the first implementation copied `reason` and `time_utc` as well.
Those two fields describe the original deferral, **not the retry's UTC start**.
The compact exported ledger explicitly annotates this limitation. Actual retry
command, physical memory before/after, process duration, output and exit status
are recorded. No precise retry start timestamp is fabricated or retroactively
inferred from file times. Measurement values are not replaced or adjusted.

Measured working-set samples are not continuous peaks. Process-final rows are
after app fixture cleanup; library_destroy phase rows still have app fixtures.
No subtraction is labelled allocator metadata/fragmentation. All inherited
measurement conditions and limits are described in METHOD.md.

No PR, Issue update, main merge, tag or Release is part of delivery.
