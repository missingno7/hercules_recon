"""Opt-in file-data-owner-v1: extern initialized (file-backed) data defined by a typed C owner.

Dispatched only for records that carry data_profile 'file-data-owner-v1'; masked.py,
local_byte_tables.py and the pre-integration fixed_disp32.py comparator raise
'Unsupported explicit data profile' for them.

A record is an address-identity and content proof for one undefined caller symbol. Its
storage claim is exactly: the owner object's complete storage, compared byte for byte
with the original file bytes at [rva, rva+size). No size or end of the original object
is proved.

1. owner   - match.py freshly compiles the separately pinned C data-only owner
             (source/flags/symbol). It must emit exactly one strong defined initialized
             object of that COFF name, outside COMDAT/BSS/code, as the last object of its
             section. VC5 emits no trailing section padding, so that allocation is the
             compiled object's storage, never inter-object alignment padding; its size
             must equal the reviewed size. No functions, executable bytes, debug
             sections, weak symbols or directives other than defaultlib.
2. content - relocation-free storage must have no original loader fixup in
             [rva-3, rva+size) and must equal the original bytes. Relocation-bearing
             storage goes through relocated_data.verify_payload on the OWNER object with
             the comparator's shared payload identities, seeded with the compared
             function, so each pointer child agrees with the caller's own records and
             with the compared function; each pointer is bound to an independently
             evidenced identity and the original HIGHLOW inventory must equal the owner
             DIR32 inventory. Payload data children must be defined by the owner itself.
3. start   - one complete original instruction outside the compared caller and selected
             batch holds image_base + rva in an absolute operand with exactly one original
             HIGHLOW fixup on that field. Host acceptance status is never consulted, but
             an anchor host that carries a reference binding in the same verification
             run must not name that address or field differently (review_host_names).
4. uses    - direct caller memory accesses must lie inside [rva, rva+size); indexed uses
             bound only their displacement; one-past uses are refused. The schema value
             file_data.extent_claim 'minimum' records only that no end, capacity or
             element count is claimed.

Not proved: the original object's size, end, capacity or C type; runtime mutation or
writes; absence of other aliases into the storage; source identity; linker layout or
whole-image equality. The native payload sha256 is pinned for consistency only; it is
never the identity source.
"""
import hashlib
import re

from coff import COFF

PROFILE = 'file-data-owner-v1'
# Canonical owners live under src/. A diagnostic runner may extend this in-process only.
SOURCE_ROOTS = ('src',)
RECORD_KEYS = {'kind', 'data_profile', 'rva', 'size', 'sha256', 'owner', 'file_data',
               'evidence', 'references'}
OPTIONAL_KEYS = {'payload_bindings'}
ANCHOR_KEYS = {'rva', 'size', 'operand', 'sha256'}
ANCHOR_MNEMONICS = ('mov', 'lea', 'push', 'movsx', 'movzx', 'cmp', 'test')


def _int(value, label, minimum=0):
    if type(value) is not int or value < minimum:
        raise ValueError('Invalid file data ' + label)
    return value


def validate_record(record, name):
    """Closed schema; never combined with loader-zero, alias or access profiles."""
    if not isinstance(record, dict) or record.get('kind') != 'data' or record.get('data_profile') != PROFILE:
        raise ValueError('File data owner profile requires kind data')
    if not RECORD_KEYS <= set(record) or set(record) - RECORD_KEYS - OPTIONAL_KEYS:
        raise ValueError('File data owner record has missing or unsupported fields')
    _int(record['rva'], 'RVA')
    _int(record['size'], 'extent', 1)
    if not isinstance(record['sha256'], str) or not re.fullmatch(r'[0-9a-f]{64}', record['sha256']):
        raise ValueError('File data owner record needs the pinned native payload sha256')
    owner = record['owner']
    if not isinstance(owner, dict) or set(owner) != {'source', 'flags', 'symbol'}:
        raise ValueError('File data owner requires an explicit source/flags/symbol owner')
    if owner['symbol'] != name:
        raise ValueError('File data owner aliases are unsupported')
    if (not isinstance(owner['source'], str) or not isinstance(owner['flags'], list) or
            any(not isinstance(flag, str) for flag in owner['flags'])):
        raise ValueError('Invalid file data owner compile profile')
    survey = record['file_data']
    claim = survey.get('extent_claim') if isinstance(survey, dict) else None
    if claim != 'minimum':
        raise ValueError('File data owner v1 supports a minimum extent claim only')
    if set(survey) != {'extent_claim', 'anchor'}:
        raise ValueError('File data owner survey has missing or unsupported fields')
    anchor = survey['anchor']
    if not isinstance(anchor, dict) or set(anchor) != ANCHOR_KEYS:
        raise ValueError('File data owner requires one reviewed native start anchor')
    for use in record['references'] if isinstance(record['references'], list) else []:
        if isinstance(use, dict) and use.get('one_past'):
            raise ValueError('A one-past use asserts an end boundary; unsupported in file data owner v1')
    return survey


