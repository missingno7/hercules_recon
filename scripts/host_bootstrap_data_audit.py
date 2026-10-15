"""Fresh bounded PC bootstrap text/movie data audit; no game execution or layout proof."""
import argparse, hashlib, json, struct, sys
from pathlib import Path

ROOT = next(p for p in Path(__file__).resolve().parents if (p/'scripts/match.py').is_file())
sys.path[:0] = [str(ROOT/'scripts'), 'C:/tools/hercules/python']
from match import compile_source, compare, DEFAULT_TOOLCHAIN, verify_toolchain
from masked import masked_compare
from coff import COFF
from pe import PE
from title_behavioral import context_files

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--original-root', type=Path, default=Path('D:/Prog/hercules_recon'))
args = parser.parse_args()
OUT = args.output.resolve()
assert OUT.is_relative_to(ROOT/'work')
OUT.mkdir(parents=True, exist_ok=True)
ORACLE = args.original_root/'work/discs/pc_install/HERCULES.EXE'
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert sha(ORACLE) == '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
tool = verify_toolchain(DEFAULT_TOOLCHAIN)
sources = ['calibration/host_bootstrap_text.c', 'calibration/movie_initializer_storage.c',
           'src/pc/host_context_bootstrap.c', 'src/pc/host_options_joystick.cpp']
watched = {Path(__file__), ORACLE, ROOT/'recovery.json', ROOT/'toolchains/manifest.json'}
for source in sources:
    watched.update(context_files(ROOT/source))
for name in ['match.py', 'coff.py', 'pe.py', 'masked.py', 'env.py', 'title_behavioral.py']:
    watched.add(ROOT/'scripts'/name)
pins = {str(p): sha(p) for p in watched}
recovery = json.loads((ROOT/'recovery.json').read_text())['functions']
compiles, functions, objects = [], [], {}
for source in sources:
    rows = [r for r in recovery if r['source'] == source]
    flags = rows[0]['flags'] if rows else ['/O2']
    assert all(r['flags'] == flags for r in rows)
    obj, cmd = compile_source(ROOT/source, flags)
    objects[source] = COFF(obj)
    compiles.append(dict(source=source, source_sha256=sha(ROOT/source), object=str(obj),
                         object_sha256=sha(obj), command=cmd))
    for r in rows:
        raw = compare(obj, r['symbol'], ORACLE, int(r['rva'], 0), r['size'])
        diag = masked_compare(obj, r['symbol'], ORACLE, int(r['rva'], 0), r['size'])
        assert raw['compiled_size'] == r['size']
        assert '_g_resource_filename' not in diag['symbols']
        if int(r['rva'], 0) in [0x1ff0, 0x83f0]:
            assert diag['symbols']['_g_host_empty_text_3f74c'] == '0x3f74c'
        assert diag['differing_bytes'] == (8 if int(r['rva'], 0) == 0x83f0 else 0)
        assert not diag['conflicts']
        functions.append(dict(id=r['id'], status_unchanged=r['status'], raw=raw, diagnostic=diag))
assert len(functions) == 7

native = PE(ORACLE)
assert native.pe.OPTIONAL_HEADER.DATA_DIRECTORY[5].VirtualAddress == 0
data = []
specs = {
    'calibration/host_bootstrap_text.c': [('_g_host_empty_text_3f74c', 0x3f74c, 1),
        ('_g_host_default_engine_name_35160', 0x35160, 20),
        ('_g_host_default_engine_name_cell_35100', 0x35100, 4)],
    'calibration/movie_initializer_storage.c': [('_g_exe_4a3810', 0xa3810, 4),
        ('_g_exe_4a3814', 0xa3814, 4), ('_g_exe_48f810', 0x8f810, 81920),
        ('_g_exe_47f750', 0x7f750, 188)]}
for source, entries in specs.items():
    co = objects[source]
    definitions = [s for s in co.symbols.values() if s['storage'] == 2 and
                   (s['section'] > 0 or s['section'] == 0 and s['value'] > 0)]
    assert {s['name'] for s in definitions} == {e[0] for e in entries}
    assert not any(s['flags'] & 0x20 and s['size'] for s in co.sections)
    for name, rva, width in entries:
        s = next(s for s in definitions if s['name'] == name)
        if s['section'] == 0:
            assert s['value'] == width
            raw, rels = bytes(width), []
        else:
            sec = co.sections[s['section']-1]
            assert s['value'] + width <= sec['size']
            raw = sec['data'][s['value']:s['value']+width] if sec['data'] else bytes(width)
            rels = [dict(x, offset=x['offset']-s['value']) for x in sec['relocations']
                    if s['value'] <= x['offset'] < s['value']+width]
        assert len(raw) == width
        ns = next(s for s in native.pe.sections if s.VirtualAddress <= rva and
                  rva + width <= s.VirtualAddress + s.Misc_VirtualSize)
        virtual_zero = rva >= ns.VirtualAddress + ns.SizeOfRawData
        expected = bytes(width) if virtual_zero else native.read_rva(rva, width)
        if name == '_g_host_default_engine_name_cell_35100':
            assert not virtual_zero and raw == bytes(4) and rels == [dict(
                offset=0, type=6, symbol='_g_host_default_engine_name_35160')]
            assert struct.unpack('<I', expected)[0] == native.image_base + 0x35160
            equality = 'Symbolic DIR32 pointer target corresponds to native cell value; not raw equality'
        else:
            assert not rels and raw == expected
            equality = 'Complete observed source prefix equals native initial bytes'
        data.append(dict(symbol=name, rva=hex(rva), bytes=width, source=source,
                         source_prefix_sha256=hashlib.sha256(raw).hexdigest(),
                         native_backing='virtual_zero' if virtual_zero else 'file_backed',
                         pointer_relocations=rels, proof=equality))
assert native.read_rva(0x35160, 20) == b'\\TITLE\\ENGINE\\TITLE\0'
assert pins == {str(p): sha(p) for p in watched}
report = dict(scope=__doc__, inputs=pins, toolchain=tool, compiles=compiles, functions=functions,
              data=data, measured_prefix_bytes=sum(x['bytes'] for x in data),
              native_base_relocations_available=False, function_states_changed=0,
              limits=['The shared empty text has a one-byte observed read view; its full original extent and TU are unknown.',
                      'The filename buffer at RVA215c10 and MFC afxChNil at RVA7a2154 are different objects.',
                      'Movie clear/write spans are measured prefixes, not full historical declarations or layout.',
                      'No native address casts, aliases, source placement controls or whole-table initializer.',
                      'No startup, window, gameplay, exception or original-process execution.'])
dest = OUT/'host_bootstrap_data_audit.json'
dest.write_bytes((json.dumps(report, indent=2)+'\n').encode())
print(json.dumps(dict(data_prefixes=len(data), bytes=report['measured_prefix_bytes'],
                      functions_preserved=len(functions), report=str(dest))))
