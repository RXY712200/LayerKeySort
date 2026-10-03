# LayerKeySort 2.x compatibility contract

Beginning with v2.0.0, LayerKeySort 2.x preserves the documented public
source/API and semantic compatibility contract below, subject to documented
bug fixes and backward-compatible additions. `v2.0.0-rc.1` was the prerelease freeze
candidate for this contract.

## Stable public contract

LayerKeySort 2.x preserves:

1. The public C function names and signatures present at 2.0.0, the meanings
   of public types, and existing enum and status numeric semantics.
2. Ownership and destruction rules: applications own item objects; Tree,
   Group, and Batch borrow their item pointers; callers destroy objects they
   receive as owned according to the public API.
3. Documented borrowed lifetimes and mutation invalidation. Actual successful
   Tree mutation can invalidate borrowed Tree nodes, Paths, and navigation
   observations. Equal-Path rekey is a successful no-op and preserves them.
4. Logical Path order defined by `lks_path_compare()`, including direction,
   level, slot, and parent/descendant rules.
5. The canonical display format and strict parser grammar documented in
   [API.md](API.md). Display text represents a coordinate; it is not permanent
   item identity and is not the sortable database key.
6. The published LK1 version-1 bytes, canonical parser, and same-version
   bytewise ordering semantics. LK1 bytes will not be silently reinterpreted.
   An incompatible external-key encoding requires a new format version.
7. Documented comparator-equal stability, Group immutability, and source
   preservation during Group merge.
8. Documented status, output, failure, and allocation-failure guarantees.
9. The `layerkeysort` CMake target as the supported source-integration target
   for the 2.x line.

This is primarily a source/API and semantic compatibility policy. It does not
promise binary ABI compatibility across arbitrary compilers, platforms, C
runtimes, or build configurations.

## Implementation output and internals

The following are not stable identity or exact-output promises:

- exact automatically generated Path values or spacing, including Paths
  selected after mutation, repair, or full rebuild;
- local repair heuristics and full-rebuild frequency;
- AVL physical shape, node addresses, private `src/*.h` interfaces, and
  diagnostic or test-only helpers;
- internal memory layout, allocation strategy, benchmark timings, and
  performance characteristics.

A compatible 2.x implementation can change those details while preserving
the public semantics above. Applications needing stable item identity must
store their own IDs separately from mutable Path coordinates. Persisting an
LK1 key preserves a coordinate at that point in time, not a permanent object
identity or a whole-Tree snapshot.

Backward-compatible API additions and documented bug fixes may occur in 2.x.
An intentional breaking change to the frozen public contract requires the
next major version. An optimization that preserves the contract does not, by
itself, require 3.0.

## Proposed 3.x compatibility boundary (unreleased candidate)

The released V3 Preview.1 through Preview.5 and `v3.0.0-rc.1` are outside
the 2.x compatibility promise above. `v2.0.0` remains the latest published
Stable release. This branch prepares an unreleased `v3.0.0` Stable candidate
for independent review; it has no Stable tag or GitHub Release yet.

V3 intentionally removes the two per-operation-comparator `LksTree`
functions and introduces comparator-bound `LksOrderedTree`; see
[V3_MIGRATION.md](V3_MIGRATION.md). If this candidate is published as Stable,
compatible 3.x updates are intended to preserve the documented public names,
signatures, status meanings, ownership and borrow lifetimes, comparator-bound
ordering, failure behavior, Path comparison, canonical display grammar, LK1 v1
bytes and ordering, Group/Batch semantics, stable sort behavior, and the
`layerkeysort` CMake source-integration target. Backward-compatible additions
and documented bug fixes may occur within 3.x; intentional breaks to those
contracts require a new major version or, for an incompatible external key,
a new key-format version.

Exact generated Path coordinates, private relabel thresholds, physical AVL
shape, node addresses, diagnostic counters, allocation layout, and benchmark
timings are not compatibility promises. A compatible 3.x implementation may
change those details while preserving the public contracts above. Paths and
LK1 keys remain ordering coordinates, not permanent item IDs. This is a
source/API and semantic policy, not a cross-toolchain binary ABI guarantee.
