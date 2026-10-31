"""Opt-in indexed disp32 uses of independently anchored DLL loader-zero owners.

Identity still comes solely from the unchanged zero_storage owner check and
either the legacy native anchor or a loader-zero-paired-v1 anchor
(loader_zero_paired.py). The record states that its exact compiled owner extent
is a proven minimum, not the complete native object: no capacity, upper bound or
runtime index range is inferred from neighbours, linker layout or this form.
An indexed use proves only that its relocated disp32 is owner + addend inside
that minimum. Direct and immediate uses keep the legacy loader-zero checks.
The shared fixed_disp32 comparator dispatches this form only for opted-in records.
"""
import struct

from fixed_indexed import verify_indexed_access

ACCESS_PROFILE = 'loader-zero-indexed-v1'
EXTENT_CLAIMS = ('minimum',)


def validate_record(record):
    if (record.get('access_profile') != ACCESS_PROFILE or record.get('kind') != 'bss' or
            record.get('anchor_profile', 'loader-zero-paired-v1') != 'loader-zero-paired-v1' or
            'fixed_disp32' in record):
        raise ValueError('Loader-zero indexed profile requires an unchanged loader-zero anchor (legacy or paired)')
    survey = record.get('loader_zero_indexed')
    if (not isinstance(survey, dict) or set(survey) != {'extent_claim', 'accesses'} or
            survey['extent_claim'] not in EXTENT_CLAIMS or not isinstance(survey['accesses'], list) or
            not any(isinstance(a, dict) and a.get('profile') == ACCESS_PROFILE for a in survey['accesses'])):
        raise ValueError('Loader-zero indexed profile needs a minimum-extent claim and an indexed access survey')


def verify_record_access(ins, off, addend, extent, caller_rva, record, expected, owner_va):
    import capstone
    if ins is None or type(off) is not int or type(addend) is not int or type(extent) is not int:
        raise ValueError('Loader-zero references require direct absolute instruction operands')
    start = ins.address - caller_rva
    if not (ins.disp_size == 4 and start + ins.disp_offset == off):
        if not (ins.imm_size == 4 and start + ins.imm_offset == off):
            raise ValueError('Loader-zero relocation is not one complete instruction operand')
        # The legacy path admits an absolute immediate address unchanged.
        return dict(offset=off, addend=addend, form='immediate')
    mem = [o for o in ins.operands if o.type == capstone.x86.X86_OP_MEM]
    if len(mem) == 1 and not (mem[0].mem.base or mem[0].mem.index):
        if mem[0].mem.segment or addend + mem[0].size > extent:
            raise ValueError('Loader-zero memory access exceeds its complete scalar owner')
        return dict(offset=off, addend=addend, width=mem[0].size, form='direct')
    receipt = dict(verify_indexed_access(ins, off, addend, extent, caller_rva), profile=ACCESS_PROFILE)
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    native = list(decoder.disasm(expected[start:start + ins.size], ins.address))
    if (len(native) != 1 or native[0].size != ins.size or
            dict(verify_indexed_access(native[0], off, addend, extent, caller_rva),
                 profile=ACCESS_PROFILE) != receipt):
        raise ValueError('Indexed native decoding differs from the reviewed compiler form')
    if struct.unpack_from('<I', expected, off)[0] != owner_va + addend:
        raise ValueError('Indexed native displacement differs from owner plus addend')
    return receipt
