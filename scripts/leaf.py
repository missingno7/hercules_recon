"""Private leaf iteration: compact packets and fresh raw comparisons. Never changes state.

packet MODULE RVA SIZE     text listing, raw bytes and caller argument setup
data   MODULE RVA SIZE     hex and ASCII of image bytes at RVA (strings, tables)
diff   MODULE RVA SIZE SYMBOL SOURCE   aligned instruction diff of compiled vs original
residual MODULE RVA SIZE SYMBOL SOURCE bounded raw/masked mismatch islands
check  MODULE RVA SIZE SYMBOL SOURCE [--flags ...]
                           fresh pinned VC5 compile; raw compare of the full /Gy contribution
"""
import argparse
import json
import sqlite3
from pathlib import Path

from env import ROOT
from match import compile_source, compare, digest
from pe import PE

DB = ROOT / 'work/archaeology.sqlite'


def module_path(db, module):
    row = db.execute('select path from modules where name=?', (module,)).fetchone()
    if not row:
        raise SystemExit('Unknown module ' + module)
    return ROOT / row[0]


def listing(db, module, start, end):
    return [f'{rva:#07x}  {hx:<16} {mn} {ops}'.rstrip() for rva, hx, mn, ops in db.execute(
        'select rva, hex, mnemonic, operands from instructions where module=? and rva>=? and rva<? order by rva',
        (module, start, end))]


def library_names(module):
    path = ROOT / 'work/library_map/library_map.json'
    if not path.exists():
        return {}
    rows = json.loads(path.read_text())['contributions'].get(module, [])
    return {int(r['rva'], 16): ','.join(r['names'][:2]) for r in rows if r['size'] > 16 and r['names']}


def packet(module, rva, size, callers=4, before=8):
    db = sqlite3.connect(DB)
    pe = PE(module_path(db, module))
    names = library_names(module)
    called = sorted({t for (t,) in db.execute(
        "select target_rva from refs where module=? and source_rva>=? and source_rva<? and kind='linear_call'",
        (module, rva, rva + size))})
    lines = [f'{module} {rva:#x} size {size} (span to next aligned entry; lead, not reviewed extent)',
             'bytes: ' + pe.read_rva(rva, size).hex(), '']
    lines += listing(db, module, rva, rva + size)
    if called:
        lines += ['', 'call targets: ' + ' '.join(f'{t:#x}' + (f' [library {names[t]}]' if t in names else '')
                                               for t in called)]
    sources = [s for (s,) in db.execute(
        "select source_rva from refs where module=? and target_rva=? and kind like '%call%' order by source_rva",
        (module, rva))]
    pointers = [s for (s,) in db.execute(
        "select source_rva from refs where module=? and target_rva=? and kind not like '%call%' order by source_rva",
        (module, rva))]
    lines += ['', f'{len(sources)} direct callers: ' + ' '.join(hex(s) for s in sources)]
    if pointers:
        lines.append('other references: ' + ' '.join(hex(s) for s in pointers))
    for s in sources[:callers]:
        rows = db.execute('select rva from instructions where module=? and rva<=? order by rva desc limit ?',
                          (module, s + 8, before + 2)).fetchall()
        lo = min(r for (r,) in rows)
        lines += ['', f'-- caller site {s:#x}'] + listing(db, module, lo, s + 12)
    return '\n'.join(lines)


def check(module, rva, size, symbol, source, flags):
    db = sqlite3.connect(DB)
    target = module_path(db, module)
    source_before = digest(Path(source).read_bytes())
    target_before = digest(target.read_bytes())
    obj, command = compile_source(source, flags)
    result = compare(obj, symbol, target, rva, size)
    if digest(Path(source).read_bytes()) != source_before or digest(target.read_bytes()) != target_before:
        raise ValueError('Source or oracle changed during diagnostic compilation')
    result.update(source_sha256=source_before, target_file_sha256=target_before, command=command)
    return result


