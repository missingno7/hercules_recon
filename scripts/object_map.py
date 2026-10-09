"""Diagnostic: candidate object (translation-unit) segments of a module's initialized .data.

VC5 LINK emits, per object in link order, the object's ordinary .data and then its string-literal
COMDATs (measured, see evidence/title_closure.json units). So in the original image a run of
literals ends an object, and ordinary data that follows a literal run starts a new object.

Items are the targets of base relocations from first-party code or from pointer slots in .data,
classified as literal (printable NUL-terminated text containing no pointer slot; strings reached
through initialized pointer tables are literals of that initializer) or ordinary.
Each segment lists the code users of its items: their range bounds which code the object owns
or exports to. Folded literals (shared by several objects) can blur a boundary. Leads only.
"""
import argparse
import json
import struct
from collections import defaultdict
from pathlib import Path

from env import ROOT
from pe import PE


def printable(pe, rva, limit):
    raw = pe.read_rva(rva, min(128, limit - rva))
    end = raw.find(b'\0')
    return end >= 1 and all(32 <= c < 127 or c in (9, 10, 13) for c in raw[:end])


def build(module, function_map):
    fm = json.loads(Path(function_map).read_text())
    spans = sorted((int(f['rva'], 16), f['size']) for f in fm['functions'])
    first_party_end = int(fm['first_party_end'], 16)
    pe = PE(ROOT / {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL'}[module.upper()])
    data = next(s for s in pe.sections if s['name'] == '.data')
    lo, hi = data['virtual_address'], data['virtual_address'] + data['raw_size']
    relocs = sorted(e.rva for b in pe.pe.DIRECTORY_ENTRY_BASERELOC for e in b.entries if e.type)
    users = defaultdict(set)
    pointed_from_data = set()
    for at in relocs:
        target = struct.unpack('<I', pe.read_rva(at, 4))[0] - pe.image_base
        if not lo <= target < hi:
            continue
        if 0x1000 <= at < first_party_end:
            owner = next((s for s, n in spans if s <= at < s + n), None)
            if owner is not None:
                users[target].add(owner)
        elif lo <= at < hi:
            pointed_from_data.add(target)
    items = sorted(set(users) | pointed_from_data)
    reloc_set = set(relocs)

    def literal(t):
        # Printable text with no pointer slot inside it (pointer tables can look like short text).
        if not printable(pe, t, hi):
            return False
        n = pe.read_rva(t, min(128, hi - t)).find(bytes(1))
        return not any(t - 3 <= r < t + n + 1 for r in reloc_set)

    kinds = {t: 'literal' if literal(t) else 'ordinary' for t in items}
    segments, current = [], None
    for t in items:
        kind = 'literal' if kinds[t] == 'literal' else 'ordinary'
        if current is None or (current['phase'] == 'literal' and kind == 'ordinary'):
            current = dict(start=t, phase='ordinary', items=[])
            segments.append(current)
        if kind == 'literal':
            current['phase'] = 'literal'
        current['items'].append(t)
    out = []
    for s in segments:
        code = sorted({u for t in s['items'] for u in users.get(t, ())})
        lit = [t for t in s['items'] if kinds[t] == 'literal']
        out.append(dict(start=hex(s['start']), end=hex(s['items'][-1]), items=len(s['items']), literals=len(lit),
                        first_literal=hex(lit[0]) if lit else None,
                        code_users=[hex(c) for c in code][:12], code_range=[hex(code[0]), hex(code[-1])] if code else None))
    return dict(module=module, segments=out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v4.json'))
    args = parser.parse_args()
    report = build(args.module, args.function_map)
    out = ROOT / 'work/closure' / f'{args.module.lower()}_objects.json'
    out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    for s in report['segments']:
        print(f"{s['start']:>8}..{s['end']:<8} items {s['items']:3d} literals {s['literals']:3d}  users {s['code_users'][:8]}")
    print('report:', out)


if __name__ == '__main__':
    main()
