"""Bounded macro experiment metrics; reuse the strict oracle without promoting.

Relocation resolution is confined to a diagnostic byte buffer, never a build.
Instruction token alignment and ordered CFG shape are diagnostics, not semantic proofs.
"""
import argparse
from collections import Counter
from difflib import SequenceMatcher
import json
from pathlib import Path
import struct
import time

from env import ROOT
from match import compile_source, compare, digest, DEFAULT_TOOLCHAIN, verify_toolchain
from coff import COFF
from pe import PE
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM


def decode(code, address):
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(code, address))
    if sum(i.size for i in instructions) != len(code):
        raise ValueError('Incomplete instruction decoding')
    # Diagnostic instruction metrics exclude natural trailing NOPs only.
    # Strict comparison above includes the entire unmodified contribution.
    while instructions and instructions[-1].mnemonic == 'nop':
        instructions.pop()
    return instructions


def structure(code, address):
    instructions = decode(code, address)
    end = instructions[-1].address + instructions[-1].size
    leaders = {address}
    for i in instructions:
        if i.mnemonic.startswith('j'):
            if i.operands[0].type != X86_OP_IMM:
                return None
            target = i.operands[0].imm
            if not address <= target < end:
                return None
            leaders.add(target)
        if i.mnemonic.startswith(('j', 'ret')) and i.address + i.size < end:
            leaders.add(i.address + i.size)
    leaders = sorted(leaders)
    index = {a: n for n, a in enumerate(leaders)}
    if not set(leaders).issubset({i.address for i in instructions}):
        return None
    blocks = []
    for n, start in enumerate(leaders):
        stop = leaders[n+1] if n+1 < len(leaders) else end
        last = next(i for i in reversed(instructions) if start <= i.address < stop)
        if last.mnemonic.startswith('ret'):
            blocks.append(['return'])
        elif last.mnemonic == 'jmp':
            blocks.append(['jump', index[last.operands[0].imm]])
        elif last.mnemonic.startswith('j'):
            blocks.append([last.mnemonic, index[last.operands[0].imm], n+1])
        else:
            blocks.append(['fallthrough', n+1])
    return blocks


def tokens_and_calls(code, address):
    instructions = decode(code, address)
    end = instructions[-1].address + instructions[-1].size
    tokens, calls, indirect = [], [], []
    for i in instructions:
        operands = i.op_str
        if i.mnemonic.startswith('j') and i.operands[0].type == X86_OP_IMM:
            target = i.operands[0].imm
            if address <= target < end:
                operands = 'local-back' if target < i.address else 'local-forward'
        if i.mnemonic == 'call':
            if i.operands[0].type == X86_OP_IMM:
                calls.append(hex(i.operands[0].imm))
            else:
                indirect.append(i.op_str)
        tokens.append(i.mnemonic + ' ' + operands)
    return tokens, calls, indirect


def encoded_absolute_references(instructions, image_base, sections):
    """Collect encoded address references even in fixed-base, relocation-free EXEs.

    Register-relative displacements and arithmetic/comparison immediates are not
    address evidence. Pointer semantics still require reviewed source evidence;
    this helper audits literal operands only and never changes target bytes.
    """
    references = set()
    for instruction in instructions:
        for operand in instruction.operands:
            value = None
            if operand.type == X86_OP_MEM and operand.mem.base == 0 and instruction.disp_size == 4:
                value = operand.mem.disp & 0xffffffff
            elif operand.type == X86_OP_IMM and instruction.mnemonic in ('mov', 'push') and instruction.imm_size == 4:
                value = operand.imm & 0xffffffff
            if value is None:
                continue
            rva = value - image_base
            if any(s['virtual_address'] <= rva < s['virtual_address'] + max(s['virtual_size'], s['raw_size']) for s in sections):
                references.add(rva)
    return references


