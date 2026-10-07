# LayerKeySort V4 compatibility

[V4 freeze](V4_FREEZE.md) defines the 4.x source/layout and persistent-format contract.
V4.0.0 Stable preserves the Preview.5 freeze after independently reviewed RC.1.

## Source compatibility

The supported public surface is the frozen 64-function / 14-type V4 header.

4.x releases must not silently:

- rename/remove public functions,
- change public struct fields/layout contracts,
- renumber public statuses,
- change documented handle/cursor lifetime semantics,
- change Group/Batch ordering semantics.

Applications should rebuild from source for their compiler/architecture/CRT combination.

## Persistent compatibility

LS1 version 1 and LKS4SNP1 are stronger persistent contracts than the library's binary ABI.

Within 4.x, existing valid bytes retain their documented meaning. Incompatible future
formats require a new family/version rather than silently reinterpreting LS1/LKS4SNP1.

Strict published V3 LK1 interpretation remains supported only for migration through
`lks_order_import_v3_lk1`.

## Binary ABI

CMake explicitly builds a STATIC library. No shared DLL/SO ABI is shipped, including
when `BUILD_SHARED_LIBS` is set.

No universal ABI compatibility is promised across:

- MSVC vs GCC/Clang,
- architectures,
- CRT choices,
- unrelated compiler ABIs.

Source rebuild is the normal supported integration model.

## Symbol model

The public header/manifest defines the supported API.

An archive symbol listing is not a public API listing: private allocator/sort/snapshot
helpers and historical Path codecs needed by strict LK1 migration may have ordinary
object linkage.

On the reviewed GCC V4.0.0 build, `nm` reports 64 public and 43 private `lks_*` text
symbols. No live legacy Tree/OrderedTree, Path-gap or legacy Group public contract is
restored by those private objects.

Installed consumers must not declare or call private symbols.

## What may change inside 4.x

Private implementation details may change if frozen semantics remain true, including:

- block capacity/policy,
- index topology,
- allocator implementation,
- helper decomposition,
- diagnostic instrumentation.

B64/minimum-32 is therefore not a compatibility constant.

## V3 compatibility boundary

V4 is a major source break from V3.

V3.1.0 remains separately available for applications that require live Path/Tree
contracts. See [migration](MIGRATION_V3_V4.md).

## Future major versions

A future V5/V6 is not scheduled by cadence.

A major version is justified only by an important architecture/compatibility problem that
cannot responsibly be solved inside the frozen 4.x contract.

Current active roadmap:
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).

See [Stable record](V4_STABLE.md) and [RC validation record](V4_RC1.md).
