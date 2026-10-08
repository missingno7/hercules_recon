"""Relocation-masked diagnostic comparison of one compiled function contribution.

Bytes outside relocation fields must be equal. Each relocation is resolved from the
target bytes into the address its symbol must have; one symbol must always imply
one address, and a reference to the function's own section must imply its own RVA.
The resulting symbol map is link evidence. This is never FUNCTION_MATCH.
"""
import struct

from coff import COFF
from pe import PE

REL32, DIR32, DIR32NB, SECTION = 0x14, 0x06, 0x07, 0x0a


def masked_compare(obj, symbol, target, rva, size):
    coff = COFF(obj)
    section = coff.function(symbol)
    number = coff.sections.index(section) + 1
    actual = section['data']
    pe = PE(target)
    expected = pe.read_rva(rva, size)
    fields, implied, conflicts, unsupported = set(), {}, [], []
    for r in section['relocations']:
        offset, kind, name = r['offset'], r['type'], r['symbol']
        if kind not in (REL32, DIR32):
            unsupported.append(r)
            continue
        fields.update(range(offset, offset + 4))
        if offset + 4 > len(expected):
            conflicts.append(dict(symbol=name, offset=offset, reason='relocation outside target extent'))
            continue
        addend = struct.unpack_from('<i', actual, offset)[0]
        value = struct.unpack_from('<I', expected, offset)[0]
        if kind == REL32:
            address = (rva + offset + 4 + struct.unpack_from('<i', expected, offset)[0] - addend) & 0xffffffff
        else:
            address = (value - pe.image_base - addend) & 0xffffffff
        sym = next((s for s in coff.symbols.values() if s['name'] == name), None)
        if sym and sym['section'] == number:
            # Own COMDAT section (switch tables, local labels): base is the function itself.
            address = (address - sym['value']) & 0xffffffff
            if address != rva:
                conflicts.append(dict(symbol=name, offset=offset, implied=hex(address), reason='own section'))
            continue
        if name in implied and implied[name] != address:
            conflicts.append(dict(symbol=name, offset=offset, implied=hex(address), previous=hex(implied[name])))
        implied.setdefault(name, address)
    differing = [i for i in range(max(len(expected), len(actual)))
                 if i not in fields and expected[i:i + 1] != actual[i:i + 1]]
    regions = []
    for i in differing:
        if regions and regions[-1][1] == i:
            regions[-1][1] = i + 1
        else:
            regions.append([i, i + 1])
    target_relocs = {e.rva - rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries
                     if e.type and rva <= e.rva < rva + size}
    dir32 = {r['offset'] for r in section['relocations'] if r['type'] == DIR32}
    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size and target_relocs != dir32:
        conflicts.append(dict(reason='absolute relocation offsets differ from target base relocations',
                              compiled=sorted(dir32), target=sorted(target_relocs)))
    equal = len(actual) == len(expected) and not differing and not conflicts and not unsupported
    return dict(masked_equal=equal, compiled_size=len(actual), target_size=len(expected),
                differing_bytes=len(differing), mismatch_regions=regions,
                symbols={k: hex(v) for k, v in sorted(implied.items())}, conflicts=conflicts,
                unsupported=[dict(u) for u in unsupported], relocations=len(section['relocations']))
