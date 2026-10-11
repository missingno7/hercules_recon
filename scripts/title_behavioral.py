"""Ordinary source-only TITLE build; derived coverage, never matching acceptance.

Existing source providers and their flags come from recovery.json. Behavioral
additions are explicit in evidence/title_behavioral_build.json. No forced link,
original code, fixed addresses, or recovery-state mutations are permitted here.
"""
import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import re
import subprocess

from env import ROOT
from coff import COFF
from function_map import build as function_inventory
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain
from pe import PE
from title_link import read_map, sources


CONFIG = ROOT / 'evidence/title_behavioral_build.json'


def context_files(path, seen=None, include_dirs=()):
    seen = set() if seen is None else seen
    path = path.resolve()
    if path in seen:
        return seen
    seen.add(path)
    for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', path.read_text(), re.M):
        child = next((directory / name for directory in (path.parent, *include_dirs)
                      if (directory / name).is_file()), None)
        if child is not None:
            context_files(child, seen, include_dirs)
    return seen


def build(output):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work') and not output.is_relative_to(ROOT / 'build'):
        raise ValueError('Build output must remain in ignored work/ or build/')
    output.mkdir(parents=True, exist_ok=True)
    dll = output / 'TITLE.DLL'
    if dll.exists():
        dll.unlink()  # Failures, including compile failures, cannot leave a stale DLL.
    state_bytes = (ROOT / 'recovery.json').read_bytes()
    config_bytes = CONFIG.read_bytes()
    config = json.loads(config_bytes)
    state = json.loads(state_bytes)
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == 'TITLE.DLL']
    graph = sources('TITLE.DLL')
    for unit in config['additional_sources']:
        if unit['source'] in graph:
            raise ValueError('Duplicate behavioral provider: ' + unit['source'])
        graph[unit['source']] = unit.get('flags', ['/O2'])
    for original, replacement in config.get('source_overrides', {}).items():
        if original not in graph:
            raise ValueError('Unknown override: ' + original)
        graph[replacement] = graph.pop(original)

    baseline = {str(p.relative_to(ROOT)): digest(p.read_bytes())
                for source in graph for p in context_files(ROOT / source)}
    extensions = config.get('unit_extensions', {})
    adaptations = config.get('source_adaptations', {})
    for source, fragments in extensions.items():
        for fragment in fragments:
            for path in context_files(ROOT / fragment, include_dirs=((ROOT / source).parent,)):
                baseline[str(path.relative_to(ROOT))] = digest(path.read_bytes())
    objects, definitions, compiled = [], defaultdict(list), []
    report = dict(scope='Behavioral build diagnostic; no FUNCTION_MATCH promotion or whole-DLL equivalence claim.',
                  toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                  recovery_sha256=digest(state_bytes), config_sha256=digest(config_bytes),
                  context_sha256=baseline, original=digest((ROOT / rows[0]['target']).read_bytes()),
                  compiled_sources=compiled, unresolved=[], duplicates=[],
                  runtime_integration='not executed', behaviorally_exact=False)
    for source, flags in graph.items():
        compiler_input = ROOT / source
        compile_flags = list(flags)
        if source in extensions or source in adaptations:
            # Materialize only changed units, never a copied source tree. These
            # are reviewable ownership/type edits, not binary patch operations.
            original_text = compiler_input.read_text()
            for change in adaptations.get(source, []):
                count = original_text.count(change['before'])
                if count == 0 and change.get('allow_already_applied') and original_text.count(change['after']) == 1:
                    continue
                if count != change.get('count', 1):
                    raise ValueError('Stale source adaptation: ' + source + ': ' + change['reason'])
                original_text = original_text.replace(change['before'], change['after'])
            for fragment in extensions.get(source, []):
                original_text += '\n#include "' + (ROOT / fragment).as_posix() + '"\n'
            compiler_input = output / 'sources' / Path(source).name
            compiler_input.parent.mkdir(exist_ok=True)
            compiler_input.write_text(original_text, encoding='utf-8')
            compile_flags.append('/I' + str((ROOT / source).parent))
        try:
            obj, command = compile_source(compiler_input, compile_flags)
        except RuntimeError as error:
            report.update(compile_failure=dict(source=source, error=str(error)), link_exit_code=None)
            (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
            raise
        objects.append(obj)
        compiled.append(dict(source=source, command=command, object_sha256=digest(obj.read_bytes()),
                             compiler_input_sha256=digest(compiler_input.read_bytes()),
                             extensions=extensions.get(source, []),
                             adaptations=adaptations.get(source, [])))
        for sym in COFF(obj).symbols.values():
            if sym['storage'] == 2 and sym['section'] > 0:
                definitions[sym['name']].append(source)

    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    env.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', ''),
               LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/nologo', '/DLL',
               '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF',
               '/OUT:' + str(dll), '/MAP:' + str(output / 'TITLE.map'),
               '/DEF:' + str(ROOT / config['exports']), *map(str, objects),
               *config['libraries']]
    result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, timeout=120)
    log = result.stdout + result.stderr
    (output / 'link.log').write_text(log, encoding='utf-8')
    report.update(link_command=command, link_exit_code=result.returncode,
                  unresolved=sorted(set(re.findall(r'unresolved external symbol (\S+)', log))),
                  duplicates=sorted(set(re.findall(r'LNK2005: (\S+) already defined', log))))
    for path, sha in baseline.items():
        if digest((ROOT / path).read_bytes()) != sha:
            raise RuntimeError('Source/header changed during build: ' + path)
    if (ROOT / 'recovery.json').read_bytes() != state_bytes or CONFIG.read_bytes() != config_bytes:
        raise RuntimeError('Recovery or build configuration changed during build')

    inventory = function_inventory('TITLE.DLL')['functions']
    by_rva = {int(r['rva'], 16): r for r in rows}
    additions = {int(r['rva'], 16): r for r in config.get('functions', [])}
    validation = json.loads((ROOT / 'evidence/title_behavioral_validation.json').read_text())
    validations = {int(r['rva'], 16): r for r in validation['functions']}
    coverage = []
    for entry in inventory:
        rva = int(entry['rva'], 16)
        recovered, addition = by_rva.get(rva), additions.get(rva)
        row = dict(module='TITLE.DLL', rva=entry['rva'], inventory_size=entry['size'],
                   extent_review_required=not entry['decoded'] or entry['last'] not in ('ret', 'jmp'),
                   behavioral_verification='unverified')
        if recovered:
            provider = config.get('source_overrides', {}).get(recovered['source'], recovered['source'])
            row.update(source=provider, symbol=recovered['symbol'],
                       existing_evidence=recovered['status'],
                       evidence=recovered.get('evidence'),
                       semantic_contract=recovered.get('evidence'),
                       dependencies=sorted(recovered.get('link_symbols', {})),
                       compiled_definition=recovered['symbol'] in definitions,
                       original_proof_preserved=True,
                       original_compile_context=provider == recovered['source'] and
                       provider not in extensions and provider not in adaptations)
        if addition:
            row.update(addition)
            row['compiled_definition'] = addition['symbol'] in definitions
        if rva in validations:
            row['behavioral_validation'] = validations[rva]
            row['behavioral_verification'] = validations[rva]['status']
        coverage.append(row)
    report.update(functions=coverage, inventory_count=len(inventory),
                  recorded_source_count=len(rows),
                  compiled_function_count=sum(r.get('compiled_definition', False) for r in coverage),
                  existing_evidence_counts=dict(Counter(r['status'] for r in rows)),
                  duplicate_providers={s: owners for s, owners in definitions.items() if len(owners) > 1})
    incomplete = json.loads((ROOT / config['incomplete_manifest']).read_text())
    report['incomplete_functions'] = incomplete['functions']
    if result.returncode == 0 and dll.is_file():
        pe = PE(dll)
        metadata = pe.metadata()
        expected_exports = [dict(name='PC_DLLEngineMain', ordinal=1)]
        actual_exports = [dict(name=e['name'], ordinal=e['ordinal']) for e in metadata['exports']]
        imported_names = [i['dll'].upper() for i in metadata['imports']]
        if pe.pe.FILE_HEADER.Machine != 0x14c or actual_exports != expected_exports:
            raise RuntimeError('Architecture or export ABI validation failed')
        if 'TITLE.DLL' in imported_names:
            raise RuntimeError('Forbidden original TITLE dependency')
        linked = read_map(output / 'TITLE.map')
        oracle = PE(ROOT / rows[0]['target'])
        exact_peers = []
        for row in rows:
            if row['status'] != 'FUNCTION_MATCH':
                continue
            actual_rva = linked[row['symbol']] - pe.image_base
            actual = pe.read_rva(actual_rva, row['size'])
            expected = oracle.read_rva(int(row['rva'], 16), row['size'])
            exact_peers.append(dict(symbol=row['symbol'], original_rva=row['rva'],
                                    linked_rva=hex(actual_rva), size=row['size'], exact=actual == expected))
        if not all(peer['exact'] for peer in exact_peers):
            raise RuntimeError('Previously byte-exact TITLE function regressed in the linked DLL')
        report.update(dll_sha256=digest(dll.read_bytes()), architecture='x86 PE32',
                      exports=metadata['exports'], imports=metadata['imports'],
                      dll=str(dll.relative_to(ROOT)), linked_symbols=len(linked),
                      preserved_exact_functions=exact_peers)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: report.get(k) for k in ('link_exit_code', 'inventory_count',
          'compiled_function_count', 'architecture', 'exports', 'dll')}))
    print('unresolved:', len(report['unresolved']), 'duplicates:', len(report['duplicates']))
    print('report:', output / 'report.json')
    return result.returncode


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/title_behavioral/build')
    args = parser.parse_args()
    raise SystemExit(build(args.output))
