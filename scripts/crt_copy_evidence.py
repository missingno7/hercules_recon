"""Recheck the historical CRT copy member identity; never patch or execute code."""
import json
import struct
from pathlib import Path

from env import ROOT
from match import DEFAULT_TOOLCHAIN, digest, verify_toolchain
from coff import COFF
from pe import PE


def members(data):
    if data[:8] != b'!<arch>\n':
        raise ValueError('Not a COFF archive')
    position = 8
    while position < len(data):
        header = data[position:position + 60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise ValueError('Invalid archive member')
        size = int(header[48:58])
        start = position + 60
        payload = data[start:start + size]
        if len(payload) != size:
            raise ValueError('Truncated archive member')
        yield payload
        position = start + size + (size & 1)


def verify():
    verify_toolchain(DEFAULT_TOOLCHAIN)
    evidence = json.loads((ROOT / 'evidence/crt_copy.json').read_text())
    path = ROOT / evidence['target']
    if digest(path.read_bytes()) != evidence['target_sha256']:
        raise ValueError('Changed CRT oracle')
    image = PE(path)
    rva = int(evidence['target_rva'], 0)
    target = image.read_rva(rva, evidence['size'])
    scratch = ROOT / 'work/crt_copy_verify'
    scratch.mkdir(parents=True, exist_ok=True)
    libraries = {}
    for row in evidence['comparisons']:
        name = row['library']
        if name not in libraries:
            archive = (DEFAULT_TOOLCHAIN / 'lib' / name).read_bytes()
            if digest(archive) != row['library_sha256']:
                raise ValueError('Changed historical library')
            libraries[name] = {digest(member): member for member in members(archive)}
        member = libraries[name][row['member_sha256']]
        object_path = scratch / row['member']
        object_path.write_bytes(member)
        obj = COFF(object_path)
        sections = [(i, s) for i, s in enumerate(obj.sections) if s['flags'] & 0x20]
        if len(sections) != 1:
            raise ValueError('Unexpected executable sections')
        index, section = sections[0]
        if section['size'] != len(target) or digest(target) != row['oracle_span_sha256']:
            raise ValueError('Changed CRT extent')
        operands = set()
        for relocation in section['relocations']:
            if relocation['type'] != 6:
                raise ValueError('Unexpected relocation')
            symbols = [s for s in obj.symbols.values() if s['name'] == relocation['symbol'] and s['section'] == index + 1]
            if len(symbols) != 1:
                raise ValueError('Unresolved section-local operand')
            offset = relocation['offset']
            addend = struct.unpack_from('<I', section['data'], offset)[0]
            expected = (image.image_base + rva + symbols[0]['value'] + addend) & 0xffffffff
            if struct.unpack_from('<I', target, offset)[0] != expected:
                raise ValueError('CRT operand differs')
            operands.update(range(offset, offset + 4))
        if len(section['relocations']) != row['relocations']:
            raise ValueError('Changed relocation count')
        if any(a != b for i, (a, b) in enumerate(zip(section['data'], target)) if i not in operands):
            raise ValueError('CRT non-relocation bytes differ')
    result = dict(members=len(evidence['comparisons']), bytes=len(target), operands_per_member=46,
                  scope='Historical library identity only; no FUNCTION_MATCH, patched object or original execution.')
    print(json.dumps(result, indent=2))
    return result


if __name__ == '__main__':
    verify()
