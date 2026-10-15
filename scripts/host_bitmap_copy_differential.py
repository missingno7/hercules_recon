"""Compare the original bitmap copy with freshly compiled VC5 source.

Executes the identified, unmodified CRT __ftol in both images. No OS calls
or game process run. Finite semantic agreement does not grant FUNCTION_MATCH.
"""
import argparse, json, hashlib, os, re, struct, subprocess, sys
from fractions import Fraction
from pathlib import Path
ROOT = next((p for p in Path(__file__).resolve().parents if (p / 'scripts/match.py').is_file()))
sys.path[:0] = [str(ROOT / 'scripts'), 'C:/tools/hercules/python', 'C:/tools/hercules/renderer-oracle-python']
from match import compile_source, compare, verify_toolchain, DEFAULT_TOOLCHAIN
from masked import masked_compare
from coff import COFF
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=ROOT / 'work/host_bitmap_copy_public')
args = parser.parse_args()
OUT = args.output.resolve()
assert OUT.is_relative_to((ROOT / 'work').resolve())
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = ROOT / 'src/pc/host_bitmap_copy.cpp'
ORACLE = Path('D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE')
LIB = DEFAULT_TOOLCHAIN / 'lib/libcmt.lib'
MAP = Path('D:/Prog/hercules_recon/work/library_map/library_map.json')
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
pack = lambda v: struct.pack('<I', v & 4294967295)
ARENA = 1627389952
ARENA_SIZE = 393216
SRC = 65568
DST = 196640
STACK = 1644167168
ESP = STACK + 32768
STOP = STACK + 61440
NV = [UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP]
SEEDS = [287454020, 573785173, 860116326, 1146447479]

def build():
    assert sha(ORACLE) == '587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d'
    assert sha(LIB) == '7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b'
    verify_toolchain(DEFAULT_TOOLCHAIN)
    imports = [DEFAULT_TOOLCHAIN / 'lib/kernel32.lib', DEFAULT_TOOLCHAIN / 'lib/user32.lib']
    watched = [SOURCE, ORACLE, LIB, MAP, *imports, Path(__file__), *[ROOT / p for p in ['scripts/coff.py', 'scripts/match.py', 'scripts/masked.py', 'scripts/pe.py', 'scripts/env.py', 'toolchains/manifest.json']]]
    pins = {str(p): sha(p) for p in watched}
    data = LIB.read_bytes()
    position = 8
    index = 0
    member = None
    while position < len(data):
        header = data[position:position + 60]
        assert header[58:] == b'`\n'
        size = int(header[48:58])
        payload = data[position + 60:position + 60 + size]
        if index == 602:
            member = payload
            break
        position += 60 + size + (size & 1)
        index += 1
    assert member is not None
    memberpath = OUT / 'ftol602.obj'
    memberpath.write_bytes(member)
    co = COFF(memberpath)
    sections = [s for s in co.sections if s['flags'] & 32 and s['size']]
    assert len(sections) == 1
    code = sections[0]
    assert code['size'] == 39 and (not code['relocations'])
    pe = pefile.PE(str(ORACLE))
    assert code['data'] == pe.get_data(69008, 39)
    row = next((x for x in json.loads(MAP.read_bytes())['contributions']['HERCULES.EXE'] if x['rva'] == '0x10d90'))
    assert row['member'] == 602 and row['size'] == 39 and ('__ftol' in row['names'])
    fixture = OUT / 'fixture.c'
    fixture.write_bytes(b'extern void __cdecl graphics_native_06530(unsigned char*,int,int,int,unsigned char*,int,int,int);\nint __cdecl main(void) { for (;;) {} }\n__declspec(dllexport) void __cdecl copy_fixture_anchor(void) { graphics_native_06530(0,0,0,0,0,0,0,0); }\n')
    compiles = []
    objects = []
    for source in [SOURCE, fixture]:
        obj, cmd = compile_source(source, ['/O2'])
        objects.append(obj)
        compiles.append(dict(source=str(source), source_sha256=sha(source), object=str(obj), object_sha256=sha(obj), command=cmd))
    dll = OUT / 'fixture.dll'
    mp = OUT / 'fixture.map'
    env = os.environ.copy()
    for key in ['CL', '_CL_', 'LINK', '_LINK_']:
        env.pop(key, None)
    env['PATH'] = str(DEFAULT_TOOLCHAIN / 'bin') + os.pathsep + env.get('PATH', '')
    cmd = [str(DEFAULT_TOOLCHAIN / 'bin/link.exe'), '/nologo', '/DLL', '/NOENTRY', '/NODEFAULTLIB', '/INCREMENTAL:NO', '/OPT:REF', '/OPT:NOICF', '/OUT:' + str(dll), '/MAP:' + str(mp), *map(str, objects), str(LIB), *map(str, imports)]
    p = subprocess.run(cmd, cwd=OUT, env=env, capture_output=True, text=True)
    (OUT / 'link.log').write_bytes((p.stdout + p.stderr).encode())
    assert p.returncode == 0, (p.stdout, p.stderr)
    pattern = re.compile('^\\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\\s+(\\S+)\\s+([0-9a-fA-F]{8})')
    symbols = {m.group(1): int(m.group(2), 16) for line in mp.read_text(errors='replace').splitlines() if (m := pattern.match(line))}
    linked = pefile.PE(str(dll))
    assert linked.get_data(symbols['__ftol'] - linked.OPTIONAL_HEADER.ImageBase, 39) == code['data']
    raw = compare(objects[0], '_graphics_native_06530', ORACLE, 25904, 240)
    diag = masked_compare(objects[0], '_graphics_native_06530', ORACLE, 25904, 240)
    assert pins == {p: sha(Path(p)) for p in pins}
    return dict(inputs=pins, compiles=compiles, link_command=cmd, dll=str(dll), dll_sha256=sha(dll), map_sha256=sha(mp), symbols=symbols, raw=raw, diagnostic=diag, ftol=dict(member=602, member_sha256=sha(memberpath), bytes=39, raw_original_equal=True, raw_linked_equal=True, code_sha256=hashlib.sha256(code['data']).hexdigest(), relocations=0, scope='Identified unmodified CRTcode,not reconstructed game function or acceptance.'))

