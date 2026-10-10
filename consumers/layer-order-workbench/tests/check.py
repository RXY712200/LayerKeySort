"""Standard-library test driver; scripts/generated fixtures are not user traces."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def require(condition, message):
    if not condition:
        raise AssertionError(message)

def run(exe, args, data=None, success=True):
    result = subprocess.run([str(exe), *map(str, args)], input=data, capture_output=True, timeout=180)
    require((result.returncode == 0) == success,
            f"exit={result.returncode}; stderr={result.stderr.decode(errors='replace')}; stdout-tail={result.stdout[-1000:]!r}")
    return result.stdout.decode(), result.stderr.decode()

def record_script(exe, script, directory, source="script"):
    trace = directory / (script.stem + '.trace')
    stdout, _ = run(exe, ['--commands', script, '--trace', trace, '--source', source])
    replay, _ = run(exe, ['--replay', trace, '--source', 'script'])
    require(stdout == replay, 'Replay changed the complete semantic output')
    lines = trace.read_text().splitlines()
    require(lines[0] == f'LLOW\t1\t{source}\tfile', 'Wrong provenance')
    require(lines[-1] == f'LLOW-END\t{len(lines) - 2}', 'Missing/wrong footer')
    return trace, stdout, [line.split('\t') for line in lines[1:-1]]

def scenes(exe, directory):
    expected = {
        'layer-composition': [1, 7, 6, 4, 5],
        'overlay-reordering': [8, 7, 4, 3, 2, 1],
        'placeholder-recovery': [5, 4, 7, 1, 6],
    }
    evidence = []
    for script in sorted((ROOT / 'scenarios').glob('*.commands')):
        trace, _, rows = record_script(exe, script, directory)
        require(len(rows) > 20, 'Scene is too short')
        actual = [int(x) for x in rows[-1][-1].split(',')]
        require(actual == expected[script.stem], f'{script.stem}: expected golden {expected[script.stem]}, actual {actual}')
        committed = ROOT/'scenarios/evidence'/(script.stem+'.trace')
        require(committed.read_text() == trace.read_text(), 'Committed semantic evidence changed')
        saved_stdout, _ = run(exe, ['--replay', committed])
        require(saved_stdout == run(exe, ['--replay', trace])[0], 'Saved trace replay changed output')
        evidence.append({'scenario':script.name, 'commands':len(rows), 'source':'authored script executed by Codex/CTest', 'trace':str(trace), 'final_sequence':actual})
    return evidence

def errors(exe, directory):
    script = directory / 'invalid.commands'
    data = (b'create Safe\ninsert-back 1\n'
            b'insert-back 999\nremove 999\nremove 1\nremove 1\n'
            b'move-before 1 1\ncompare 1 1\nremove 0\nremove -1\n'
            b'remove 18446744073709551616\ninsert-after NULL\n'
            b'create bad/name\nsize extra\nunknown\n' + b'x' * 1000 +
            b'\nsize\nsi\x00ze\nhelp\ninsert-back NULL\nlist\nquit\n')
    script.write_bytes(data)
    trace, stdout, rows = record_script(exe, script, directory)
    statuses = [r[7] for r in rows]
    require(statuses == ['OK','OK','APP_UNKNOWN_OBJECT','APP_UNKNOWN_OCCURRENCE','OK',
                         'APP_RETIRED_OCCURRENCE','APP_RETIRED_OCCURRENCE','APP_RETIRED_OCCURRENCE',
                         'APP_ID_FORMAT','APP_ID_FORMAT','APP_ID_FORMAT','APP_ARGUMENT_COUNT',
                         'APP_NAME_FORMAT','APP_ARGUMENT_COUNT','APP_UNKNOWN_COMMAND','APP_LINE_LIMIT',
                         'OK','APP_INPUT_FORMAT','OK','OK','OK','OK'], 'Error classification/precedence regression')
    require(rows[-1][-1] == '2' and 'occ=2 object=NULL' in stdout, 'Drain/recovery/reinsertion failed')
    run(exe, ['--commands', script, '--trace', trace], success=False)
    run(exe, ['--commands', script, '--trace', script], success=False)
    require(script.read_bytes() == data, 'Input was clobbered')
    run(exe, ['--commands', directory/'missing'], success=False)
    run(exe, ['--commands'], success=False)
    run(exe, ['--source','human-terminal'], data=b'quit\n', success=False)
    # Startup I/O and bounded EOF without newline are ordinary consumer paths.
    run(exe, ['--source','script'], data=b'create End\ninsert-back NULL\nsize')
    cap = directory/'objects.commands'
    cap.write_text('\n'.join(['create X']*256+['create Overflow','insert-back 256','list','quit'])+'\n')
    _, cap_output, cap_rows = record_script(exe,cap,directory)
    require(cap_rows[256][7]=='APP_OBJECT_LIMIT' and 'object=256 name=X' in cap_output,
            'Object capacity/ownership boundary failed')
    boundary = directory/'line-boundary.commands'
    boundary.write_bytes(b'create Boundary\r\n'+b'size'+b' '*251+b'\r\n'+
                         b'size'+b' '*252+b'\r\nsize\r\r\nquit\r\n')
    _, _, boundary_rows = record_script(exe,boundary,directory)
    require([r[7] for r in boundary_rows]==['OK','OK','APP_LINE_LIMIT','OK','OK'],
            'CRLF normalized 255/256-byte boundary incorrect')
    require(bytes.fromhex(boundary_rows[3][10])==b'size\r',
            'Reader removed more than one line-ending CR')
    return {'commands':len(rows), 'errors_checked':sum(s != 'OK' for s in statuses), 'trace':str(trace)}

def replay_errors(exe, directory):
    interactive = ROOT/'scenarios/evidence/codex-terminal.trace'
    output, _ = run(exe, ['--replay',interactive])
    require('VERIFIED commands=23 application_or_library_errors=1 live=4 issued=5 objects=2' in output,
            'Recorded automated terminal session failed replay')
    script = ROOT/'scenarios/layer-composition.commands'
    trace, _, _ = record_script(exe, script, directory)
    text = trace.read_text()
    variants = [text.replace('LLOW\t1', 'LLOW\t2', 1),
                '\n'.join(text.splitlines()[:-1]) + '\n',
                text.replace('\tOK\t', '\tNOT_OK\t', 1),
                text.replace('LLOW-END\t32', 'LLOW-END\t99'),
                text + 'garbage\n']
    lines = text.splitlines()
    altered = lines[1].split('\t'); altered[10] = 'f'
    variants.append('\n'.join([lines[0], '\t'.join(altered), *lines[2:]]) + '\n')
    altered = lines[-2].split('\t'); altered[-1] = '99'
    variants.append('\n'.join([*lines[:-2], '\t'.join(altered), lines[-1]]) + '\n')
    for i, content in enumerate(variants):
        bad = directory/f'bad-{i}.trace'; bad.write_text(content)
        run(exe, ['--replay', bad], success=False)
    replay_trace = directory/'replay-recording.trace'
    run(exe, ['--replay',trace,'--trace',replay_trace,'--source','script'])
    run(exe, ['--replay',replay_trace])
    require(replay_trace.read_text().splitlines()[0]=='LLOW\t1\tscript\treplay', 'Replay execution mode missing')
    return {'rejected_corrupt_traces':len(variants),'codex_terminal_replay_commands':23}

def sizes(exe, directory):
    evidence = []
    for size in (16,128,2048):
        lines = ['create Shared'] + ['insert-back NULL' if i % 5 == 0 else 'insert-back 1' for i in range(size)]
        lines += ['size','list-reverse',f'move-front {size}',f'move-back {size}',
                  f'move-before 1 {size}',f'move-after 1 {size}', 'move-front 2',
                  'move-front 2',f'move-back {size}',f'move-back {size}',
                  'move-before 2 2','move-after 2 2',f'compare 2 {size}']
        if size == 2048:
            lines += ['insert-back 1']
        # Distinct reproducible application-semantic reorders, not measured user frequencies.
        for step in range(128):
            a=(step*13+3)%size+1; b=(step*31+7)%size+1
            lines += [f'move-before {a} {b}' if step%2 else f'move-after {a} {b}']
        lines += ['remove 3', 'move-front 3', 'insert-back 1', 'compare 2 2', 'size', 'quit']
        script=directory/f'generated-{size}.commands'; script.write_text('\n'.join(lines)+'\n')
        trace, stdout, rows=record_script(exe,script,directory,'generated')
        require(int(rows[-1][8])==size and '3' not in rows[-1][-1].split(','), 'Size/retired ID failed')
        require(str(size+1) in rows[-1][-1].split(','), 'Fresh occurrence ID not present')
        require(any(r[7]=='APP_RETIRED_OCCURRENCE' for r in rows), 'Retirement path not tested')
        if size==2048:
            require(any(r[7]=='APP_OCCURRENCE_LIMIT' for r in rows), 'Live capacity failure missing')
        require(f'live={size}' in stdout, 'Wrong final active size')
        evidence.append({'active_size':size,'commands':len(rows),'source':'generated engineering fixture','trace':str(trace)})
    return evidence

if __name__ == '__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('--exe',type=Path,required=True)
    parser.add_argument('--case',choices=['scenes','errors','replay','sizes'],required=True)
    parser.add_argument('--output',type=Path,required=True); args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    directory=Path(tempfile.mkdtemp(prefix='run-',dir=args.output))
    result={'scenes':scenes,'errors':errors,'replay':replay_errors,'sizes':sizes}[args.case](args.exe,directory)
    (args.output/'latest-summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'case':args.case,'result':result}))
