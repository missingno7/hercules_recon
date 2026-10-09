"""Diagnostic closure analysis of a module from masked/accepted rows (never acceptance).

Translation-unit evidence: the linker emits each object's .data and .bss contributions
contiguously, in link order, like its code. Functions whose private data intervals overlap
belong to one data cluster; a cluster whose functions are contiguous in code and whose data
follows the code order of its neighbours is a candidate translation unit.

"Private" addresses are referenced by at most --max-users functions. Unreconstructed
functions contribute no references, so clusters are lower bounds on unit extent.
"""
import argparse
import json
from collections import defaultdict
from pathlib import Path

from env import ROOT
from pe import PE


def build(module, max_users):
    state = json.loads((ROOT / 'recovery.json').read_text())
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper()]
    pe = PE(ROOT / rows[0]['target'])
    data = next(s for s in pe.sections if s['name'] == '.data')
    init_end = data['virtual_address'] + data['raw_size']
    end = data['virtual_address'] + data['virtual_size']
    refs = defaultdict(set)
    for r in rows:
        for addr in r.get('link_symbols', {}).values():
            a = int(addr, 16)
            if data['virtual_address'] <= a < end:
                refs[a].add(int(r['rva'], 16))
    private = {a: users for a, users in refs.items() if len(users) <= max_users}
    intervals = {}
    for kind, lo, hi in (('bss', init_end, end), ('init', data['virtual_address'], init_end)):
        per_fn = defaultdict(list)
        for a, users in private.items():
            if lo <= a < hi:
                for u in users:
                    per_fn[u].append(a)
        intervals[kind] = {fn: (min(v), max(v)) for fn, v in per_fn.items()}
    # Merge overlapping bss intervals into clusters.
    spans = sorted((lo, hi, fn) for fn, (lo, hi) in intervals['bss'].items())
    clusters = []
    for lo, hi, fn in spans:
        if clusters and lo <= clusters[-1]['hi']:
            clusters[-1]['hi'] = max(clusters[-1]['hi'], hi)
            clusters[-1]['functions'].append(fn)
        else:
            clusters.append(dict(lo=lo, hi=hi, functions=[fn]))
    code = sorted(int(r['rva'], 16) for r in rows)
    for c in clusters:
        fns = sorted(c['functions'])
        between = [f for f in code if fns[0] < f < fns[-1] and f not in fns]
        c.update(lo=hex(c['lo']), hi=hex(c['hi']), functions=[hex(f) for f in fns],
                 code_span=[hex(fns[0]), hex(fns[-1])], interleaved_rows=[hex(f) for f in between])
    # Point runs: single-owner bss addresses in ascending order; a new run starts whenever the
    # owner's code position goes backwards. Each run is a candidate unit's data block.
    def point_runs(lo_bound, hi_bound):
        points = sorted((a, next(iter(u))) for a, u in refs.items() if len(u) == 1 and lo_bound <= a < hi_bound)
        found = []
        for a, fn in points:
            if found and fn >= found[-1]['last_fn']:
                found[-1].update(hi=a, last_fn=fn)
                found[-1]['functions'].add(fn)
            else:
                found.append(dict(lo=a, hi=a, first_fn=fn, last_fn=fn, functions={fn}))
        return [dict(data=[hex(r['lo']), hex(r['hi'])], code=[hex(r['first_fn']), hex(r['last_fn'])],
                     functions=sorted(hex(f) for f in r['functions'])) for r in found], len(points)
    runs, bss_points = point_runs(init_end, end)
    init_runs, init_points = point_runs(data['virtual_address'], init_end)
    order = [int(c['code_span'][0], 16) for c in clusters]
    inversions = sum(1 for i in range(1, len(order)) if order[i] < order[i - 1])
    return dict(module=module, max_users=max_users, private_addresses=len(private),
                bss_point_runs=runs, bss_single_owner_points=bss_points,
                init_point_runs=init_runs, init_single_owner_points=init_points,
                bss_clusters=len(clusters), cluster_code_order_inversions=inversions,
                contiguous_clusters=sum(1 for c in clusters if not c['interleaved_rows']), clusters=clusters,
                init_intervals={hex(f): [hex(lo), hex(hi)] for f, (lo, hi) in sorted(intervals['init'].items())})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--max-users', type=int, default=3)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = build(args.module, args.max_users)
    output = (args.output or ROOT / f'work/closure/{args.module.lower()}_clusters.json').resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(f"{len(report['init_point_runs'])} init runs over {report['init_single_owner_points']} points; "
          f"{len(report['bss_point_runs'])} bss runs over {report['bss_single_owner_points']} points; {report['private_addresses']} private data addresses; {report['bss_clusters']} bss clusters; "
          f"{report['contiguous_clusters']} contiguous in code; {report['cluster_code_order_inversions']} "
          f"code-order inversions between clusters; report: {output}")


if __name__ == '__main__':
    main()
