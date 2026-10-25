"""Derive an ENG1 causal reading graph; neither recovery nor behavioral proof.

Native call/relocation edges are instruction or initialized-cell facts. Assigning
them to heuristic function spans is a lead. Copied-host edges are separately
reviewed contracts. Generated output belongs in ignored work/.
"""
import argparse
from bisect import bisect_right
from collections import Counter
import json
from pathlib import Path
import sqlite3

from env import ROOT
from function_map import build as inventory
from match import digest
from pe import PE


def derive(output, build_report):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('Derived graph output must remain under work/')
    config_bytes = (ROOT / 'config/eng1/sources.json').read_bytes()
    recovery_bytes = (ROOT / 'recovery.json').read_bytes()
    config, recovery = json.loads(config_bytes), json.loads(recovery_bytes)
    image = PE(ROOT / 'assets/pc/HERCULES/ENG1.DLL')
    if digest(image.data) != config['target_sha256']:
        raise ValueError('ENG1 oracle identity changed')
    leads = inventory('ENG1.DLL')
    rows = {int(row['rva'], 0): dict(row) for row in config.get('functions', [])}
    rows.update({int(row['rva'], 0): row for row in recovery['functions']
                 if Path(row['target']).name.upper() == 'ENG1.DLL'})
    nodes = {int(row['rva'], 0): dict(rva=row['rva'], span_size=row['size'],
             scope='diagnostic_start', source=None) for row in leads['functions']}
    for rva, row in rows.items():
        nodes.setdefault(rva, dict(rva=hex(rva), scope='explicit_source_provider',
                                  span_size=row.get('size')))
        nodes[rva].update(source=row['source'], symbol=row['symbol'],
                          existing_evidence=row.get('status'),
                          implementation_status=row.get('implementation_status', 'recorded_reconstruction'),
                          behavioral_verification='unverified_unless_individual_receipt_states_otherwise')
    starts = sorted(nodes)
    end = int(leads['first_party_end'], 0)
    # Existing library evidence is interval evidence, including contributions
    # before the conventional first-party end marker.
    libraries = json.loads((ROOT / 'work/library_map/library_map.json').read_text())['contributions']['ENG1.DLL']
    library_spans = [(int(row['rva'], 0), int(row['rva'], 0) + row['size'])
                     for row in libraries if row['size'] > 16]

    def classification(rva):
        section = next((s for s in image.sections if s['virtual_address'] <= rva
                        < s['virtual_address'] + max(s['virtual_size'], s['raw_size'])), None)
        if section is None:
            return 'outside_image'
        if not section['characteristics'] & 0x20000000:
            return 'data'
        if any(lo <= rva < hi for lo, hi in library_spans) or rva >= end:
            return 'library_or_tail_code'
        return 'first_party_code_lead'

    edges = []
    db = sqlite3.connect('file:' + (ROOT / 'work/archaeology.sqlite').as_posix() + '?mode=ro', uri=True)
    try:
        for target, source, kind in db.execute(
                "select target_rva,source_rva,kind from refs where module=? and kind in ('linear_call','pe_highlow_pointer')",
                ('ENG1.DLL',)):
            if target <= 0:
                continue
            if kind == 'linear_call':
                try:
                    code = image.read_rva(source, 5)
                except ValueError:
                    continue
                if code[0] != 0xe8 or source + 5 + int.from_bytes(code[1:], 'little', signed=True) != target:
                    continue
                index = bisect_right(starts, source) - 1
                owner = starts[index] if index >= 0 and source < end else None
                edge = dict(kind='direct_call', from_rva=hex(owner) if owner is not None else None,
                            instruction_rva=hex(source), target_rva=hex(target),
                            confidence='native_call_verified_owner_span_lead')
            else:
                try:
                    value = int.from_bytes(image.read_rva(source, 4), 'little')
                except ValueError:
                    continue
                if value != image.image_base + target:
                    continue
                edge = dict(kind='relocated_pointer', from_rva=hex(source), target_rva=hex(target),
                            confidence='initialized_cell_verified_semantic_owner_unproved')
            edge['target_kind'] = classification(target)
            edges.append(edge)
    finally:
        db.close()
    links = []
    if build_report.exists():
        measured = json.loads(build_report.read_text())
        if measured['config_sha256'] != digest(config_bytes) or measured['recovery_sha256'] != digest(recovery_bytes):
            raise ValueError('Rebuild ENG1 before deriving current unresolved links')
        links = [dict(symbol=symbol, consumers=measured['undefined_consumers'].get(symbol, []),
                      scope='measured_link_dependency_not_native_identity') for symbol in measured['unresolved']]
    result = dict(scope='Derived causal reading graph; recovery.json alone decides function acceptance.',
                  oracle_sha256=digest(image.data), recovery_sha256=digest(recovery_bytes),
                  config_sha256=digest(config_bytes),
                  inventory_scope='Start/span leads, not a reviewed complete first-party inventory.',
                  roots=['0x2c180', '0x20300', '0x1f0f0', '0x1f320'],
                  function_nodes=[nodes[rva] for rva in starts], native_edges=edges,
                  unresolved_links=links,
                  limitations=['Indirect target sets and data ownership require consumer review.',
                               'Native edges do not prove reconstructed source behavior.',
                               'Library intervals are diagnostic historical-library evidence.'])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print(json.dumps(dict(nodes=len(nodes), edges=dict(Counter(e['kind'] for e in edges)),
                          unresolved=len(links), output=str(output))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/dependencies.json')
    parser.add_argument('--build-report', type=Path, default=ROOT / 'work/eng1_behavioral/build/report.json')
    args = parser.parse_args()
    derive(args.output, args.build_report)
