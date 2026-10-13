from __future__ import annotations
import argparse, hashlib, json, os, sys
from pathlib import Path

EXPECTED_EXE = '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
BASE = 0x400000
TARGET = 0x43A590

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def main() -> None:
    parser = argparse.ArgumentParser(description='Read-only full .text candidate audit for the pinned original dirty-byte references')
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--repo-root', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    exe, root, output = args.exe.resolve(), args.repo_root.resolve(), args.output.resolve()
    if sha(exe.read_bytes()) != EXPECTED_EXE:
        parser.error('original EXE hash does not match the pinned image')
    try:
        output.relative_to(root / 'work')
    except ValueError:
        parser.error('--output must be under <repo-root>/work')
    if output == exe or output.parent == exe.parent or exe.parent in output.parents:
        parser.error('output must be outside the original image directory')
    ownership_path = root / 'renderer_contract' / 'evidence' / 'state_ownership.json'
    expected_ownership = 'ecd1e0eaddcf35ceb5f8beeb5c798ffd9b54d29ae1fd0ae2760a3de344ee4508'
    if sha(ownership_path.read_bytes()) != expected_ownership:
        parser.error('state_ownership.json does not match the pinned evidence version')
    os.environ['HERCULES_PYTHON_DEPS'] = r'C:\tools\hercules\python'
    sys.path[:0] = [str(root / 'scripts'), r'C:\tools\hercules\python']
    import capstone
    from pe import PE
    pe = PE(exe)
    section = next(row for row in pe.sections if row['name'] == '.text')
    start = section['virtual_address']
    raw = pe.read_rva(start, section['raw_size'])
    strict = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    strict.detail = True
    strict_rows = list(strict.disasm(raw, BASE + start))
    strict_bytes = sum(row.size for row in strict_rows)
    strict_stop = start + strict_bytes
    full = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    full.detail = True
    full.skipdata = True
    decoded = list(full.disasm(raw, BASE + start))
    pseudo = [row for row in decoded if row.mnemonic == '.byte']
    hits = []
    service_targets = {0x6FF0: 'lock_6ff0', 0x7140: 'unlock_7140', 0x6620: 'present_6620'}
    service_callers = {name: [] for name in service_targets.values()}
    for insn in decoded:
        if insn.mnemonic == '.byte':
            continue
        if insn.mnemonic == 'call' and insn.operands and insn.operands[0].type == capstone.x86.X86_OP_IMM:
            target_rva = insn.operands[0].imm - BASE
            if target_rva in service_targets:
                service_callers[service_targets[target_rva]].append(hex(insn.address - BASE))
        for operand in insn.operands:
            if operand.type != capstone.x86.X86_OP_MEM or operand.mem.base or operand.mem.index:
                continue
            address = operand.mem.disp & 0xffffffff
            if address != TARGET:
                continue
            access = ''.join(label for flag, label in ((capstone.CS_AC_READ,'read'),(capstone.CS_AC_WRITE,'write')) if operand.access & flag)
            hits.append({'site_rva': hex(insn.address-BASE), 'address': hex(address), 'width': operand.size,
                         'access': access or 'unknown', 'instruction': f'{insn.mnemonic} {insn.op_str}'})
    ownership = json.loads(ownership_path.read_text(encoding='utf-8'))
    candidates = []
    for hit in hits:
        rva = int(hit['site_rva'],16)
        intervals = []
        for interval in ownership['bounded_dependency_intervals']:
            if interval['module'] != 'HERCULES.EXE':
                continue
            lo, size = int(interval['rva'],16), int(interval['size'])
            if lo <= rva < lo + size:
                observed = sha(pe.read_rva(lo,size))
                if observed != interval['sha256']:
                    raise RuntimeError(f"span hash mismatch: {interval['name']}")
                intervals.append({'name': interval['name'], 'classification': interval['classification'],
                                  'rva': interval['rva'], 'size': size, 'sha256': observed})
        candidates.append({**hit, 'bounded_intervals': intervals,
                           'scope': 'inside_reviewed_span' if intervals else 'outside_reviewed_spans'})
    if len(hits) != 17:
        raise RuntimeError(f'expected 17 complete .text absolute-memory candidates, found {len(hits)}')
    if sum(bool(row['bounded_intervals']) for row in candidates) != 14:
        raise RuntimeError('expected 14 candidates inside hash-verified renderer spans')
    expected_callers = {
        'lock_6ff0': ['0x55e8', '0x6d0e', '0x6d59', '0x6d79', '0x7415', '0xac92'],
        'unlock_7140': ['0x5672', '0x69ab', '0x6bf6', '0x6d44', '0x6d5e', '0x6dc7', '0xb2d8'],
        'present_6620': ['0x5677', '0x6764', '0x69be', '0x6d6a', '0x6e9f', '0x9772', '0xb2dd'],
    }
    if service_callers != expected_callers:
        raise RuntimeError(f'full .text service call candidate mismatch: {service_callers!r}')
    gaps=[]
    cursor=start
    for insn in decoded:
        if insn.mnemonic == '.byte':
            rva=insn.address-BASE
            if gaps and gaps[-1]['end_rva_exclusive']==hex(rva):
                gaps[-1]['end_rva_exclusive']=hex(rva+insn.size); gaps[-1]['size']+=insn.size
            else:
                gaps.append({'start_rva':hex(rva),'end_rva_exclusive':hex(rva+insn.size),'size':insn.size})
        cursor=insn.address-BASE+insn.size
    receipt={
      'scope':'Read-only full .text linear-sweep candidates; this is not execution/reachability proof. Original executable is read-only.',
      'oracle':{'path':str(exe),'sha256':sha(pe.data),'image_base':hex(pe.image_base)},
      'tool':{'capstone_version':capstone.__version__,'section_name':'.text','section_rva':hex(start),'file_backed_bytes':len(raw)},
      'coverage':{'strict_first_stop_rva':hex(strict_stop),'strict_decoded_instructions':len(strict_rows),'strict_decoded_bytes':strict_bytes,
                  'full_skipdata_valid_instructions':len(decoded)-len(pseudo),'pseudoops':len(pseudo),'skipped_bytes':sum(row.size for row in pseudo),
                  'byte_coverage_complete':cursor==start+len(raw),'gaps':gaps},
      'target':hex(TARGET),'candidate_count':len(candidates),'inside_hash_verified_renderer_spans':sum(bool(row['bounded_intervals']) for row in candidates),
      'outside_reviewed_spans':sum(not row['bounded_intervals'] for row in candidates),'candidates':candidates,
      'surface_service_call_candidates':service_callers,
      'surface_service_call_scope':'Full .text skipdata linear-sweep candidate lists; all listed calls also lie before strict stop 0x1124a. Candidate decode is not reachability proof.',
      'state_ownership_sha256':sha(ownership_path.read_bytes()),
      'limitations':['Linear sweep can decode embedded data as instructions. Computed and indirect aliases are not covered.',
                     'The strict decoder is prefix-only; do not use it as a complete caller inventory.']
    }
    if not receipt['coverage']['byte_coverage_complete']:
        raise RuntimeError('full skipdata sweep did not cover all file-backed .text bytes')
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'output':str(output),'strict_first_stop_rva':hex(strict_stop),
                      'full_valid_instructions':len(decoded)-len(pseudo),'pseudoops':len(pseudo),
                      'candidates':len(candidates),'inside':receipt['inside_hash_verified_renderer_spans'],
                      'outside':receipt['outside_reviewed_spans']},indent=2))

if __name__=='__main__': main()
