# Local Layer Order Workbench — Phase 1

A standalone C17 terminal application using **LayerKeySort Mini v1.0.0** through
its public header and CMake target. It maintains an editable layer occurrence
list, checks every command against an independent array oracle, and records and
replays implementation-independent semantic traces. It is a consumer validation
project, not a new Mini release, renderer or evidence of external user adoption.

[简体中文使用说明](README.zh-CN.md) | [Trace specification](docs/TRACE.md) |
[Validation and limitations](docs/VALIDATION.md)

## Independent build

Requires CMake 3.21+, a C17 compiler and a matching build tool. Tests additionally
require Python 3.8+ (standard library only); the application has no Python runtime
dependency. Full is not required. Do not configure the repository root to build
this application.

```sh
cmake -S consumers/layer-order-workbench -B wb-build \
  -DMINI_SOURCE=/absolute/path/to/mini -DCMAKE_BUILD_TYPE=Debug
cmake --build wb-build --config Debug
ctest --test-dir wb-build -C Debug --output-on-failure
```

For repository integration point `MINI_SOURCE` at the repository's `mini/`.
For the actual released dependency, [download the Mini-only package and checksum](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0),
verify the SHA-256 manifest, extract it outside this repository, and point
`MINI_SOURCE` at `LayerKeySort-Mini-v1.0.0`. Copying only this consumer directory
and the extracted Mini package is sufficient. The automatic GitHub whole-repository
source archive is not the independent Mini distribution.

The test helper can fetch and verify the exact published assets, without changing
any tag or Release:

```sh
python consumers/layer-order-workbench/tests/extract_release.py --output /new/path/official-mini
# MINI_SOURCE=/new/path/official-mini/LayerKeySort-Mini-v1.0.0
```

Windows MSVC: launch a Developer PowerShell, use an absolute `MINI_SOURCE`, then
run `wb-build/Debug/layer_order_workbench.exe`. MinGW: add `-G "MinGW Makefiles"`
and run `wb-build/layer_order_workbench.exe`. Unix: `wb-build/layer_order_workbench`.
Release uses a separate build directory with `-DCMAKE_BUILD_TYPE=Release` and
`--config Release`. Both Mini and consumer warnings are treated as errors.

For GCC/Clang, direct application compilation can also link the already-built
Mini CMake target archive (same compiler/ABI); it does not include private source:

```sh
# From this consumer directory; set MINI_SOURCE and MINI_ARCHIVE to real paths.
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
  -Isrc -I"$MINI_SOURCE/include" src/main.c src/command.c src/oracle.c \
  src/workbench.c src/trace.c "$MINI_ARCHIVE" -o layer_order_workbench
```

For the Unix CMake build above the archive is usually
`wb-build/mini-dependency/liblayerkeysort_mini.a` (relative to its build location).

## Terminal commands

Start the executable without `--commands` or `--replay`. A real terminal gets a
prompt; redirected stdin is also supported. `help` and `--help` explain syntax.

| Command | Meaning |
| --- | --- |
| `create NAME` | Create application-owned layer object; return sequential object ID |
| `insert-front PAYLOAD`, `insert-back PAYLOAD` | Insert a fresh occurrence, return sequential occurrence ID |
| `insert-before PAYLOAD ANCHOR`, `insert-after PAYLOAD ANCHOR` | Insert relative to a live occurrence ID |
| `remove OCC` | Remove occurrence and immediately clear its active handle mapping |
| `move-front OCC`, `move-back OCC` | Reposition an existing occurrence |
| `move-before OCC ANCHOR`, `move-after OCC ANCHOR` | Reposition relative to a live occurrence |
| `list`, `list-reverse` | Traverse Mini and show occurrence ID/object ID/name or NULL |
| `compare OCC OCC` | Compare live occurrences, exact result -1/0/+1 |
| `size`, `help`, `quit` | Count, command help, orderly cleanup |

`PAYLOAD` is an existing decimal object ID or the literal `NULL`. `OCC` and
`ANCHOR` are **occurrence IDs**, never payload IDs or raw handles. For example:

```text
create Ink
insert-back 1
insert-back 1
insert-before NULL 2
move-front 2
compare 2 1
list
remove 1
move-back 1
quit
```

The duplicate Ink references are separate occurrences. NULL placeholders also
have identities. A removed ID produces `APP_RETIRED_OCCURRENCE`; no freed handle
is passed to Mini. Reinsertion gets a new ID. Payload storage is stable and
application-owned; removing an occurrence does not free a shared object. Objects
are intentionally retained until shutdown, which first destroys Mini and then
releases application storage.

