#!/usr/bin/env python3
"""Execute frozen byte vectors through public API on the selected architecture."""
import json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
c=json.loads((ROOT/'tests/fixtures/v4_wire_golden.json').read_text())
for key in c['ls1']:subprocess.run([sys.argv[1],'key',key],check=True)
for wire in c['wire']:
    actual=subprocess.check_output([sys.argv[1],'wire',wire],text=True).strip()
    if actual!=wire:raise SystemExit('Frozen wire mismatch')
subprocess.run([sys.argv[1],'migration',*reversed(c['v3_lk1'])],check=True)
for key in json.loads((ROOT/'tests/fixtures/v3_lk1_golden.json').read_text()):
    subprocess.run([sys.argv[1],'legacy',key],check=True)
print('Frozen LS1/wire/LK1 corpus PASS; canonical bytes match architecture-independent fixtures')
