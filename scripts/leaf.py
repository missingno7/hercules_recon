"""Private leaf iteration: compact packets and fresh raw comparisons. Never changes state.

packet MODULE RVA SIZE     text listing, raw bytes and caller argument setup
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


def packet(module, rva, size, callers=4, before=8):
    db = sqlite3.connect(DB)
    pe = PE(module_path(db, module))
    lines = [f'{module} {rva:#x} size {size} (span to next aligned entry; lead, not reviewed extent)',
             'bytes: ' + pe.read_rva(rva, size).hex(), '']
    lines += listing(db, module, rva, rva + size)
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


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['packet', 'check'])
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
    if not args.symbol or not args.source:
        parser.error('check requires SYMBOL and SOURCE')
    result = check(args.module, rva, size, args.symbol, args.source, args.flags)
    print(f"{'EXACT' if result['exact'] else 'MISMATCH'} {result['compiled_size']}/{result['target_size']} bytes; "
          f"{result['different_bytes']} differing; {len(result['relocations'])} object relocations")
    if result['mismatch_regions']:
        print('mismatch intervals:', result['mismatch_regions'][:16])
        from coff import COFF
        actual = COFF(result['object']).function(args.symbol)['data']
        print('compiled:', actual.hex())
    if result['relocations']:
        print('relocations:', json.dumps(result['relocations']))
    raise SystemExit(0 if result['exact'] else 1)


if __name__ == '__main__':
    main()
