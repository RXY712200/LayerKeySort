# Reproduction

Run replay_live.py with a new output directory from the repository root. It uses the pinned public archive, GCC/CMake/Ninja and Cargo --locked, and verifies the frozen trace hashes. Archive with archive_matrix.py and recompute summarize_matrix.py. See final-matrix for both complete raw receipts and toolchain metadata.

Run run_lifecycle.py with a new output directory on native Windows or Linux, after obtaining the exact Git objects in the script. analyse_lifecycle.py reconciles raw stdout and regenerates per-cell statistics. Historical probe folders retain the asymmetric Treap scan and are excluded from final estimates. The failed Linux fetch receipts document a staging failure before measurement. The later nonshallow source staging completed. scale.py and holdouts.py use the actual strict Windows Release build. Holdout diagnostics are separate from timing.

### Recompute wall-clock and phase ratios

Run `python analyse_lifecycle.py <copied receipt directory>` for each archived host. The script checks process stdout against raw rows before writing analysis.json. Use copies to keep archived snapshots intact. The existing `ratios` key summarizes matched median summed phases. `wall_ratios` summarizes matched median process-wall times independently and appears only when the input captured wall time. Warmup and diagnostic cases are excluded from both ratios.
