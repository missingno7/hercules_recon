"""Opt-in complete compiler-local byte table proof; default comparator unchanged.

The comparator is the pinned masked.relocated_compare plus explicit table and
access checks. A parity test protects every unchanged line. These embedded
allocations are delimited by complete COFF symbol/section boundaries, including
padding, rather than an incomplete native index. This proves bytes, not a C type
or the runtime range of the index.
"""
import hashlib
import json
import struct
from coff import COFF
from pe import PE
from masked import binding_records, REL32, DIR32

PROFILE = 'coff-local-byte-table-v1'


def verify_table(coff, section, number, pe, rva, table, records):
    if set(table) != {'profile', 'symbol', 'offset', 'size', 'sha256', 'evidence'}:
        raise ValueError('Invalid byte table declaration')
    name, off, size = table['symbol'], table['offset'], table['size']
    if type(off) is not int or type(size) is not int or off < 0 or not 1 <= size <= 65536:
        raise ValueError('Invalid bounded byte table extent')
    choices = [s for s in coff.symbols.values() if s['name'] == name]
    if len(choices) != 1:
        raise ValueError('Byte table requires a unique COFF label')
    sym = choices[0]
    if (not name.startswith('$L') or sym['section'] != number or sym['value'] != off or
            sym['type'] or sym['storage'] != 6 or sym['aux']):
        raise ValueError('Byte table requires a plain compiler-local label in own section')
    labels = [s for s in coff.symbols.values() if s['section'] == number and
              s['storage'] != 3 and s['value'] >= off]
    end = min([s['value'] for s in labels if s['value'] > off] + [section['size']])
    if sum(s['value'] == off for s in labels) != 1 or off + size != end:
        raise ValueError('Byte table must cover complete COFF allocation including padding')
    if len(section['data']) != section['size'] or off + size > section['size']:
        raise ValueError('Incomplete compiler section')
    if any(rel['offset'] < end and off < rel['offset'] + 4 for rel in section['relocations']):
        raise ValueError('Byte tables cannot contain relocations')
    record = records.get(name, {})
    if (record.get('kind') != 'local' or record.get('role') != 'table' or
            record.get('rva') != rva + off or record.get('size') != size):
        raise ValueError('Byte table binding must cover the complete allocation')
    payload = section['data'][off:end]
    if hashlib.sha256(payload).hexdigest() != table['sha256']:
        raise ValueError('Byte table hash differs from complete compiler allocation')
    # Bounded streaming section search; retain at most two hits, never offsets
    # for a whole image. Reject duplicate content anywhere in file-backed image.
    hits = []
    for sec in pe.sections:
        length = min(sec['virtual_size'], sec['raw_size'])
        if length <= 0:
            continue
        data = pe.read_rva(sec['virtual_address'], length)
        pos = data.find(payload)
        while pos >= 0:
            hits.append(sec['virtual_address'] + pos)
            if len(hits) > 1:
                raise ValueError('Complete byte table has ambiguous native placement')
            pos = data.find(payload, pos + 1)
    if hits != [rva + off]:
        raise ValueError('Complete byte table differs or has wrong native placement')
    native_relocs = [e.rva for block in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                     for e in block.entries if e.type and rva + off - 3 <= e.rva < rva + end]
    if native_relocs:
        raise ValueError('Native byte table overlaps relocation fields')
    return dict(symbol=name, offset=off, size=size, rva=rva + off,
                sha256=table['sha256'], extent_basis='complete COFF allocation', unique=True)