def anchor_spans(record):
    survey = record['file_data']
    spans = [(survey['anchor']['rva'], survey['anchor']['size'])]
    for at, length in spans:
        _int(at, 'anchor RVA')
        _int(length, 'anchor size', 1)
    return spans


def owner_profile(record, name, root):
    """match.py hook: the pinned owner compile profile (a C data-only unit under src/)."""
    validate_record(record, name)
    owner = record['owner']
    source = (root / owner['source']).resolve()
    if not any(source.is_relative_to((root / base).resolve()) for base in SOURCE_ROOTS):
        raise ValueError('File data owner must be canonical repository source')
    if source.suffix != '.c' or any(flag.lower().startswith(('/tp', '-tp')) for flag in owner['flags']):
        raise ValueError('File data owner requires a C data-only profile')
    return source, tuple(owner['flags'])


def review_inputs(record, name, row, ledger, selected):
    """Reject overlap with this caller or selected batch, regardless of host status.

    Stable witnesses / three consistent host rows (strict for RECONSTRUCTED)
    determine scheduling in planning, not verification or acceptance.
    """
    validate_record(record, name)
    for at, length in anchor_spans(record):
        for other in ledger:
            if other.get('target') != row['target'] or not other.get('rva'):
                continue
            if other['id'] != row['id'] and other['id'] not in selected:
                continue
            start = int(other['rva'], 0)
            if at < start + other['size'] and start < at + length:
                raise ValueError('File data anchor overlaps the compared or selected caller: ' + other['id'])


def review_host_names(checks, row, ledger, bindings):
    """match.py hardening after the comparator: no anchor host may name the cell differently.

    ``checks`` are the comparator's data_checks for ``row``; ``bindings`` maps row id to the
    (binding, evidence, path) triple of every row that carries a reference binding in this
    verification run (accepted relocated rows and the selected batch). For each other row of
    the same target whose code contains the anchor's address field and which carries such a
    binding: its use of that field, if any, must be this symbol at addend 0 and not one-past;
    no record may name this start address (or storage covering it) with another symbol; and
    this symbol may not be bound at another address. Hosts without a binding in the run impose
    nothing, so the check never depends on acceptance status itself.
    """
    for check in checks:
        if not isinstance(check, dict) or check.get('profile') != PROFILE:
            continue
        name, start, field = check['symbol'], check['rva'], check['anchor']['field']
        for host in ledger:
            if (host['id'] == row['id'] or host.get('target') != row['target'] or not host.get('rva') or
                    host['id'] not in bindings):
                continue
            base = int(host['rva'], 0)
            if not base <= field < base + host['size']:
                continue
            records = bindings[host['id']][0].get('symbols', {})
            for other, record in records.items() if isinstance(records, dict) else ():
                if not isinstance(record, dict):
                    continue
                at, length = record.get('rva'), record.get('size')
                covers = type(at) is int and type(length) is int and at <= start < at + length
                if other != name and (at == start or covers):
                    raise ValueError('File data anchor host names the cell differently: ' + host['id'] + ' ' + other)
                if other == name and at != start:
                    raise ValueError('File data anchor host binds this symbol elsewhere: ' + host['id'])
                uses = record.get('references')
                for use in uses if isinstance(uses, list) else []:
                    if isinstance(use, dict) and use.get('offset') == field - base and (
                            other != name or use.get('addend') != 0 or use.get('one_past')):
                        raise ValueError('File data anchor host binds the anchor field differently: ' +
                                         host['id'] + ' ' + other)


def verify_owner_object(obj, name, extent):
    """The owner object is the last object of its section: allocation == compiled object storage."""
    from relocated_data import complete_allocation
    coff = COFF(obj)
    choices = [s for s in coff.symbols.values() if s['name'] == name]
    if len(choices) != 1:
        raise ValueError('File data owner symbol is missing or ambiguous')
    sym = choices[0]
    if sym['storage'] != 2 or sym['type'] or sym.get('aux') or not 1 <= sym['section'] <= len(coff.sections):
        raise ValueError('File data owner requires one strong defined data object')
    sec = coff.sections[sym['section'] - 1]
    if (not sec['flags'] & 0x40 or sec['flags'] & (0x20 | 0x80 | 0x1000 | 0x02000000 | 0x20000000) or
            len(sec['data']) != sec['size']):
        raise ValueError('File data owner object must lie in complete initialized non-COMDAT data')
    for other in coff.sections:
        if other['flags'] & 0x20000020 and other['size']:
            raise ValueError('File data owner cannot contain executable code')
        if other['name'].startswith('.debug'):
            raise ValueError('File data owner cannot carry debug sections')
        if other['name'] == '.drectve' and any(
                not re.fullmatch(rb'[-/]defaultlib:[A-Za-z0-9_.-]+', token, re.I) for token in other['data'].split()):
            raise ValueError('File data owner has unsupported linker directives')
    for other in coff.symbols.values():
        if other['storage'] == 105 or (other['type'] & 0x20 and other['section'] > 0):
            raise ValueError('File data owner cannot define functions or weak aliases')
    complete_allocation(coff, sym, extent)
    if sym['value'] + extent != sec['size']:
        raise ValueError('File data owner object must end its section; inter-object padding is not object storage')
    return coff, sym