def audit_symbol_map(spec, coff, direct_destinations, absolute_operands):
    """Audit actual addresses, including ordinary COFF struct-member addends.

    A source symbol can name a record base while the PC references its members.
    No operand is masked and no acceptance comparison is changed here.
    """
    defined={f['symbol']:int(f['rva'],0) for f in spec['functions']}
    for name,value in spec['symbol_rvas'].items():
        destination=int(value,0)
        if name in defined:
            if defined[name]!=destination:raise ValueError('Internal symbol map disagrees with region')
            continue
        if destination in direct_destinations | absolute_operands:continue
        addresses=[]
        for f in spec['functions']:
            c=coff.function(f['symbol'])
            for relocation in c['relocations']:
                if relocation['symbol']==name and relocation['type']==6:
                    addend=struct.unpack_from('<I',c['data'],relocation['offset'])[0]
                    addresses.append((destination+addend)&0xffffffff)
        if not addresses or not set(addresses).issubset(absolute_operands):
            raise ValueError('External symbol lacks a decoded original reference: '+name)


def direct_external_references(instructions, image_base, start, size):
    """Calls and outgoing unconditional tail jumps are actual code references.

    Local branches and conditional transfers do not identify an external callee.
    This only audits the diagnostic symbol map; strict comparison is unchanged.
    """
    references = set()
    for instruction in instructions:
        if instruction.mnemonic not in ('call', 'jmp') or instruction.operands[0].type != X86_OP_IMM:
            continue
        destination = instruction.operands[0].imm - image_base
        if instruction.mnemonic == 'call' or not start <= destination < start + size:
            references.add(destination)
    return references


def declared_context_files(spec, root):
    """Explicit repository compile dependencies, not a preprocessor or cache."""
    result = {}
    for name in spec.get('context_files', []):
        path = (root / name).resolve()
        if Path(name).is_absolute() or not path.is_relative_to(root.resolve()) or name in result:
            raise ValueError('Invalid or duplicate context file: ' + name)
        result[name] = digest(path.read_bytes())
    return result