Limits: 255 input bytes per normalized line; four tokens; names contain 1–47 ASCII
letters/digits/underscore/hyphen; 256 objects, 2,048 simultaneously live occurrences,
16,384 successful insertions per process. Numeric IDs must be decimal 1–16,384;
zero denotes NULL only internally. These are explicit **application** limits,
not Mini capacity claims. Blank/comment lines (`#` after leading whitespace) are
ignored. Overlong lines are fully drained, NUL-containing input is rejected,
and parsing/state errors do not end the session. Application errors use `APP_*`;
actual Mini allocation failures use `MINI_OUT_OF_MEMORY`. They are not conflated.

## Scripts, trace capture and replay

```sh
wb-build/layer_order_workbench --commands consumers/layer-order-workbench/scenarios/layer-composition.commands \
  --source script --trace /new/path/composition.trace
wb-build/layer_order_workbench --replay /new/path/composition.trace
```

The trace path must **not exist**: exclusive creation protects old evidence and
input files. Every processed command gets a versioned trace record with semantic
IDs, status, pre/post size and complete expected sequence. No pointer addresses
are used as identity. Replay executes public Mini operations again and rejects
any changed record/result, malformed input, missing completion footer or trailing
data. I/O, oracle mismatch and replay mismatch cause nonzero exit and cleanup.
Normal expected command errors are recorded and leave the session running; a
successful script can therefore contain error-recovery cases and exit zero.

`--source` is `script`, `generated`, `codex-interactive`, `human-terminal` or
`unspecified`. The last two terminal-specific labels require actual terminal
stdin (they cannot label a file/pipe as terminal interaction). `human-terminal`
is a caller declaration of operator identity, not proof of external adoption.
Replay records its current `replay` mode, while original trace provenance remains
in the input artifact. Three [saved scenario traces](scenarios/evidence/) were
authored scripts actually executed by Codex/CTest, **not** human/editor-user traces.
An additional saved 23-command terminal session used Codex automation over
Windows ConPTY with batched stdin, not human typing; its capture is explicitly
marked `codex-interactive` and is also replay-tested.

## Oracle, tests and sanitizer

`oracle.c` contains only an ordered array of occurrence/payload IDs and independent
array insertion/removal/repositioning. It has no Mini includes, links or traversal
answers. Commands compute a candidate expected sequence **before** calling Mini;
errors retain the previous model. Verification checks complete forward/reverse
order, count/endpoints, stable live handles, exact payload pointers including
duplicates/NULL, live-map retirement and application ownership/reference counts.
At up to 32 elements every ordered pair is checked. Larger cases check 32 fixed
pairs `((61i+7)%n, (127i+3)%n)`, both endpoint relations and every self relation.
Requested comparisons are independently checked as well.

CTest runs units/public foreign-order checks, three golden scenarios and replay,
input/ID/lifecycle errors, corrupt/truncated trace rejection, and generated
16/128/2,048-item cases. The unit test deliberately bypasses the oracle to reorder
via public Mini APIs and confirms divergence detection; its expected diagnostic
is not a newly discovered Mini defect.

Linux GNU-compatible linkers and MinGW add `workbench_oom`: test-only wrapping of
`malloc`/`calloc`/`free` exercises creation and all four insertion failures/retries,
checking outputs/model/handles/ownership and zero tracked Mini allocations after
cleanup. Only that test executable is wrapped. No Mini private source/header is
included, no production file is modified, and no public allocator API is implied.
MSVC/AppleClang omit that unsupported linker mechanism; other checks still run.

On supported non-Windows GCC/Clang environments:

```sh
cmake -S consumers/layer-order-workbench -B wb-asan \
  -DMINI_SOURCE=/absolute/path/to/mini -DCMAKE_C_COMPILER=clang \
  -DCMAKE_BUILD_TYPE=Debug -DWORKBENCH_SANITIZERS=ON
cmake --build wb-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir wb-asan --output-on-failure
```

The option instruments the actual Mini target and all consumer/test targets.
The new consumer CI covers GCC, Clang, MSVC, AppleClang, Linux Clang ASan/UBSan
and a separate published-ZIP integration job. Actual pass/fail evidence belongs
to the exact-SHA run linked in the Draft PR, not to workflow presence alone.

## Boundaries and next decisions

No Full dependency, automatic sorting, persistence, GUI, synchronization or stale
pointer detection is added. No performance superiority is claimed. Per-command
oracle checks, linear application lookups, bounded fixed storage and full-sequence
trace output are deliberate validation costs; this application is not a latency
benchmark. [Mini roadmap #7](https://github.com/RXY712200/LayerKeySort/issues/7)
remains unchanged. Representative downstream traces, Full-specific adapters and
allocator/RSS/statistical performance studies require later bounded work.
