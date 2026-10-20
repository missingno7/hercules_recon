"""Cross-module twin finder: function spans whose bytes agree after masking absolute (base-relocated)
fields and external rel32 call/jump displacements. Diagnostic lead only."""
import hashlib
import json
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
import env  # noqa: E402,F401
import capstone  # noqa: E402
from pe import PE  # noqa: E402

PATHS = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL', 'ENG1.DLL': 'assets/pc/HERCULES/ENG1.DLL',
         'ENG3.DLL': 'assets/pc/HERCULES/ENG3.DLL', 'HERCULES.EXE': 'work/discs/pc_install/HERCULES.EXE'}
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def masked(pe, relocs, rva, size):
    data = bytearray(pe.read_rva(rva, size))
    for off in range(size):
        if rva + off in relocs:
            data[off:off + 4] = b'\0\0\0\0'
    for ins in md.disasm(bytes(pe.read_rva(rva, size)), rva):
        if ins.mnemonic in ('call', 'jmp') and ins.size == 5 and ins.bytes[0] in (0xe8, 0xe9):
            target = int(ins.op_str, 16) if ins.op_str.startswith('0x') else None
            if target is None or not (rva <= target < rva + size):
                o = ins.address - rva + 1
                data[o:o + 4] = b'\0\0\0\0'
    return hashlib.sha256(bytes(data)).hexdigest()


(ROOT / 'work/twins').mkdir(parents=True, exist_ok=True)
state = json.loads((ROOT / 'recovery.json').read_text())
status = {}
for r in state['functions']:
    status[(Path(r['target']).name.upper(), int(r['rva'], 16))] = r['status'] + ('/masked' if r.get('proof', {}).get('scope', '').startswith('relocation-masked') else '')
groups = defaultdict(list)
for module, path in PATHS.items():
    pe = PE(ROOT / path)
    relocs = {e.rva for b in getattr(pe.pe, 'DIRECTORY_ENTRY_BASERELOC', []) for e in b.entries if e.type}
    for f in json.loads((ROOT / f'work/function_map/{module}.json').read_text())['functions']:
        rva, size = int(f['rva'], 16), f['size']
        if size < 32:
            continue
        groups[(size, masked(pe, relocs, rva, size))].append((module, rva, status.get((module, rva))))
multi = {k: v for k, v in groups.items() if len({m for m, _, _ in v}) > 1}
# leverage: groups where at least one member is recorded and another is not
lever = [(k, v) for k, v in multi.items() if any(s for _, _, s in v) and any(not s for _, _, s in v)]
unrec_bytes = sum(k[0] * sum(1 for _, _, s in v if not s) for k, v in lever)
print('cross-module twin groups:', len(multi), 'bytes in groups:', sum(k[0] * len(v) for k, v in multi.items()))
print('groups with a recorded member and an unrecorded member:', len(lever), 'unrecorded bytes:', unrec_bytes)
pairs = defaultdict(int)
for k, v in multi.items():
    mods = sorted({m for m, _, _ in v})
    pairs[tuple(mods)] += k[0]
print({'+'.join(p): b for p, b in pairs.items()})
json.dump([dict(size=k[0], members=[dict(module=m, rva=hex(r), status=s) for m, r, s in v]) for k, v in sorted(multi.items(), key=lambda kv: -kv[0][0])],
          open(ROOT / 'work/twins/twins.json', 'w'), indent=1)
for k, v in sorted(lever, key=lambda kv: -kv[0][0])[:40]:
    print(k[0], [(m, hex(r), s) for m, r, s in v])
