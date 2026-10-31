"""Opt-in loader-zero-paired-v1 anchors (owner ruling D3, 2026-10-31).

An address-identity proof only. One pinned two-instruction native span
outside the compared caller:

    mov r32, imm32        ; B8+r, imm32 = image_base + owner_rva + k, k >= 1,
                          ; exactly one original HIGHLOW fixup, at that imm32
    mov r', [r32 - k]     ; immediately adjacent scalar load (1, 2 or 4 bytes),
                          ; base r32, no index, no segment, displacement -k

Because the first instruction is a plain register move that always falls
through, executing it from its start immediately executes the second with
r32 = owner + k, so that load addresses owner + 0 exactly. That fall-through
fact depends only on the pinned bytes: another control edge entering at the
second instruction's start (for example a loop back edge) adds paths but cannot
remove this one, and nothing else is inferred from such an edge. The owner extent
is a declared minimum that must cover k + width; no capacity, element count,
upper bound, runtime index range or neighbouring-symbol layout is inferred.

Storage checks mirror the pinned zero_storage verifier unchanged; keep this
module separate because accepted proofs pin zero_storage.py.
"""
import hashlib

ANCHOR_PROFILE = 'loader-zero-paired-v1'
IDENTITY_KEYS = {'rva', 'size', 'addend', 'sha256'}
GPR32 = ('eax', 'ecx', 'edx', 'ebx', 'ebp', 'esi', 'edi')


def _storage(pe, rva, extent):
    """Identical storage gate to zero_storage.verify_native."""
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
    for directory in pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY:
        if directory.Size and directory.VirtualAddress < end and rva < directory.VirtualAddress + directory.Size:
            raise ValueError('Loader-zero owner overlaps a PE data directory')
    if pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[9].Size or pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[10].Size:
        raise ValueError('Loader-zero support requires separately reviewed TLS/load-config effects')
    relocations = [entry for block in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', [])
                   for entry in block.entries if entry.type]
    if any(entry.rva < end and rva < entry.rva + 4 for entry in relocations):
        raise ValueError('Loader-zero storage contains an original loader fixup')
    return sec, relocations


def verify_native(pe, rva, extent, identity, caller_rva, caller_size):
    import capstone
    sec, relocations = _storage(pe, rva, extent)
    if not isinstance(identity, dict) or set(identity) != IDENTITY_KEYS:
        raise ValueError('Paired loader-zero identity requires one reviewed two-instruction span')
    at, size, k = (identity[key] for key in ('rva', 'size', 'addend'))
    if any(type(v) is not int for v in (at, size, k)) or at < 0 or not 2 <= size <= 30 or k < 1:
        raise ValueError('Invalid paired loader-zero identity span')
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
    pair = list(decoder.disasm(raw, at))
    if len(pair) != 2 or sum(i.size for i in pair) != size:
        raise ValueError('Paired identity must be exactly two adjacent complete instructions')
    first, second = pair
    # First: plain B8+r register load of an absolute immediate; always falls through.
    if (first.mnemonic != 'mov' or first.size != 5 or not 0xb8 <= first.bytes[0] <= 0xbf or
            any(first.prefix) or len(first.operands) != 2 or
            first.operands[0].type != capstone.x86.X86_OP_REG or
            first.operands[1].type != capstone.x86.X86_OP_IMM or first.imm_size != 4 or
            first.reg_name(first.operands[0].reg) not in GPR32):
        raise ValueError('Paired anchor must start with mov r32, imm32')
    reg = first.operands[0].reg
    if first.operands[1].imm & 0xffffffff != pe.image_base + rva + k:
        raise ValueError('Independent loader-zero anchor identifies another storage cell')
    # Second: scalar register load from [reg - k], nothing else.
    if (second.mnemonic != 'mov' or len(second.operands) != 2 or
            second.operands[0].type != capstone.x86.X86_OP_REG or
            second.operands[1].type != capstone.x86.X86_OP_MEM or second.addr_size != 4):
        raise ValueError('Paired anchor second instruction must be a scalar register load')
    op = second.operands[1]
    width = op.size
    # Raw-byte prefix rule: only 66 for a word load, then 8A/8B; no rep/lock/segment/67.
    lead = 1 if width == 2 else 0
    if (width not in (1, 2, 4) or bytes(second.bytes[:lead]) != b'f' * lead or
            second.bytes[lead] not in (0x8a, 0x8b)):
        raise ValueError('Paired anchor second instruction must be a scalar register load')
    if op.mem.base != reg or op.mem.index or op.mem.segment or op.mem.disp != -k:
        raise ValueError('Paired anchor load must dereference exactly the anchored register minus its addend')
    if k + width > extent:
        raise ValueError('Paired anchor exceeds the minimum owner extent')
    field = at + first.imm_offset
    if [(e.rva, e.type) for e in relocations if e.rva < at + size and at < e.rva + 4] != [(field, 3)]:
        raise ValueError('Loader-zero identity needs exactly one original HIGHLOW operand fixup')
    return dict(section=sec['name'], zero_start=sec['virtual_address'] + sec['raw_size'],
                zero_end=sec['virtual_address'] + sec['virtual_size'], identity_rva=at,
                anchor_profile=ANCHOR_PROFILE, addend=k, load_width=width)
