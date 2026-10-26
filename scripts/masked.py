"""Relocation-masked diagnostics for complete function or shared-text contributions.

Bytes outside relocation fields must be equal. Each relocation is resolved from the
target bytes into the address its symbol must have; one symbol must always imply
one address, and a reference to the function's own section must imply its own RVA.
The resulting symbol map is link evidence. This is never FUNCTION_MATCH.
"""
import struct
import hashlib
import json

from coff import COFF
from pe import PE

REL32, DIR32, DIR32NB, SECTION = 0x14, 0x06, 0x07, 0x0a


def binding_records(bindings):
    """Share the comparator's cheap document guard with pre-compile readers."""
    if not isinstance(bindings, dict) or set(bindings) - {'symbols', 'tables', 'indirect_calls'}:
        raise ValueError('Invalid binding document')
    records = bindings.get('symbols', {})
    if not isinstance(records, dict):
        raise ValueError('Invalid symbol bindings')
    return records


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


def relocated_compare(obj, symbol, target, rva, size, bindings, storage_owners=None, storage_aliases=None):
    """Compare one complete compiler contribution with independently reviewed fixups.

    ``bindings`` is {"symbols": {COFF_NAME: RECORD}, "tables": [TABLE, ...]}.
    Each RECORD has kind (function/data/bss/import/local), rva, size, evidence (a
    nonempty list of durable reference strings), and references: a list of
    {offset, type, addend} exactly describing its COFF uses. Numbers are integers.
    A reviewed use may explicitly set one_past:true for a data end pointer;
    this requires addend==size and a DIR32 immediate, never a memory displacement.
    Data additionally has sha256 of its complete original storage payload.
    BSS requires a separately freshly compiled complete owner plus an independent
    native absolute-address instruction anchor; it is never inferred from zeros.
    storage_owners maps each BSS symbol to its owner object. Explicit WORD aliases
    additionally need storage_aliases: freshly compiled include-adapted objects
    equal across every function, storage contribution and external symbol after
    only the recorded renaming. Ordinary aliases remain rejected. Build provenance is
    caller-owned, just as for obj; match.py freezes and recompiles those inputs.
    Imports have dll and exactly one of name/ordinal. Local records have role
    code/table/section and identify symbols in this same compiler contribution.
    Section records delimit this whole contribution; reviewed addends identify
    its code/table destinations (including ordinary COFF section-symbol uses). TABLE
    has offset, size and evidence and delimits an embedded DIR32 jump table.

    This routine never obtains identity from masked_compare's implied symbols.
    The caller owns review of evidence, target identity, and fresh pinned builds;
    equality here alone cannot establish that provenance. Unsupported constructs
    raise ValueError, byte/reference mismatches return relocated_equal=False.
    Only /Gy complete contributions are currently supported, including all their
    compiler padding; no target-sized prefix or guessed RET boundary is admitted.
    """
    # env has already been loaded by pe.py; do not require a new dependency path.
    import capstone

    def integer(value, label, minimum=0):
        if type(value) is not int or value < minimum:
            raise ValueError('Invalid integer ' + label)
        return value

    def evidence(record):
        refs = record.get('evidence')
        if not isinstance(refs, list) or not refs or any(not isinstance(s, str) or not s.strip() for s in refs):
            raise ValueError('Missing independent evidence reference')

    def interval(start, length, executable=None):
        integer(start, 'RVA')
        integer(length, 'extent', 1)
        choices = [s for s in pe.sections if s['virtual_address'] <= start and
                   start + length <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])]
        if len(choices) != 1 or (executable is not None and
                bool(choices[0]['characteristics'] & 0x20000000) != executable):
            raise ValueError('Extent must be wholly in unique file-backed ' +
                             ('executable code' if executable else 'storage'))
        return pe.read_rva(start, length)

    coff, pe = COFF(obj), PE(target)
    if pe.pe.FILE_HEADER.Machine != 0x14c or pe.pe.OPTIONAL_HEADER.Magic != 0x10b:
        raise ValueError('Relocated comparison supports i386 PE32 only')
    section = coff.function(symbol)
    number = coff.sections.index(section) + 1
    actual = section['data']
    if len(actual) != section['size'] or size != section['size'] or size <= 0:
        raise ValueError('Complete compiler-owned function extent must equal target extent')
    expected = interval(rva, size, True)
    records = binding_records(bindings)
    by_name = {}
    for s in coff.symbols.values():
        by_name.setdefault(s['name'], []).append(s)
    referenced_names = {rel['symbol'] for rel in section['relocations']}
    used_names = referenced_names | {symbol}
    if any(len(by_name.get(name, [])) != 1 for name in used_names):
        raise ValueError('Missing or ambiguous duplicate COFF symbol')
    if set(records) != referenced_names:
        raise ValueError('Binding inventory differs from referenced COFF symbols')

    table_bytes, table_starts = set(), set()
    tables = bindings.get('tables', [])
    if not isinstance(tables, list):
        raise ValueError('Invalid table inventory')
    for table in tables:
        evidence(table)
        off, length = integer(table['offset'], 'table offset'), integer(table['size'], 'table size', 1)
        cells = set(range(off, off + length))
        if length % 4 or off + length > size or table_bytes & cells:
            raise ValueError('Invalid or overlapping jump table bounds')
        table_bytes |= cells
        table_starts.update(range(off, off + length, 4))

    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    instructions = []
    off = 0
    while off < size:
        if off in table_bytes:
            off += 1
            continue
        end = off
        while end < size and end not in table_bytes:
            end += 1
        decoded = list(decoder.disasm(actual[off:end], rva + off))
        if sum(i.size for i in decoded) != end - off:
            raise ValueError('Incomplete instruction decode outside declared tables')
        instructions.extend(decoded)
        off = end
    starts = {i.address - rva for i in instructions}
    operand_fields = {}
    for ins in instructions:
        for field_off, field_size in ((ins.imm_offset, ins.imm_size), (ins.disp_offset, ins.disp_size)):
            if field_size == 4:
                operand_fields[ins.address - rva + field_off] = ins

    reviewed, bases, errors, receipts, storage_extents = {}, {}, [], [], []
    alias_checks = {}
    for name, record in records.items():
        if not isinstance(record, dict):
            raise ValueError('Invalid symbol record')
        evidence(record)
        dest, extent = integer(record['rva'], 'symbol RVA'), integer(record['size'], 'symbol extent', 1)
        kind, sym = record['kind'], by_name[name][0]
        if sym['section'] < 0 or sym.get('storage', 2) == 105:
            raise ValueError('Absolute, debug and weak/alias symbols need an explicit verifier')
        if kind == 'function':
            interval(dest, extent, True)
            if not sym['type'] & 0x20:
                raise ValueError('Function binding contradicts COFF symbol type')
            if name == symbol and (dest != rva or extent != size):
                raise ValueError('Recursive function binding differs from complete function')
        elif kind == 'data':
            payload = interval(dest, extent, False)
            if sym['type'] & 0x20:
                raise ValueError('Data binding contradicts function type')
            if record.get('sha256') != hashlib.sha256(payload).hexdigest():
                errors.append('Data payload identity differs: ' + name)
            if sym['section']:
                sec = coff.sections[sym['section'] - 1]
                pos = sym['value']
                if sym['section'] == number or sec['flags'] & 0x20 or pos + extent > len(sec['data']):
                    raise ValueError('Defined data storage extent is incomplete or executable')
                if any(rel['offset'] < pos + extent and rel['offset'] + 4 > pos for rel in sec['relocations']):
                    raise ValueError('Relocated data payload requires separate verification')
                if sec['data'][pos:pos + extent] != payload:
                    errors.append('Compiled data payload differs: ' + name)
        elif kind == 'bss':
            from zero_storage import verify_common, verify_zero_section, verify_native
            if sym['type'] & 0x20 or sym['section'] != 0 or sym['value'] not in (0, extent):
                raise ValueError('Loader-zero caller symbol must be undefined or exact-size COMMON data')
            owner = (storage_owners or {}).get(name)
            owner_name = record.get('owner', {}).get('symbol')
            if owner is None or not isinstance(owner_name, str):
                raise ValueError('Loader-zero binding needs its independently compiled owner object')
            if owner_name != name:
                from storage_alias import alias_names, verify_alias_object, verify_owner_alias_object
                names = alias_names(record, name)
                adapted = (storage_aliases or {}).get(name)
                if adapted is None:
                    raise ValueError('Loader-zero aliases need a freshly compiled rename object')
                if record['source_alias']['kind'] == 'owner-compile-renaming':
                    alias_checks[name] = verify_owner_alias_object(obj, owner, adapted, names)
                else:
                    alias_checks[name] = verify_alias_object(obj, adapted, symbol, names)
            elif 'source_alias' in record:
                raise ValueError('Same-name loader-zero owner cannot carry an alias adapter')
            representation = record.get('owner_storage')
            if representation == 'zero-section-v1':
                verify_zero_section(owner, owner_name, extent)
            elif 'owner_storage' not in record:
                verify_common(owner, owner_name, extent)
            else:
                raise ValueError('Unsupported loader-zero owner storage representation')
            verify_native(pe, dest, extent, record.get('identity'), rva, size)
        elif kind == 'import':
            if sym['section'] or extent != 4 or bool(record.get('name')) == ('ordinal' in record):
                raise ValueError('Import must be an undefined exact four-byte IAT identity')
            interval(dest, 4, False)
            identities = [(module.dll.decode('ascii').lower(), i.name.decode('ascii') if i.name else None,
                           i.ordinal, i.address - pe.image_base)
                          for module in getattr(pe.pe, 'DIRECTORY_ENTRY_IMPORT', []) for i in module.imports]
            matches = [i for i in identities if i[0] == record['dll'].lower() and i[3] == dest and
                       (i[1] == record['name'] if 'name' in record else i[1] is None and i[2] == record['ordinal'])]
            if len(matches) != 1:
                raise ValueError('Independent import identity differs from PE IAT')
        elif kind == 'local':
            if sym['section'] != number or dest != rva + sym['value'] or dest + extent > rva + size:
                raise ValueError('Own-section binding differs from complete contribution')
            role = record.get('role')
            positions = set(range(dest - rva, dest - rva + extent))
            if role == 'code':
                if dest - rva not in starts or positions & table_bytes:
                    raise ValueError('Local code binding does not identify instruction storage')
            elif role == 'table':
                if not positions <= table_bytes:
                    raise ValueError('Local table binding exceeds declared jump table')
            elif role == 'section':
                if sym['value'] != 0 or dest != rva or extent != size:
                    raise ValueError('Section binding must cover this complete contribution')
            else:
                raise ValueError('Unsupported local symbol role')
        else:
            raise ValueError('Unsupported binding kind')
        if kind != 'local':
            if name != symbol and dest < rva + size and dest + extent > rva:
                raise ValueError('Nonlocal symbol overlaps the compared function; alias proof unsupported')
            if any(dest < previous_end and dest + extent > previous_start
                   for previous_start, previous_end in storage_extents):
                raise ValueError('Overlapping symbol bindings require an explicit alias verifier')
            storage_extents.append((dest, dest + extent))
        if sym['section'] > 0:
            base = dest - sym['value']
            if sym['section'] in bases and bases[sym['section']] != base:
                raise ValueError('Inconsistent independently bound COFF section addresses')
            bases[sym['section']] = base
            if sym['section'] == number and base != rva:
                raise ValueError('Own-section symbol has inconsistent address')
        uses = record.get('references')
        if not isinstance(uses, list) or not uses:
            raise ValueError('Missing reviewed relocation uses')
        for use in uses:
            uoff = integer(use['offset'], 'relocation offset')
            typ = integer(use['type'], 'relocation type')
            addend = integer(use['addend'], 'relocation addend', -0x80000000)
            one_past = use.get('one_past', False)
            if type(one_past) is not bool:
                raise ValueError('Reviewed one_past must be an explicit boolean')
            if one_past and (kind != 'data' or addend != extent or typ != DIR32):
                raise ValueError('Reviewed one-past use must identify exactly the data object end')
            if addend > 0x7fffffff or uoff in reviewed:
                raise ValueError('Conflicting or duplicate reviewed relocation')
            reviewed[uoff] = (name, typ, addend, one_past)

    fields, actual_offsets = set(), set()
    for rel in section['relocations']:
        off, typ, name = rel['offset'], rel['type'], rel['symbol']
        integer(off, 'COFF relocation offset')
        cells = set(range(off, off + 4))
        if typ not in (REL32, DIR32) or off + 4 > size or fields & cells:
            raise ValueError('Unsupported, overlapping or out-of-bounds COFF relocation')
        if off not in operand_fields and off not in table_starts:
            raise ValueError('Relocation is not one complete instruction operand or table entry')
        addend = struct.unpack_from('<i', actual, off)[0]
        if off not in reviewed or reviewed[off][:3] != (name, typ, addend):
            raise ValueError('COFF relocation differs from reviewed symbol/type/addend use')
        record = records[name]
        kind = record['kind']
        if kind in ('function', 'import') and addend != 0:
            raise ValueError('Function/import interior addends need separate verification')
        one_past = reviewed[off][3]
        if kind in ('data', 'bss', 'local') and not (0 <= addend < record['size'] or one_past):
            raise ValueError('Interior offset exceeds independently bound object')
        destination = record['rva'] + addend
        if (kind == 'local' and record.get('role') == 'section' and
                destination - rva not in starts and destination - rva not in table_starts):
            raise ValueError('Section-symbol addend does not identify an instruction or table entry')
        ins = operand_fields.get(off)
        if kind == 'bss':
            if typ != DIR32 or ins is None:
                raise ValueError('Loader-zero references require direct absolute instruction operands')
            if ins.disp_size == 4 and ins.address - rva + ins.disp_offset == off:
                mem = [op for op in ins.operands if op.type == capstone.x86.X86_OP_MEM]
                if (len(mem) != 1 or mem[0].mem.base or mem[0].mem.index or mem[0].mem.segment or
                        addend + mem[0].size > record['size']):
                    raise ValueError('Loader-zero memory access exceeds its complete scalar owner')
        if one_past and (ins is None or ins.imm_size != 4 or ins.address - rva + ins.imm_offset != off):
            raise ValueError('Reviewed one-past pointer requires an immediate operand, not memory')
        if (typ == DIR32 and ins is not None and
                (ins.group(capstone.CS_GRP_CALL) or ins.group(capstone.CS_GRP_JUMP)) and
                ins.operands[0].type == capstone.x86.X86_OP_IMM):
            raise ValueError('Absolute relocation cannot encode a direct relative control transfer')
        if typ == REL32:
            if kind != 'function' or ins is None or ins.size != 5 or ins.bytes[0] not in (0xe8, 0xe9) or ins.imm_offset != 1:
                raise ValueError('REL32 currently requires a direct function-entry call/jump')
            native = (rva + off + 4 + struct.unpack_from('<i', expected, off)[0]) & 0xffffffff
        else:
            native = (struct.unpack_from('<I', expected, off)[0] - pe.image_base) & 0xffffffff
        if off in table_starts:
            if typ != DIR32 or kind != 'local' or record.get('role') not in ('code', 'section') or destination - rva not in starts:
                raise ValueError('Jump table entries must point to own decoded instruction boundaries')
        if native != destination:
            errors.append('Relocation destination differs at ' + hex(off) + ': ' + name)
        fields |= cells
        actual_offsets.add(off)
        receipts.append(dict(offset=off, type=typ, symbol=name, addend=addend,
                             kind=kind, destination_rva=destination, target_destination_rva=native,
                             evidence=record['evidence'], one_past=one_past))
    if set(reviewed) != actual_offsets or not table_starts <= actual_offsets:
        raise ValueError('Reviewed relocation/table entry inventory differs from COFF')
    from callback_transfer import verify_calls
    callback_calls = verify_calls(pe, bindings, records, reviewed, instructions, rva, size, receipts)
    for ins in instructions:
        if not (ins.group(capstone.CS_GRP_CALL) or ins.group(capstone.CS_GRP_JUMP)):
            continue
        operand = ins.operands[0] if ins.operands else None
        if operand is None:
            raise ValueError('Unclassified control transfer')
        offset = ins.address - rva
        if operand.type == capstone.x86.X86_OP_IMM:
            if offset + ins.imm_offset not in actual_offsets and operand.imm - rva not in starts:
                raise ValueError('Unbound external or non-instruction direct control transfer')
        elif operand.type == capstone.x86.X86_OP_MEM:
            use = reviewed.get(offset + ins.disp_offset)
            record = records[use[0]] if use else {}
            mem = operand.mem
            valid_import = record.get('kind') == 'import' and mem.base == 0 and mem.index == 0
            table_destination = record.get('rva', -1) + (use[2] if use else 0) - rva
            valid_table = (record.get('kind') == 'local' and record.get('role') in ('table', 'section') and
                           table_destination in table_starts and
                           ins.group(capstone.CS_GRP_JUMP) and mem.base == 0 and mem.index != 0 and mem.scale == 4)
            valid_callback = offset in callback_calls and ins.group(capstone.CS_GRP_CALL)
            if not (valid_import or valid_table or valid_callback):
                raise ValueError('Unsupported indirect control transfer')
        else:
            raise ValueError('Register indirect control transfer needs separate verification')

    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size:
        native_relocs = [(e.rva - rva, e.type) for block in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                         for e in block.entries if e.type and rva - 3 <= e.rva < rva + size]
        wanted = sorted((rel['offset'], 3) for rel in section['relocations'] if rel['type'] == DIR32)
        if sorted(native_relocs) != wanted:
            raise ValueError('Original HIGHLOW relocation inventory differs from compiler DIR32 fields')
    different = [i for i in range(size) if i not in fields and actual[i] != expected[i]]
    result = dict(relocated_equal=not different and not errors, compiled_size=size, target_size=size,
                differing_nonreference_bytes=len(different), errors=errors, references=receipts,
                compiled_sha256=hashlib.sha256(actual).hexdigest(), target_sha256=hashlib.sha256(expected).hexdigest(),
                bindings_sha256=hashlib.sha256(json.dumps(bindings, sort_keys=True, separators=(',', ':')).encode()).hexdigest(),
                scope='complete compiler contribution and independently supplied reference bindings; build provenance is caller-owned')
    if alias_checks:
        result['source_alias_checks'] = alias_checks
    if callback_calls:
        result['indirect_call_checks'] = list(callback_calls.values())
    return result
