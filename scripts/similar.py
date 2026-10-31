"""Integrator-only staging and regression of relocation-masked reconstructions.

stage PLAN     install a region source under src/, compile it once, and
               require relocation-masked equality with a consistent symbol map for
               every listed function; rows become CODEGEN_SIMILAR (diagnostic only).
verify         recompile every masked row's source and recheck it.

Plan: {"source": "src/<module>/<region>.c", "candidate": "...c", "module": "TITLE.DLL", "flags" (default ["/O2"]),
       "functions": [{"rva", "size", "symbol", "evidence"}]}
A region file must define exactly the functions recorded for it. An existing provider can instead be
staged in place ("in_place": true, no candidate): it is never rewritten, and every other function it
defines is pinned by name in each row's proof (co_defined_functions); verify requires that exact set. Masked equality is
never acceptance: match.py and promote.py verify raw bytes or independently bound references.

In-place plans replace planned rows by id, so a reviewed rename of a masked row's symbol restages that row.
"moves": [row ids] re-sources individual masked rows of the plan's module from another provider; each
must be listed in the plan with its id and RVA. The rows left in the old provider group are rechecked in
the same transaction, and their pinned co-defined set then includes the moved symbol. A strict row only
blocks a planned symbol in its own module: another module may stage a masked row on the same shared source.
"""
import argparse
import hashlib
import json
import os
import sqlite3
import time
from pathlib import Path

from env import ROOT
from match import compile_source, compare, digest, verify_toolchain, DEFAULT_TOOLCHAIN
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


def check_rows(rows, source, co_defined=()):
    # The complete oracle identity belongs to each row, even when a changed
    # image happens to retain these function bytes. Masked equality cannot
    # establish that identity by itself.
    source_path = ROOT / source
    source_before = digest(source_path.read_bytes())
    targets = {}
    for r in rows:
        target = ROOT / r['target']
        expected = r['target_sha256']
        if target in targets and targets[target] != expected:
            raise ValueError(f'Conflicting oracle identities for {target}')
        targets[target] = expected
    for target, expected in targets.items():
        if digest(target.read_bytes()) != expected:
            raise ValueError(f'Target hash mismatch: {target}')
    flag_sets = {tuple(r.get('flags') or ['/O2']) for r in rows}
    if len(flag_sets) != 1:
        raise ValueError(f'{source}: rows record different compiler flags {sorted(flag_sets)}')
    obj, _ = compile_source(ROOT / source, list(next(iter(flag_sets))))
    if digest(source_path.read_bytes()) != source_before:
        raise ValueError('Source changed during compilation')
    db = sqlite3.connect(DB)
    expected = {r['symbol'] for r in rows}
    defined = defined_functions(obj)
    if not expected <= defined or defined - expected != set(co_defined):
        raise ValueError(f'{source} must define exactly {sorted(expected)} plus pinned co-defined '
                         f'{sorted(co_defined)}; found {sorted(defined)}')
    results = {}
    for r in rows:
        module = Path(r['target']).name.upper()
        rva, size = int(r['rva'], 16), r['size']
        # /O2 /Gy contributions start and end on 16-byte boundaries; unoptimized (/Od) units pack
        # functions back to back at unaligned addresses, so only the interior-entry check applies.
        aligned = '/Od' not in (r.get('flags') or [])
        if (aligned and (rva & 15 or (rva + size) & 15)) or interior_entries(db, module, rva, size):
            raise ValueError(f"Extent problem for {r['id']}")
        m = masked_compare(obj, r['symbol'], ROOT / r['target'], rva, size)
        if not m['masked_equal']:
            raise ValueError(f"{r['id']} not masked equal: {m['differing_bytes']} bytes, {m['conflicts'][:2]}")
        if r.get('status') == 'FUNCTION_MATCH' and not compare(obj, r['symbol'], ROOT / r['target'], rva, size)['exact']:
            raise ValueError('Raw peer regressed: ' + r['id'])
        results[r['id']] = dict(m, masked_sha256=masked_identity(obj, r['symbol']))
    for target, expected in targets.items():
        if digest(target.read_bytes()) != expected:
            raise ValueError(f'Target changed during verification: {target}')
    if digest(source_path.read_bytes()) != source_before:
        raise ValueError('Source changed during verification')
    return results


def write_state(state):
    temporary = STATE.with_name('recovery.json.similar-tmp')
    temporary.write_bytes((json.dumps(state, indent=2) + '\n').encode('utf-8'))
    for attempt in range(20):
        try:
            os.replace(temporary, STATE)
            return
        except OSError:
            # Windows refuses the replace while another process holds the file open for reading.
            if attempt == 19:
                raise
            time.sleep(0.5)


