# Migrating from stable V2.0.0 to V3 3.x

This document describes migration from historical stable `v2.0.0` to the V3
API introduced in published `v3.0.0`. The 3.1.0 source retains the same 59
public functions. The active 3.x compatibility contract is in
[COMPATIBILITY.md](COMPATIBILITY.md).

V2 let one `LksTree` accept caller-selected Paths and per-operation comparators.
An explicit insert or rekey could silently invalidate the ordering assumed by
the next comparator insert. V3 separates those models:

| V2 operation | V3 manual coordinate Tree | V3 comparator-managed Tree |
| --- | --- | --- |
| `lks_tree_create()` / destroy / size | Unchanged | `lks_ordered_tree_create(&comparator)` / destroy / size |
| `lks_tree_insert(path, item)` | Unchanged | No arbitrary Path insert |
| `lks_tree_insert_item(item, &comparator)` | Removed | `lks_ordered_tree_insert(item)` |
| `lks_tree_locate_item(item, &comparator)` | Removed | `lks_ordered_tree_locate(item)` |
| `lks_tree_find_path(path)` | Unchanged | `lks_ordered_tree_find_path(path)` |
| `lks_tree_remove_path(path)` | Unchanged | `lks_ordered_tree_remove_path(path)` |
| `lks_tree_rekey(old, new)` | Unchanged | No arbitrary rekey |
| Node Path/item/physical child/parent access | Unchanged, using `LksTreeNode` | Same borrowed `LksTreeNode` accessors |
| Virtual-root navigation | `lks_tree_root_child_*()` | `lks_ordered_tree_root_child_*()` |

The managed container copies the `LksComparator` descriptor at creation; the
descriptor object may then expire. Its callback code, context pointer, and
all resident item values used for comparison remain caller-owned. They must
stay alive and preserve consistent ordering semantics until the container is
destroyed. Changing an item's comparator-relevant fields in place can break
search and insertion. Save its exact Path, remove it first, update it, then
insert it again.

Both containers own structural AVL nodes and Paths, borrow items, and use
`lks_path_compare()` for coordinate order. A successful mutation invalidates
previous borrowed node, Path, and navigation observations. An equal-Path
manual rekey is a successful no-op and preserves them. The managed container
does not expose arbitrary movement because it would violate its bound order.

Path, display text, LK1 v1, Group, GroupBatch, and `lks_sort()` semantics are
unchanged. Exact automatically generated Paths and AVL physical shape are not
compatibility promises. There is no automatic whole-Tree or item serialization.
Physical navigation is an ephemeral index view, not logical Path hierarchy.
