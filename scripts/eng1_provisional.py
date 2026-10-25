"""Generate ignored fail-on-entry source only for explicit ENG1 diagnostics.

This never reconstructs unknown behavior. Missing entries remain incomplete and
are excluded from implementation coverage. Every generated body reports its
confirmed original identity and terminates; no arguments or return are assumed.
"""
import argparse
import json
from pathlib import Path

from env import ROOT

MANIFEST = ROOT / 'config/eng1/incomplete.json'


def render(entries):
    lines = ['/* GENERATED TRANSITIONAL PLACEHOLDERS. No reconstructed behavior.',
             ' * Generated only in ignored work/ by scripts/eng1_provisional.py.',
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'work/eng1_diagnostic/incomplete_bodies.c')
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / 'work').resolve()):
        raise ValueError('Generated diagnostic placeholders belong only under ignored work/')
    manifest = json.loads(MANIFEST.read_bytes())
    entries = [r for r in manifest['functions'] if r.get('kind') in ('unimplemented_function', 'unimplemented_binding')]
    output.parent.mkdir(parents=True, exist_ok=True)
    header = (ROOT / 'src/eng1/incomplete.h').as_posix()
    output.write_bytes(render(entries).replace(b'"incomplete.h"', ('"' + header + '"').encode()))
    print(f'{len(entries)} fail-on-entry diagnostic bodies: {output}; no source recovery or link-closure claim')


if __name__ == '__main__':
    main()
