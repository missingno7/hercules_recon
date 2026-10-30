"""Two further explicit preferred-base anchor forms for stripped fixed-base i386 EXEs.

Opt-in only. Nothing imports this module unless a loader-zero record names one of
PROFILES as its anchor_profile. It reuses the fixed-i386-disp32-v1 record shape and
review rules (reviewed native containing function outside every current pending
row, exact image facts, pinned pending snapshot, complete caller access survey) and
changes only which anchor instruction encodings may establish the owner base.

fixed-i386-moffs32-v1
    MOV AL/AX/EAX <-> [moffs32] (opcodes A0..A3, optional single 66 prefix), the
    accumulator-only short encoding of the same direct absolute dereference that
    fixed-i386-disp32-v1 accepts as mod00 rm101.  Displacement must equal the
    owner base VA exactly.

fixed-i386-regimm-v1
    A two-instruction straight-line dereference: ``mov r32, imm32`` (B8+r, no
    prefix) materializes an address in r, and a later MOV/MOVSX/MOVZX/PUSH reads or
    writes ``[r]``, ``[r + disp8]`` or ``[r + disp32]`` (base r only: no SIB, index or segment)
    with imm + disp == owner base VA.  Every instruction strictly between them, in
    the complete linear decode of the reviewed native function, must neither write
    r (or any sub-register, explicit or implicit) nor transfer control.  The
    fall-through path from the materialization therefore dereferences the base.

Both forms prove only what fixed-i386-disp32-v1 proves: some independently reviewed
native code accesses the owner base address with the given width at the preferred
image base.  They prove no original symbol, type, TU, object start or extent; the
owner extent remains the separately compiled exact reconstructed owner.
"""
import hashlib

from fixed_disp32 import image_facts, validate_exclusions, _code, _instruction, _scalar_prefixes

MOFFS = 'fixed-i386-moffs32-v1'
REGIMM = 'fixed-i386-regimm-v1'
PROFILES = (MOFFS, REGIMM)
_GPR32 = {'eax': ('eax', 'ax', 'al', 'ah'), 'ecx': ('ecx', 'cx', 'cl', 'ch'),
          'edx': ('edx', 'dx', 'dl', 'dh'), 'ebx': ('ebx', 'bx', 'bl', 'bh'),
          'ebp': ('ebp', 'bp'), 'esi': ('esi', 'si'), 'edi': ('edi', 'di')}
_MOV_IMM = {0xb8: 'eax', 0xb9: 'ecx', 0xba: 'edx', 0xbb: 'ebx', 0xbd: 'ebp', 0xbe: 'esi', 0xbf: 'edi'}


def _gate(pe, rva, extent):
    """Identical native storage gate to fixed_disp32.verify_native (kept separate, unmodified)."""
    p, oh = pe.pe, pe.pe.OPTIONAL_HEADER
    facts = image_facts(pe)
    if (p.FILE_HEADER.Machine != 0x14c or oh.Magic != 0x10b or
            not p.FILE_HEADER.Characteristics & 1 or p.FILE_HEADER.Characteristics & 0x2000 or
            facts['directories'][5] != [0, 0] or getattr(p, 'DIRECTORY_ENTRY_BASERELOC', [])):
        raise ValueError('Fixed anchor form requires a stripped fixed-base i386 EXE')
    if (type(rva) is not int or type(extent) is not int or rva < 0 or extent <= 0 or
            rva + extent > min(0x100000000 - pe.image_base, oh.SizeOfImage)):
        raise ValueError('Invalid fixed anchor owner extent')
    end = rva + extent
    sections = [s for s in pe.sections if s['virtual_address'] < end and
                rva < s['virtual_address'] + max(s['virtual_size'], s['raw_size'])]
    if len(sections) != 1:
        raise ValueError('Fixed anchor owner needs one unambiguous section')
    sec = sections[0]
    if (rva < sec['virtual_address'] + sec['raw_size'] or end > sec['virtual_address'] + sec['virtual_size'] or
            sec['characteristics'] & (0x20000020 | 0x02000000) or
            sec['characteristics'] & 0xc0000000 != 0xc0000000):
        raise ValueError('Fixed anchor owner must be wholly in a writable zero tail')
    if any(d.Size and d.VirtualAddress < end and rva < d.VirtualAddress + d.Size for d in oh.DATA_DIRECTORY):
        raise ValueError('Fixed anchor owner overlaps a PE directory')
    if any(oh.DATA_DIRECTORY[n].Size or oh.DATA_DIRECTORY[n].VirtualAddress for n in (9, 10)):
        raise ValueError('Fixed anchor form rejects unreviewed TLS/load-config effects')
    return facts, sec