def verify_native_storage(pe, rva, extent):
    """The claimed extent is ordinary file-backed initialized data in a relocatable image."""
    oh = pe.pe.OPTIONAL_HEADER
    if not oh.DATA_DIRECTORY[5].Size or not getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []):
        raise ValueError('File data owner v1 requires original base relocations (DLL); stripped images unsupported')
    end = rva + extent
    choices = [s for s in pe.sections if s['virtual_address'] <= rva and
               end <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])]
    if len(choices) != 1:
        raise ValueError('File data extent must lie in one file-backed section')
    flags = choices[0]['characteristics']
    if flags & (0x20000020 | 0x02000000) or not flags & 0x40:
        raise ValueError('File data extent must be non-executable initialized data')
    for directory in oh.DATA_DIRECTORY:
        if directory.Size and directory.VirtualAddress < end and rva < directory.VirtualAddress + directory.Size:
            raise ValueError('File data extent overlaps a PE data directory')
    if oh.DATA_DIRECTORY[9].Size or oh.DATA_DIRECTORY[10].Size:
        raise ValueError('File data owner requires separately reviewed TLS/load-config effects')
    return choices[0]


def _fixups(pe):
    return [(e.rva, e.type) for block in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
            for e in block.entries if e.type]


def _decoder():
    import capstone
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    return decoder


def _code(pe, at, size, caller_rva, caller_size, label):
    if at < caller_rva + caller_size and caller_rva < at + size:
        raise ValueError('File data ' + label + ' must be independent of the compared caller')
    if len([s for s in pe.sections if s['characteristics'] & 0x20000000 and s['virtual_address'] <= at and
            at + size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])]) != 1:
        raise ValueError('File data ' + label + ' must be file-backed executable code')
    return pe.read_rva(at, size)


def _prefixes(ins):
    prefixes = []
    for byte in ins.bytes:
        if byte not in (0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0x66, 0x67, 0xf0, 0xf2, 0xf3):
            break
        prefixes.append(byte)
    return prefixes


def _one_fixup(pe, field):
    if [f for f in _fixups(pe) if f[0] < field + 4 and field < f[0] + 4] != [(field, 3)]:
        raise ValueError('File data anchor needs exactly one original HIGHLOW operand fixup')


def verify_anchor(pe, rva, extent, anchor, caller_rva, caller_size):
    """Mirrors zero_storage.verify_native for a file-backed start; no caller-implied address."""
    import capstone
    at, size, operand = (anchor[k] for k in ('rva', 'size', 'operand'))
    if any(type(v) is not int for v in (at, size, operand)) or at < 0 or not 1 <= size <= 15 or operand < 0:
        raise ValueError('Invalid file data anchor instruction')
    raw = _code(pe, at, size, caller_rva, caller_size, 'anchor')
    if hashlib.sha256(raw).hexdigest() != anchor['sha256']:
        raise ValueError('File data anchor instruction hash changed')
    instructions = list(_decoder().disasm(raw, at))
    if len(instructions) != 1 or instructions[0].size != size:
        raise ValueError('File data anchor must cover one complete instruction')
    ins = instructions[0]
    if ins.mnemonic not in ANCHOR_MNEMONICS or operand >= len(ins.operands) or _prefixes(ins) not in ([], [0x66]):
        raise ValueError('Unsupported file data anchor instruction')
    op = ins.operands[operand]
    if op.type == capstone.x86.X86_OP_IMM and ins.imm_size == 4 and ins.mnemonic in ('mov', 'push'):
        address, field, form, width = op.imm & 0xffffffff, at + ins.imm_offset, 'immediate', None
    elif (op.type == capstone.x86.X86_OP_MEM and ins.disp_size == 4 and ins.addr_size == 4 and
          op.mem.base == 0 and op.mem.index == 0 and op.mem.segment == 0):
        address, field, width = op.mem.disp & 0xffffffff, at + ins.disp_offset, op.size
        form = 'lea-direct' if ins.mnemonic == 'lea' else 'direct'
        if form == 'direct' and not 1 <= width <= extent:
            raise ValueError('File data anchor access exceeds the claimed extent')
    else:
        raise ValueError('File data anchor needs an absolute imm32 or direct disp32 operand')
    if address != pe.image_base + rva:
        raise ValueError('Independent file data anchor identifies another address')
    _one_fixup(pe, field)
    return dict(identity_rva=at, field=field, form=form, mnemonic=ins.mnemonic, width=width)