def verify_accesses(tables, records, operand_fields, rva):
    import capstone
    for table in tables:
        uses = records[table['symbol']].get('references', [])
        if not uses:
            raise ValueError('Byte table needs complete indexed load uses')
        for use in uses:
            ins = operand_fields.get(use['offset'])
            if (ins is None or use['type'] != DIR32 or use['addend'] != 0 or
                    ins.mnemonic not in ('mov', 'movzx', 'movsx') or
                    ins.address - rva + ins.disp_offset != use['offset'] or ins.disp_size != 4 or
                    len(ins.operands) != 2 or ins.operands[0].type != capstone.x86.X86_OP_REG):
                raise ValueError('Byte table use must be a direct indexed byte load')
            mem = ins.operands[1]
            if (mem.type != capstone.x86.X86_OP_MEM or mem.size != 1 or
                    bool(mem.mem.base) == bool(mem.mem.index) or mem.mem.scale != 1 or mem.mem.segment):
                raise ValueError('Byte table use must be a direct indexed byte load')


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
    has offset, size and evidence and delimits an embedded DIR32 jump table;
    coff-local-byte-table-v1 additionally declares a complete relocation-free
    byte allocation with its symbol and SHA-256. Each local table record must
    equal exactly one declared table, never a subrange or a union of tables.

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
    if 'coff_identity' in bindings:
        from coff_identity import verify_pair
        canonical = verify_pair(coff, symbol, bindings['coff_identity'])
        by_name[symbol] = [canonical]
    if any(len(by_name.get(name, [])) != 1 for name in used_names):
        raise ValueError('Missing or ambiguous duplicate COFF symbol')
    if set(records) != referenced_names:
        raise ValueError('Binding inventory differs from referenced COFF symbols')

    table_bytes, table_starts = set(), set()
    byte_tables = []
    tables = bindings.get('tables', [])
    if not isinstance(tables, list):
        raise ValueError('Invalid table inventory')
    for table in tables:
        evidence(table)
        off, length = integer(table['offset'], 'table offset'), integer(table['size'], 'table size', 1)
        if table.get('profile') == PROFILE and (off + length > size or length > 65536):
            raise ValueError('Invalid bounded byte table extent')
        cells = set(range(off, off + length))
        if table.get('profile') == PROFILE:
            byte_tables.append(verify_table(coff, section, number, pe, rva, table, records))
            if table_bytes & cells:
                raise ValueError('Overlapping byte table')
            table_bytes |= cells
            continue
        if 'profile' in table:
            raise ValueError('Unsupported explicit table profile')
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
    alias_checks, data_checks = {}, []
    payload_identities = {name: (rec.get('kind'), rec.get('rva'), rec.get('size'), rec.get('sha256'),
                              rec.get('dll'), rec.get('name'), rec.get('ordinal'))
                          for name, rec in records.items() if isinstance(rec, dict)}
    for name, record in records.items():
        if not isinstance(record, dict):
            raise ValueError('Invalid symbol record')
        evidence(record)
        dest, extent = integer(record['rva'], 'symbol RVA'), integer(record['size'], 'symbol extent', 1)
        kind, sym = record['kind'], by_name[name][0]
        if 'data_profile' in record and (kind != 'data' or record['data_profile'] != 'relocated-data-v1'):
            raise ValueError('Unsupported explicit data profile')
        if 'anchor_profile' in record and (kind != 'bss' or record['anchor_profile'] != 'cmp-test-absolute-v1'):
            raise ValueError('Unsupported explicit loader-zero anchor profile')
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
                    from relocated_data import verify_payload
                    # Payloads can point back to this function even when the
                    # code has no self-reference binding of its own.
                    bases.setdefault(number, rva)
                    data_checks.append(verify_payload(coff, pe, name, record, interval, bases, errors,
                                                      payload_identities))
                elif 'data_profile' in record:
                    raise ValueError('Relocated data profile requires a relocation-bearing defined payload')
                elif sec['data'][pos:pos + extent] != payload:
                    errors.append('Compiled data payload differs: ' + name)
            elif 'data_profile' in record:
                raise ValueError('Relocated data profile requires a caller-defined payload')
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
            if record.get('anchor_profile') == 'cmp-test-absolute-v1':
                from zero_compare_anchor import verify_native as verify_compare_native
                verify_compare_native(pe, dest, extent, record.get('identity'), rva, size)
            else:
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
                if sum(table['offset'] == dest - rva and table['size'] == extent
                       for table in tables) != 1:
                    raise ValueError('Local table binding must equal exactly one declared table')
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

    verify_accesses(byte_tables, records, operand_fields, rva)
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
    # callback_transfer.py is pinned by accepted proofs; dispatch the separate
    # fixed-slot profile here and pass only the other records to it.
    calls = bindings.get('indirect_calls', [])
    if not isinstance(calls, list):
        raise ValueError('Invalid explicit callback inventory')
    fixed = [c for c in calls if isinstance(c, dict) and c.get('profile') == 'fixed-slot-v1']
    rooted = [c for c in calls if c not in fixed]
    callback_calls = verify_calls(pe, dict(bindings, indirect_calls=rooted), records, reviewed,
                                  instructions, rva, size, receipts)
    if fixed:
        from fixed_slot import verify_call
        for record in fixed:
            if record['offset'] in callback_calls:
                raise ValueError('Invalid/duplicate fixed-slot offset')
            callback_calls[record['offset']] = verify_call(pe, record, records, reviewed, instructions, rva, size)
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
            proof = callback_calls.get(offset, {})
            valid_callback = bool(proof) and (ins.group(capstone.CS_GRP_CALL) or
                             proof.get('profile') == 'fixed-slot-v1' and proof.get('transfer') == 'jmp')
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
    if byte_tables:
        result['byte_table_checks'] = byte_tables
    if alias_checks:
        result['source_alias_checks'] = alias_checks
    if data_checks:
        result['data_checks'] = data_checks
    if callback_calls:
        result['indirect_call_checks'] = list(callback_calls.values())
    return result
