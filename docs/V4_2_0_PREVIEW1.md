# Full v4.2.0 Preview.1 candidate

Implementation revision: 22544cf283694dd27cef203caba9cf1838845947. Published Stable is v4.1.0. The candidate is provisional.

The Windows 45-trace ablation compared unchanged v4.1.0, ancestor comparison, affected-range location updates, and the combination in rotated order. Median candidate/control cell ratios were 0.90538, 0.82874 and 0.71422 respectively. Combined compare-heavy ratio was 0.53093, and worst combined cell ratio was 0.86120. Seven fresh formal processes per cell followed repeat-zero warmup. See [raw screen](../benchmarks/research/full-v4.2.0-preview.1/mutation-screen.csv). This is Windows evidence. The full cross-host campaign remains outstanding.

[Architecture and decision](design/V4_2_0_PREVIEW1_ARCHITECTURE.md) and [consumer ledger](FULL_CONSUMER_HANDOFF.md) bind the implementation revision. The historical guide baseline covers released source 70d7f0ef5857b54381b863d292a213909d87459f.

| Guide | Candidate synchronization | Verification scope |
|---|---|---|
| Walkthrough | Comparison and shifted locations | Source inspection and live-order CTest |
| API reference | Frozen compare and handle contracts | 64-function manifest |
| Internals | Height walk and changed-range writes | Mutation and snapshot CTests |
| Validation | Windows release and ablation evidence | Full remote matrix pending |

Linux comparative timing, current same-campaign OM and v4.0 controls, adversarial coverage beyond the bounded label test, resource/tail profiling and actual OrdWeave candidate substitution remain open.
