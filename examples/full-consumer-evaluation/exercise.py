"""Independent generated command semantics. Not a human adoption/session claim."""
import argparse
import random
import subprocess
import tempfile
from pathlib import Path


def application_scene(exe):
    commands = """rename 0 Backdrop
visible 0 0
rename 1 Title
add 0 back
add 1 back
add 0 back
capture 0
move 3 front
remove 1
show
restore 0
show
quit
"""
    result = subprocess.run([exe], input=commands, text=True, capture_output=True, check=True)
    layers = [line for line in result.stdout.splitlines() if line.startswith("layer ")]
    assert layers == [
        "layer occurrence=3 object=0 name=Backdrop visible=0",
        "layer occurrence=2 object=1 name=Title visible=1",
        "layer occurrence=1 object=0 name=Backdrop visible=0",
        "layer occurrence=2 object=1 name=Title visible=1",
        "layer occurrence=3 object=0 name=Backdrop visible=0",
    ], layers
    assert result.stdout.splitlines()[-1] == "OK 3 1 2 3"
    print("document workflow: shared layer instances, names/visibility, retirement, historical restore PASS")


def large_document(exe):
    """Several thousand live residents; oracle never asks Full for expected order."""
    rng = random.Random(493)
    commands, expected, ids = [], [], []
    def emit(cmd, ok=True):
        commands.append(cmd)
        expected.append(("OK" if ok else "ERROR", ids.copy()))
    for i in range(1, 2049):
        ids.append(i); emit(f"add {i % 256} back")
    saved = ids.copy(); emit("capture 0")
    for _ in range(2048):
        a, b = rng.sample(ids, 2)
        ids.remove(a); ids.insert(ids.index(b), a); emit(f"move {a} before {b}")
    for _ in range(1024):
        a = rng.choice(ids); ids.remove(a); emit(f"remove {a}")
    ids = saved.copy(); emit("restore 0")
    emit("quit")
    result = subprocess.run([exe], input="\n".join(commands)+"\n", text=True, capture_output=True, check=True)
    rows = result.stdout.splitlines()
    assert len(rows) == len(expected)
    for i, (row, want) in enumerate(zip(rows, expected)):
        fields = row.split()
        assert (fields[0], list(map(int, fields[2:]))) == want, (i, commands[i])
        assert int(fields[1]) == len(want[1])
    print(f"large_document peak_residents=2048 commands={len(commands)} seed=493 exact_all_rows=true")


def exercise(exe, seed, steps):
    rng = random.Random(seed)
    ids, history, commands, expected = [], {}, [], []
    next_id = 1

    def emit(cmd, ok=True):
        commands.append(cmd)
        expected.append(("OK" if ok else "ERROR", ids.copy()))

    with tempfile.TemporaryDirectory(prefix="full-editor-") as tmp:
        wire = Path(tmp) / "document.bin"
        malformed = Path(tmp) / "bad.bin"
        malformed.write_bytes(b"LKS4SNP1\0")
        emit("rename 0 Background"); emit("visible 0 0")
        emit("visible 0 2", False)
        emit("capture 0"); history[0] = []
        emit(f"save 0 {wire}"); emit(f"load 1 {wire}"); history[1] = []
        emit("restore 1")
        for step in range(steps):
            op = rng.randrange(12)
            if (op < 3 or not ids) and len(ids) < 160:
                if ids and op == 2:
                    anchor = rng.choice(ids)
                    after = rng.randrange(2)
                    pos = ids.index(anchor) + after
                    ids.insert(pos, next_id)
                    emit(f"insert {rng.randrange(16)} {'after' if after else 'before'} {anchor}")
                else:
                    front = rng.randrange(2)
                    ids.insert(0 if front else len(ids), next_id)
                    emit(f"add {rng.randrange(16)} {'front' if front else 'back'}")
                next_id += 1
            elif op < 5 and ids:
                victim = rng.choice(ids); ids.remove(victim); emit(f"remove {victim}")
                emit(f"remove {victim}", False)  # Consumer retires mapping; never probes dangling handles.
            elif op < 9 and ids:
                victim, anchor = rng.choice(ids), rng.choice(ids)
                where = rng.choice(["front", "back", "before", "after"])
                if where in ("front", "back"):
                    ids.remove(victim); ids.insert(0 if where == "front" else len(ids), victim)
                    emit(f"move {victim} {where}")
                else:
                    if victim != anchor:
                        ids.remove(victim); ids.insert(ids.index(anchor) + (where == "after"), victim)
                    emit(f"move {victim} {where} {anchor}")
            elif op == 9:
                slot = rng.randrange(8); history[slot] = ids.copy(); emit(f"capture {slot}")
            elif op == 10 and history:
                slot = rng.choice(list(history)); ids = history[slot].copy(); emit(f"restore {slot}")
            else:
                emit(rng.choice(["move 999999 front", "add 256 front", "add -1 back", "garbage", "remove 18446744073709551616", "move 0 before 1", "capture 32"]), False)
            if step % 997 == 0:
                history[8] = ids.copy(); emit("capture 8"); emit(f"save 8 {wire}")
                emit(f"load 9 {wire}"); history[9] = history[8].copy()
                emit(f"load 9 {malformed}", False)
                ids = history[9].copy(); emit("restore 9")
        emit("add " + "1" * 2000, False)
        emit("quit")
        result = subprocess.run([exe], input="\n".join(commands) + "\n", text=True, capture_output=True)
        rows = [r for r in result.stdout.splitlines() if r.startswith(("OK ", "ERROR "))]
        assert result.returncode == 0, result.stderr
        assert len(rows) == len(expected), (len(rows), len(expected), result.stderr)
        for i, (row, want) in enumerate(zip(rows, expected)):
            t = row.split(); actual = (t[0], list(map(int, t[2:])))
            if actual != want or int(t[1]) != len(want[1]):
                # Only on mismatch, preserve the exact command prefix for reproduction.
                Path(f"full-failure-{seed}-{i}.txt").write_text("\n".join(commands[:i+1]) + "\n")
                raise AssertionError((seed, i, commands[i], actual, want))
        print(f"seed={seed} generated_steps={steps} processed_commands={len(commands)} all_external_rows_match=true")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("exe")
    parser.add_argument("--steps", type=int, default=12000)
    parser.add_argument("--large", action="store_true")
    parser.add_argument("--application", action="store_true")
    args = parser.parse_args()
    if args.application:
        application_scene(args.exe)
    elif args.large:
        large_document(args.exe)
    else:
        for seed in (17, 712200, 0xAB19):
            exercise(args.exe, seed, args.steps)
