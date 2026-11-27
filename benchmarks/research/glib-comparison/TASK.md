# Native GLib versus LayerKeySort Full v4.2.0 — initial comparison

Conduct a focused native GLib versus LayerKeySort Full v4.2.0 benchmark (released source: f751e1ee2afd682874281d6dc495969f9eca49eb).

Compare:
1. LksOrder vs GSequence: insertion, removal, movement, precedence comparison, and mixed workloads.
2. LksManagedOrder vs GSequence: sorted insertion, lookup, and removal.
3. lks_sort vs native GLib stable array sorting: random, reverse, nearly sorted, and duplicate-heavy inputs.

Use actual GLib, not a reimplementation. Record its exact version.

Use identical workloads, equivalent operation semantics, release optimization settings, and payloads. Test 1,000, 10,000, 100,000, and 1,000,000 elements where practical, with three measured repetitions after warmup.

Measure elapsed time and comparable peak memory. Verify output correctness and distinguish process memory from requested allocation bytes. Preserve unfavorable results and disclose semantic differences.

Reuse existing benchmark infrastructure where practical. Use one available native platform for this initial experiment.

Publish benchmark code, raw results, and a concise comparison report to a separate GitHub research branch.

Do not modify production code, change versions, involve OrdWeave, investigate root causes, conduct repeated audits, or write a paper. Complete this single experimental task and report in English.

This file archives the already-issued task. If execution is underway, continue the existing work rather than starting it again. No additional experimental scope or validation stage is introduced.