def ratio(n, d, cw):
    exact = Fraction(n, d)
    value = struct.unpack('<f', struct.pack('<f', float(exact)))[0]
    bits = struct.unpack('<I', struct.pack('<f', value))[0]
    current = Fraction(value)
    mode = cw >> 10 & 3
    if mode in [1, 3] and current > exact:
        bits -= 1
    if mode == 2 and current < exact:
        bits += 1
    return Fraction(struct.unpack('<f', struct.pack('<I', bits))[0])

def execute(kind, b, s):
    pe = pefile.PE(str(ORACLE if kind == 'native' else Path(b['dll'])))
    base = pe.OPTIONAL_HEADER.ImageBase
    size = pe.OPTIONAL_HEADER.SizeOfImage
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(base, size + 4095 & ~4095)
    u.mem_write(base, pe.get_memory_mapped_image().ljust(size, b'\x00'))
    u.mem_map(ARENA, ARENA_SIZE)
    u.mem_map(STACK, 65536)
    entry = 4220208 if kind == 'native' else b['symbols']['_graphics_native_06530']
    width = 240 if kind == 'native' else b['raw']['compiled_size']
    ftol = 4263312 if kind == 'native' else b['symbols']['__ftol']
    before = bytes((i * 29 + (i >> 8) * 17 + 43 & 255 for i in range(ARENA_SIZE)))
    u.mem_write(ARENA, before)
    expected = bytearray(before)
    sw, sh, sp, dw, dh, dp = s.get('shape', [7, 5, 11, 13, 9, 17])
    src = s.get('source', SRC)
    dst = s.get('target', DST)
    cw = s.get('cw', 639)
    valid = src is not None and dst is not None and (sw >= 1) and (sh >= 1) and (sp >= sw) and (dw >= 1) and (dh >= 1) and (dp >= dw)
    if valid:
        assert 0 <= src and src + sp * sh < ARENA_SIZE and (0 <= dst) and (dst + dp * dh < ARENA_SIZE)
        sx, sy = (ratio(sw, dw, cw), ratio(sh, dh, cw))
        for row in range(dh):
            line = int(row * sy) * sp
            for col in range(dw):
                expected[dst + row * dp + col] = expected[src + line + int(col * sx)]
    args = [0 if src is None else ARENA + src, sw, sh, sp, 0 if dst is None else ARENA + dst, dw, dh, dp]
    u.mem_write(ESP - 4096, bytes([167]) * 4352)
    u.mem_write(ESP, pack(STOP) + b''.join((pack(x) for x in args)))
    u.reg_write(UC_X86_REG_ESP, ESP)
    u.reg_write(UC_X86_REG_FPCW, cw)
    u.reg_write(UC_X86_REG_FPTAG, 65535)
    for r, v in zip(NV, SEEDS):
        u.reg_write(r, v)
    instructions = 0
    illegal = []
    writes = []
    conversions = 0
    allowed = [] if not valid else [(ARENA + dst + row * dp, ARENA + dst + row * dp + dw) for row in range(dh)]

    def on_code(machine, a, w, user):
        nonlocal instructions, conversions
        instructions += 1
        if a == STOP:
            machine.emu_stop()
            return
        if a == ftol:
            conversions += 1
        if not (entry <= a < entry + width or ftol <= a < ftol + 39):
            illegal.append(hex(a))
            machine.emu_stop()

    def on_write(machine, access, a, w, v, user):
        if not (ESP - 4096 <= a and a + w <= ESP + 36 or any((lo <= a and a + w <= hi for lo, hi in allowed))):
            writes.append((hex(a), w))
            machine.emu_stop()
    u.hook_add(UC_HOOK_CODE, on_code)
    u.hook_add(UC_HOOK_MEM_WRITE, on_write)
    u.emu_start(entry, STOP, count=20000000)
    actual = bytes(u.mem_read(ARENA, ARENA_SIZE))
    checks = dict(whole_arena=actual == bytes(expected), sentinel=u.reg_read(UC_X86_REG_EIP) == STOP, stack=u.reg_read(UC_X86_REG_ESP) == ESP + 4, nonvolatiles=all((u.reg_read(r) == v for r, v in zip(NV, SEEDS))), control_word=u.reg_read(UC_X86_REG_FPCW) == cw, fpu_stack_empty=u.reg_read(UC_X86_REG_FPTAG) == 65535, conversion_count=conversions == (dh * (dw + 1) if valid else 0), stack_guards=bytes(u.mem_read(ESP - 4096, 32)) == bytes([167]) * 32 and bytes(u.mem_read(ESP + 36, 32)) == bytes([167]) * 32, execution_whitelist=not illegal, write_whitelist=not writes)
    assert all(checks.values()), (kind, s, checks, illegal, writes, conversions)
    return dict(checks=checks, arena_sha256=hashlib.sha256(actual).hexdigest(), conversions=conversions, control_word=u.reg_read(UC_X86_REG_FPCW), fpu_tag=u.reg_read(UC_X86_REG_FPTAG), instructions=instructions)

