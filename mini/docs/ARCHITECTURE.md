# Architecture — v1.0.0

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

Basic operations and movement are O(1); comparison, traversal and destruction
are O(n). One allocation
per occurrence and pointer chasing are accepted tradeoffs. There is no random
access, allocator control, synchronization, persistent identity or stale-pointer
protection. Preview.3 adds independent randomized modeling and systematic
single-allocation failure testing. The production representation remains unchanged.

## Movement and comparison

Small private unlink/link helpers reconnect an existing node after argument and
ownership validation. They do not alter size, owner, item or lifetime. Endpoints,
self moves and already-adjacent placement are checked before unlinking so valid
no-ops write no links. Anchor neighbors are read after unlinking to avoid stale
adjacency. No fallible work occurs during the link commit.

Comparison first validates both handles, returns zero for identical occurrences,
then follows next links from a searching for b. Finding b returns -1; otherwise
both valid same-order handles imply +1. This O(n) worst-case walk is not an
invariant scan and maintains no ranks or cached positions. It allocates/frees
nothing and changes no state. Tests use explicit expected sequences, all-pair
sign checks, antisymmetry/transitivity and allocation/free counter snapshots.

## Preview.3 reliability evidence

The public-header-only model links the real library and uses a dynamically
allocated flat array of records (unique test ID, handle, item, test owner).
Array indices represent expected positions; memmove changes the reference order,
independently of production links. Deleted records are erased immediately and
no retired handles are compared or dereferenced. Payloads remain alive.

Private tests retain source inclusion solely for structural access and allocator
substitution. A test-only live allocation ledger rejects non-Mini or repeated
frees. Absolute allocation-call failure points exercise each operation's single
real allocation, with exact snapshots and successful retries. Synthetic SIZE_MAX
state checks capacity/validation precedence without creating enormous lists;
the actual size is restored before normal structural verification.

The checker validates bounded forward/reverse traversal, reciprocal links,
endpoints, owners, uniqueness, exact expected sequence/count and handle identity.
Six-node movement patterns supplement existing boundary tests. Queries, no-ops,
errors, movements and comparisons are checked with allocation counters/snapshots.
The [development record](DEVELOPMENT.md) states executed tools and limitations.
No production correctness defect was found; no speculative redesign was made.

## RC.1 engineering and debt

The production structures and all 18 API implementations are unchanged. Strict
warnings, target-specific sanitizers, independent consumers and the optional
benchmark are build/test facilities, not new runtime behavior. Optional
coexistence never enters the required standalone target graph.

Accepted costs: O(n) arbitrary comparison, one allocation per occurrence,
pointer chasing and no indexed access. Deliberate non-goals: persistence,
automatic sorting, distributed ordering, built-in locking and stale detection.
No confirmed supported-operation correctness defect is known from executed
checks. Potential future work includes broader workload/allocator measurements;
these opportunities are not existing bugs and authorize no redesign.
[Performance](PERFORMANCE.md) separates measured timing from sizeof estimates.
