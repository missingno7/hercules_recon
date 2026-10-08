"""Integrator-only leaf staging: review extents, then add IDENTIFIED rows for promote.py.

review MODULE RVA SIZE   print the mechanical extent review (no state change)
stage PLAN               review every function, freshly compile the plan candidate,
                         require raw exactness, then append IDENTIFIED rows.

Acceptance remains promote.py: this only records reviewed target context.
A plan is {"source": "src/...c", "candidate": "...c",
           "functions": [{"id", "module", "rva", "size", "symbol", "evidence"}]}.
"""
import argparse
import json
import sqlite3
from pathlib import Path

from env import ROOT
import capstone
from match import compile_source, compare, digest
from pe import PE

DB = ROOT / 'work/archaeology.sqlite'
STATE = ROOT / 'recovery.json'
BOUNDARY = ('Mechanically reviewed leaf extent: one closed body ending in return, only NOP alignment '
            'to the next 16-byte entry, no interior entries, no outgoing branches/calls/absolute '
            'references; address-taken sources are instruction immediates or data tables.')


def review(module, rva, size):
    db = sqlite3.connect(DB)
    path = ROOT / db.execute('select path from modules where name=?', (module,)).fetchone()[0]
    pe = PE(path)
    data = pe.read_rva(rva, size)
    body = data.rstrip(b'\x90')
    problems = []
    if rva & 15 or (rva + size) & 15:
        problems.append('extent not 16-byte aligned')
    if len(data) - len(body) >= 16:
        problems.append('16 or more padding bytes')
    # Decode from the entry itself: the archaeology index decodes linearly and can
    # desynchronize on a preceding jump table.
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    insns = list(md.disasm(body, rva))
    if sum(i.size for i in insns) != len(body):
        problems.append('entry decode does not cover the body')
    if not insns or insns[-1].mnemonic not in ('ret', 'jmp'):
        problems.append('body does not end in ret/jmp')
    outgoing = db.execute("select count(*) from refs where module=? and source_rva>=? and source_rva<? and "
                          "target_rva>0 and (kind!='linear_branch' or target_rva<? or target_rva>=?)",
                          (module, rva, rva + size, rva, rva + size)).fetchone()[0]
    if outgoing:
        problems.append(f'{outgoing} outgoing references')
    interior = db.execute("select kind, count(*) from refs where module=? and target_rva>? and target_rva<? and "
                          "(source_rva<? or source_rva>=?) group by kind",
                          (module, rva, rva + size, rva, rva + size)).fetchall()
    warnings = []
    for kind, count in interior:
        # Calls/pointers into the body are strong evidence of another entry; linear-decode
        # branches from neighbouring padding or data are weak and are reported for review.
        (warnings if kind == 'linear_branch' else problems).append(f'{count} external {kind} into the interior')
    text = [s for s in pe.sections if s['characteristics'] & 0x20000000]
    callers, tables, immediates = [], [], []
    va = (pe.image_base + rva).to_bytes(4, 'little')
    for source, kind in db.execute('select source_rva, kind from refs where module=? and target_rva=?', (module, rva)):
        if 'call' in kind or kind == 'linear_branch':
            callers.append(hex(source) + ('' if 'call' in kind else ' (tail jump)'))
            continue
        in_text = any(s['virtual_address'] <= source < s['virtual_address'] + s['virtual_size'] for s in text)
        insn = db.execute('select hex from instructions where module=? and rva<=? and rva+size>? order by rva desc limit 1',
                          (module, source, source)).fetchone()
        if in_text and insn and va in bytes.fromhex(insn[0]):
            starts = db.execute('select rva from instructions where module=? and rva<=? order by rva desc limit 1',
                                (module, source)).fetchone()[0]
            immediates.append(hex(starts))
        elif in_text:
            problems.append(f'code-section data reference {source:#x} (possible jump table)')
        else:
            tables.append(hex(source))
    return dict(module=module, rva=hex(rva), size=size, body_size=len(body), ok=not problems, problems=problems,
                warnings=warnings,
                callers=callers, address_immediates=immediates, data_tables=tables, target_sha256=digest(data))


def stage(plan_path):
    plan = json.loads(Path(plan_path).read_text())
    source = plan['source']
    if not (ROOT / source).resolve().is_relative_to((ROOT / 'src').resolve()):
        raise ValueError('Plan source must be under src/')
    state = json.loads(STATE.read_text())
    ids = {r['id'] for r in state['functions']}
    places = {(Path(r['target']).name.upper(), int(r['rva'], 16)) for r in state['functions']}
    candidate = ROOT / plan['candidate']
    obj, _ = compile_source(candidate, ['/O2'])
    db = sqlite3.connect(DB)
    rows = []
    for f in plan['functions']:
        module, rva, size = f['module'], int(f['rva'], 16), f['size']
        if f['id'] in ids or (module.upper(), rva) in places:
            raise ValueError('Already tracked: ' + f['id'])
        result = review(module, rva, size)
        if not result['ok']:
            raise ValueError(f"Extent review failed for {f['id']}: {result['problems']}")
        target_rel = db.execute('select path from modules where name=?', (module,)).fetchone()[0]
        target = ROOT / target_rel
        proof = compare(obj, f['symbol'], target, rva, size)
        if not proof['exact']:
            raise ValueError(f"Candidate not raw exact for {f['id']}: {proof['mismatch_regions'][:4]}")
        row = dict(id=f['id'], status='IDENTIFIED', name_status='provisional semantic/offset label',
                   target=Path(target_rel).as_posix(), target_sha256=digest(target.read_bytes()),
                   rva=hex(rva), size=size, body_size=result['body_size'], source=source,
                   symbol=f['symbol'], flags=['/O2'], evidence=f['evidence'], boundary=BOUNDARY)
        if result['callers']:
            row['callers'] = result['callers']
        rows.append(row)
    state['functions'].extend(rows)
    STATE.write_text(json.dumps(state, indent=2) + '\n')
    print(f'Staged {len(rows)} IDENTIFIED rows for {source}; promote with:')
    print('  python scripts/promote.py ' + ' '.join(r['id'] for r in rows) + f' --source {plan["candidate"]}')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    r = sub.add_parser('review')
    r.add_argument('module')
    r.add_argument('rva')
    r.add_argument('size')
    s = sub.add_parser('stage')
    s.add_argument('plan', type=Path)
    args = parser.parse_args()
    if args.command == 'review':
        print(json.dumps(review(args.module, int(args.rva, 0), int(args.size, 0)), indent=1))
    else:
        stage(args.plan)


if __name__ == '__main__':
    main()
