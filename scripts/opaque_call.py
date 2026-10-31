"""Opt-in near indirect CALLs whose runtime target stays explicitly unidentified.

A record admits one compiled ``call`` (FF /2, no prefix) that the shared
comparator otherwise refuses, and proves only instruction identity:

* register (``call eax``) and register-based memory (``call [ecx+0x10]``)
  operands carry no relocation, and the native instruction bytes are identical;
* an absolute memory cell (``call [cell+k]``) has exactly one DIR32 field, bound
  by the comparator to a loader-zero (BSS) record, whose identity rests on its
  freshly compiled exact owner and independent native anchor, and reads 4 bytes
  inside that record's proven extent; the native displacement is that cell and
  every other instruction byte is identical.

It claims no callee identity, calling convention, argument use or runtime ABI
compatibility: the target is whatever value the register or cell holds at run
time. Every record states that (target "unidentified", abi "unclaimed") and pins
the reviewed operand, so a changed compiler form or a different native operand
fails. JMP, far, prefixed, segment-overridden, import and relocated
register-based forms are refused. The comparator still requires every byte of
the contribution to match; unrecorded indirect transfers stay refused.

Review hardening (adversarial review, 2026-10-31): the target image must carry a
base-relocation inventory, because only that inventory distinguishes an
unrelocated literal from an original address (relocation-stripped HERCULES.EXE
is out of scope); payload-hash ``data`` records may not cover PE data
directories or import slots in a row that opts in, so no import can be re-labelled
as data and called through a cell or a loaded register.
"""
import struct

PROFILE = 'opaque-indirect-call-v1'
KEYS = {'profile', 'offset', 'operand', 'target', 'abi', 'evidence'}
REGISTERS = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')
DIR32 = 6


def validate_record(record):
    if not isinstance(record, dict) or set(record) != KEYS or record['profile'] != PROFILE:
        raise ValueError('Invalid opaque indirect call record')
    if type(record['offset']) is not int or record['offset'] < 0:
        raise ValueError('Invalid opaque call offset')
    if record['target'] != 'unidentified' or record['abi'] != 'unclaimed':
        raise ValueError('Opaque calls claim no callee identity or ABI')
    evidence = record['evidence']
    if not isinstance(evidence, list) or not evidence or any(
            not isinstance(item, str) or not item.startswith('evidence/') for item in evidence):
        raise ValueError('Opaque call needs durable review evidence')
    operand = record['operand']
    form = operand.get('form') if isinstance(operand, dict) else None
    if form == 'register':
        valid = set(operand) == {'form', 'register'} and operand['register'] in REGISTERS
    elif form == 'memory':
        valid = (set(operand) == {'form', 'base', 'index', 'scale', 'disp'} and
                 operand['base'] in REGISTERS + (None,) and operand['index'] in REGISTERS + (None,) and
                 (operand['base'] or operand['index']) and operand['index'] != 'esp' and
                 type(operand['scale']) is int and operand['scale'] in (1, 2, 4, 8) and
                 type(operand['disp']) is int)
    elif form == 'cell':
        valid = (set(operand) == {'form', 'symbol', 'addend'} and isinstance(operand['symbol'], str) and
                 type(operand['addend']) is int and operand['addend'] >= 0)
    else:
        valid = False
    if not valid:
        raise ValueError('Invalid opaque call operand')


def describe(ins):
    """Decode-level shape of one near indirect CALL; claims nothing about its target."""
    from capstone.x86 import X86_OP_MEM, X86_OP_REG
    raw = bytes(ins.bytes)
    # A first byte of FF excludes every prefix (operand size, segment, lock, rep).
    if (ins.mnemonic != 'call' or len(raw) < 2 or raw[0] != 0xff or (raw[1] >> 3) & 7 != 2 or
            len(ins.operands) != 1 or ins.operands[0].size != 4):
        raise ValueError('Opaque call must be one unprefixed near FF /2 CALL')
    operand = ins.operands[0]
    if operand.type == X86_OP_REG:
        return dict(form='register', register=ins.reg_name(operand.reg))
    if operand.type != X86_OP_MEM or operand.mem.segment:
        raise ValueError('Opaque call operand must be a register or flat memory')
    mem = operand.mem
    base = ins.reg_name(mem.base) if mem.base else None
    index = ins.reg_name(mem.index) if mem.index else None
    if base is None and index is None:
        if ins.disp_size != 4:
            raise ValueError('Opaque absolute call needs a disp32 cell')
        return dict(form='cell', disp=mem.disp & 0xffffffff)
    return dict(form='memory', base=base, index=index, scale=mem.scale, disp=mem.disp)


