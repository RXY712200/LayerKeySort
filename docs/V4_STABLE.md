# LayerKeySort V4.0.0 Stable release record

## Decision and lineage

Stage 6 completes the fixed V4 Preview.1–5 / RC.1 development cycle. RC.1 passed
independent review; no known release-blocking correctness, OOM or frozen-format
defect remains. Stable promotion changes only version metadata and current
presentation. Production source, algorithms, tests, fixtures and CI are unchanged.

- Production main before promotion: `c33afa2ac08248bffa6dde51bd590a5d799b394a`.
- Preview.5 freeze base: `f413382e96175b79c117956fe4a57212785fb104`.
- Exact starting RC.1: `9ad636fb2e292070505a86f59011d433296651b1`.
- Historical [freeze](V4_FREEZE.md) and [RC validation](V4_RC1.md) remain unchanged.
- Final Stable revision: the peeled annotated [`v4.0.0` tag](https://github.com/RXY712200/LayerKeySort/tree/v4.0.0).
  The [Stable Release](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.0.0)
  records its exact 40-character SHA after candidate and separate main CI pass.
  A committed file cannot contain its own final commit hash; the immutable tag and
  Release are the authoritative final-SHA record, not a moving branch link.

V4.0.0 is recommended Stable. V3.1.0 remains a valid historical Stable for callers
requiring Path/Tree contracts; V4 is a major source break. See [migration](MIGRATION_V3_V4.md).

## Frozen compatibility

64 public functions / 14 public types, unchanged callbacks, fields and signatures.
No public Path, Tree, OrderedTree or transitional immutable_group API remains.
Status values: OK0, INVALID_ARGUMENT1, OUT_OF_MEMORY2, BUFFER_TOO_SMALL3,
NOT_FOUND5, INTERNAL_ERROR7, ALREADY_EXISTS8, CAPACITY_LIMIT9, INVALIDATED10,
REENTRANT11, DOMAIN_MISMATCH12; holes4/6 remain unknown.

LS1 version1 preserves seven exact vectors, canonical lowercase namespace/ordinal
hex, same-domain lexical order and semantic cross-domain rejection. LKS4SNP1
preserves four exact binary vectors, flags0, explicit big-endian fields, counting,
overflow and trailing-byte rejection. The 17 strict V3 LK1 migration vectors remain
unchanged, including published ordering semantics. No format is reinterpreted.
Win32/x64 run identical fixtures. 4.x source/layout compatibility requires rebuilding;
no universal cross-toolchain/CRT/architecture binary ABI is promised. The ordinary
library is static; private archive symbols do not become supported API.

## Validation and publication criteria

Local configurations: GCC16.2 strict C17 Release (-Wall -Wextra -Wpedantic -Werror),
MSVC x64 Debug, Release with symbols, x64 ASan and Win32 Debug. Each must pass
39/39 ordinary CTest tests and seven public examples. Local Clang and GCC sanitizer
runtime libraries are unavailable; their coverage comes from exact-SHA remote jobs.
The known local default optimized MSVC executable-disappearance anomaly is not
attributed to a cause without evidence; Release with symbols is the verified configuration.

Unchanged mutation oracle, managed regression, Group/Batch, ordinary parser torture,
marker concurrency, OOM/failpoint and allocation-free assertions remain mandatory.
Focused Preview.4 replay uses timeline/churn/distant_drag/equal, original initial
populations, seed7 and 10k operations: 16 replays / 16 maintenance, comparison and
cleanup counters must match the frozen evidence. No timing experiment or tuning.
Clean GCC/MSVC x64/Win32 C17/C++17 consumers reference all64 functions through
public headers: add_subdirectory, offline FetchContent, install/export/find_package,
LayerKeySort::layerkeysort and amalgamation. Ordinary consumers need no Python.

Both the exact v4-stable candidate and its fast-forwarded main commit must pass
six separate CI configurations: Windows x64/Win32, Ubuntu GCC/Clang/Clang ASan+
UBSan and macOS AppleClang. Each runs39 CTest tests, seven examples, manifest and
clean consumers. GCC validates assets. Clang sanitizer retains the RC extended
600k-step mutation, long parser, 100k distant-move and 200k-churn campaign.
Only green main permits an annotated v4.0.0 tag and non-draft/non-prerelease latest
Stable Release. Historical Preview/RC branches, tags, Releases and v3.1.0 remain.

## Source assets

LayerKeySort-4.0.0-amalgamation.zip and LayerKeySort-4.0.0-SHA256SUMS.txt contain
no binaries. ZIP membership is exactly layerkeysort.h, layerkeysort.c, example.c,
LICENSE and README.txt. Independent generations must be byte-identical, with
normalized ZIP metadata, exact membership, checksum and tamper/extra-file rejection.
Packaged examples compile/run; uploaded sizes/digests and downloaded bytes must
match local assets. Final sizes and SHA-256 values are published with the Release.

## Costs and technical debt

[Preview.4 evidence](V4_PREVIEW4.md) and [captured workloads](../benchmarks/results/v4-preview4/README.md)
remain the principal measurement record; no historical timing data is rewritten.

| Classification | Item / boundary |
|---|---|
| Resolved / superseded | V3 large synchronous live-coordinate relabel architecture |
| Resolved / superseded | Live Path/LK1 growth as the primary V4 representation |
| Resolved / superseded | Path/Tree overhead in immutable Group representation |
| Resolved / superseded | Live-coordinate persistence coupling; explicit snapshots instead |
| Partially addressed | Structural operation bounds; no formal whole-library latency theorem |
| Partially addressed | Allocator overhead; per-resident allocation and RSS retention remain |
| Partially addressed | Portability breadth; tested desktop x86/x64 configurations, not all platforms |
| Deferred | Real-user trace corpus and stronger formal whole-library bounds |
| Deferred | Per-record pooling/slab experiments and stale-handle defensive registry |
| Deferred | Incremental export and persistent live handles |
| Deferred | Broader platform coverage, Mini and embedded/MCU editions |

Private B64/minimum32 is unchanged policy, not a public promise. Local block writes,
block slack, balanced index updates, comparator/callback and allocator/OS costs
remain. Move structural work is independent of logical distance in the selected
representation; no universal O(1) or whole-operation O(log N) latency is promised.
Full capture/export costs O(N+A); retained histories, association bytes and Group
scratch can dominate memory. Item/context lifetimes and mutable serialization are
caller responsibilities; own removal/source destruction expires handles. No stale
pointer probe, distributed/CRDT guarantee or persistent handle is supplied.

No new research cycle, V5 feature or performance policy is scheduled by this record.
