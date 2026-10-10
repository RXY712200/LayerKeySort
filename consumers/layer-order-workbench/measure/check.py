import csv,io,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from collect import CASES,SIZES,quantiles,canonical_digest
exe=sys.argv[1]
assert quantiles([1,2,3,4,5])==dict(minimum=1,median=3,maximum=5,mad=1,iqr=2)
assert canonical_digest(b'a\r\nb\r\n')==canonical_digest(b'a\nb\n')
assert canonical_digest(b'a\rb')!=canonical_digest(b'a\nb')
for size in SIZES:
    for case in CASES:
        for mode in ['isolated','pipeline']:
            result=subprocess.run([exe,case,str(size),mode],check=True,capture_output=True,text=True)
            rows=list(csv.DictReader(io.StringIO(result.stdout)))
            assert len(rows)==2 and [r['phase'] for r in rows]==['warmup','measured']
            for row in rows:
                count=size if case in ['sequential-insert','head-tail-insert'] else 72 if case=='mixed-editor' else 64
                assert int(row['commands'])==count
                expected=count*(size+1) if case in ['forward-traverse','reverse-traverse'] else count
                if case!='mixed-editor':assert int(row['api_calls'])==expected
                assert int(row['final'])==size
                assert int(row['api_ns'])+int(row['map_ns'])+int(row['oracle_ns'])+int(row['parse_trace_output_ns'])<=int(row['execute_ns'])
                assert int(row['resolution_ns'])>0
                if mode=='isolated':assert row['oracle_ns']==row['parse_trace_output_ns']=='0'
bad=subprocess.run([exe,'unknown','16','isolated'],capture_output=True)
assert bad.returncode!=0
print('All workload timing schemas/call counts/bounds and statistics PASS')
if sys.platform.startswith('linux'):
    # Reproduce the pre-exec high-water mark trap using a large live launcher.
    launcher_storage=bytearray(64*1024*1024)
    result=subprocess.run([sys.argv[2],'16'],check=True,capture_output=True,text=True)
    memory=list(csv.DictReader(io.StringIO(result.stdout)))
    assert int(memory[0]['peak_rss_bytes'])<len(launcher_storage), 'Child peak includes pre-exec launcher memory'
    print('Linux current-address-space peak ignores 64 MiB launcher: PASS')
