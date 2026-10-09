"""Diagnostic function map of a module's first-party code (leads, not reviewed extents).

Starts: exports, direct call targets, address-taken immediates in code, code pointers held in
non-code sections, and entries after RET + NOP alignment (unreferenced functions).
Pointers stored inside code are jump-table entries and never start a function.
Library code located by scripts/library_map.py is excluded. Each span is decoded
from its own entry with capstone.
"""
import argparse
import json
import sqlite3
from pathlib import Path

import capstone

from env import ROOT
from pe import PE

DB = ROOT / 'work/archaeology.sqlite'


def build(module):
    db = sqlite3.connect(DB)
    path = db.execute('select path from modules where name=?', (module,)).fetchone()[0]
    pe = PE(ROOT / path)
    text = next(s for s in pe.sections if s['characteristics'] & 0x20000000)
    lo = text['virtual_address']
    hi = lo + min(text['virtual_size'], text['raw_size'])
    code = pe.read_rva(lo, hi - lo)
    library = json.loads((ROOT / 'work/library_map/library_map.json').read_text())['contributions'][module]
    lib = set()
    for c in library:
        if c['size'] > 16:  # lone RET bodies are ambiguous, not library evidence
            lib.update(range(int(c['rva'], 16), int(c['rva'], 16) + c['size']))
    end = min([r for r in lib] + [hi])
    starts = set()
    for target, source, kind in db.execute('select target_rva, source_rva, kind from refs where module=?', (module,)):
        if not lo <= target < end:
            continue
        if kind == 'linear_call':
            starts.add(target)
        elif kind == 'pe_highlow_pointer' and not lo <= source < hi:
            starts.add(target)
        elif kind == 'linear_immediate' and lo <= source < hi:
            starts.add(target)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    # Entries after RET followed by NOP alignment.
    prologue = {0x53, 0x55, 0x56, 0x57, 0x83, 0x8b, 0xa1}  # push reg, sub/mov esp forms, mov eax,[abs]
    heuristic = set()
    proven = [(int(r['rva'], 16), r['size']) for r in json.loads((ROOT / 'recovery.json').read_text())['functions']
              if Path(r['target']).name.upper() == module.upper() and r['status'] in ('CODEGEN_SIMILAR', 'FUNCTION_MATCH')]
    for i in range(0, end - lo - 1):
        if code[i] in (0xc3, 0xc2):
            j = i + (3 if code[i] == 0xc2 else 1)
            k = j
            while k < end - lo and code[k] == 0x90:
                k += 1
            if (lo + k) % 16 == 0 and k > j and k < end - lo and code[k] != 0x90:
                starts.add(lo + k)
            elif k == j and (lo + j) % 16 == 0 and j < end - lo and code[j] in prologue:
                # RET ending exactly on an alignment boundary, followed by a typical prologue. This is
                # a lead only (about 30% were mid-function on first measurement); never inside a
                # proven (masked/accepted) extent.
                if lo + j not in starts and not any(r < lo + j < r + s for r, s in proven):
                    starts.add(lo + j)
                    heuristic.add(lo + j)
    starts.add(lo)
    for export in pe.metadata().get('exports') or []:
        if lo <= export['rva'] < end:
            starts.add(export['rva'])
    ordered = sorted(s for s in starts if s % 16 == 0)
    rows = []
    for i, s in enumerate(ordered):
        e = ordered[i + 1] if i + 1 < len(ordered) else end
        body = code[s - lo:e - lo].rstrip(b'\x90')
        insns = list(md.disasm(body, s))
        decoded = sum(x.size for x in insns) == len(body)
        last = insns[-1].mnemonic if insns else None
        relocs = db.execute("select count(*) from refs where module=? and source_rva>=? and source_rva<? and "
                            "kind in ('linear_call','linear_memory','linear_immediate','pe_highlow_pointer') and target_rva>0",
                            (module, s, e)).fetchone()[0]
        callers = db.execute("select count(*) from refs where module=? and target_rva=? and kind='linear_call'",
                             (module, s)).fetchone()[0]
        rows.append(dict(rva=hex(s), size=e - s, body_size=len(body), decoded=decoded, last=last,
                         references=relocs, callers=callers, heuristic_start=s in heuristic))
    return dict(module=module, first_party_end=hex(end), functions=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = (args.output or ROOT / f'work/function_map/{args.module.lower()}.json').resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Diagnostic outputs belong in ignored work/')
    report = build(args.module)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    rows = report['functions']
    suspect = [r for r in rows if not r['decoded'] or r['last'] not in ('ret', 'jmp')]
    print(f"{len(rows)} spans to {report['first_party_end']}; {sum(r['size'] for r in rows)} bytes; "
          f"{len(suspect)} need review (decode/terminator); report: {output}")


if __name__ == '__main__':
    main()
