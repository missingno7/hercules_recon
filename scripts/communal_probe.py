"""Diagnostic: how pinned VC5 LINK orders communal (uninitialized non-static C) globals.

Synthetic objects only; no game code is compiled or linked. Writes into an ignored work/
directory and prints the resulting data layout for two link orders.
"""
import os
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RTM = Path(r'C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953')
SOURCES = {
    'a.c': 'int zeta; char mid_buf[64];\nint alpha;\nstatic int a_static;\nint fa(void) { return zeta + alpha + mid_buf[1] + a_static; }\n',
    'b.c': 'extern int zeta;\nint beta[8]; int gamma;\nint delta = 5;\nint fb(void) { return zeta + beta[2] + gamma + delta; }\n',
    'c.c': 'int aaa_first; int alpha;\nint epsilon[100];\nint fc(void) { return aaa_first + alpha + epsilon[3]; }\n'
           'int __stdcall DllMain(void *h, unsigned long r, void *p) { return 1; }\n',
    'd.cpp': 'extern "C" { int cpp_one; int cpp_two[16]; static int cpp_static;\n'
             'int fd(void) { return cpp_one + cpp_two[1] + cpp_static; } }\n',
}
work = Path(tempfile.mkdtemp(prefix='communal-', dir=ROOT / 'work'))
env = dict(os.environ, PATH=str(RTM / 'bin') + os.pathsep + os.environ['PATH'], INCLUDE=str(RTM / 'include'),
           LIB=str(RTM / 'lib'))
for name, text in SOURCES.items():
    (work / name).write_text(text)
    subprocess.run([str(RTM / 'bin/cl.exe'), '/nologo', '/c', '/O2', '/Gy', name], cwd=work, env=env, check=True,
                   capture_output=True)
    # COFF storage class of each data symbol: communal (external, section 0, value = size) vs defined.
    import sys
    sys.path.insert(0, str(ROOT / 'scripts'))
    from coff import COFF
    obj = COFF(work / (Path(name).stem + '.obj'))
    kinds = {s['name']: ('communal' if s['section'] == 0 and s['value'] else 'defined' if s['section'] > 0 else 'extern')
             for s in obj.symbols.values() if s['storage'] in (2, 3) and not s['type'] & 0x20 and s['name'].startswith('_')}
    print(name, {k: v for k, v in kinds.items() if not k.startswith('_f')})
for order in (['a', 'b', 'c'], ['c', 'b', 'a'], ['a', 'd', 'b', 'c'], ['d', 'a', 'b', 'c']):
    out = f'probe_{"".join(order)}'
    r = subprocess.run([str(RTM / 'bin/link.exe'), '/nologo', '/DLL', '/NOENTRY', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF',
                        f'/OUT:{out}.dll', f'/MAP:{out}.map'] + [f'{o}.obj' for o in order],
                       cwd=work, env=env, capture_output=True, text=True)
    if r.returncode:
        print(order, 'link failed', r.stdout[-300:])
        continue
    rows = []
    for line in (work / f'{out}.map').read_text(errors='ignore').splitlines():
        m = re.match(r'\s*(000[0-9]):([0-9a-f]{8})\s+(_\w+)\s', line)
        if m and not m.group(3).startswith('_f') and 'DllMain' not in m.group(3):
            rows.append((m.group(1), int(m.group(2), 16), m.group(3)))
    print('link order', order, '->', [(sec, n, hex(a)) for sec, a, n in sorted(rows)])
print('work dir', work)
