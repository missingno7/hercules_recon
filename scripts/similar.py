"""Integrator-only staging and regression of relocation-masked reconstructions.

stage PLAN     install a region source under calibration/, compile it once, and
               require relocation-masked equality with a consistent symbol map for
               every listed function; rows become CODEGEN_SIMILAR (diagnostic only).
verify         recompile every masked row's source and recheck it.

Plan: {"source": "calibration/<module>/<region>.c", "candidate": "...c", "module": "TITLE.DLL",
       "functions": [{"rva", "size", "symbol", "evidence"}]}
A region file must define exactly the functions recorded for it. Masked equality is
never FUNCTION_MATCH: only a natural link that places the real bytes can promote.
"""
import argparse
import hashlib
import json
import os
import sqlite3
from pathlib import Path

from env import ROOT
from match import compile_source, digest, verify_toolchain, DEFAULT_TOOLCHAIN
from coff import COFF
from masked import masked_compare

DB = ROOT / 'work/archaeology.sqlite'
STATE = ROOT / 'recovery.json'
SCOPE = 'relocation-masked diagnostic equality with consistent implied symbol map; not acceptance'
BOUNDARY = ('Span to the next 16-byte entry; extent proven by equal compiled /Gy contribution size, '
            'masked bytes, and absolute relocation offsets equal to target base relocations.')


def masked_identity(obj, symbol):
    section = COFF(obj).function(symbol)
    data = bytearray(section['data'])
    for r in section['relocations']:
        data[r['offset']:r['offset'] + 4] = b'\0\0\0\0'
    return hashlib.sha256(bytes(data)).hexdigest()


def defined_functions(obj):
    return {s['name'] for s in COFF(obj).symbols.values() if s['section'] > 0 and s['type'] & 0x20}


def interior_entries(db, module, rva, size):
    return db.execute("select count(*) from refs where module=? and target_rva>? and target_rva<? and "
                      "kind in ('linear_call','pe_highlow_pointer') and (source_rva<? or source_rva>=?)",
                      (module, rva, rva + size, rva, rva + size)).fetchone()[0]


def check_rows(rows, source):
    obj, _ = compile_source(ROOT / source, ['/O2'])
    db = sqlite3.connect(DB)
    expected = {r['symbol'] for r in rows}
    if defined_functions(obj) != expected:
        raise ValueError(f'{source} must define exactly {sorted(expected)}; found {sorted(defined_functions(obj))}')
    results = {}
    for r in rows:
        module = Path(r['target']).name.upper()
        rva, size = int(r['rva'], 16), r['size']
        if rva & 15 or (rva + size) & 15 or interior_entries(db, module, rva, size):
            raise ValueError(f"Extent problem for {r['id']}")
        m = masked_compare(obj, r['symbol'], ROOT / r['target'], rva, size)
        if not m['masked_equal']:
            raise ValueError(f"{r['id']} not masked equal: {m['differing_bytes']} bytes, {m['conflicts'][:2]}")
        results[r['id']] = dict(m, masked_sha256=masked_identity(obj, r['symbol']))
    return results


def write_state(state):
    temporary = STATE.with_name('recovery.json.similar-tmp')
    temporary.write_bytes((json.dumps(state, indent=2) + '\n').encode('utf-8'))
    os.replace(temporary, STATE)


