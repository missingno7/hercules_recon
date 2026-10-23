"""Inventory the complete TITLE source graph and measure recorded peers, without promotion.

Run title_behavioral.py --output work/LANE first, then pass work/LANE/report.json.
This reuses only that explicitly hash-checked build. It inventories every emitted
section/member and reports mixed outcomes instead of excluding an entire unit.
The existing raw/masked comparators and recovery.json remain authoritative.
"""
import argparse
from collections import Counter
import json
from pathlib import Path

from env import ROOT
from coff import COFF
from match import compare, digest
from masked import masked_compare
from title_integration import load_report
from title_link import read_map
from pe import PE
from linked_compare import compare_function, image_comparison


def audit(report_path, linked_bytes=False):
    report_path = report_path.resolve()
    report_bytes = report_path.read_bytes()
    report, dll, _ = load_report(report_path)
    state_bytes = (ROOT / 'recovery.json').read_bytes()
    state = json.loads(state_bytes)
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == 'TITLE.DLL']
    coverage = report['functions']
    linked_rows = []
    linked = original = None
    if linked_bytes:
        if report.get('unresolved') or report.get('duplicates') or report.get('incomplete_functions'):
            raise ValueError('Linked diagnostic requires a complete build without unresolved names or placeholders')
        if any('/FORCE' in arg.upper() for arg in report['link_command']):
            raise ValueError('Forced links cannot supply the linked-byte report')
        map_path = (ROOT / report['link_map']).resolve()
        if digest(map_path.read_bytes()) != report['link_map_sha256']:
            raise ValueError('Stale link map')
        mapping = read_map(map_path)
        linked, original = PE(dll), PE(ROOT / rows[0]['target'])
        if mapping.get('_PC_DLLEngineMain') != linked.image_base + report['exports'][0]['rva']:
            raise ValueError('Map/export identity mismatch')
    objects, frozen, units = {}, {}, []
    for compiled in report['compiled_sources']:
        command = compiled['command']
        obj = Path(next(arg[3:] for arg in command if arg.startswith('/Fo'))).resolve()
        compiler_input = Path(command[-1]).resolve()
        for path, expected in ((obj, compiled['object_sha256']),
                               (compiler_input, compiled['compiler_input_sha256'])):
            if digest(path.read_bytes()) != expected:
                raise ValueError('Stale build artifact: ' + str(path))
            frozen[path] = expected
        source = compiled['source']
        if source in objects:
            raise ValueError('Duplicate source in build report')
        objects[source] = obj
        coff = COFF(obj)
        members = [s for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20]
        expected_symbols = {r['symbol'] for r in coverage if r.get('source') == source and r.get('symbol')}
        actual_symbols = {s['name'] for s in members}
        if not expected_symbols.issubset(actual_symbols):
            raise ValueError('Build coverage names missing from object: ' + source)
        units.append(dict(source=source, object_sha256=compiled['object_sha256'],
                          members=[dict(symbol=s['name'], section=s['section'], offset=s['value'],
                                        mapped=s['name'] in expected_symbols) for s in members],
                          sections=[dict(number=n, name=s['name'], size=s['size'], flags=s['flags'],
                                         payload_sha256=digest(s['data']), relocations=s['relocations'])
                                    for n, s in enumerate(coff.sections, 1)],
                          outcomes=[], unrecorded_leads=[]))
    by_source = {u['source']: u for u in units}
    if linked_bytes:
        frozen[map_path] = report['link_map_sha256']
        frozen[dll] = report['dll_sha256']
    frozen.update({ROOT / r['target']: r['target_sha256'] for r in rows})
    for path, expected in frozen.items():
        if digest(path.read_bytes()) != expected:
            raise ValueError('Build input/oracle identity changed: ' + str(path))
    for row in rows:
        obj = objects[row['source']]
        raw = compare(obj, row['symbol'], ROOT / row['target'], int(row['rva'], 0), row['size'])
        masked = masked_compare(obj, row['symbol'], ROOT / row['target'], int(row['rva'], 0), row['size'])
        prior = row.get('proof', {})
        protected = row['status'] == 'FUNCTION_MATCH' or (
            row['status'] == 'CODEGEN_SIMILAR' and 'link_symbols' in row)
        preserved = raw['exact'] if row['status'] == 'FUNCTION_MATCH' else (
            masked['masked_equal'] and masked['symbols'] == row.get('link_symbols', {}))
        by_source[row['source']]['outcomes'].append(dict(
            id=row['id'], status=row['status'], symbol=row['symbol'], target_size=row['size'],
            compiled_size=raw['compiled_size'], raw_equal=raw['exact'],
            masked_equal=masked['masked_equal'], masked_different_bytes=masked['differing_bytes'],
            binding_conflicts=masked['conflicts'], recorded_bindings_equal=masked['symbols'] == row.get('link_symbols', {}),
            protected_peer=protected, protected_peer_preserved=preserved if protected else None,
            source_context_unchanged=digest((ROOT / row['source']).read_bytes()) == prior.get('source_sha256')
                and str((ROOT / row['source']).resolve()) == str(Path(next(
                    c['command'][-1] for c in report['compiled_sources'] if c['source'] == row['source'])).resolve())))
    recorded = {(r['source'], r['symbol']) for r in rows}
    for lead in coverage:
        key = (lead.get('source'), lead.get('symbol'))
        if key in recorded:
            continue
        source, symbol = key
        # Inventory spans of unrecorded source remain provisional even if the
        # complete object contribution happens to agree. Never promote them here.
        measured = masked_compare(objects[source], symbol, ROOT / rows[0]['target'],
                                  int(lead['rva'], 0), lead['inventory_size'])
        by_source[source]['unrecorded_leads'].append(dict(
            symbol=symbol, rva=lead['rva'], target_extent_provisional=True,
            target_size=lead['inventory_size'], compiled_size=measured['compiled_size'],
            masked_equal=measured['masked_equal'], differing_bytes=measured['differing_bytes'],
            binding_conflicts=measured['conflicts']))
    if linked_bytes:
        by_key = {(r['source'], r['symbol']): r for r in rows}
        entries = sorted({mapping[m['symbol']] - linked.image_base for u in units for m in u['members']
                          if m['symbol'] in mapping})
        coffs = {source: COFF(obj) for source, obj in objects.items()}
        for entry in coverage:
            source, symbol = entry['source'], entry['symbol']
            if symbol not in mapping:
                raise ValueError('Mapped function missing from linked image: ' + symbol)
            rva = mapping[symbol] - linked.image_base
            recorded_row = by_key.get((source, symbol))
            target_size = recorded_row['size'] if recorded_row else entry['inventory_size']
            measured = compare_function(coffs[source].function(symbol), linked, original, rva,
                                        int(entry['rva'], 0), target_size,
                                        next((address for address in entries if address > rva), None))
            linked_rows.append(dict(source=source, symbol=symbol,
                                    target_extent_provisional=recorded_row is None, **measured))
    for path, expected in frozen.items():
        if digest(path.read_bytes()) != expected:
            raise ValueError('Artifact changed during diagnostics: ' + str(path))
    load_report(report_path)
    if report_path.read_bytes() != report_bytes or (ROOT / 'recovery.json').read_bytes() != state_bytes:
        raise ValueError('Build report or recovery state changed during diagnostics')
    outcomes = [r for u in units for r in u['outcomes']]
    output = dict(scope='Complete source-graph diagnostic, not acceptance or proof of native layout; no state changes.',
                build_report=str(report_path.relative_to(ROOT)), build_report_sha256=digest(report_bytes),
                recovery_sha256=digest(state_bytes), dll_sha256=report['dll_sha256'],
                units=units, summary=dict(
                    sources=len(units), emitted_function_symbols=sum(len(u['members']) for u in units),
                    recorded_rows=len(outcomes), raw_equal=sum(r['raw_equal'] for r in outcomes),
                    unrecorded_leads=sum(len(u['unrecorded_leads']) for u in units),
                    masked_equal=sum(r['masked_equal'] for r in outcomes),
                    raw_regressions=[r['id'] for r in outcomes if r['status'] == 'FUNCTION_MATCH' and not r['raw_equal']],
                    masked_codegen_differences=[r['id'] for r in outcomes if r['status'] == 'CODEGEN_SIMILAR' and not r['masked_equal']],
                    binding_map_differences=[r['id'] for r in outcomes if r['protected_peer'] and r['masked_equal']
                                             and r['status'] == 'CODEGEN_SIMILAR' and not r['recorded_bindings_equal']],
                    statuses=dict(Counter(r['status'] for r in outcomes))))
    if linked_bytes:
        output['linked'] = dict(image=image_comparison(linked, original), functions=linked_rows,
            summary=dict(categories=dict(Counter(r['category'] for r in linked_rows)),
                         raw_equal_at_original_rva=sum(r['raw_equal_at_original_rva'] for r in linked_rows),
                         at_original_rva=sum(r['same_rva'] for r in linked_rows),
                         target_bytes_by_category={c: sum(r['target_size'] for r in linked_rows if r['category'] == c)
                                                   for c in sorted({r['category'] for r in linked_rows})}))
    return output, report_path.parent / ('linked_diagnostic.json' if linked_bytes else 'matching_diagnostic.json')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('--linked', action='store_true', help='also measure complete actual linked contributions and raw PE sections')
    args = parser.parse_args()
    result, output = audit(args.report, args.linked)
    output.write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print(json.dumps(result['summary'], indent=2))
    if args.linked:
        print(json.dumps(result['linked']['summary'], indent=2))
    print('Diagnostic only; report:', output)
