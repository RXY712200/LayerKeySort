# Development — v1.0.0-preview.3

Requirements: C17 compiler, CMake 3.16 or later and a matching build tool. No
third-party runtime libraries or sibling project sources are needed. The static
library target is `layerkeysort_mini`. CMake adds conventional compiler warnings;
the public test links that target, and the example includes only its public header.

From the Mini directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Windows MinGW PowerShell, when the toolchain is on PATH:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\lks_mini_basic.exe
```

Visual Studio uses its default generator and `--config Debug`; CMAKE_BUILD_TYPE
is for single-configuration generators. An optimized check may use a separate
Release build directory. Sanitizers can be enabled via compiler/linker flags on
platforms whose toolchains support them; do not assume sanitizer availability.

## Tests

- `mini_public`: lifetime, queries, all insertions/removals, forward/reverse order,
  size, stable live handles, duplicate/NULL payloads, NULL arguments and outputs,
  defaults, boundaries and valid foreign handles.
- `mini_internal`: private invariants after legal mutations, creation and basic
  insertion OOM, synthetic capacity limit, unchanged links on failure, no
  allocation on removal, allocation balance and caller-owned stack payloads.
- `mini_example`: build and execute the public API example.

CHECK macros stay active in Release, independent of NDEBUG. Internal source
inclusion and allocation substitution occur only in the isolated test target.
Do not introduce public diagnostic hooks. Never dereference a freed handle in
tests. Preview.3 adds systematic fault injection and an independent randomized model;
sanitizer execution depends on actual toolchain availability.

## Standalone extraction check

Copy the whole Mini directory, including LICENSE, to a new location outside the
parent repository, then configure/build/test the copy with a separate build
directory. Example PowerShell from a parent checkout (use new destination paths):

```powershell
Copy-Item -LiteralPath .\mini -Destination C:\Temp\lks-mini-preview1 -Recurse
cmake -S C:\Temp\lks-mini-preview1 -B C:\Temp\lks-mini-preview1-build -DCMAKE_BUILD_TYPE=Debug
cmake --build C:\Temp\lks-mini-preview1-build --config Debug
ctest --test-dir C:\Temp\lks-mini-preview1-build -C Debug --output-on-failure
```

All relative links and source references resolve inside the copied directory.
The test-only `../src/lks_mini.c` reference stays inside Mini.

## Contributions

Keep production simple, preserve the [contract](CONTRACT.md), and inspect the
complete diff before committing. Preview.3 changes are confined to this project
directory. Run ordinary and extracted builds/tests; record actual toolchain and
results. Do not advertise unavailable APIs. The remaining milestones are RC.1 integration and Stable validation. Each requires separate authorization.

Preview.2 extends public tests with explicit movement sequences, endpoint and
self/adjacent no-ops, NULL/foreign inputs, payload/handle/size preservation and
all-pair comparison signs including antisymmetry and ordered-triple transitivity.
Private tests check invariants and zero allocation/free counter deltas across
actual moves, no-ops, comparisons and errors. Earlier OOM/capacity tests remain.

## Preview.3 reproducible verification record

`mini_model` links the actual library and public header, with no source inclusion
or private-field access. Its dynamic flat-array model tracks occurrence IDs,
production handles, payloads and owners; position is the array index. It checks
statuses, complete forward/reverse sequence, size, payloads and live identity
after each operation and also checks untouched orders for isolation.

The deterministic PRNG is xorshift32 with shifts 13, 17, 5 in unsigned 32-bit
arithmetic. Seeds: `0x12345678`, `0x9e3779b9`, `0xc0ffee01`, `0xdeadbeef`.
Each seed runs 6,000 operations over three orders (24,000 total), with 0..48
active occurrences per order. Operation types rotate through four insertions,
removal, four moves, first/last/next/prev/item and comparison; random choices
select orders, live occurrences and NULL/duplicate payloads. At the bound,
insertion slots become removals. Every type is exercised and counts are printed.

Every 31 operations, collections of at most eight receive all-pair comparison
checks; larger collections receive eight deterministic pairs. Antisymmetry and
ordered-triple transitivity are checked. Adversarial cases repeatedly insert at
head/tail or alternate ends, repeatedly move one occurrence across the collection,
exercise self/adjacent no-ops and drain to empty. Failures print seed, operation
number/type, relevant IDs and the failed expectation; status/order mismatches
include actual/expected values. Assertions remain active in Release.

`mini_internal` adds 24 failure/recovery cases: creation in four contexts,
front/back insertion into 0..3 nodes, and before/after at every live anchor.
Every operation has exactly one allocation, so failure point 1 is sufficient.
Snapshots check size, endpoints, every link, owner, item and surviving handle;
ledger accounting checks balance. Each failure is followed by a successful retry.
A 190-case legal-input matrix covers NULL inputs/outputs, valid foreign handles,
multiple foreign operands, foreign self-moves, output defaults, precedence and
synthetic capacity. These paths allocate/free nothing. Removal allocates nothing.
Caller-owned heap payloads survive destruction and are freed by the test itself.

Executed environment: Windows x64, MSYS2 UCRT64 GCC 16.2.0, CMake 4.4.4,
GNU Make 4.4.1, MinGW Makefiles. Debug and Release use C17 without extensions,
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`. Complete CTest includes
`mini_public`, `mini_internal`, `mini_example`, `mini_model`. Extracted standalone
builds use the same compiler. Production static analysis uses:

```sh
gcc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -fanalyzer -I include -c src/lks_mini.c -o analyzed.o
```

The sanitizer probe compiled a trivial `int main(void) { return 0; }` with:

```sh
gcc -fsanitize=address,undefined probe.c -o probe.exe
```

It failed at link time (exit 1): `cannot find -lasan` and `cannot find -lubsan`.
Therefore no instrumented suite was run. Allocator accounting supplements memory
checks; it is not equivalent to ASan/UBSan. MSVC, Clang, other operating systems
and architectures remain unverified. No production defects were discovered in
these tests; this is bounded evidence, not an exhaustive proof. Cross-platform
CI/integration remain RC.1 work. Accepted O(n) comparison, per-node allocation,
pointer chasing, no random access/persistence/stale detection/internal locks
remain unchanged. No separate enormous stress campaign is claimed.
