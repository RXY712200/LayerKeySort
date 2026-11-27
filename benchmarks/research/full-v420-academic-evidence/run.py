"""Bounded common native Windows/Linux campaign. Standard-library Python runner."""
import argparse
import csv
import ctypes
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PIN = "f751e1ee2afd682874281d6dc495969f9eca49eb"
SEEDS = ["0x91e10da5c79e7b1d", "0x243f6a8885a308d3", "0x13198a2e03707344"]
LEGACY = ["insert", "remove", "move", "compare", "mixed", "managed_insert",
          "managed_lookup", "managed_remove", "sort_random", "sort_reverse", "sort_nearly", "sort_duplicates"]
NEW = ["mix_query", "mix_balanced", "mix_mutation", "mix_hotspot", "lifecycle_order", "managed_lifecycle"]
FIELDS = ["implementation", "scenario", "n", "trace_steps", "elapsed_ms", "baseline_resident_bytes",
          "peak_resident_bytes", "peak_private_commit_bytes", "trace_hash", "result_hash", "final_count",
          "correctness", "create_ms", "populate_ms", "destroy_ms"]


def execute(args, **kw):
    return subprocess.check_output(args, cwd=ROOT, text=True, **kw).strip()


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def available_memory():
    if platform.system() == "Windows":
        class M(ctypes.Structure):
            _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
                (n, ctypes.c_ulonglong) for n in ("total", "avail", "page", "apage", "virt", "avirt", "ext")]
        m = M()
        m.length = ctypes.sizeof(m)
        if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m)):
            raise OSError("GlobalMemoryStatusEx")
        return m.avail
    # Hosted runner has no container constraint; account for cgroup limits if reproduced in one.
    values = dict(line.split(":", 1) for line in Path("/proc/meminfo").read_text().splitlines())
    available = int(values["MemAvailable"].split()[0]) * 1024
    cap = Path("/sys/fs/cgroup/memory.max")
    used = Path("/sys/fs/cgroup/memory.current")
    if cap.exists() and used.exists() and cap.read_text().strip() != "max":
        available = min(available, max(0, int(cap.read_text()) - int(used.read_text())))
    return available


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--gcc", default="gcc")
    p.add_argument("--glib-prefix", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--smoke", action="store_true")
    p.add_argument("--include-legacy", action="store_true", help="Linux: original scenarios; Windows: reuse archived measurements")
    args = p.parse_args()
    windows = platform.system() == "Windows"
    assert windows or platform.system() == "Linux"
    assert not execute(["git", "diff", PIN, "--", "src", "include", "cmake/ProductionSources.cmake"])
    prefix = args.glib_prefix.resolve()
    env = dict(os.environ)
    env["PATH"] = str(prefix / "bin") + os.pathsep + str(Path(args.gcc).resolve().parent) + os.pathsep + env["PATH"]
    if windows:
        glib_flags = ["-I", str(prefix / "include/glib-2.0"), "-I", str(prefix / "lib/glib-2.0/include"), "-L", str(prefix / "lib"), "-lglib-2.0", "-lpsapi"]
    else:
        env["PKG_CONFIG_PATH"] = str(prefix / "lib/pkgconfig")
        env["LD_LIBRARY_PATH"] = str(prefix / "lib") + os.pathsep + env.get("LD_LIBRARY_PATH", "")
        glib_flags = shlex.split(execute(["pkg-config", "--cflags", "--libs", "glib-2.0"], env=env))
    build_dir = ROOT / "build/glib-overall"
    build_dir.mkdir(parents=True, exist_ok=True)
    exe = build_dir / ("benchmark.exe" if windows else "benchmark")
    sources = re.findall(r"src/\w+\.c", (ROOT / "cmake/ProductionSources.cmake").read_text())
    build = [args.gcc, "-std=c17", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", "include", str(HERE / "benchmark.c"), *sources, *glib_flags, "-o", str(exe)]
    subprocess.run(build, cwd=ROOT, env=env, check=True)
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    cpu = execute(["powershell", "-NoProfile", "-Command", "Get-CimInstance Win32_Processor | Select-Object -ExpandProperty Name"]) if windows else execute(["lscpu"])
    metadata = dict(platform=platform.system(), os=platform.platform(), cpu=cpu, gcc=execute([args.gcc, "--version"]),
                    glib_runtime=execute([str(exe), "version"], env=env), source_pin=PIN,
                    checkout_commit=execute(["git", "rev-parse", "HEAD"]), production_equivalence="empty git diff against release for src/include/canonical manifest",
                    build_command=build, binary_sha256=sha(exe), c_sha256=sha(HERE / "benchmark.c"), runner_sha256=sha(Path(__file__)),
                    seeds=SEEDS, repetitions=3, warmups=1, fresh_process=True,
                    timeout_seconds=240, memory_guard="(96 MiB + 512*N) <= available memory / 2",
                    memory_metric="PeakWorkingSetSize / PeakPagefileUsage (Windows) or ru_maxrss*1024 (Linux); Linux private commit unavailable, encoded 0",
                    started_utc=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                    github_run_id=env.get("GITHUB_RUN_ID"), github_run_attempt=env.get("GITHUB_RUN_ATTEMPT"))
    if windows:
        metadata["glib_dll_sha256"] = sha(prefix / "bin/libglib-2.0-0.dll")
    else:
        metadata["ldd"] = execute(["ldd", str(exe)], env=env)
        libs = list((prefix / "lib").glob("libglib-2.0.so.*"))
        metadata["glib_library_sha256"] = {lib.name: sha(lib) for lib in libs if not lib.is_symlink()}
    (out / "environment.json").write_text(json.dumps(metadata, indent=2) + "\n")
    errors, skipped, count = [], [], 0
    with (out / "raw.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, ["phase", "repetition", "seed", "position", "available_memory_bytes", *FIELDS])
        writer.writeheader()
        for n in ([1000] if args.smoke else [1000, 10000, 100000, 1000000]):
            for scenario in (LEGACY if args.include_legacy else []) + NEW:
                for seed in (SEEDS if scenario.startswith("mix_") or scenario == "managed_lifecycle" else SEEDS[:1]):
                    for rep in range(4):
                        pair = []
                        for position, impl in enumerate(("full", "glib") if rep % 2 == 0 else ("glib", "full")):
                            available = available_memory()
                            if 96 * 1024**2 + 512 * n > available // 2:
                                skipped.append(dict(scenario=scenario, n=n, seed=seed, repetition=rep, implementation=impl, available=available))
                                continue
                            try:
                                run = subprocess.run([str(exe), impl, scenario, str(n), seed], env=env, text=True, capture_output=True, timeout=240)
                                if run.returncode:
                                    raise RuntimeError(f"exit {run.returncode}: {run.stderr}")
                                cells = next(csv.reader(run.stdout.splitlines()))
                                if len(cells) != len(FIELDS) or cells[FIELDS.index("correctness")] != "OK":
                                    raise RuntimeError("invalid output: " + run.stdout)
                                row = dict(zip(FIELDS, cells))
                                row.update(phase="warmup" if rep == 0 else "measured", repetition=rep, seed=seed,
                                           position=position, available_memory_bytes=available)
                                writer.writerow(row)
                                f.flush()
                                pair.append(row)
                                count += 1
                            except (subprocess.TimeoutExpired, RuntimeError) as e:
                                errors.append(dict(scenario=scenario, n=n, seed=seed, repetition=rep, implementation=impl, error=str(e)))
                        if len(pair) == 2 and any(pair[0][k] != pair[1][k] for k in ("trace_hash", "result_hash", "final_count")):
                            errors.append(dict(scenario=scenario, n=n, seed=seed, repetition=rep, error="pair mismatch"))
                print(n, scenario, "complete", flush=True)
    (out / "exceptions.json").write_text(json.dumps(dict(errors=errors, skipped=skipped), indent=2) + "\n")
    print(json.dumps(dict(samples=count, errors=len(errors), skipped=len(skipped))), flush=True)
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
