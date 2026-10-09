"""Diagnostic natural link of every canonical TITLE.DLL source (closure progress; never acceptance).

Compiles each source that owns a TITLE row (masked, reconstructed or exact) with the pinned
toolchain and its row flags, then links them as a DLL with LIBCMT and KERNEL32 only (no stubs,
no aliases, no placement). The report classifies what is missing:

  title functions   referenced first-party code with no reconstruction yet
  data              globals that are referenced but defined by no source (owner unknown)
  other             anything else (library or import gaps)

and lists duplicate definitions. Data-only units come from calibration/title/data_units.json.

Layout (diagnostic): when symbols are unresolved, a second link with /FORCE:UNRESOLVED produces an
image used only to read where things landed: section sizes, natural function RVAs against the
original, and for each data unit its linked bytes against the original object (pointer slots
compared by relative target). The forced image is never executed, compared as a whole or accepted.
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
from pe import PE

LIBRARIES = ['libcmt.lib', 'kernel32.lib']


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
        key[unit['source']] = int(unit['link_before'], 16) - 0.5
    return {s: out[s] for s in sorted(out, key=lambda s: key[s])}


def data_units():
    path = ROOT / 'calibration/title/data_units.json'
    return json.loads(path.read_text())['units'] if path.exists() else []


def read_map(path):
    symbols = {}
    for line in path.read_text(errors='ignore').splitlines():
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if m:
            symbols[m.group(1)] = int(m.group(2), 16)
    return symbols


def compare_object(ours, o_base, orig, lo, hi):
    """Bytes of [lo, hi) in the original against the same span at o_base in ours; pointers by relative target."""
    size = hi - lo
    a, b = orig.read_rva(lo, size), ours.read_rva(o_base, size)
    rel = lambda pe, base: {e.rva - base for blk in pe.pe.DIRECTORY_ENTRY_BASERELOC for e in blk.entries
                            if e.type and 0 <= e.rva - base < size}
    ra, rb = rel(orig, lo), rel(ours, o_base)
    slots_equal = sum(1 for off in ra & rb
                      if int.from_bytes(a[off:off + 4], 'little') - orig.image_base - lo
                      == int.from_bytes(b[off:off + 4], 'little') - ours.image_base - o_base)
    covered = {i for r in ra | rb for i in range(r, r + 4)}
    plain = [i for i in range(size) if i not in covered]
    return dict(size=size, pointer_slots=len(ra), pointer_slots_equal=slots_equal, pointer_slots_extra=len(rb - ra),
                plain_bytes=len(plain), plain_bytes_differing=sum(a[i] != b[i] for i in plain))


def layout(module, lane, objects, env, tc, definitions):
    command = [str(tc / 'bin/link.exe'), '/nologo', '/DLL', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:NOREF',
               '/FORCE:UNRESOLVED', '/OUT:' + str(lane / 'layout.dll'), '/MAP:' + str(lane / 'layout.map'),
               *map(str, objects), *LIBRARIES]
    subprocess.run(command, cwd=lane, env=env, capture_output=True, text=True, timeout=120)
    if not (lane / 'layout.dll').exists():
        return dict(error='forced layout link produced no image')
    ours, symbols = PE(lane / 'layout.dll'), read_map(lane / 'layout.map')
    # File statics are absent from the map: recover each from a placed function's relocation to it
    # (linked dword minus the object's addend).
    for obj in objects:
        coff = COFF(obj)
        for sym in coff.symbols.values():
            if not (sym['type'] & 0x20 and sym['section'] > 0 and sym['name'] in symbols):
                continue
            fn = coff.function(sym['name'])
            for r in fn['relocations']:
                if r['type'] == 0x06 and '$S' in r['symbol'] and '$SG' not in r['symbol'] and r['symbol'] not in symbols:
                    addend = int.from_bytes(fn['data'][r['offset']:r['offset'] + 4], 'little')
                    linked = int.from_bytes(ours.read_rva(symbols[sym['name']] - ours.image_base + r['offset'], 4), 'little')
                    symbols[r['symbol']] = linked - addend
    state = json.loads((ROOT / 'recovery.json').read_text())
    rows = [r for r in state['functions'] if Path(r['target']).name.upper() == module.upper()]
    orig = PE(ROOT / rows[0]['target'])
    deltas = defaultdict(int)
    for r in rows:
        if r['symbol'] in symbols:
            deltas[symbols[r['symbol']] - ours.image_base - int(r['rva'], 16)] += 1
    sections = {s['name']: dict(ours=[hex(s['virtual_address']), s['virtual_size']]) for s in ours.sections}
    for s in orig.sections:
        sections.setdefault(s['name'], {})['original'] = [hex(s['virtual_address']), s['virtual_size']]
    units = []
    for unit in data_units():
        lo, hi = (int(x, 16) for x in unit['original'])
        base = symbols[unit['first_symbol']] - ours.image_base
        data = next(s for s in ours.sections if s['virtual_address'] <= base < s['virtual_address'] + s['virtual_size'])
        o_data = next(s for s in orig.sections if s['virtual_address'] <= lo < s['virtual_address'] + s['virtual_size'])
        units.append(dict(source=unit['source'], ours=hex(base), original=hex(lo),
                          section_offset_ours=hex(base - data['virtual_address']),
                          section_offset_original=hex(lo - o_data['virtual_address']),
                          bytes=compare_object(ours, base, orig, lo, hi)))
    # Data symbols defined by our sources with an implied original address: per defining object,
    # do they keep their original offsets relative to each other (one common delta)?
    implied = {}
    for r in rows:
        for name, address in (r.get('link_symbols') or {}).items():
            implied[name] = int(address, 16)
    function_symbols = {r['symbol'] for r in rows}
    objects_data = defaultdict(list)
    for name, owners in definitions.items():
        if name in implied and name in symbols and name not in function_symbols and len(owners) == 1:
            region = 'data' if implied[name] < 0x290e8 else 'bss' if implied[name] < 0x2b370 else 'communal'
            objects_data[(owners[0], region)].append((name, symbols[name] - ours.image_base - implied[name]))
    data_objects = []
    for (source, region), items in sorted(objects_data.items()):
        counts = defaultdict(int)
        for _, d in items:
            counts[d] += 1
        modal, n = max(counts.items(), key=lambda kv: kv[1])
        data_objects.append(dict(source=source, region=region, symbols=len(items), common_delta=hex(modal) if modal >= 0 else '-' + hex(-modal),
                                 at_common_delta=n, off=[(s, hex(d - modal)) for s, d in items if d != modal][:8]))
    top = sorted(deltas.items(), key=lambda kv: -kv[1])[:8]
    return dict(sections=sections, function_rows_placed=sum(deltas.values()),
                function_rows_at_original_rva=deltas.get(0, 0),
                function_rva_deltas_most_common=[(hex(d) if d >= 0 else '-' + hex(-d), n) for d, n in top],
                data_units=units, data_objects=data_objects)


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
    placed = layout(module, lane, objects, env, tc, definitions)
    return dict(scope='Diagnostic natural link of canonical TITLE sources; no stubs, aliases or placement; not acceptance.',
                toolchain=toolchain, libraries=LIBRARIES, sources=rows, link_exit_code=result.returncode, layout=placed,
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
    lay = report['layout']
    print(f"layout: {lay.get('function_rows_placed')} function rows placed, {lay.get('function_rows_at_original_rva')} at the "
          f"original RVA; most common deltas {lay.get('function_rva_deltas_most_common', [])[:4]}")
    for unit in lay.get('data_units', []):
        print(f"  data unit {unit['source']}: section offset ours {unit['section_offset_ours']} vs original "
              f"{unit['section_offset_original']}; bytes {unit['bytes']}")
    for obj in lay.get('data_objects', []):
        print(f"  {obj['region']} in {obj['source']}: {obj['at_common_delta']}/{obj['symbols']} symbols keep their relative offsets"
              + (f"; off: {obj['off']}" if obj['off'] else ''))
    for name in u.get('other', [])[:40]:
        print('  other:', name)
    for name in report['duplicates'][:40]:
        print('  duplicate:', name)


if __name__ == '__main__':
    main()
