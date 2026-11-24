"""Independent trace-design gate. No import/call of the RNG or trace generator.

Reads serialized operations, reconstructs a flat sequence, checks intended
mixtures/orientations/locality and pair schedules. Hash reproduction is separate.
"""
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import statistics


class DesignError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise DesignError(message)


def examine(path, scenario, *, enforce_design=True):
    tokens = Path(path).read_text(encoding="ascii").split()
    require(len(tokens) >= 3, "truncated header")
    n, steps, final_n = map(int, tokens[:3])
    require(n in (1000, 10000, 100000) and steps == 20000, "wrong bounds")
    require(len(tokens) == 3 + steps*5 + final_n, "trailing or truncated input")
    require(0 <= final_n <= n+steps, "invalid final count")
    sequence, ever = list(range(n)), set(range(n))
    counts, orientations = Counter(), {k: Counter() for k in ("I", "M")}
    schedule, directions, distances, move_sources, kinds_by_direction = [], [], [], [], Counter()
    controls=[]
    next_id, same, noops = n, 0, 0
    for step in range(steps):
        at = 3+step*5
        kind = tokens[at]; a, b, after, expected = map(int, tokens[at+1:at+5])
        require(kind in ("C", "I", "M", "R") and after in (0, 1), "bad operation/direction")
        require(b in sequence, "anchor not live")
        require(expected in (-1, 0, 1), "bad compare expectation")
        j = sequence.index(b)
        # Locality pairs share source-rank/count controls and all nonmove
        # anchor ranks. Actual occurrence IDs/state may diverge after moves.
        controls.append([len(sequence),None if kind=="I" else sequence.index(a) if a in sequence else -1,None if kind=="M" else j])
        if kind == "I":
            require(expected == 0, "noncompare expectation")
            require(a == next_id and a not in ever, "occurrence ID reused/not sequential")
            next_id += 1; ever.add(a)
            sequence.insert(j+after, a)
        else:
            require(a in sequence, "target not live")
            i = sequence.index(a)
            if kind == "C":
                require(expected == (i > j)-(i < j), "wrong comparison oracle")
                same += i == j
            elif kind == "R":
                require(expected == 0, "noncompare expectation")
                if scenario == "hotspot": require(a != n//2, "hotspot removed")
                sequence.pop(i)
            else:
                require(expected == 0, "noncompare expectation")
                displacement = j + after - (i < j+after)
                noop = a == b or displacement == i
                noops += noop
                distances.append(abs(i-j)); move_sources.append(i/len(sequence))
                if enforce_design:
                    require(not noop, "unintended move no-op")
                    if scenario == "short": require(1 <= abs(i-j) <= 8, "short locality violated")
                if a != b:
                    sequence.pop(i); sequence.insert(sequence.index(b)+after, a)
        if kind in ("I", "M"):
            orientations[kind][after] += 1; kinds_by_direction[after] += 1
            if scenario == "hotspot": require(b == n//2, "hotspot anchor changed")
        counts[kind] += 1; schedule.append(kind); directions.append(after)
    expected_final = list(map(int, tokens[3+steps*5:]))
    require(sequence == expected_final and len(sequence) == final_n, "final sequence differs")
    if enforce_design:
        weights = {"compare-heavy": {"C":90,"M":6,"I":2,"R":2},
                   "update-heavy": {"C":10,"M":60,"I":15,"R":15},
                   "short": {"C":20,"M":60,"I":10,"R":10},
                   "long": {"C":20,"M":60,"I":10,"R":10},
                   "hotspot": {"C":20,"M":20,"I":40,"R":20}}[scenario]
        for kind, percent in weights.items():
            require(counts[kind] > 0, f"missing intended category {kind}")
            mean = steps*percent/100
            tolerance = max(12, 6*math.sqrt(mean*(1-percent/100)))
            require(abs(counts[kind]-mean) <= tolerance, f"category imbalance {kind}")
        for kind, counter in orientations.items():
            total = sum(counter.values())
            require(counter[0] > 0 and counter[1] > 0, f"constant direction {kind}")
            require(.2 <= counter[1]/total <= .8, f"orientation imbalance {kind}")
        require(.35 <= kinds_by_direction[1]/sum(kinds_by_direction.values()) <= .65, "overall orientation imbalance")
        if scenario == "long": require(statistics.median(distances) >= n/8, "long locality insufficient")
    report = {"n": n, "operations": steps, "final_n": final_n,
              "counts": dict(sorted(counts.items())),
              "orientation": {k: {"before":v[0], "after":v[1]} for k,v in orientations.items()},
              "move_distance": {"min":min(distances), "median":statistics.median(distances), "max":max(distances), "mean":statistics.mean(distances)},
              "move_source_mean_fraction": statistics.mean(move_sources),
              "move_noops":noops, "same_occurrence_comparisons":same,
              "pair_control_sha256":hashlib.sha256(json.dumps(controls,separators=(",",":")).encode("ascii")).hexdigest()}
    return report, schedule, directions, controls


def check_pair(short, long):
    require(short[0]["n"] == long[0]["n"], "pair size differs")
    require(short[1] == long[1], "pair category schedule differs")
    require(short[2] == long[2], "pair direction schedule differs")
    require(short[3] == long[3], "pair source/count/nonmove-anchor controls differ")
    require(short[0]["final_n"] == long[0]["final_n"], "pair final counts differ")
    return {"n":short[0]["n"], "category_schedule_identical":True,
            "direction_schedule_identical":True,
            "source_count_nonmove_anchor_controls_identical":True,
            "short_median_distance":short[0]["move_distance"]["median"],
            "long_median_distance":long[0]["move_distance"]["median"]}
