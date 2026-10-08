# Architecture — v1.0.0-preview.1

The production library consists of one public header and one C17 source file.
The public types are opaque. A private order holds head, tail and a size_t count;
a private occurrence holds prev, next, owner and a payload pointer. There are
no indices, position labels, allocation registries or redundant coordinates.
The non-intrusive design requires no changes to business objects.

## Structural invariants

1. Empty orders have NULL head and tail; nonempty orders have both.
2. The head's prev and tail's next are NULL.
3. Neighbor links are reciprocal.
4. Every live node belongs to one order and is reachable exactly once.
5. Recorded size equals the number of reachable nodes.
6. Legal orders contain no cycles.

Insertion links a fully initialized node between known neighbors, updates any
endpoint and increments the size. Removal repairs the two neighboring links and
endpoints, decrements size and frees only that node. Existing live node addresses
never change. Destroy walks forward, saves next before freeing and finally frees
the order. Production calls do not scan for invariants.

## Allocation and failure

Creation uses malloc for one order. Insertion validates arguments and ownership,
checks size against SIZE_MAX, then allocates one node. All fallible preparation
precedes link changes. No operation computes a potentially overflowing size+1
before the limit check. Removal and destruction allocate nothing.

All ordinary errors preserve the list. Allocation uses standard malloc/free;
the public API has no allocator settings. Payloads are neither inspected nor
freed. There are no production global diagnostics or shared mutable state.

## Validation and accepted limitations

The public test links the actual library. A separate internal test compiles the
same source with test-local malloc/free substitution to check basic OOM behavior,
allocation balance and private invariants. The bounded forward/backward checker
verifies endpoints, owners, count, exact node sequence and reciprocal links;
bounded traversal detects cycles. A synthetic SIZE_MAX count exercises the guard
and is restored before structural validation. It does not represent a physically
allocated SIZE_MAX-node order.

All basic operations are O(1); traversal and destruction are O(n). One allocation
per occurrence and pointer chasing are accepted tradeoffs. There is no random
access, allocator control, synchronization, persistent identity or stale-pointer
protection. Full randomized modeling and systematic fault injection belong to a
later reliability milestone, not this foundation. Movement and comparison are
not implemented in Preview.1.
