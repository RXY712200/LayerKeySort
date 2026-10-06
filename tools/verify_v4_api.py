#!/usr/bin/env python3
"""Verify the frozen V4 primary header; comments/release version text may change."""
import json,re,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def contract():
    s=(ROOT/'include/layerkeysort.h').read_text(encoding='utf-8-sig')
    s=re.sub(r'/\*.*?\*/','',s,flags=re.S)
    s=re.sub(r'(#define LKS_VERSION_(?:PRERELEASE|STRING))[^\n]*',r'\1 RELEASE_TEXT',s)
    functions=sorted(set(re.findall(r'\b(lks_\w+)\s*\(',s)))
    types=sorted(set(re.findall(r'\b(Lks\w+)\b',s)))
    statuses={n:int(v) for n,v in re.findall(r'(LKS_STATUS_\w+)\s*=\s*(\d+)',s)}
    macros=re.findall(r'^#define (LKS_VERSION_\w+)',s,re.M)
    return {'types':types,'statuses':statuses,'functions':functions,'version_macros':macros,
            'canonical_header_tokens':re.findall(r'[A-Za-z_]\w*|[0-9]+|[^\s]',s)}
def main():
    hashes=json.loads((ROOT/'tests/fixtures/v4_fixture_sha256.json').read_text())
    for name,digest in hashes.items():
        content=(ROOT/'tests/fixtures'/name).read_text(encoding='utf-8').encode('utf-8')
        if hashlib.sha256(content).hexdigest()!=digest:raise SystemExit('Frozen fixture changed: '+name)
    actual=contract();expected=json.loads((ROOT/'tests/fixtures/v4_public_api.json').read_text())
    if actual!=expected:raise SystemExit('Frozen V4 API mismatch: explicit freeze review required')
    fixture=(ROOT/'tests/v4_api_compile.c').read_text()
    if sorted(set(re.findall(r'ref_(lks_\w+)',fixture)))!=actual['functions']:
        raise SystemExit('Typed function fixture differs from manifest')
    for path in ['tests/distribution/consumer/api_compile.cpp','tests/distribution/amalgamation/api_compile.cpp']:
        if (ROOT/path).read_text()!=fixture:raise SystemExit('Typed C++ fixture drift: '+path)
    print('Frozen V4 manifest PASS:',len(actual['functions']),'functions')
if __name__=='__main__':main()
