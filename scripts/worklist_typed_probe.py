"""Replay the private typed worklist recipe; diagnostics never grant acceptance.

Generated C++, objects, images and case traces stay under work/. Only newly
compiled fixture code and unpatched instructions in Unicorn execute. This tool
does not load an original OS module or modify recovery/build-plan state.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from env import ROOT
from match import DEFAULT_TOOLCHAIN, compile_source, digest
from masked import masked_compare
from pe import PE
from region import apply_patch_text
from title_link import read_map
import worklist_live_reads_differential as live

RECIPE = ROOT / 'evidence/worklist_typed_recipe_2026_10_25.json'


def materialize(recipe, output):
    baseline = ROOT / recipe['baseline']
    if digest(baseline.read_bytes()) != recipe['baseline_file_sha256']:
        raise ValueError('Typed recipe baseline changed; review before replay')
    text = apply_patch_text(baseline.read_text(), recipe['source_patch'])
    suffix = recipe.get('source_suffix', '.cpp')
    if suffix not in ('.c', '.cpp'):
        raise ValueError('Typed recipe source suffix must be .c or .cpp')
    source = output / ('typed' + suffix)
    source.write_text(text, encoding='utf-8', newline='\n')
    if digest(source.read_bytes()) != recipe['materialized_source_sha256']:
        raise ValueError('Typed recipe materialization differs')
    return source


def runtime(output, source):
    built, _, _ = live.build(output, source)
    # The typed candidate has no guards. Retain the fixture's intercepted model
    # endpoints because the existing execution harness resolves their names.
    # This changes private link retention only, never a compiler contribution.
    command = [arg.replace('/OPT:REF', '/OPT:NOREF')
               for arg in built['commands'][-1]]
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    linked = subprocess.run(command, cwd=output, env=env, capture_output=True,
                            text=True, timeout=60)
    (output / 'noref_link.log').write_text(linked.stdout + linked.stderr)
    if linked.returncode:
        raise ValueError('Private typed fixture link failed')
    candidate = PE(output / 'fixture.dll')
    native = PE(live.ORACLE)
    symbols = read_map(output / 'fixture.map')
    checks = []
    for case in live.cases():
        original, _ = live.execute(native, symbols, built['source_ranges'], case, True)
        recovered, guards = live.execute(candidate, symbols, built['source_ranges'], case, False)
        if guards:
            raise ValueError('Typed candidate unexpectedly called a guard model')
        checks.append(dict(case=case, native=original, source=recovered,
                           equal=original == recovered,
                           different=[k for k in original if original[k] != recovered[k]]))
    for path, expected in built['input_sha256'].items():
        if digest(Path(path).read_bytes()) != expected:
            raise ValueError('Fixture input changed during execution')
    traces = json.dumps(checks, sort_keys=True).encode() + b'\n'
    (output / 'cases.json').write_bytes(traces)
    compilation = next(c for c in built['commands'] if c[-1] == str(source))
    obj = Path(next(arg[3:] for arg in compilation if arg.startswith('/Fo')))
    return obj, compilation, dict(
        cases=len(checks), failed_cases=sum(not c['equal'] for c in checks),
        all_equal=all(c['equal'] for c in checks), cases_sha256=digest(traces),
        source_guard_invocations=0, build=built, relink_command=command,
        relinked_image_sha256=digest(candidate.data),
        scope='Group0 synthetic banks/aliases and modeled callbacks; no prefix-guard or capacity proof.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/worklist_typed_replay')
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--recipe', type=Path, default=RECIPE)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'work').resolve()):
        parser.error('Generated probe files belong under work/')
    output.mkdir(parents=True, exist_ok=True)
    recipe_path = args.recipe.resolve()
    if not recipe_path.is_relative_to((ROOT / 'evidence').resolve()):
        parser.error('Typed recipes must be repository evidence')
    recipe = json.loads(recipe_path.read_text())
    oracle = ROOT / recipe['target']
    if digest(oracle.read_bytes()) != recipe['target_sha256']:
        raise ValueError('Typed recipe oracle changed')
    source = materialize(recipe, output)
    if args.compile_only:
        obj, command = compile_source(source, ['/O2', '/I' + str((ROOT / live.SOURCE).parent)])
        comparison = None
    else:
        obj, command, comparison = runtime(output / 'runtime', source)
    checks = {f['symbol']: masked_compare(obj, f['symbol'], oracle, int(f['rva'], 0), f['size'])
              for f in recipe['functions']}
    report = dict(
        scope='Fresh private compiler diagnostic; matching_acceptance_added is always zero.',
        recipe_path=str(recipe_path), recipe_sha256=digest(recipe_path.read_bytes()),
        source_sha256=digest(source.read_bytes()),
        object=str(obj), object_sha256=digest(obj.read_bytes()), command=command,
        target_sha256=recipe['target_sha256'], checks=checks, runtime=comparison,
        matching_acceptance_added=0, remaining_obligations=recipe['remaining_obligations'])
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(
        functions={n:dict(size=c['compiled_size'], different=c['differing_bytes'],
                          masked_equal=c['masked_equal']) for n, c in checks.items()},
        runtime=None if comparison is None else {k:comparison[k] for k in ('cases', 'failed_cases')},
        matching_acceptance_added=0)))
    if comparison is not None and not comparison['all_equal']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