def evaluate(source, spec_path, output, obj=None):
    started = time.perf_counter()
    spec = json.loads(spec_path.read_text())
    context_files = declared_context_files(spec, ROOT)
    if obj is not None and context_files:
        raise ValueError('Declared dependencies require a fresh compile; use a legacy analysis spec for existing objects')
    target = ROOT / spec['target']
    pe = PE(target)
    if digest(pe.data) != spec['target_sha256']:
        raise ValueError('Oracle identity changed')
    direct_destinations, absolute_operands = set(), set()
    base_relocations = {e.rva for block in getattr(pe.pe,'DIRECTORY_ENTRY_BASERELOC',[])
                        for e in block.entries if e.type == 3}
    for f in spec['functions']:
        rva = int(f['rva'],0)
        original = pe.read_rva(rva,f['size'])
        instructions = decode(original,pe.image_base+rva)
        absolute_operands.update(encoded_absolute_references(instructions, pe.image_base, pe.sections))
        direct_destinations.update(direct_external_references(instructions, pe.image_base, rva, f['size']))
        for relocation in base_relocations:
            if rva <= relocation and relocation+4 <= rva+f['size']:
                absolute_operands.add(struct.unpack_from('<I',original,relocation-rva)[0]-pe.image_base)
    source_hash = digest(source.read_bytes())
    command = None
    if obj is None:
        obj, command = compile_source(source, spec['flags'])
    coff = COFF(obj)
    audit_symbol_map(spec,coff,direct_destinations,absolute_operands)
    rows = []
    for f in spec['functions']:
        rva = int(f['rva'], 0)
        expected = pe.read_rva(rva, f['size'])
        if digest(expected) != f['target_sha256']:
            raise ValueError('Reviewed function extent/hash changed')
        strict = compare(obj, f['symbol'], target, rva, f['size'])
        contribution = coff.function(f['symbol'])
        diagnostic = bytearray(contribution['data'])
        for relocation in contribution['relocations']:
            offset = relocation['offset']
            destination = int(spec['symbol_rvas'][relocation['symbol']], 0)
            addend = struct.unpack_from('<I', diagnostic, offset)[0]
            if relocation['type'] == 0x14:
                if diagnostic[offset-1] not in (0xe8, 0xe9):
                    raise ValueError('Expected direct CALL/JMP REL32')
                value = destination - (rva + offset + 4) + addend
            elif relocation['type'] == 6:
                value = pe.image_base + destination + addend
            else:
                raise ValueError('Unsupported relocation kind')
            struct.pack_into('<I', diagnostic, offset, value & 0xffffffff)
        different = sum(a != b for a, b in zip(diagnostic, expected)) + abs(len(diagnostic)-len(expected))
        denominator = max(len(diagnostic), len(expected))
        original_tokens, original_calls, original_indirect = tokens_and_calls(expected, pe.image_base+rva)
        generated_tokens, generated_calls, generated_indirect = tokens_and_calls(bytes(diagnostic), pe.image_base+rva)
        lcs = sum(b.size for b in SequenceMatcher(None, original_tokens, generated_tokens, autojunk=False).get_matching_blocks())
        original_cfg = structure(expected, pe.image_base+rva)
        candidate_cfg = structure(bytes(diagnostic), pe.image_base+rva)
        cfg_equal = original_cfg is not None and original_cfg == candidate_cfg
        calls_equal = original_calls == generated_calls
        indirect_count_equal = len(original_indirect) == len(generated_indirect)
        instruction_pct = 100 * lcs / max(len(original_tokens), len(generated_tokens))
        if strict['exact']:
            category = 'raw exact'
        elif different == 0:
            category = 'relocation-only diagnostic equality'
        elif cfg_equal and calls_equal and indirect_count_equal and instruction_pct >= 90:
            category = 'near codegen candidate'
        elif not cfg_equal:
            category = 'structural/source-form investigation'
        else:
            category = 'uncertain allocation/type/source-form'
        rows.append(dict(symbol=f['symbol'], control=f['control'], target_size=f['size'],
                         candidate_size=len(diagnostic), strict_equal=strict['exact'],
                         raw_different_bytes=strict['different_bytes'],
                         raw_byte_percent=round(100*(1-strict['different_bytes']/denominator), 3),
                         normalized_different_bytes=different,
                         normalized_byte_percent=round(100*(1-different/denominator), 3),
                         instruction_percent=round(instruction_pct, 3),
                         original_instruction_count=len(original_tokens),
                         candidate_instruction_count=len(generated_tokens),
                         ordered_cfg_equal=cfg_equal,
                         original_blocks=len(original_cfg) if original_cfg else None,
                         candidate_blocks=len(candidate_cfg) if candidate_cfg else None,
                         direct_call_targets_equal=calls_equal,
                         original_direct_calls=original_calls,candidate_direct_calls=generated_calls,
                         original_indirect_calls=original_indirect,candidate_indirect_calls=generated_indirect,
                         relocation_count=len(contribution['relocations']),
                         category=category))
    if digest(source.read_bytes()) != source_hash or digest(target.read_bytes()) != spec['target_sha256']:
        raise ValueError('Source or target changed during measurement')
    if context_files != declared_context_files(spec, ROOT):
        raise ValueError('Declared compile dependencies changed during measurement')
    summary = dict(functions=len(rows), exact=sum(r['strict_equal'] for r in rows),
                   new_exact=sum(r['strict_equal'] and not r['control'] for r in rows),
                   exact_bytes=sum(r['target_size'] for r in rows if r['strict_equal']),
                   diagnostic_equal=sum(r['normalized_different_bytes']==0 for r in rows),
                   ordered_cfg_equal=sum(r['ordered_cfg_equal'] for r in rows),
                   instruction_over_90=sum(r['instruction_percent']>90 for r in rows),
                   nonexact_instruction_over_90=sum(r['instruction_percent']>90 and not r['strict_equal'] for r in rows),
                   categories=dict(Counter(r['category'] for r in rows)))
    report = dict(schema_version=1, scope='Experiment metrics only; no promotion or executable rewriting.',
                  source=str(source),source_sha256=source_hash,source_bytes=source.stat().st_size,
                  source_lines=len(source.read_text().splitlines()),
                  toolchain=verify_toolchain(DEFAULT_TOOLCHAIN),command=command,
                  object=str(obj),object_sha256=digest(Path(obj).read_bytes()),
                  target_sha256=spec['target_sha256'],flags=spec['flags'],
                  elapsed_seconds=round(time.perf_counter()-started,3),
                  summary=summary,functions=rows)
    if 'context_files' in spec:
        report['context_files_sha256'] = context_files
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(summary))
    for row in rows:
        print(f"{row['symbol']:26s} {row['candidate_size']:3d}/{row['target_size']:3d} "
              f"raw={row['raw_byte_percent']:6.2f}% norm={row['normalized_byte_percent']:6.2f}% "
              f"insn={row['instruction_percent']:6.2f}% CFG={row['ordered_cfg_equal']} {row['category']}")
    return report