def _context(pe, profile, spans):
    """Reviewed containing function: hash, independence, complete linear decode."""
    import capstone
    boundary = profile.get('boundary')
    if not isinstance(boundary, dict):
        raise ValueError('Fixed anchor form needs a reviewed native function boundary')
    raw = _code(pe, boundary.get('rva'), boundary.get('size'))
    if any(boundary['rva'] < lo + length and lo < boundary['rva'] + boundary['size'] for lo, length in spans):
        raise ValueError('Fixed anchor containing native function overlaps a pending/selected caller')
    if hashlib.sha256(raw).hexdigest() != boundary.get('sha256'):
        raise ValueError('Fixed anchor native context hash changed')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    decoded = list(decoder.disasm(raw, boundary['rva']))
    if sum(i.size for i in decoded) != len(raw):
        raise ValueError('Fixed anchor native context does not decode completely')
    return boundary, decoded


def _at(decoded, ins):
    hits = [k for k, i in enumerate(decoded) if i.address == ins.address and bytes(i.bytes) == bytes(ins.bytes)]
    if len(hits) != 1:
        raise ValueError('Fixed anchor instruction is not at a reviewed context instruction boundary')
    return hits[0]


def _moffs(pe, ins, n, base_va, extent):
    import capstone
    raw = bytes(ins.bytes)
    opcode = raw[1] if raw[:1] == b'\x66' else raw[0]
    if (ins.mnemonic != 'mov' or opcode not in (0xa0, 0xa1, 0xa2, 0xa3) or not _scalar_prefixes(ins) or
            ins.modrm_offset or ins.addr_size != 4 or ins.disp_size != 4 or
            ins.disp_offset != len(raw) - 4 or n >= len(ins.operands)):
        raise ValueError('Fixed moffs32 requires MOV accumulator <-> [moffs32]')
    op = ins.operands[n]
    if (op.type != capstone.x86.X86_OP_MEM or op.mem.base or op.mem.index or op.mem.segment or
            op.size not in (1, 2, 4) or op.size > extent):
        raise ValueError('Fixed moffs32 requires one unsegmented scalar memory operand')
    if op.mem.disp & 0xffffffff != base_va:
        raise ValueError('Fixed moffs32 anchor identifies another base/interior cell')
    return dict(form='moffs32', opcode=opcode, disp_offset=ins.disp_offset, disp_size=4,
                address_size=4, mnemonic='mov', width=op.size)


