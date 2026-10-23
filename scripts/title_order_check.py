"""Check the narrow constrained-order pilot. No recovery writes or whole-game logic claim."""
import argparse
from collections import Counter
import json
from pathlib import Path

from env import ROOT
from coff import COFF
from match import digest
from pe import PE
from title_integration import load_report
from title_link import read_map
from linked_compare import compare_function, verify_direct_call_region

PILOT = ('_title_04410', '_title_04610', '_title_04710')


def check(baseline_path, ordered_path):
    state_bytes = (ROOT / 'recovery.json').read_bytes()
    rows = [r for r in json.loads(state_bytes)['functions'] if r['target'].endswith('/TITLE.DLL')]
    by_symbol = {r['symbol']: r for r in rows}
    native_starts = {name: int(r['rva'], 0) for name, r in by_symbol.items()}
    snapshots = {ROOT / 'recovery.json': digest(state_bytes)}
    builds = []
    for path in (baseline_path, ordered_path):
        path = path.resolve()
        snapshots[path] = digest(path.read_bytes())
        report, dll, _ = load_report(path)
        if report.get('unresolved') or report.get('duplicates') or report.get('incomplete_functions'):
            raise ValueError('Pilot needs complete source-only builds')
        if any('/FORCE' in a.upper() for a in report['link_command']):
            raise ValueError('Forced link cannot prove the pilot')
        map_path = ROOT / report['link_map']
        snapshots[map_path] = report['link_map_sha256']
        snapshots[dll] = report['dll_sha256']
        objects = {}
        for unit in report['compiled_sources']:
            obj = Path(next(a[3:] for a in unit['command'] if a.startswith('/Fo')))
            snapshots[obj] = unit['object_sha256']
            compiler_input = Path(unit['command'][-1])
            snapshots[compiler_input] = unit['compiler_input_sha256']
            if unit['source'] in objects:
                raise ValueError('Duplicate compiled source')
            objects[unit['source']] = COFF(obj)
        pe = PE(dll)
        mapping = {name: va-pe.image_base for name, va in read_map(map_path).items()}
        builds.append((report, objects, pe, mapping))
    base, ordered = builds
    if base[0].get('layout_constraint') or not ordered[0].get('layout_constraint', {}).get('mapped_order_verified'):
        raise ValueError('Expected unconstrained baseline and verified ordered build')
    if base[1].keys() != ordered[1].keys():
        raise ValueError('Source graph changed')
    for source, a in base[1].items():
        b = ordered[1][source]
        # COFF storage-class FILE auxiliary records encode the private source path.
        # They are reported as excluded metadata; all emitted sections stay literal.
        symbols = lambda c: [s for s in c.symbols.values() if s['storage'] != 103]
        if a.sections != b.sections or symbols(a) != symbols(b):
            raise ValueError('Compiled code/data/symbol contribution changed: ' + source)
    original_path = ROOT / rows[0]['target']
    snapshots[original_path] = rows[0]['target_sha256']
    original = PE(original_path)
    summaries, pilot = [], []
    for report, objects, pe, mapping in builds:
        measured = []
        entries = sorted({mapping[s['name']] for obj in objects.values() for s in obj.symbols.values()
                          if s['type'] & 0x20 and s['section'] > 0 and s['name'] in mapping})
        for entry in report['functions']:
            name = entry['symbol']
            section = objects[entry['source']].function(name)
            rva = mapping[name]
            native = int(entry['rva'], 0)
            size = by_symbol[name]['size'] if name in by_symbol else entry['inventory_size']
            result = compare_function(section, pe, original, rva, native, size,
                                      next((a for a in entries if a > rva), None))
            measured.append(dict(symbol=name, **result))
            if name in by_symbol and by_symbol[name]['status'] == 'FUNCTION_MATCH' and not result['raw_equal']:
                raise ValueError('Accepted TITLE peer regressed: ' + name)
            if report is ordered[0] and name in PILOT:
                if not result['raw_equal_at_original_rva']:
                    raise ValueError('Pilot is not raw-equal at its native entry')
                calls = verify_direct_call_region(section, pe.read_rva(rva, size), original.read_rva(native, size),
                                                  native, native_starts, mapping)
                pilot.append(dict(symbol=name, rva=hex(native), bytes=size, direct_references=calls,
                                  linked_sha256=result['linked_sha256'], target_sha256=result['target_sha256']))
        summaries.append(dict(categories=dict(Counter(r['category'] for r in measured)),
            raw_equal_bytes=sum(r['target_size'] for r in measured if r['raw_equal']),
            same_rva=sum(r['same_rva'] for r in measured),
            raw_equal_at_original_rva=sum(r['raw_equal_at_original_rva'] for r in measured),
            raw_equal_symbols=[r['symbol'] for r in measured if r['raw_equal']]))
    if {r['symbol'] for r in pilot} != set(PILOT):
        raise ValueError('Incomplete pilot coverage')
    for path, expected in snapshots.items():
        if digest(path.read_bytes()) != expected:
            raise ValueError('Stale or changed input: ' + str(path))
    load_report(baseline_path)
    load_report(ordered_path)
    return dict(scope='Constrained source-only link diagnostic; no acceptance or whole-game semantic certificate',
        recovery_sha256=digest(state_bytes), compiled_units_preserved=len(base[1]),
        preservation='Complete section contents/relocations and non-FILE symbols equal; timestamps and FILE path metadata excluded',
        baseline=summaries[0], ordered=summaries[1], pilot=pilot,
        limits=['Pilot certifies instruction bytes and direct callee identities, not the callees implementations.',
                'Behavior depending on numeric code addresses outside this region is not certified.',
                'No source steering, binary rewriting or runtime execution in this experiment.'],
        inputs={str(p.relative_to(ROOT)).replace('\\', '/'): h for p, h in snapshots.items()})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('baseline', type=Path)
    parser.add_argument('ordered', type=Path)
    args = parser.parse_args()
    result = check(args.baseline, args.ordered)
    output = args.ordered.resolve().parent / 'order_pilot.json'
    output.write_bytes((json.dumps(result, indent=2)+'\n').encode())
    print(json.dumps({k: result[k] for k in ('compiled_units_preserved', 'baseline', 'ordered', 'pilot')}, indent=2))
    print('Diagnostic only:', output)
