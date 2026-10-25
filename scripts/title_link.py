"""Diagnostic natural link of every canonical TITLE.DLL source (closure progress; never acceptance).

Compiles each source that owns a TITLE row (masked, reconstructed or exact) with the pinned
toolchain and its row flags, then links them as a DLL with LIBCMT and KERNEL32 only (no stubs,
no aliases, no placement). The report classifies what is missing:

  title functions   referenced first-party code with no reconstruction yet
  data              globals that are referenced but defined by no source (owner unknown)
  other             anything else (library or import gaps)

and lists duplicate definitions. Data-only units come from config/title/data_units.json.
"""
import argparse
import json
import os
import re
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

from env import ROOT
from coff import COFF
from match import DEFAULT_TOOLCHAIN, compile_source, digest, verify_toolchain

LIBRARIES = ['libcmt.lib', 'oldnames.lib', 'kernel32.lib']


def sources(module):
    state = json.loads((ROOT / 'recovery.json').read_text())
    grouped = {}
    for r in state['functions']:
        if Path(r['target']).name.upper() == module.upper() and r.get('source'):
            grouped.setdefault(r['source'], set()).add(tuple(r.get('flags') or ['/O2']))
    for source, flags in grouped.items():
        if len(flags) > 1:
            raise ValueError(f'{source} has rows with different flags: {flags}')
    # Link order hypothesis: code order (lowest owned RVA); data units just before `link_before`.
    key = {}
    for r in state['functions']:
        if Path(r['target']).name.upper() == module.upper() and r.get('source'):
            key[r['source']] = min(key.get(r['source'], 1 << 32), int(r['rva'], 16))
    out = {s: list(next(iter(f))) for s, f in grouped.items()}
    for unit in data_units():
        out.setdefault(unit['source'], ['/O2'])
        if unit['source'] not in key:
            key[unit['source']] = int(unit['link_before'], 16) - 0.5
    return {s: out[s] for s in sorted(out, key=lambda s: key[s])}


def data_units():
    path = ROOT / 'config/title/data_units.json'
    return json.loads(path.read_text())['units'] if path.exists() else []


def read_map(path):
    symbols = {}
    for line in path.read_text(errors='ignore').splitlines():
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m:
            symbols[m.group(1)] = int(m.group(2), 16)
    return symbols


def virtual_data(pe, rva, size):
    """Read one mapped section, reporting loader-zero bytes separately.

    This is a data-layout diagnostic, not file-backed or function equality.
    Never turn missing/truncated file bytes into zeros.
    """
    if size < 0:
        raise ValueError('negative data size')
    section = next((s for s in pe.sections
                    if s['virtual_address'] <= rva < s['virtual_address'] + s['virtual_size']), None)
    if section is None or rva + size > section['virtual_address'] + section['virtual_size']:
        raise ValueError('data range outside one mapped section')
    offset = rva - section['virtual_address']
    file_size = min(size, max(0, section['raw_size'] - offset))
    data = pe.read_rva(rva, file_size) if file_size else b''
    if len(data) != file_size:
        raise ValueError('truncated file-backed data')
    zero_size = size - file_size
    return data + bytes(zero_size), zero_size


def build(module):
    toolchain = verify_toolchain(DEFAULT_TOOLCHAIN)
    lane = Path(tempfile.mkdtemp(prefix='title-link-', dir=ROOT / 'work'))
    objects, definitions, rows = [], defaultdict(list), []
    for source, flags in sources(module).items():
        obj, _ = compile_source(ROOT / source, flags)
        objects.append(obj)
        for sym in COFF(obj).symbols.values():
            defined = (sym['storage'] == 2 or (sym['storage'] == 3 and '$S' in sym['name'])) and sym['section'] > 0
            communal = sym['storage'] == 2 and sym['section'] == 0 and sym['value'] > 0 and not sym['type'] & 0x20
            if defined or communal:
                definitions[sym['name']].append(source)
        rows.append(dict(source=source, flags=flags, source_sha256=digest((ROOT / source).read_bytes())))
    env = os.environ.copy()
    for key in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(key, None)
    tc = DEFAULT_TOOLCHAIN
    env.update(PATH=str(tc / 'bin') + os.pathsep + env.get('PATH', ''), LIB=str(tc / 'lib'))
    command = [str(tc / 'bin/link.exe'), '/nologo', '/DLL', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF',
               '/OUT:' + str(lane / 'diagnostic.dll'), '/MAP:' + str(lane / 'diagnostic.map'),
               *map(str, objects), *LIBRARIES]
    result = subprocess.run(command, cwd=lane, env=env, capture_output=True, text=True, timeout=120)
    log = result.stdout + result.stderr
    (lane / 'link.log').write_text(log)
    unresolved = sorted(set(re.findall(r'unresolved external symbol (\S+)', log)))
    duplicates = sorted(set(re.findall(r'LNK2005: (\S+) already defined', log)))
    classes = defaultdict(list)
    for name in unresolved:
        if re.match(r'_title_[0-9a-f]{5}$', name):
            classes['title_functions'].append(name)
        elif re.match(r'_g_', name):
            classes['data'].append(name)
        else:
            classes['other'].append(name)
    return dict(scope='Diagnostic natural link of canonical TITLE sources; no stubs, aliases or placement; not acceptance.',
                toolchain=toolchain, libraries=LIBRARIES, sources=rows, link_exit_code=result.returncode,
                unresolved_count=len(unresolved), unresolved=dict(classes), duplicates=duplicates,
                duplicate_source_definitions={k: v for k, v in definitions.items() if len(v) > 1},
                lane=str(lane.relative_to(ROOT)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('module', nargs='?', default='TITLE.DLL')
    args = parser.parse_args()
    report = build(args.module)
    out = ROOT / 'work/closure' / f'{args.module.lower()}_link.json'
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    u = report['unresolved']
    print(f"{len(report['sources'])} sources; link exit {report['link_exit_code']}; unresolved {report['unresolved_count']} "
          f"(title functions {len(u.get('title_functions', []))}, data {len(u.get('data', []))}, "
          f"other {len(u.get('other', []))}); duplicates {len(report['duplicates'])}; report: {out}")
    for name in u.get('other', [])[:40]:
        print('  other:', name)
    for name in report['duplicates'][:40]:
        print('  duplicate:', name)


if __name__ == '__main__':
    main()
