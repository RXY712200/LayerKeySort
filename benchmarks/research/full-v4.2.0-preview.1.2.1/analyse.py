"""Reconcile and summarize a capture directory, retaining decision cells."""
import argparse
import csv
from collections import defaultdict
import gzip
import hashlib
import json
from pathlib import Path
from statistics import median, quantiles

parser = argparse.ArgumentParser()
parser.add_argument('capture', type=Path)
args = parser.parse_args()
root = args.capture
metadata = json.loads((root/'metadata.json').read_text(encoding='utf-8'))
raw_bytes = (root/'raw.csv').read_bytes()
receipt_file = root/'invocations.jsonl'
if receipt_file.exists():
    receipt_text = receipt_file.read_text(encoding='utf-8')
else:
    receipt_text = gzip.decompress((root/'invocations.jsonl.gz').read_bytes()).decode()
calls = [json.loads(line) for line in receipt_text.splitlines()]
by_tag = {call['tag']: call for call in calls if call['tag'].startswith('replay-')}
assert len(by_tag)==540
assert all(x['status']=='ok' and x['code']==0 and x['available']>=metadata['guard_bytes'] for x in calls)
fields = ('seconds','live','peak','allocs','frees','cleanup','rss','peak_rss','assignments',
          'max_assignments','splits','merges','redistributions','cross_moves','total_frees')
groups = defaultdict(list)
for row in csv.DictReader(raw_bytes.decode().splitlines()):
    tag = f"replay-{row['trace']}-{row['repeat']}-{row['variant']}"
    assert by_tag.pop(tag)['stdout'].strip().split(',')==[row[f] for f in fields]
    assert row['cleanup']=='0'
    groups[row['variant'],row['trace']].append(row)
assert not by_tag and len(groups)==60
cells = {}
for key, rows in groups.items():
    assert sorted(int(row['repeat']) for row in rows)==list(range(9))
    formal = [float(row['seconds']) for row in rows if row['mode']=='prod' and int(row['repeat'])>0]
    diagnostic, = [row for row in rows if row['mode']=='diag']
    assert len(formal)==7
    q1, _, q3 = quantiles(formal,n=4,method='inclusive')
    cells[key] = dict(scenario=diagnostic['scenario'], median=median(formal), low=min(formal),
                      high=max(formal), iqr=q3-q1, live=int(diagnostic['live']), peak=int(diagnostic['peak']))
for key, cell in cells.items():
    control = cells['base',key[1]]
    cell['ratio'] = cell['median']/control['median']
    cell['live_ratio'] = cell['live']/control['live']
    cell['peak_ratio'] = cell['peak']/control['peak']
report = dict(raw_sha256=hashlib.sha256(raw_bytes).hexdigest(), calls=len(calls), rows=540, variants={})
for variant in ('transfer','b32','b128'):
    result = {key[1]: cell for key,cell in cells.items() if key[0]==variant}
    summary = dict(overall=median(cell['ratio'] for cell in result.values()),
                   maximum_live=max(cell['live_ratio'] for cell in result.values()),
                   maximum_peak=max(cell['peak_ratio'] for cell in result.values()),
                   worst=max(result,key=lambda key:result[key]['ratio']),
                   wins=sum(cell['ratio']<1 for cell in result.values()))
    for scenario in ('compare-heavy','update-heavy','long','hotspot','short'):
        summary[scenario] = median(cell['ratio'] for cell in result.values() if cell['scenario']==scenario)
    report['variants'][variant]=summary
with (root/'cells.csv').open('w',newline='',encoding='utf-8') as stream:
    writer = csv.DictWriter(stream,fieldnames=['variant','trace',*next(iter(cells.values()))],lineterminator='\n')
    writer.writeheader()
    for (variant,trace), cell in sorted(cells.items()):
        writer.writerow(dict(variant=variant,trace=trace,**cell))
(root/'summary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(report,indent=2))
