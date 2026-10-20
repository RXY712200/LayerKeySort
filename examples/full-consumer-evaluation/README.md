# Full consumer evaluation

Standalone C17 layer-document editor and evaluation of **Full v4.0.0 Stable**.
This is Codex-authored executable integration evidence, not independent human
adoption, customer feedback, or ChatGPT acceptance. No production changes.

Read [REPORT.md](REPORT.md), [PERFORMANCE.md](PERFORMANCE.md),
[HISTORY.md](HISTORY.md), and [DEPENDENCIES.md](DEPENDENCIES.md).

## Build and run

From a source checkout root, PowerShell with GCC/CMake on PATH:

```powershell
cmake -S examples/full-consumer-evaluation -B ../full-consumer-build `
  -G "MinGW Makefiles" -DFULL_SOURCE="$PWD" -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build ../full-consumer-build -j 8
ctest --test-dir ../full-consumer-build --output-on-failure
../full-consumer-build/full_editor.exe
```

Tested official alternatives:

- `-DFULL_AMALGAMATION=<directory containing released layerkeysort.c/.h>`:
  downloaded official v4.0.0 five-file ZIP, verified SHA-256, built and tested.
- Omit both source settings and use `-DCMAKE_PREFIX_PATH=<installed prefix>`:
  `find_package(LayerKeySort 4 CONFIG REQUIRED)` and exported target.
- `FULL_SOURCE` uses documented `add_subdirectory` with tests/examples/install OFF.

All application sources include only the public header. No private functions or
structures. GNU allocation wrappers are separate executables, never the editor.
The CMake file is standalone; it is not wired into root CMake or CI.

For MSVC use a Developer Command Prompt, `NMake Makefiles`, and Debug or Release.
`-DEVAL_SANITIZE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo` enables MSVC ASan or
GCC/Clang ASan/UBSan where those runtimes exist. This machine lacks GCC sanitizer
runtimes; see executed evidence. MSVC allocation-wrapper targets are unavailable,
not silently replaced by no-op tests. The collection driver currently uses
Windows `.exe` filenames. No fresh Linux/macOS execution is asserted.

## Layer document workflow

```text
rename 0 Backdrop
rename 1 Title
visible 0 0
add 0 back
add 1 back
add 0 back
capture 0
move 3 front
remove 1
show
restore 0
show
quit
```

The application owns 256 mutable layer objects (name/visibility/identity). Each
insertion creates a distinct occurrence ID; two occurrences can share one layer.
`show` displays current layer state; `list` displays occurrence order. `help`
lists commands. Before/after commands use occurrence IDs, not object IDs.
Retired IDs are rejected by the application without probing freed handles.

`capture <slot>` keeps historical occurrence order. `save <slot> <file>` writes
LKS4SNP1; `load <slot> <file>` validates and loads it; `restore <slot>` replaces
live ordering with fresh handles. File paths cannot contain spaces in this small
command grammar. Run saves from a scratch/build directory, not the checkout.

**Scope:** saving stores ordering associations, not object payloads, names,
visibility, or an application database. Loading/restoring requires this session's
application-owned identity registry. This is not cross-session document recovery.
Each captured state uses a distinct process-local namespace. A deployed editor
must supply globally appropriate persistent document/state domain identities
before mixing LS1 keys from different sessions. Namespace uniqueness and business
identity are application responsibilities, not an LS1 discovery mechanism.

Limits belong to this consumer: 4,096 live occurrences, 262,143 issued IDs,
32 history slots, 1,023 command bytes, 16 MiB input file. They are not Full limits.
Malformed application files/commands leave live state unchanged. File writes are
buffered writes, not atomic replacement, durable save, or crash recovery.

## Evaluation components

| File | Responsibility |
|---|---|
| editor.c | Application objects/IDs, Full adapter, historical/file workflow |
| model.c/.h | Independent flat occurrence sequence; no library dependencies |
| exercise.py | Second, Python list model; generated command semantics and domain scene |
| contracts.c | Managed/Group/Batch/sort/cursor/LS1/wire/migration/error scenarios |
| measure.c | Separate public-API timing and working-set driver |
| allocation.c/.h | GNU linker interception, requested bytes/failpoints; research only |
| failures.c | Prepare/fail/recover sweeps and managed edit recovery |
| collect.py | Fresh-process warmups/repeats, raw CSV/metadata/hashes |

The C model predicts transitions before adapter calls. Python generates expected
sequences without querying Full. Each editor command checks traversal, size,
endpoints, resident identity, reverse order, all pairs for <=32 residents and
deterministic sparse pairs otherwise. Large-document coverage reaches 2,048 live
residents; measurement validation reaches 100,000. The latter is not a full
independent per-operation large-document oracle.

```powershell
python -B exercise.py <full_editor.exe> --steps 120000
python -B exercise.py <full_editor.exe> --large
python -B collect.py <Release-build-directory> <new-output-directory>
```

Run these scripts using this directory's absolute script paths when elsewhere.
Mismatch command prefixes are preserved only on failure. No mismatch occurred.
Reproducers are generator version + seeds + step counts, not copied huge logs.

The [evidence directory](evidence/) contains compact raw data and test logs.
Large builds, official ZIP, intermediate captures and complete compiler logs stay
outside the repository. [EVIDENCE.md](EVIDENCE.md) records exact locations/hashes.
