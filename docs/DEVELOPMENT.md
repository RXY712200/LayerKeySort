# Developing LayerKeySort

## Repository layout

- `include/` — public header.
- `src/` — production C implementation and private headers.
- `tests/` — property, stress, benchmark, and public API validation code.
- `examples/` — standalone public API example.
- `demo/` — executable validation/demo entry point.
- `docs/` — user documentation, visualizer, and site assets.
- `LayerKeySort.slnx` — Visual Studio solution.
- `LayerKeySort.vcxproj` — Visual Studio project and build configurations.
- `CMakeLists.txt` — reusable production library, example, and test targets.
- `.github/workflows/ci.yml` — MSVC, GCC, and Clang validation.

## Toolchain

Both Visual Studio and CMake compile as C17. The CMake matrix also targets
GCC and Clang on Ubuntu. New production C source belongs in both build systems.

## CMake build and tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

`layerkeysort` contains production source only. `layerkeysort_example` links
that library. `layerkeysort_tests` links a separately compiled diagnostic
variant so allocation counters and single-shot fault injection remain usable
without shared mutable instrumentation in ordinary production allocations.
For a supported GCC or Clang toolchain, `-DLKS_ENABLE_SANITIZERS=ON` enables
AddressSanitizer and UndefinedBehaviorSanitizer. Diagnostic fault injection is
process-global test state and carries no concurrent-test guarantee.

## Build configurations

The Visual Studio project defines **Debug**, **Release**, and **ASan** configurations for x64. Open `LayerKeySort.slnx` in Visual Studio and select the configuration and x64 platform before building.

## Running the validation runner

The Visual Studio executable entry point is `demo/main.c`. With no argument it
runs the V2 focused tests, public API smoke, deterministic property tests,
stress tests, and OOM tests. The CMake test executable uses `tests/v2_main.c`
for the same preview suite. `tests/benchmark.c` preserves the historical v1
benchmark evidence and remains available through its existing focused runner
switches; its exact v1 baseline expectations do not describe V2 Path output.

The entry point also handles the existing focused arguments below; these spellings are read from `demo/main.c`:

- `--stage14.1-only`
- `--stage14.2-only`
- `--stage14.2-oom-only`
- `--stage14.2-release-smoke`
- `--stage14.3-frozen-smoke`
- `--public-api-usage-smoke`

Pass an argument through Visual Studio’s Debugging / Command Arguments setting, or run the built executable with that argument. The project is configured as a console application and its output name follows the project configuration. These runner options are development validation hooks, not a stable public command-line interface.

## AddressSanitizer note

On Windows, the ASan runtime DLL `clang_rt.asan_dynamic-x86_64.dll` may need its MSVC runtime directory present in the current process `PATH` when launching an ASan-built executable. This is an environment/runtime lookup note, not a LayerKeySort defect. For a one-off PowerShell launch, prepend the appropriate installed MSVC runtime directory to the current process only:

```powershell
$env:PATH = "<MSVC runtime directory>;$env:PATH"
./path/to/LayerKeySort.exe
```

This does not change the persistent system PATH. Do not obtain runtime DLLs from unofficial download sites.

## Validation coverage

The repository contains public API smoke coverage, deterministic property tests, stress tests, allocation-failure and out-of-memory tests, and frozen regression checks. The Visual Studio project provides Debug, Release, and AddressSanitizer builds. The committed project also contains older development entry points and benchmark code; their presence does not make them public API.

## Coding constraints

Preview.3 uses the full public slot domain `0..65535` (65,536 values) while
retaining the compact numeric `uint16_t` slot array and aligned `size_t` level
array in one Path step allocation. The public API accepts/returns slots as
`unsigned int`; constructors and append operations validate the range before
narrowing. The `uint16_t` representation covers the complete public domain.
Path storage grows with step count and capacity; formatted text is generated
on demand and is never stored in each Path.

`src/slot_codec.c` owns the fixed-width three-character radix-54 conversion.
Its exact rank-ordered alphabet is
`23456789ABCDEFGHJKMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz`:
`2..9`, then allowed uppercase letters, then allowed lowercase letters.
The digits are **codec ranks**, so `'2'` is rank zero. Their ASCII byte order
matches rank order, enabling token-only lexical monotonicity without locale
rules. The codec omits visually confusable `0/O/o` and `1/I/i/L/l`; it uses no
punctuation so tokens are easier to copy through logs, URLs, configuration,
and shells, and `/` remains a structural separator. See [API.md](API.md) for
the complete character order and public text contract.

