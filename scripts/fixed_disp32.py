"""Explicit preferred-base i386 memory anchors; no relocation/provenance claim.

The comparator owns the complete contribution/reference checks. Explicit access
profiles dispatch only their access validation to separate modules.
"""
import hashlib
import json
import struct

from coff import COFF
from pe import PE
from masked import binding_records, REL32, DIR32

PROFILE = 'fixed-i386-disp32-v1'
# Further explicit anchor forms (scripts/fixed_anchor_forms.py); same record shape.
FORMS = ('fixed-i386-moffs32-v1', 'fixed-i386-regimm-v1')


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def image_facts(pe):
    p, oh = pe.pe, pe.pe.OPTIONAL_HEADER
    return dict(sha256=hashlib.sha256(pe.data).hexdigest(), machine=p.FILE_HEADER.Machine,
                magic=oh.Magic, characteristics=p.FILE_HEADER.Characteristics,
                image_base=pe.image_base, size_of_image=oh.SizeOfImage,
                sections=pe.sections, directories=[[d.VirtualAddress, d.Size] for d in oh.DATA_DIRECTORY])


def pending_intervals(rows, target):
    """All pending diagnostic rows, including padding; not just today's batch."""
    return sorted([dict(id=r['id'], rva=int(r['rva'], 0), size=r['size']) for r in rows
                   if r['target'] == target and r['status'] == 'CODEGEN_SIMILAR'], key=lambda r: r['id'])


def validate_exclusions(profile, required):
    excluded = profile.get('excluded')
    if (not isinstance(excluded, list) or len(excluded) > 10000 or
            any(not isinstance(x, dict) or set(x) != {'id', 'rva', 'size'} or
                not isinstance(x['id'], str) or type(x['rva']) is not int or x['rva'] < 0 or
                type(x['size']) is not int or x['size'] <= 0 for x in excluded) or
            len({x['id'] for x in excluded}) != len(excluded) or
            fingerprint(excluded) != profile.get('excluded_sha256')):
        raise ValueError('Invalid fixed disp32 pending exclusion snapshot')
    # Supersets survive later promotions; omissions or moved extents never do.
    if any(x not in excluded for x in required):
        raise ValueError('Fixed disp32 exclusion snapshot omits a pending/selected row')
    return excluded


def review_inputs(record, rows, row, observed, root):
    """Called before compilation; evidence and ledger are frozen by match.py."""
    profile = record.get('fixed_disp32')
    if not isinstance(profile, dict) or set(profile) != {
            'image', 'boundary', 'encoding', 'excluded', 'excluded_sha256', 'accesses'}:
        raise ValueError('Incomplete fixed disp32 profile')
    # The recorded snapshot documents independence at binding time; independence itself is
    # decided against the CURRENT ledger, so newly staged pending rows never invalidate a
    # proof unless they actually overlap the anchor's containing native function.
    validate_exclusions(profile, [])
    boundary = profile['boundary']
    if isinstance(boundary, dict) and type(boundary.get('rva')) is int and type(boundary.get('size')) is int:
        for pending in pending_intervals(rows, row['target']):
            if pending['id'] != row['id'] and (boundary['rva'] < pending['rva'] + pending['size'] and
                                               pending['rva'] < boundary['rva'] + boundary['size']):
                raise ValueError('Fixed disp32 containing native function overlaps a current pending row: '
                                 + pending['id'])
    if not isinstance(boundary, dict) or set(boundary) != {'rva', 'size', 'sha256', 'anchor_offset', 'review'}:
        raise ValueError('Fixed disp32 needs a reviewed native function boundary')
    review = boundary['review']
    if not isinstance(review, str) or not review.startswith('evidence/'):
        raise ValueError('Fixed disp32 boundary review must be durable evidence')
    path = (root / review).resolve()
    if path not in observed:
        raise ValueError('Fixed disp32 boundary review must be independently pinned')
    document = json.loads(path.read_bytes())
    native = {k: v for k, v in boundary.items() if k != 'review'}
    if (document.get('target_sha256') != row['target_sha256'] or
            native not in document.get('native_functions', [])):
        raise ValueError('Fixed disp32 boundary lacks a native review record')


def _code(pe, at, size):
    if (type(at) is not int or type(size) is not int or at < 0 or not 0 < size <= 0x100000 or
            len([s for s in pe.sections if s['characteristics'] & 0x20000000 and
                 s['virtual_address'] <= at and at + size <=
                 s['virtual_address'] + min(s['virtual_size'], s['raw_size'])]) != 1):
        raise ValueError('Fixed disp32 context must be complete file-backed executable code')
    return pe.read_rva(at, size)


