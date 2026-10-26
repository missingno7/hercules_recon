"""Opt-in physical stack proof for the immutable, copied PC interface slots."""
import hashlib
import json
from pathlib import Path

from env import ROOT

REVIEW = 'evidence/interface_slot_review_2026_10_26.json'
REVIEW_SHA256 = 'dd02d7df4e27385f2b579f61124f5b4025e126309e05272eab5f75c089b2b23e'
ROOT_SYMBOL = '_g_engine_interface'


def wrapper_shape(code, address):
    """Recognize the complete straight-line wrapper, including compiler padding.

    Track each loaded argument relative to entry ESP, so saved return addresses,
    duplicated/reordered arguments and arbitrary register pushes cannot qualify.
    """
    import capstone as cs
    from capstone.x86 import X86_OP_REG, X86_OP_MEM, X86_OP_IMM
    decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32); decoder.detail = True
    ins = list(decoder.disasm(code, address))
    if not code or sum(i.size for i in ins) != len(code):
        raise ValueError('Incomplete fixed-slot wrapper decode')
    while ins and bytes(ins[-1].bytes) == b'\x90':
        ins.pop()
    registers, pushed, transfer = {}, [], None
    for n, i in enumerate(ins):
        ops = i.operands
        if i.mnemonic in ('call', 'jmp'):
            if (len(i.bytes) != 6 or bytes(i.bytes[:2]) not in (b'\xff\x15', b'\xff\x25') or
                    len(ops) != 1 or ops[0].type != X86_OP_MEM or ops[0].size != 4 or
                    ops[0].mem.base or ops[0].mem.index or ops[0].mem.segment):
                raise ValueError('Fixed-slot needs exact absolute FF15/FF25 transfer')
            transfer = i
            suffix = ins[n + 1:]
            if i.mnemonic == 'jmp':
                if n or suffix:
                    raise ValueError('Fixed-slot tail JMP must preserve entry stack')
                cleanup = 0
            else:
                cleanup = 4 * len(pushed)
                if (len(suffix) != 2 or bytes(suffix[-1].bytes) != b'\xc3' or
                        suffix[0].mnemonic != 'add' or len(suffix[0].operands) != 2 or
                        suffix[0].operands[0].type != X86_OP_REG or
                        suffix[0].reg_name(suffix[0].operands[0].reg) != 'esp' or
                        suffix[0].operands[1].type != X86_OP_IMM or
                        suffix[0].operands[1].imm != cleanup or not pushed or
                        pushed[::-1] != list(range(1, len(pushed) + 1))):
                    raise ValueError('Fixed-slot argument forwarding/cleanup/RET differs')
            break
        if (i.mnemonic == 'mov' and len(ops) == 2 and ops[0].type == X86_OP_REG and
                ops[0].size == 4 and i.reg_name(ops[0].reg) not in ('esp', 'ebp') and
                ops[1].type == X86_OP_MEM and ops[1].size == 4):
            m = ops[1].mem
            word = m.disp - 4 * len(pushed)
            if i.reg_name(m.base) != 'esp' or m.index or m.segment or word < 4 or word % 4:
                raise ValueError('Fixed-slot load must read its own stack argument')
            registers[ops[0].reg] = word // 4
        elif i.mnemonic == 'push' and len(ops) == 1 and ops[0].type == X86_OP_REG and ops[0].size == 4 and ops[0].reg in registers:
            pushed.append(registers[ops[0].reg])
        else:
            raise ValueError('Unsupported fixed-slot wrapper instruction')
    if transfer is None:
        raise ValueError('Missing fixed-slot transfer')
    return dict(offset=transfer.address - address, transfer=transfer.mnemonic,
                pushed_dwords=len(pushed), cleanup_bytes=cleanup,
                address=transfer.operands[0].mem.disp & 0xffffffff)


