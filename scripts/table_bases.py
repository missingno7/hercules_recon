"""Diagnostic: indexed accesses in the original just below a staged array symbol (oracle only).

A region draft may name a record table by the first field it touched, so one table can appear
under several symbols with different bases. For every data symbol that staged sources declare as
an array, this lists register-relative memory operands and address immediates in first-party
code whose displacement lies within WINDOW bytes below the symbol and is not itself a staged
symbol. Accesses inside a staged function that references the symbol are skipped: its masked
equality already ties them to the symbol (for example a pointer bumped before reuse). The rest
suggest that the real object starts lower. They are leads, never acceptance.
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

import capstone
from capstone import x86

from env import ROOT
from pe import PE

WINDOW = 0x40


def oracle_operands(pe, spans, lo_data, hi_data):
    """{displacement: [(insn rva, kind)]} for register-relative operands and data immediates."""
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    base = pe.image_base
    found = defaultdict(list)
    for rva, size in spans:
        for insn in md.disasm(pe.read_rva(rva, size), base + rva):
            for op in insn.operands:
                if op.type == x86.X86_OP_MEM and (op.mem.base or op.mem.index):
                    disp = (op.mem.disp & 0xffffffff) - base
                    if lo_data <= disp < hi_data:
                        found[disp].append((insn.address - base, 'indexed'))
                elif op.type == x86.X86_OP_IMM:
                    value = (op.imm & 0xffffffff) - base
                    if lo_data <= value < hi_data:
                        found[value].append((insn.address - base, 'immediate'))
    return found


def build(module, function_map):
    state = json.loads((ROOT / 'recovery.json').read_text())
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper() and r.get('link_symbols')]
    symbols, sources, extents = {}, defaultdict(set), defaultdict(list)
    for r in rows:
        for name, address in r['link_symbols'].items():
            if name.startswith('_g_'):
                symbols[name] = int(address, 16)
                sources[name].add(r['source'])
                extents[name].append((int(r['rva'], 16), r['size']))
    arrays = set()
    for source in {s for v in sources.values() for s in v}:
        text = (ROOT / source).read_text()
        arrays |= {'_' + m for m in re.findall(r'\b(g_[0-9a-f]{5})\s*\[', text)}
    fm = json.loads(Path(function_map).read_text())
    spans = [(int(f['rva'], 16), f['size']) for f in fm['functions']]
    pe = PE(ROOT / rows[0]['target'])
    data = [s for s in pe.sections if s['name'] in ('.data', '.rdata')]
    lo_data = min(s['virtual_address'] for s in data)
    hi_data = max(s['virtual_address'] + s['virtual_size'] for s in data)
    found = oracle_operands(pe, spans, lo_data, hi_data)
    staged = set(symbols.values())
    leads = []
    for name in sorted(arrays & set(symbols)):
        s = symbols[name]
        explained = extents[name]
        below = {}
        for d in sorted(found):
            if not s - WINDOW <= d < s or d in staged:
                continue
            sites = [a for a, k in found[d] if k == 'indexed' and not any(lo <= a < lo + n for lo, n in explained)]
            if sites:
                below[hex(d)] = sorted({hex(a) for a in sites})[:6]
        if below:
            leads.append(dict(symbol=name, address=hex(s), sources=sorted(sources[name]), indexed_below=below))
    return dict(module=module, window=WINDOW, arrays=len(arrays & set(symbols)), leads=leads)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    parser.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v3.json'))
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = build(args.module, args.function_map)
    output = (args.output or ROOT / f'work/closure/{args.module.lower()}_table_bases.json').resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(f"{report['arrays']} staged array symbols; {len(report['leads'])} with indexed accesses below them; report: {output}")
    for lead in report['leads']:
        print(f"  {lead['symbol']} {lead['address']}: {lead['indexed_below']}")


if __name__ == '__main__':
    main()