def _regimm(pe, ins, n, base_va, extent, decoded, at, materialize):
    import capstone
    if ins.mnemonic not in ('mov', 'movsx', 'movzx', 'push') or n >= len(ins.operands):
        raise ValueError('Fixed regimm requires a dereferencing MOV/MOVSX/MOVZX/PUSH')
    op = ins.operands[n]
    mems = [o for o in ins.operands if o.type == capstone.x86.X86_OP_MEM]
    if (op.type != capstone.x86.X86_OP_MEM or len(mems) != 1 or not op.mem.base or op.mem.index or
            op.mem.segment or ins.addr_size != 4 or not _scalar_prefixes(ins) or
            op.size not in (1, 2, 4) or op.size > extent):
        raise ValueError('Fixed regimm requires one unsegmented base-register scalar memory operand')
    register = ins.reg_name(op.mem.base)
    mod, rm = ins.modrm >> 6, ins.modrm & 7
    if register not in _GPR32 or mod == 3 or rm == 4 or (mod == 0 and rm == 5):
        raise ValueError('Fixed regimm requires a non-SIB [r32], [r32 + disp8] or [r32 + disp32] operand')
    mat = _instruction(pe, materialize)
    raw = bytes(mat.bytes)
    if (mat.size != 5 or raw[0] not in _MOV_IMM or _MOV_IMM[raw[0]] != register or
            mat.mnemonic != 'mov' or mat.imm_size != 4 or mat.imm_offset != 1 or materialize['operand'] != 1):
        raise ValueError('Fixed regimm materialization must be one unprefixed mov r32, imm32')
    first = _at(decoded, mat)
    if first >= at:
        raise ValueError('Fixed regimm materialization must precede the dereference')
    aliases = set(_GPR32[register])
    for between in decoded[first + 1:at]:
        if any(between.group(g) for g in (capstone.CS_GRP_JUMP, capstone.CS_GRP_CALL, capstone.CS_GRP_RET,
                                          capstone.CS_GRP_IRET, capstone.CS_GRP_INT, capstone.CS_GRP_BRANCH_RELATIVE)):
            raise ValueError('Fixed regimm path contains a control transfer')
        _, written = between.regs_access()
        if aliases & {between.reg_name(r) for r in written}:
            raise ValueError('Fixed regimm address register is overwritten before the dereference')
    immediate = mat.operands[1].imm & 0xffffffff
    if (immediate + op.mem.disp) & 0xffffffff != base_va:
        raise ValueError('Fixed regimm dereference identifies another base/interior cell')
    return dict(form='regimm', register=register, immediate=immediate, displacement=op.mem.disp,
                materialize=dict(rva=mat.address, size=mat.size), modrm=ins.modrm,
                address_size=4, mnemonic=ins.mnemonic, width=op.size)


def verify_native(pe, rva, extent, identity, caller_rva, caller_size, profile, anchor_profile):
    if anchor_profile not in PROFILES:
        raise ValueError('Unsupported fixed anchor form profile')
    facts, sec = _gate(pe, rva, extent)
    if facts != profile.get('image'):
        raise ValueError('Fixed anchor image fingerprint changed')
    encoding = profile.get('encoding')
    if not isinstance(encoding, dict):
        raise ValueError('Fixed anchor form needs a pinned encoding receipt')
    ins = _instruction(pe, identity)
    excluded = validate_exclusions(profile, [])
    spans = [(caller_rva, caller_size)] + [(x['rva'], x['size']) for x in excluded]
    if any(ins.address < lo + length and lo < ins.address + ins.size for lo, length in spans):
        raise ValueError('Fixed anchor overlaps a pending/selected caller')
    boundary, decoded = _context(pe, profile, spans)
    at = _at(decoded, ins)
    if boundary.get('anchor_offset') != ins.address - boundary['rva']:
        raise ValueError('Fixed anchor native context offset changed')
    base_va = pe.image_base + rva
    if anchor_profile == MOFFS:
        receipt = _moffs(pe, ins, identity['operand'], base_va, extent)
    else:
        materialize = encoding.get('materialize')
        if not isinstance(materialize, dict) or set(materialize) != {'rva', 'size', 'operand', 'sha256'}:
            raise ValueError('Fixed regimm needs one pinned materialization instruction')
        receipt = _regimm(pe, ins, identity['operand'], base_va, extent, decoded, at, materialize)
        receipt['materialize'] = dict(materialize)
    if receipt != encoding:
        raise ValueError('Fixed anchor reviewed encoding receipt changed')
    return dict(profile=anchor_profile, identity_rva=ins.address, width=receipt['width'],
                section=sec['name'], preferred_base_only=True, global_isolation_proved=False)
