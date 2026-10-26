"""Rank the reference blockers of pending instruction-equal rows (planning only).

Freshly compiles each pending reference-only row's recorded source profile, lists
its COFF relocation targets and classifies them against the current ledger and
the identity records of accepted binding documents. It then ranks unresolved
symbols by how many rows each would unlock. The implied native destination is
recorded for diagnosis only: a pending caller's own operand never establishes
identity. Output stays under work/ and grants no acceptance.

    python scripts/blocker_queue.py --census work/realignment/census/recorded.json --output work/blockers/queue.json
"""
import argparse
import collections
import glob
import json
import struct
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from env import ROOT
from coff import COFF
from match import compile_source
from pe import PE

ACCEPTED = {'FUNCTION_MATCH', 'FUNCTION_MATCH_RELOCATED', 'LINKED_MATCH'}
MODULES = {'eng1': 'ENG1.DLL', 'eng3': 'ENG3.DLL', 'title': 'TITLE.DLL', 'exe': 'HERCULES.EXE'}
EASY = {'function_accepted', 'function_shape_equal', 'local', 'import', 'known'}


def known_identities(ledger):
    """(module, COFF name) pairs already bound in a binding document of an accepted row."""
    status = {r['id']: r['status'] for r in ledger}
    known = set()
    for path in glob.glob(str(ROOT / 'evidence/*.json')):
        try:
            document = json.loads(Path(path).read_text(encoding='utf-8'))
        except (OSError, UnicodeError, json.JSONDecodeError):
            continue
        functions = document.get('functions') if isinstance(document, dict) else None
        if not isinstance(functions, dict):
            continue
        for fid, binding in functions.items():
            if isinstance(binding, dict) and status.get(fid) in ACCEPTED:
                for name in binding.get('symbols', {}):
                    known.add((MODULES.get(fid.split('.')[0]), name))
    return known


def zone(pe, rva):
    for s in pe.sections:
        if s['virtual_address'] <= rva < s['virtual_address'] + s['virtual_size']:
            return 'file' if rva < s['virtual_address'] + s['raw_size'] else 'zero'
    return 'none'


def classify_group(rows, ledger_rows, by_symbol, category, known, images):
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    source, flags = rows[0]['source'], rows[0]['flags']
    try:
        coff = COFF(compile_source(ROOT / source, list(flags))[0])
    except Exception as exc:  # report, never guess
        return [dict(id=r['id'], error=str(exc)[:200]) for r in rows]
    out = []
    for r in rows:
        led = ledger_rows[r['id']]
        pe = images[led['target']]
        rva, size = int(led['rva'], 16), led['size']
        try:
            section = coff.function(r['symbol'])
        except ValueError as exc:
            out.append(dict(id=r['id'], error=str(exc)[:200]))
            continue
        number = coff.sections.index(section) + 1
        native = pe.read_rva(rva, size)
        indirect = sum(1 for i in decoder.disasm(native, rva) if i.mnemonic in ('call', 'jmp')
                       and not i.op_str.startswith('0x') and '*4' not in i.op_str)
        refs = []
        for rel in section['relocations']:
            name, off, typ = rel['symbol'], rel['offset'], rel['type']
            sym = next(s for s in coff.symbols.values() if s['name'] == name)
            addend = struct.unpack_from('<i', section['data'], off)[0]
            if typ == 20:
                dest = (rva + off + 4 + struct.unpack_from('<i', native, off)[0]) & 0xffffffff
            else:
                dest = (struct.unpack_from('<I', native, off)[0] - pe.image_base) & 0xffffffff
            dest -= addend
            module = r['module']
            if (module, name) in known:
                kind = 'known'
            elif name.startswith('__imp_'):
                kind = 'import'
            elif sym['section'] == number:
                kind = 'local'
            elif sym['type'] & 0x20:
                callees = by_symbol.get((led['target'], name), [])
                if len(callees) != 1:
                    kind = 'function_unknown' if not callees else 'function_ambiguous'
                elif int(callees[0]['rva'], 16) != dest:
                    kind = 'function_mismatch'
                elif callees[0]['status'] in ACCEPTED:
                    kind = 'function_accepted'
                elif category.get(callees[0]['id']) in ('reference_only', 'raw_equal'):
                    kind = 'function_shape_equal'
                else:
                    kind = 'function_near'
            elif sym['section'] == 0:
                kind = 'data_' + zone(pe, dest) + ('_common' if sym['value'] else '')
            else:
                own = coff.sections[sym['section'] - 1]
                kind = 'data_same_unit' + ('_relocated' if own['relocations'] else '')
            refs.append(dict(symbol=name, kind=kind, offset=off, type=typ, addend=addend,
                             implied_destination_rva=dest))
        out.append(dict(id=r['id'], module=r['module'], size=size, source=source,
                        indirect_transfers=indirect, refs=refs))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--limit', type=int, default=40)
    args = parser.parse_args()
    if not args.output.resolve().is_relative_to((ROOT / 'work').resolve()):
        parser.error('Planning output belongs under work/')
    ledger = json.loads((ROOT / 'recovery.json').read_bytes())['functions']
    ledger_rows = {r['id']: r for r in ledger}
    census = json.loads(args.census.read_bytes())
    category = {r['id']: r['category'] for r in census['rows']}
    by_symbol = collections.defaultdict(list)
    for r in ledger:
        if r.get('symbol') and r.get('rva'):
            by_symbol[(r['target'], r['symbol'])].append(r)
    known = known_identities(ledger)
    pending = [r for r in census['rows'] if r['category'] == 'reference_only'
               and ledger_rows.get(r['id'], {}).get('status') == 'CODEGEN_SIMILAR']
    groups = collections.defaultdict(list)
    for r in pending:
        groups[(r['source'], tuple(r['flags']))].append(r)
    images = {t: PE(ROOT / t) for t in {ledger_rows[r['id']]['target'] for r in pending}}
    with ThreadPoolExecutor(args.jobs) as pool:
        rows = [x for chunk in pool.map(lambda g: classify_group(g, ledger_rows, by_symbol, category, known, images),
                                        groups.values()) for x in chunk]
    unlock, unlock_bytes, single = collections.Counter(), collections.Counter(), collections.Counter()
    for row in rows:
        hard = sorted({(row['module'], x['symbol'], x['kind']) for x in row.get('refs', []) if x['kind'] not in EASY})
        row['blockers'] = [dict(symbol=s, kind=k) for _, s, k in hard]
        if row.get('indirect_transfers') or not hard:
            continue
        for key in hard:
            unlock[key] += 1 / len(hard)
            unlock_bytes[key] += row['size']
        if len(hard) == 1:
            single[hard[0]] += 1
    ready = [r['id'] for r in rows if 'error' not in r and not r['blockers'] and not r['indirect_transfers']]
    ranking = [dict(module=m, symbol=s, kind=k, weighted_rows=round(v, 2), sole_blocker_rows=single[(m, s, k)],
                    bytes=unlock_bytes[(m, s, k)]) for (m, s, k), v in unlock.most_common(args.limit)]
    report = dict(scope='Planning only; implied destinations are diagnostics, never identity. recovery.json is the authority.',
                  pending=len(pending), compile_groups=len(groups),
                  ready_without_new_identity=ready, ranking=ranking, rows=rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    print(f'{len(pending)} pending rows, {len(groups)} compiles, {len(ready)} ready without a new identity')
    for item in ranking[:15]:
        print(f"  {item['weighted_rows']:6.1f} rows {item['bytes']:6d} B  {item['module']} {item['symbol']} ({item['kind']})")


if __name__ == '__main__':
    main()