def _instruction(pe, identity):
    import capstone
    if not isinstance(identity, dict) or set(identity) != {'rva', 'size', 'operand', 'sha256'}:
        raise ValueError('Fixed disp32 needs one pinned instruction')
    at, size, operand = (identity[k] for k in ('rva', 'size', 'operand'))
    if type(operand) is not int or operand < 0 or type(size) is not int or not 1 <= size <= 15:
        raise ValueError('Invalid fixed disp32 instruction')
    raw = _code(pe, at, size)
    if hashlib.sha256(raw).hexdigest() != identity['sha256']:
        raise ValueError('Fixed disp32 instruction hash changed')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(raw, at))
    if len(instructions) != 1 or instructions[0].size != size:
        raise ValueError('Fixed disp32 needs one complete instruction')
    return instructions[0]


def _scalar_prefixes(ins):
    # Capstone suppresses ignored REP prefixes on e.g. MOV. Inspect raw bytes.
    prefixes = []
    for byte in ins.bytes:
        if byte not in (0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0x66, 0x67, 0xf0, 0xf2, 0xf3):
            break
        prefixes.append(byte)
    return prefixes in ([], [0x66])


def encoding(ins, operand):
    return dict(disp_offset=ins.disp_offset, disp_size=ins.disp_size,
                address_size=ins.addr_size, modrm=ins.modrm,
                mnemonic=ins.mnemonic, width=ins.operands[operand].size)


def verify_native(pe, rva, extent, identity, caller_rva, caller_size, profile):
    import capstone
    facts = image_facts(pe)
    if facts != profile.get('image'):
        raise ValueError('Fixed disp32 image fingerprint changed')
    p, oh = pe.pe, pe.pe.OPTIONAL_HEADER
    if (p.FILE_HEADER.Machine != 0x14c or oh.Magic != 0x10b or
            not p.FILE_HEADER.Characteristics & 1 or p.FILE_HEADER.Characteristics & 0x2000 or
            facts['directories'][5] != [0, 0] or getattr(p, 'DIRECTORY_ENTRY_BASERELOC', [])):
        raise ValueError('Fixed disp32 requires a stripped fixed-base i386 EXE')
    if (type(rva) is not int or type(extent) is not int or rva < 0 or extent <= 0 or
            rva + extent > min(0x100000000 - pe.image_base, oh.SizeOfImage)):
        raise ValueError('Invalid fixed disp32 owner extent')
    end = rva + extent
    sections = [s for s in pe.sections if s['virtual_address'] < end and
                rva < s['virtual_address'] + max(s['virtual_size'], s['raw_size'])]
    if len(sections) != 1:
        raise ValueError('Fixed disp32 owner needs one unambiguous section')
    sec = sections[0]
    if (rva < sec['virtual_address'] + sec['raw_size'] or end > sec['virtual_address'] + sec['virtual_size'] or
            sec['characteristics'] & (0x20000020 | 0x02000000) or
            sec['characteristics'] & 0xc0000000 != 0xc0000000):
        raise ValueError('Fixed disp32 owner must be wholly in a writable zero tail')
    if any(d.Size and d.VirtualAddress < end and rva < d.VirtualAddress + d.Size for d in oh.DATA_DIRECTORY):
        raise ValueError('Fixed disp32 owner overlaps a PE directory')
    if any(oh.DATA_DIRECTORY[n].Size or oh.DATA_DIRECTORY[n].VirtualAddress for n in (9, 10)):
        raise ValueError('Fixed disp32 rejects unreviewed TLS/load-config effects')
    ins = _instruction(pe, identity)
    excluded = validate_exclusions(profile, [])
    if any(ins.address < lo + length and lo < ins.address + ins.size for lo, length in
           [(caller_rva, caller_size)] + [(x['rva'], x['size']) for x in excluded]):
        raise ValueError('Fixed disp32 anchor overlaps a pending/selected caller')
    boundary = profile.get('boundary', {})
    raw = _code(pe, boundary.get('rva'), boundary.get('size'))
    if any(boundary['rva'] < lo + length and lo < boundary['rva'] + boundary['size']
           for lo, length in [(caller_rva, caller_size)] + [(x['rva'], x['size']) for x in excluded]):
        raise ValueError('Fixed disp32 containing native function overlaps a pending/selected caller')
    if (hashlib.sha256(raw).hexdigest() != boundary.get('sha256') or
            boundary.get('anchor_offset') != ins.address - boundary['rva']):
        raise ValueError('Fixed disp32 native context hash/offset changed')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    decoded = list(decoder.disasm(raw, boundary['rva']))
    if (sum(i.size for i in decoded) != len(raw) or
            not any(i.address == ins.address and bytes(i.bytes) == bytes(ins.bytes) for i in decoded)):
        raise ValueError('Fixed disp32 anchor is not at a reviewed context instruction boundary')
    n = identity['operand']
    if ins.mnemonic not in ('mov', 'movsx', 'movzx', 'push') or n >= len(ins.operands):
        raise ValueError('Fixed disp32 requires a dereferencing MOV/MOVSX/MOVZX/PUSH')
    op = ins.operands[n]
    if (op.type != capstone.x86.X86_OP_MEM or op.mem.base or op.mem.index or op.mem.segment or
            ins.addr_size != 4 or not _scalar_prefixes(ins) or
            not ins.modrm_offset or ins.modrm & 0xc7 != 5 or ins.disp_size != 4 or
            op.size not in (1, 2, 4) or op.size > extent):
        raise ValueError('Fixed disp32 requires unsegmented non-SIB mod00 rm101 scalar memory')
    if op.mem.disp != pe.image_base + rva:
        raise ValueError('Fixed disp32 anchor identifies another base/interior cell')
    if encoding(ins, n) != profile.get('encoding'):
        raise ValueError('Fixed disp32 reviewed encoding receipt changed')
    return dict(profile=PROFILE, identity_rva=ins.address, disp_offset=ins.disp_offset,
                width=op.size, modrm=ins.modrm, address_size=ins.addr_size,
                section=sec['name'], preferred_base_only=True, global_isolation_proved=False)