def stage(plan_path):
    plan = json.loads(Path(plan_path).read_text())
    source = plan['source']
    if not (ROOT / source).resolve().is_relative_to((ROOT / 'calibration').resolve()):
        raise ValueError('Masked reconstructions belong under calibration/')
    db = sqlite3.connect(DB)
    module = plan['module']
    target_rel = Path(db.execute('select path from modules where name=?', (module,)).fetchone()[0]).as_posix()
    prefix = module.split('.')[0].lower().replace('hercules', 'exe')
    state_before = STATE.read_bytes()
    state = json.loads(state_before)
    taken = {(Path(r['target']).name.upper(), int(r['rva'], 16)): r for r in state['functions']}
    # Optional consolidation: rows of these masked sources move into `source`; the old files
    # are removed only after every moved row verifies in the new file.
    replaces = [s for s in plan.get('replaces', []) if s != source]
    for old in replaces:
        if not (ROOT / old).resolve().is_relative_to((ROOT / 'calibration').resolve()):
            raise ValueError('Only calibration sources can be replaced')
    moved = [r for r in state['functions'] if r.get('source') in replaces]
    if any(r['status'] != 'CODEGEN_SIMILAR' or r.get('proof', {}).get('scope') != SCOPE for r in moved):
        raise ValueError('Only masked rows can be moved')
    existing = [r for r in state['functions'] if r.get('source') == source]
    prior_evidence = {(r['rva'], r['symbol']): r['evidence'] for r in moved}
    rows = []
    for f in plan['functions']:
        key = (module.upper(), int(f['rva'], 16))
        if key in taken and taken[key].get('source') not in [source] + replaces:
            raise ValueError(f"{f['rva']} already tracked as {taken[key]['id']}")
    if {(r['rva'], r['symbol']) for r in moved} - {(f['rva'], f['symbol']) for f in plan['functions']}:
        raise ValueError('A replaced source has rows missing from the plan')
    for f in plan['functions']:
        name = f['symbol'].lstrip('_@').split('@')[0]
        if name.startswith(prefix + '_'):
            name = name[len(prefix) + 1:]
        evidence = f['evidence']
        if (f['rva'], f['symbol']) in prior_evidence:
            evidence = f"{evidence} Previously: {prior_evidence[(f['rva'], f['symbol'])]}"
        rows.append(dict(id=f'{prefix}.{name}', status='CODEGEN_SIMILAR',
                         name_status='provisional address label', target=target_rel,
                         target_sha256=digest((ROOT / target_rel).read_bytes()), rva=f['rva'], size=f['size'],
                         source=source, symbol=f['symbol'], flags=['/O2'], evidence=evidence,
                         boundary=BOUNDARY))
    keep = [r for r in existing if (r['rva'], r['symbol']) not in {(x['rva'], x['symbol']) for x in rows}]
    content = (ROOT / plan['candidate']).read_bytes().replace(b'\r\n', b'\n')
    # Worker lanes include shared calibration headers from candidates/<lane>/.
    installed = (ROOT / source).parent
    for header in sorted((ROOT / 'calibration').rglob('*.h')):
        lane_path = '../../' + header.relative_to(ROOT).as_posix()
        local = Path(os.path.relpath(header, installed)).as_posix()
        content = content.replace(f'"{lane_path}"'.encode(), f'"{local}"'.encode())
    destination = ROOT / source
    before = destination.read_bytes() if destination.exists() else None
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(content)
    try:
        results = check_rows(keep + rows, source)
        if STATE.read_bytes() != state_before:
            raise ValueError('recovery.json changed during staging')
    except BaseException:
        if before is None:
            destination.unlink()
        else:
            destination.write_bytes(before)
        raise
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    replaced = {r['id'] for r in existing} | {r['id'] for r in moved}
    state['functions'] = [r for r in state['functions'] if r['id'] not in replaced]
    for r in keep + rows:
        res = results[r['id']]
        r['link_symbols'] = res['symbols']
        r['proof'] = dict(scope=SCOPE, source_sha256=digest(content), masked_sha256=res['masked_sha256'],
                          toolchain=toolchain)
        state['functions'].append(r)
    write_state(state)
    for old in replaces:
        (ROOT / old).unlink(missing_ok=True)
    print(f'Staged {len(rows)} CODEGEN_SIMILAR rows ({len(keep)} retained) in {source}'
          + (f'; replaced {len(replaces)} sources' if replaces else ''))


def verify(report=None):
    state = json.loads(STATE.read_text())
    groups = {}
    for r in state['functions']:
        if r['status'] == 'CODEGEN_SIMILAR' and r.get('proof', {}).get('scope') == SCOPE:
            groups.setdefault(r['source'], []).append(r)
    failures, count = [], 0
    for source, rows in sorted(groups.items()):
        if digest((ROOT / source).read_bytes()) != rows[0]['proof']['source_sha256']:
            failures.append(f'{source}: source differs from recorded proof')
            continue
        try:
            results = check_rows(rows, source)
        except ValueError as error:
            failures.append(f'{source}: {error}')
            continue
        for r in rows:
            res = results[r['id']]
            if res['masked_sha256'] != r['proof']['masked_sha256'] or res['symbols'] != r['link_symbols']:
                failures.append(f"{r['id']}: masked identity or symbol map changed")
            count += 1
    print(f'{count} masked rows verified in {len(groups)} sources; {len(failures)} failures')
    for f in failures:
        print('  ' + f)
    if failures:
        raise SystemExit(1)


def summary(module, function_map=None):
    """Counters with explicit definitions (one source for reports and evidence)."""
    state = json.loads(STATE.read_text())
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper()]
    masked = [r for r in rows if r['status'] == 'CODEGEN_SIMILAR' and r.get('proof', {}).get('scope') == SCOPE]
    legacy = [r for r in rows if r['status'] == 'CODEGEN_SIMILAR' and r not in masked]
    exact = [r for r in rows if r['status'] == 'FUNCTION_MATCH']
    out = dict(module=module,
               definitions=dict(masked_rows='CODEGEN_SIMILAR rows with the relocation-masked proof scope',
                                legacy_similar_rows='older CODEGEN_SIMILAR calibration rows (relocation-only diagnostics)',
                                function_match_rows='raw-exact accepted rows',
                                covered='masked + legacy similar + function match rows (one row per function)'),
               masked_rows=len(masked), masked_bytes=sum(r['size'] for r in masked),
               legacy_similar_rows=len(legacy), legacy_similar_bytes=sum(r['size'] for r in legacy),
               function_match_rows=len(exact), function_match_bytes=sum(r['size'] for r in exact))
    covered = masked + legacy + exact
    out['covered_functions'] = len({(r['target'], r['rva']) for r in covered})
    out['covered_bytes'] = sum(r['size'] for r in covered)
    if function_map and Path(function_map).exists():
        spans = json.loads(Path(function_map).read_text())['functions']
        out['first_party_spans'] = len(spans)
        out['first_party_bytes'] = sum(f['size'] for f in spans)
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    s = sub.add_parser('stage')
    s.add_argument('plan', type=Path)
    sub.add_parser('verify')
    m = sub.add_parser('summary')
    m.add_argument('module', nargs='?', default='TITLE.DLL')
    m.add_argument('--function-map', default=str(ROOT / 'work/function_map/title_v3.json'))
    args = parser.parse_args()
    if args.command == 'stage':
        stage(args.plan)
    elif args.command == 'summary':
        print(json.dumps(summary(args.module, args.function_map), indent=1))
    else:
        verify()


if __name__ == '__main__':
    main()