def stage(plan_path):
    plan = json.loads(Path(plan_path).read_text())
    source = plan['source']
    in_place = bool(plan.get('in_place'))
    if in_place:
        # Existing provider recorded where it already lives (no copy, no rewrite).
        if not (ROOT / source).resolve().is_relative_to((ROOT / 'src').resolve()):
            raise ValueError('In-place masked providers belong under src/')
        if plan.get('candidate') or plan.get('replaces'):
            raise ValueError('In-place staging takes no candidate or replaced sources')
    elif not (ROOT / source).resolve().is_relative_to((ROOT / 'src').resolve()):
        raise ValueError('Masked reconstructions belong under src/')
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
        if not (ROOT / old).resolve().is_relative_to((ROOT / 'src').resolve()):
            raise ValueError('Only canonical src/ sources can be replaced')
    moved = [r for r in state['functions'] if r.get('source') in replaces]
    if any(r['status'] != 'CODEGEN_SIMILAR' or r.get('proof', {}).get('scope') != SCOPE for r in moved):
        raise ValueError('Only masked rows can be moved')
    explicit = _explicit_moves(plan, state, source, module, in_place)
    plan_flags = list(plan.get('flags') or ['/O2'])
    existing = [r for r in state['functions'] if r.get('source') == source
                and list(r.get('flags') or ['/O2']) == plan_flags]
    if in_place:
        # RECONSTRUCTED rows of the same provider stay as they are; their functions are pinned co-defined.
        # A planned function that is RECONSTRUCTED now replaces that row once it verifies masked equal.
        planned = {f['symbol'] for f in plan['functions']}
        # Legacy CODEGEN_SIMILAR rows (recorded before masked proofs existed) are upgraded the same way.
        legacy = lambda r: r['status'] == 'CODEGEN_SIMILAR' and r.get('proof', {}).get('scope') != SCOPE
        upgraded = [r for r in existing if (r['status'] == 'RECONSTRUCTED' or legacy(r)) and r['symbol'] in planned]
        existing = [r for r in existing if r not in upgraded]
        # Strict rows (FUNCTION_MATCH, LINKED_MATCH) keep their own proofs; like RECONSTRUCTED rows their
        # functions are pinned co-defined, and they are never restaged as masked rows.
        strict = [r for r in existing if r['status'] in ('FUNCTION_MATCH', 'FUNCTION_MATCH_RAW', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH')]
        # A strict row protects its own module's symbol; a twin module may stage a masked row on shared source.
        if {r['symbol'] for r in strict if Path(r['target']).name.upper() == module.upper()} & planned:
            raise ValueError('A strict row cannot be staged as a masked row')
        existing = [r for r in existing if r['status'] not in ('RECONSTRUCTED', 'FUNCTION_MATCH', 'FUNCTION_MATCH_RAW', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH')]
        if any(r['status'] != 'CODEGEN_SIMILAR' or r.get('proof', {}).get('scope') != SCOPE for r in existing):
            raise ValueError('In-place staging only extends sources whose recorded rows are masked or reconstructed rows')
        for r in upgraded:
            taken.pop((Path(r['target']).name.upper(), int(r['rva'], 16)), None)
        moved = moved + upgraded
    prior_evidence = {(r['rva'], r['symbol']): r['evidence'] for r in moved}
    rows = []
    moved_ids = {r['id'] for r in explicit}
    for f in plan['functions']:
        key = (module.upper(), int(f['rva'], 16))
        if key in taken and taken[key]['id'] in moved_ids:
            if f.get('id') != taken[key]['id']:
                raise ValueError('A moved row keeps its id and RVA: ' + taken[key]['id'])
        elif key in taken and taken[key].get('source') not in [source] + replaces:
            raise ValueError(f"{f['rva']} already tracked as {taken[key]['id']}")
    by_id = {r['id']: r for r in existing}
    for f in plan['functions']:
        if f.get('id') in by_id and by_id[f['id']]['rva'] != f['rva']:
            raise ValueError('A restaged row keeps its RVA: ' + f['id'])
    if {(r['rva'], r['symbol']) for r in moved} - {(f['rva'], f['symbol']) for f in plan['functions']}:
        raise ValueError('A replaced source has rows missing from the plan')
    for f in plan['functions']:
        name = f['symbol'].lstrip('_@').split('@')[0]
        if name.startswith(prefix + '_'):
            name = name[len(prefix) + 1:]
        evidence = f['evidence']
        if (f['rva'], f['symbol']) in prior_evidence:
            evidence = f"{evidence} Previously: {prior_evidence[(f['rva'], f['symbol'])]}"
        rows.append(dict(id=f.get('id') or f'{prefix}.{name}', status='CODEGEN_SIMILAR',
                         name_status='provisional address label', target=target_rel,
                         target_sha256=digest((ROOT / target_rel).read_bytes()), rva=f['rva'], size=f['size'],
                         source=source, symbol=f['symbol'], flags=list(plan.get('flags') or ['/O2']), evidence=evidence,
                         boundary=BOUNDARY))
    # Planned rows replace existing rows by id too, so a reviewed symbol rename restages its row.
    planned_ids = {x['id'] for x in rows}
    keep = [r for r in existing if (r['rva'], r['symbol']) not in {(x['rva'], x['symbol']) for x in rows}
            and r['id'] not in planned_ids]
    if in_place:
        content = (ROOT / source).read_bytes()
        if b'\r' in content:
            raise ValueError('In-place provider must already be LF text')
        obj, _ = compile_source(ROOT / source, list(plan.get('flags') or ['/O2']))
        co_defined = sorted(defined_functions(obj) - {r['symbol'] for r in keep + rows})
        results = check_rows(keep + rows, source, co_defined)
        old_groups = _remaining_groups(state, explicit)
        if STATE.read_bytes() != state_before or (ROOT / source).read_bytes() != content:
            raise ValueError('recovery.json or source changed during staging')
        _record(state, existing, moved + explicit, keep, rows, results, content, co_defined, old_groups)
        print(f'Staged {len(rows)} CODEGEN_SIMILAR rows ({len(keep)} retained) in place in {source}; '
              f'{len(co_defined)} pinned co-defined functions'
              + (f'; moved {sorted(moved_ids)}; rechecked {len(old_groups)} old provider groups' if explicit else ''))
        return
    content =(ROOT / plan['candidate']).read_bytes().replace(b'\r\n', b'\n')
    # Worker lanes include canonical headers from candidates/<lane>/.
    installed = (ROOT / source).parent
    for header in sorted((ROOT / 'src').rglob('*.h')):
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
    _record(state, existing, moved, keep, rows, results, content, [])
    for old in replaces:
        (ROOT / old).unlink(missing_ok=True)
    print(f'Staged {len(rows)} CODEGEN_SIMILAR rows ({len(keep)} retained) in {source}'
          + (f'; replaced {len(replaces)} sources' if replaces else ''))


def _explicit_moves(plan, state, source, module, in_place):
    """Masked rows of this module re-sourced individually from another provider (plan "moves")."""
    moves = plan.get('moves', [])
    if not isinstance(moves, list) or len(set(moves)) != len(moves) or any(not isinstance(m, str) for m in moves):
        raise ValueError('moves must list distinct row ids')
    if moves and not in_place:
        raise ValueError('Explicit masked-row moves need an in-place provider')
    planned = {f.get('id'): f['rva'] for f in plan['functions']}
    explicit = []
    for row_id in moves:
        row = next((r for r in state['functions'] if r['id'] == row_id), None)
        if (row is None or row['status'] != 'CODEGEN_SIMILAR' or row.get('proof', {}).get('scope') != SCOPE or
                Path(row['target']).name.upper() != module.upper() or row.get('source') == source or
                planned.get(row_id) != row['rva']):
            raise ValueError('A move names a masked row of this module tracked by another provider and '
                             'listed in the plan: ' + row_id)
        explicit.append(row)
    return explicit


def _remaining_groups(state, explicit):
    """Recheck each old provider group left behind by explicit moves; return (rows, co_defined) pairs.

    The remaining masked rows keep their masked identity, symbol map and source pin; only their pinned
    co-defined set grows by the moved symbols, exactly as in-place staging would record it.
    """
    groups = []
    moved_ids = {r['id'] for r in explicit}
    for old_source, flags in sorted({(r['source'], tuple(r.get('flags') or ['/O2'])) for r in explicit}):
        remaining = [r for r in state['functions'] if r.get('source') == old_source and r['id'] not in moved_ids
                     and r['status'] == 'CODEGEN_SIMILAR' and r.get('proof', {}).get('scope') == SCOPE
                     and tuple(r.get('flags') or ['/O2']) == flags]
        if not remaining:
            continue
        content = (ROOT / old_source).read_bytes()
        obj, _ = compile_source(ROOT / old_source, list(flags))
        co_defined = sorted(defined_functions(obj) - {r['symbol'] for r in remaining})
        results = check_rows(remaining, old_source, co_defined)
        for r in remaining:
            res = results[r['id']]
            if (res['masked_sha256'] != r['proof']['masked_sha256'] or res['symbols'] != r['link_symbols'] or
                    digest(content) != r['proof']['source_sha256']):
                raise ValueError('A row left in the old provider changed: ' + r['id'])
        groups.append((remaining, co_defined))
    return groups


def _record(state, existing, moved, keep, rows, results, content, co_defined, old_groups=()):
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    replaced = {r['id'] for r in existing} | {r['id'] for r in moved}
    state['functions'] = [r for r in state['functions'] if r['id'] not in replaced]
    for r in keep + rows:
        res = results[r['id']]
        r['link_symbols'] = res['symbols']
        r['proof'] = dict(scope=SCOPE, source_sha256=digest(content), masked_sha256=res['masked_sha256'],
                          toolchain=toolchain)
        if co_defined:
            r['proof']['co_defined_functions'] = list(co_defined)
        state['functions'].append(r)
    for remaining, pinned in old_groups:
        for r in remaining:
            if pinned:
                r['proof']['co_defined_functions'] = list(pinned)
            else:
                r['proof'].pop('co_defined_functions', None)
    write_state(state)


def verify(report=None):
    state = json.loads(STATE.read_text())
    groups = {}
    for r in state['functions']:
        if r['status'] == 'CODEGEN_SIMILAR' and r.get('proof', {}).get('scope') == SCOPE:
            # One compilation per (source, flags): a source built with different preprocessor/flag
            # settings for two modules (e.g. /Od vs /Od /DENGINE3) is two compilations.
            groups.setdefault((r['source'], tuple(r.get('flags') or ['/O2'])), []).append(r)
    failures, count = [], 0
    for (source, _flags), rows in sorted(groups.items()):
        if digest((ROOT / source).read_bytes()) != rows[0]['proof']['source_sha256']:
            failures.append(f'{source}: source differs from recorded proof')
            continue
        try:
            raw_peers = [r for r in state['functions'] if r.get('source') == source and r['status'] == 'FUNCTION_MATCH' and list(r.get('flags') or ['/O2']) == rows[0]['flags']]
            co_defined = set(rows[0]['proof'].get('co_defined_functions', ())) - {r['symbol'] for r in raw_peers}
            results = check_rows(rows + raw_peers, source, co_defined)
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
    relocated = [r for r in rows if r['status'] == 'FUNCTION_MATCH_RELOCATED']
    linked = [r for r in rows if r['status'] == 'LINKED_MATCH']
    recovered = [r for r in rows if r['status'] == 'RECONSTRUCTED']
    out = dict(module=module,
               definitions=dict(masked_rows='CODEGEN_SIMILAR rows with the relocation-masked proof scope',
                                legacy_similar_rows='older CODEGEN_SIMILAR calibration rows (relocation-only diagnostics)',
                                function_match_rows='raw-exact accepted rows',
                                covered='all recorded source extents; not an exhaustive first-party census'),
               masked_rows=len(masked), masked_bytes=sum(r['size'] for r in masked),
               legacy_similar_rows=len(legacy), legacy_similar_bytes=sum(r['size'] for r in legacy),
               function_match_rows=len(exact), function_match_bytes=sum(r['size'] for r in exact),
               relocated_rows=len(relocated), relocated_bytes=sum(r['size'] for r in relocated),
               linked_rows=len(linked), linked_bytes=sum(r['size'] for r in linked),
               recovered_rows=len(recovered), recovered_bytes=sum(r['size'] for r in recovered))
    covered = rows
    out['covered_functions'] = len({(r['target'], r['rva']) for r in covered})
    out['covered_bytes'] = sum(r['size'] for r in covered)
    if function_map and Path(function_map).exists():
        mapping = json.loads(Path(function_map).read_text())
        if mapping['module'].upper() != module.upper():
            raise ValueError('Function map belongs to a different module')
        spans = mapping['functions']
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
    m.add_argument('--function-map')
    args = parser.parse_args()
    if args.command == 'stage':
        stage(args.plan)
    elif args.command == 'summary':
        mapping = args.function_map
        if mapping is None and args.module.upper() == 'TITLE.DLL':
            mapping = ROOT / 'work/function_map/title_v4.json'
        print(json.dumps(summary(args.module, mapping), indent=1))
    else:
        verify()


if __name__ == '__main__':
    main()
