# V3 architecture

This describes the current `v3.0.0` implementation. The [API reference](API.md)
and [3.x compatibility contract](COMPATIBILITY.md) define public behavior;
physical AVL shape, generated coordinates, and policy thresholds remain private.

## Two Tree models

`LksTree` is a manual coordinate container. Callers select Paths for explicit
insertion and rekey. It does not enforce a comparator invariant. `LksOrderedTree`
copies a comparator descriptor at creation and owns a private `LksTree` index.
It chooses and, when needed, changes Paths to preserve comparator order. It
does not expose arbitrary Path insertion or rekey. Both models borrow item
objects and own their nodes and Path copies. The ordered model also borrows
the comparator context; the callback, context, and resident items must retain
their comparison semantics while the Tree lives.

Both models use a Path-keyed AVL index. Physical parent and child links are
index structure, not Path-prefix relationships or logical item order. In-order
traversal follows `lks_path_compare()`. Physical navigation is a transient
implementation-defined view. No subtree-size index is maintained.

## Managed insertion and relabel

Ordered insertion finds the comparator upper bound, so equal items are placed
after existing equal items. It first tries to generate a coordinate directly
between neighboring Paths. Endpoint insertion can carry a saturated slot
into an ancestor: append advances an ordinary slot (the negative root runs
in reverse slot order), while prepend can allocate a new negative-root
coordinate and carry a full root slot to the next level. The first 64
successful inserts in one endpoint run use spacing ten; later inserts use
spacing one. An interior insert or successful removal resets the run hint;
failed operations leave it unchanged. A direct
candidate beyond the preferred depth can trigger adaptive relabel. Current
private policy prefers depth at most six, permits open-end direct depth up to
sixteen, starts relabel with eight logical neighbors, and doubles the window
as necessary. These are tuning choices, not validity or compatibility limits.

Relabel operates on a contiguous logical-order region. For a bounded region,
gap generation prepares replacement Paths; a region covering the entire Tree
uses sparse bulk Path generation. The plan prepares all Paths, scratch space,
and the new node before it changes the Tree. It verifies strict Path increase
and exterior bounds. Commit replaces existing Path pointers in unchanged
in-order ranks, links the new node, balances the AVL, and frees old Paths
without fallible allocation. Failure during preparation leaves the Tree
unchanged. A full-range relabel may replace all `n` resident coordinates and
temporarily hold old and new Paths and scratch storage.

The V3 online path does not reconstruct the physical Tree as a fallback.
Balanced index search visits `O(log n)` nodes, but each visit may inspect a
Path and invoke a caller comparator. Candidate generation depends on Path
depth. A relabel may prepare `n + 1` Paths, and failed windows also cost work.
Complete managed insertion has no claimed worst-case `O(log n)` time or
formal amortized bound. See [current measurements](BENCHMARKS.md) and
[validation](VALIDATION.md).

## Representation boundaries

A Path is an ordering coordinate, not an item ID. Display text is readable
and parseable; the separately versioned LK1 key persists and bytewise-sorts
one coordinate. Neither representation serializes a Tree or caller payloads.
The exact display and LK1 grammars are specified in [API.md](API.md).
Historical design alternatives and V2 policy decisions are retained in
[design history](history/DESIGN_HISTORY.md).
