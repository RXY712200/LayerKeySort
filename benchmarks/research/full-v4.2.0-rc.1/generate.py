"""Derive RC live-order presentation from the immutable Preview.3 archive."""
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from statistics import median
from xml.sax.saxutils import escape

HERE = Path(__file__).resolve().parent
ARCHIVE = HERE.parent / 'full-v4.2.0-preview.3' / 'final-matrix'
CONTROLS = dict(stable40='Stable 4.0', stable41='Stable 4.1', preview1='Preview 1.2',
                list='List', tree='Treap', tag='Native tag', list_range='Native list')


def svg(path, title, body, width=760, height=340):
    content = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" '
               f'role="img"><title>{escape(title)}</title>'
               f'<rect width="100%" height="100%" fill="white"/>'
               f'<g font-family="sans-serif" font-size="13" fill="#172840">{body}</g></svg>')
    path.write_text(content + '\n', encoding='utf-8')


def label(x, y, value):
    return f'<text x="{x}" y="{y}">{escape(str(value))}</text>'


def derive():
    result = {}
    bars = label(12, 21, 'Full / control elapsed ratio (lower is faster)')
    for host_index, host in enumerate(('Windows', 'Linux')):
        raw = (ARCHIVE / host / 'raw.csv').read_bytes()
        samples, diagnostics = defaultdict(list), {}
        for row in csv.DictReader(raw.decode().splitlines()):
            cell = (int(row['n']), int(row['seed']), row['scenario'], row['trace_sha256'])
            key = cell, row['implementation']
            if row['mode'] == 'prod' and 1 <= int(row['repeat']) <= 7:
                samples[key].append(float(row['seconds']))
            elif row['mode'] == 'diag':
                diagnostics[key] = int(row['requested_peak']) if row['requested_peak'] else None
        assert len(samples) == 360 and all(len(v) == 7 for v in samples.values())
        times = {key: median(v) for key, v in samples.items()}
        cells = sorted(cell for cell, name in times if name == 'full')
        summary = {}
        for i, (control, name) in enumerate(CONTROLS.items()):
            ratios = [(times[cell, 'full'] / times[cell, control], cell) for cell in cells]
            overall = median(r for r, _ in ratios)
            summary[control] = dict(overall=overall, compare_heavy=median(r for r,c in ratios if c[2]=='compare-heavy'),
                                    wins=sum(r < 1 for r,c in ratios), cells=len(ratios),
                                    worst=max(ratios), adverse_over_10_percent=[(r,c) for r,c in ratios if r>1.1])
            x, y = 15 + host_index * 375, 80 + 33*i
            bars += label(x,y,name)
            bars += f'<rect x="{x+98}" y="{y-11}" width="{overall*105:.2f}" height="13" fill="#266784"/>'
            bars += label(x+104+overall*105,y,f'{overall:.3f}')
        x = 15+host_index*375
        bars += label(x,45,host)+f'<path d="M{x+203} 54v242" stroke="#657483" stroke-dasharray="4 4"/>'
        bars += label(x+186,318,'1.00')
        scatter = {}
        for scenario in ('compare-heavy','update-heavy'):
            choices = [c for c in cells if c[:3] == (100000,17,scenario)]
            if not choices:
                continue
            cell, = choices
            scatter[scenario] = {}
            for control in ('full','tag','list_range'):
                peak = diagnostics[cell,control]
                if peak:
                    scatter[scenario][control] = dict(peak_requested_bytes=peak, elapsed_ms=1000*times[cell,control])
        result[host] = dict(raw_sha256=hashlib.sha256(raw).hexdigest(), comparisons=summary, scatter=scatter)
        for scenario, points in scatter.items():
            max_x = max(p['peak_requested_bytes'] for p in points.values())/2**20*1.15
            max_y = max(p['elapsed_ms'] for p in points.values())*1.15
            body = label(70,25,f'{host}, 100,000 initial occurrences, seed 17, {scenario}')
            body += '<path d="M90 45v230h590" fill="none" stroke="#172840"/>'
            for i in range(6):
                body += label(90+118*i-8,295,f'{max_x*i/5:.1f}')
                body += label(28,280-46*i,f'{max_y*i/5:.1f}')
            body += label(260,325,'Diagnostic peak requested memory (MiB)')+label(10,45,'ms')
            for name, point in points.items():
                x = 90+590*point['peak_requested_bytes']/2**20/max_x
                y = 275-230*point['elapsed_ms']/max_y
                body += f'<circle cx="{x:.2f}" cy="{y:.2f}" r="5" fill="#266784"/>'+label(x+8,y-5,CONTROLS.get(name,'Full'))
            svg(HERE / f'{host.lower()}-{scenario}.svg', f'{host} memory and elapsed time',body)
    svg(HERE / 'live-ratios.svg', 'Matched median live-order elapsed ratios by host', bars)
    (HERE / 'live-derived.json').write_text(json.dumps(result, indent=2)+'\n',encoding='utf-8')


if __name__ == '__main__':
    derive()