def restore_wave(spec, number, final_source):
    """Reconstruct an earlier measured source from reversible, hash-checked edits."""
    waves = spec['waves']
    if not 0 <= number < len(waves):
        raise ValueError('Wave outside ledger')
    if digest(final_source.encode()) != waves[-1]['source_sha256']:
        raise ValueError('Final experimental source identity changed')
    lines = final_source.splitlines(keepends=True)
    for n in range(len(waves)-1, number, -1):
        for change in sorted(waves[n]['source_changes'], key=lambda c:c['after_line'], reverse=True):
            pos = change['after_line']
            after = change['after'].splitlines(keepends=True)
            if lines[pos:pos+len(after)] != after:
                raise ValueError('Wave edit context changed')
            lines[pos:pos+len(after)] = change['before'].splitlines(keepends=True)
        if digest(''.join(lines).encode()) != waves[n-1]['source_sha256']:
            raise ValueError('Reversed wave hash mismatch')
    return ''.join(lines)


def replay(spec_path):
    spec = json.loads(spec_path.read_text())
    final_source = (ROOT/spec['source']).read_text()
    previous_exact = set()
    for wave in spec['waves']:
        number = wave['wave']
        source = ROOT/'work/macro_actor_replay'/f'wave{number:02}.c'
        source.parent.mkdir(parents=True,exist_ok=True)
        source.write_bytes(restore_wave(spec,number,final_source).encode())
        result = evaluate(source,spec_path,ROOT/'build/macro_actor_replay'/f'wave{number:02}.json')
        for key,value in result['summary'].items():
            if wave['summary'][key] != value:
                raise ValueError(f'Wave {number} summary changed: {key}')
        exact = {r['symbol'] for r in result['functions'] if r['strict_equal']}
        if not previous_exact.issubset(exact):
            raise ValueError('Previously exact experimental contribution regressed')
        previous_exact = exact
    print('All recorded waves reproduced by fresh whole-region compilation.')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path)
    parser.add_argument('--spec',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--replay',type=Path,help='Reconstruct and freshly compile every recorded wave')
    parser.add_argument('--object',type=Path,help='Analyze an existing object; never an acceptance path')
    args=parser.parse_args()
    if args.replay:
        if any((args.source,args.spec,args.output,args.object)):
            parser.error('--replay is standalone')
        replay(args.replay)
    else:
        if not all((args.source,args.spec,args.output)):
            parser.error('--source, --spec and --output are required')
        if not any(args.output.resolve().is_relative_to(ROOT/folder) for folder in ('build','work','candidates')):
            parser.error('Generated reports belong under build/, work/ or candidates/')
        evaluate(args.source,args.spec,args.output,args.object)
