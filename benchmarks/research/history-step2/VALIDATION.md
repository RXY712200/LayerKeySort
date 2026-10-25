# Validation and reproduction

Final production code: `7ac34f54dd7c7c544605efd0f8d97dbc4f02cd12`.
Final process-order protocol: `a29e9472aebd4a3b7c0ec8cd50d94d5337662ed1`.
The enclosing evidence commit changes neither production code nor the protocol.

## Functional builds

Run from the candidate checkout, with build directories outside it:

```sh
cmake -S . -B ../verify-gcc -DCMAKE_BUILD_TYPE=Release -DLKS_BUILD_BENCHMARKS=ON -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build ../verify-gcc --parallel 4
ctest --test-dir ../verify-gcc --output-on-failure
```

The completed strict GCC Debug/Release and MSVC x64 Debug/Release configurations
each passed 39/39. MSVC RelWithDebInfo with `/fsanitize=address /Zi` and
`/INCREMENTAL:NO` passed 39/39. Run MSVC CTest from its developer environment.
Research harness self-tests passed 20/20 for GCC and MSVC ASan, and Linux A/B/C
passed 20/20 each. The seven public examples and distribution source,
FetchContent, installed C17/C++ and deterministic amalgamation consumers passed.
Actual commands and outputs are preserved in `evidence/validation/` and the
Linux workflow logs. The initial research ASan resource rejection remains in
the archive alongside its successful guarded retries; it is not a hidden pass.
Windows GCC ASan/UBSan runtime libraries were unavailable. Existing Linux Clang
sanitizer CI supplies that coverage; no 10m sanitizer claim is made.

## Matched research reproduction

Use the A/B/C exact roots in README and REPORT. Build all three first; do not
run production timing and instrumented accounting concurrently. The collector
runs fresh processes in its fixed rotating order and records commands, UTC,
repetitions, actual available memory, admission status and stdout/stderr.

```sh
python3 benchmarks/research/history-step2/collect.py --help
python3 benchmarks/research/history-step2/collect.py ../history-a ../history-b ../history-c ../capture-history --suite history
python3 benchmarks/research/history-step2/analyze.py ../capture-history ../analysis-history
```

Repeat with `--suite guard`, `--suite long` and `--suite shrink`, using separate
capture and analysis directories. On Windows, `resource_followup.ps1` takes
`-OriginalCapture`, `-BuildA`, `-BuildB`, `-BuildC`, `-Output`: it preserves
original skips, retries them once and adds the bounded 10m upper cases. Use an
original final-history capture, not an upper directory containing prior retries.
The guard remains min(3 GiB, 40% available), with a 3 GiB admission reserve,
2 GiB runtime floor and 360-second process timeout. A skipped case is not a
measurement. Do not lower guards to reproduce an unsafe case.

Each capture directory has `runs.jsonl`, `environment.json`, `raw.jsonl.gz`.
The analyzer reads either individual raw files or this compact bundle. Re-run
it into a fresh directory and compare outputs with delivered analysis. A timed
phase total includes calibration/source work; it is not pure history time.

## Evidence checksums and source identity

`SHA256SUMS.txt` covers canonical committed Git blobs rather than platform-
converted checkout text. Verify with Python from the repository root:

```python
import hashlib, subprocess
from pathlib import Path
base = Path('benchmarks/research/history-step2')
for line in (base/'SHA256SUMS.txt').read_text().splitlines():
    digest, path = line.split('  ', 1)
    blob = subprocess.check_output(['git', 'show', 'HEAD:'+path])
    assert hashlib.sha256(blob).hexdigest() == digest, path
```

The manifest excludes itself. `FILES.txt` is the entire Step 2 diff from the
accepted base, including the manifest. Outer Linux exploratory ZIPs are exact
GitHub artifacts; the final Linux artifact is unpacked with its own 44-file
SHA256SUMS and source manifests. Verify that inner manifest against Git blobs
under `evidence/linux-final/` to avoid checkout newline conversion. Raw binary
ZIP/gzip bytes are preserved exactly. Provenance JSON identifies all three
research run IDs, artifact IDs and downloaded outer SHA256 digests.

No benchmark is a human acceptance record. No result establishes a universal
speedup, formal bound, arbitrary-duration allocator behavior or permission to
merge. Independent review decides whether the memory tradeoff and timing risks
are acceptable.