def diff(module, rva, size, symbol, source, flags):
    import difflib
    from coff import COFF
    from residual import instruction_rows
    from masked import REL32, DIR32
    db = sqlite3.connect(DB)
    result = check(module, rva, size, symbol, source, flags)
    section = COFF(result['object']).function(symbol)
    pe = PE(module_path(db, module))
    target_fields = {e.rva - rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries
                     if e.type and rva <= e.rva < rva + size}
    original = instruction_rows(pe.read_rva(rva, size), rva, target_fields)
    compiled = instruction_rows(section['data'], 0, {r['offset'] for r in section['relocations'] if r['type'] in (REL32, DIR32)})
    lines = []
    matcher = difflib.SequenceMatcher(None, [r[1] for r in original], [r[1] for r in compiled], autojunk=False)
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        lines.append(f'--- {tag} original[{a0}:{a1}] compiled[{b0}:{b1}]')
        lines += [f'  - {o:#05x}  {t}' for o, _, t, _ in original[a0:a1]]
        lines += [f'  + {o:#05x}  {t}' for o, _, t, _ in compiled[b0:b1]]
    return chr(10).join(lines) or 'instruction streams equal after address normalization'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['packet', 'check', 'data', 'diff', 'residual'])
    parser.add_argument('module')
    parser.add_argument('rva')
    parser.add_argument('size')
    parser.add_argument('symbol', nargs='?')
    parser.add_argument('source', nargs='?', type=Path)
    parser.add_argument('--flags', nargs='*', default=['/O2'])
    parser.add_argument('--callers', type=int, default=4)
    parser.add_argument('--islands', type=int, default=4)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    rva, size = int(args.rva, 0), int(args.size, 0)
    if args.command == 'packet':
        print(packet(args.module, rva, size, args.callers))
        return
    if args.command == 'data':
        pe = PE(module_path(sqlite3.connect(DB), args.module))
        data = pe.read_rva(rva, size)
        for i in range(0, len(data), 16):
            chunk = data[i:i + 16]
            text = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
            print(f'{rva + i:#07x}  {chunk.hex(" "):<47}  {text}')
        return
    if not args.symbol or not args.source:
        parser.error('check requires SYMBOL and SOURCE')
    if args.command == 'residual':
        from residual import residual
        if args.islands < 1:
            parser.error('--islands must be positive')
        if args.report and not any(args.report.resolve().is_relative_to((ROOT / d).resolve()) for d in ('work', 'build')):
            parser.error('Generated residual reports belong under work/ or build/')
        result = residual(args.module, rva, size, args.symbol, args.source, args.flags, args.islands)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(f"RAW {'EXACT' if result['raw']['exact'] else 'DIFFERENT'}; "
              f"MASKED {'EQUAL' if result['masked']['masked_equal'] else 'DIFFERENT'}; "
              f"{result['masked']['compiled_size']}/{size} bytes; "
              f"{result['masked']['differing_bytes']} residual bytes; "
              f"{len(result['masked']['conflicts'])} binding conflicts; "
              f"{len(result['masked']['unsupported'])} unsupported relocations")
        for island in result['islands']:
            print(f"{island['decoded_span']}: {island['kind']}")
            for side in ('original', 'compiled'):
                print(side + ':')
                for row in island[side]:
                    print(f"  {row['offset']} {row['instruction']}")
                if island['omitted_instructions'][side]:
                    print(f"  {island['omitted_instructions'][side]} further instructions omitted")
        print(f"{result['omitted_islands']} further islands omitted; diagnostic only")
        return
    if args.command == 'diff':
        print(diff(args.module, rva, size, args.symbol, args.source, args.flags))
        return
    result = check(args.module, rva, size, args.symbol, args.source, args.flags)
    print(f"{'EXACT' if result['exact'] else 'MISMATCH'} {result['compiled_size']}/{result['target_size']} bytes; "
          f"{result['different_bytes']} differing; {len(result['relocations'])} object relocations")
    if result['mismatch_regions']:
        print('mismatch intervals:', result['mismatch_regions'][:16])
        from coff import COFF
        actual = COFF(result['object']).function(args.symbol)['data']
        print('compiled:', actual.hex())
    if result['relocations']:
        from masked import masked_compare
        db = sqlite3.connect(DB)
        m = masked_compare(result['object'], args.symbol, module_path(db, args.module), rva, size)
        print(f"MASKED {'EQUAL' if m['masked_equal'] else 'DIFFERENT'}: {m['differing_bytes']} non-relocation "
              f"bytes differ; {m['relocations']} relocations; conflicts {m['conflicts']}")
        print('implied symbols:', json.dumps(m['symbols']))
        if m['unsupported']:
            print('unsupported relocations:', m['unsupported'])
        raise SystemExit(0 if m['masked_equal'] else 1)
    raise SystemExit(0 if result['exact'] else 1)


if __name__ == '__main__':
    main()