`src/path_text.c` alone formats the Path text: one direction character,
three codec characters per step, one `/` between steps, and minimal decimal
metadata for a nonzero first absolute level or later level delta greater than
one. `/` is a separator, never the old unary level count. The formatter has
one canonical output per valid Path, but no public parser, stable persistence
format, or cross-Preview text compatibility is defined. A zero-based chain
with consecutive levels uses exactly `4 * depth` text characters. In the
general case, add the decimal digits of emitted level metadata to that
per-step cost. This is separate from internal numeric Path storage.

| Depth | Preview.2 text length | Preview.3 text length |
| ---: | ---: | ---: |
| 1 | 3 | 4 |
| 2 | 6 | 8 |
| 4 | 15 | 16 |
| 8 | 45 | 32 |
| 16 | 153 | 64 |
| 32 | 561 | 128 |
| 64 | 2145 | 256 |

These verified consecutive-level examples show a small shallow-text cost
and shorter deep text. Skipped levels cost decimal metadata digits rather
than a number of slashes proportional to the level. The focused tests also
format a first level of `SIZE_MAX` with decimal metadata. Levels are
`size_t`; later levels must be strictly increasing and their deltas positive,
so a step at `SIZE_MAX` cannot have a following step.

`src/lks_policy_internal.h` owns policy values, separate from Path storage
and text encoding. Its sparse bulk rule is integer division
`((index + 1) * 65536) / (blocks + 1)` for block indexes `0..blocks-1`.
It distributes roots through the full coordinate space and leaves both
endpoints unused. With one block, the slot is 32768. Bulk recursion uses at
most 26 child blocks, a topology choice independent of the codec's radix 54;
the goal is insertion room, not slot exhaustion. The initial online slot is
also 32768. Endpoint target spacing stays 10. Preferred/hard depths 4/6,
local windows 8/16/32/64, minimum repair gain 1, and minimum useful spacing
2 remain the Preview.2 values. This isolates the widened coordinate domain
before any heuristic retuning; these constants are provisional.

### Observed Preview.3 effect

| Ascending online, N=1024 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| Maximum depth | 4 | 1 |
| Repair attempts | 563 | 0 |
| Local successes | 38 | 0 |
| Abandoned repairs | 525 | 0 |
| Accepted deeper Paths | 520 | 0 |
| Full rebuilds | 5 | 0 |
| Peak live bytes | 210176 | 90200 |

| All-equal online, N=128 | Preview.2 | Preview.3 |
| --- | ---: | ---: |
| Maximum depth | 5 | 1 |
| Local repairs | 5 | 0 |
| Peak live bytes | 22760 | 11352 |

The wider slot domain substantially reduced congestion in these measured
workloads. Forced local-repair and fallback regression cases still matter.
These samples do not establish that every workload stays at depth one, that
repair is obsolete, or that worst-case complexity has been solved.

### Historical merge code

Current public V2 `lks_group_merge` stably merges the ordered item sequences,
placing comparator-equal Base items before Incoming items, then builds a new
Group with fresh Paths. Neither source Group changes. The private historical
v1 owned-base helper instead preserves an exclusively owned Base object and
plans Incoming coordinates; it remains only to compare against v1 regression
evidence. `tests/property.c` and `tests/benchmark.c` exercise it under
`LKS_ENABLE_V1_REGRESSION_HELPERS`. The ordinary CMake production library
excludes that gate; the separate diagnostic target and Visual Studio test
configurations enable it. Symbol inspection found the helper absent from the
production library and present in the diagnostic library. It is not public API.

The online repair policy lives in `src/lks_policy_internal.h`. The
smallest eligible complete positive subtree is rebuilt off-Tree with the same
sparse bulk layout; enclosing ancestors are considered within a fixed node
limit. The root Path of the selected subtree remains fixed. All new Paths,
nodes, and child storage are ready before an allocation-free pointer splice.
The full rebuild remains the final fallback. Repair counters are process-wide
only in diagnostic builds; normal production builds do not update them.

- Keep implementation in portable C17 under both build systems.
- Preserve public ownership and ordering semantics; generated Path coordinates
  can change when the preview allocator policy changes.
- Preserve stable ordering and the documented ownership rules.
- Keep relevant regression coverage passing when behavior changes.

## Per-Preview documentation review

Before every future Preview commit or release, review and update as applicable
`README.md`, `docs/API.md`, `docs/USAGE.md`, `docs/DEVELOPMENT.md`,
`CHANGELOG.md`, and the active roadmap GitHub Issue. The review is mandatory
even when a file needs only a small change or none. When useful, leave a
completed Preview implementation-result comment on the active roadmap Issue,
especially if a planned problem was solved, an assumption changed, an issue
was superseded, measurements changed technical-debt priority, or work was
deliberately deferred. Review before posting; documentation work itself does
not imply a release.

## Line endings

Do not normalize line endings as part of unrelated work. Windows Git configurations may report LF/CRLF conversion notices; review whether a change is substantive before altering files.