def main():
    b = build()
    scenarios = [dict(name='normal'), dict(name='null_source', source=None), dict(name='null_target', target=None)]
    for i in [0, 1, 2, 3, 4, 5]:
        shape = [7, 5, 11, 13, 9, 17]
        shape[i] = 0
        scenarios.append(dict(name='zero_guard_' + str(i), shape=shape))
    scenarios.extend([dict(name='negative_source_width', shape=[-1, 5, 11, 13, 9, 17]), dict(name='negative_target_height', shape=[7, 5, 11, 13, -1, 17]), dict(name='short_source_pitch', shape=[7, 5, 6, 13, 9, 17]), dict(name='short_target_pitch', shape=[7, 5, 11, 13, 9, 12]), dict(name='one_pixel', shape=[1, 1, 1, 1, 1, 1]), dict(name='identity_padded', shape=[17, 11, 23, 17, 11, 29]), dict(name='upscale', shape=[3, 2, 7, 17, 13, 23]), dict(name='downscale', shape=[31, 23, 37, 7, 5, 13]), dict(name='fractional_boundary', shape=[1, 1, 3, 49, 17, 53]), dict(name='self_copy', source=SRC, target=SRC, shape=[17, 11, 23, 17, 11, 23]), dict(name='forward_overlap', source=SRC, target=SRC + 3, shape=[17, 11, 23, 19, 9, 27]), dict(name='backward_overlap', source=SRC + 5, target=SRC, shape=[17, 11, 23, 13, 9, 21]), dict(name='game_frame', shape=[320, 240, 324, 320, 240, 336])])
    for cw in [895, 1663, 2687, 3711]:
        scenarios.append(dict(name='control_' + hex(cw), cw=cw, shape=[7, 5, 11, 49, 17, 53]))
    cases = []
    for s in scenarios:
        n = execute('native', b, s)
        c = execute('source', b, s)
        assert {k: v for k, v in n.items() if k != 'instructions'} == {k: v for k, v in c.items() if k != 'instructions'}, s
        cases.append(dict(scenario=s, native=n, source=c, equal=True))
    assert b['inputs'] == {p: sha(Path(p)) for p in b['inputs']}
    report = dict(scope='Actual original/fresh indexed copy and byte-identical CRTftol. Whole guarded arenas, overlap, padding and FPU control. No modeled conversion, real OS/process/renderer or whole-game proof.', build=b, cases=cases, case_count=len(cases), all_equal=True)
    p = OUT / 'bitmap_copy_differential.json'
    p.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(dict(cases=len(cases), all_equal=True, report=str(p)), indent=2))
if __name__ == '__main__':
    main()
