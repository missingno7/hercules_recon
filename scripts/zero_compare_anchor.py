"""Opt-in cmp-test-absolute-v1 anchors.

Storage and independence checks mirror the pinned zero_storage verifier; only
the instruction classifier differs: CMP/TEST absolute memory, never immediates.
Keep this module separate because accepted proofs pin zero_storage.py.
"""
import hashlib


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
    if ins.mnemonic not in ('cmp', 'test') or operand >= len(ins.operands):
        raise ValueError('Unsupported loader-zero identity instruction')
    op = ins.operands[operand]
    if (op.type == capstone.x86.X86_OP_MEM and ins.disp_size == 4 and
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