def _payload_children(coff, record):
    graph = record.get('payload_bindings')
    if not isinstance(graph, dict) or set(graph) != {'symbols'} or not isinstance(graph['symbols'], dict):
        raise ValueError('Relocation-bearing file data needs an explicit payload binding inventory')
    for child, binding in graph['symbols'].items():
        if not isinstance(binding, dict) or binding.get('kind') not in ('function', 'data', 'import'):
            raise ValueError('Unsupported file data payload child')
        defined = [s for s in coff.symbols.values() if s['name'] == child and s['section'] > 0]
        if binding['kind'] == 'data' and not defined:
            raise ValueError('File data payload data children must be defined by the owner unit')


def verify_uses(record, extent, operand_fields, caller_rva):
    """Direct (base/index-free) caller memory accesses must lie wholly inside the owner storage.

    Indexed uses bound only their displacement (addend < extent, checked by the comparator);
    runtime index ranges are not inferred or claimed.
    """
    import capstone
    widths = []
    for use in record['references'] if isinstance(record['references'], list) else []:
        if not isinstance(use, dict) or type(use.get('offset')) is not int or type(use.get('addend')) is not int:
            continue  # the comparator rejects malformed uses itself
        ins = operand_fields.get(use['offset'])
        if ins is None or not (ins.disp_size == 4 and ins.address - caller_rva + ins.disp_offset == use['offset']):
            continue
        mem = [op for op in ins.operands if op.type == capstone.x86.X86_OP_MEM]
        if ins.mnemonic == 'lea' or len(mem) != 1 or mem[0].mem.base or mem[0].mem.index:
            continue
        if use['addend'] < 0 or use['addend'] + mem[0].size > extent:
            raise ValueError('File data direct access exceeds the claimed extent')
        widths.append(dict(offset=use['offset'], addend=use['addend'], width=mem[0].size))
    return widths


def verify_record(pe, sym, name, record, owner_obj, caller_rva, caller_size, interval, errors, operand_fields,
                  identities):
    """Comparator hook for one undefined caller symbol; returns a receipt for data_checks.

    ``identities`` is the comparator's shared payload_identities (the caller's own records,
    seeded with the compared function), exactly as the legacy relocated-data path shares it.
    """
    survey = validate_record(record, name)
    if sym['section'] != 0 or sym['value'] != 0 or sym['type'] & 0x20 or sym.get('storage', 2) != 2:
        raise ValueError('File data caller symbol must be undefined, non-COMMON external data')
    dest, extent = record['rva'], record['size']
    direct = verify_uses(record, extent, operand_fields, caller_rva)
    verify_native_storage(pe, dest, extent)
    payload = interval(dest, extent, False)
    if record['sha256'] != hashlib.sha256(payload).hexdigest():
        errors.append('Data payload identity differs: ' + name)
    if owner_obj is None:
        raise ValueError('File data owner object was not freshly compiled')
    coff, osym = verify_owner_object(owner_obj, name, extent)
    sec, pos = coff.sections[osym['section'] - 1], osym['value']
    rels = [r for r in sec['relocations'] if r['offset'] < pos + extent and pos < r['offset'] + 4]
    payload_check = None
    if rels:
        _payload_children(coff, record)
        from relocated_data import verify_payload
        # Relocated-data semantics on the owner object: its own section bases, the caller's identities.
        payload_check = verify_payload(coff, pe, name, dict(record, data_profile='relocated-data-v1'),
                                       interval, {}, errors, identities)
    else:
        if 'payload_bindings' in record:
            raise ValueError('Payload bindings require a relocation-bearing owner payload')
        if any(dest - 3 <= at < dest + extent for at, _ in _fixups(pe)):
            raise ValueError('File data payload contains an unbound original loader fixup')
        if sec['data'][pos:pos + extent] != payload:
            errors.append('Compiled owner payload differs: ' + name)
    anchor = verify_anchor(pe, dest, extent, survey['anchor'], caller_rva, caller_size)
    receipt = dict(profile=PROFILE, symbol=name, rva=dest, bytes=extent, extent_claim=survey['extent_claim'],
                   storage_claim='owner object complete storage, compared byte for byte; no original size/end',
                   anchor=anchor, owner_relocations=len(rels), direct_uses=direct,
                   owner_payload_sha256=hashlib.sha256(sec['data'][pos:pos + extent]).hexdigest())
    if payload_check:
        receipt['payload_check'] = payload_check
    return receipt
