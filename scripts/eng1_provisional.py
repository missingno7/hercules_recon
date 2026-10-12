"""Synchronize explicit fail-on-entry source for a transitional ENG1 loader build.

This never reconstructs unknown behavior. Missing entries remain incomplete and
are excluded from implementation coverage. Every generated body reports its
confirmed original identity and terminates; no arguments or return are assumed.
"""
import argparse
import json
from pathlib import Path
import re

from env import ROOT
from match import digest
from pe import PE

CONFIG = ROOT / 'evidence/eng1_behavioral_build.json'
MANIFEST = ROOT / 'evidence/eng1_behavioral_incomplete.json'
SOURCE = ROOT / 'src/eng1/behavioral/incomplete_bodies.c'


def render(entries):
    lines = ['/* GENERATED TRANSITIONAL PLACEHOLDERS. No reconstructed behavior.',
             ' * Synchronize with python scripts/eng1_provisional.py.',
             ' * Every entry logs its original RVA and MUST NOT RETURN.',
             ' * Opaque cdecl arguments are deliberately ignored; no returning ABI',
             ' * is claimed. Replace with real source before gameplay acceptance. */',
             '#include <windows.h>', '#include "incomplete.h"', '']
    for row in entries:
        name = row['symbol'][1:]
        lines += ['/* INCOMPLETE: ENG1.DLL RVA '+row['rva']+'; '+row['kind']+'. */',
                  'void __cdecl '+name+'(void)', '{',
                  '    eng1_incomplete_behavior('+row['rva']+'UL,',
                  '        "unimplemented entry '+name+'; opaque arguments; no success return");',
                  '    ExitProcess(0xe0484731UL);', '}', '']
    return ('\n'.join(lines).rstrip()+'\n').encode()


def synchronize(report_path):
    report_path = report_path.resolve()
    if not report_path.is_relative_to(ROOT/'work'):
        raise ValueError('The diagnostic build report must be under ignored work/')
    report = json.loads(report_path.read_bytes())
    if report.get('includes_provisional_functions'):
        raise ValueError('Regenerate from the genuine source-only build, never from a provisional link')
    config = json.loads(CONFIG.read_bytes())
    if report['config_sha256'] != digest(CONFIG.read_bytes()) or report['recovery_sha256'] != digest((ROOT/'recovery.json').read_bytes()):
        raise ValueError('Source-only build report is stale; rebuild first')
    for path,expected in report['context_sha256'].items():
        if digest((ROOT/path).read_bytes())!=expected:
            raise ValueError('Source-only context is stale; rebuild first: '+path)
    image = PE(ROOT/'assets/pc/HERCULES/ENG1.DLL')
    if digest(image.data) != report['target_sha256']:
        raise ValueError('Native ENG1 oracle identity changed')
    named = json.loads((ROOT/'evidence/eng1_unresolved_identity_map.json').read_bytes())
    identities = {x['symbol']:int(x['rva'],0) for x in named['functions']}
    region = json.loads((ROOT/'evidence/actor_continuation_region.json').read_bytes())
    if region['target_sha256'] != report['target_sha256']:
        raise ValueError('Collision identity evidence uses another oracle')
    for symbol in ['_query_collision','_resolve_collision']:
        identities[symbol] = int(region['symbol_rvas'][symbol],0)
    providers = {int(x['rva'],0):x for x in report['functions']+report['unindexed_providers'] if x['compiled_definition']}
    entries = []
    for symbol in report['unresolved']:
        if not re.fullmatch(r'_[A-Za-z_]\w*',symbol):
            raise ValueError('Unreviewed decoration/protocol: '+symbol)
        found = re.fullmatch(r'_(?:eng1_frame_target|eng1_native_code_target|eng1_native_dep|eng1_call|eng1_target|dispatch_local|dispatch_engine|resource_helper)_([0-9a-f]+)',symbol)
        rva = int(found.group(1),16) if found else identities.get(symbol)
        if rva is None:
            raise ValueError('Missing native identity: '+symbol)
        section = next((s for s in image.sections if s['virtual_address'] <= rva < s['virtual_address']+s['virtual_size']),None)
        if section is None or section['name'] != '.text':
            raise ValueError('Missing data/library dependency cannot become a function: '+symbol)
        existing = providers.get(rva)
        row = dict(module='ENG1.DLL',rva=hex(rva),symbol=symbol,source=str(SOURCE.relative_to(ROOT)),
                   kind='unimplemented_binding' if existing else 'unimplemented_function',
                   condition='Always fails on entry; behavior and returning argument protocol unsupported.',
                   runtime_report='Logs original RVA/name, raises noncontinuable0xe0484731, then ExitProcess; never fabricates success.',
                   evidence='evidence/eng1_unresolved_identity_map.json' if not found else 'Encoded native RVA and verified original .text section; diagnostic target, function extent not universally reviewed.',
                   consumers=report['undefined_consumers'][symbol],
                   implementation_status='known_incomplete',compiled_source_is_not_reconstructed_behavior=True)
        if existing:row['existing_genuine_provider']=existing
        entries.append(row)
    manifest=json.loads(MANIFEST.read_bytes())
    manifest['functions']=[x for x in manifest['functions'] if x.get('kind') not in ('unimplemented_function','unimplemented_binding')]+entries
    manifest['provisional_generation']=dict(source_only_report_sha256=digest(report_path.read_bytes()),
        oracle_sha256=report['target_sha256'],recovery_sha256=report['recovery_sha256'],
        symbols=len(entries),native_rvas=len({x['rva'] for x in entries}),
        scope='Transitional loader/startup diagnostics only; excluded from source coverage and never behaviorally accepted.')
    MANIFEST.write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
    SOURCE.write_bytes(render(entries))
    config['provisional_source']=str(SOURCE.relative_to(ROOT))
    CONFIG.write_bytes((json.dumps(config,indent=2)+'\n').encode())
    # This operation changes only the source-graph configuration pin. Preserve
    # every measured data/source/oracle pin; never refresh them opportunistically.
    data_manifest_path = ROOT / 'evidence/eng1_data_evidence_manifest.json'
    data_manifest = json.loads(data_manifest_path.read_bytes())
    config_path = str(CONFIG.relative_to(ROOT)).replace('\\', '/')
    if config_path not in data_manifest['inputs_sha256']:
        raise ValueError('Data manifest lacks the reviewed source-graph configuration pin')
    data_manifest['inputs_sha256'][config_path] = digest(CONFIG.read_bytes())
    data_manifest_path.write_bytes((json.dumps(data_manifest,indent=2)+'\n').encode())
    print(json.dumps(dict(provisional_symbols=len(entries),native_rvas=len({x['rva'] for x in entries}),source=str(SOURCE.relative_to(ROOT)))))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,default=ROOT/'work/eng1_behavioral/build/report.json')
    synchronize(parser.parse_args().report)