def review_inputs(record, target):
    """Also used before compilation to freeze the complete independent inputs."""
    keys = {'profile', 'offset', 'root', 'slot', 'transfer', 'pushed_dwords',
            'cleanup_bytes', 'review_sha256', 'evidence'}
    if not isinstance(record, dict) or set(record) != keys or record['profile'] != 'fixed-slot-v1':
        raise ValueError('Invalid explicit fixed-slot proof')
    for key in ('offset', 'slot', 'pushed_dwords', 'cleanup_bytes'):
        if type(record[key]) is not int or record[key] < 0:
            raise ValueError('Invalid fixed-slot ' + key)
    slot = record['slot']
    if slot > 996 or slot % 4 or record['root'] != ROOT_SYMBOL or record['transfer'] not in ('call', 'jmp'):
        raise ValueError('Invalid fixed-slot root/slot/transfer')
    if record['evidence'] != [REVIEW + '#slots.' + hex(slot)]:
        raise ValueError('Fixed-slot needs the pinned slot review')
    path = ROOT / REVIEW
    raw = path.read_bytes()
    if record['review_sha256'] != REVIEW_SHA256 or hashlib.sha256(raw).hexdigest() != REVIEW_SHA256:
        raise ValueError('Fixed-slot review hash mismatch')
    review = json.loads(raw)
    observed = {path.resolve(): REVIEW_SHA256}
    module = None
    # The target may come from --original-root; the other reviewed originals
    # (uncommitted assets/ and work/discs files) live under that same root.
    originals = ROOT
    for name, row in review['inputs'].items():
        if name == Path(target).name.upper():
            parts = Path(row['path']).parts
            resolved = Path(target).resolve()
            if resolved.parts[-len(parts):] == parts:
                originals = Path(*resolved.parts[:-len(parts)])
    for name, row in review['inputs'].items():
        original = Path(row['path']).parts[:1] == ('assets',) or Path(row['path']).parts[:2] == ('work', 'discs')
        item = ((originals if original else ROOT) / row['path']).resolve()
        # Compare the actual target image, including --original-root copies.
        source = Path(target).resolve() if name == Path(target).name.upper() else item
        actual = hashlib.sha256(source.read_bytes()).hexdigest()
        if actual != row['sha256']:
            raise ValueError('Fixed-slot original input hash mismatch: ' + name)
        observed[source] = actual
        if source == Path(target).resolve():
            module = name
    if module not in review['modules']:
        raise ValueError('Fixed-slot target is not a reviewed module')
    return review, module, observed


def verify_call(pe, record, records, reviewed, instructions, rva, size):
    review, module, _ = review_inputs(record, pe.path)
    owner = records.get(record['root'], {})
    # Either verified loader-zero representation (exact-size COMMON or
    # zero-section-v1) proves the complete 1000-byte root.
    if (owner.get('kind') != 'bss' or owner.get('size') != 1000 or
            owner.get('owner_storage', 'common') not in ('common', 'zero-section-v1') or
            owner.get('owner', {}).get('symbol') != ROOT_SYMBOL or
            owner.get('rva') != review['modules'][module]['root_rva']):
        raise ValueError('Fixed-slot needs its verified 1000-byte zero-section owner')
    compiled = wrapper_shape(b''.join(bytes(i.bytes) for i in instructions), rva)
    native = wrapper_shape(pe.read_rva(rva, size), rva)
    for key in ('offset', 'transfer', 'pushed_dwords', 'cleanup_bytes'):
        if compiled[key] != record[key] or native[key] != record[key]:
            raise ValueError('Fixed-slot wrapper ' + key + ' differs')
    slot = record['slot']; field = record['offset'] + 2
    if (reviewed != {field: (ROOT_SYMBOL, 6, slot, False)} or
            compiled['address'] != slot or native['address'] != pe.image_base + owner['rva'] + slot):
        raise ValueError('Fixed-slot DIR32 root/addend differs')
    row = review['slots'].get(hex(slot), {})
    verdict = row.get('modules', {}).get(module, {})
    form = {key: record[key] for key in ('transfer', 'pushed_dwords', 'cleanup_bytes')}
    abi = row.get('provider_abi', {})
    if (verdict.get('verdict') != 'agree' or form not in verdict.get('forms', []) or
            abi.get('unresolved') or abi.get('effective_ret_immediates') != [0] or
            (record['transfer'] == 'call' and max(abi.get('argument_words', []), default=0) > record['pushed_dwords'])):
        raise ValueError('Fixed-slot provider ABI is conflict/unresolved or incompatible')
    return dict(profile='fixed-slot-v1', offset=record['offset'], root=ROOT_SYMBOL,
                slot=slot, transfer=record['transfer'], pushed_dwords=record['pushed_dwords'],
                cleanup_bytes=record['cleanup_bytes'], provider_rva=row['provider_rva'],
                review_sha256=REVIEW_SHA256, evidence=record['evidence'])