def _loader_managed(pe, start, end):
    """Whether [start, end) touches any PE data directory or import thunk slot."""
    slots = [i.address - pe.image_base for m in getattr(pe.pe, 'DIRECTORY_ENTRY_IMPORT', []) for i in m.imports]
    return (any(d.Size and d.VirtualAddress < end and start < d.VirtualAddress + d.Size
                for d in pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY) or
            any(start < slot + 4 and slot < end for slot in slots))


def verify_calls(pe, opaque, records, reviewed, instructions, rva, expected):
    """Return {offset: receipt} for every opted-in call; raise on any deviation."""
    import capstone
    if not pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].Size:
        raise ValueError('Opaque calls need a base-relocated image: a literal would be indistinguishable from an address')
    for name, cell in records.items():
        if cell.get('kind') == 'data' and (name.startswith('__imp_') or
                                           _loader_managed(pe, cell['rva'], cell['rva'] + cell['size'])):
            raise ValueError('Opaque calls refuse data records over import slots or PE directories')
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    starts = {ins.address - rva: ins for ins in instructions}
    receipts = {}
    for record in opaque:
        validate_record(record)
        offset = record['offset']
        ins = starts.get(offset)
        if offset in receipts or ins is None:
            raise ValueError('Opaque call offset is duplicated or not a compiled instruction boundary')
        end = offset + ins.size
        shape = describe(ins)
        native = list(decoder.disasm(bytes(expected[offset:end]), ins.address))
        if len(native) != 1 or native[0].size != ins.size:
            raise ValueError('Native opaque call instruction boundary differs')
        native_shape = describe(native[0])
        fields = sorted(field for field in reviewed if offset - 3 <= field < end)
        if shape['form'] != 'cell':
            if fields:
                raise ValueError('Opaque register-based call cannot contain a relocation')
            if bytes(expected[offset:end]) != bytes(ins.bytes) or native_shape != shape:
                raise ValueError('Native opaque call bytes differ')
            if record['operand'] != shape:
                raise ValueError('Reviewed opaque call operand differs')
        else:
            field = offset + ins.disp_offset
            if fields != [field] or native_shape['form'] != 'cell':
                raise ValueError('Opaque cell call needs exactly its own disp32 relocation')
            name, kind, addend, one_past = reviewed[field]
            cell = records.get(name, {})
            if kind != DIR32 or one_past or cell.get('kind') != 'bss':
                raise ValueError('Opaque cell call needs an independently proven loader-zero (BSS) cell')
            if not (0 <= addend and addend + 4 <= cell['size']):
                raise ValueError('Opaque cell call reads outside its proven cell')
            if record['operand'] != dict(form='cell', symbol=name, addend=addend):
                raise ValueError('Reviewed opaque call operand differs')
            raw, disp = bytes(ins.bytes), ins.disp_offset
            if (bytes(expected[offset:field]) != raw[:disp] or bytes(expected[field + 4:end]) != raw[disp + 4:] or
                    native_shape['disp'] != struct.unpack_from('<I', expected, field)[0] or
                    native_shape['disp'] != pe.image_base + cell['rva'] + addend):
                raise ValueError('Native opaque call differs from its proven cell')
        receipts[offset] = dict(profile=PROFILE, offset=offset, size=ins.size, operand=record['operand'],
                                target='unidentified', abi='unclaimed', evidence=record['evidence'])
    return receipts
