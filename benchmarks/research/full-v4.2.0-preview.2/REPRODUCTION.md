# Preview.2 reproduction and evidence

From an exact Full checkout, with Python 3.12+, GCC C17, Git, CMake and Ninja, ensure the pins listed by `run.py` and the frozen public archive `f41b7be182c2ef766ee0dec1211500f7dd762f54` exist in the local object store. Fetch missing SHA objects by exact revision. Then run:

```text
python -B benchmarks/research/full-v4.2.0-preview.2/run.py <new-output> fe00536e7bd7f4e5128432ea4a29e54158074903
python -B benchmarks/research/full-v4.2.0-preview.2/analyse.py <new-output>
```

Omit the second argument to stage the source-only wire prototype described by `wire.inc`; `Linux` and `Windows` in this archive are that screen. `final-Linux` and `final-Windows` use exact production binaries. The hosted screen used private research revision `a0c0dc9784d55e143d5aa9a4f406f2b993c58389` and [run 45403456609](https://github.com/RXY712200/lks0011/actions/runs/45403456609). Exact candidate and 45-cell live replay used `f720828df2e9f7c12eab01289479d108174f18fd` and [run 45404122363](https://github.com/RXY712200/lks0011/actions/runs/45404122363). The public recipes require no access to the private repository.

`run.py` stages the archives and builds strictly flagged Release libraries with separate timing/diagnostic binaries. It applies 512 MiB available memory plus the prospective per-case bound and a 120-second timeout. Raw gzip receipts store admission, commands, output and stderr. The analysis verifies rows against subprocess output and reports per-cell medians/ranges/IQR. The early final archive has two link receipts labeled `sparse-diag-link` with distinct executable destinations; the sparse timing/diagnostic measurements label exact source variants. The metadata's repeated sparse binary key refers to the later, candidate sparse executable. Both explicit build paths are in the receipts. Timed phase sum excludes driver verification, setup of external buffers and some teardown between phases. It is a consistent observed lifecycle boundary, not an estimate of total process wall time. Requested bytes exclude fixture and caller-owned wire buffers.

For the supplemental QPC/RSS profile, build both exact Full controls with `-DLKS_BUILD_BENCHMARKS=ON`, then call `scale_windows.py <baseline-build> <candidate-build> <new-output>`. The four fixed traces use an independent seed and the existing native sequence/identity/currentness oracle. The diagnostic samples report requested memory and allocator traffic. RSS is sampled separately on native Windows. The percentile samples include initial build, and the phase lines distinguish construction from mutation wall time.

The native OrdWeave source, reproduction commands, build/test outcomes, original and final receipts and larger/fault holdouts are in the [research archive](https://github.com/RXY712200/OrdWeave/tree/1e40955a783f91ec835170bea664bfa8e324b465/benchmarks/research/full-preview2-20261124). The earlier generated fixture and native archive remain at their original commit. No current application trace represents observed human activity.

Local core validation used strict GCC Debug and Release with `-DLKS_BUILD_BENCHMARKS=ON`, `cmake --build <build> --parallel 3`, and `ctest --test-dir <build> --output-on-failure`. Run `tools/verify_v4_api.py`, `tools/validate_distribution.py` and `tools/validate_release_assets.py` with Python from the root. `verification` holds the actual local logs and a pre-screen strict compile failure (prototype-only helper compiled unused in the Full adapter, corrected before timing).
