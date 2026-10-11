"""Relocation-masked diagnostics for complete function or shared-text contributions.

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
    pe = PE(target)
    return _compare_section(coff, section, number, pe, rva, size)


def _compare_section(coff, section, number, pe, rva, size):
    """Shared operand comparison; callers establish the contribution boundary."""
    actual = section['data']
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
            # Own code section (switch tables, local labels): check the complete contribution base.
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


def masked_unit_compare(obj, target, rva, size, expected_functions):
    """Read-only full shared-text diagnostic; never a function or unit acceptance.

    Require the complete object's code and an explicit native-derived function
    offset inventory. No function slicing, RET boundary guessing, object changes,
    or relaxation of COFF.function/the existing staging and promotion gates.
    Initialized sections referenced by code must also have one consistent base
    and match completely, including compiler-emitted alignment bytes.
    """
    coff, pe = COFF(obj), PE(target)
    code = [(n, s) for n, s in enumerate(coff.sections, 1) if s['flags'] & 0x20 and s['size']]
    if len(code) != 1 or code[0][1]['flags'] & 0x1000:
        raise ValueError('Unit diagnostic requires exactly one complete non-COMDAT code section')
    number, section = code[0]
    if section['flags'] & 0x20000020 != 0x20000020 or len(section['data']) != section['size']:
        raise ValueError('Unit text must be complete executable code')
    functions = [s for s in coff.symbols.values() if s['section'] > 0 and s['type'] & 0x20]
    offsets = {s['name']: s['value'] for s in functions}
    if (len(functions) < 2 or len(offsets) != len(functions) or
            any(s['section'] != number for s in functions) or offsets != expected_functions or
            len(set(offsets.values())) != len(offsets) or min(offsets.values()) != 0 or
            any(v < 0 or v >= section['size'] for v in offsets.values())):
        raise ValueError('Complete function inventory/offsets differ from the explicit unit context')
    if size <= 0 or not any(s['characteristics'] & 0x20000000 and
                           s['virtual_address'] <= rva and
                           rva + size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])
                           for s in pe.sections):
        raise ValueError('Target unit interval must be wholly inside file-backed executable code')
    for rel in section['relocations']:
        if rel['offset'] < 0 or rel['offset'] + 4 > len(section['data']):
            raise ValueError('Unit relocation exceeds the complete code contribution')
    result = _compare_section(coff, section, number, pe, rva, size)
    bases, conflicts = {}, []
    for name, address in result['symbols'].items():
        sym = next((s for s in coff.symbols.values() if s['name'] == name), None)
        if sym and sym['section'] > 0 and sym['section'] != number:
            base = (int(address, 0) - sym['value']) & 0xffffffff
            other = sym['section']
            if other in bases and bases[other] != base:
                conflicts.append(dict(section=other, symbol=name, reason='defined section bases differ'))
            bases.setdefault(other, base)
    data, unmapped = [], []
    for n, sec in enumerate(coff.sections, 1):
        if n == number or not sec['size'] or sec['name'] == '.drectve' or sec['name'].startswith('.debug'):
            continue
        if n not in bases:
            unmapped.append(dict(section=n, name=sec['name'], bytes=sec['size']))
            continue
        if sec['relocations'] or len(sec['data']) != sec['size']:
            conflicts.append(dict(section=n, reason='mapped data needs a separate relocation/zero-storage proof'))
            continue
        base = bases[n]
        if not any(s['virtual_address'] <= base and
                   base + sec['size'] <= s['virtual_address'] + s['raw_size'] for s in pe.sections):
            conflicts.append(dict(section=n, reason='mapped data exceeds file-backed target data'))
            continue
        native = pe.read_rva(base, sec['size'])
        data.append(dict(section=n, name=sec['name'], rva=hex(base), bytes=sec['size'], equal=native == sec['data']))
    return dict(result, scope='complete shared-text and mapped initialized-section diagnostic only; not acceptance',
                function_offsets=offsets, text_flags=hex(section['flags']), mapped_data=data,
                unmapped_sections=unmapped, section_conflicts=conflicts,
                unit_equal=result['masked_equal'] and not conflicts and not unmapped and all(d['equal'] for d in data))
