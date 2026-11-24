"""V0.2 post-audit trace correction; not a preregistration of V0.1.

PCG32 Python adaptation of pcg-c-basic at bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3.
Reference copyright 2014 Melissa O'Neill; Apache-2.0, see third_party/PCG-LICENSE.txt.
Changed language, explicit masks and SHA-256 domain-separated initialization.
The sequence model/workload design is research code, not LayerKeySort production.
"""
import hashlib
from pathlib import Path

SEEDS = (17, 305419896, 2654435769)
HOLDOUT = 3512641005
SIZES = (1000, 10000, 100000)
SCENARIOS = ("compare-heavy", "update-heavy", "short", "long", "hotspot")
VERSION = "0.3.0"
RNG_REFERENCE = "bc39cd76ac3d541e618606bcc6e1e5ba5e5e6aa3"


class PCG32:
    """XSH-RR 64/32, reference two-step seeding; integer rejection sampling."""
    def __init__(self, state, sequence):
        self.state = 0
        self.inc = ((sequence << 1) | 1) & ((1 << 64) - 1)
        self.next32()
        self.state = (self.state + state) & ((1 << 64) - 1)
        self.next32()

    def next32(self):
        old = self.state
        self.state = (old * 6364136223846793005 + self.inc) & ((1 << 64) - 1)
        value = (((old >> 18) ^ old) >> 27) & 0xffffffff
        rotation = old >> 59
        return ((value >> rotation) | (value << ((-rotation) & 31))) & 0xffffffff

    def bounded(self, bound):
        if not 0 < bound <= 1 << 32:
            raise ValueError("PCG32 bound outside 1..2^32")
        threshold = (1 << 32) % bound
        while True:
            value = self.next32()
            if value >= threshold:
                return value % bound


def stream(n, seed, family, purpose):
    if seed == HOLDOUT:
        raise ValueError("Reserved holdout must not be consumed")
    material = f"lks-d-manuscript-v03|{n}|{seed}|{family}|{purpose}".encode("ascii")
    digest = hashlib.sha256(material).digest()
    return PCG32(int.from_bytes(digest[:8], "big"), int.from_bytes(digest[8:16], "big"))


def trace(n, seed, scenario, path, steps=20000):
    if n not in SIZES or seed not in SEEDS or scenario not in SCENARIOS or steps != 20000:
        raise ValueError("Outside fixed exploratory correction matrix")
    family = "locality" if scenario in ("short", "long") else scenario
    kinds = stream(n, seed, family, "kind")
    targets = stream(n, seed, family, "target")
    anchors = stream(n, seed, family, "anchor")
    directions = stream(n, seed, family, "direction")
    local = stream(n, seed, family, "short-offset" if scenario == "short" else "long-resample")
    sequence = list(range(n))
    created, hotspot, operations = n, n // 2, []
    for _ in range(steps):
        ticket = kinds.bounded(100)
        thresholds = (90, 96, 98) if scenario == "compare-heavy" else (10, 70, 85) if scenario == "update-heavy" else (20, 80, 90)
        if scenario == "hotspot":
            kind = "C" if ticket < 20 else "I" if ticket < 60 else "R" if ticket < 80 else "M"
        else:
            kind = "C" if ticket < thresholds[0] else "M" if ticket < thresholds[1] else "I" if ticket < thresholds[2] else "R"
        length = len(sequence)
        after = directions.bounded(2)  # Exactly one direction draw per operation.
        if scenario == "hotspot" and kind in ("R", "M"):
            protected = sequence.index(hotspot)
            i = targets.bounded(length - 1)
            i += i >= protected
        else:
            i = targets.bounded(length)
        j = anchors.bounded(length)  # One base anchor draw, even when overridden.
        if scenario == "hotspot" and kind in ("I", "M"):
            j = sequence.index(hotspot)
        if kind == "M":
            def useful(k):
                return k != i and i != k + after - (i < k + after)
            if scenario == "short":
                candidates = [k for k in range(max(0, i-8), min(length, i+9)) if useful(k)]
                j = candidates[local.bounded(len(candidates))]
            else:
                while not useful(j):
                    j = local.bounded(length) if scenario != "hotspot" else j
                    if scenario == "hotspot" and not useful(j):
                        # Keep the hotspot anchor fixed; resample the source instead.
                        protected = sequence.index(hotspot)
                        i = local.bounded(length-1); i += i >= protected
        a, b, expected = sequence[i], sequence[j], 0
        if kind == "C":
            expected = (i > j) - (i < j)
        elif kind == "I":
            a = created; created += 1; sequence.insert(j+after, a)
        elif kind == "R":
            sequence.pop(i)
        else:
            sequence.pop(i); sequence.insert(sequence.index(b)+after, a)
        operations.append((kind, a, b, after, expected))
    text = f"{n} {steps} {len(sequence)}\n"
    text += "".join(f"{k} {a} {b} {d} {e}\n" for k, a, b, d, e in operations)
    text += " ".join(map(str, sequence)) + "\n"
    # Explicit LF bytes: reproduction does not depend on host text translation.
    Path(path).write_bytes(text.encode("ascii"))
    return {"n": n, "seed": seed, "scenario": scenario, "operations": steps,
            "sha256": hashlib.sha256(text.encode("ascii")).hexdigest(), "final_n": len(sequence)}
