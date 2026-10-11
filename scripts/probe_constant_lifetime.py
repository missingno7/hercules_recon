"""Minimal reproducible RTM constant-web discriminator; no game or acceptance input.

Prediction: repeated stores in separate conditions can share a constant web;
later uses crossing a call move it from a scratch to a callee-saved register.
Falsifier: controls retain the same allocation, or repeated fresh outputs differ.
The result does not predict allocation under the full TITLE function's pressure.
"""
import argparse
import json
from pathlib import Path
import tempfile

from env import ROOT
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from coff import COFF
from match import compile_source, digest, verify_toolchain, DEFAULT_TOOLCHAIN
from region import output_identities


def sources():
    template = '''extern int ga[8], g0, g1; void f(void);
void probe(void) {
  if (g0) { ga[0] = 6; ga[1] = 6; ga[2] = 6; ga[3] = 6; CALL }
  if (g1) { ga[4] = 6; ga[5] = 6; ga[6] = 6; ga[7] = 6; CALL }
}
'''
    return {'two_if_calls': template.replace('CALL', 'f();'),
            'two_if_no_calls': template.replace('CALL', '')}


def run(output, repeats):
    output.mkdir(parents=True, exist_ok=True)
    report = dict(scope='Synthetic compiler evidence only; no target comparison or recovery change.',
                  prediction=__doc__.split('Prediction: ', 1)[1].strip(),
                  toolchain=verify_toolchain(DEFAULT_TOOLCHAIN), flags=['/O2'],
                  toolchain_manifest_sha256=digest((ROOT / 'toolchains/manifest.json').read_bytes()),
                  rebuild_scope='Complete decoded function contributions and noncode output identity; not timestamped whole-object or image equality.',
                  build_count=0, cases=[])
    # Record the discriminator before compiling, including control source hashes.
    recipes = sources()
    report['sources'] = {name: digest(source.encode()) for name, source in recipes.items()}
    (output / 'prediction.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for name, text in recipes.items():
        source = output / (name + '.c')
        source.write_text(text, encoding='utf-8', newline='\n')
        builds = []
        for _ in range(repeats):
            before = digest(source.read_bytes())
            obj, command = compile_source(source, ['/O2'])
            if digest(source.read_bytes()) != before:
                raise ValueError('Probe input changed during compilation')
            section = COFF(obj).function('_probe')
            instructions = list(Cs(CS_ARCH_X86, CS_MODE_32).disasm(section['data'], 0))
            if sum(i.size for i in instructions) != len(section['data']):
                raise ValueError('Incomplete probe instruction decoding')
            functions, whole = output_identities(obj, ['_probe'])
            builds.append(dict(command=command, source_sha256=before,
                               object=str(obj), object_sha256=digest(obj.read_bytes()),
                               function_output_id=functions['_probe'], measured_output_id=whole,
                               size=section['size'], relocation_count=len(section['relocations']),
                               literal6_loads=[f'{i.mnemonic} {i.op_str}' for i in instructions
                                               if i.mnemonic == 'mov' and i.op_str.endswith(', 6')],
                               callee_saved_pushes=[i.op_str for i in instructions
                                                    if i.mnemonic == 'push' and i.op_str in ('esi', 'edi', 'ebx', 'ebp')]))
            report['build_count'] += 1
        report['cases'].append(dict(name=name, builds=builds,
                                    repeated_output_equal=len({b['measured_output_id'] for b in builds}) == 1))
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for case in report['cases']:
        first = case['builds'][0]
        print(f"{case['name']}: {first['size']} bytes; {first['literal6_loads']}; "
              f"saved {first['callee_saved_pushes']}; repeated output equal={case['repeated_output_equal']}")
    if not all(c['repeated_output_equal'] for c in report['cases']):
        raise ValueError('Fresh probe outputs are not reproducible')
    print(output / 'report.json')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--repeats', type=int, default=2)
    args = parser.parse_args()
    if args.repeats < 2:
        parser.error('Use at least two fresh builds per control')
    parent = (ROOT / 'work/compiler_constant_lifetime').resolve()
    parent.mkdir(parents=True, exist_ok=True)
    output = args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix='probe-', dir=parent))
    if not output.is_relative_to((ROOT / 'work').resolve()):
        parser.error('Probe outputs belong in ignored work/')
    if output.exists() and any(output.iterdir()):
        parser.error('Use an empty private output directory to preserve previous receipts')
    run(output, args.repeats)


if __name__ == '__main__':
    main()
