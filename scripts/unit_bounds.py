"""Diagnostic: minimal translation-unit intervals implied by per-object .bss sharing (oracle only).

In C, an uninitialized variable placed in the per-object .bss region (before the communal tail) is
a file static, so every function that uses it belongs to the same object. Objects are contiguous
code ranges in link order, so the user span of each such variable must lie inside one object.
Merging overlapping spans gives minimal unit intervals: a gap between two merged intervals is a
possible boundary; inside an interval there is none. Variables of C++ units (shared DLL-wide,
listed with --exclude) do not constrain ownership and are skipped.

Second rule: one object's .bss is contiguous (its variables are ordered by name, not by user), so
two intervals whose .bss address ranges interleave are the same object and are merged too.
"""
import argparse
import json
import sqlite3
from pathlib import Path

from env import ROOT

DB = ROOT / 'work/archaeology.sqlite'


def build(module, function_map, bss_lo, tail, exclude):
    fm = json.loads(Path(function_map).read_text())
    spans = sorted((int(f['rva'], 16), f['size']) for f in fm['functions'])
    end = int(fm['first_party_end'], 16)
    own = lambda a: next((s for s, n in spans if s <= a < s + n), None)
    db = sqlite3.connect(DB)
    users = {}
    for t, s in db.execute("select target_rva, source_rva from refs where module=? and kind in ('linear_memory', 'linear_immediate') "
                           "and target_rva>=? and target_rva<?", (module, bss_lo, tail)):
        o = own(s)
        if o is not None and o < end and t not in exclude:
            users.setdefault(t, set()).add(o)
    groups = [dict(lo=min(u), hi=max(u), vars=[t]) for t, u in users.items()]
    changed = True
    while changed:  # merge on overlapping code spans or interleaving .bss ranges
        changed = False
        groups.sort(key=lambda g: g['lo'])
        out = []
        for g in groups:
            hit = next((m for m in out if (g['lo'] <= m['hi'] and m['lo'] <= g['hi']) or
                        (min(g['vars']) <= max(m['vars']) and min(m['vars']) <= max(g['vars']))), None)
            if hit:
                hit['lo'], hit['hi'] = min(hit['lo'], g['lo']), max(hit['hi'], g['hi'])
                hit['vars'] += g['vars']
                changed = True
            else:
                out.append(g)
        groups = out
    merged = sorted(groups, key=lambda g: g['lo'])
    out = []
    for m in merged:
        funcs = [s for s, _ in spans if m['lo'] <= s <= m['hi']]
        out.append(dict(code=[hex(m['lo']), hex(m['hi'])], functions=len(funcs), bss=[hex(min(m['vars'])), hex(max(m['vars']))],
                        variables=len(m['vars'])))
    return dict(module=module, scope=f'per-object .bss {bss_lo:#x}..{tail:#x}', excluded=[hex(x) for x in sorted(exclude)],
                intervals=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v3.json'))
    parser.add_argument('--bss', default='0x29200')
    parser.add_argument('--tail', default='0x2b370', help='start of the communal tail')
    parser.add_argument('--exclude', nargs='*', default=['0x29e08', '0x29f98'], help='C++-defined shared variables')
    args = parser.parse_args()
    report = build(args.module, args.function_map, int(args.bss, 16), int(args.tail, 16), {int(x, 16) for x in args.exclude})
    out = ROOT / 'work/closure' / f'{args.module.lower()}_unit_bounds.json'
    out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    for i in report['intervals']:
        print(f"code {i['code'][0]:>8}..{i['code'][1]:<8} {i['functions']:3d} fns   bss {i['bss'][0]}..{i['bss'][1]} ({i['variables']} vars)")
    print('report:', out)


if __name__ == '__main__':
    main()
