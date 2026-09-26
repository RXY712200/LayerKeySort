# LayerKeySort

## What it is

LayerKeySort is a C library for stable ordering of caller-owned item pointers. It builds sorted Groups and can merge Groups or a GroupBatch while retaining a Path for each item's current position.

## Current status

**LayerKeySort v1.0.0** · Language: **C17**.

The public header identifies this release with `LKS_VERSION_MAJOR == 1` and `LKS_VERSION_MINOR == 0`. The existing major/minor macro format is retained; the patch component is represented by the release/documentation version `1.0.0`.

## Build requirements

The checked-in Visual Studio project was built with MSVC for x64 in C17 mode. Open `001.slnx` or `LayerKeySort.vcxproj` and select one of the existing x64 configurations:

- **Debug**
- **Release**
- **ASan** (MSVC AddressSanitizer)

The project explicitly compiles `.c` files as C. The core source avoids MSVC-specific language extensions, but GCC and Clang portability has not been verified for this release.

## Quick start

The complete, standalone example is [`examples/basic.c`](examples/basic.c). It uses only the public header, builds two input Groups, merges them, prints the sorted items, and destroys the result and Batch. Its output demonstrates that equal keys retain their source order.

The essential calls are:

```c
LksComparator comparator = { compare_item_key, NULL };
LksGroupBatch *batch = NULL;
LksGroup *result = NULL;
LksStatus status;

status = lks_group_batch_build(items, item_count, 3, &comparator, &batch);
if (status == LKS_STATUS_OK) {
    status = lks_group_batch_merge_all(batch, &comparator, &result);
}
/* Read result with lks_group_size() / lks_group_item_at(). */
lks_group_destroy(result);
lks_group_batch_destroy(batch);
```

See the example for the comparator definition and complete error cleanup. The smoke test `tests/public_api_usage.c` compiles a similar use case while including `layerkeysort.h` only; it verifies the duplicate-key order and borrowed item identities.

## Comparator

Set `LksComparator.compare` to an `LksCompareFn` callback and `context` to caller-owned data (or `NULL`). A negative result places `left` before `right`, zero means equivalent under the current comparison rule, and a positive result places `left` after `right`. LayerKeySort passes `context` through without interpreting it. The callback should implement a consistent ordering for the duration of an operation.

## Stable ordering

LayerKeySort v1 provides stable ordering: items that compare equal retain their original source/input order. In a two-source `lks_group_merge`, equal items from `base` precede equal items from `incoming`; order within each source is preserved. `lks_group_batch_merge_all` preserves the original Batch chunk order for equal items.

## Path model

A Path is an ordering position, not a business key. The zero position formats as `000`; positive positions begin with `0`, negative positions with `1`. Steps use slots `A0` through `Z9`; `/` marks skipped levels. Use `lks_path_compare` to compare positions.

Paths assigned inside independent Groups are local positions and cannot be compared as global positions across those Groups. Merge creates positions for incoming items in the result's coordinate space; consumers should read the result Paths after merging.

## Group / Batch / Merge

`lks_group_build` creates one sorted Group. `lks_group_merge` returns a separate merged Group without modifying either input. `lks_group_batch_build` divides the input array into consecutive chunks of the requested size and sorts each chunk; `lks_group_batch_merge_all` returns a separate merged result while leaving the Batch intact.

## Ownership

LayerKeySort borrows business item pointers. It does not clone or free the items; the caller manages their lifetime and must keep them valid while a comparator or result access may use them.

The caller owns Paths returned by Path constructors, clone, and gap functions, and must release them with `lks_path_destroy`. Trees, Groups, and Batches returned by their create/build/merge functions are caller-owned and must be released with the corresponding destroy function. A Tree owns its stored Path copies and nodes. A Batch owns its Groups. Accessors such as `lks_group_batch_group_at`, `lks_group_item_at`, `lks_group_path_at`, and Tree node accessors return borrowed pointers whose lifetime is tied to their owning object (and, for item pointers, to the caller).

## Error handling

Functions returning `LksStatus` report `LKS_STATUS_INVALID_ARGUMENT`, `LKS_STATUS_OUT_OF_MEMORY`, or a more specific status as applicable; use `lks_status_string` for a readable description. Pointer-returning Path and Tree constructors return `NULL` on invalid input or allocation failure.

On failure, Group and Batch builders set their output object pointer to `NULL` and leave input arrays and items unchanged. Public `lks_group_merge` and `lks_group_batch_merge_all` set the result pointer to `NULL`; an allocation failure leaves their input Groups/Batch usable and unchanged. Path append operations leave the Path unchanged on failure. Tree insertion leaves the Tree's logical contents unchanged on failure.

## Memory / allocator notes

Paths, Trees, Groups, and Batches allocate internal storage dynamically. Destroy every object whose ownership the API gives to the caller. Item storage remains caller-owned. Allocator measurements are requested internal allocation sizes, not process RSS or a fixed memory limit; LayerKeySort makes no real-time allocation guarantee.

There is no explicit thread-safety design or guarantee for concurrent access to shared mutable objects. Use external synchronization when sharing mutable LayerKeySort objects across threads.

## Testing

The repository retains deterministic property, stress, AddressSanitizer, allocation-accounting, and internal deterministic allocation-failure tests. Fault injection and allocator introspection are private test facilities, not public library features.

## Tested configurations

LayerKeySort v1.0.0 has been exercised with MSVC x64 Debug, MSVC x64 Release, and the project's MSVC x64 AddressSanitizer configuration. GCC/Clang builds have not been run.

## Known scope / non-goals

This release does not provide serialization, a text parser for Path values, a fixed memory ceiling, or a public allocator/fault-injection API. New algorithms, optimizations, and representation changes are outside the v1.0.0 scope.

## License

No `LICENSE` file is present. The license decision is pending; this repository does not declare a license here.

## Project layout

```text
include/layerkeysort.h       Public API
src/                C implementation and private headers
tests/              Property, stress, benchmark, and API-usage checks
examples/basic.c    Minimal public-API example
demo/main.c         Test and validation runner
001.slnx            Visual Studio solution
LayerKeySort.vcxproj         Visual Studio C project
README.md           User documentation
```
