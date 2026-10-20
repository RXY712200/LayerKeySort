"""Fresh-process measurements, raw evidence, and SHA-256 manifest.
Run after a Release build. Instrumented output is separate from timings.
"""
import argparse
import csv
import hashlib
import io
import json
import platform
import statistics
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("build", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    metadata = {"provenance": "Codex generated engineering evaluation; not human adoption",
                "platform": platform.platform(), "python": sys.version,
                "base_main": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
                "compiler": subprocess.check_output(["gcc", "--version"], text=True).splitlines()[0],
                "warmups": "one complete discarded run per size", "repeats": 5,
                "source_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in Path(__file__).parent.glob("*.c")}}
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2))
    timed = []
    for n in (64, 10000, 100000):
        for repeat in range(6):
            result = subprocess.run([str(args.build.resolve() / "full_measure.exe"), str(n), str(args.output.resolve() / "document.bin")], capture_output=True, text=True, check=True)
            label = "warmup" if repeat == 0 else str(repeat)
            (args.output / f"timed-{n}-{label}.csv").write_text(result.stdout)
            if repeat:
                for row in csv.DictReader(io.StringIO(result.stdout)):
                    timed.append({"repeat": repeat, **row})
        allocation = args.build.resolve() / "full_alloc.exe"
        if allocation.exists():
            result = subprocess.run([str(allocation), str(n), str(args.output.resolve() / "document.bin")], capture_output=True, text=True, check=True)
            (args.output / f"alloc-{n}.csv").write_text(result.stdout)
        print(f"collected n={n}, 1 warmup + 5 timed + separate allocation run", flush=True)
    with (args.output / "timings.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=timed[0]); writer.writeheader(); writer.writerows(timed)
    summary = []
    for n in (64, 10000, 100000):
        for phase in dict.fromkeys(r["phase"] for r in timed):
            rows = [r for r in timed if int(r["residents"]) == n and r["phase"] == phase]
            if not rows:
                continue
            vals = [float(r["operation_bracket_seconds"]) * 1000 for r in rows]
            summary.append({"n": n, "phase": phase, "repeats": len(rows),
                            "operations": rows[0]["operations"],
                            "operation_total_ms_min": min(vals), "operation_total_ms_median": statistics.median(vals),
                            "operation_total_ms_max": max(vals),
                            "largest_observed_bracket_us": max(float(r["max_bracket_seconds"]) * 1e6 for r in rows)})
    with (args.output / "summary.csv").open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=summary[0]); writer.writeheader(); writer.writerows(summary)
    manifest = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(args.output.iterdir()) if p.is_file()}
    (args.output / "SHA256.json").write_text(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
