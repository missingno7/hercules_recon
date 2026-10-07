"""Rank VC5 candidates; resolved calls are diagnostic and never grant a match."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from env import ROOT
from match import compile_source
from coff import COFF
from pe import PE

TARGET = 'assets/pc/HERCULES/TITLE.DLL'
TARGET_SHA256 = 'b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b'
CONTROL_SOURCE = 'calibration/title_control.c'
PILOT_SOURCE = 'src/title/pilot.c'
# Original CALL operand offsets and their independently decoded destination RVAs.
# No bytes from this mapping or oracle enter compilation or an output executable.
CONTROL = [
    ('_title_4a10', 0x4a10, 48, [(16, '_title_4a40', 0x4a40),
                               (26, '_title_4a40', 0x4a40)]),
    ('_title_6430', 0x6430, 80, [(3, '_title_c610', 0xc610),
                               (11, '_title_6480', 0x6480),
                               (31, '_title_65a0', 0x65a0),
                               (38, '_title_c600', 0xc600),
                               (46, '_title_6530', 0x6530),
                               (51, '_title_64f0', 0x64f0),
                               (58, '_title_c620', 0xc620),
                               (66, '_title_6480', 0x6480)]),
]
PILOT = [('_unlink_12c', 0x97b0, 64), ('_unlink_0e4', 0x98a0, 80),
         ('_make_colour', 0x189a0, 64), ('_sentinel_count', 0xcee0, 32)]
FLAGS = [['/O2'], ['/O1'], ['/O2', '/Og-'], ['/O2', '/G5'],
         ['/O2', '/G6'], ['/Ox'], ['/O2', '/Ob0'], ['/O2', '/Op']]


def distance(left, right):
    return sum(a != b for a, b in zip(left, right)) + abs(len(left) - len(right))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/toolchain-calibration.json')
    parser.add_argument('--pilot-source', type=Path, default=ROOT / PILOT_SOURCE)
    parser.add_argument('--tools-root', type=Path, default=Path(r'C:\tools\hercules'))
    args = parser.parse_args()
    target = ROOT / TARGET
    if sha(target) != TARGET_SHA256:
        raise ValueError('Calibration oracle hash mismatch')
    pe = PE(target)
    destinations = {}
    for _, rva, size, calls in CONTROL:
        original = pe.read_rva(rva, size)
        for offset, name, destination in calls:
            decoded = rva + offset + 4 + struct.unpack_from('<i', original, offset)[0]
            if original[offset - 1] != 0xe8 or decoded != destination:
                raise ValueError(f'Original direct CALL evidence changed at {rva + offset:#x}')
            if name in destinations and destinations[name] != destination:
                raise ValueError(f'Ambiguous external symbol {name}')
            destinations[name] = destination
    manifest = json.loads((ROOT / 'toolchains/manifest.json').read_text())
    report = {
        'schema_version': 1,
        'scope': 'Compiler calibration only. REL32 diagnostic equality is not FUNCTION_MATCH.',
        'target': TARGET, 'target_sha256': TARGET_SHA256,
        'sources': {CONTROL_SOURCE: sha(ROOT / CONTROL_SOURCE),
                    args.pilot_source.relative_to(ROOT).as_posix(): sha(args.pilot_source)},
        'call_evidence': [{'symbol': symbol, 'rva': hex(rva), 'size': size,
                          'calls': [{'operand_offset': o, 'symbol': n, 'destination_rva': hex(d)}
                                    for o, n, d in calls]}
                         for symbol, rva, size, calls in CONTROL],
        'symbol_order': [s for s, *_ in PILOT] + [s for s, *_ in CONTROL],
        'configurations': [],
    }
    for tc in manifest['toolchains']:
        for flags in FLAGS:
            row = {'toolchain': tc['id'], 'flags': flags, 'functions': []}
            for source, specs in [(args.pilot_source, PILOT),
                                  (ROOT / CONTROL_SOURCE, CONTROL)]:
                obj, _ = compile_source(source, flags, args.tools_root / tc['directory'])
                coff = COFF(obj)
                for symbol, rva, size, *_ in specs:
                    function = coff.function(symbol)
                    raw = function['data']
                    expected = pe.read_rva(rva, size)
                    resolved = bytearray(raw)
                    for relocation in function['relocations']:
                        offset, name = relocation['offset'], relocation['symbol']
                        if relocation['type'] != 0x14 or name not in destinations:
                            raise ValueError(f'Unproved relocation: {relocation}')
                        if resolved[offset - 1] not in (0xe8, 0xe9):
                            raise ValueError('Expected direct call/tail-jump relocation')
                        addend = struct.unpack_from('<i', resolved, offset)[0]
                        struct.pack_into('<i', resolved, offset,
                                         destinations[name] - (rva + offset + 4) + addend)
                    relocs = len(function['relocations'])
                    row['functions'].append({
                        'symbol': symbol, 'compiled_size': len(raw), 'target_size': size,
                        'raw_different_bytes': distance(raw, expected),
                        'relocations': relocs,
                        'strict_equal': raw == expected and relocs == 0,
                        'diagnostic_resolved_different_bytes': distance(resolved, expected),
                    })
            report['configurations'].append(row)
            strict = sum(f['strict_equal'] for f in row['functions'])
            diagnostic = sum(f['diagnostic_resolved_different_bytes'] == 0
                             for f in row['functions'] if f['relocations'])
            print(f"{tc['id']} {' '.join(flags):18s} strict={strict}/4 call-diagnostic={diagnostic}/2")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(report, indent=2) + '\n').encode('utf-8'))


if __name__ == '__main__':
    main()
