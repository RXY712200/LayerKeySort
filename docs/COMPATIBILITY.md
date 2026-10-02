# LayerKeySort 2.x compatibility contract

Beginning with v2.0.0, LayerKeySort 2.x preserves the documented public
source/API and semantic compatibility contract below, subject to documented
bug fixes and backward-compatible additions. RC.1 was the prerelease freeze
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

## V3 Preview development boundary

The released experimental V3 Preview.1 through Preview.4 are outside the 2.x
compatibility promise above. Stable users can continue using the published
`v2.0.0` tag.
V3 intentionally removes the two per-operation-comparator `LksTree`
functions and introduces a comparator-bound `LksOrderedTree`; see
[V3_MIGRATION.md](V3_MIGRATION.md). Further Preview API changes may occur
before a stable V3 release. Path comparison, display text, and LK1 v1 bytes
are preserved through V3 Preview.4, but this does not
freeze every future V3 API or generated Path coordinate.
