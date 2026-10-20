"""Derived byte coverage and shared research families; never acceptance or routing.

Only recovery.json classifies recovered functions. Optional function maps supply
untracked span leads, not proven missing functions. Research annotations identify
questions and references only. Ranked bytes are ceilings, not predicted gains.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path

from env import ROOT
from similar import SCOPE


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def key(row):
    return Path(row.get('target', row.get('module', ''))).name.upper(), int(row['rva'], 0)


def category(row):
    if row is None:
        return 'untracked_lead'
    if row['status'] == 'FUNCTION_MATCH':
        return 'strict'
    if row['status'] == 'LINKED_MATCH':
        return 'linked_strict'
    if row['status'] == 'CODEGEN_SIMILAR':
        return 'masked' if row.get('proof', {}).get('scope') == SCOPE else 'legacy_similar'
    if row['status'] == 'RECONSTRUCTED':
        return 'reconstructed_non_strict'
    raise ValueError(f"Unknown recovery status: {row['status']}")


def validate_intervals(rows, label):
    groups = defaultdict(list)
    for row in rows:
        module, rva = key(row)
        if rva < 0 or row['size'] <= 0:
            raise ValueError(f'Invalid interval in {label}')
        groups[module].append((rva, rva + row['size']))
    for module, spans in groups.items():
        ordered = sorted(spans)
        if any(a[1] > b[0] for a, b in zip(ordered, ordered[1:])):
            raise ValueError(f'Overlapping intervals in {label}: {module}')


def build(state, maps=(), annotations=None):
    rows = state['functions']
    validate_intervals(rows, 'recovery.json')
    recorded = {key(r): r for r in rows}
    spans = {}
    mapped_modules = set()
    for mapping in maps:
        module = mapping['module'].upper()
        if module in mapped_modules:
            raise ValueError(f'Multiple function maps for {module}')
        mapped_modules.add(module)
        entries = [dict(f, module=module) for f in mapping['functions']]
        validate_intervals(entries, 'function map')
        for f in entries:
            k = key(f)
            if k in recorded and recorded[k]['size'] != f['size']:
                raise ValueError(f'Stale function-map extent: {module}:{f["rva"]}')
            # No heuristic interval may overlap a different recorded body.
            if any(key(r)[0] == module and key(r) != k and
                   k[1] < key(r)[1] + r['size'] and key(r)[1] < k[1] + f['size'] for r in rows):
                raise ValueError(f'Function-map interval overlaps recorded contribution: {module}:{f["rva"]}')
            spans[k] = f
    modules = {}
    for module in sorted({key(r)[0] for r in rows} | mapped_modules):
        metrics = {c: dict(functions=0, bytes=0) for c in
                   ('strict', 'linked_strict', 'masked', 'legacy_similar', 'reconstructed_non_strict')}
        for r in rows:
            if key(r)[0] == module:
                bucket = metrics[category(r)]
                bucket['functions'] += 1
                bucket['bytes'] += r['size']
        unknown = [f for k, f in spans.items() if k[0] == module and k not in recorded]
        metrics['untracked_map_leads'] = (dict(functions=len(unknown), bytes=sum(f['size'] for f in unknown))
                                         if module in mapped_modules else None)
        metrics['map_scope'] = 'heuristic first-party prefix; may contain drafts and suspect extents' if module in mapped_modules else None
        modules[module] = metrics
    families = []
    for family in (annotations or {}).get('families', []):
        members, seen = [], set()
        for member in family['members']:
            k = key(member)
            if k in seen:
                raise ValueError(f'Duplicate family member: {family["id"]}')
            seen.add(k)
            r = recorded.get(k)
            f = r or spans.get(k)
            members.append(dict(module=k[0], rva=hex(k[1]), category=category(r),
                                bytes=f['size'] if f else None))
        pending = [m for m in members if m['category'] not in ('strict', 'linked_strict', 'masked')]
        families.append(dict(id=family['id'], kind=family['kind'],
                             pending_codegen_functions=len(pending),
                             pending_codegen_byte_ceiling=sum(m['bytes'] or 0 for m in pending),
                             unmeasured_members=sum(m['bytes'] is None for m in pending),
                             members=members, evidence=family['evidence'],
                             next_discriminator=family['next_discriminator']))
    families.sort(key=lambda f: (-f['pending_codegen_byte_ceiling'], f['id']))
    return dict(scope='Derived diagnostics only; recovery.json alone grants recovery state.',
                definitions=dict(strict='FUNCTION_MATCH raw contribution bytes including padding',
                                 linked_strict='LINKED_MATCH raw bytes at the original RVA in the natural link of the reviewed plan',
                                 masked='CODEGEN_SIMILAR with the exact masked proof scope; no strict gain',
                                 legacy_similar='older CODEGEN_SIMILAR diagnostics; not counted as masked equality',
                                 reconstructed_non_strict='RECONSTRUCTED rows; status does not prove exhaustive behavior',
                                 untracked_map_leads='absent from recovery.json; may already have private or calibration drafts',
                                 ranking='descending unresolved span bytes, no difficulty/probability/model score; overlapping families are not additive'),
                modules=modules, research_families=families)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--function-map', action='append', type=Path, default=[])
    parser.add_argument('--families', type=Path, default=ROOT / 'evidence/convergence_families.json')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    state_path = ROOT / 'recovery.json'
    inputs = [state_path, args.families, *args.function_map]
    snapshots = {p: p.read_bytes() for p in inputs}
    report = build(load(state_path), [load(p) for p in args.function_map], load(args.families))
    if any(p.read_bytes() != content for p, content in snapshots.items()):
        raise ValueError('Inputs changed while deriving report')
    report['inputs'] = {str(p.resolve()): hashlib.sha256(content).hexdigest() for p, content in snapshots.items()}
    encoded = json.dumps(report, indent=2) + '\n'
    if args.output:
        output = args.output.resolve()
        if not any(output.is_relative_to((ROOT / d).resolve()) for d in ('work', 'build')):
            parser.error('Derived reports belong in ignored work/ or build/')
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded, encoding='utf-8')
        print(output)
    else:
        print(encoded, end='')


if __name__ == '__main__':
    main()
