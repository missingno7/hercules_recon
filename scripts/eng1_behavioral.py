"""Fresh source-only ENG1 build and derived dependency diagnostic, never acceptance."""
import argparse
from collections import Counter, defaultdict
import json
import os
from pathlib import Path
import re
import subprocess

from coff import COFF
from env import ROOT
from function_map import build as function_inventory
from match import DEFAULT_TOOLCHAIN, compile_source, compare, digest, verify_toolchain
from pe import PE
from refinery_forms import function_text, replace_function
from title_behavioral import context_files
from title_link import read_map

CONFIG = ROOT / 'config/eng1/sources.json'


def build(output, include_incomplete=False, original_root=None):
    output = output.resolve()
    if not output.is_relative_to(ROOT / 'work'):
        raise ValueError('ENG1 outputs must remain under ignored work/')
    output.mkdir(parents=True, exist_ok=True)
    dll = output / 'ENG1.DLL'
    if dll.exists():
        dll.unlink()
    state_bytes, config_bytes = (ROOT / 'recovery.json').read_bytes(), CONFIG.read_bytes()
    state, config = json.loads(state_bytes), json.loads(config_bytes)
    # Cross-module identity rows (*.shared_*) reuse another module's unit (e.g. a TITLE unit compiled
    # masked-equal at an ENG1 twin); they document shared source, not an ENG1 provider.
    rows = [row for row in state['functions'] if Path(row['target']).name.upper() == 'ENG1.DLL'
            and '.shared_' not in row['id']]
    original_root = Path(original_root).resolve() if original_root is not None else ROOT
    target = original_root / 'assets/pc/HERCULES/ENG1.DLL'
    if digest(target.read_bytes()) != config['target_sha256']:
        raise ValueError('Original ENG1 identity changed')
    graph = {}
    for row in rows:
        flags = row['flags']
        if row['source'] in graph and graph[row['source']] != flags:
            raise ValueError('Conflicting flags for source: ' + row['source'])
        graph[row['source']] = flags
    for unit in config['additional_sources']:
        if unit['source'] in graph:
            # A provider recorded in recovery.json (e.g. staged in place) is the same object.
            if graph[unit['source']] != unit.get('flags', ['/O2']):
                raise ValueError('Conflicting flags for source provider: ' + unit['source'])
            continue
        graph[unit['source']] = unit.get('flags', ['/O2'])
    incomplete_bytes=(ROOT/config['incomplete_manifest']).read_bytes()
    incomplete_rows=json.loads(incomplete_bytes)['functions']
    provisional=[x for x in incomplete_rows if x.get('kind') in ('unimplemented_function','unimplemented_binding')]
    provisional_symbols={x['symbol'] for x in provisional}
    if any('incomplete_bodies' in source for source in graph):
        raise ValueError('Generated placeholders cannot be canonical source providers')
    if include_incomplete:
        from eng1_provisional import render
        source = output / 'diagnostic' / 'incomplete_bodies.c'
        source.parent.mkdir(exist_ok=True)
        header = (ROOT / 'src/eng1/incomplete.h').as_posix()
        source.write_bytes(render(provisional).replace(b'"incomplete.h"', ('"' + header + '"').encode()))
        graph[source.relative_to(ROOT).as_posix()] = ['/O2']
    contexts = {str(path.relative_to(ROOT)): digest(path.read_bytes())
                for source in graph for path in context_files(ROOT / source)}
    contexts[config['incomplete_manifest']]=digest(incomplete_bytes)
    objects, compiled, strict = [], [], []
    definitions, undefined = defaultdict(list), defaultdict(list)
    function_definitions = {}
    literal_contributions=defaultdict(list)
    report = dict(scope='Behavioral source/link evidence; no recovery promotion or playable-engine claim.',
                  includes_provisional_functions=include_incomplete,
                  architecture=None, target_sha256=config['target_sha256'],
                  read_only_original_root=str(original_root),
                  recovery_sha256=digest(state_bytes), config_sha256=digest(config_bytes),
                  context_sha256=contexts, toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),
                  compiled_sources=compiled, preserved_exact_functions=strict,
                  behaviorally_exact=False, runtime_integration='Build-only report; runtime observations are recorded separately.')
    adaptations = config.get('source_adaptations', {})
    extensions = config.get('unit_extensions', {})
    omissions = config.get('omit_functions', {})
    aliases = config.get('symbol_aliases', {})
    for source, flags in graph.items():
        compiler_input, compile_flags = ROOT / source, list(flags)
        compile_flags += ['/D' + old + '=' + binding['provider'] for old, binding in aliases.items()]
        if source in adaptations or source in extensions or source in omissions:
            text = compiler_input.read_text()
            for omitted in omissions.get(source, []):
                body = function_text(text, omitted['function'])
                declaration = body[:body.index('{')].rstrip() + ';'
                text = replace_function(text, omitted['function'], declaration)
            for edit in adaptations.get(source, []):
                if text.count(edit['before']) != edit.get('count', 1):
                    raise ValueError('Stale ENG1 source adaptation: ' + source + ': ' + edit['reason'])
                text = text.replace(edit['before'], edit['after'])
            for fragment in extensions.get(source, []):
                text += '\n#include "' + (ROOT / fragment).as_posix() + '"\n'
                for path in context_files(ROOT / fragment, include_dirs=((ROOT / source).parent,)):
                    contexts[str(path.relative_to(ROOT))] = digest(path.read_bytes())
            compiler_input = output / 'sources' / (source.replace('/', '__'))
            compiler_input.parent.mkdir(exist_ok=True)
            compiler_input.write_bytes(text.encode('utf-8'))
            compile_flags.append('/I' + str((ROOT / source).parent))
            # Includes introduced by an adaptation are part of the actual input.
            # The disposable generated file is already hashed separately below.
            for path in context_files(compiler_input, include_dirs=((ROOT / source).parent,)):
                if path != compiler_input:
                    contexts[str(path.relative_to(ROOT))] = digest(path.read_bytes())
        try:
            obj, command = compile_source(compiler_input, compile_flags)
        except RuntimeError as error:
            report.update(compile_failure=dict(source=source, error=str(error)), link_exit_code=None)
            (output / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
            raise
        coff = COFF(obj)
        objects.append(obj)
        compiled.append(dict(source=source, command=command, object_sha256=digest(obj.read_bytes()),
                             compiler_input_sha256=digest(compiler_input.read_bytes()),
                             adaptations=adaptations.get(source, []), extensions=extensions.get(source, []),
                             omitted_duplicate_functions=omissions.get(source, []), symbol_aliases=aliases))
        for symbol in coff.symbols.values():
            if symbol['storage'] != 2:
                continue
            name = symbol['name']
            if symbol['section'] > 0:
                definitions[name].append(source)
                if name.startswith('??_C@'):
                    section=coff.sections[symbol['section']-1]
                    literal_contributions[name].append(dict(source=source,
                        data=section['data'],size=section['size'],flags=section['flags'],
                        relocations=section['relocations']))
                if symbol['type'] & 0x20:
                    function_definitions[name] = source
            elif symbol['section'] == 0 and symbol['value'] == 0:
                undefined[name].append(dict(source=source, kind='function' if symbol['type'] & 0x20 else 'untyped_external'))
        for row in rows:
            if row['source'] != source or row['status'] != 'FUNCTION_MATCH':
                continue
            result = compare(obj, row['symbol'], target, int(row['rva'], 0), row['size'])
            strict.append(dict(symbol=row['symbol'], original_rva=row['rva'], size=row['size'], exact=result['exact']))
            if not result['exact']:
                raise ValueError('Accepted ENG1 function regressed: ' + row['id'])

    environment = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        environment.pop(key, None)
    environment.update(PATH=str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + environment.get('PATH', ''),
                       LIB=str(DEFAULT_TOOLCHAIN / 'lib'))
    command = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/nologo', '/DLL', '/NODEFAULTLIB',
               '/INCREMENTAL:NO', '/OPT:NOREF', '/OUT:' + str(dll),
               '/MAP:' + str(output / 'ENG1.map'), '/DEF:' + str(ROOT / config['exports']),
               *map(str, objects), *config['libraries']]
    result = subprocess.run(command, cwd=output, env=environment, capture_output=True, text=True, timeout=120)
    log = result.stdout + result.stderr
    (output / 'link.log').write_bytes(log.encode('utf-8'))
    unresolved = sorted(set(re.findall(r'unresolved external symbol (\S+)', log)))
    duplicates = {name: owners for name, owners in definitions.items() if len(owners) > 1}
    pooled_literals={}
    for name in list(duplicates):
        contributions=literal_contributions.get(name,[])
        for contribution in contributions:
            if not contribution['data'] and contribution['flags']&0x80:
                contribution['data']=bytes(contribution['size'])
        if (len(contributions)==len(duplicates[name]) and
            all(x['flags']&0x1000 and not x['flags']&0x20000000 and
                x['size']==len(x['data'])==contributions[0]['size'] and
                not x['relocations'] and x['data']==contributions[0]['data']
                for x in contributions)):
            pooled_literals[name]=dict(sources=duplicates.pop(name),
                size=len(contributions[0]['data']),
                sha256=digest(contributions[0]['data']),
                classification='Identical loaded bytes in non-executable compiler string-literal COMDATs; explicit COFF zero-fill retained; ordinary linker pooling, no first-party storage conflict.')
    report.update(link_command=command, link_exit_code=result.returncode, unresolved=unresolved,
                  duplicate_providers=duplicates, compiler_pooled_literals=pooled_literals,
                  undefined_consumers={name: undefined[name] for name in unresolved})
    inventory = function_inventory('ENG1.DLL', original_root=original_root)['functions']
    providers = {int(row['rva'], 0): dict(row) for row in rows}
    alternatives = defaultdict(list)
    for row in config.get('functions', []):
        rva = int(row['rva'], 0)
        alternatives[rva].append(row)
        if rva not in providers:
            providers[rva] = row
    coverage = []
    indexed_rvas = {int(entry['rva'], 0) for entry in inventory}
    unindexed = [dict(module='ENG1.DLL', rva=hex(rva), source=provider['source'],
                      symbol=provider['symbol'],
                      compiled_definition=provider['symbol'] in function_definitions,
                      extent_review_required=True,
                      note='Explicit recovered provider absent from diagnostic start inventory.')
                 for rva, provider in sorted(providers.items()) if rva not in indexed_rvas]
    for entry in inventory:
        row = dict(module='ENG1.DLL', rva=entry['rva'], inventory_size=entry['size'],
                   extent_review_required=not entry['decoded'] or entry['last'] not in ('ret', 'jmp'),
                   compiled_definition=False, behavioral_verification='unverified')
        provider = providers.get(int(entry['rva'], 0))
        if provider:
            row.update(source=provider['source'], symbol=provider['symbol'],
                       existing_evidence=provider.get('status'),
                       behavioral_verification=provider.get('implementation_status', 'unverified'),
                       semantic_contract=provider.get('contract', provider.get('evidence')),
                       compiled_definition=provider['symbol'] in function_definitions)
        if int(entry['rva'], 0) in alternatives:
            row['additional_evidence'] = alternatives[int(entry['rva'], 0)]
        coverage.append(row)
    report.update(functions=coverage, inventory_count=len(inventory), recorded_source_count=len(rows),
                  compiled_function_definitions=function_definitions,
                  mapped_compiled_function_count=sum(row['compiled_definition'] for row in coverage),
                  compiled_function_symbols=len(function_definitions),
                  unindexed_providers=unindexed,
                  compiled_provider_rva_count=sum(row['compiled_definition'] for row in coverage)
                                             + sum(row['compiled_definition'] for row in unindexed),
                  inventory_scope='Diagnostic starts/leads plus explicit providers; not a reviewed complete first-party inventory.',
                  existing_evidence_counts=dict(Counter(row['status'] for row in rows)),
                  incomplete_functions=incomplete_rows,
                  provisional_function_count=len(provisional_symbols.intersection(function_definitions)),
                  genuine_compiled_function_symbols=len(set(function_definitions)-provisional_symbols),
                  source_complete=False, playable=False)
    for entry in coverage:
        entry['provisional_symbols']=[x['symbol'] for x in provisional if int(x['rva'],0)==int(entry['rva'],0) and x['symbol'] in function_definitions]
    if (ROOT / 'recovery.json').read_bytes() != state_bytes or CONFIG.read_bytes() != config_bytes:
        raise ValueError('Recovery/configuration changed during ENG1 build')
    for path, expected in contexts.items():
        if digest((ROOT / path).read_bytes()) != expected:
            raise ValueError('ENG1 build context changed: ' + path)
    if result.returncode == 0:
        image = PE(dll)
        metadata = image.metadata()
        if metadata['machine'] != 0x14c or [{k: e[k] for k in ('name', 'ordinal')} for e in metadata['exports']] != [dict(name='PC_DLLEngineMain', ordinal=1)]:
            raise ValueError('ENG1 architecture/export mismatch')
        if any(row['dll'].upper() in ('ENG1.DLL', 'ENG3.DLL', 'TITLE.DLL') for row in metadata['imports']):
            raise ValueError('Forbidden dispatch dependency on original engine')
        symbols = read_map(output / 'ENG1.map')
        for row in strict:
            linked_rva = symbols[row['symbol']] - image.image_base
            if image.read_rva(linked_rva, row['size']) != PE(target).read_rva(int(row['original_rva'], 0), row['size']):
                raise ValueError('Linked accepted ENG1 function regressed: ' + row['symbol'])
            row['linked_rva'] = hex(linked_rva)
        report.update(architecture='x86 PE32', exports=metadata['exports'], imports=metadata['imports'],
                      dll=str(dll.relative_to(ROOT)), dll_sha256=digest(dll.read_bytes()),
                      link_map=str((output / 'ENG1.map').relative_to(ROOT)),
                      link_map_sha256=digest((output / 'ENG1.map').read_bytes()))
    (output / 'report.json').write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps({key: report.get(key) for key in ('link_exit_code', 'inventory_count',
                     'mapped_compiled_function_count', 'compiled_function_symbols', 'architecture', 'dll')}))
    print('unresolved:', len(unresolved), 'duplicate providers:', len(duplicates),
          'preserved exact functions:', len(strict))
    print('report:', output / 'report.json')
    return result.returncode


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_behavioral/build')
    parser.add_argument('--original-root', type=Path,
                        help='Read originals and derived analysis from another checkout; source/recovery stay local')
    parser.add_argument('--include-incomplete',action='store_true',help='Explicit transitional loader build; missing bodies fail on entry and do not count as implemented')
    arguments = parser.parse_args()
    raise SystemExit(build(arguments.output,arguments.include_incomplete,arguments.original_root))
