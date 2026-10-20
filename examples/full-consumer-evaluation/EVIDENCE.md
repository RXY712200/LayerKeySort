# Executed evidence and delivery boundaries

Evaluation: 2026-10-20, Windows x64 build 26300, Ryzen 9 9955HX, 16C/32T,
16,304,992,256 bytes RAM. GCC 16.2.0 (MSYS2 UCRT64), MSVC **19.51.36260.0**
(tool installation directory 14.51.36231), CMake **4.4.3**, Python **3.13.14**.
These are fresh local executions, not inherited CI pass claims.

## Exact test matrix

| Configuration | Executed result | Raw evidence |
|---|---|---|
| GCC Debug evaluation | 5/5 CTest | [log](evidence/gcc-debug-ctest.log) |
| GCC Release evaluation, strict C17/warnings | 5/5 CTest | [log](evidence/gcc-release-ctest.log) |
| MSVC x64 Debug evaluation | 4/4 CTest | [combined log](evidence/msvc-complete.log) |
| MSVC x64 Release evaluation | 4/4 CTest | same combined log |
| MSVC x64 RelWithDebInfo ASan, evaluation + library | 4/4 CTest | same combined log |
| MSVC ASan extended generated editor campaign | 3 x 120k steps; 430,529 processed commands matched | same combined log |
| MSVC ASan 100k-resident measurement scenario | All phases completed, no reported sanitizer error | same combined log; not production performance data |
| Official released amalgamation, GCC Release | 5/5 CTest | [log](evidence/distribution-ctest.log) |
| Installed static package/find_package, GCC Release | 5/5 CTest | [log](evidence/installed-ctest.log) |
| Unchanged root Full GCC Release, benchmarks ON | 39/39 CTest | [log](evidence/baseline-gcc-ctest.log) |
| Unchanged root Full MSVC x64 Release, benchmarks ON | 39/39 CTest | [log](evidence/baseline-msvc-ctest.log) |
| Seven root public example executables, GCC Release | All exited 0 | [stdout](evidence/examples.log) |
| GCC Release extended generated editor campaign | 3 x 120k steps; 430,529 commands matched | [log](evidence/long-model.txt) |
| GNU external allocation failpoints | 279 OOM points across 18 cases, then success; zero requested cleanup | GCC/ZIP/installed logs |
| Fresh GNU sanitizer configuration/build attempt | Link failed: missing -lasan/-lubsan | [failure log](evidence/gcc-sanitizer.log) |
| Diff whitespace/scope review | Passed; only isolated Full evaluation subtree added | Git diff/commit inventory |

MSVC omits only the GNU linker-interception test, not a silently passing
replacement. Its four tests are public contracts, regular model, large document,
and domain workflow. GCC adds the allocation-failure test as test five.
The regular model contributes 42,289 processed commands over three 12k-step seeds
(counts 14,092 / 14,064 / 14,133). Extended seed counts are 144,369 / 143,478 /
142,682. The independent large-document scene is 5,123 commands at 2,048 peak
residents. No human terminal-session evidence is claimed.

The root 39-test runs include API/fixture, hostile-input, OOM, marker, C/C++
consumer, integration and benchmark-smoke gates as named in the complete logs.
They complement the consumer; existing tests were not edited or substituted
for the independent application.

### Commands and build locations

All paths below are outside the checkout except the source directory. Actual
commands used the absolute checkout for FULL_SOURCE. For example:

```text
cmake -S examples/full-consumer-evaluation -B ../full-eval-build-gcc-release -G "MinGW Makefiles" -DFULL_SOURCE=<checkout> -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build ../full-eval-build-gcc-release -j 8
ctest --test-dir ../full-eval-build-gcc-release -V
python -B -X utf8 examples/full-consumer-evaluation/exercise.py ../full-eval-build-gcc-release/full_editor.exe --steps 120000
python -B -X utf8 examples/full-consumer-evaluation/collect.py ../full-eval-build-gcc-release ../full-eval-evidence/delivery-measurement
```

