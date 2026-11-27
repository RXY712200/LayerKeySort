"""Native Windows public-API comparison; Python standard library only."""
import argparse
import csv
import ctypes
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import time
import urllib.request

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PIN = "f751e1ee2afd682874281d6dc495969f9eca49eb"
PACKAGE = "mingw-w64-ucrt-x86_64-glib2-2.90.1-1-any.pkg.tar.zst"
URL = "https://repo.msys2.org/mingw/ucrt64/" + PACKAGE
SCENARIOS = ["insert", "remove", "move", "compare", "mixed", "managed_insert",
             "managed_lookup", "managed_remove", "sort_random", "sort_reverse",
             "sort_nearly", "sort_duplicates"]
FIELDS = ["implementation", "scenario", "n", "trace_steps", "elapsed_ms",
          "baseline_working_set_bytes", "peak_working_set_bytes", "peak_commit_bytes",
          "trace_hash", "result_hash", "final_count", "correctness"]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(args, **kwargs):
    return subprocess.check_output(args, cwd=ROOT, text=True, **kwargs).strip()


class Memory(ctypes.Structure):
    _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
        (name, ctypes.c_ulonglong) for name in
        ("physical", "available", "pagefile", "available_pagefile", "virtual",
         "available_virtual", "extended")]


def available_memory():
    m = Memory()
    m.length = ctypes.sizeof(m)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)):
        raise OSError("GlobalMemoryStatusEx failed")
    return m.available


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--gcc", default=r"D:\msys64\ucrt64\bin\gcc.exe")
    p.add_argument("--output", type=Path, default=HERE / "results")
    p.add_argument("--smoke", action="store_true")
    args = p.parse_args()
    if platform.system() != "Windows":
        raise SystemExit("This initial protocol targets native Windows only")
    # Refuse a comparison against a silently changed production implementation.
    command(["git", "cat-file", "-e", PIN + "^{commit}"])
    if command(["git", "diff", PIN, "--", "src", "include", "cmake/ProductionSources.cmake"]):
        raise SystemExit("Production source differs from released v4.2.0")
    deps = ROOT / "build/glib-deps"
    deps.mkdir(parents=True, exist_ok=True)
    package = deps / PACKAGE
    if not package.exists():
        urllib.request.urlretrieve(URL, package)
    if not (deps / "ucrt64/include/glib-2.0/glib.h").exists():
        subprocess.run(["tar", "-xf", str(package), "-C", str(deps)], check=True)
    glib = deps / "ucrt64"
    exe = ROOT / "build/glib-benchmark.exe"
    sources = re.findall(r"src/[\w]+\.c", (ROOT / "cmake/ProductionSources.cmake").read_text())
    build = [args.gcc, "-std=c17", "-O3", "-DNDEBUG", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-I", "include", "-I", str(glib / "include/glib-2.0"),
             "-I", str(glib / "lib/glib-2.0/include"), str(HERE / "benchmark.c"), *sources,
             "-L", str(glib / "lib"), "-lglib-2.0", "-lpsapi", "-o", str(exe)]
    subprocess.run(build, cwd=ROOT, check=True)
    env = dict(os.environ)
    env["PATH"] = str(glib / "bin") + os.pathsep + str(Path(args.gcc).parent) + os.pathsep + env["PATH"]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    metadata = dict(source_commit=PIN, source_equivalence="git diff PIN -- src include cmake/ProductionSources.cmake empty",
                    checkout_head=command(["git", "rev-parse", "HEAD"]),
                    os=platform.platform(), machine=platform.machine(), cpu=platform.processor(),
                    cpu_detail=command(["powershell", "-NoProfile", "-Command", "Get-CimInstance Win32_Processor | Select-Object -ExpandProperty Name"]),
                    compiler=command([args.gcc, "--version"]), build_command=build,
                    glib_runtime=command([str(exe), "version"], env=env), package_url=URL,
                    package_sha256=sha(package), glib_dll_sha256=sha(glib / "bin/libglib-2.0-0.dll"),
                    executable_sha256=sha(exe), benchmark_sha256=sha(HERE / "benchmark.c"),
                    runner_sha256=sha(Path(__file__)), seed="0x91e10da5c79e7b1d",
                    started_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                    timing="QPC timed API loop only; setup, independent oracle, validation and destruction excluded",
                    memory="Windows process PeakWorkingSetSize and PeakPagefileUsage, including setup and validation; not requested allocation bytes",
                    memory_guard="96 MiB + 256*N <= half current available physical RAM; no weakening",
                    timeout_seconds=180, repetitions=3, warmups=1, fresh_process_per_sample=True)
    (output / "environment.json").write_text(json.dumps(metadata, indent=2) + "\n")
    for name in (".BUILDINFO", ".PKGINFO"):
        if (deps / name).exists():
            (output / ("glib" + name + ".txt")).write_bytes((deps / name).read_bytes())
    errors, skipped, rows = [], [], []
    with (output / "raw.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, ["phase", "repetition", "position", "available_memory_bytes", *FIELDS])
        writer.writeheader()
        for n in ([1000] if args.smoke else [1000, 10000, 100000, 1000000]):
            for scenario in SCENARIOS:
                for rep in range(4):
                    pair = []
                    for pos, impl in enumerate((["full", "glib"] if rep % 2 == 0 else ["glib", "full"])):
                        available = available_memory()
                        if 96 * 1024**2 + 256 * n > available // 2:
                            skipped.append(dict(n=n, scenario=scenario, repetition=rep, implementation=impl,
                                                available_memory_bytes=available, reason="memory admission"))
                            continue
                        try:
                            result = subprocess.run([str(exe), impl, scenario, str(n)], env=env,
                                                    text=True, capture_output=True, timeout=180)
                            if result.returncode:
                                raise RuntimeError(f"exit {result.returncode}: {result.stderr}")
                            values = next(csv.reader(io.StringIO(result.stdout)))
                            if len(values) != len(FIELDS) or values[-1] != "OK":
                                raise RuntimeError("malformed output: " + result.stdout)
                            row = dict(zip(FIELDS, values))
                            row.update(phase="warmup" if rep == 0 else "measured", repetition=rep,
                                       position=pos, available_memory_bytes=available)
                            writer.writerow(row)
                            f.flush()
                            rows.append(row)
                            pair.append(row)
                        except (RuntimeError, subprocess.TimeoutExpired) as e:
                            errors.append(dict(n=n, scenario=scenario, repetition=rep, implementation=impl, error=str(e)))
                    if len(pair) == 2 and any(pair[0][k] != pair[1][k] for k in ("trace_hash", "result_hash", "final_count")):
                        errors.append(dict(n=n, scenario=scenario, repetition=rep, error="paired correctness hash mismatch"))
                print(n, scenario, "complete", flush=True)
    (output / "exceptions.json").write_text(json.dumps(dict(errors=errors, skipped=skipped), indent=2) + "\n")
    columns = ["scenario", "n", "implementation", "samples", "median_ms", "min_ms", "max_ms",
               "median_peak_working_set_MiB", "median_peak_commit_MiB", "median_baseline_working_set_MiB"]
    with (output / "summary.csv").open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(columns)
        for n in sorted({int(r["n"]) for r in rows}):
            for scenario in SCENARIOS:
                for impl in ("full", "glib"):
                    group = [r for r in rows if r["phase"] == "measured" and int(r["n"]) == n and r["scenario"] == scenario and r["implementation"] == impl]
                    if not group:
                        continue
                    times = [float(r["elapsed_ms"]) for r in group]
                    writer.writerow([scenario, n, impl, len(group), statistics.median(times), min(times), max(times),
                                     *[statistics.median(float(r[k]) for r in group) / 1024**2 for k in
                                       ("peak_working_set_bytes", "peak_commit_bytes", "baseline_working_set_bytes")]])
    print(json.dumps(dict(samples=len(rows), measured=sum(r["phase"] == "measured" for r in rows), errors=len(errors), skipped=len(skipped))))
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
