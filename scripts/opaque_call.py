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
fails. This profile refuses JMP, far, prefixed, segment-overridden, import and
relocated register-based forms. The comparator still requires every byte of the
contribution to match; unrecorded indirect transfers stay refused.

Sibling profile ``opaque-tail-jump-v1`` admits one compiled near JMP (FF /4, no
prefix) through an absolute memory cell only: ``jmp [cell+k]``, the tail form of
a wrapper that forwards through a stored pointer. Its record has the same schema
(target "unidentified", abi "unclaimed", durable evidence) with the cell operand
form alone, and its cell checks are exactly those of the CALL cell form above:
one DIR32 field bound to a proven loader-zero (BSS) record, 4 bytes read inside
that record's proven extent, the native displacement equal to that cell, every
other instruction byte identical, and the same base-relocation and import-slot
hardening below. The JMP must also be in tail position: every byte of the
contribution after it, compiled and native, is compiler padding (NOP or INT3). It proves instruction identity and the cell, and nothing about
the jump target, its arguments, stack cleanup or return convention. Register and
register-based memory JMPs (jump tables and all other dispatch) are not admitted,
so such a transfer cannot enter through this form; neither are far, prefixed or
segment-overridden JMPs. Each profile admits only its own mnemonic: a CALL record
on a JMP, or a JMP record on a CALL, is refused. Every receipt carries the
profile of the record it proves.

Review hardening (adversarial review, 2026-10-31): the target image must carry a
base-relocation inventory, because only that inventory distinguishes an
unrelocated literal from an original address (relocation-stripped HERCULES.EXE
is out of scope); payload-hash ``data`` records may not cover PE data
directories or import slots in a row that opts in, so no import can be re-labelled
as data and called through a cell or a loaded register; a BSS cell record whose
extent touches an import slot or PE data directory is refused as well (a thunk
array in a zero tail is loader-written, not loader-zero). Both profiles share this
hardening.
"""
import struct

PROFILE = 'opaque-indirect-call-v1'
JUMP_PROFILE = 'opaque-tail-jump-v1'
PROFILES = (PROFILE, JUMP_PROFILE)
KEYS = {'profile', 'offset', 'operand', 'target', 'abi', 'evidence'}
REGISTERS = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')
DIR32 = 6
PADDING = (b'\x90', b'\xcc')


def validate_record(record):
    if not isinstance(record, dict) or set(record) != KEYS or record['profile'] not in PROFILES:
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
    if record['profile'] == JUMP_PROFILE and form != 'cell':
        raise ValueError('Opaque tail jumps admit only an absolute cell operand')
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


def describe_jump(ins):
    """Decode-level shape of one near absolute-memory JMP; claims nothing about its target."""
    from capstone.x86 import X86_OP_MEM
    raw = bytes(ins.bytes)
    # A first byte of FF excludes every prefix (operand size, segment, lock, rep).
    if (ins.mnemonic != 'jmp' or len(raw) < 2 or raw[0] != 0xff or (raw[1] >> 3) & 7 != 4 or
            len(ins.operands) != 1 or ins.operands[0].size != 4):
        raise ValueError('Opaque tail jump must be one unprefixed near FF /4 JMP')
    operand = ins.operands[0]
    if operand.type != X86_OP_MEM or operand.mem.segment:
        raise ValueError('Opaque tail jump operand must be flat memory, never a register')
    mem = operand.mem
    if mem.base or mem.index:
        raise ValueError('Opaque tail jump cannot be register-based: jump tables and dispatch stay refused')
    if ins.disp_size != 4:
        raise ValueError('Opaque tail jump needs a disp32 cell')
    return dict(form='cell', disp=mem.disp & 0xffffffff)


def _loader_managed(pe, start, end):
    """Whether [start, end) touches any PE data directory or import thunk slot."""
    slots = [i.address - pe.image_base for m in getattr(pe.pe, 'DIRECTORY_ENTRY_IMPORT', []) for i in m.imports]
    return (any(d.Size and d.VirtualAddress < end and start < d.VirtualAddress + d.Size
                for d in pe.pe.OPTIONAL_HEADER.DATA_DIRECTORY) or
            any(start < slot + 4 and slot < end for slot in slots))


def verify_calls(pe, opaque, records, reviewed, instructions, rva, expected):
    """Return {offset: receipt} for every opted-in call or tail jump; raise on any deviation."""
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
        # Each profile admits only its own mnemonic: the decode below refuses the other.
        what, shape_of = ('tail jump', describe_jump) if record['profile'] == JUMP_PROFILE else ('call', describe)
        offset = record['offset']
        ins = starts.get(offset)
        if offset in receipts or ins is None:
            raise ValueError('Opaque %s offset is duplicated or not a compiled instruction boundary' % what)
        end = offset + ins.size
        shape = shape_of(ins)
        native = list(decoder.disasm(bytes(expected[offset:end]), ins.address))
        if len(native) != 1 or native[0].size != ins.size:
            raise ValueError('Native opaque %s instruction boundary differs' % what)
        native_shape = shape_of(native[0])
        if record['profile'] == JUMP_PROFILE:
            # Tail position: nothing but compiler padding may follow, in the compiled and the native bytes.
            following = [i for i in instructions if i.address - rva >= end]
            if (sum(i.size for i in following) != len(expected) - end or
                    any(bytes(i.bytes) not in PADDING for i in following) or
                    any(byte not in (0x90, 0xcc) for byte in expected[end:])):
                raise ValueError('Opaque tail jump must be the last instruction, followed only by compiler padding')
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
                raise ValueError('Opaque cell %s needs exactly its own disp32 relocation' % what)
            name, kind, addend, one_past = reviewed[field]
            cell = records.get(name, {})
            if kind != DIR32 or one_past or cell.get('kind') != 'bss':
                raise ValueError('Opaque cell %s needs an independently proven loader-zero (BSS) cell' % what)
            if _loader_managed(pe, cell['rva'], cell['rva'] + cell['size']):
                raise ValueError('Opaque cell %s refuses a cell over import slots or PE directories' % what)
            if not (0 <= addend and addend + 4 <= cell['size']):
                raise ValueError('Opaque cell %s reads outside its proven cell' % what)
            if record['operand'] != dict(form='cell', symbol=name, addend=addend):
                raise ValueError('Reviewed opaque %s operand differs' % what)
            raw, disp = bytes(ins.bytes), ins.disp_offset
            if (bytes(expected[offset:field]) != raw[:disp] or bytes(expected[field + 4:end]) != raw[disp + 4:] or
                    native_shape['disp'] != struct.unpack_from('<I', expected, field)[0] or
                    native_shape['disp'] != pe.image_base + cell['rva'] + addend):
                raise ValueError('Native opaque %s differs from its proven cell' % what)
        receipts[offset] = dict(profile=record['profile'], offset=offset, size=ins.size, operand=record['operand'],
                                target='unidentified', abi='unclaimed', evidence=record['evidence'])
    return receipts
