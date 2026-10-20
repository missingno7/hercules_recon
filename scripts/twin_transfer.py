"""Compile the recorded member's source of each twin group and masked-compare it at the unrecorded twins."""
import json
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from masked import masked_compare  # noqa: E402
from match import compile_source  # noqa: E402

PATHS = {'TITLE.DLL': 'assets/pc/HERCULES/TITLE.DLL', 'ENG1.DLL': 'assets/pc/HERCULES/ENG1.DLL',
         'ENG3.DLL': 'assets/pc/HERCULES/ENG3.DLL', 'HERCULES.EXE': 'work/discs/pc_install/HERCULES.EXE'}
state = json.loads((ROOT / 'recovery.json').read_text())
rows = {(Path(r['target']).name.upper(), int(r['rva'], 16)): r for r in state['functions']}
groups = json.loads((ROOT / 'work/twins/twins.json').read_text())
jobs = defaultdict(list)
for g in groups:
    rec = [m for m in g['members'] if m['status'] and ('masked' in m['status'] or 'FUNCTION_MATCH' in m['status'])]
    unrec = [m for m in g['members'] if not m['status']]
    if rec and unrec:
        r = rows[(rec[0]['module'], int(rec[0]['rva'], 16))]
        for u in unrec:
            jobs[(r['source'], tuple(r['flags']))].append((r, u, g['size']))
results = []
for (source, flags), items in jobs.items():
    obj, _ = compile_source(ROOT / source, list(flags))
    for r, u, size in items:
        m = masked_compare(obj, r['symbol'], ROOT / PATHS[u['module']], int(u['rva'], 16), size)
        results.append(dict(module=u['module'], rva=u['rva'], size=size, from_module=Path(r['target']).name, from_rva=r['rva'],
                            symbol=r['symbol'], source=source, masked_equal=m['masked_equal'],
                            differing=m['differing_bytes'], conflicts=m['conflicts'][:3], symbols=m['symbols']))
json.dump(results, open(ROOT / 'work/twins/twin_transfer.json', 'w'), indent=1)
ok = [x for x in results if x['masked_equal']]
print('twin targets', len(results), 'masked equal', len(ok), sum(x['size'] for x in ok))
for x in results:
    print('EQ ' if x['masked_equal'] else f"{x['differing']:3d}", x['module'], x['rva'], x['size'], '<-', x['from_module'], x['from_rva'],
          x['source'], '' if x['masked_equal'] else x['conflicts'][:1])
