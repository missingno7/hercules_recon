"""Opt-in indexed disp32 uses of independently anchored loader-zero owners.

Identity still comes solely from the existing fixed anchor and exact owner.
Only the static relocated address (owner + addend) is bounded here; no runtime
index range, effective-address footprint, or source type is inferred.
The shared fixed_disp32 comparator dispatches this form only for opted-in records.
"""
import struct

from fixed_disp32 import (PROFILE, FORMS, _scalar_prefixes,
                          verify_access as verify_direct_access)

ACCESS_PROFILE = 'fixed-i386-indexed-v1'


def validate_record(record):
    if 'access_profile' not in record:
        return
    if (record['access_profile'] != ACCESS_PROFILE or record.get('kind') != 'bss' or
            record.get('anchor_profile') not in (PROFILE,) + FORMS):
        raise ValueError('Indexed access profile requires an existing fixed loader-zero anchor')
    accesses = record.get('fixed_disp32', {}).get('accesses', [])
    if not any(isinstance(a, dict) and a.get('profile') == ACCESS_PROFILE for a in accesses):
        raise ValueError('Indexed access profile needs an explicit indexed access survey')


def indexed_encoding(ins):
    """Decode only unsegmented scalar dereferences with a real disp32 and register.

    Accept ModRM mod10, or SIB mod00 with no base and a real index. Absolute
    mod00/rm101 and absolute SIB without index remain on the legacy direct path.
    """
    import capstone
    mems = [(n, o) for n, o in enumerate(ins.operands)
            if o.type == capstone.x86.X86_OP_MEM] if ins else []
    if (ins is None or ins.mnemonic == 'lea' or ins.group(capstone.CS_GRP_CALL) or
            ins.group(capstone.CS_GRP_JUMP) or ins.addr_size != 4 or
            not _scalar_prefixes(ins) or not ins.modrm_offset or ins.disp_size != 4 or
            len(mems) != 1):
        raise ValueError('Indexed access requires one scalar ModRM/SIB disp32 dereference')
    n, op = mems[0]
    mod, rm = ins.modrm >> 6, ins.modrm & 7
    if (op.mem.segment or not (op.mem.base or op.mem.index) or op.size not in (1, 2, 4) or
            not (mod == 2 or mod == 0 and rm == 4 and not op.mem.base and op.mem.index)):
        raise ValueError('Indexed access requires an unsegmented register-indexed disp32 form')
    # Pin the complete instruction form as well as the decoded addressing tuple.
    raw = bytearray(ins.bytes)
    raw[ins.disp_offset:ins.disp_offset + 4] = b'\0' * 4
    return dict(instruction=bytes(raw).hex(), mnemonic=ins.mnemonic, operand=n,
                modrm=ins.modrm, sib=ins.sib if rm == 4 else None,
                base=ins.reg_name(op.mem.base), index=ins.reg_name(op.mem.index),
                scale=op.mem.scale, disp_offset=ins.disp_offset, disp_size=4,
                address_size=4, width=op.size)


def verify_indexed_access(ins, off, addend, extent, caller_rva):
    encoding = indexed_encoding(ins)
    if (type(off) is not int or type(addend) is not int or type(extent) is not int or
            ins.address - caller_rva + ins.disp_offset != off or not 0 <= addend < extent):
        raise ValueError('Indexed relocation must be exactly the disp32 with addend inside its owner')
    # Unlike a direct access, width does not bound the effective address here.
    return dict(profile=ACCESS_PROFILE, offset=off, addend=addend,
                width=encoding['width'], encoding=encoding)


def verify_record_access(ins, off, addend, extent, caller_rva, record, expected, owner_va):
    import capstone
    indexed = ins is not None and any(o.type == capstone.x86.X86_OP_MEM and
                                     (o.mem.base or o.mem.index) for o in ins.operands)
    if record.get('access_profile') != ACCESS_PROFILE or not indexed:
        return verify_direct_access(ins, off, addend, extent, caller_rva)
    receipt = verify_indexed_access(ins, off, addend, extent, caller_rva)
    start = ins.address - caller_rva
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    native = list(decoder.disasm(expected[start:start + ins.size], ins.address))
    if (len(native) != 1 or native[0].size != ins.size or
            verify_indexed_access(native[0], off, addend, extent, caller_rva) != receipt):
        raise ValueError('Indexed native decoding differs from the reviewed compiler form')
    if struct.unpack_from('<I', expected, off)[0] != owner_va + addend:
        raise ValueError('Indexed native displacement differs from owner plus addend')
    return receipt
