"""The same finite B64/B128/B256 screen at a second scale.

Rotate capacity order between rounds; preserve the untuned 10k screen. No other
capacity or tuning strategy is introduced. Every row must pass its flat oracle.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--build", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--scale", type=int, default=100000)
args = parser.parse_args()
if args.output.exists():
    raise SystemExit("Refusing to overwrite capacity evidence")
rows = []
families = ["endpoint", "random", "local_drag", "distant_drag", "churn", "equal", "duplicates", "timeline"]
suffix = ".exe" if os.name == "nt" else ""
for repeat in range(4):
    capacities = [64, 128, 256]
    capacities = capacities[repeat % 3:] + capacities[:repeat % 3]
    for family in families:
        initial = 0 if family in ["endpoint", "random", "equal", "duplicates"] else args.scale
        for capacity in capacities:
            kind = "diagnostic" if repeat == 3 else "timed"
            target = f"lks_capacity_{capacity}_{kind}_trace"
            command = [str(args.build.resolve() / (target + suffix)), family,
                       str(initial), str(args.scale), "0", "7", str(repeat)]
            result = subprocess.run(command, check=True, text=True, capture_output=True)
            row = json.loads(result.stdout.splitlines()[-1])
            if not row["oracle"] or row["cleanup_live_bytes"] or row["cleanup_live_blocks"]:
                raise RuntimeError("Capacity oracle/cleanup failure")
            row["target"] = target
            rows.append(row)
            print(capacity, family, repeat, "PASS", flush=True)
args.output.parent.mkdir(parents=True, exist_ok=True)
with args.output.open("w", encoding="utf-8", newline="") as stream:
    writer = csv.DictWriter(stream, sorted({key for row in rows for key in row}))
    writer.writeheader()
    writer.writerows(rows)
