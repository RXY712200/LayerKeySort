"""Verify and summarize archived seven-control matrices without rerunning them."""
import csv
import hashlib
import json
import statistics
from collections import defaultdict
from pathlib import Path

root = Path(__file__).resolve().parent / "final-matrix"
variants = ('stable40', 'stable41', 'preview1', 'full', 'list', 'tree', 'tag', 'list_range')
summary = {}
for host in ('Linux', 'Windows'):
    folder = root / host
    with (folder / 'raw.csv').open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    assert len(rows) == 3240
    groups = defaultdict(list)
    for row in rows:
        key = tuple(row[x] for x in ('n', 'seed', 'scenario', 'implementation'))
        assert row['implementation'] in variants
        groups[key].append(row)
    assert len(groups) == 360
    cells = {}
    for key, group in groups.items():
        formal = [float(r['seconds']) for r in group if r['mode'] == 'prod' and int(r['repeat'])]
        diag, = [r for r in group if r['mode'] == 'diag']
        assert len(formal) == 7 and len(group) == 9 and int(diag['requested_after_destroy']) == 0
        cells[key] = dict(median=statistics.median(formal), minimum=min(formal), maximum=max(formal),
                          iqr=statistics.quantiles(formal, n=4, method='inclusive')[2] -
                          statistics.quantiles(formal, n=4, method='inclusive')[0],
                          requested=int(diag['requested_live']), peak_requested=int(diag['requested_peak']),
                          working_set=diag['working_set'], peak_working_set=diag['peak_working_set'])
    ratios = {}
    for kind in variants:
        if kind == 'full':
            continue
        ratios[kind] = [(key, cell['median']/cells[key[:3]+(kind,)]['median'])
                        for key, cell in cells.items() if key[-1] == 'full']
    summary[host] = dict(rows=len(rows), sha256=hashlib.sha256((folder/'raw.csv').read_bytes()).hexdigest(),
                         comparisons={kind:dict(median=statistics.median(v for _,v in values),
                             compare_heavy=statistics.median(v for k,v in values if k[2]=='compare-heavy'),
                             worst=max(v for _,v in values), candidate_faster=sum(v<1 for _,v in values))
                             for kind,values in ratios.items()})
    with (folder/'cells.csv').open('w', newline='') as stream:
        names = ['n','seed','scenario','implementation',*next(iter(cells.values())),'candidate_ratio']
        w = csv.writer(stream,lineterminator='\n')
        w.writerow(names)
        for key,cell in sorted(cells.items()):
            w.writerow([*key,*cell.values(),cells[key[:3]+('full',)]['median']/cell['median']])
(root/'summary.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(summary,indent=2))
