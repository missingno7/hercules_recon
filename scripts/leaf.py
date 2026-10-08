"""Private leaf iteration: compact packets and fresh raw comparisons. Never changes state.

packet MODULE RVA SIZE     text listing, raw bytes and caller argument setup
data   MODULE RVA SIZE     hex and ASCII of image bytes at RVA (strings, tables)
diff   MODULE RVA SIZE SYMBOL SOURCE   aligned instruction diff of compiled vs original
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
    obj, command = compile_source(source, flags)
    result = compare(obj, symbol, target, rva, size)
    result.update(source_sha256=digest(Path(source).read_bytes()), command=command)
    return result


def diff(module, rva, size, symbol, source, flags):
    import difflib
    import re
    import capstone
    from coff import COFF
    db = sqlite3.connect(DB)
    obj, _ = compile_source(source, flags)
    section = COFF(obj).function(symbol)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    relocated = {r['offset'] for r in section['relocations']}

    def rows(code, base, fields):
        out = []
        for i in md.disasm(code, base):
            text = f'{i.mnemonic} {i.op_str}'.strip()
            key = text
            relocated = any(i.address - base <= f < i.address - base + i.size for f in fields)
            if relocated:
                # Operands holding relocation fields differ by design; keep only the shape.
                key = re.sub(r'(0x[0-9a-f]+|[0-9]+)', 'R', text)
                key = re.sub(r'\[([a-z]{3})\]', r'[ + R]', key)
                if i.mnemonic == 'call' or i.mnemonic.startswith('j'):
                    key = f'{i.mnemonic} EXTERNAL'
            elif i.mnemonic.startswith('j') or i.mnemonic == 'call':
                m = re.fullmatch(r'0x([0-9a-f]+)', i.op_str)
                if m:
                    target = int(m.group(1), 16) - base
                    if 0 <= target < len(code):
                        key = text = f'{i.mnemonic} +{target:#x}'
                    else:
                        key = f'{i.mnemonic} EXTERNAL'
            out.append((i.address - base, key, text))
        return out
    pe = PE(module_path(db, module))
    target_fields = {e.rva - rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries
                     if e.type and rva <= e.rva < rva + size}
    original = rows(pe.read_rva(rva, size), rva, target_fields)
    compiled = rows(section['data'], 0, {r['offset'] for r in section['relocations']})
    lines = []
    matcher = difflib.SequenceMatcher(None, [r[1] for r in original], [r[1] for r in compiled], autojunk=False)
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        lines.append(f'--- {tag} original[{a0}:{a1}] compiled[{b0}:{b1}]')
        lines += [f'  - {o:#05x}  {t}' for o, _, t in original[a0:a1]]
        lines += [f'  + {o:#05x}  {t}' for o, _, t in compiled[b0:b1]]
    return chr(10).join(lines) or 'instruction streams equal after address normalization'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['packet', 'check', 'data', 'diff'])
    parser.add_argument('module')
    parser.add_argument('rva')
    parser.add_argument('size')
    parser.add_argument('symbol', nargs='?')
    parser.add_argument('source', nargs='?', type=Path)
    parser.add_argument('--flags', nargs='*', default=['/O2'])
    parser.add_argument('--callers', type=int, default=4)
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
