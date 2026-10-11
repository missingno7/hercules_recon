"""Bounded compiler mismatch explanations. Never acceptance or behavior proof."""
from collections import Counter
import re
import env  # Configure the pinned analysis dependency path before lazy decoding.

def instruction_rows(code, base, fields):
    """Mask only the encoded relocation operand, retaining constants and scales.

    Linear decoding may include inline data. The keys are for presentation;
    full byte and relocation-binding checks decide equality separately.
    """
    import capstone
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    out = []
    for i in md.disasm(code, base):
        text = f'{i.mnemonic} {i.op_str}'.strip()
        operands = i.op_str.split(', ')
        local_fields = {f - (i.address - base) for f in fields
                        if i.address - base <= f < i.address - base + i.size}
        for field in local_fields:
            if field == i.disp_offset and i.disp_size == 4:
                for n, operand in enumerate(i.operands):
                    if operand.type != X86_OP_MEM:
                        continue
                    def memory(match):
                        inside = match.group(1)
                        if re.fullmatch(r'(?:0x[0-9a-f]+|[0-9]+)', inside):
                            return '[R]'
                        inside, count = re.subn(r' (?:\+|-) (?:0x[0-9a-f]+|[0-9]+)$', ' + R', inside)
                        return '[' + inside + ('' if count else ' + R') + ']'
                    operands[n] = re.sub(r'\[([^\]]+)\]', memory, operands[n])
            elif field == i.imm_offset and i.imm_size == 4:
                for n, operand in enumerate(i.operands):
                    if operand.type == X86_OP_IMM:
                        operands[n] = 'R'
        key = (i.mnemonic + ' ' + ', '.join(operands)).strip()
        if i.mnemonic.startswith('j') or i.mnemonic == 'call':
            m = re.fullmatch(r'0x([0-9a-f]+)', i.op_str)
            if 'R' in operands:
                key = f'{i.mnemonic} EXTERNAL'
            elif m:
                target = int(m.group(1), 16) - base
                key = f'{i.mnemonic} +{target:#x}' if 0 <= target < len(code) else f'{i.mnemonic} EXTERNAL'
        out.append((i.address - base, key, text, i.size))
    return out


def mismatch_islands(expected, actual, regions, target_fields=(), object_fields=(), limit=4):
    """Group residual byte spans within eight bytes and explain complete windows."""
    if limit < 1:
        raise ValueError('Island limit must be positive')
    original = instruction_rows(expected, 0, target_fields)
    compiled = instruction_rows(actual, 0, object_fields)
    groups = []
    for lo, hi in regions:
        if groups and lo <= groups[-1][1] + 8:
            groups[-1][1] = hi
        else:
            groups.append([lo, hi])
    islands = []
    for lo, hi in groups[:limit]:
        a = [r for r in original if r[0] < hi and r[0] + r[3] > lo]
        b = [r for r in compiled if r[0] < hi and r[0] + r[3] > lo]
        start = min([lo] + [r[0] for r in a + b])
        end = max([hi] + [r[0] + r[3] for r in a + b])
        a = [r for r in original if start <= r[0] < end]
        b = [r for r in compiled if start <= r[0] < end]
        complete = all(rows and rows[0][0] == start and rows[-1][0] + rows[-1][3] == end
                       and sum(r[3] for r in rows) == end - start for rows in (a, b))
        ak, bk = [r[1] for r in a], [r[1] for r in b]
        kind = 'instruction_or_layout_difference' if complete else 'partial_or_undecoded_window'
        if complete and ak != bk and Counter(ak) == Counter(bk):
            kind = 'instruction_order_difference'
        elif complete and ak != bk:
            def registers(keys):
                return [re.sub(r'\b(?:eax|ecx|edx|ebx|ebp|esi|edi)\b', 'GPR32',
                               re.sub(r'\b(?:ax|cx|dx|bx|bp|si|di)\b', 'GPR16',
                                      re.sub(r'\b(?:al|cl|dl|bl|ah|ch|dh|bh)\b', 'GPR8', k))) for k in keys]
            if registers(ak) == registers(bk):
                kind = 'register_operand_difference'
        islands.append(dict(mismatch_span=[lo, hi], decoded_span=[start, end], kind=kind,
                            original=[dict(offset=hex(r[0]), instruction=r[2]) for r in a[:16]],
                            compiled=[dict(offset=hex(r[0]), instruction=r[2]) for r in b[:16]],
                            omitted_instructions=dict(original=max(0, len(a)-16), compiled=max(0, len(b)-16))))
    return dict(islands=islands, total_islands=len(groups), omitted_islands=max(0, len(groups)-limit))


def residual(module, rva, size, symbol, source, flags, limit=4):
    from pathlib import Path
    import sqlite3
    from leaf import check, module_path, DB
    from coff import COFF
    from masked import masked_compare
    from masked import REL32, DIR32
    from match import digest
    from pe import PE
    result = check(module, rva, size, symbol, source, flags)
    target = module_path(sqlite3.connect(DB), module)
    pe = PE(target)
    section = COFF(result['object']).function(symbol)
    masked = masked_compare(result['object'], symbol, target, rva, size)
    target_fields = {e.rva-rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries
                     if e.type and rva <= e.rva < rva + size}
    islands = mismatch_islands(pe.read_rva(rva, size), section['data'], masked['mismatch_regions'],
                              target_fields, {r['offset'] for r in section['relocations'] if r['type'] in (REL32, DIR32)}, limit)
    if digest(target.read_bytes()) != result['target_file_sha256'] or digest(Path(source).read_bytes()) != result['source_sha256']:
        raise ValueError('Source or oracle changed during residual analysis')
    return dict(scope='Diagnostic linear instruction windows; may include inline data. No recovery-state change.',
                raw=result, masked=masked, **islands)
