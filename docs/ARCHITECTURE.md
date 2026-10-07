# LayerKeySort V4 architecture

LayerKeySort V4 separates two problems that earlier versions coupled together:

> **live position is contextual; historical/exported position is immutable.**

The current Stable model is intentionally narrow. LayerKeySort manages order; the
application owns business identity, item payloads, storage and higher-level domain logic.

## 1. Mutable live order

`LksOrder` owns live resident records arranged in bounded local blocks with a balanced
block index. A resident is addressed by an opaque `LksOrderHandle`.

A handle represents one **live occurrence**, not:

- a sortable coordinate,
- a persistent business ID,
- a serialized key,
- a physical block/index address.

Handles survive unrelated insertion/removal and resident-preserving maintenance such
as moves, splits, redistributions, merges and index rotations. A resident's own removal
or source destruction expires its handle.

Live mutation therefore does not require rewriting externally visible coordinates for
the whole collection.

The current B64/minimum-32 block policy is private implementation policy. It is not part
of API, wire or persistence compatibility.

## 2. Comparator-managed live order

`LksManagedOrder` uses the same live-order core and binds comparison semantics to it.

It provides stable comparator-equal insertion and comparator-based locate without
creating a second ordering engine. Comparator-visible item data must remain consistent
while resident; change it by remove → edit → reinsert.

## 3. Immutable Group and Batch

`LksGroup` is a flat immutable borrowed-item sequence.

`LksGroupBatch` owns immutable sorted chunks and can merge them while preserving the
documented stable equality precedence.

Neither type reintroduces live Path/Tree state. They are batch utilities, not a second
mutable ordering system.

## 4. Immutable historical representation

`LksSnapshot` captures logical order explicitly.

A snapshot owns:

- copied namespace bytes,
- row metadata,
- copied application association bytes,
- detached source-provenance bookkeeping where applicable.

It does **not** retain live handles, blocks, item pointers, comparator contexts or a former
source pointer.

This allows historical snapshots to survive later live mutation and source/item destruction.

### LS1

LS1 is a sortable **snapshot-domain** key, not a live position.

Within one namespace/domain, fixed-width ordinal encoding preserves bytewise order.
Cross-domain semantic ordering is rejected.

### LKS4SNP1

LKS4SNP1 serializes immutable snapshot state with explicit versioned big-endian fields.
It does not serialize block topology, live handles or native structs.

Restore resolves application associations back to caller-owned items and creates a fresh
live order with fresh handles and topology.

## 5. V3 migration boundary

V4's primary public header no longer exports live V3 Path/Tree/OrderedTree APIs.

Private strict V3 LK1 decoding remains only to support `lks_order_import_v3_lk1` and
regression evidence. V3.1.0 remains available for applications that intentionally keep
the old Path/Tree contract.

## 6. Ownership and synchronization

Applications own item payloads, comparator contexts, business IDs and external storage.

LayerKeySort owns its structural objects and copied snapshot bytes.

Mutable live sources require caller serialization. Atomic source-marker lifetime
bookkeeping is not a general locking scheme. Immutable snapshot/Group reads may be
shared when object lifetime is externally protected.

## 7. Long-term product boundary

The mature project is intended to remain narrow around three responsibilities:

1. **Mutable Order Engine**
2. **Immutable Historical Representation**
3. **Immutable Batch Utilities**

LayerKeySort is not intended to become a database, CRDT engine, persistent identity
registry or generalized application-state system.

Current post-v4 roadmap and active technical debt are tracked in
[Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3).

[Freeze](V4_FREEZE.md) defines the V4.0 compatibility contract.
[Stable record](V4_STABLE.md) records the reviewed release evidence.
