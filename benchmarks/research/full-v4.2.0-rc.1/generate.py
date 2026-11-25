"""Derive RC live-order presentation from the immutable Preview.3 archive."""
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path
from math import ceil, floor, log10
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


def plot_scatter(host, scenario, points, chinese):
    colors = ('#176082', '#8c391b', '#4b538c', '#3f7130',
              '#a13d68', '#784a18', '#00736f', '#655350')
    chinese_names = ('Full', '稳定版 4.0', '稳定版 4.1', '预览版 1.2',
                     '链表', 'Treap', '原生标签区间', '原生链表区间')
    workload = {'compare-heavy': '比较密集', 'update-heavy': '更新密集'}[scenario] if chinese else scenario
    size = '初始实例数 100,000，种子 17' if chinese else '100,000 initial occurrences, seed 17'
    unit = '诊断运行的峰值请求内存（MiB）' if chinese else 'Diagnostic peak requested memory (MiB)'
    elapsed = '正式运行耗时中位数（ms，对数轴）' if chinese else 'Formal median elapsed (ms, log scale)'
    x_max = ceil(max(p['peak_requested_bytes'] for p in points.values()) / 2**20)
    lo = floor(log10(min(p['elapsed_ms'] for p in points.values())))
    hi = ceil(log10(max(p['elapsed_ms'] for p in points.values())))
    body = label(60,23,f'{host} · {size} · {workload}') + label(60,45,elapsed)
    body += '<path d="M85 70v220h580" fill="none" stroke="#172840"/>'
    for exp in range(lo, hi+1):
        y = 290 - 220*(exp-lo)/(hi-lo)
        body += label(20,y+4,f'{10**exp:g}')
        body += f'<path d="M85 {y}h580" stroke="#ddd"/>'
    for x_tick in range(0,x_max+1,2):
        body += label(80+580*x_tick/x_max,309,x_tick)
    body += label(220,333,unit)
    # Keep observed coordinates. Concentric outlines show near-coincident samples.
    for i, (name, point) in enumerate(points.items()):
        x = 85+580*point['peak_requested_bytes']/2**20/x_max
        y = 290-220*(log10(point['elapsed_ms'])-lo)/(hi-lo)
        radius = 4 + i % 4 * 2
        legend_x, legend_y = 32+(i%4)*180, 354+(i//4)*25
        color = colors[i]
        value = escape(f"{name}: {point['peak_requested_bytes']} bytes; {point['elapsed_ms']:.6f} ms")
        body += f'<circle cx="{x:.2f}" cy="{y:.2f}" r="{radius}" fill="none" stroke="{color}" stroke-width="2"><title>{value}</title></circle>'
        body += f'<circle cx="{legend_x}" cy="{legend_y-4}" r="5" fill="{color}"/>'
        body += label(legend_x+10,legend_y,chinese_names[i] if chinese else CONTROLS.get(name,'Full'))
    suffix = '.zh-CN' if chinese else ''
    svg(HERE / f'{host.lower()}-{scenario}{suffix}.svg', f'{host} · {workload} · {elapsed}', body, height=400)


def derive():
    result = {}
    bars = label(12, 21, 'Full / control elapsed ratio (lower is faster)')
    bars_cn = label(12, 21, 'Full / 对照组耗时比（越小越快）')
    names_cn = ('稳定版 4.0', '稳定版 4.1', '预览版 1.2', '链表', 'Treap', '原生标签区间', '原生链表区间')
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
            bar = f'<rect x="{x+120}" y="{y-11}" width="{overall*105:.2f}" height="13" fill="#266784"/>'
            value = label(x+126+overall*105,y,f'{overall:.3f}')
            bars += label(x,y,name) + bar + value
            bars_cn += label(x,y,names_cn[i]) + bar + value
        x = 15+host_index*375
        axis = label(x,45,host)+f'<path d="M{x+225} 54v242" stroke="#657483" stroke-dasharray="4 4"/>' + label(x+208,318,'1.00')
        bars += axis
        bars_cn += axis
        scatter = {}
        for scenario in ('compare-heavy','update-heavy'):
            choices = [c for c in cells if c[:3] == (100000,17,scenario)]
            if not choices:
                continue
            cell, = choices
            scatter[scenario] = {}
            for control in ('full', *CONTROLS):
                peak = diagnostics[cell,control]
                assert peak and times[cell,control] > 0, (host, cell, control)
                scatter[scenario][control] = dict(peak_requested_bytes=peak, elapsed_ms=1000*times[cell,control])
        result[host] = dict(raw_sha256=hashlib.sha256(raw).hexdigest(), comparisons=summary, scatter=scatter)
        for scenario, points in scatter.items():
            plot_scatter(host, scenario, points, chinese=False)
            plot_scatter(host, scenario, points, chinese=True)
    svg(HERE / 'live-ratios.svg', 'Matched median live-order elapsed ratios by host', bars)
    svg(HERE / 'live-ratios.zh-CN.svg', 'Windows 与 Linux 的配对耗时比中位数', bars_cn)
    (HERE / 'live-derived.json').write_text(json.dumps(result, indent=2)+'\n',encoding='utf-8')


if __name__ == '__main__':
    derive()
