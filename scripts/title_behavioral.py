"""Ordinary source-only TITLE build; derived coverage, never matching acceptance.

Existing source providers and their flags come from recovery.json. Behavioral
additions are explicit in evidence/title_behavioral_build.json. An opt-in matching
profile may constrain function order; it never claims natural layout. No forced
link, original code, fixed-address bodies, or recovery-state mutations occur here.
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


def selected_adaptations(config, matching_diagnostic=False):
    """Omit only explicitly tagged runtime instrumentation in the diagnostic profile."""
    return {source: [change for change in changes
                     if not (matching_diagnostic and change.get('diagnostic_only', False))]
            for source, changes in config.get('source_adaptations', {}).items()}


def selected_additional_sources(config, matching_diagnostic=False):
    return [unit for unit in config['additional_sources']
            if not (matching_diagnostic and unit.get('diagnostic_only', False))]


def ordered_functions(rows, config):
    """Native-start ordering of named source functions, not code payloads or padding."""
    entries = {int(r['rva'], 0): r['symbol'] for r in rows}
    if len(entries) != len(rows):
        raise ValueError('Duplicate native function entries')
    for row in config.get('functions', []):
        rva = int(row['rva'], 0)
        if rva in entries and entries[rva] != row['symbol']:
            raise ValueError('Conflicting ordered function identity')
        entries[rva] = row['symbol']
    names = [entries[rva] for rva in sorted(entries)]
    if len(set(names)) != len(names) or any(not re.fullmatch(r'[A-Za-z_?@$][A-Za-z0-9_?@$]*', n) for n in names):
        raise ValueError('Duplicate or unsafe ordered symbol')
    return names


def order_file_bytes(names):
    # LINK 5.00 adds the x86 C underscore itself (LNK4037 otherwise names __foo).
    # Keep this pilot restricted to its measured cdecl symbol convention.
    if any(not re.fullmatch(r'_[A-Za-z_][A-Za-z0-9_]*', name) for name in names):
        raise ValueError('Code-order pilot supports x86 C cdecl symbols only')
    return ('\n'.join(name[1:] for name in names) + '\n').encode('ascii')


def build(output, matching_diagnostic=False, original_code_order=False):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work') and not output.is_relative_to(ROOT / 'build'):
        raise ValueError('Build output must remain in ignored work/ or build/')
    if original_code_order and not matching_diagnostic:
        raise ValueError('Original code order is an explicit matching diagnostic only')
    if matching_diagnostic and output == ROOT / 'work/title_behavioral/build':
        raise ValueError('Matching diagnostic requires a private output directory')
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
    for unit in selected_additional_sources(config, matching_diagnostic):
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
    adaptations = selected_adaptations(config, matching_diagnostic)
    for source, fragments in extensions.items():
        for fragment in fragments:
            for path in context_files(ROOT / fragment, include_dirs=((ROOT / source).parent,)):
                baseline[str(path.relative_to(ROOT))] = digest(path.read_bytes())
    objects, definitions, compiled = [], defaultdict(list), []
    report = dict(scope='Behavioral build diagnostic; no FUNCTION_MATCH promotion or whole-DLL equivalence claim.',
                  build_profile='matching diagnostic' if matching_diagnostic else 'behavioral with runtime tracing',
                  omitted_trace_adaptations=sum(change.get('diagnostic_only', False)
                      for changes in config.get('source_adaptations', {}).values() for change in changes)
                      if matching_diagnostic else 0,
                  omitted_diagnostic_sources=[u['source'] for u in config['additional_sources']
                                              if matching_diagnostic and u.get('diagnostic_only', False)],
                  toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                  recovery_sha256=digest(state_bytes), config_sha256=digest(config_bytes),
                  context_sha256=baseline, original=digest((ROOT / rows[0]['target']).read_bytes()),
                  compiled_sources=compiled, unresolved=[], duplicates=[],
                  runtime_integration='not executed', behaviorally_exact=False)
    for source, flags in graph.items():
        compiler_input = ROOT / source
        compile_flags = list(flags)
        if matching_diagnostic and source == 'src/title/behavioral/title_entry.c':
            compile_flags.append('/DHERCULES_TITLE_NO_TRACE')
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
    if original_code_order:
        names = ordered_functions(rows, config)
        if any(len(definitions.get(name, [])) != 1 for name in names):
            raise ValueError('Ordered function lacks a unique source definition')
        order_path = output / 'function_order.txt'
        order_bytes = order_file_bytes(names)
        order_path.write_bytes(order_bytes)
        command.insert(1, '/ORDER:@' + str(order_path))
        report['layout_constraint'] = dict(kind='native function order',
            scope='Source-produced COMDAT ordering only; no natural-layout or logic-equivalence claim',
            path=order_path.relative_to(ROOT).as_posix(), sha256=digest(order_bytes), functions=len(names))
    result = subprocess.run(command, cwd=output, env=env, capture_output=True, text=True, timeout=120)
    log = result.stdout + result.stderr
    (output / 'link.log').write_text(log, encoding='utf-8')
    report.update(link_command=command, link_exit_code=result.returncode,
                  unresolved=sorted(set(re.findall(r'unresolved external symbol (\S+)', log))),
                  duplicates=sorted(set(re.findall(r'LNK2005: (\S+) already defined', log))))
    if original_code_order and order_path.read_bytes() != order_bytes:
        raise RuntimeError('Function order changed during linking')
    if original_code_order and re.search(r'warning LNK\d+', log):
        report['constraint_failure'] = 'Linker warning; inspect link.log before claiming constrained order'
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        raise RuntimeError(report['constraint_failure'])
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
        if original_code_order:
            addresses = [linked[name] for name in names]
            if any(a >= b for a, b in zip(addresses, addresses[1:])):
                raise RuntimeError('Linker did not implement the requested function order')
            report['layout_constraint']['mapped_order_verified'] = True
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
                      link_map=str((output / 'TITLE.map').relative_to(ROOT)),
                      link_map_sha256=digest((output / 'TITLE.map').read_bytes()),
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
    parser.add_argument('--matching-diagnostic', action='store_true',
                        help='omit tagged optional trace insertions; retain all providers and proof limits')
    parser.add_argument('--original-code-order', action='store_true',
                        help='explicitly order source function COMDATs by native starts; requires matching diagnostic')
    args = parser.parse_args()
    if args.matching_diagnostic and args.output.resolve() == (ROOT / 'work/title_behavioral/build').resolve():
        parser.error('--matching-diagnostic requires a private --output directory')
    raise SystemExit(build(args.output, args.matching_diagnostic, args.original_code_order))
