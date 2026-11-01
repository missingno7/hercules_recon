"""Address-formation (LEA) use of an independently anchored DLL loader-zero owner.

An opt-in companion to loader-zero-indexed-v1 (loader_zero_indexed.py, unchanged).
The record, its anchor, its minimum-extent claim and its access survey are exactly
those of the indexed dereference form. This module only admits one more instruction
shape in that survey: ``lea reg32, [index*scale + disp32]`` (8D /r, mod00 rm100,
SIB base101, a real index, scale 1/2/4/8, no prefix) whose disp32 is the HIGHLOW
field owner + addend with 0 <= addend < the proven minimum extent.

LEA computes an address and accesses no memory. The receipt therefore carries
access='address' and no width, and proves only that the address was formed from
the owner base plus the addend. It makes no claim about what the computed address
is later used for, the width of any later access, an upper bound or a runtime
index range. The HIGHLOW fixup at the field is checked by the shared comparator
(native base-relocation inventory against the compiler DIR32 fields).
Everything that is not this exact LEA goes to loader_zero_indexed unchanged.
"""
import struct

from loader_zero_indexed import ACCESS_PROFILE, verify_record_access as verify_dereference_access

ACCESS = 'address'
LEA_OPCODE = 0x8d


def is_lea(ins):
    """True for any instruction whose opcode byte (after legacy prefixes) is 8D."""
    if ins is None:
        return False
    raw = bytes(ins.bytes)
    i = 0
    while i < len(raw) and raw[i] in (0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0x66, 0x67, 0xf0, 0xf2, 0xf3):
        i += 1
    return ins.mnemonic == 'lea' or raw[i:i + 1] == bytes([LEA_OPCODE])


def lea_encoding(ins):
    """Pin the complete instruction form: no prefixes, SIB, no base, real index, disp32."""
    import capstone
    raw = bytes(ins.bytes) if ins is not None else b''
    mems = [o for o in ins.operands if o.type == capstone.x86.X86_OP_MEM] if ins is not None else []
    if (ins is None or len(raw) != 7 or raw[0] != LEA_OPCODE or ins.mnemonic != 'lea' or
            raw[1] & 0xc7 != 0x04 or raw[2] & 7 != 5 or (raw[2] >> 3) & 7 == 4 or
            ins.addr_size != 4 or ins.disp_size != 4 or ins.disp_offset != 3 or
            ins.modrm != raw[1] or ins.sib != raw[2] or len(mems) != 1 or len(ins.operands) != 2 or
            ins.operands[0].type != capstone.x86.X86_OP_REG or ins.operands[0].size != 4):
        raise ValueError('Indexed address formation requires one unprefixed lea reg32, [index*scale + disp32]')
    mem = mems[0].mem
    if (mem.segment or mem.base or not mem.index or mem.scale not in (1, 2, 4, 8) or
            1 << (raw[2] >> 6) != mem.scale):
        raise ValueError('Indexed address formation requires no segment, no base, one index and scale 1/2/4/8')
    shape = bytearray(raw)
    shape[3:7] = b'\0' * 4
    return dict(instruction=bytes(shape).hex(), mnemonic='lea', modrm=ins.modrm, sib=ins.sib,
                base=None, index=ins.reg_name(mem.index), scale=mem.scale, disp_offset=3, disp_size=4,
                address_size=4)


def verify_lea_access(ins, off, addend, extent, caller_rva):
    encoding = lea_encoding(ins)
    if (type(off) is not int or type(addend) is not int or type(extent) is not int or
            ins.address - caller_rva + ins.disp_offset != off or not 0 <= addend < extent):
        raise ValueError('Indexed address relocation must be exactly the disp32 with addend inside its owner')
    # No width: an address computation proves nothing about any access made through it.
    return dict(profile=ACCESS_PROFILE, access=ACCESS, offset=off, addend=addend, encoding=encoding)


def verify_record_access(ins, off, addend, extent, caller_rva, record, expected, owner_va):
    """loader-zero-indexed-v1 dispatch: LEA here, everything else unchanged."""
    if not is_lea(ins):
        return verify_dereference_access(ins, off, addend, extent, caller_rva, record, expected, owner_va)
    import capstone
    receipt = verify_lea_access(ins, off, addend, extent, caller_rva)
    start = ins.address - caller_rva
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    native = list(decoder.disasm(expected[start:start + ins.size], ins.address))
    try:
        same = len(native) == 1 and native[0].size == ins.size and             verify_lea_access(native[0], off, addend, extent, caller_rva) == receipt
    except ValueError:
        same = False
    if not same:
        raise ValueError('Indexed native decoding differs from the reviewed compiler form')
    if struct.unpack_from('<I', expected, off)[0] != owner_va + addend:
        raise ValueError('Indexed native displacement differs from owner plus addend')
    return receipt
