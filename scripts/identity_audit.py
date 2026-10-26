"""Audit: are callee identities used by accepted proofs unique masked shapes in their module?

Compiler-free. For each kind=function record in an accepted row's pinned binding document,
mask every 4-byte immediate/displacement and rel32 field of the callee's native span and look
for the same masked byte pattern at any 16-byte-aligned or function-map start elsewhere in the
module's executable sections. Diagnostic only.
"""
import collections, glob, json, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import env  # noqa
from pe import PE
import capstone

ledger = {r['id']: r for r in json.loads((ROOT / 'recovery.json').read_bytes())['functions']}
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True
images, code_cache = {}, {}


def mask(code, base):
    out = bytearray(code)
    for i in md.disasm(code, base):
        for off, size in ((i.imm_offset, i.imm_size), (i.disp_offset, i.disp_size)):
            if size == 4:
                p = i.address - base + off
                out[p:p + 4] = b'\0\0\0\0'
    return bytes(out)


def sections(target):
    pe = images.setdefault(target, PE(ROOT / target))
    if target not in code_cache:
        code_cache[target] = [(s['virtual_address'], pe.read_rva(s['virtual_address'], min(s['virtual_size'], s['raw_size'])))
                              for s in pe.sections if s['characteristics'] & 0x20000000]
    return pe, code_cache[target]


starts = collections.defaultdict(set)
for f in glob.glob(str(ROOT / 'work/realignment/census/function_map_*.json')):
    try:
        m = json.load(open(f))
    except Exception:
        continue
    for fn in (m.get('functions') if isinstance(m, dict) else m) or []:
        if isinstance(fn, dict) and 'rva' in fn:
            starts[Path(f).stem.replace('function_map_', '').upper()].add(int(fn['rva'], 0) if isinstance(fn['rva'], str) else fn['rva'])

records = {}
for row in ledger.values():
    path = row.get('proof', {}).get('bindings_path')
    if row['status'] != 'FUNCTION_MATCH_RELOCATED' or not path or not (ROOT / path).exists():
        continue
    binding = json.loads((ROOT / path).read_text(encoding='utf-8'))['functions'].get(row['id'], {})
    for name, rec in binding.get('symbols', {}).items():
        if isinstance(rec, dict) and rec.get('kind') == 'function' and rec['rva'] != int(row['rva'], 0):
            records.setdefault((row['target'], rec['rva'], rec['size'], name), []).append(row['id'])

result = collections.Counter()
flagged = []
for (target, rva, size, name), users in sorted(records.items()):
    pe, secs = sections(target)
    try:
        own = mask(pe.read_rva(rva, size), rva)
    except Exception:
        result['unreadable'] += 1
        continue
    module = Path(target).name.upper()
    others = []
    for base, data in secs:
        cands = {a for a in range(base, base + len(data) - size + 1, 16)} | {s for s in starts.get(module.replace('.', '.'), ()) if base <= s <= base + len(data) - size}
        for a in cands:
            if a == rva:
                continue
            chunk = data[a - base:a - base + size]
            if len(chunk) == size and chunk[:1] == own[:1] and mask(chunk, a) == own:
                others.append(a)
    if others:
        result['duplicate_shape'] += 1
        flagged.append(dict(target=target, rva=hex(rva), size=size, symbol=name, duplicates=[hex(x) for x in sorted(others)],
                            accepted_users=sorted(set(users))))
    else:
        result['unique_shape'] += 1
out = ROOT / 'work/identity_audit.json'
out.write_text(json.dumps(dict(counts=result, flagged=flagged), indent=1) + '\n', encoding='utf-8')
print(dict(result), 'identity records;', sum(len(f['accepted_users']) for f in flagged), 'accepted-row uses of duplicate shapes')
for f in flagged[:25]:
    print(' ', f['target'][-8:], f['rva'], f['size'], f['symbol'], 'dups', f['duplicates'][:4], 'users', len(f['accepted_users']))
