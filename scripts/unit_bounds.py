"""Conditional TU grouping leads from shared zero-storage references (oracle only).

These are hypotheses, not minimal or proven TU boundaries. Ordinary C external
explicit-zero definitions can occupy .bss too, and their users can cross TUs.
Neither address order nor the language identifies linkage or defining-object
ownership. The legacy grouping assumes local ownership and contiguous code/data
contributions; it is useful only after those premises receive independent evidence.
Interleaving variable addresses is likewise conditional, never a same-object proof.
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
    return dict(module=module, scope=f'Conditional shared-storage grouping in {bss_lo:#x}..{tail:#x}; not TU proof',
                ownership_proven=False,
                assumptions=['Referenced cells are local to one defining object (not established by BSS)',
                             'Code and storage contributions are contiguous in original link order'],
                limitations=['External explicit-zero C definitions can have cross-TU users',
                             'The selected address window does not prove a communal-tail boundary',
                             'Function-map coverage may omit unreviewed unaligned entries'],
                excluded=[hex(x) for x in sorted(exclude)],
                intervals=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v4.json'))
    parser.add_argument('--bss', default='0x290e8', help='lower bound of the selected zero-storage window')
    parser.add_argument('--tail', default='0x2b370', help='exclusive upper bound (legacy window, not a proven communal start)')
    parser.add_argument('--exclude', nargs='*', default=['0x29128', '0x29e08', '0x29f98', '0x29c2c', '0x2a244'],
                        help='cells excluded from the local-ownership hypothesis, regardless of source language')
    args = parser.parse_args()
    report = build(args.module, args.function_map, int(args.bss, 16), int(args.tail, 16), {int(x, 16) for x in args.exclude})
    out = ROOT / 'work/closure' / f'{args.module.lower()}_unit_bounds.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(report['scope'])
    for i in report['intervals']:
        print(f"code {i['code'][0]:>8}..{i['code'][1]:<8} {i['functions']:3d} fns   bss {i['bss'][0]}..{i['bss'][1]} ({i['variables']} vars)")
    print('report:', out)


if __name__ == '__main__':
    main()
