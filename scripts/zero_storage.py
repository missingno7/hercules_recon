"""Explicit COMMON-owner and native loader-zero checks for relocated proofs.

No zero-byte similarity or caller-implied address can identify an owner. The
reviewed binding must supply an independent native anchor, and match.py must
freshly compile its separately pinned source/include context on every run.
"""
import hashlib
from coff import COFF


def owner_profile(record, name, root):
    owner = record.get('owner')
    if not isinstance(owner, dict) or set(owner) != {'source', 'flags', 'symbol'}:
        raise ValueError('Loader-zero binding requires an explicit source/flags/symbol owner')
    if owner['symbol'] != name:
        from storage_alias import alias_profile
        alias_profile(record, name, root)
    elif 'source_alias' in record:
        raise ValueError('Same-name loader-zero owner cannot carry an alias adapter')
    if not isinstance(owner['source'], str) or not isinstance(owner['flags'], list) or any(
            not isinstance(flag, str) for flag in owner['flags']):
        raise ValueError('Invalid loader-zero owner compile profile')
    source = (root / owner['source']).resolve()
    if not any(source.is_relative_to((root / directory).resolve()) for directory in ('src', 'calibration')):
        raise ValueError('Loader-zero owner must be canonical repository source')
    if record.get('source_alias', {}).get('kind') == 'owner-compile-renaming' and (
            source.suffix != '.c' or any(flag.lower().startswith(('/tp', '-tp')) for flag in owner['flags'])):
        raise ValueError('Owner renaming requires a C data-only owner profile')
    return source, tuple(owner['flags'])


def verify_common(obj, name, extent):
    coff = COFF(obj)
    choices = [s for s in coff.symbols.values() if s['name'] == name]
    if len(choices) != 1:
        raise ValueError('Loader-zero owner symbol is missing or ambiguous')
    sym = choices[0]
    if (sym['storage'] != 2 or sym['section'] != 0 or sym['type'] & 0x20 or
            sym['value'] <= 0 or sym['value'] != extent or sym.get('aux')):
        raise ValueError('Loader-zero owner must be one complete exact-size COMMON definition')
    return sym


def verify_native(pe, rva, extent, identity, caller_rva, caller_size):
    import capstone
    if (type(rva) is not int or type(extent) is not int or rva < 0 or extent <= 0 or
            rva + extent > min(0x100000000, pe.pe.OPTIONAL_HEADER.SizeOfImage)):
        raise ValueError('Invalid loader-zero storage extent')
    end = rva + extent
    sections = [s for s in pe.sections if s['virtual_address'] < end and
                rva < s['virtual_address'] + max(s['virtual_size'], s['raw_size'])]
    if len(sections) != 1:
        raise ValueError('Loader-zero storage must have one unambiguous section')
    sec = sections[0]
    if (rva < sec['virtual_address'] + sec['raw_size'] or
            end > sec['virtual_address'] + sec['virtual_size'] or
            sec['characteristics'] & (0x20000020 | 0x02000000) or
            sec['characteristics'] & 0xc0000000 != 0xc0000000):
        raise ValueError('Loader-zero owner must lie wholly in a writable non-executable zero tail')
    # Reject loader-managed storage, not just an initialized payload on disk.
    for directory in pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY:
        if directory.Size and directory.VirtualAddress < end and rva < directory.VirtualAddress + directory.Size:
            raise ValueError('Loader-zero owner overlaps a PE data directory')
    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[9].Size or pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[10].Size:
        raise ValueError('Loader-zero support requires separately reviewed TLS/load-config effects')
    relocations = [entry for block in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                   for entry in block.entries if entry.type]
    if any(entry.rva < end and rva < entry.rva + 4 for entry in relocations):
        raise ValueError('Loader-zero storage contains an original loader fixup')
    if not isinstance(identity, dict) or set(identity) != {'rva', 'size', 'operand', 'sha256'}:
        raise ValueError('Loader-zero identity requires one reviewed native instruction anchor')
    at, size, operand = (identity[k] for k in ('rva', 'size', 'operand'))
    if any(type(v) is not int for v in (at, size, operand)) or at < 0 or not 1 <= size <= 15 or operand < 0:
        raise ValueError('Invalid loader-zero identity instruction')
    if at < caller_rva + caller_size and caller_rva < at + size:
        raise ValueError('Loader-zero identity must be independent of the compared caller')
    code_sections = [s for s in pe.sections if s['characteristics'] & 0x20000000 and
                     s['virtual_address'] <= at and at + size <= s['virtual_address'] + min(s['virtual_size'], s['raw_size'])]
    if len(code_sections) != 1:
        raise ValueError('Loader-zero identity must be file-backed executable code')
    raw = pe.read_rva(at, size)
    if hashlib.sha256(raw).hexdigest() != identity['sha256']:
        raise ValueError('Loader-zero identity instruction hash changed')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(raw, at))
    if len(instructions) != 1 or instructions[0].size != size:
        raise ValueError('Loader-zero identity must cover one complete instruction')
    ins = instructions[0]
    if ins.mnemonic not in ('mov', 'lea', 'push') or operand >= len(ins.operands):
        raise ValueError('Unsupported loader-zero identity instruction')
    op = ins.operands[operand]
    if op.type == capstone.x86.X86_OP_IMM and ins.imm_size == 4:
        address, field = op.imm, at + ins.imm_offset
    elif (op.type == capstone.x86.X86_OP_MEM and ins.disp_size == 4 and
          op.mem.base == 0 and op.mem.index == 0 and op.mem.segment == 0 and op.size <= extent):
        address, field = op.mem.disp, at + ins.disp_offset
    else:
        raise ValueError('Loader-zero identity needs an absolute address operand')
    if address != pe.image_base + rva:
        raise ValueError('Independent loader-zero anchor identifies another storage cell')
    if [(e.rva, e.type) for e in relocations if e.rva < field + 4 and field < e.rva + 4] != [(field, 3)]:
        raise ValueError('Loader-zero identity needs exactly one original HIGHLOW operand fixup')
    return dict(section=sec['name'], zero_start=sec['virtual_address'] + sec['raw_size'],
                zero_end=sec['virtual_address'] + sec['virtual_size'], identity_rva=at)