def verify_access(ins, off, addend, extent, caller_rva):
    """Storage address must be consumed directly, never materialized/escaped.

    Interior scalar fields are allowed only within the exact aggregate owner.
    This does not infer footprints of pointers loaded from that owner.
    """
    import capstone
    mem = [o for o in ins.operands if o.type == capstone.x86.X86_OP_MEM] if ins else []
    if (ins is None or ins.mnemonic == 'lea' or ins.group(capstone.CS_GRP_CALL) or
            ins.group(capstone.CS_GRP_JUMP) or ins.addr_size != 4 or
            not _scalar_prefixes(ins) or
            ins.disp_size != 4 or ins.address - caller_rva + ins.disp_offset != off or len(mem) != 1 or
            mem[0].mem.base or mem[0].mem.index or mem[0].mem.segment or
            mem[0].size not in (1, 2, 4) or addend < 0 or addend + mem[0].size > extent):
        raise ValueError('Fixed disp32 access escapes, is indexed, or exceeds its exact owner')
    return dict(offset=off, addend=addend, width=mem[0].size)


verify_native_fixed = verify_native


def access_form(record):
    """Closed opt-in dispatch; absent access_profile preserves the legacy path."""
    if 'access_profile' not in record:
        return None
    if record['access_profile'] == 'loader-zero-indexed-v1':
        from loader_zero_indexed import validate_record, verify_record_access
    else:
        from fixed_indexed import validate_record, verify_record_access
    validate_record(record)
    return verify_record_access


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
    if 'coff_identity' in bindings:
        from coff_identity import verify_pair
        canonical = verify_pair(coff, symbol, bindings['coff_identity'])
        by_name[symbol] = [canonical]
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
    access_forms = {}
    alias_checks, data_checks = {}, []
    payload_identities = {name: (rec.get('kind'), rec.get('rva'), rec.get('size'), rec.get('sha256'),
                              rec.get('dll'), rec.get('name'), rec.get('ordinal'))
                          for name, rec in records.items() if isinstance(rec, dict)}
    for name, record in records.items():
        if not isinstance(record, dict):
            raise ValueError('Invalid symbol record')
        access_forms[name] = access_form(record)
        evidence(record)
        dest, extent = integer(record['rva'], 'symbol RVA'), integer(record['size'], 'symbol extent', 1)
        kind, sym = record['kind'], by_name[name][0]
        if 'data_profile' in record and (kind != 'data' or record['data_profile'] != 'relocated-data-v1'):
            raise ValueError('Unsupported explicit data profile')
        if 'anchor_profile' in record and (kind != 'bss' or record['anchor_profile'] not in ('cmp-test-absolute-v1', 'loader-zero-paired-v1', PROFILE) + FORMS):
            raise ValueError('Unsupported explicit loader-zero anchor profile')
        if 'alias_profile' in record and kind != 'bss':
            raise ValueError('Unsupported explicit alias profile')
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
            if 'alias_profile' in record:
                from storage_alias_byte import verify_record
                alias_checks[name] = verify_record(obj, (storage_aliases or {}).get(name), symbol, name, record)
            elif owner_name != name:
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
            if record.get('anchor_profile') == PROFILE:
                verify_native_fixed(pe, dest, extent, record.get('identity'), rva, size, record['fixed_disp32'])
            elif record.get('anchor_profile') in FORMS:
                from fixed_anchor_forms import verify_native as verify_native_form
                verify_native_form(pe, dest, extent, record.get('identity'), rva, size, record['fixed_disp32'],
                                   record['anchor_profile'])
            elif record.get('anchor_profile') == 'loader-zero-paired-v1':
                from loader_zero_paired import verify_native as verify_paired_native
                verify_paired_native(pe, dest, extent, record.get('identity'), rva, size)
            elif record.get('anchor_profile') == 'cmp-test-absolute-v1':
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

    access_surveys = {name: [] for name, rec in records.items()
                      if rec.get('anchor_profile') in (PROFILE,) + FORMS or access_forms[name] is not None}
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
            form = access_forms[name]
            if name in access_surveys:
                access_surveys[name].append(
                    verify_access(ins, off, addend, record['size'], rva) if form is None else
                    form(ins, off, addend, record['size'], rva, record, expected, pe.image_base + record['rva']))
            if typ != DIR32 or ins is None:
                raise ValueError('Loader-zero references require direct absolute instruction operands')
            # An explicit form validates its own memory footprint; direct uses
            # inside that form still go through the unchanged verify_access.
            if form is None and ins.disp_size == 4 and ins.address - rva + ins.disp_offset == off:
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
    for name, accesses in access_surveys.items():
        survey = ('loader_zero_indexed' if records[name].get('access_profile') == 'loader-zero-indexed-v1'
                  else 'fixed_disp32')
        if accesses != records[name][survey].get('accesses'):
            raise ValueError('Fixed disp32 reviewed access survey differs from complete COFF uses')
    if set(reviewed) != actual_offsets or not table_starts <= actual_offsets:
        raise ValueError('Reviewed relocation/table entry inventory differs from COFF')
    from callback_transfer import verify_calls
    # callback_transfer.py is pinned by accepted proofs; dispatch the separate
    # fixed-slot profile here and pass only the other records to it.
    calls = bindings.get('indirect_calls', [])
    if not isinstance(calls, list):
        raise ValueError('Invalid explicit callback inventory')
    fixed = [c for c in calls if isinstance(c, dict) and c.get('profile') == 'fixed-slot-v1']
    opaque = [c for c in calls if isinstance(c, dict) and c.get('profile') == 'opaque-indirect-call-v1']
    rooted = [c for c in calls if c not in fixed and c not in opaque]
    callback_calls = verify_calls(pe, dict(bindings, indirect_calls=rooted), records, reviewed,
                                  instructions, rva, size, receipts)
    if fixed:
        from fixed_slot import verify_call
        for record in fixed:
            if record['offset'] in callback_calls:
                raise ValueError('Invalid/duplicate fixed-slot offset')
            callback_calls[record['offset']] = verify_call(pe, record, records, reviewed, instructions, rva, size)
    if opaque:
        # Explicitly unidentified call targets (opaque_call.py): instruction identity only.
        from opaque_call import verify_calls as verify_opaque_calls
        for offset, proof in verify_opaque_calls(pe, opaque, records, reviewed, instructions, rva, expected).items():
            if offset in callback_calls:
                raise ValueError('Invalid/duplicate opaque call offset')
            callback_calls[offset] = proof
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
        elif callback_calls.get(offset, {}).get('profile') == 'opaque-indirect-call-v1':
            continue
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
    if data_checks:
        result['data_checks'] = data_checks
    if callback_calls:
        result['indirect_call_checks'] = list(callback_calls.values())
    opaque_count = sum(1 for proof in callback_calls.values() if proof.get('profile') == 'opaque-indirect-call-v1')
    if opaque_count:
        result['scope'] += '; %d opaque indirect CALL(s): targets unidentified, ABI unclaimed' % opaque_count
    return result
