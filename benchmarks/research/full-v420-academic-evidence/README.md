# Overall native GLib comparison — Full v4.2.0

[Research plan](PLAN.md) · [Integrated report](REPORT.md) · [All summarized cells](results/summary.csv) · [Per-seed summaries](results/seed-summary.csv)

This directory contains one bounded native Windows/Linux comparison against actual GLib 2.90.1. It extends the [original Windows campaign](https://github.com/RXY712200/LayerKeySort/tree/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420), preserving its observations rather than repeating them. Full is fixed at released v4.2.0 (`f751e1ee2afd682874281d6dc495969f9eca49eb`). Production `src`, public `include` and `cmake/ProductionSources.cmake` must match that pin. No production build, API, version or published asset is changed.

## Data and code

- `benchmark.c`: adapted original public-API harness, native clock/memory sampling, added mixed traces, independent untimed rank oracle and separate create/populate/destroy timings.
- `run.py`: builds the single executable for both implementations, enforces admission/timeouts, runs fixed traces, checks paired fingerprints and preserves every sample/error/skip.
- `linux.sh`: obtains and hashes official GLib source, builds a native release library in a local prefix and executes the Linux campaign.
- `analyze.py`: validates the complete captured dataset, cross-platform trace/result identity and three repetitions per seed, then regenerates `REPORT.md`, `summary.csv` and `seed-summary.csv`. It does not execute benchmarks.
- `results/windows-prior`: exact original Git-blob copies of raw data and environment plus their provenance; no new runs.
- `results/windows-new`: new local native Windows observations and environment.
- `results/linux-new`: new native Linux observations, environment and GLib build provenance.
- `results/execution.json`: actual research workflow/run identity and evidence acquisition details.
- `results/smoke-windows`: 224 successful small-scale protocol-validation runs, excluded from the comparative campaign summaries.

Seeds, operation probabilities, input distributions, timing boundaries and semantic differences are defined in the report. The added mixtures use arbitrary currently live query pairs; the old mixed trace retains its adjacent-pair definition. A rank oracle is necessary to validate arbitrary post-mutation answers without quadratic reference scanning. It is not a timed comparator or a third benchmark control. Its final order is also checked against independent ID-linked reference updates.

The original Linux Meson log is archived losslessly as `results/linux-new/glib-meson-log.txt.gz`, including its generated whitespace. `results/execution.json` records both compressed and original SHA-256 values. Python's `gzip.decompress()` recovers the exact original bytes; the original is also retained in the workflow artifact. Source-file hashes in environment records describe the working files used by the compiler; Git may normalize Windows/Linux text line endings. Both native campaigns used executable protocol commit `27ae443abf1181491ab318e22af97de5f5a3cb52`.

## Native Windows reproduction

Requirements: Python 3, native MSYS2 UCRT64 GCC and the same GLib 2.90.1-1 release package/runtime dependencies used by the original campaign. The original [instructions](https://github.com/RXY712200/LayerKeySort/blob/298079288dcf6c75ac3ec04461c8b59259c2193c/benchmarks/research/glib-full-v420/README.md) provide the package URL, dependency names and checksum. Package SHA-256: `9a7826349279bcca0c935b01880d812be9de9609aef2205be68fd7a6128c1695`; native DLL SHA-256: `3a1abec0184b238db54399cb9243393d9436d96b8fdb9bd9f2875da207c5b458`.

From the repository root, with the extracted GLib UCRT64 prefix:

```powershell
python benchmarks/research/full-v420-academic-evidence/run.py --gcc D:/msys64/ucrt64/bin/gcc.exe --glib-prefix D:/LayerKeySort/.local/glib-full-native/build/glib-deps/ucrt64 --include-legacy --smoke --output build/glib-overall-smoke
python benchmarks/research/full-v420-academic-evidence/run.py --gcc D:/msys64/ucrt64/bin/gcc.exe --glib-prefix D:/LayerKeySort/.local/glib-full-native/build/glib-deps/ucrt64 --output build/glib-overall-windows-reproduction
```

Change those machine-specific executable/prefix paths to your local installation. Put its UCRT64 DLL directory on the child-process PATH if it is not already inherited; the runner prepends the supplied GCC and GLib directories without changing global settings. Omit `--include-legacy` for the real Windows extension: the original twelve scenarios already have preserved measurements. The small smoke run includes them to confirm that the adapted harness retains their semantics. Native GLib is dynamically linked; Full and both adapters are compiled together at C17 `-O3 -DNDEBUG`, strict warnings and no LTO. The MSYS2 GLib release package build uses GCC 16.2.0, release mode and disabled debug; exact package provenance is in the original evidence.

## Native Linux reproduction

On Ubuntu 24.04, install the native prerequisites:

```sh
sudo apt-get update
sudo apt-get install -y gcc ninja-build pkg-config libffi-dev libpcre2-dev zlib1g-dev gettext python3-venv
bash benchmarks/research/full-v420-academic-evidence/linux.sh
```

`linux.sh` downloads [official GLib 2.90.1 source](https://download.gnome.org/sources/glib/2.90/glib-2.90.1.tar.xz), verifies SHA-256 `93c941aa17d5eb1d53fe838365f29a8b4e539c222a256d974ec8f30fc413e396`, and builds it using Meson release optimization with debug disabled. Meson and packaging are installed in an ignored local Python virtual environment; the resolved Meson version and full build-option/log evidence are archived. Native GLib goes into `build/native-glib`, not a system prefix. The runner explicitly selects that library via pkg-config and `LD_LIBRARY_PATH` and records `ldd` and the actual library hash/runtime version. Raw results go into `build/glib-overall-linux-results`.

The isolated [research workflow](../../../.github/workflows/research-glib-overall.yml) runs this command on a native Ubuntu runner only for the dedicated research branch when executable protocol files change. It does not modify or replace production CI. Report/data-only commits do not rerun the campaign. The completed native Linux run is linked in `results/execution.json`.

To use an already-built matching native prefix without rebuilding GLib:

```sh
python3 benchmarks/research/full-v420-academic-evidence/run.py --glib-prefix build/native-glib --include-legacy --output build/glib-overall-linux-reproduction
```

## Analyze existing evidence without rerunning

```sh
python benchmarks/research/full-v420-academic-evidence/analyze.py
```

New measurements should use fresh output directories. Never overwrite the captured raw CSVs to obtain a different outcome. The analyzer expects the full successful bounded campaign: 384 prior Windows rows, 512 added Windows rows and 896 Linux rows, with all paired/cross-host fingerprints matching. It will reject incomplete evidence rather than silently drop failed or missing observations. On a less-resourced reproduction machine the runner still preserves skips/errors, and the full-campaign analyzer will correctly decline to label it complete.

The memory admission rule is `(96 MiB + 512*N) <= half currently available memory`, including a Linux cgroup-v2 cap where present. A child timeout is 240 seconds. Neither rule is weakened to force a large cell. The original campaign used its original 256*N estimate, retained as historical metadata; both campaigns admitted all requested cells.

## Interpretation and licensing

Memory is whole-process resident high-water usage; Windows also supplies private-commit high-water usage. Linux unavailable private commit is represented by 0 and is not a measured zero. Payloads, trace/reference arrays and shared libraries are included. No requested allocation-byte equivalence or isolated container memory claim is made. Added oracle memory prevents treating new mixed totals as directly interchangeable with legacy-harness totals.

Create/populate/destroy costs are available for new measurements, not retroactively invented for the old Windows data. The retained stable-sort distributions are secondary evidence; no sorting optimization or new external control was introduced. Full snapshot/history/Group/Batch features have no forced GSequence equivalent. No per-operation latency instrumentation was added to the aggregate-loop harness.

Research harness code is covered by the repository MIT license. It adapts the original MIT-licensed benchmark at `298079288dcf6c75ac3ec04461c8b59259c2193c`. GLib is LGPL-2.1-or-later; source/build URLs and identities are recorded, but its binaries, downloaded source tree and dependency archives are not committed or redistributed here. All measurements, including unfavorable ones and observed ranges, remain available. No claim of independent acceptance or universal performance superiority is made.