Debug used `../full-eval-build-gcc-debug`. MSVC commands ran inside
`D:\vs\Common7\Tools\VsDevCmd.bat -arch=x64 -host_arch=x64`, with NMake Makefiles
and `../full-eval-build-msvc-debug`, `...-msvc-release`, `...-msvc-asan`.
ASan used `RelWithDebInfo`, `-DEVAL_SANITIZE=ON` (compile `/fsanitize=address /Zi`,
link `/INCREMENTAL:NO`). Ordinary MSVC Debug uses CMake `/Od /RTC1`; Release
uses `/O2 /Ob2 /DNDEBUG`. Evaluator MSVC sources add `/W4 /WX`.

Root baselines used `../full-baseline-gcc-release` and
`../full-baseline-msvc-release`, Release with `LKS_BUILD_BENCHMARKS=ON`.
The root GCC build additionally used the strict warning flags above.
Root installation ran `cmake --install ../full-baseline-gcc-release --prefix
../full-eval-install`. The installed consumer used that prefix, no FULL_SOURCE.
The ZIP consumer set FULL_AMALGAMATION to the downloaded extracted distribution,
no source checkout includes. Both build/test logs are preserved.

## Artifact ownership and hashes

**Pushed:** evaluation source and reports; `evidence/` contains raw aggregate
timing rows for all 15 retained fresh processes, three full discarded warmups,
three independent allocation runs, summary, metadata and complete selected test
logs. No per-repeat favorable selection occurred. `summary.csv` is derived from
the raw rows; redundant individual non-warmup CSV copies remain local.

[`evidence/SHA256.json`](evidence/SHA256.json) hashes exact committed evidence
bytes. [`metadata.json`](evidence/metadata.json) preserves collection-time C
source hashes and provenance. A final contracts.c cleanup removed only an extra
blank line at EOF after capture; the original collection hash and final hash
are both retained. No performance implementation changed after collection.
Git may normalize text line endings; the evidence hashes describe committed
canonical LF bytes, and the local-artifact manifest separately covers local bytes.

**Local only:**

```text
C:\Users\rxy71\Documents\Codex\2026-10-06\new-chat-2\full-eval-evidence\
```

This contains the downloaded official ZIP/extracted five-file package, complete
original issue JSON snapshots, full compiler logs, earlier superseded trial
captures, the complete delivery capture including individual per-repeat CSVs,
and `ARTIFACT_SHA256.json` for local raw artifacts. That manifest's digest is
reported in the final delivery response. Sibling build directories contain
generated binaries/caches and are deliberately not committed. They are
rebuildable intermediates, not additional validation results.

Official ZIP:
`LayerKeySort-4.0.0-amalgamation.zip`, 33,993 bytes,
SHA-256 `ed828c547cba05e39d05e28c9f6eaec7ffd325839d291713b7cfe0e47bef1a1d`.
It matched the published asset digest and exact five-file membership. No release
asset was replaced or republished. Download instructions and hash gates did not
need an undocumented workaround.

[`baseline.json`](evidence/baseline.json) records exact source/tag identities,
public header digest and retrieved Issue body hashes/update dates/comment counts.
Issue snapshots are evidence of the reviewed state, not frozen GitHub issues.
The live issues were not edited.

## Missing or deliberately excluded evidence

- No fresh Clang/Linux/macOS/Win32 consumer execution. WSL has no Linux distro;
  Clang is not on PATH or in the installed VS LLVM location.
- GCC ASan/UBSan unavailable as linked runtimes. MSVC ASan succeeded; UBSan/LSan
  were not thereby performed. No universal leak-free claim from ASan alone.
- No real customer/editor adoption, independent human usage/session review or
  product acceptance. Independent model logic is not an independent evaluator.
- No unsupported dangling-handle probing, mutable-container race campaign,
  exception/longjmp escape tests, allocator metadata attribution, power-loss
  storage durability or practical SIZE_MAX/revision exhaustion.
- Operation-count stress is reproducible, but not days-long uptime or a
  general fragmentation/latency bound. The performance driver does not consume
  private block counters; block layout explanations come from separate source
  inspection, not hidden application dependencies.

These gaps are not passing tests and are not automatically product defects.
