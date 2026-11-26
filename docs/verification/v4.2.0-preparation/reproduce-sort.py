"""Rerun current bilingual Usage C snippet and independent stability checks.

Run from the repository root: python -X utf8
docs/verification/v4.2.0-preparation/reproduce-sort.py
Uses the canonical production manifest and GCC; generated programs stay in build/.
Receipts are fresh executions, never reconstructions of the earlier missing logs.
"""
from pathlib import Path
import datetime, gzip, hashlib, json, os, platform, re, shutil, subprocess

root = Path.cwd()
evidence = root / 'docs/verification/v4.2.0-preparation'
build = root / 'build/stable-sort-repair'
build.mkdir(parents=True, exist_ok=True)
compiler = shutil.which('gcc')
if compiler is None:
    raise SystemExit('GCC must be on PATH')

def digest(data):
    return hashlib.sha256(data).hexdigest()

def git(*args):
    return subprocess.check_output(['git', *args], cwd=root).decode().strip()

sources = re.findall(r'^\s+(src/[^\s]+\.c)\s*$', (root / 'cmake/ProductionSources.cmake').read_text(), re.M)
assert len(sources) == 16
code = []
for lang in ['', '.zh-CN']:
    text = (root / f'docs/USAGE{lang}.md').read_text(encoding='utf-8')
    code.append(re.search(r'```c\n(.*?)```', text, re.S)[1])
assert code[0] == code[1], 'Both language editions must contain the exact same C snippet'
snippet = build / 'usage-sort.c'
snippet.write_text(code[0], encoding='utf-8', newline='\n')
flags = ['-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O0', '-g', '-Iinclude']
receipt = dict(execution_kind='new independent-review repair execution; not recovered historical output',
    started_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    source_predecessor=git('rev-parse', 'HEAD'), platform=platform.platform(),
    python=platform.python_version(), compiler_path=compiler,
    source_tree=git('rev-parse', 'HEAD:src'),
    header_blob=git('hash-object', 'include/layerkeysort.h'),
    canonical_manifest_blob=git('hash-object', 'cmake/ProductionSources.cmake'),
    inputs={p: digest((root / p).read_bytes()) for p in [*sources, 'include/layerkeysort.h',
        'cmake/ProductionSources.cmake', 'docs/USAGE.md', 'docs/USAGE.zh-CN.md',
        'docs/verification/v4.2.0-preparation/sort-stability-harness.c',
        'docs/verification/v4.2.0-preparation/reproduce-sort.py']},
    snippet_sha256=digest(snippet.read_bytes()), commands=[])

commands = [[compiler, '--version']]
for name, source in [('usage-sort', snippet), ('sort-stability', evidence / 'sort-stability-harness.c')]:
    exe = build / (name + ('.exe' if os.name == 'nt' else ''))
    commands += [[compiler, *flags, str(source.relative_to(root)), *sources, '-o', str(exe.relative_to(root))], [str(exe)]]
for i, command in enumerate(commands):
    result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    filename = f'repair-sort-{i:02d}.log.gz'
    (evidence / filename).write_bytes(gzip.compress(result.stdout, mtime=0))
    receipt['commands'].append(dict(command=command, cwd=str(root), exit_code=result.returncode,
        log=filename, sha256=digest(result.stdout), output_bytes=len(result.stdout),
        compression='gzip; sha256 is of original uncompressed command output'))
    (evidence / 'sort-repair-commands.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(i, 'exit', result.returncode, 'bytes', len(result.stdout), flush=True)
    if result.returncode:
        raise SystemExit(result.returncode)

verified = 0
for name in ['commands.json', 'sort-repair-commands.json']:
    data = json.loads((evidence / name).read_text(encoding='utf-8'))
    for entry in data if isinstance(data, list) else data['commands']:
        assert digest(gzip.decompress((evidence / entry['log']).read_bytes())) == entry['sha256']
        assert entry['exit_code'] == 0
        verified += 1
print(f'PASS: bilingual exact snippet, stability harness, {verified} original/new receipt hashes')
