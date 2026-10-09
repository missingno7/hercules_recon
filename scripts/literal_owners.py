"""Diagnostic: owners of string-like data referenced from first-party code (oracle only).

Every base relocation inside first-party code whose target lies in initialized .data and
points at a printable NUL-terminated string is attributed to the function span containing
the relocation (scripts/function_map.py report). Literal order versus owner code order is
evidence for translation-unit boundaries and link order; it is never acceptance.
"""
import argparse
import json
import struct
from pathlib import Path

from env import ROOT
from pe import PE


def string_at(pe, rva, limit=96):
    try:
        raw = pe.read_rva(rva, limit)
    except Exception:  # noqa: BLE001
        return None
    end = raw.find(b'\0')
    if end < 2:
        return None
    text = raw[:end]
    if all(32 <= b < 127 or b in (9, 10, 13) for b in text):
        return text.decode('ascii')
    return None


def build(module, function_map, all_data=False):
    fm = json.loads(Path(function_map).read_text())
    spans = sorted((int(f['rva'], 16), f['size']) for f in fm['functions'])
    first_party_end = int(fm['first_party_end'], 16)
    path = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL'}[module.upper()]
    pe = PE(ROOT / path)
    data = next(s for s in pe.sections if s['name'] == '.data')
    init_end = data['virtual_address'] + data['raw_size']
    base = pe.image_base
    literals = {}
    for block in pe.pe.DIRECTORY_ENTRY_BASERELOC:
        for entry in block.entries:
            if not entry.type or not 0x1000 <= entry.rva < first_party_end:
                continue
            target = struct.unpack('<I', pe.read_rva(entry.rva, 4))[0] - base
            if not data['virtual_address'] <= target < init_end:
                continue
            text = string_at(pe, target)
            if text is None:
                if not all_data:
                    continue
                text = '<data>'
            owner = next((s for s, size in spans if s <= entry.rva < s + size), None)
            literals.setdefault(target, dict(text=text, owners=set()))['owners'].add(owner)
    rows = [dict(addr=hex(a), text=v['text'][:40], owners=sorted(hex(o) for o in v['owners'] if o is not None))
            for a, v in sorted(literals.items())]
    # Runs of non-decreasing owner code order over ascending literal addresses.
    runs, last = [], None
    for r in rows:
        if len(r['owners']) != 1:
            continue
        owner = int(r['owners'][0], 16)
        if runs and owner >= last:
            runs[-1]['hi'] = r['addr']
            runs[-1]['owners'].add(owner)
        else:
            runs.append(dict(lo=r['addr'], hi=r['addr'], owners={owner}))
        last = owner
    runs = [dict(data=[r['lo'], r['hi']], code=[hex(min(r['owners'])), hex(max(r['owners']))], owners=len(r['owners']))
            for r in runs]
    return dict(module=module, literals=len(rows), single_owner=sum(1 for r in rows if len(r['owners']) == 1),
                runs=runs, rows=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v4.json'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--all-data', action='store_true', help='include non-string initialized data targets')
    args = parser.parse_args()
    report = build(args.module, args.function_map, args.all_data)
    suffix = 'init_data' if args.all_data else 'literals'
    output = (args.output or ROOT / f'work/closure/{args.module.lower()}_{suffix}.json').resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(f"{report['literals']} string literals referenced from first-party code; {report['single_owner']} single-owner; "
          f"{len(report['runs'])} non-decreasing runs; report: {output}")


if __name__ == '__main__':
    main()
