"""Conditional storage-user clusters; never proof of translation-unit boundaries.

Loader-zero placement does not establish file-static storage or C++ ownership:
ordinary external C definitions initialized to zero can also emit .bss. Merging
code users and interleaved storage ranges is useful only under independently
supported local-ownership and contiguous-contribution hypotheses. A gap is not
a proven TU boundary. Supply the reviewed range explicitly; no universal
communal-tail cutoff or language-specific exclusion list is assumed.
"""
import argparse
from contextlib import closing
import json
import sqlite3
from pathlib import Path

from env import ROOT

DB = ROOT / 'work/archaeology.sqlite'


def build(module, function_map, bss_lo, tail, exclude, original_root=None):
    if not 0 <= bss_lo < tail <= 0x100000000:
        raise ValueError('Expected an increasing 32-bit storage range')
    fm = json.loads(Path(function_map).read_text())
    spans = sorted((int(f['rva'], 16), f['size']) for f in fm['functions'])
    end = int(fm['first_party_end'], 16)
    own = lambda a: next((s for s, n in spans if s <= a < s + n), None)
    database = Path(original_root).resolve() / 'work/archaeology.sqlite' if original_root is not None else DB
    with closing(sqlite3.connect(database.as_uri() + '?mode=ro', uri=True)) as db:
        references = db.execute("select target_rva, source_rva from refs where module=? and kind in ('linear_memory', 'linear_immediate') "
                                "and target_rva>=? and target_rva<?", (module, bss_lo, tail)).fetchall()
    users = {}
    for t, s in references:
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
    return dict(module=module, scope='Conditional storage-user clusters; not TU or source-language proof.',
                storage_range=[hex(bss_lo), hex(tail)],
                assumptions=['Selected storage has independently supported local ownership.',
                             'Code and storage contributions are contiguous in the hypothesized link.'],
                excluded=[hex(x) for x in sorted(exclude)],
                intervals=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', required=True)
    parser.add_argument('--bss', required=True, help='start RVA of the explicitly reviewed storage range')
    parser.add_argument('--tail', required=True, help='exclusive end RVA; not an inferred communal boundary')
    parser.add_argument('--exclude', nargs='*', default=[], help='shared or uncertain storage addresses to omit')
    parser.add_argument('--original-root', type=Path, help='Read the original-reference database from another checkout')
    args = parser.parse_args()
    report = build(args.module, args.function_map, int(args.bss, 16), int(args.tail, 16), {int(x, 16) for x in args.exclude}, args.original_root)
    out = ROOT / 'work/closure' / f'{args.module.lower()}_unit_bounds.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    for i in report['intervals']:
        print(f"code {i['code'][0]:>8}..{i['code'][1]:<8} {i['functions']:3d} fns   bss {i['bss'][0]}..{i['bss'][1]} ({i['variables']} vars)")
    print('report:', out)


if __name__ == '__main__':
    main()
