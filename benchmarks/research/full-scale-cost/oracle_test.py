"""Python list semantics versus the C oracle, without linking Full."""
import random
import subprocess
import sys

rng = random.Random(712200)
ids, commands, expected = [], [], []
for _ in range(20000):
    free = sorted(set(range(256)) - set(ids))
    if free and (not ids or rng.randrange(3) == 0):
        i = rng.choice(free); before = rng.choice(ids + [4294967295])
        ids.insert(len(ids) if before == 4294967295 else ids.index(before), i)
        commands.append(f"i {i} {before}")
    elif rng.randrange(3) == 0:
        i = rng.choice(ids); ids.remove(i); commands.append(f"r {i} 0")
    else:
        i, before = rng.choice(ids), rng.choice(ids + [4294967295])
        if i != before:
            ids.remove(i); ids.insert(len(ids) if before == 4294967295 else ids.index(before), i)
        commands.append(f"m {i} {before}")
    expected.append([len(ids)] + ids.copy())
r = subprocess.run([sys.argv[1]], input="\n".join(commands)+"\n", capture_output=True, text=True, check=True)
actual = [list(map(int, line.split())) for line in r.stdout.splitlines()]
assert actual == expected
# Ensure the checker itself rejects a corrupted identity/sequence.
bad = [row.copy() for row in actual]; bad[-1] = [999]
assert bad != expected
print("20,000 C-oracle transitions match independent Python lists; corrupted observation rejected")
