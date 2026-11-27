# Full v4.2.0 — overall native GLib comparison

This focused task supersedes the earlier nine-step proposal at ab76b79316af8114dcf738e0983d516bc3184d0d. It is one comparative research task, not a product roadmap.

Baseline: released Full v4.2.0, f751e1ee2afd682874281d6dc495969f9eca49eb.
Existing Windows experiment and code: https://github.com/RXY712200/LayerKeySort/tree/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420

Extend that experiment into a single overall Full-versus-native-GLib comparison. Reuse its harness and results, and reuse existing Full research traces/diagnostics where applicable.

- Cover native Windows and Linux, preferably matching GLib 2.90.1; record exact versions and environment.
- Cover equivalent explicit order operations (insert/remove/move/precedence), managed order operations (sorted insert/lookup/remove), and representative mixed workloads (query-heavy, balanced, mutation-heavy, hotspot). Include arbitrary live-pair precedence after mutation. Preserve the original four stable sort distributions as secondary evidence, not an optimization target.
- Keep the existing 1k/10k/100k/1m scales where practical. For added stochastic workloads use three fixed seeds and three measured repetitions after warmup. Reuse unchanged prior measurements rather than rerunning unnecessarily. Use common workload definitions across platforms and exact matched traces within each host.
- Report elapsed time, memory and create/populate/destroy costs. Use comparable process memory; add isolated container allocation metrics only if straightforward and validated on both sides. Existing LKS diagnostic data may be referenced with its original limits. Add latency distributions only where the existing harness can measure them reliably with low overhead. Do not build a separate instrumentation project.
- Verify semantic equivalence and correctness, preserve raw data and unfavorable observations, disclose environment/measurement limitations and unmeasured cases. Do not force comparisons of non-equivalent snapshot/history/Group/Batch functionality.
- Publish one concise integrated report, reproducible code and new raw results on a research branch. Distinguish prior and new measurements. Stop when the bounded comparison is complete.

Out of scope: new long-duration stress campaigns, new Boost/LLVM/other external controls, duplicating historical Full research, causal/ablation/theoretical investigations, paper writing, product changes or releases, OrdWeave system-level research. These are not requirements for this task. No performance target or requirement to beat GLib. No repeated audits.