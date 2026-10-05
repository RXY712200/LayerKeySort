# Ordering theory: model mapping and Phase 2 shortlist

**Phase 1 only. Decision: shortlist two directions, not a production change.**
Start with multiscale coordinate slack; consider bounded chunk coordinates
only if their external rewrite cost is justified. Neither inherits a literature
bound without a new argument for this representation.

Baseline: main `c33afa2ac08248bffa6dde51bd590a5d799b394a`; latest Stable
remains `v3.1.0`. The independently reviewed
[tail-cost research](https://github.com/RXY712200/LayerKeySort/blob/ce812dda83bdc7f9581cbb2761921aa3c7ba75cd/docs/TAIL_COST_RESEARCH.md)
ended **B — Tradeoff only**, with no approved candidate. Its implementations
remain on their separate research branch.

Evidence labels below distinguish **Literature fact**, **LayerKeySort fact**,
**Mapping inference**, and **Open question**. References are primary papers,
with section locators; this is a design filter, not a general survey.

## 1. LayerKeySort abstract model

**LayerKeySort fact:** `LksOrderedTree` maintains comparator-managed total
order, placing new comparator-equal items after existing equals. An analytical
tie-break by insertion sequence describes that stable order; it is not a new
public identity field. Updating a comparator-visible key requires removal
before changing the item and reinserting. Items/context remain caller-owned.
Manual-coordinate `LksTree` is a separate use case. See [API](API.md).

| Model | Observable result | Where LayerKeySort fits |
|---|---|---|
| A: order maintenance | Given live handles, answer whether A precedes B | A required capability, but not the complete contract |
| B: list labeling | Each item has a label whose independent comparison respects order | Public Paths provide this capability |
| C: serialized sortable labels | Labels can leave the structure and still be compared without a resolver | Canonical same-version LK1 adds this capability |

**Mapping inference:** LayerKeySort is closest to online list labeling with
variable-size hierarchical labels, plus comparator search, deletion and
reinsertion. It is not physical file maintenance: relabeling rewrites owned
coordinates rather than moving caller payloads through array slots.

**LayerKeySort fact:** Path comparison, canonical display and LK1 bytes are
fixed by the [3.x contract](COMPATIBILITY.md). Path is a mutable coordinate,
not identity. Persisted LK1 is a snapshot, not automatically refreshed after
relabel; the library provides no external database transaction. Failure
atomicity protects the live library object on recoverable allocation failure,
not application storage. Published Groups retain immutable coordinates.
Exact removal is allocation-free and does not relabel surviving Paths;
deletion-triggered redistribution from classical file maintenance cannot be
copied directly into that operation.

An insertion's cost must distinguish comparator search, affected resident
Paths, actual coordinate-value changes, bytes generated/rewritten, maximum
synchronous pause, depth/LK1 distributions, allocations and resident memory.
Replacing an equal-valued Path still costs allocation/destruction. Classical
unit relabel counts omit these costs.

## 2. Relevant theory families

### Order maintenance: internal representation versus exported labels

**Literature fact:** Dietz–Sleator give constant amortized updates with constant
order queries, and a second constant worst-case solution. Operations receive
list positions/records; they do not search for a comparator insertion rank.
The representation uses tags and indirection [DS, problem definition and
algorithms]. Bender et al. simplify the machinery: their Section 2.1 uses
small sublists and a top-level structure to reduce amortized update cost;
Sections 2–3 relate tag ranges to a virtual tree [BCDFZ]. These results use
their machine/word and representation assumptions, not arbitrary-length
serialized-label byte costs.

**Mapping inference:** A changed sublist representative can affect many
logical flattened labels although an internal order query follows that
representative once. Exporting a copied `(representative tag, local tag)`
inside a Path charges every affected item again. Replacing it with an opaque
handle that requires lookup would violate B/C. Comparator upper-bound search
also remains separate; constant order maintenance does not imply constant
managed insertion.

### Online labeling / file maintenance: the relevant cost tension

**Literature fact:** For deterministic online labeling with a sufficiently
large ordered input universe, assigning labels in a fixed set of size
`m = c*n`, constant `c > 1`, has worst-sequence total relabel cost
`Omega(n log^2 n)` [BKS, main result]. For `m = n^(1+epsilon)`, fixed positive
epsilon, the matching scale is `Theta(n log n)` under the polynomial-label
paper's input-universe assumptions [BBCKS, Introduction/Theorem 2.1]. These
are total costs for an adversarial sequence of n insertions, not a claim
about every insertion or these workload seeds.

**Mapping inference:** More usable label space can reduce unavoidable
relabel work. These lower bounds do not prove a LayerKeySort latency bound,
nor that its current full-range cliff is unavoidable. Its expandable Paths
do not specify one fixed m as a function of n; a fixed-depth, fixed-level
coordinate family would first need an explicit finite-space mapping.
Deletion/churn and variable-byte rewrites need separate analysis. Randomized
models must not be assigned these deterministic lower bounds indiscriminately.

### Density/slack and hierarchy

**Literature fact:** Packed-memory arrays maintain scale-dependent occupancy
thresholds and redistribute a containing interval when local density exceeds
them. Upper/lower thresholds leave headroom after redistribution, including
under deletions [BDF, Section 2.3, Theorem 2.5]. Their amortized
`O(log^2 n)` element-movement analysis and cache-transfer bound belong to an
array capacity/density model. Bender–Hu further adapt redistribution to
nonuniform insertion patterns [BH, adaptive rebalance algorithm].

**Mapping inference:** Borrow the occupancy invariant and reserved capacity,
not the physical array or its bound. A hierarchy can describe logical
coordinate intervals independently of AVL shape. Balanced label-space
allocation and explicit future slack are materially different from making
every prepared label shallower. Adaptation requires its own reserve invariant;
recent insertion frequency alone is not a guarantee against adversarial gaps.

### Variable-length labels

**Literature fact:** Finite-alphabet strings with a length ceiling have a
finite number of encodings; relaxing the ceiling increases available label
space. This is a counting fact, not an insertion-performance theorem.

**LayerKeySort fact:** Slots have 65,536 values; levels are strictly increasing
`size_t` values. Nonzero Path depth is expandable within representable limits.
LK1 uses absolute levels: for depth d, its length is
`6 + sum(3*N_i + 5)` characters, where `N_i` is the minimal positive number
of bytes encoding level i. Depth alone does not specify encoded cost.

**Mapping inference:** With a fixed positive direction and increasing level
pattern, d-step equal-depth slot tuples form an ordered family of
`65536^d` labels. This illustrates extra capacity, but says nothing about
capacity inside a particular existing gap. Mixed depths, parent-before-child,
descending level order and negative-root reversal prohibit treating arbitrary
Path text as an integer grid. Repeated subdivision can accumulate long
labels; depth and level-byte budgets must accompany a relabel policy.

### Deamortization

**Literature fact:** DS provide worst-case order maintenance; BCDFZ discuss
deamortization/file-maintenance algorithms, and BDF Section 2.3 identifies
prior deamortized work. Such scheduling retains the source model's invariants.

**Mapping inference:** Incremental preparation can spread generation work,
but a final per-node Path publication/destruction loop can still be linear.
Shadow generations need rules for intervening inserts/removals, stale prepared
bounds, recovery, and memory duplication. Incremental in-place relabel is not
automatically wrong: every completed operation would need valid published
Paths and failure rollback. That protocol is unproved here. A small background
work budget alone is not a deamortization proof.

## 3. What transfers

- Separate comparator rank search from coordinate maintenance.
- Model **usable** coordinate capacity and population at several logical
  scales, with slack after repair and split/merge hysteresis.
- Treat additional depth as paid label-space capacity, with explicit byte
  costs, not as a free escape from congestion.
- Count flattened external-coordinate changes through every hierarchical
  operation; include rejected preparation and destruction.
- Preserve stable equality, exterior bounds and prepare/validate/commit.
  Metadata allocation/update must participate in the same transaction.
- Keep exact removal allocation-free, without survivor relabeling. Record
  underfull regions with allocation-free bookkeeping; any allocating repair
  or coordinate-changing consolidation must wait for a permitted insertion.

These are **mapping inferences**, not transferred asymptotic guarantees.

## 4. What does not transfer

| Direction | Rank | Reason for excluding it from the prototype shortlist |
|---|---|---|
| Drop-in constant-time order-maintenance tags | Reject | Internal indirection hides exported-label rewrite work; insertion-rank search remains |
| Replace AVL with a packed-memory array | Reject | Physical scan/cache benefits do not establish lower Path/LK1 tail cost; unnecessary index redesign |
| Unbounded variable-length midpoint labels | Reject | Trades relabels for unchecked storage/comparison cost; does not preserve a controlled resource model |
| Generic incremental full-range migration | Reject | No complete publication/rollback/intervening-mutation protocol or bounded final commit; revisit only with one |
| Reuse direct7/open_dense or tune thresholds alone | Reject | Reviewed tradeoff failed; no new capacity invariant |

Serialized coordinates do not add distributed ordering, immutable IDs or
automatic persistence synchronization. No public API change is proposed.

## 5. Key theoretical tradeoffs

1. **Relabel count versus label-space/length:** fixed short labels impose
   congestion pressure; expandable labels spend depth, bytes and comparison
   time. The literature identifies a tension, not our optimal operating point.
2. **Current latency versus future slack:** dense redistribution can reduce
   one pause while increasing recurring repairs. The reviewed timeline case
   had a valid depth-seven candidate; small equal-depth repairs were rejected,
   the right bound disappeared early, and larger preparations deepened until
   full-range bulk succeeded. `open_dense` removed measured timeline full
   events but increased recurring work and random/duplicate costs. This is
   **LayerKeySort evidence**, not a theorem.
3. **Internal hierarchy versus external rewrites:** a prefix-tag update costs
   all descendant flat labels, even if internal metadata is shared.
4. **Amortized work versus worst pause:** total relabel bounds permit large
   individual operations; fewer full-range events alone is insufficient.
5. **Atomicity versus scheduling/memory:** prepared old/new generations coexist.
   An allocation failure must not expose half-updated items, Paths or metadata.

**Open question:** Can a finite, efficiently countable coordinate family
inside the real exterior bounds provide useful reserve without excessive
levels, bytes or metadata? That feasibility test precedes performance claims.

## 6. Shortlist for Phase 2

### 1. Multiscale slack-aware regional redistribution — High

- **Core idea:** Define an enumerable finite coordinate family, with an
  explicit level pattern/depth budget, inside a logical interval. Track
  occupancy and usable capacity across containing intervals; choose a region
  with adequate post-insertion reserve before allocating replacement Paths.
  Redistribute sparsely with scale-dependent thresholds and hysteresis.
  Numeric display/LK1 differences are not capacity measurements. The hierarchy
  is logical metadata, not physical AVL topology.
- **Why it may address the cliff:** Choose repair by capacity/reserve rather
  than repeated generate-and-destroy attempts seeking strictly lower maximum
  depth. A same-depth repair could qualify through demonstrated slack; this
  is not the rejected unconditional equal-depth acceptance experiment.
- **Expected tradeoff:** More metadata and potentially more proactive local
  relabeling; fewer repeated failed preparations are a hypothesis. Depth may
  stay comparable, but byte and reserve budgets must be tested. Global
  expansion remains possible; no tail bound is promised.
- **Atomicity difficulty:** Medium. Prepare ordered coordinates, the pending
  node and staged occupancy metadata; validate exterior bounds and all
  invariants; commit without allocation. OOM must retain old metadata too.
  Removal cannot eagerly apply the paper's lower-density redistribution.
- **Implementation complexity:** Medium–high, chiefly capacity mapping and
  maintaining the logical hierarchy under deletion/reinsertion.
- **Prototype would test:** First prove monotone rank-to-Path generation and
  correct capacity counts for constrained families and boundaries. Then use
  the unchanged frozen campaign: five medium seeds, reserved holdout, large
  timeline/priority, long churn and controls. Measure reserve consumption,
  rejected preparation, affected coordinates/bytes, latency, depth/LK1,
  traffic/memory and allocation-failure rollback. Kill it if forecasting
  adds comparable work or duplicate/churn behavior worsens materially.

### 2. Bounded chunk prefix/suffix coordinates — Medium

- **Core idea:** Use small contiguous logical chunks with a public Path prefix
  and locally sparse suffix coordinates. Separate suffix capacity management
  from rarer prefix-space maintenance. Define occupancy hysteresis and an
  explicit depth/LK1 budget; a split must leave reserve in both pieces.
  Every exposed Path remains fully materialized and independently sortable.
- **Why it may address the cliff:** Local suffix redistribution/splits may
  isolate a hotspot rather than repeatedly preparing ever-larger regions.
  This is a mapping from order-maintenance indirection, not its hidden-tag
  implementation or constant-time result [BCDFZ, Section 2.1].
- **Expected tradeoff:** Chunk metadata and extra suffix bytes; fewer broad
  repairs are unverified. Prefix changes rewrite every affected member Path,
  so a top-level relabel can still be large. Chunk boundaries must not turn
  overflow into a new synchronous cliff.
- **Atomicity difficulty:** High. Stage chunk splits/merges, prefixes and all
  affected suffix Paths together; preserve stable comparator rank and the
  AVL search invariant. No lazy resolver may replace public Path/LK1 values.
  Removal only retires membership/empty metadata without survivor relabeling;
  any coordinate-changing merge is staged during a subsequent insertion.
- **Implementation complexity:** High relative to direction 1; explicit
  prefix/suffix composition must honor increasing levels and exterior bounds.
- **Prototype would test:** Validate flattening and boundary ordering first;
  replay the same frozen campaign and add focused boundary/split correctness
  and OOM cases without changing campaign definitions. Count all prefix-induced
  rewrites, bytes and peak preparation memory. Compare against direction 1;
  abandon if apparent internal savings disappear after external costs.

**Priority:** Prototype direction 1 first. Direction 2 is conditional, not a
request to build both. No third direction merits implementation yet. Neither
revives a rejected candidate, changes Path/LK1 semantics, or commits 3.2/V4
scope. Phase 2 has not started.

### Primary references

- **DS:** Dietz and Sleator, *Two Algorithms for Maintaining Order in a List*,
  STOC 1987; [author-hosted CMU technical-report version (1988)](https://www.cs.cmu.edu/~sleator/papers/maintaining-order.pdf).
- **BCDFZ:** Bender, Cole, Demaine, Farach-Colton and Zito, *Two Simplified
  Algorithms for Maintaining Order in a List*, ESA 2002, Sections 2.1–3 and 5;
  [author-hosted PostScript](https://www3.cs.stonybrook.edu/~bender/pub/esa2002-orders.ps),
  [DOI](https://doi.org/10.1007/3-540-45749-6_17).
- **BKS:** Bulánek, Koucký and Saks, *Tight Lower Bounds for the Online
  Labeling Problem*, SIAM J. Computing 44(6), 2015;
  [author-hosted manuscript](https://sites.math.rutgers.edu/~saks/PUBS/labeling-122214.pdf).
- **BBCKS:** Babka, Bulánek, Čunát, Koucký and Saks, *On Online Labeling with
  Polynomially Many Labels*, [2012 primary preprint](https://arxiv.org/abs/1210.3197),
  Introduction and Theorem 2.1. Its historical discussion of randomized
  results is not a statement of current randomized bounds.
- **BDF:** Bender, Demaine and Farach-Colton, *Cache-Oblivious B-Trees*,
  SIAM J. Computing 35(2), 2005, Section 2.3/Theorem 2.5;
  [author-hosted paper](https://erikdemaine.org/papers/CacheObliviousBTrees_SICOMP/paper.pdf).
- **BH:** Bender and Hu, *An Adaptive Packed-Memory Array*, PODS 2006;
  [author-hosted paper](https://www3.cs.stonybrook.edu/~bender/newpub/2006-BenderHu-pods-apma.pdf).
