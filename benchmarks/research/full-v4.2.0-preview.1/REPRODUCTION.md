# Preview.1 final comparison reproduction

Run the commands from a Full checkout containing this directory. Use Windows or native Linux, Python 3.12 or later, GCC, CMake, Ninja, Git, and Rust/Cargo. On Windows use the MinGW toolchain for C. The hosted capture resolves the three Full revisions directly and uses the j-hui git dependency locked at ac4b902d103beb30cee3316d20cd10701f52d15a. The public Full research archive contains its source and MIT license and the C baseline sources and notices. Registry dependencies require network access.

    python -B benchmarks/research/full-v4.2.0-preview.1/replay/replay.py <new-workspace>

The replay wrapper stages the published Full archive and checks each trace digest. It checks out the two released source commits and the production candidate, then builds the seven adapters in Release configurations. It writes new data under the supplied empty path. It retains the original measured harness separately from the analysis. The hosted workflow runs the same collector and CMake recipe with the original input preparation. The Windows C timer is QPC and both platforms use Rust Instant. The first Linux capture used the archived C wall clock. A separate corrective Linux run uses CLOCK_MONOTONIC and is the primary Linux result. The public wrapper applies the same single clock substitution only to its staged Linux C harness. Each trace has 20,000 operations. Times cover the same replay range and its dispatch/checks, excluding initialization and verification after replay.

The collector rotates order among seven implementations and runs serial children. It separates diagnostic requests from production timing, discards repeat zero from formal analysis, preserves stdout/stderr, and enforces the original 512 MiB available-memory guard and 120-second child timeout. Requested storage excludes common harness buffers. Linux process memory and Rust RSS fields are unavailable in this harness.

To recompute the captured comparison, run:

    python -B benchmarks/research/full-v4.2.0-preview.1/summarize_matrix.py

The same-language microcontrol uses compare_screen.c linked separately against the v4.1.0 and candidate production archives. Build with GCC C17 -O3 -DNDEBUG and the Full public/private include paths. Run Stable, candidate, candidate, Stable. The raw control files preserve both headers and captures. Discard each repeat zero. The internal ancestor loop can inline and is a diagnostic comparison, while the two public paths are matched external calls. The ordinal probe is a fixed-state limit and includes no dynamic maintenance. The cross-language ranking includes native implementation and wrapper differences.

The public wrapper's --c-smoke mode was executed on Windows from an empty staging path. It fetched and built all three pinned Full sources and passed the five independent C semantic fixtures. The Rust adapters were built with --locked and passed the semantic fixture on both hosted jobs. The complete public wrapper has not been executed locally with Rust.
