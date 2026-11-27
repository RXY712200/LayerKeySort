# Full v4.2.0 academic evidence plan

Planning only; no new experiments launched. Released source: f751e1ee2afd682874281d6dc495969f9eca49eb (v4.2.0).
Existing native Windows GLib 2.90.1 report: https://github.com/RXY712200/LayerKeySort/blob/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420/REPORT.md

Scope: dynamic order maintenance, not universal sorting superiority. No production, API, wire, version or release changes. OrdWeave stays independent. Keep all adverse results.

## Reuse
Consult docs/FULL_RC1_EVIDENCE.md, V4_2_0_PREVIEW1.md, V4_2_0_PREVIEW1_2.md, V4_2_0_PREVIEW1_2_1.md, V4_2_0_PREVIEW2.md, V4_2_0_PREVIEW3.md, IMPLEMENTATION_INTERNALS.md and FULL_SELECTION_GUIDE.md. Existing 45-cell live matrices, three seeds, seven repetitions, affected-range/ancestry/label/capacity ablations, hotspot/tail profiles, sparse million-item shrink/regrow, requested-byte diagnostics, classical tag/list-range OM controls and aligned history lifecycle evidence cover parts of requirements 2–10, 13, 15–18. Retain original pins and limitations; do not call them new GLib measurements.

## Stage 1 — native GLib reproducibility and dynamic workloads (1–6)
1.1 Port existing harness to native Linux; pin GLib 2.90.1 where feasible. Reproduce original workloads as bridge; record host/compiler/package/linkage/allocator and clock. Compare within host, not absolute speeds across machines.
1.2 Reuse frozen dynamic traces: query-heavy, mutation-heavy, balanced, hotspot/local, distant moves. Five prespecified independent seeds, five fresh measured processes after warmup; 1k/10k/100k and bounded 1m subset. Include arbitrary current live pairs after mutations, not only adjacent pairs. Same semantics, payload, traces and independent oracle. Static sorting only bridge reproduction.
1.3 Sustained churn subset at 100k/1m, balanced and hotspot, three seeds and three repetitions, up to 10m trace steps where safely admitted. Track per-epoch throughput and arbitrary-pair comparison, growth/shrink and correctness checkpoints. Preserve timeout/admission skips.
Gate: correct and equivalent operations, trace hashes, raw observations, documented unavailable cells; no speed target.

## Stage 2 — resource, tail and lifecycle evidence (7–10)
2.1 Separate validated allocation diagnostics for both implementations: requested live/peak bytes, allocation/free counts, cumulative traffic; exclude shared harness and borrowed payload consistently. If GLib allocation coverage cannot be verified, mark metric unavailable. RSS/private commit and retained allocator memory are distinct. Production timing uninstrumented.
2.2 Reuse sustained traces for per-operation type latency distributions (p50/p95/p99/p99.9/max, sample counts and timing overhead), per-epoch memory trajectory and structural costs, label repairs/fallback/AVL. Separate diagnostic timing from production. No pooled unlike operations or unsupported quantiles.
2.3 Equivalent create/populate/replay/destroy and full lifecycle cost; cleanup and baseline checks. Do not claim GLib history/snapshot equivalence. Reuse old history lifecycle evidence separately.
Gate: comparable boundaries, validated accounting, raw adverse and tail observations.

## Stage 3 — alternative controls, mechanisms and synthesis (11–18)
3.1 Screen Boost.MultiIndex random_access for stable iterator relative edits and positional comparison; sequenced alone lacks cheap arbitrary precedence. LLVM Instruction::comesBefore is intra-basic-block with invalidation/renumbering semantics; scoped mechanism or limited comparison only. Reuse existing pinned j-hui native tag/list-range and Threaded Treap. Add at most one new external runtime control after semantic screening, record exclusions.
3.2 Reconcile existing ablations with released source; only missing decisive research-only diagnostics/ablations (maximum two new variants unless scope explicitly expanded). Examine compare paths, block/handle memory, shifts, allocation and index work; distinguish observation, hypothesis and causal evidence. No production modifications.
3.3 Document complexity from source and literature: N live occurrences, B capacity, M blocks, R bounded repair window; fast path vs worst case, allocator/comparator costs, representation and auxiliary space. No unconditional O(1) claim. Evidence-to-claim matrix, threats to validity, applicable boundaries and remaining gaps, including synthetic versus external field evidence. No paper drafting or publication guarantee.
Gate: all 18 requirements mapped to existing/new/excluded/open evidence, claims traceable, limitations explicit.

Nine fixed steps, three stages. Stage 1 first; review preceding results before stages 2 and 3. Freeze detailed grids and resource budgets before measurement. Repairs stay within the same step. No new product release.
